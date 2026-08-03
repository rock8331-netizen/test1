namespace YoutubeDownloader.Models
{
    public class DownloadProgress
    {
        public double Percent { get; set; }
        public string Speed { get; set; } = "";
        public string Eta { get; set; } = "";
        public string Filename { get; set; } = "";
        public string Step { get; set; } = "";
        public string Message { get; set; } = "";
    }
}
