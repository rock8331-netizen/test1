# 핵심 다운로드 엔진 기능 명세서 (Core Engine Specifications)

## 📌 개요
본 문서는 YouTube Downloader 프로젝트의 핵심 백엔드 엔진 기능 및 파싱/오류 처리 규격을 정의합니다.

---

## ⚙️ 1. 핵심 다운로드 엔진 기능 명세

### 1.1 URL 스마트 자동 감지 (Smart URL Parsing & Scope Resolution)
* **작동 규격**:
  - URL에 `watch?v=VIDEO_ID`가 포함된 경우: 사용자가 특정 영상 시청 중 주소를 복사해온 것이므로 `noplaylist: True`를 자동 적용하여 **단일 영상 1개만 다운로드**.
  - URL에 `playlist?list=PLAYLIST_ID` 형태만 포함된 경우: `noplaylist: False`를 자동 적용하여 **플레이리스트 전체 다운로드**.
* **설계 이점**: 사용자가 수동으로 범위를 선택하는 불필요한 라디오 버튼을 제거하여 최소주의(KISS 원칙) UX 달성.

### 1.2 플레이리스트 결함 허용 및 완주 보장 (Fault Tolerance)
* **개별 영상 오류 건너뛰기 (`ignoreerrors: True`)**:  
  플레이리스트 수십 개 다운로드 중 일부 영상이 비공개, 삭제, 연령제한, 지역제한되더라도 다운로드 프로세스가 전체 취소(Abort)되지 않고, 해당 항목만 경고 로그 기록 후 **남은 영상 다운로드를 끝까지 완주**.
* **이어받기 및 덮어쓰기 (`continue_dl: True`, `overwrites: True`)**:  
  - 일반 다운로드: `archive.txt`를 참조하여 기존 완료 파일은 빠르게 건너뜀.
  - 새로받기 선택 시: `overwrites: True` 및 `ignore_archive=True`를 적용하여 이전 손상 파일 및 아카이브 기록을 무시하고 깨끗하게 재다운로드.
