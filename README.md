# 실시간 파일 무결성 감시 프로그램 (File Integrity Monitor)

MFC(Microsoft Foundation Classes)를 사용한 Windows 기반 실시간 파일 무결성 감시 프로그램입니다.

## 개요

지정한 폴더 내 파일의 SHA-256 해시를 기준선으로 저장하고, 파일 변경을 실시간으로 감지하여 변조·추가·삭제 이벤트를 경보합니다.

## 주요 기능

- **기준선 생성** — 감시 폴더의 모든 파일에 대한 SHA-256 해시 스냅샷 생성 및 저장
- **실시간 감시** — `ReadDirectoryChangesW` API를 사용한 백그라운드 스레드 기반 파일 변경 감지
- **무결성 검사** — 기준선 해시와 현재 해시 비교를 통한 변조 탐지
- **전체 검사** — 감시 중지 중 발생한 변경 사항 일괄 점검
- **경보 로그** — 이벤트를 UI 리스트와 로그 파일에 기록

## 탐지 이벤트

| 이벤트 | 심각도 | 설명 |
|--------|--------|------|
| 변조 | 위험 | 기준선 해시와 현재 해시 불일치 |
| 삭제 | 위험 | 기준선에 등록된 파일 삭제 |
| 추가 | 주의 | 기준선에 없는 새 파일 생성 |
| 이름변경 | 주의/정보 | 파일 이름 변경 |

## 프로젝트 구조

```
SecureProgram_Study/
├── HashEngine.h / .cpp         # SHA-256 해시 계산 (BCrypt API)
├── FileMonitor.h / .cpp        # 실시간 파일 감시 (ReadDirectoryChangesW)
├── BaselineDB.h / .cpp         # 기준선 DB 저장 및 비교
├── AlertManager.h / .cpp       # 경보 탐지 및 로그 관리
├── SecureProgram_StudyDlg.h / .cpp  # MFC 메인 대화상자 (UI)
└── SecureProgram_Study.vcxproj
```

## 개발 환경

| 항목 | 내용 |
|------|------|
| IDE | Visual Studio 2022 |
| 프레임워크 | MFC (Microsoft Foundation Classes) |
| 언어 | C++17 |
| 운영체제 | Windows 11 |
| 문자 집합 | Unicode |

## 빌드 방법

1. `SecureProgram_Study.sln` 을 Visual Studio 2022에서 열기
2. 구성: `Debug | x64` 또는 `Release | x64` 선택
3. **빌드 → 솔루션 다시 빌드 (Rebuild Solution)**

> **요구사항:** Visual Studio 2022 + MFC 구성 요소 설치 필요

## 사용 방법

1. **찾아보기** 버튼으로 감시할 폴더 선택
2. **기준선 생성** 클릭 — 현재 파일 상태를 기준선으로 저장
3. **감시 시작** 클릭 — 실시간 감시 시작
4. 파일 변경 발생 시 하단 리스트에 경보 표시
5. **전체 검사** 로 현재 상태와 기준선 전체 비교 가능

## 생성 파일

감시 폴더 내부에 아래 파일이 자동 생성됩니다. (감시 대상 제외)

| 파일 | 설명 |
|------|------|
| `fim_baseline.db` | 기준선 해시 DB (탭 구분 텍스트) |
| `fim_baseline.log` | 경보 이벤트 로그 |

## 사용 기술

- **Windows BCrypt API** — SHA-256 해시 계산
- **ReadDirectoryChangesW** — 실시간 디렉토리 변경 감지
- **std::filesystem (C++17)** — 파일/디렉토리 열거
- **PostMessage / WM_APP** — 백그라운드 스레드 → UI 스레드 안전한 메시지 전달
- **std::mutex** — 경보 이력 스레드 안전 접근
