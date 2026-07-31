using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Windows;
using System.Windows.Input;
using YoutubeDownloader.MVVM.Base;
using YoutubeDownloader.Services;

namespace YoutubeDownloader.ViewModels
{
    public class MainViewModel : ObservableObject
    {
        private readonly DownloadService _downloadService;
        private CancellationTokenSource? _cts;

        private string _url = "";
        private string _selectedQuality = "best";
        private string _selectedCookie = "none";
        private bool _saveSubtitles;
        private bool _forceDownload;
        private string _outputPath = "";

        private double _progressPct;
        private string _progressPctText = "0.0%";
        private string _progressFile = "";
        private string _progressMeta = "";
        private bool _isProgressVisible;

        private string _logText = "";
        private bool _isDownloading;
        private string _actionButtonText = "🚀   다운로드 시작";
        private string _actionButtonBackground = "#E11D48";

        public string Url
        {
            get => _url;
            set => SetProperty(ref _url, value);
        }

        public string SelectedQuality
        {
            get => _selectedQuality;
            set => SetProperty(ref _selectedQuality, value);
        }

        public string SelectedCookie
        {
            get => _selectedCookie;
            set => SetProperty(ref _selectedCookie, value);
        }

        public bool SaveSubtitles
        {
            get => _saveSubtitles;
            set => SetProperty(ref _saveSubtitles, value);
        }

        public bool ForceDownload
        {
            get => _forceDownload;
            set => SetProperty(ref _forceDownload, value);
        }

        public string OutputPath
        {
            get => _outputPath;
            set => SetProperty(ref _outputPath, value);
        }

        public double ProgressPct
        {
            get => _progressPct;
            set => SetProperty(ref _progressPct, value);
        }

        public string ProgressPctText
        {
            get => _progressPctText;
            set => SetProperty(ref _progressPctText, value);
        }

        public string ProgressFile
        {
            get => _progressFile;
            set => SetProperty(ref _progressFile, value);
        }

        public string ProgressMeta
        {
            get => _progressMeta;
            set => SetProperty(ref _progressMeta, value);
        }

        public bool IsProgressVisible
        {
            get => _isProgressVisible;
            set => SetProperty(ref _isProgressVisible, value);
        }

        public string LogText
        {
            get => _logText;
            set => SetProperty(ref _logText, value);
        }

        public bool IsDownloading
        {
            get => _isDownloading;
            set
            {
                if (SetProperty(ref _isDownloading, value))
                {
                    ActionButtonText = value ? "🛑   중단" : "🚀   다운로드 시작";
                    ActionButtonBackground = value ? "#99001B" : "#E11D48";
                }
            }
        }

        public string ActionButtonText
        {
            get => _actionButtonText;
            set => SetProperty(ref _actionButtonText, value);
        }

        public string ActionButtonBackground
        {
            get => _actionButtonBackground;
            set => SetProperty(ref _actionButtonBackground, value);
        }

        public ICommand StartOrStopDownloadCommand { get; }
        public ICommand PasteUrlCommand { get; }
        public ICommand BrowseOutputFolderCommand { get; }
        public ICommand OpenCompletedFolderCommand { get; }
        public ICommand ShowHelpCommand { get; }
        public ICommand ClearLogCommand { get; }
        public ICommand CopyLogCommand { get; }

        public MainViewModel()
        {
            _downloadService = new DownloadService();
            _outputPath = Path.GetFullPath(DownloadService.GetDefaultOutputPath());

            StartOrStopDownloadCommand = new RelayCommand(ExecuteStartOrStopDownload);
            PasteUrlCommand = new RelayCommand(ExecutePasteUrl);
            BrowseOutputFolderCommand = new RelayCommand(ExecuteBrowseOutputFolder);
            OpenCompletedFolderCommand = new RelayCommand(ExecuteOpenCompletedFolder);
            ShowHelpCommand = new RelayCommand(ExecuteShowHelp);
            ClearLogCommand = new RelayCommand(ExecuteClearLog);
            CopyLogCommand = new RelayCommand(ExecuteCopyLog);

            AppendLog("✅ YouTube Downloader v2.0 준비 완료");
            AppendLog($"🔧 C++ Core DLL: {NativeInterop.GetDllStatus()}");
            AppendLog($"🐍 Python 백엔드: {Path.GetFullPath(DownloadService.GetBackendPyPath())}");
        }

        public void AppendLog(string message)
        {
            Application.Current?.Dispatcher?.InvokeAsync(() =>
            {
                string time = DateTime.Now.ToString("HH:mm:ss");
                LogText += $"[{time}] {message}{Environment.NewLine}";
            });
        }

        private void ExecuteStartOrStopDownload()
        {
            if (!IsDownloading)
                StartDownload();
            else
                StopDownload();
        }

        private async void StartDownload()
        {
            string url = Url.Trim();
            if (string.IsNullOrEmpty(url))
            {
                MessageBox.Show("유튜브 URL을 입력해 주세요.", "입력 오류",
                                MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            IsDownloading = true;
            AppendLog($"\n🚀 [시작] URL: {url}");
            AppendLog($"   화질: {SelectedQuality} | 쿠키: {SelectedCookie} | 자막: {SaveSubtitles} | 강제: {ForceDownload}");

            _cts = new CancellationTokenSource();

            await _downloadService.StartDownloadAsync(
                url,
                SelectedQuality,
                OutputPath,
                SelectedCookie,
                SaveSubtitles,
                ForceDownload,
                (pct, file, speed, eta) =>
                {
                    Application.Current?.Dispatcher?.InvokeAsync(() =>
                    {
                        IsProgressVisible = true;
                        ProgressPct = pct;
                        ProgressPctText = $"{pct:F1}%";
                        ProgressFile = file;
                        ProgressMeta = string.IsNullOrEmpty(speed) ? "" : $"⚡ {speed}  ⏱ ETA {eta}";
                    });
                },
                (msg) => AppendLog(msg),
                _cts.Token
            );

            IsDownloading = false;
        }

        private void StopDownload()
        {
            _cts?.Cancel();
            _downloadService.StopDownload();
            AppendLog("🛑 [중단] 다운로드가 중단되었습니다.");
            IsDownloading = false;
        }

        private void ExecutePasteUrl()
        {
            try
            {
                Url = Clipboard.GetText().Trim();
            }
            catch { }
        }

        private void ExecuteBrowseOutputFolder()
        {
            try
            {
                var dlg = new Microsoft.Win32.OpenFolderDialog
                {
                    InitialDirectory = Directory.Exists(OutputPath) ? OutputPath : "",
                    Title = "저장 폴더를 선택하세요"
                };
                if (dlg.ShowDialog() == true)
                    OutputPath = dlg.FolderName;
            }
            catch
            {
                var dlg = new System.Windows.Forms.FolderBrowserDialog
                {
                    SelectedPath = OutputPath,
                    Description = "저장 폴더를 선택하세요"
                };
                if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                    OutputPath = dlg.SelectedPath;
            }
        }

        private void ExecuteOpenCompletedFolder()
        {
            Directory.CreateDirectory(OutputPath);
            Process.Start("explorer.exe", OutputPath);
        }

        private void ExecuteShowHelp()
        {
            var mainWindow = Application.Current.MainWindow;
            var helpWindow = new HelpWindow { Owner = mainWindow };
            helpWindow.ShowDialog();
        }

        private void ExecuteClearLog()
        {
            LogText = "";
        }

        private void ExecuteCopyLog()
        {
            if (!string.IsNullOrEmpty(LogText))
                Clipboard.SetText(LogText);
        }
    }
}
