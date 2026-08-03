#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace YoutubeCore {

struct DownloadOptions {
    std::string url;
    std::string quality = "best";
    std::string output_path;
    std::string cookies = "none";
    bool save_subtitles = false;
    bool force_download = false;
};

struct DownloadProgress {
    double percent = 0.0;
    std::string speed;
    std::string eta;
    std::string filename;
};

class BackendRunner {
public:
    BackendRunner();
    ~BackendRunner();

    static std::string FindPythonExe();
    static std::string FindBackendPyPath();
    static std::string GetDefaultOutputPath();

    bool Start(const DownloadOptions& opts,
               std::function<void(const DownloadProgress&)> on_progress,
               std::function<void(const std::string&)> on_log,
               std::function<void(bool success)> on_complete);

    void Stop();
    bool IsRunning() const { return m_running.load(); }

private:
    void WorkerThread(DownloadOptions opts,
                      std::function<void(const DownloadProgress&)> on_progress,
                      std::function<void(const std::string&)> on_log,
                      std::function<void(bool success)> on_complete);

    std::atomic<bool> m_running{false};
    HANDLE m_hProcess{NULL};
    DWORD m_pid{0};
};

} // namespace YoutubeCore
