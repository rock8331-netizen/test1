# Agent Rules & Coding Standards

본 프로젝트는 [docs/MVVM_ARCHITECTURE_STANDARD.md](file:///e:/Dev/python_pj/No15_test/test1/docs/MVVM_ARCHITECTURE_STANDARD.md) 규격서를 표준 개발 가이드로 따르며, 모든 개발 시 아래 7가지 핵심 원칙을 반드시 준수합니다.

---

## 1. 글로벌 표준 MVVM 패턴 준수 (Standard MVVM Pattern)
- UI와 로직은 **Model - View - ViewModel - Services** 계층으로 완벽히 분리합니다.
- XAML 컨트롤 제어는 100% `{Binding ...}`과 `ICommand`를 사용하며, 비하인드 코드(`*.xaml.cs`)에 이벤트 핸들러를 넣지 않습니다.

## 2. 1파일 1클래스 원칙 (One Class per File)
- 하나의 소스 파일(.cs, .cpp, .py 등)에는 **단 하나의 클래스(Class)**만 정의해야 합니다.
- 파일 하나에 복수의 클래스, 구조체를 혼재하여 작성하는 것을 엄격히 금지합니다.

## 3. ViewModel의 UI 참조 금지 (No UI Controls in ViewModel)
- ViewModel 안에서 UI 컨트롤 객체(TextBox, Button, Window 등)를 직접 참조하거나 조작하지 않습니다.

## 4. 사전 제안 및 승인 원칙 (Proposal & Approval Before Action)
- 요청받지 않은 명칭(예: PRO, Premium 등)이나 임의의 수식어/기능을 멋대로 추가하지 마십시오.
- 좋은 아이디어나 개선안이 있을 경우 코드에 바로 반영하지 않고 **사용자에게 먼저 질문하여 허락을 받은 후 진행**합니다.

## 5. 버전 명기 표기 표준 (Version Naming Standard)
- 버전 정보는 임의의 수식어 대신 **'v2.0'** 또는 **'2.0'**으로만 정확히 표기합니다.

## 6. 서비스 계층 독립성 및 도메인별 최적 패턴 적용
- C++ DLL 호출 및 외부 백엔드 프로세스 실행은 `Services/` 계층으로 추상화하여 구동합니다.
- **UI 계층**: MVVM 패턴
- **C++ 코어 엔진**: C-API Export & RAII 패턴
- **Python 데이터 수집/분석**: ETL 데이터 파이프라인 패턴

## 7. 단일 파일 통합 배포 (Single File Packaging)
- 최종 배포는 외부 DLL 및 다국어 언어 폴더가 노출되지 않도록 **단일 실행 파일(`.exe`)**로 압축 통합하여 배포합니다.
