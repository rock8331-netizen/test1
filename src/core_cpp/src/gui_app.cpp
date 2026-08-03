#include "../include/gui_app.h"
#include "../include/help_dialog.h"
#include <shlobj.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

#define IDC_BTN_HELP        1001
#define IDC_BTN_OPEN_FOLDER 1002
#define IDC_EDIT_URL        1003
#define IDC_BTN_PASTE       1004
#define IDC_COMBO_QUALITY   1005
#define IDC_COMBO_COOKIE    1006
#define IDC_CHK_SUBTITLES   1007
#define IDC_CHK_FORCE       1008
#define IDC_EDIT_OUTPUT     1009
#define IDC_BTN_BROWSE      1010
#define IDC_BTN_ACTION      1011
#define IDC_PROGRESS_BAR    1012
#define IDC_STATIC_PROGRESS 1013
#define IDC_EDIT_LOG        1014
#define IDC_BTN_CLEAR_LOG   1015
#define IDC_BTN_COPY_LOG    1016

#define WM_USER_APPEND_LOG   (WM_USER + 101)
#define WM_USER_PROGRESS     (WM_USER + 102)
#define WM_USER_COMPLETE     (WM_USER + 103)

namespace YoutubeCore {

static std::wstring Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (len <= 0) return L"";
    std::vector<wchar_t> wbuf(len);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wbuf.data(), len);
    return std::wstring(wbuf.data());
}

static std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (len <= 0) return "";
    std::vector<char> buf(len);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buf.data(), len, NULL, NULL);
    return std::string(buf.data());
}

GuiApp::GuiApp(HINSTANCE hInstance) : m_hInstance(hInstance) {
    m_runner = std::make_unique<BackendRunner>();
    m_hBgBrush = CreateSolidBrush(RGB(245, 246, 248));      // Canvas background
    m_hConsoleBrush = CreateSolidBrush(RGB(18, 18, 18));    // Terminal black console
    m_hRedBrush = CreateSolidBrush(RGB(225, 29, 72));        // Accent Red
}

GuiApp::~GuiApp() {
    if (m_hBgBrush) DeleteObject(m_hBgBrush);
    if (m_hConsoleBrush) DeleteObject(m_hConsoleBrush);
    if (m_hRedBrush) DeleteObject(m_hRedBrush);
    if (m_hFontNormal) DeleteObject(m_hFontNormal);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontMonospace) DeleteObject(m_hFontMonospace);
}

