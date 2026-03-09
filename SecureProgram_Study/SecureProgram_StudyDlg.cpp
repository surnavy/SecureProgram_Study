#include "pch.h"
#include "framework.h"
#include "SecureProgram_Study.h"
#include "SecureProgram_StudyDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ---- CAboutDlg ------------------------------------------------------------

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}
void CAboutDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// ---- CSecureProgramStudyDlg -----------------------------------------------

CSecureProgramStudyDlg::CSecureProgramStudyDlg(CWnd* pParent)
    : CDialogEx(IDD_SECUREPROGRAM_STUDY_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CSecureProgramStudyDlg::~CSecureProgramStudyDlg()
{
    m_monitor.Stop();
    delete m_pBaselineDB;
}

void CSecureProgramStudyDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_PATH,   m_editPath);
    DDX_Control(pDX, IDC_LIST_ALTERS, m_listAlerts);

    // IDC_EDIT_EXCLUDE 는 다이얼로그 에디터에 컨트롤 추가 후 활성화
    if (GetDlgItem(IDC_EDIT_EXCLUDE))
        DDX_Control(pDX, IDC_EDIT_EXCLUDE, m_editExclude);
}

BEGIN_MESSAGE_MAP(CSecureProgramStudyDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BTN_BROWSE,   &CSecureProgramStudyDlg::OnBnClickedBtnBrowse)
    ON_BN_CLICKED(IDC_BTN_BASELINE, &CSecureProgramStudyDlg::OnBnClickedBtnBaseline)
    ON_BN_CLICKED(IDC_BTN_START,    &CSecureProgramStudyDlg::OnBnClickedBtnStart)
    ON_BN_CLICKED(IDC_BTN_STOP,     &CSecureProgramStudyDlg::OnBnClickedBtnStop)
    ON_BN_CLICKED(IDC_BTN_VERIFY,        &CSecureProgramStudyDlg::OnBnClickedBtnVerify)
    ON_BN_CLICKED(IDC_BTN_CLEAR,         &CSecureProgramStudyDlg::OnBnClickedBtnClear)
    ON_BN_CLICKED(IDC_BTN_APPLY_PATTERN, &CSecureProgramStudyDlg::OnBnClickedBtnApplyPattern)
    ON_MESSAGE(WM_ALERT_NOTIFY,     &CSecureProgramStudyDlg::OnAlertNotify)
    ON_MESSAGE(WM_TRAYICON,         &CSecureProgramStudyDlg::OnTrayIcon)
    ON_COMMAND(ID_TRAY_SHOW,        &CSecureProgramStudyDlg::OnTrayShow)
    ON_COMMAND(ID_TRAY_EXIT,        &CSecureProgramStudyDlg::OnTrayExit)
END_MESSAGE_MAP()

// ---- 초기화 ---------------------------------------------------------------

BOOL CSecureProgramStudyDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu)
    {
        CString strAboutMenu;
        if (strAboutMenu.LoadString(IDS_ABOUTBOX))
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);
    SetWindowText(_T("파일 무결성 감시 프로그램"));

    // 최소화 버튼 추가
    ModifyStyle(0, WS_MINIMIZEBOX);

    InitListCtrl();
    GetDlgItem(IDC_BTN_STOP)->EnableWindow(FALSE);
    AddTrayIcon();

    // 기본 제외 패턴 설정 (컨트롤이 다이얼로그에 추가된 경우에만)
    if (m_editExclude.GetSafeHwnd())
        m_editExclude.SetWindowText(_T("*.tmp;*.bak;~$*;*.log;*.db"));

    SetStatusText(_T("준비"));
    return TRUE;
}

void CSecureProgramStudyDlg::InitListCtrl()
{
    m_listAlerts.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listAlerts.InsertColumn(0, _T("시각"),      LVCFMT_LEFT, 140);
    m_listAlerts.InsertColumn(1, _T("심각도"),    LVCFMT_LEFT,  60);
    m_listAlerts.InsertColumn(2, _T("이벤트"),    LVCFMT_LEFT,  80);
    m_listAlerts.InsertColumn(3, _T("파일 경로"), LVCFMT_LEFT, 280);
    m_listAlerts.InsertColumn(4, _T("상세"),      LVCFMT_LEFT, 200);
}

// ---- 버튼 핸들러 ----------------------------------------------------------

