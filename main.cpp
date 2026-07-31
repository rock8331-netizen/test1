#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

// 콘솔 한글 인코딩(UTF-8) 자동 설정 함수
void initConsoleEncoding() {
#ifdef _WIN32
    // 윈도우 콘솔 입출력 코드를 UTF-8(Code Page 65001)로 설정
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

int main() {
    // 프로그램 시작 시 콘솔 한글 깨짐 방지 설정 적용
    initConsoleEncoding();

    std::cout << "==========================================" << std::endl;
    std::cout << "  C++ Core Engine Minimal Text Example   " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::endl;

    std::cout << "[1] 기본 텍스트 출력 테스트" << std::endl;
    std::string message = "안녕하세요! C++ 코어 엔진 예제 프로그램입니다.";
    std::cout << "메시지: " << message << std::endl << std::endl;

    std::cout << "[2] C++ 표준 라이브러리 (std::vector) 동작 테스트" << std::endl;
    std::vector<std::string> features = {
        "1. C++ High-Performance Computation",
        "2. C# GUI Interop (P/Invoke)",
        "3. Python Analytics Integration"
    };

    for (const auto& feature : features) {
        std::cout << " - " << feature << std::endl;
    }

    std::cout << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "  프로그램이 성공적으로 실행되었습니다. " << std::endl;
    std::cout << "==========================================" << std::endl;

    return 0;
}