int GuiApp::Run(int nCmdShow) {
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES | ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = GuiApp::WndProcStatic;
    wc.hInstance = m_hInstance;
    wc.hIcon = LoadIconW(m_hInstance, MAKEINTRESOURCEW(101));
    if (!wc.hIcon) wc.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(32512)); // IDI_APPLICATION
    wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32514));            // IDC_ARROW
    wc.hbrBackground = m_hBgBrush;
    wc.lpszClassName = L"YoutubeDownloaderWin32";
    wc.hIconSm = wc.hIcon;

    RegisterClassExW(&wc);

    m_hWnd = CreateWindowExW(
        0, L"YoutubeDownloaderWin32", L"YouTube Downloader v2.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 410, 420,
        NULL, NULL, m_hInstance, this
    );

    if (!m_hWnd) return 0;

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    // Initial logs
    AppendLog("✅ YouTube Downloader v2.0 준비 완료 (C++ Native 88KB)");
    AppendLog("⚡ 런타임 독립실행 0.01s 초고속 엔진 구동");

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK GuiApp::WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GuiApp* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = (CREATESTRUCTW*)lParam;
        pThis = (GuiApp*)pCreate->lpCreateParams;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    pThis = (GuiApp*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    if (pThis) return pThis->WndProc(hWnd, msg, wParam, lParam);
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT GuiApp::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateControls(hWnd);
        break;

    case WM_SIZE:
        LayoutControls(hWnd, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 380;
        mmi->ptMinTrackSize.y = 380;
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
        HWND hCtl = (HWND)lParam;
        HDC hdc = (HDC)wParam;
        if (hCtl == m_hEditLog) {
            SetTextColor(hdc, RGB(248, 249, 250));
            SetBkColor(hdc, RGB(18, 18, 18));
            return (INT_PTR)m_hConsoleBrush;
        }
        SetTextColor(hdc, RGB(30, 41, 59));
        SetBkColor(hdc, RGB(245, 246, 248));
        return (INT_PTR)m_hBgBrush;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_BTN_ACTION) OnStartOrStopDownload();
        else if (id == IDC_BTN_PASTE) OnPasteUrl();
        else if (id == IDC_BTN_BROWSE) OnBrowseOutputFolder();
        else if (id == IDC_BTN_OPEN_FOLDER) OnOpenCompletedFolder();
        else if (id == IDC_BTN_HELP) OnShowHelp();
        else if (id == IDC_BTN_CLEAR_LOG) OnClearLog();
        else if (id == IDC_BTN_COPY_LOG) OnCopyLog();
        break;
    }

    case WM_USER_APPEND_LOG: {
        char* str = (char*)lParam;
        if (str) {
            AppendLog(str);
            free(str);
        }
        break;
    }

    case WM_USER_PROGRESS: {
        DownloadProgress* prog = (DownloadProgress*)lParam;
        if (prog) {
            UpdateProgress(*prog);
            delete prog;
        }
        break;
    }

    case WM_USER_COMPLETE: {
        m_isDownloading = false;
        SetWindowTextW(m_hBtnAction, L"🚀 다운로드 시작");
        EnableWindow(m_hBtnAction, TRUE);
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void GuiApp::CreateControls(HWND hWnd) {
    NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);
    m_hFontNormal = CreateFontIndirectW(&ncm.lfMessageFont);

    LOGFONTW lfBold = ncm.lfMessageFont;
    lfBold.lfWeight = FW_BOLD;
    m_hFontBold = CreateFontIndirectW(&lfBold);

    m_hFontMonospace = CreateFontW(
        13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas"
    );

    // 0: Header Buttons
    m_hBtnHelp = CreateWindowExW(0, L"BUTTON", L"❓ 도움말", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_HELP, m_hInstance, NULL);
    m_hBtnOpenFolder = CreateWindowExW(0, L"BUTTON", L"📁 완료 폴더", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_OPEN_FOLDER, m_hInstance, NULL);

    // 2: URL Input & Paste
    m_hEditUrl = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_URL, m_hInstance, NULL);
    m_hBtnPaste = CreateWindowExW(0, L"BUTTON", L"📋 붙여넣기", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_PASTE, m_hInstance, NULL);

    // 4: Quality & Cookie Combos
    m_hComboQuality = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_COMBO_QUALITY, m_hInstance, NULL);
    const wchar_t* qualities[] = { L"Best (최고)", L"2160p (4K)", L"1440p (2K)", L"1080p (FHD)", L"720p (HD)", L"480p", L"360p" };
    for (const wchar_t* q : qualities) SendMessageW(m_hComboQuality, CB_ADDSTRING, 0, (LPARAM)q);
    SendMessageW(m_hComboQuality, CB_SETCURSEL, 0, 0);

    m_hComboCookie = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_COMBO_COOKIE, m_hInstance, NULL);
    const wchar_t* cookies[] = { L"none", L"edge", L"chrome", L"firefox" };
    for (const wchar_t* c : cookies) SendMessageW(m_hComboCookie, CB_ADDSTRING, 0, (LPARAM)c);
    SendMessageW(m_hComboCookie, CB_SETCURSEL, 0, 0);

    m_hChkSubtitles = CreateWindowExW(0, L"BUTTON", L"자막 (.srt)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_CHK_SUBTITLES, m_hInstance, NULL);
    m_hChkForce = CreateWindowExW(0, L"BUTTON", L"새로 받기", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 0, 0, 0, hWnd, (HMENU)IDC_CHK_FORCE, m_hInstance, NULL);

    // 6: Output Path & Browse
    std::string default_out = BackendRunner::GetDefaultOutputPath();
    std::wstring wdefault_out = Utf8ToWide(default_out);
    m_hEditOutput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wdefault_out.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_OUTPUT, m_hInstance, NULL);
    m_hBtnBrowse = CreateWindowExW(0, L"BUTTON", L"📁 찾아보기", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_BROWSE, m_hInstance, NULL);

    // 8: Action Button
    m_hBtnAction = CreateWindowExW(0, L"BUTTON", L"🚀 다운로드 시작", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_ACTION, m_hInstance, NULL);

    // 10: Progress Bar & Text
    m_hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, L"", WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 0, 0, 0, 0, hWnd, (HMENU)IDC_PROGRESS_BAR, m_hInstance, NULL);
    m_hStaticProgress = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP, 0, 0, 0, 0, hWnd, (HMENU)IDC_STATIC_PROGRESS, m_hInstance, NULL);

    // 12: Terminal Log Console & Buttons
    m_hEditLog = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL, 0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_LOG, m_hInstance, NULL);
    m_hBtnClearLog = CreateWindowExW(0, L"BUTTON", L"지우기", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_CLEAR_LOG, m_hInstance, NULL);
    m_hBtnCopyLog = CreateWindowExW(0, L"BUTTON", L"복사", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDC_BTN_COPY_LOG, m_hInstance, NULL);

    // Set fonts
    HWND hwnds[] = { m_hBtnHelp, m_hBtnOpenFolder, m_hEditUrl, m_hBtnPaste, m_hComboQuality, m_hComboCookie,
                     m_hChkSubtitles, m_hChkForce, m_hEditOutput, m_hBtnBrowse, m_hBtnAction, m_hStaticProgress,
                     m_hBtnClearLog, m_hBtnCopyLog };
    for (HWND h : hwnds) SendMessageW(h, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);
    SendMessageW(m_hBtnAction, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
    SendMessageW(m_hEditLog, WM_SETFONT, (WPARAM)m_hFontMonospace, TRUE);
}