void CSecureProgramStudyDlg::OnBnClickedBtnBrowse()
{
    BROWSEINFO bi{};
    bi.hwndOwner = GetSafeHwnd();
    bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpszTitle = _T("감시할 폴더를 선택하세요");

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolder(&bi);
    if (pidl)
    {
        TCHAR path[MAX_PATH];
        if (SHGetPathFromIDList(pidl, path))
            m_editPath.SetWindowText(path);
        CoTaskMemFree(pidl);
    }
}

void CSecureProgramStudyDlg::OnBnClickedBtnBaseline()
{
    CString path;
    m_editPath.GetWindowText(path);
    if (path.IsEmpty())
    {
        AfxMessageBox(_T("감시할 폴더 경로를 먼저 입력하세요."), MB_ICONWARNING);
        return;
    }
    if (m_monitor.IsRunning())
    {
        AfxMessageBox(_T("감시 중에는 기준선을 생성할 수 없습니다.\n먼저 감시를 중지하세요."), MB_ICONWARNING);
        return;
    }

    // ── 진단: 경로와 파일 목록 확인 ──────────────────
    CString dbPath = GetBaselineFilePath();
    CString diagMsg;
    diagMsg.Format(_T("[진단]\n감시 경로: %s\nDB 경로: %s"), path, dbPath);
    AfxMessageBox(diagMsg, MB_ICONINFORMATION);

    SetStatusText(_T("기준선 생성 중..."));
    BeginWaitCursor();

    delete m_pBaselineDB;
    m_pBaselineDB = new BaselineDB(std::wstring(dbPath));
    bool ok = m_pBaselineDB->BuildFromDirectory(std::wstring(path), true);

    EndWaitCursor();

    if (ok)
    {
        bool saved = m_pBaselineDB->Save();
        CString msg;
        msg.Format(_T("기준선 생성 완료: %zu개 파일\n저장: %s"),
                   m_pBaselineDB->Count(),
                   saved ? _T("성공") : _T("실패"));
        AfxMessageBox(msg, MB_ICONINFORMATION);
        SetStatusText(msg);
    }
    else
    {
        // 파일이 0개인지 디렉토리 자체 문제인지 구분
        CString errMsg;
        errMsg.Format(_T("기준선 생성 실패\n경로 [%s] 에서 파일을 찾지 못했습니다.\n폴더가 비어있거나 접근 권한을 확인하세요."), path);
        AfxMessageBox(errMsg, MB_ICONERROR);
        SetStatusText(_T("기준선 생성 실패"));
    }
}

void CSecureProgramStudyDlg::OnBnClickedBtnStart()
{
    CString path;
    m_editPath.GetWindowText(path);
    if (path.IsEmpty())
    {
        AfxMessageBox(_T("감시할 폴더 경로를 먼저 입력하세요."), MB_ICONWARNING);
        return;
    }
    if (!EnsureBaselineDB()) return;
    if (m_monitor.IsRunning()) return;

    // 로그 파일 경로: DB 파일과 같은 위치에 .log 확장자
    CString logPath = GetBaselineFilePath();
    logPath.Replace(_T(".db"), _T(".log"));

    m_alertManager.Initialize(
        m_pBaselineDB,
        [this](const AlertRecord& record)
        {
            // 백그라운드 스레드 -> PostMessage로 UI 스레드에 전달
            auto* p = new AlertRecord(record);
            PostMessage(WM_ALERT_NOTIFY, 0, reinterpret_cast<LPARAM>(p));
        },
        std::wstring(logPath)
    );

    // DB 파일과 로그 파일은 감시 제외 (무한 루프 방지)
    m_alertManager.ClearExcludedPaths();
    m_alertManager.AddExcludedPath(std::wstring(GetBaselineFilePath()));
    m_alertManager.AddExcludedPath(std::wstring(logPath));

    bool ok = m_monitor.Start(
        std::wstring(path),
        [this](const FileEvent& evt) { m_alertManager.OnFileEvent(evt); },
        true
    );

    if (ok)
    {
        GetDlgItem(IDC_BTN_START)->EnableWindow(FALSE);
        GetDlgItem(IDC_BTN_STOP)->EnableWindow(TRUE);
        GetDlgItem(IDC_BTN_BASELINE)->EnableWindow(FALSE);
        SetStatusText(_T("감시 중..."));
    }
    else
        AfxMessageBox(_T("감시 시작 실패: 폴더 접근 권한을 확인하세요."), MB_ICONERROR);
}

