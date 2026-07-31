using System.Windows;
using YoutubeDownloader.ViewModels;

namespace YoutubeDownloader
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            DataContext = new MainViewModel();
        }
    }
}