void GuiApp::LayoutControls(HWND hWnd, int width, int height) {
    int margin = 10;
    int y = margin;
    int content_w = width - (margin * 2);

    // Row 0: Top Header
    SetWindowPos(m_hBtnHelp, NULL, width - margin - 150, y, 70, 24, SWP_NOZORDER);
    SetWindowPos(m_hBtnOpenFolder, NULL, width - margin - 75, y, 75, 24, SWP_NOZORDER);
    y += 28;

    // Row 2: URL & Paste
    SetWindowPos(m_hEditUrl, NULL, margin, y, content_w - 90, 26, SWP_NOZORDER);
    SetWindowPos(m_hBtnPaste, NULL, margin + content_w - 85, y, 85, 26, SWP_NOZORDER);
    y += 32;

    // Row 4: Quality, Cookie, Subtitles, Force
    int half_w = (content_w - 10) / 2;
    SetWindowPos(m_hComboQuality, NULL, margin, y, half_w, 24, SWP_NOZORDER);
    SetWindowPos(m_hComboCookie, NULL, margin + half_w + 10, y, half_w, 24, SWP_NOZORDER);
    y += 28;

    SetWindowPos(m_hChkSubtitles, NULL, margin, y, half_w, 22, SWP_NOZORDER);
    SetWindowPos(m_hChkForce, NULL, margin + half_w + 10, y, half_w, 22, SWP_NOZORDER);
    y += 28;

    // Row 6: Output & Browse
    SetWindowPos(m_hEditOutput, NULL, margin, y, content_w - 90, 26, SWP_NOZORDER);
    SetWindowPos(m_hBtnBrowse, NULL, margin + content_w - 85, y, 85, 26, SWP_NOZORDER);
    y += 32;

    // Row 8: Action Button
    SetWindowPos(m_hBtnAction, NULL, margin, y, content_w, 32, SWP_NOZORDER);
    y += 36;

    // Row 10: Progress Bar & Text
    SetWindowPos(m_hProgressBar, NULL, margin, y, content_w, 6, SWP_NOZORDER);
    y += 8;
    SetWindowPos(m_hStaticProgress, NULL, margin, y, content_w, 18, SWP_NOZORDER);
    y += 20;

    // Row 12: Terminal Log Console & Buttons
    int log_h = height - y - margin;
    if (log_h < 60) log_h = 60;

    SetWindowPos(m_hBtnClearLog, NULL, width - margin - 100, y - 20, 48, 18, SWP_NOZORDER);
    SetWindowPos(m_hBtnCopyLog, NULL, width - margin - 48, y - 20, 48, 18, SWP_NOZORDER);

    SetWindowPos(m_hEditLog, NULL, margin, y, content_w, log_h, SWP_NOZORDER);
}

