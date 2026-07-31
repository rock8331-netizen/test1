using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace YoutubeDownloader
{
    public partial class MainWindow : Window
    {
        // ── Python 백엔드 경로 ─────────────────────────────────
        private static readonly string BackendPy = Path.Combine(
            AppDomain.CurrentDomain.BaseDirectory,
            "..", "..", "analytics_py", "yt_backend.py");

        // ── 기본 저장 경로 ─────────────────────────────────────
        private static readonly string DefaultOutput = Path.Combine(
            AppDomain.CurrentDomain.BaseDirectory,
            "..", "..", "..", "data", "downloads");

        // ── 현재 실행 중인 다운로드 프로세스 ──────────────────
        private Process? _downloadProcess;
        private CancellationTokenSource? _cts;
        private bool _isRunning = false;

        public MainWindow()
        {
            InitializeComponent();
            TxtOutput.Text = Path.GetFullPath(DefaultOutput);
            AppendLog("✅ YouTube Downloader v2.0 준비 완료");
            AppendLog($"🔧 C++ Core DLL: {NativeInterop.GetDllStatus()}");
            AppendLog($"🐍 Python 백엔드: {Path.GetFullPath(BackendPy)}");
        }

        // ── 로그 출력 ─────────────────────────────────────────
        private void AppendLog(string msg)
        {
            Dispatcher.InvokeAsync(() =>
            {
                string time = DateTime.Now.ToString("HH:mm:ss");
                TxtLog.AppendText($"[{time}] {msg}{Environment.NewLine}");
                TxtLog.ScrollToEnd();
            });
        }

        // ── 진행률 UI 업데이트 ─────────────────────────────────
        private void UpdateProgress(double pct, string filename, string speed, string eta)
        {
            Dispatcher.InvokeAsync(() =>
            {
                ProgressPanel.Visibility = Visibility.Visible;
                PbProgress.Value = pct;
                TxtProgressPct.Text  = $"{pct:F1}%";
                TxtProgressFile.Text = filename;
                TxtProgressMeta.Text = string.IsNullOrEmpty(speed) ? "" : $"⚡ {speed}  ⏱ ETA {eta}";
            });
        }

        // ── 완료/초기 상태 리셋 ────────────────────────────────
        private void ResetRunningState()
        {
            Dispatcher.InvokeAsync(() =>
            {
                _isRunning = false;
                BtnAction.Content    = "🚀  다운로드 시작";
                BtnAction.IsEnabled  = true;
                BtnAction.Background = (Brush)FindResource("AccentRedBrush");
            });
        }

        // ── 붙여넣기 버튼 ──────────────────────────────────────
        private void BtnPaste_Click(object sender, RoutedEventArgs e)
        {
            try { TxtUrl.Text = Clipboard.GetText().Trim(); } catch { }
        }

        // ── 찾아보기 버튼 ──────────────────────────────────────
        private void BtnBrowse_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new System.Windows.Forms.FolderBrowserDialog
            {
                SelectedPath = TxtOutput.Text,
                Description  = "저장 폴더를 선택하세요",
                ShowNewFolderButton = true
            };
            if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                TxtOutput.Text = dlg.SelectedPath;
        }

        // ── 완료 폴더 열기 버튼 ───────────────────────────────
        private void BtnOpenFolder_Click(object sender, RoutedEventArgs e)
        {
            string folder = TxtOutput.Text;
            Directory.CreateDirectory(folder);
            Process.Start("explorer.exe", folder);
        }

        // ── 로그 클리어 ───────────────────────────────────────
        private void BtnClearLog_Click(object sender, RoutedEventArgs e) => TxtLog.Clear();

        // ── 로그 클립보드 복사 ────────────────────────────────
        private void BtnCopyLog_Click(object sender, RoutedEventArgs e)
        {
            if (!string.IsNullOrEmpty(TxtLog.Text))
                Clipboard.SetText(TxtLog.Text);
        }

        // ── 선택된 화질 태그 반환 ─────────────────────────────
        private string GetSelectedQuality()
        {
            var panel = QualityPanel;
            foreach (RadioButton rb in panel.Children)
                if (rb.IsChecked == true)
                    return rb.Tag?.ToString() ?? "best";
            return "best";
        }

        // ── 메인 액션 버튼 (시작 / 중단 토글) ────────────────
        private void BtnAction_Click(object sender, RoutedEventArgs e)
        {
            if (!_isRunning)
                StartDownload();
            else
                StopDownload();
        }

        // ── 다운로드 시작 ─────────────────────────────────────
        private async void StartDownload()
        {
            string url = TxtUrl.Text.Trim();
            if (string.IsNullOrEmpty(url))
            {
                MessageBox.Show("유튜브 URL을 입력해 주세요.", "입력 오류",
                                MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            _isRunning = true;
            BtnAction.Content    = "🛑  중단";
            BtnAction.Background = new SolidColorBrush(Color.FromRgb(0x88, 0x00, 0x00));

            string quality = GetSelectedQuality();
            string output  = TxtOutput.Text;
            string cookies = (CboCookie.SelectedItem as ComboBoxItem)?.Content?.ToString() ?? "none";
            bool   saveSub = ChkSub.IsChecked == true;
            bool   force   = ChkForce.IsChecked == true;

            AppendLog($"\n🚀 [시작] URL: {url}");
            AppendLog($"   화질: {quality} | 쿠키: {cookies} | 자막: {saveSub} | 강제: {force}");

            _cts = new CancellationTokenSource();
            await Task.Run(() => RunPythonBackend(url, quality, output, cookies, saveSub, force, _cts.Token));
            ResetRunningState();
        }

        // ── 다운로드 중단 ─────────────────────────────────────
        private void StopDownload()
        {
            _cts?.Cancel();
            try
            {
                if (_downloadProcess != null && !_downloadProcess.HasExited)
                {
                    // C++ DLL로 프로세스 강제 종료
                    NativeInterop.KillProcessById(_downloadProcess.Id);
                    _downloadProcess.Kill(true);
                }
            }
            catch { }

            BtnAction.Content   = "⏳  중단 중...";
            BtnAction.IsEnabled = false;
            AppendLog("🛑 [중단 요청] 다운로드를 중단합니다...");
        }

        // ── Python 백엔드 실행 및 JSON Lines 수신 ────────────
        private void RunPythonBackend(string url, string quality, string output,
            string cookies, bool saveSub, bool force, CancellationToken ct)
        {
            string pyPath     = Path.GetFullPath(BackendPy);
            string? pythonExe = FindPython();

            if (pythonExe == null)
            {
                AppendLog("❌ Python 실행 파일을 찾을 수 없습니다.");
                return;
            }

            var args = $"\"{pyPath}\" \"{url}\" " +
                       $"--output \"{output}\" " +
                       $"--quality {quality} " +
                       $"--cookies {cookies} " +
                       (saveSub ? "--save-sub " : "") +
                       (force   ? "--ignore-archive" : "");

            var psi = new ProcessStartInfo
            {
                FileName               = pythonExe,
                Arguments              = args,
                UseShellExecute        = false,
                RedirectStandardOutput = true,
                RedirectStandardError  = true,
                StandardOutputEncoding = System.Text.Encoding.UTF8,
                CreateNoWindow         = true,
            };

            _downloadProcess = new Process { StartInfo = psi };
            _downloadProcess.Start();

            // stdout: JSON Lines 파싱
            string? line;
            while ((line = _downloadProcess.StandardOutput.ReadLine()) != null)
            {
                if (ct.IsCancellationRequested) break;
                ParseJsonLine(line);
            }

            // stderr: 직접 로그
            string err = _downloadProcess.StandardError.ReadToEnd();
            if (!string.IsNullOrWhiteSpace(err))
                AppendLog($"⚠️ {err.Trim()}");

            if (!_downloadProcess.HasExited)
                _downloadProcess.WaitForExit(3000);
        }

        // ── JSON Lines 파싱 ───────────────────────────────────
        private void ParseJsonLine(string line)
        {
            if (string.IsNullOrWhiteSpace(line)) return;
            try
            {
                using var doc = JsonDocument.Parse(line);
                var root = doc.RootElement;
                string type = root.GetProperty("type").GetString() ?? "";

                switch (type)
                {
                    case "info":
                        AppendLog(root.GetProperty("msg").GetString() ?? "");
                        break;

                    case "progress":
                        double pct  = root.TryGetProperty("percent",  out var p)   ? p.GetDouble() : 0;
                        string spd  = root.TryGetProperty("speed",    out var sp)  ? sp.GetString() ?? "" : "";
                        string eta  = root.TryGetProperty("eta",      out var e)   ? e.GetString()  ?? "" : "";
                        string file = root.TryGetProperty("filename", out var fn)  ? fn.GetString() ?? "" : "";
                        UpdateProgress(pct, file, spd, eta);
                        break;

                    case "done":
                        string status = root.TryGetProperty("status", out var st) ? st.GetString() ?? "" : "";
                        int    files  = root.TryGetProperty("files",  out var fc) ? fc.GetInt32()  : 0;
                        long   bytes  = root.TryGetProperty("total_bytes", out var tb) ? tb.GetInt64() : 0;

                        char[] buf = new char[32];
                        string sizeStr = NativeInterop.FormatBytes(bytes);
                        string icon    = status == "completed" ? "✅" : "⚠️";
                        AppendLog($"\n{icon} [{status.ToUpper()}] 파일 {files}개 / {sizeStr}");

                        Dispatcher.InvokeAsync(() =>
                        {
                            PbProgress.Value    = 100;
                            TxtProgressPct.Text = "100%";
                        });
                        break;

                    case "error":
                        AppendLog($"❌ {root.GetProperty("msg").GetString()}");
                        break;
                }
            }
            catch
            {
                // JSON이 아닌 일반 출력은 그냥 표시
                AppendLog(line);
            }
        }

        // ── Python 실행 파일 탐색 ─────────────────────────────
        private static string? FindPython()
        {
            // 명시적 알려진 경로 우선 탐색
            string[] knownPaths = {
                @"C:\Users\user\AppData\Local\Python\bin\python.exe",
                @"C:\Users\user\AppData\Local\Programs\Python\Python314\python.exe",
                @"C:\Users\user\AppData\Local\Programs\Python\Python311\python.exe",
                @"C:\Users\user\anaconda3\python.exe",
                @"C:\Users\user\miniconda3\python.exe",
            };
            foreach (string p in knownPaths)
                if (File.Exists(p)) return p;

            // PATH 에서 탐색
            foreach (string name in new[] { "python", "python3" })
            {
                string? found = FindExecutable(name);
                if (found != null) return found;
            }
            return null;
        }

        private static string? FindExecutable(string name)
        {
            string[] paths = (Environment.GetEnvironmentVariable("PATH") ?? "")
                             .Split(';', StringSplitOptions.RemoveEmptyEntries);
            foreach (string dir in paths)
            {
                string full = Path.Combine(dir, name + ".exe");
                if (File.Exists(full)) return full;
            }
            return null;
        }
    }
}