void CSecureProgramStudyDlg::OnBnClickedBtnStop()
{
    m_alertManager.SetSuppressed(true);
    m_monitor.Stop();
    m_alertManager.SetSuppressed(false);

    GetDlgItem(IDC_BTN_START)->EnableWindow(TRUE);
    GetDlgItem(IDC_BTN_STOP)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_BASELINE)->EnableWindow(TRUE);
    SetStatusText(_T("감시 중지됨"));
}

void CSecureProgramStudyDlg::OnBnClickedBtnVerify()
{
    if (!EnsureBaselineDB()) return;

    SetStatusText(_T("전체 무결성 검사 중..."));
    BeginWaitCursor();

    auto diffs = m_pBaselineDB->VerifyAll();
    m_alertManager.OnDiffResults(diffs);

    CString msg;
    if (diffs.empty())
        msg = _T("전체 검사 완료: 이상 없음");
    else
        msg.Format(_T("전체 검사 완료: %zu건 이상 탐지"), diffs.size());
    SetStatusText(msg);
    EndWaitCursor();
}

void CSecureProgramStudyDlg::OnBnClickedBtnClear()
{
    m_listAlerts.DeleteAllItems();
    m_alertManager.ClearHistory();
    SetStatusText(_T("로그 초기화됨"));
}

// ---- WM_ALERT_NOTIFY (UI 스레드) ------------------------------------------

LRESULT CSecureProgramStudyDlg::OnAlertNotify(WPARAM, LPARAM lParam)
{
    auto* p = reinterpret_cast<AlertRecord*>(lParam);
    if (p) { AddAlertToList(*p); delete p; }
    return 0;
}

// ---- 내부 헬퍼 ------------------------------------------------------------

void CSecureProgramStudyDlg::AddAlertToList(const AlertRecord& record)
{
    // 최신 항목이 위로 오도록 0번 위치에 삽입
    int row = m_listAlerts.InsertItem(0, CString(record.time.c_str()));

    CString sev;
    switch (record.severity)
    {
    case AlertSeverity::Critical: sev = _T("[위험]"); break;
    case AlertSeverity::Warning:  sev = _T("[주의]"); break;
    default:                      sev = _T("[정보]"); break;
    }

    m_listAlerts.SetItemText(row, 1, sev);
    m_listAlerts.SetItemText(row, 2, CString(record.eventType.c_str()));
    m_listAlerts.SetItemText(row, 3, CString(record.filePath.c_str()));
    m_listAlerts.SetItemText(row, 4, CString(record.detail.c_str()));

    CString status;
    status.Format(_T("마지막 이벤트: %s [%s]"),
                  CString(record.time.c_str()),
                  CString(record.eventType.c_str()));
    SetStatusText(status);

    // 창이 숨겨진 상태일 때 트레이 풍선 알림
    if (!IsWindowVisible() && record.severity == AlertSeverity::Critical)
    {
        DWORD icon = (record.severity == AlertSeverity::Critical) ? NIIF_ERROR : NIIF_WARNING;
        ShowTrayBalloon(CString(record.eventType.c_str()),
                        CString(record.filePath.c_str()), icon);
    }
}

void CSecureProgramStudyDlg::SetStatusText(const CString& text)
{
    CWnd* p = GetDlgItem(IDC_STATIC_STATUS);
    if (p) p->SetWindowText(text);
}

bool CSecureProgramStudyDlg::EnsureBaselineDB()
{
    if (m_pBaselineDB && m_pBaselineDB->Count() > 0) return true;

    CString dbPath = GetBaselineFilePath();
    if (!dbPath.IsEmpty() && PathFileExists(dbPath))
    {
        delete m_pBaselineDB;
        m_pBaselineDB = new BaselineDB(std::wstring(dbPath));
        if (m_pBaselineDB->Load() && m_pBaselineDB->Count() > 0) return true;
    }

    AfxMessageBox(_T("기준선이 없습니다.\n'기준선 생성' 버튼을 먼저 클릭하세요."), MB_ICONWARNING);
    return false;
}

CString CSecureProgramStudyDlg::GetBaselineFilePath() const
{
    CString path;
    m_editPath.GetWindowText(path);
    if (path.IsEmpty()) return _T("");
    if (path.Right(1) != _T("\\")) path += _T("\\");
    path += _T("fim_baseline.db");
    return path;
}

