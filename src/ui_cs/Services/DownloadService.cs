using System;
using System.Diagnostics;
using System.IO;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using YoutubeDownloader.Models;

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

            // Extract embedded yt_backend.py for standalone Single-File execution
            string tempDir = Path.Combine(Path.GetTempPath(), "YoutubeDownloader");
            Directory.CreateDirectory(tempDir);
            string tempPyPath = Path.Combine(tempDir, "yt_backend.py");
            try
            {
                var assembly = System.Reflection.Assembly.GetExecutingAssembly();
                using Stream? stream = assembly.GetManifestResourceStream("YoutubeDownloader.Resources.yt_backend.py")
                                     ?? assembly.GetManifestResourceStream("YoutubeDownloader.yt_backend.py");
                if (stream != null)
                {
                    using FileStream fs = new FileStream(tempPyPath, FileMode.Create, FileAccess.Write);
                    stream.CopyTo(fs);
                    return tempPyPath;
                }
            }
            catch { }

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
                @"C:\Users\user\AppData\Local\Python\bin\python.exe",
                @"C:\Users\user\AppData\Local\hermes\hermes-agent\venv\Scripts\python.exe",
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
            DownloadOptions options,
            Action<DownloadProgress> onProgress,
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

            var args = $"\"{pyPath}\" \"{options.Url}\" " +
                       $"--output \"{options.OutputPath}\" " +
                       $"--quality {options.Quality} " +
                       $"--cookies {options.Cookies} " +
                       (options.SaveSubtitles ? "--save-sub " : "") +
                       (options.ForceDownload ? "--ignore-archive" : "");

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

                                if (type == "stage")
                                {
                                    string step = root.TryGetProperty("step", out var st) ? st.GetString() ?? "" : "";
                                    string msg = root.TryGetProperty("msg", out var sm) ? sm.GetString() ?? "" : "";
                                    onProgress(new DownloadProgress { Step = step, Message = msg });
                                }
                                else if (type == "download_progress" || type == "progress")
                                {
                                    double pct = root.GetProperty("percent").GetDouble();
                                    string fname = root.TryGetProperty("filename", out var f) ? f.GetString() ?? "" : "";
                                    string speed = root.TryGetProperty("speed", out var s) ? s.GetString() ?? "" : "";
                                    string eta = root.TryGetProperty("eta", out var e) ? e.GetString() ?? "" : "";

                                    onProgress(new DownloadProgress
                                    {
                                        Percent = pct,
                                        Filename = fname,
                                        Speed = speed,
                                        Eta = eta,
                                        Step = "downloading"
                                    });
                                }
                                else if (type == "log" || type == "info")
                                {
                                    string msg = root.TryGetProperty("message", out var m) ? (m.GetString() ?? "") : (root.TryGetProperty("msg", out var m2) ? (m2.GetString() ?? "") : "");
                                    if (!string.IsNullOrEmpty(msg)) onLog(msg);
                                }
                                else if (type == "download_complete" || type == "done")
                                {
                                    onLog("[SUCCESS] Download pipeline completed.");
                                }
                                else if (type == "error")
                                {
                                    string err = root.TryGetProperty("message", out var m) ? (m.GetString() ?? "") : (root.TryGetProperty("msg", out var m2) ? (m2.GetString() ?? "") : "");
                                    onLog($"[ERROR] {err}");
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
