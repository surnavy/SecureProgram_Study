# SecureProgram_Study

MFC를 사용한 **실시간 파일 무결성 감시 프로그램 (File Integrity Monitor)**.

## 개발 환경

- **IDE**: Visual Studio 2022
- **프레임워크**: MFC (Microsoft Foundation Classes)
- **언어**: C++ (C++17)
- **OS**: Windows 11

## 프로젝트 구조

```
SecureProgram_Study/
├── SecureProgram_Study.sln
├── SecureProgram_Study/
│   ├── SecureProgram_Study.cpp / .h       # 앱 클래스
│   ├── SecureProgram_StudyDlg.cpp / .h    # 메인 대화상자 (탭 컨테이너)
│   ├── FileMonitor.cpp / .h               # 핵심: ReadDirectoryChangesW 감시 스레드
│   ├── HashEngine.cpp / .h                # SHA-256 해시 계산 (BCrypt API)
│   ├── BaselineDB.cpp / .h                # 기준선 DB 저장/로드 (SQLite or JSON)
│   ├── AlertManager.cpp / .h              # 변조 탐지 및 알림 처리
│   ├── LogManager.cpp / .h                # 이벤트 로그 기록
│   ├── SettingsDlg.cpp / .h               # 설정 대화상자
│   └── resource.h                         # 리소스 ID 정의
└── CLAUDE.md
```

## 코딩 컨벤션

- MFC 헝가리언 표기법 사용 (m_, p_, n_, str_ 등)
- 리소스 ID 접두사: `IDC_` (컨트롤), `IDD_` (대화상자), `IDR_` (메뉴/리본)
- 멤버 변수: `m_` 접두사
- 포인터: `p` 접두사

## 보안 관련 지침

- 암호화: Windows CryptoAPI (BCrypt) 또는 OpenSSL 사용
- 민감한 데이터는 사용 후 메모리에서 즉시 제거 (`SecureZeroMemory`)
- 버퍼 오버플로우 방지: `_s` 접미사 함수 사용 (`strcpy_s`, `sprintf_s` 등)
- 입력값 항상 검증

## 주요 기능

### 핵심 기능
- [ ] 감시 디렉토리 등록/제거
- [ ] 실시간 파일 변경 감지 (`ReadDirectoryChangesW` API)
- [ ] SHA-256 해시 기반 파일 무결성 검사
- [ ] 기준선(Baseline) 스냅샷 생성 및 저장
- [ ] 변조/추가/삭제 이벤트 탐지 및 알림

### UI
- [ ] 감시 대상 목록 (CListCtrl)
- [ ] 실시간 이벤트 로그 뷰 (CListCtrl)
- [ ] 감시 시작/중지 버튼
- [ ] 트레이 아이콘 (백그라운드 실행)
- [ ] 설정 다이얼로그 (해시 알고리즘, 알림 방식 등)

### 알림
- [ ] 윈도우 토스트 알림
- [ ] 이벤트 로그 파일 저장

## 핵심 Windows API

- `ReadDirectoryChangesW` - 실시간 디렉토리 변경 감지
- `BCryptHashData` (BCrypt) - SHA-256 해시 계산
- `CreateThread` / `CWinThread` - 감시 백그라운드 스레드
- `Shell_NotifyIcon` - 시스템 트레이 아이콘