// ---- MFC 표준 메시지 처리기 -----------------------------------------------

void CSecureProgramStudyDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    { CAboutDlg dlgAbout; dlgAbout.DoModal(); }
    else
        CDialogEx::OnSysCommand(nID, lParam);
}

void CSecureProgramStudyDlg::OnDestroy()
{
    m_monitor.Stop();
    RemoveTrayIcon();
    CDialogEx::OnDestroy();
}

void CSecureProgramStudyDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        dc.DrawIcon((rect.Width() - cxIcon + 1) / 2,
                    (rect.Height() - cyIcon + 1) / 2, m_hIcon);
    }
    else
        CDialogEx::OnPaint();
}

HCURSOR CSecureProgramStudyDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

// ---- 트레이 아이콘 --------------------------------------------------------

void CSecureProgramStudyDlg::AddTrayIcon()
{
    m_nid.cbSize           = sizeof(NOTIFYICONDATA);
    m_nid.hWnd             = GetSafeHwnd();
    m_nid.uID              = 1;
    m_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon            = m_hIcon;
    _tcscpy_s(m_nid.szTip, _T("파일 무결성 감시 프로그램"));
    Shell_NotifyIcon(NIM_ADD, &m_nid);
}

void CSecureProgramStudyDlg::RemoveTrayIcon()
{
    Shell_NotifyIcon(NIM_DELETE, &m_nid);
}

void CSecureProgramStudyDlg::ShowTrayBalloon(const CString& title,
                                              const CString& msg,
                                              DWORD icon)
{
    m_nid.uFlags      = NIF_INFO;
    m_nid.dwInfoFlags = icon;
    m_nid.uTimeout    = 5000;
    _tcscpy_s(m_nid.szInfoTitle, title);
    _tcscpy_s(m_nid.szInfo,      msg);
    Shell_NotifyIcon(NIM_MODIFY, &m_nid);

    // 다음 호출을 위해 플래그 복원
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
}

void CSecureProgramStudyDlg::ShowMainWindow()
{
    ShowWindow(SW_SHOW);
    ShowWindow(SW_RESTORE);
    SetForegroundWindow();
}

void CSecureProgramStudyDlg::HideToTray()
{
    ShowWindow(SW_HIDE);
}

// 최소화 시 트레이로 숨기기
void CSecureProgramStudyDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    if (nType == SIZE_MINIMIZED)
        HideToTray();
}

// 트레이 아이콘 클릭/더블클릭 처리
LRESULT CSecureProgramStudyDlg::OnTrayIcon(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(lParam))
    {
    case WM_LBUTTONDBLCLK:
    case NIN_BALLOONUSERCLICK:  // 풍선 알림 클릭
        ShowMainWindow();
        break;

    case WM_RBUTTONUP:
    {
        // 우클릭 컨텍스트 메뉴 표시
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, ID_TRAY_SHOW, _T("열기"));
        menu.AppendMenu(MF_SEPARATOR);
        menu.AppendMenu(MF_STRING, ID_TRAY_EXIT, _T("종료"));

        // 메뉴가 올바르게 닫히도록 포그라운드로 설정
        SetForegroundWindow();
        POINT pt;
        GetCursorPos(&pt);
        menu.TrackPopupMenu(TPM_RIGHTBUTTON, pt.x, pt.y, this);
        break;
    }
    }
    return 0;
}

void CSecureProgramStudyDlg::OnTrayShow()
{
    ShowMainWindow();
}

void CSecureProgramStudyDlg::OnTrayExit()
{
    RemoveTrayIcon();
    DestroyWindow();
}

// ---- 제외 패턴 적용 -------------------------------------------------------

void CSecureProgramStudyDlg::OnBnClickedBtnApplyPattern()
{
    if (!m_editExclude.GetSafeHwnd()) return;

    CString input;
    m_editExclude.GetWindowText(input);

    // 세미콜론으로 분리해 패턴 목록 생성
    std::vector<std::wstring> patterns;
    int pos = 0;
    CString token;
    while ((token = input.Tokenize(_T(";"), pos)) != _T(""))
    {
        token.Trim();
        if (!token.IsEmpty())
            patterns.push_back(std::wstring(token));
    }

    m_alertManager.SetExcludedPatterns(patterns);

    CString msg;
    msg.Format(_T("제외 패턴 %zu개 적용됨"), patterns.size());
    SetStatusText(msg);
}
