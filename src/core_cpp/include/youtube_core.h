#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #define YOUTUBE_API __declspec(dllexport)
#else
    #define YOUTUBE_API __attribute__((visibility("default")))
#endif

/**
 * ffmpeg.exe 경로를 탐색하여 out_path 버퍼에 채워 넣는다.
 * @param out_path  탐색된 ffmpeg.exe가 있는 폴더 경로 (결과 버퍼, 최소 512바이트)
 * @param buf_size  out_path 버퍼 크기
 * @return  1: 발견됨, 0: 발견 못함
 */
YOUTUBE_API int FindFFmpegPath(char* out_path, int buf_size);

/**
 * 바이트 수치를 사람이 읽기 쉬운 MB / GB 문자열로 변환한다.
 * @param bytes     변환할 바이트 수치
 * @param out_str   결과 문자열 버퍼 (최소 32바이트)
 * @param buf_size  out_str 버퍼 크기
 */
YOUTUBE_API void FormatBytes(long long bytes, char* out_str, int buf_size);

/**
 * 지정한 디렉터리 내 파일 개수 및 총 바이트 수를 반환한다.
 * @param dir_path      스캔할 디렉터리 경로 (UTF-8)
 * @param out_count     파일 개수 (결과)
 * @param out_total_bytes 전체 바이트 수 (결과)
 * @return  1: 성공, 0: 디렉터리 없음/오류
 */
YOUTUBE_API int ScanDirectory(const char* dir_path, int* out_count, long long* out_total_bytes);

/**
 * 지정한 PID의 프로세스를 강제 종료한다. (다운로드 중단용)
 * @param pid   종료할 프로세스 ID
 * @return  1: 성공, 0: 실패
 */
YOUTUBE_API int KillProcessById(int pid);

#ifdef __cplusplus
}
#endif
