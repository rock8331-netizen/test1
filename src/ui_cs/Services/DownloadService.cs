using System;
using System.Diagnostics;
using System.IO;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace YoutubeDownloader.Services
{
    public class DownloadService
    {
        private Process? _currentProcess;

        public static string GetBackendPyPath()
        {
            string baseDir = AppDomain.CurrentDomain.BaseDirectory;
            string[] candidates = {
                Path.Combine(baseDir, "analytics_py", "yt_backend.py"),
                Path.GetFullPath(Path.Combine(baseDir, "..", "src", "analytics_py", "yt_backend.py")),
                Path.GetFullPath(Path.Combine(baseDir, "..", "..", "src", "analytics_py", "yt_backend.py")),
                Path.GetFullPath(Path.Combine(baseDir, "..", "..", "..", "src", "analytics_py", "yt_backend.py"))
            };
            foreach (string p in candidates)
            {
                if (File.Exists(p)) return p;
            }
            return candidates[0];
        }

        public static string GetDefaultOutputPath()
        {
            string baseDir = AppDomain.CurrentDomain.BaseDirectory;
            string p = Path.GetFullPath(Path.Combine(baseDir, "downloads"));
            Directory.CreateDirectory(p);
            return p;
        }

        private static string? FindPython()
        {
            string[] candidates = {
                @"C:\Users\user\AppData\Local\Programs\Python\Python312\python.exe",
                @"C:\Users\user\AppData\Local\Programs\Python\Python311\python.exe",
                @"C:\Users\user\AppData\Local\Programs\Python\Python310\python.exe",
                "python.exe"
            };
            foreach (string p in candidates)
            {
                try
                {
                    if (File.Exists(p)) return p;
                }
                catch { }
            }
            return "python.exe";
        }

        public async Task StartDownloadAsync(
            string url,
            string quality,
            string output,
            string cookies,
            bool saveSubtitles,
            bool forceDownload,
            Action<double, string, string, string> onProgress,
            Action<string> onLog,
            CancellationToken ct)
        {
            string pyPath = Path.GetFullPath(GetBackendPyPath());
            string? pythonExe = FindPython();

            if (pythonExe == null)
            {
                onLog("❌ Python 실행 파일을 찾을 수 없습니다.");
                return;
            }

            var args = $"\"{pyPath}\" \"{url}\" " +
                       $"--output \"{output}\" " +
                       $"--quality {quality} " +
                       $"--cookies {cookies} " +
                       (saveSubtitles ? "--save-sub " : "") +
                       (forceDownload ? "--ignore-archive" : "");

            var psi = new ProcessStartInfo
            {
                FileName = pythonExe,
                Arguments = args,
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = System.Text.Encoding.UTF8,
                CreateNoWindow = true,
            };

            await Task.Run(() =>
            {
                try
                {
                    _currentProcess = new Process { StartInfo = psi };
                    _currentProcess.Start();

                    string? line;
                    while (!ct.IsCancellationRequested &&
                           _currentProcess != null &&
                           !_currentProcess.HasExited &&
                           (line = _currentProcess.StandardOutput.ReadLine()) != null)
                    {
                        if (string.IsNullOrWhiteSpace(line)) continue;

                        if (line.TrimStart().StartsWith("{"))
                        {
                            try
                            {
                                using var doc = JsonDocument.Parse(line);
                                var root = doc.RootElement;
                                string type = root.GetProperty("type").GetString() ?? "";

                                if (type == "download_progress")
                                {
                                    double pct = root.GetProperty("percent").GetDouble();
                                    string fname = root.GetProperty("filename").GetString() ?? "";
                                    string speed = root.GetProperty("speed").GetString() ?? "";
                                    string eta = root.GetProperty("eta").GetString() ?? "";

                                    onProgress(pct, fname, speed, eta);
                                }
                                else if (type == "log")
                                {
                                    string msg = root.GetProperty("message").GetString() ?? "";
                                    onLog(msg);
                                }
                                else if (type == "download_complete")
                                {
                                    onLog("✅ [완료] 다운로드가 성공적으로 끝났습니다.");
                                }
                                else if (type == "error")
                                {
                                    string err = root.GetProperty("message").GetString() ?? "";
                                    onLog($"❌ [오류] {err}");
                                }
                            }
                            catch
                            {
                                onLog(line);
                            }
                        }
                        else
                        {
                            onLog(line);
                        }
                    }

                    if (_currentProcess != null && !_currentProcess.HasExited)
                        _currentProcess.WaitForExit(2000);
                }
                catch (Exception ex)
                {
                    onLog($"❌ [예외] {ex.Message}");
                }
                finally
                {
                    StopDownload();
                }
            }, ct);
        }

        public void StopDownload()
        {
            try
            {
                if (_currentProcess != null && !_currentProcess.HasExited)
                {
                    NativeInterop.KillProcessById(_currentProcess.Id);
                    _currentProcess.Kill(true);
                    _currentProcess.Dispose();
                }
            }
            catch { }
            finally
            {
                _currentProcess = null;
            }
        }
    }
}
