using System.Windows;
using YoutubeDownloader.ViewModels;

namespace YoutubeDownloader
{
    public partial class HelpWindow : Window
    {
        public HelpWindow()
        {
            InitializeComponent();
            var vm = new HelpViewModel
            {
                CloseAction = Close
            };
            DataContext = vm;
        }
    }
}
