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
        private string _actionButtonText = "다운로드 시작";
        private string _actionButtonBackground = "#E11D48";
        private string _statusSummary = "준비됨";

        public string StatusSummary
        {
            get => _statusSummary;
            set => SetProperty(ref _statusSummary, value);
        }

        public string Url
        {
            get => _url;
            set
            {
                if (SetProperty(ref _url, value))
                {
                    OnPropertyChanged(nameof(HasUrlText));
                }
            }
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
                    ActionButtonText = value ? "다운로드 중단" : "다운로드 시작";
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

        public bool HasUrlText => !string.IsNullOrWhiteSpace(Url);

        public ICommand StartOrStopDownloadCommand { get; }
        public ICommand PasteUrlCommand { get; }
        public ICommand ClearUrlCommand { get; }
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
            ClearUrlCommand = new RelayCommand(ExecuteClearUrl);
            BrowseOutputFolderCommand = new RelayCommand(ExecuteBrowseOutputFolder);
            OpenCompletedFolderCommand = new RelayCommand(ExecuteOpenCompletedFolder);
            ShowHelpCommand = new RelayCommand(ExecuteShowHelp);
            ClearLogCommand = new RelayCommand(ExecuteClearLog);
            CopyLogCommand = new RelayCommand(ExecuteCopyLog);

            AppendLog("[INIT] YouGrab v2.0 ready.");
            AppendLog($"[INIT] C++ Core DLL: {NativeInterop.GetDllStatus()}");
            AppendLog($"[INIT] Python Backend: {Path.GetFullPath(DownloadService.GetBackendPyPath())}");
        }

        private void ExecuteClearUrl()
        {
            Url = "";
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
            StatusSummary = "다운로드 중...";
            AppendLog($"[START] Target URL: {url}");
            AppendLog($"        Quality: {SelectedQuality} | Cookie: {SelectedCookie} | Subtitles: {SaveSubtitles} | Overwrite: {ForceDownload}");

            _cts = new CancellationTokenSource();

            var options = new YoutubeDownloader.Models.DownloadOptions
            {
                Url = url,
                Quality = SelectedQuality,
                OutputPath = OutputPath,
                Cookies = SelectedCookie,
                SaveSubtitles = SaveSubtitles,
                ForceDownload = ForceDownload
            };

            await _downloadService.StartDownloadAsync(
                options,
                (progress) =>
                {
                    Application.Current?.Dispatcher?.InvokeAsync(() =>
                    {
                        if (!string.IsNullOrEmpty(progress.Message))
                        {
                            StatusSummary = progress.Message;
                        }
                        else if (!string.IsNullOrEmpty(progress.Step))
                        {
                            switch (progress.Step)
                            {
                                case "parsing": StatusSummary = "URL 분석 중..."; break;
                                case "ffmpeg": StatusSummary = "FFmpeg 엔진 검증 중..."; break;
                                case "extracting": StatusSummary = "메타데이터 분석 중..."; break;
                                case "downloading": StatusSummary = $"{progress.Percent:F0}% 수신 중"; break;
                                case "merging": StatusSummary = "고화질 병합 작업 중..."; break;
                                case "subtitles": StatusSummary = "자막 파일 처리 중..."; break;
                                case "cleaning": StatusSummary = "임시 파일 청소 중..."; break;
                                case "completed": StatusSummary = "저장 완료"; break;
                                case "skipped_archive":
                                    StatusSummary = "이전 기록 존재";
                                    ProgressFile = $"[이미 받았던 기록이 있는 파일] {progress.Filename}";
                                    ProgressMeta = "이미 받았던 기록이 있는 파일입니다. (새로 받으려면 '새로 받기' 선택)";
                                    ProgressPct = 100.0;
                                    ProgressPctText = "100.0%";
                                    IsProgressVisible = true;
                                    break;
                            }
                        }

                        if (progress.Step == "downloading" || (progress.Percent > 0 && progress.Step != "skipped_archive"))
                        {
                            IsProgressVisible = true;
                            ProgressPct = progress.Percent;
                            ProgressPctText = $"{progress.Percent:F1}%";
                            ProgressFile = string.IsNullOrEmpty(progress.Filename) ? ProgressFile : progress.Filename;
                            ProgressMeta = string.IsNullOrEmpty(progress.Speed) ? "" : $"{progress.Speed} | ETA {progress.Eta}";
                        }
                    });
                },
                (msg) => AppendLog(msg),
                _cts.Token
            );

            IsDownloading = false;
            ProgressPct = 100.0;
            ProgressPctText = "100.0%";
            ProgressMeta = "수신 완료 및 저장 성공";
            StatusSummary = "다운로드 완료 (저장 완료)";
        }

        private void StopDownload()
        {
            _cts?.Cancel();
            _downloadService.StopDownload();
            AppendLog("[STOP] Download process terminated by user.");
            IsDownloading = false;
            StatusSummary = "중단됨";
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
            catch { }
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
