using System;
using System.Windows.Input;
using YoutubeDownloader.MVVM.Base;

namespace YoutubeDownloader.ViewModels
{
    public class HelpViewModel : ObservableObject
    {
        public Action? CloseAction { get; set; }

        public ICommand CloseCommand { get; }

        public HelpViewModel()
        {
            CloseCommand = new RelayCommand(() => CloseAction?.Invoke());
        }
    }
}
