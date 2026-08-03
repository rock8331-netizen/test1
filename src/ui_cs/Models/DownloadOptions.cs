namespace YoutubeDownloader.Models
{
    public class DownloadOptions
    {
        public string Url { get; set; } = "";
        public string Quality { get; set; } = "best";
        public string OutputPath { get; set; } = "";
        public string Cookies { get; set; } = "none";
        public bool SaveSubtitles { get; set; }
        public bool ForceDownload { get; set; }
    }
}