void GuiApp::OnStartOrStopDownload() {
    if (m_isDownloading) {
        m_runner->Stop();
        AppendLog("🛑 [중단] 다운로드가 사용자에 의해 중단되었습니다.");
        m_isDownloading = false;
        SetWindowTextW(m_hBtnAction, L"🚀 다운로드 시작");
        return;
    }

    wchar_t url_wbuf[2048] = {};
    GetWindowTextW(m_hEditUrl, url_wbuf, 2048);
    std::string url = WideToUtf8(url_wbuf);

    if (url.empty()) {
        MessageBoxW(m_hWnd, L"유튜브 URL을 입력해 주세요.", L"입력 오류", MB_OK | MB_ICONWARNING);
        return;
    }

    wchar_t out_wbuf[MAX_PATH] = {};
    GetWindowTextW(m_hEditOutput, out_wbuf, MAX_PATH);

    DownloadOptions opts;
    opts.url = url;
    opts.output_path = WideToUtf8(out_wbuf);

    int q_idx = (int)SendMessageW(m_hComboQuality, CB_GETCURSEL, 0, 0);
    const char* q_tags[] = { "best", "2160p", "1440p", "1080p", "720p", "480p", "360p" };
    if (q_idx >= 0 && q_idx < 7) opts.quality = q_tags[q_idx];

    int c_idx = (int)SendMessageW(m_hComboCookie, CB_GETCURSEL, 0, 0);
    const char* c_tags[] = { "none", "edge", "chrome", "firefox" };
    if (c_idx >= 0 && c_idx < 4) opts.cookies = c_tags[c_idx];

    opts.save_subtitles = (SendMessageW(m_hChkSubtitles, BM_GETCHECK, 0, 0) == BST_CHECKED);
    opts.force_download = (SendMessageW(m_hChkForce, BM_GETCHECK, 0, 0) == BST_CHECKED);

    m_isDownloading = true;
    SetWindowTextW(m_hBtnAction, L"🛑 다운로드 중단");

    AppendLog("\n🚀 [시작] URL: " + url);

    HWND hWndMain = m_hWnd;
    m_runner->Start(
        opts,
        [hWndMain](const DownloadProgress& p) {
            DownloadProgress* copy = new DownloadProgress(p);
            PostMessageW(hWndMain, WM_USER_PROGRESS, 0, (LPARAM)copy);
        },
        [hWndMain](const std::string& msg) {
            char* copy = _strdup(msg.c_str());
            PostMessageW(hWndMain, WM_USER_APPEND_LOG, 0, (LPARAM)copy);
        },
        [hWndMain](bool success) {
            PostMessageW(hWndMain, WM_USER_COMPLETE, success ? 1 : 0, 0);
        }
    );
}

void GuiApp::OnPasteUrl() {
    if (OpenClipboard(m_hWnd)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* text = (wchar_t*)GlobalLock(hData);
            if (text) {
                SetWindowTextW(m_hEditUrl, text);
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }
}

void GuiApp::OnBrowseOutputFolder() {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = m_hWnd;
    bi.lpszTitle = L"다운로드 저장 폴더 선택";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH] = {};
        if (SHGetPathFromIDListW(pidl, path)) {
            SetWindowTextW(m_hEditOutput, path);
        }
        CoTaskMemFree(pidl);
    }
}

void GuiApp::OnOpenCompletedFolder() {
    wchar_t out_wbuf[MAX_PATH] = {};
    GetWindowTextW(m_hEditOutput, out_wbuf, MAX_PATH);
    CreateDirectoryW(out_wbuf, NULL);
    ShellExecuteW(NULL, L"open", out_wbuf, NULL, NULL, SW_SHOW);
}

void GuiApp::OnShowHelp() {
    HelpDialog::Show(m_hWnd);
}

void GuiApp::OnClearLog() {
    SetWindowTextW(m_hEditLog, L"");
}

void GuiApp::OnCopyLog() {
    int len = GetWindowTextLengthW(m_hEditLog);
    if (len > 0) {
        std::vector<wchar_t> buf(len + 1);
        GetWindowTextW(m_hEditLog, buf.data(), len + 1);

        if (OpenClipboard(m_hWnd)) {
            EmptyClipboard();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
            if (hMem) {
                wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
                memcpy(pMem, buf.data(), (len + 1) * sizeof(wchar_t));
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
            CloseClipboard();
        }
    }
}

void GuiApp::AppendLog(const std::string& msg) {
    time_t t = time(nullptr);
    tm tm_info;
    localtime_s(&tm_info, &t);
    std::ostringstream ss;
    ss << "[" << std::setfill('0') << std::setw(2) << tm_info.tm_hour << ":"
       << std::setfill('0') << std::setw(2) << tm_info.tm_min << ":"
       << std::setfill('0') << std::setw(2) << tm_info.tm_sec << "] "
       << msg << "\r\n";

    std::wstring wformatted = Utf8ToWide(ss.str());

    int len = GetWindowTextLengthW(m_hEditLog);
    SendMessageW(m_hEditLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(m_hEditLog, EM_REPLACESEL, FALSE, (LPARAM)wformatted.c_str());
}

void GuiApp::UpdateProgress(const DownloadProgress& prog) {
    SendMessageW(m_hProgressBar, PBM_SETPOS, (WPARAM)(int)prog.percent, 0);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << prog.percent << "% - "
       << prog.filename << " (⚡ " << prog.speed << " | ⏱ ETA " << prog.eta << ")";

    std::wstring wtext = Utf8ToWide(ss.str());
    SetWindowTextW(m_hStaticProgress, wtext.c_str());
}

} // namespace YoutubeCore
