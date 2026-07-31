# C++ / C# / Python 하이브리드 프로젝트 개발 및 2대 노트북 동기화 가이드

이 문서는 윈도우 환경에서 C++ 개발 환경을 구축하고, 한글 인코딩 문제 해결, 2대의 노트북 간 GitHub 동기화 워크플로우를 정리한 설명서입니다.

---

## 1. C++ 개발 환경 최초 세팅 (새 노트북 B 용)

새로운 컴(노트북 B)에서 작업할 때 아래 3가지 단계만 터미널(PowerShell)에 입력하면 5분 안에 동일한 환경이 구성됩니다.

### 1단계: 초경량 C++ 컴파일러 설치 (약 50MB)
```powershell
winget install MartinStorsjo.LLVM-MinGW.UCRT
```
> **참고**: 설치 완료 후 터미널 창을 닫고 새로 열어야 `g++` 명령어가 인식됩니다.

### 2단계: Git 사용자 정보 등록
```powershell
git config --global user.name "rock8331"
git config --global user.email "rock8331@gmail.com"
```

### 3단계: GitHub 프로젝트 다운로드 (Clone)
작업할 폴더 위치로 이동 후:
```powershell
git clone https://github.com/rock8331-netizen/test1.git
cd test1
```

---

## 2. 2대의 노트북 간 일상 개발 동기화 루틴

노트북 A와 노트북 B를 번갈아 사용할 때는 **`git pull`**과 **`git push`**만 기억하시면 됩니다.

```
[노트북 A 작업 끝] ---> git push ---> [GitHub] ---> git pull ---> [노트북 B 작업 시작]
```

### 🔹 작업 시작할 때 (최신 코드 가져오기)
작업을 시작하기 전 무조건 최신 코드를 다운로드합니다.
```powershell
git pull
```

### 🔹 작업 마칠 때 (내 작업 올리기)
작업을 마치거나 다른 노트북으로 이동하기 전 실행합니다.
```powershell
git add .
git commit -m "작업 내용 요약"
git push
```

---

## 3. C++ 컴파일 및 타 컴퓨터 배포 방법

### 1) 빌드 및 실행 명령어
```powershell
# 1. 컴파일 (독립 실행 파일 생성)
g++ -static main.cpp -o main.exe

# 2. 실행
.\main.exe
```

### 2) `-static` 옵션의 역할 (배포 팁)
* `-static` 옵션을 사용하여 빌드하면 C++ 필수 라이브러리가 `.exe` 파일 내부로 포함됩니다.
* 다른 윈도우 컴퓨터에 C++ 개발 도구가 설치되어 있지 않아도 **`main.exe` 파일 하나만 복사하면 100% 정상 실행**됩니다.

---

## 4. 콘솔 한글 깨짐 방지 처리 (UTF-8)

윈도우 콘솔 기본 인코딩(CP949)으로 인해 발생하는 한글 깨짐을 방지하기 위해 `main.cpp` 시작 부분에 아래 설정을 적용합니다.

```cpp
#ifdef _WIN32
#include <windows.h>
#endif

void initConsoleEncoding() {
#ifdef _WIN32
    // 윈도우 콘솔 입출력 코드페이지를 UTF-8(65001)로 설정
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

int main() {
    initConsoleEncoding();
    // ...
}
```

---

## 5. 용량 및 쓰레기 파일 관리 (.gitignore)

* 프로젝트 루트 폴더에 `.gitignore` 파일이 작성되어 있어 컴파일 생성물(`*.exe`, `*.obj`, `build/`)은 Git 추적에서 제외됩니다.
* 개발 후 공간을 정리하고 싶다면 프로젝트 내의 `main.exe`나 `build/` 폴더만 삭제하시면 잔여물 없이 깔끔히 정리됩니다.
