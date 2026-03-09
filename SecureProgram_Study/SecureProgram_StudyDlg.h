#pragma once

#include "FileMonitor.h"
#include "BaselineDB.h"
#include "AlertManager.h"

// AlertManager 콜백(백그라운드 스레드) -> UI 스레드 마샬링용 메시지
// LPARAM: AlertRecord* (new로 할당, 수신측에서 delete)
#define WM_ALERT_NOTIFY  0x8001
#define WM_TRAYICON      0x8002  // 트레이 아이콘 콜백 메시지

// 트레이 아이콘 컨텍스트 메뉴 ID
#define ID_TRAY_SHOW     1100
#define ID_TRAY_EXIT     1101

class CSecureProgramStudyDlg : public CDialogEx
{
public:
    CSecureProgramStudyDlg(CWnd* pParent = nullptr);
    ~CSecureProgramStudyDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SECUREPROGRAM_STUDY_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

protected:
    HICON m_hIcon;

    // 핵심 모듈
    FileMonitor     m_monitor;
    BaselineDB*     m_pBaselineDB = nullptr;
    AlertManager    m_alertManager;

    // 컨트롤 변수
    CEdit           m_editPath;
    CEdit           m_editExclude;  // 제외 패턴 입력
    CListCtrl       m_listAlerts;

    // 내부 헬퍼
    void    InitListCtrl();
    void    AddAlertToList(const AlertRecord& record);
    void    SetStatusText(const CString& text);
    bool    EnsureBaselineDB();
    CString GetBaselineFilePath() const;

    // 트레이 아이콘
    void    AddTrayIcon();
    void    RemoveTrayIcon();
    void    ShowTrayBalloon(const CString& title, const CString& msg, DWORD icon = NIIF_WARNING);
    void    ShowMainWindow();
    void    HideToTray();

    NOTIFYICONDATA m_nid{};

    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnDestroy();
    afx_msg void OnSize(UINT nType, int cx, int cy);  // 최소화 시 트레이로

    // 버튼 핸들러
    afx_msg void OnBnClickedBtnBrowse();
    afx_msg void OnBnClickedBtnBaseline();
    afx_msg void OnBnClickedBtnStart();
    afx_msg void OnBnClickedBtnStop();
    afx_msg void OnBnClickedBtnVerify();
    afx_msg void OnBnClickedBtnClear();
    afx_msg void OnBnClickedBtnApplyPattern();

    // 사용자 정의 메시지 핸들러
    afx_msg LRESULT OnAlertNotify(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTrayIcon(WPARAM wParam, LPARAM lParam);

    // 트레이 컨텍스트 메뉴
    afx_msg void OnTrayShow();
    afx_msg void OnTrayExit();

    DECLARE_MESSAGE_MAP()
};
