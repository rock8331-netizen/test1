/**
 * youtube_core.cpp
 * YoutubeDownloader v2.0 - C++ Core Engine
 *
 * 역할:
 *   - ffmpeg.exe 고속 탐색 (WinGet 패키지 경로, PATH, 로컬 폴더)
 *   - 디렉터리 파일 스캔 (용량 집계)
 *   - 바이트 → MB/GB 포맷팅
 *   - 다운로드 프로세스 강제 종료
 */

#include "../include/youtube_core.h"

#include <windows.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────
// 내부 헬퍼: PATH 환경변수에서 ffmpeg.exe 탐색
// ─────────────────────────────────────────────────────────
static bool FindInPath(char* out, int buf_size) {
    char path_env[32768] = {};
    DWORD len = GetEnvironmentVariableA("PATH", path_env, sizeof(path_env));
    if (len == 0) return false;

    char* ctx = nullptr;
    char* tok = strtok_s(path_env, ";", &ctx);
    while (tok) {
        std::string candidate = std::string(tok) + "\\ffmpeg.exe";
        if (GetFileAttributesA(candidate.c_str()) != INVALID_FILE_ATTRIBUTES) {
            // 파일 있으면 폴더 경로만 복사
            strncpy_s(out, buf_size, tok, _TRUNCATE);
            return true;
        }
        tok = strtok_s(nullptr, ";", &ctx);
    }
    return false;
}

// ─────────────────────────────────────────────────────────
// 내부 헬퍼: WinGet 패키지 폴더에서 ffmpeg.exe 탐색
// ─────────────────────────────────────────────────────────
static bool FindInWinGet(char* out, int buf_size) {
    char user_profile[MAX_PATH] = {};
    if (!GetEnvironmentVariableA("USERPROFILE", user_profile, MAX_PATH)) return false;

    std::string winget_base = std::string(user_profile)
        + "\\AppData\\Local\\Microsoft\\WinGet\\Packages";

    if (!fs::exists(winget_base)) return false;

    std::error_code ec;
    for (auto& entry : fs::recursive_directory_iterator(winget_base, ec)) {
        if (ec) break;
        if (entry.is_regular_file() && entry.path().filename() == "ffmpeg.exe") {
            std::string dir = entry.path().parent_path().string();
            strncpy_s(out, buf_size, dir.c_str(), _TRUNCATE);
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────
// Export: ffmpeg.exe 경로 탐색
// ─────────────────────────────────────────────────────────
YOUTUBE_API int FindFFmpegPath(char* out_path, int buf_size) {
    if (!out_path || buf_size <= 0) return 0;
    out_path[0] = '\0';

    // 1순위: PATH에서 탐색
    if (FindInPath(out_path, buf_size)) return 1;

    // 2순위: WinGet 패키지 폴더에서 탐색
    if (FindInWinGet(out_path, buf_size)) return 1;

    return 0;
}

// ─────────────────────────────────────────────────────────
// Export: 바이트 → MB / GB 포맷팅
// ─────────────────────────────────────────────────────────
YOUTUBE_API void FormatBytes(long long bytes, char* out_str, int buf_size) {
    if (!out_str || buf_size <= 0) return;

    constexpr long long GB = 1024LL * 1024 * 1024;
    constexpr long long MB = 1024LL * 1024;
    constexpr long long KB = 1024LL;

    if (bytes >= GB)
        snprintf(out_str, buf_size, "%.2f GB", (double)bytes / GB);
    else if (bytes >= MB)
        snprintf(out_str, buf_size, "%.2f MB", (double)bytes / MB);
    else if (bytes >= KB)
        snprintf(out_str, buf_size, "%.2f KB", (double)bytes / KB);
    else
        snprintf(out_str, buf_size, "%lld B", bytes);
}

// ─────────────────────────────────────────────────────────
// Export: 디렉터리 파일 스캔 (개수 + 총 바이트)
// ─────────────────────────────────────────────────────────
YOUTUBE_API int ScanDirectory(const char* dir_path, int* out_count, long long* out_total_bytes) {
    if (!dir_path || !out_count || !out_total_bytes) return 0;

    *out_count = 0;
    *out_total_bytes = 0;

    std::error_code ec;
    fs::path p(dir_path);
    if (!fs::exists(p, ec) || !fs::is_directory(p, ec)) return 0;

    for (auto& entry : fs::recursive_directory_iterator(p, ec)) {
        if (ec) break;
        if (entry.is_regular_file(ec) && !ec) {
            (*out_count)++;
            auto sz = entry.file_size(ec);
            if (!ec) *out_total_bytes += (long long)sz;
        }
    }
    return 1;
}

// ─────────────────────────────────────────────────────────
// Export: PID로 프로세스 강제 종료 (다운로드 중단용)
// ─────────────────────────────────────────────────────────
YOUTUBE_API int KillProcessById(int pid) {
    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
    if (!hProc) return 0;
    BOOL result = TerminateProcess(hProc, 1);
    CloseHandle(hProc);
    return result ? 1 : 0;
}
