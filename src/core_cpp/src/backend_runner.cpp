#include "../include/backend_runner.h"
#include <shlwapi.h>
#include <iostream>
#include <sstream>
#include <vector>

namespace YoutubeCore {

BackendRunner::BackendRunner() {}

BackendRunner::~BackendRunner() {
    Stop();
}

std::string BackendRunner::FindPythonExe() {
    char user_profile[MAX_PATH] = {};
    GetEnvironmentVariableA("USERPROFILE", user_profile, MAX_PATH);

    std::vector<std::string> candidates = {
        std::string(user_profile) + "\\AppData\\Local\\Python\\bin\\python.exe",
        std::string(user_profile) + "\\AppData\\Local\\hermes\\hermes-agent\\venv\\Scripts\\python.exe",
        std::string(user_profile) + "\\AppData\\Local\\Programs\\Python\\Python312\\python.exe",
        std::string(user_profile) + "\\AppData\\Local\\Programs\\Python\\Python311\\python.exe",
        std::string(user_profile) + "\\AppData\\Local\\Programs\\Python\\Python310\\python.exe",
        "python.exe"
    };

    for (const auto& path : candidates) {
        if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }
    return "python.exe";
}

std::string BackendRunner::FindBackendPyPath() {
    char exe_path[MAX_PATH] = {};
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    PathRemoveFileSpecA(exe_path); // Base dir

    std::vector<std::string> candidates = {
        std::string(exe_path) + "\\analytics_py\\yt_backend.py",
        std::string(exe_path) + "\\..\\src\\analytics_py\\yt_backend.py",
        std::string(exe_path) + "\\..\\..\\src\\analytics_py\\yt_backend.py",
        std::string(exe_path) + "\\..\\..\\..\\src\\analytics_py\\yt_backend.py"
    };

    for (const auto& path : candidates) {
        char full[MAX_PATH] = {};
        if (GetFullPathNameA(path.c_str(), MAX_PATH, full, NULL)) {
            if (GetFileAttributesA(full) != INVALID_FILE_ATTRIBUTES) {
                return std::string(full);
            }
        }
    }
    return candidates[0];
}

std::string BackendRunner::GetDefaultOutputPath() {
    char exe_path[MAX_PATH] = {};
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    PathRemoveFileSpecA(exe_path);
    std::string out = std::string(exe_path) + "\\downloads";
    CreateDirectoryA(out.c_str(), NULL);
    return out;
}

bool BackendRunner::Start(const DownloadOptions& opts,
                           std::function<void(const DownloadProgress&)> on_progress,
                           std::function<void(const std::string&)> on_log,
                           std::function<void(bool success)> on_complete) {
    if (m_running.load()) return false;
    m_running.store(true);

    std::thread t(&BackendRunner::WorkerThread, this, opts, on_progress, on_log, on_complete);
    t.detach();
    return true;
}

void BackendRunner::Stop() {
    if (!m_running.load()) return;
    m_running.store(false);

    if (m_hProcess != NULL) {
        TerminateProcess(m_hProcess, 1);
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
    }
}

static std::string ExtractJsonField(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\":";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return "";
    pos += pattern.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.length()) return "";

    if (json[pos] == '"') {
        size_t end_pos = json.find('"', pos + 1);
        if (end_pos != std::string::npos) {
            return json.substr(pos + 1, end_pos - pos - 1);
        }
    } else {
        size_t end_pos = json.find_first_of(",}\r\n", pos);
        if (end_pos != std::string::npos) {
            return json.substr(pos, end_pos - pos);
        }
    }
    return "";
}

void BackendRunner::WorkerThread(DownloadOptions opts,
                                  std::function<void(const DownloadProgress&)> on_progress,
                                  std::function<void(const std::string&)> on_log,
                                  std::function<void(bool success)> on_complete) {
    std::string py = FindPythonExe();
    std::string script = FindBackendPyPath();

    std::ostringstream cmd;
    cmd << "\"" << py << "\" \"" << script << "\" \"" << opts.url << "\" "
        << "--output \"" << opts.output_path << "\" "
        << "--quality " << opts.quality << " "
        << "--cookies " << opts.cookies << " ";
    if (opts.save_subtitles) cmd << "--save-sub ";
    if (opts.force_download) cmd << "--ignore-archive ";

    std::string cmd_str = cmd.str();

    HANDLE hReadPipe = NULL, hWritePipe = NULL;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        if (on_log) on_log("❌ [오류] 파이프 생성 실패");
        m_running.store(false);
        if (on_complete) on_complete(false);
        return;
    }
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    char* cmd_buf = _strdup(cmd_str.c_str());

    BOOL ok = CreateProcessA(NULL, cmd_buf, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    free(cmd_buf);
    CloseHandle(hWritePipe);

    if (!ok) {
        if (on_log) on_log("❌ [오류] Python 백엔드 프로세스 실행 실패");
        CloseHandle(hReadPipe);
        m_running.store(false);
        if (on_complete) on_complete(false);
        return;
    }

    m_hProcess = pi.hProcess;
    m_pid = pi.dwProcessId;
    CloseHandle(pi.hThread);

    char buffer[4096];
    std::string line_buf;
    DWORD bytes_read = 0;
    bool success = true;

    while (m_running.load() && ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytes_read, NULL) && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        line_buf.append(buffer);

        size_t pos = 0;
        while ((pos = line_buf.find('\n')) != std::string::npos) {
            std::string line = line_buf.substr(0, pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            line_buf.erase(0, pos + 1);

            if (line.empty()) continue;

            if (line[0] == '{') {
                std::string type = ExtractJsonField(line, "type");
                if (type == "progress" || type == "download_progress") {
                    std::string pct_s = ExtractJsonField(line, "percent");
                    std::string fname = ExtractJsonField(line, "filename");
                    std::string speed = ExtractJsonField(line, "speed");
                    std::string eta   = ExtractJsonField(line, "eta");

                    DownloadProgress p;
                    p.percent = pct_s.empty() ? 0.0 : std::atof(pct_s.c_str());
                    p.filename = fname;
                    p.speed = speed;
                    p.eta = eta;

                    if (on_progress) on_progress(p);
                } else if (type == "info" || type == "log") {
                    std::string msg = ExtractJsonField(line, "msg");
                    if (msg.empty()) msg = ExtractJsonField(line, "message");
                    if (!msg.empty() && on_log) on_log(msg);
                } else if (type == "error") {
                    std::string err = ExtractJsonField(line, "msg");
                    if (err.empty()) err = ExtractJsonField(line, "message");
                    if (on_log) on_log("❌ [오류] " + err);
                    success = false;
                } else if (type == "done") {
                    if (on_log) on_log("✅ [완료] 다운로드가 성공적으로 종료되었습니다.");
                }
            } else {
                if (on_log) on_log(line);
            }
        }
    }

    WaitForSingleObject(m_hProcess, 1000);
    CloseHandle(hReadPipe);
    CloseHandle(m_hProcess);
    m_hProcess = NULL;
    m_running.store(false);

    if (on_complete) on_complete(success);
}

} // namespace YoutubeCore
