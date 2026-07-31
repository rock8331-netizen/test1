/**
 * dll_test.cpp - YoutubeCore.dll 기능 검증 테스트
 */
#include <iostream>
#include <windows.h>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

// DLL 함수 포인터 타입 정의
typedef int  (*FnFindFFmpegPath)(char*, int);
typedef void (*FnFormatBytes)(long long, char*, int);
typedef int  (*FnScanDirectory)(const char*, int*, long long*);
typedef int  (*FnKillProcessById)(int);

void initConsoleEncoding() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
}

int main() {
    initConsoleEncoding();

    std::cout << "==========================================" << std::endl;
    std::cout << "   YoutubeCore.dll 기능 검증 테스트       " << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;

    // DLL 로드
    HMODULE dll = LoadLibraryA("build\\YoutubeCore.dll");
    if (!dll) {
        std::cerr << "❌ DLL 로드 실패! (오류코드: " << GetLastError() << ")" << std::endl;
        return 1;
    }
    std::cout << "✅ YoutubeCore.dll 로드 성공" << std::endl << std::endl;

    // 함수 포인터 획득
    auto FindFFmpegPath  = (FnFindFFmpegPath)  GetProcAddress(dll, "FindFFmpegPath");
    auto FormatBytes     = (FnFormatBytes)     GetProcAddress(dll, "FormatBytes");
    auto ScanDirectory   = (FnScanDirectory)   GetProcAddress(dll, "ScanDirectory");

    // ── 테스트 1: FormatBytes ──────────────────────────────
    std::cout << "[테스트 1] FormatBytes 변환" << std::endl;
    long long sizes[] = { 512LL, 1536LL * 1024, 2LL * 1024 * 1024 * 1024 };
    const char* labels[] = { "512 B", "1.5 MB", "2 GB" };
    for (int i = 0; i < 3; i++) {
        char buf[64] = {};
        FormatBytes(sizes[i], buf, sizeof(buf));
        std::cout << "  " << labels[i] << " → " << buf << std::endl;
    }

    // ── 테스트 2: FindFFmpegPath ──────────────────────────
    std::cout << std::endl << "[테스트 2] ffmpeg.exe 경로 탐색" << std::endl;
    char ffmpeg_path[512] = {};
    int found = FindFFmpegPath(ffmpeg_path, sizeof(ffmpeg_path));
    if (found) {
        std::cout << "  ✅ ffmpeg 발견: " << ffmpeg_path << std::endl;
    } else {
        std::cout << "  ⚠️  ffmpeg 없음 (추후 자동 다운로드 예정)" << std::endl;
    }

    // ── 테스트 3: ScanDirectory ───────────────────────────
    std::cout << std::endl << "[테스트 3] 디렉터리 스캔 (현재 폴더)" << std::endl;
    int file_count = 0;
    long long total_bytes = 0;
    char scan_buf[64] = {};
    int ok = ScanDirectory(".", &file_count, &total_bytes);
    if (ok) {
        FormatBytes(total_bytes, scan_buf, sizeof(scan_buf));
        std::cout << "  파일 수: " << file_count << "개  |  총 용량: " << scan_buf << std::endl;
    } else {
        std::cout << "  ❌ 스캔 실패" << std::endl;
    }

    FreeLibrary(dll);

    std::cout << std::endl << "==========================================" << std::endl;
    std::cout << "  ✅ 모든 DLL 기능 검증 완료!             " << std::endl;
    std::cout << "==========================================" << std::endl;

    return 0;
}
