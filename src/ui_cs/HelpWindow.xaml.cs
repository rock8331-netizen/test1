using System.Diagnostics;
using System.Windows;
using System.Windows.Navigation;
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

        private void Hyperlink_RequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            Process.Start(new ProcessStartInfo(e.Uri.AbsoluteUri) { UseShellExecute = true });
            e.Handled = true;
        }
    }
}
