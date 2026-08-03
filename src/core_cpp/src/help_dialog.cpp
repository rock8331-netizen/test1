#include "../include/help_dialog.h"
#include <commctrl.h>
#include <string>

namespace YoutubeCore {

static HBRUSH g_hHelpBgBrush = NULL;
static HBRUSH g_hHelpCardBrush = NULL;
static HFONT  g_hHelpTitleFont = NULL;
static HFONT  g_hHelpBoldFont = NULL;
static HFONT  g_hHelpNormalFont = NULL;

static LRESULT CALLBACK HelpWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);

        g_hHelpNormalFont = CreateFontIndirectW(&ncm.lfMessageFont);

        LOGFONTW lfBold = ncm.lfMessageFont;
        lfBold.lfWeight = FW_BOLD;
        g_hHelpBoldFont = CreateFontIndirectW(&lfBold);

        LOGFONTW lfTitle = ncm.lfMessageFont;
        lfTitle.lfWeight = FW_BOLD;
        lfTitle.lfHeight = -16;
        g_hHelpTitleFont = CreateFontIndirectW(&lfTitle);

        g_hHelpBgBrush = CreateSolidBrush(RGB(245, 246, 248));
        g_hHelpCardBrush = CreateSolidBrush(RGB(255, 255, 255));

        // Create UI Controls inside Help Window
        HWND hTitle = CreateWindowExW(0, L"STATIC", L"❓ YouTube Downloader v2.0 가이드", WS_CHILD | WS_VISIBLE | SS_LEFT, 16, 16, 370, 24, hWnd, NULL, NULL, NULL);
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hHelpTitleFont, TRUE);

        HWND hSub = CreateWindowExW(0, L"STATIC", L"주요 기능 및 사용 방법 안내", WS_CHILD | WS_VISIBLE | SS_LEFT, 16, 40, 370, 18, hWnd, NULL, NULL, NULL);
        SendMessageW(hSub, WM_SETFONT, (WPARAM)g_hHelpNormalFont, TRUE);

        // Developer Info Panel
        HWND hDevBox = CreateWindowExW(0, L"STATIC",
            L"👨‍💻 개발자 (Developer): BJS                  📅 빌드 일자: 2026.08.03\r\n"
            L"🔖 버전 (Version): v2.0                         ⚡ Engine: C++ Win32 Core",
            WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER, 16, 68, 370, 48, hWnd, NULL, NULL, NULL);
        SendMessageW(hDevBox, WM_SETFONT, (WPARAM)g_hHelpBoldFont, TRUE);

        // Main Guide Text Edit Box
        const wchar_t* guide_text =
            L"🚀 [기본 사용 방법]\r\n"
            L"1. 유튜브 영상/플레이리스트 주소를 복사 후 [📋 붙여넣기] 버튼 클릭\r\n"
            L"2. 원하는 화질 및 쿠키/자막/새로받기 부가 옵션 선택\r\n"
            L"3. [🚀 다운로드 시작] 버튼 클릭 시 자동 다운로드 진행\r\n\r\n"

            L"🎬 [화질 선택 (Quality)]\r\n"
            L"• Best (최고): 4K(2160p) / 2K(1440p) / 1080p 원본 최고화질 수신\r\n"
            L"• 1080p / 720p / 480p / 360p 해상도 지정 수신 가능\r\n\r\n"

            L"🍪 [쿠키 연동 (Cookie)]\r\n"
            L"• 연령 제한 영상이나 멤버십 전용 동영상 수신 시 사용 중인 브라우저(Edge, Chrome, Firefox)를 선택하세요.\r\n\r\n"

            L"⚙️ [부가 옵션 안내]\r\n"
            L"• 자막 (.srt): 한국어/영어 자막 파일 함께 다운로드\r\n"
            L"• 새로 받기: 기존 다운로드 히스토리를 무시하고 강제 재다운로드\r\n\r\n"

            L"⚡ [하이브리드 코어 시스템]\r\n"
            L"C++ Win32 Native 엔진과 yt-dlp 파이프라인이 결합하여 0.01초 구동과 빠른 다운로드 속도를 제공합니다.";

        HWND hGuideEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", guide_text,
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            16, 126, 370, 270, hWnd, NULL, NULL, NULL);
        SendMessageW(hGuideEdit, WM_SETFONT, (WPARAM)g_hHelpNormalFont, TRUE);

        // Confirm Button
        HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"확인", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 286, 406, 100, 30, hWnd, (HMENU)IDOK, NULL, NULL);
        SendMessageW(hBtnOk, WM_SETFONT, (WPARAM)g_hHelpBoldFont, TRUE);

        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(30, 41, 59));
        SetBkColor(hdc, RGB(245, 246, 248));
        return (INT_PTR)g_hHelpBgBrush;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(30, 41, 59));
        SetBkColor(hdc, RGB(255, 255, 255));
        return (INT_PTR)g_hHelpCardBrush;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(hWnd);
            return TRUE;
        }
        break;

    case WM_DESTROY:
        if (g_hHelpBgBrush) DeleteObject(g_hHelpBgBrush);
        if (g_hHelpCardBrush) DeleteObject(g_hHelpCardBrush);
        if (g_hHelpTitleFont) DeleteObject(g_hHelpTitleFont);
        if (g_hHelpBoldFont) DeleteObject(g_hHelpBoldFont);
        if (g_hHelpNormalFont) DeleteObject(g_hHelpNormalFont);
        g_hHelpBgBrush = NULL;
        break;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void HelpDialog::Show(HWND hParent) {
    HINSTANCE hInstance = GetModuleHandleW(NULL);

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = HelpWndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(101));
    wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32514));
    wc.hbrBackground = CreateSolidBrush(RGB(245, 246, 248));
    wc.lpszClassName = L"YoutubeDownloaderHelpWin32";

    RegisterClassExW(&wc);

    RECT rcParent = { 0 };
    if (hParent) GetWindowRect(hParent, &rcParent);

    int width = 418;
    int height = 485;
    int x = rcParent.left + (rcParent.right - rcParent.left - width) / 2;
    int y = rcParent.top + (rcParent.bottom - rcParent.top - height) / 2;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME, L"YoutubeDownloaderHelpWin32", L"사용 가이드 및 도움말",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, width, height,
        hParent, NULL, hInstance, NULL
    );

    if (hParent) EnableWindow(hParent, FALSE);

    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    MSG msg;
    while (IsWindow(hDlg) && GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hParent) {
        EnableWindow(hParent, TRUE);
        SetForegroundWindow(hParent);
    }
}

} // namespace YoutubeCore
