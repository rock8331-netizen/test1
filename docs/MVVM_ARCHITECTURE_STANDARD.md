# 글로벌 표준 MVVM 아키텍처 규격서 (Global Standard MVVM Architecture Specification)

## 📌 개요 (Overview)
본 규격서는 본 소프트웨어 프로젝트 개발 시 모든 에이전트와 개발자가 예외 없이 준수해야 하는 **글로벌 표준 MVVM (Model-View-ViewModel) 디자인 패턴 및 기술 영역별 최적 개발 방법론**을 정의합니다.

---

## 🏛️ 1. 계층 구조 및 역할 분리 (Layered Architecture)

모든 코드베이스는 다음 4가지 핵심 계층으로 철저히 분리되어야 합니다.

```
src/
├── MVVM/Base/         ➔ MVVM 인프라 베이스 클래스 (ObservableObject, RelayCommand 등)
├── Services/          ➔ 데이터 획득, 프로세스 실행, API/DLL 호출을 전담하는 서비스 계층
├── ViewModels/        ➔ UI 상태값, 바인딩 속성, ICommand 동작을 관리하는 중계 계층
├── Views/             ➔ 오직 XAML 기반으로 화면 레이아웃만 정의하는 사용자 인터페이스
└── Models/            ➔ 순수 데이터 구조체 및 DTO
```

---

## 📜 2. 7대 핵심 개발 규칙 (Core Engineering Rules)

### 1️⃣ 1파일 1클래스 원칙 (One Class per File)
- 하나의 소스 파일(`.cs`, `.cpp`, `.py` 등)에는 **오직 단 하나의 클래스(Class)**만 정의합니다.
- 복수의 클래스, 구조체, 열거형을 한 파일에 섞어서 작성하는 것을 엄격히 금지합니다.

### 2️⃣ 순수 View 코드비하인드 원칙 (Zero Code-Behind Logic)
- `*.xaml.cs` 비하인드 코드 파일에는 클릭 이벤트(`Click="..."`)나 UI 제어 로직을 직접 작성할 수 없습니다.
- 비하인드 코드는 `DataContext` 연결 및 윈도우 창 닫기/생성 제어 이외의 모든 비즈니스 로직을 금지합니다.

### 3️⃣ ViewModel의 UI 개체 참조 엄금 (No UI Framework Reference)
- ViewModel 클래스 내부에서는 `System.Windows.Controls` 등 UI 컨트롤 개체(`TextBox`, `Button`, `MessageBox` 등)를 직접 생성하거나 참조할 수 없습니다.
- 모든 UI와 ViewModel 간의 소통은 **데이터 바인딩(`{Binding ...}`)**과 **커맨드(`ICommand`)**로만 수행합니다.

### 4️⃣ 서비스 계층 분리 (Service Layer Decoupling)
- 비즈니스 로직, 데이터 수신, C++ Native DLL 연동, 프로세스 실행 등은 ViewModel 내부에서 직접 처리하지 않고, 별도의 `Services/` 계층으로 추상화하여 주입받습니다.

### 5️⃣ 사전 제안 및 승인 원칙 (Proposal & Approval Before Action)
- 요청받지 않은 명칭(예: PRO, Premium 등)이나 임의의 수식어/기능을 멋대로 추가하지 마십시오.
- 리팩토링이나 개선에 대한 아이디어가 있을 경우 **사용자에게 먼저 제안하여 허락을 받은 후** 구현합니다.

### 6️⃣ 버전 명기 표기 표준 (Version Naming Standard)
- 버전 정보는 임의의 다른 수식어 대신 **'v2.0'** 또는 **'2.0'**으로만 정확히 표기합니다.

### 7️⃣ 단일 파일 독립 배포 표준 (Publish Single File Standard)
- 최종 빌드 출력물은 외부 DLL 파일이나 다국어 언어 폴더가 흩어지지 않도록 **단일 통합 실행 파일(`.exe`)**로 압축 배포합니다.

---

## 🌐 3. 도메인별 최적 개발 방법론 (Polyglot Architecture Standards)

본 프로젝트는 UI, C++ 코어 엔진, Python 분석 파이프라인의 3개 기술 영역으로 나뉘며, 각 도메인 특성에 맞춘 최적의 디자인 패턴을 조합하여 개발합니다.

| 도메인 계층 | 기술 스택 | 적용 개발 방법론 | 기술적 적용 이유 |
| :--- | :--- | :--- | :--- |
| **UI 계층 (`ui_cs/`)** | C# WPF (.NET 8) | **글로벌 표준 MVVM 패턴** | UI 화면 제어와 데이터 상태값의 양방향 바인딩 및 비하인드 코드 완전 분리 |
| **코어 엔진 계층 (`core_cpp/`)** | C++ Win32 | **C-API Export & RAII 패턴** | 메모리 안전성 극대화 및 외부(C# P/Invoke) 연동을 위한 명확한 `extern "C"` 래퍼 제공 |
| **데이터 분석/수집 계층 (`analytics_py/`)** | Python | **ETL 데이터 파이프라인 패턴** | 크롤링, 메타데이터 추출, 파싱 및 JSON 처리에 최적화된 단방향 모듈 파이프라인 구축 |

---

## 🛠️ 4. 기술 스택 및 인터페이스 표준 (Technical Implementation)

- **UI Framework**: WPF (.NET 8.0)
- **Data Binding Base**: `INotifyPropertyChanged` 기반 `ObservableObject`
- **Command Binding Base**: `ICommand` 기반 `RelayCommand`
- **Native Interop**: C++ Win32 DLL + P/Invoke (`NativeInterop.cs`)
- **Backend Service**: `DownloadService.cs`
