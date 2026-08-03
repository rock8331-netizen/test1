#pragma once
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <memory>
#include "backend_runner.h"

namespace YoutubeCore {

class GuiApp {
public:
    GuiApp(HINSTANCE hInstance);
    ~GuiApp();

    int Run(int nCmdShow);

private:
    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateControls(HWND hWnd);
    void LayoutControls(HWND hWnd, int width, int height);

    void OnStartOrStopDownload();
    void OnPasteUrl();
    void OnBrowseOutputFolder();
    void OnOpenCompletedFolder();
    void OnShowHelp();
    void OnClearLog();
    void OnCopyLog();

    void AppendLog(const std::string& msg);
    void UpdateProgress(const DownloadProgress& prog);

    HINSTANCE m_hInstance;
    HWND m_hWnd{NULL};

    // UI Controls
    HWND m_hBtnHelp{NULL};
    HWND m_hBtnOpenFolder{NULL};

    HWND m_hEditUrl{NULL};
    HWND m_hBtnPaste{NULL};

    HWND m_hComboQuality{NULL};
    HWND m_hComboCookie{NULL};
    HWND m_hChkSubtitles{NULL};
    HWND m_hChkForce{NULL};

    HWND m_hEditOutput{NULL};
    HWND m_hBtnBrowse{NULL};

    HWND m_hBtnAction{NULL};

    HWND m_hProgressBar{NULL};
    HWND m_hStaticProgress{NULL};

    HWND m_hEditLog{NULL};
    HWND m_hBtnClearLog{NULL};
    HWND m_hBtnCopyLog{NULL};

    // Brushes & Fonts
    HFONT m_hFontNormal{NULL};
    HFONT m_hFontBold{NULL};
    HFONT m_hFontMonospace{NULL};
    HBRUSH m_hBgBrush{NULL};
    HBRUSH m_hConsoleBrush{NULL};
    HBRUSH m_hRedBrush{NULL};

    std::unique_ptr<BackendRunner> m_runner;
    bool m_isDownloading{false};
};

} // namespace YoutubeCore
