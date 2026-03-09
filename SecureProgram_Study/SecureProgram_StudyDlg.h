#pragma once

#include "FileMonitor.h"
#include "BaselineDB.h"
#include "AlertManager.h"

// AlertManager 콜백(백그라운드 스레드) -> UI 스레드 마샬링용 메시지
// LPARAM: AlertRecord* (new로 할당, 수신측에서 delete)
#define WM_ALERT_NOTIFY  0x8001

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
    CListCtrl       m_listAlerts;

    // 내부 헬퍼
    void    InitListCtrl();
    void    AddAlertToList(const AlertRecord& record);
    void    SetStatusText(const CString& text);
    bool    EnsureBaselineDB();
    CString GetBaselineFilePath() const;

    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnDestroy();

    // 버튼 핸들러
    afx_msg void OnBnClickedBtnBrowse();
    afx_msg void OnBnClickedBtnBaseline();
    afx_msg void OnBnClickedBtnStart();
    afx_msg void OnBnClickedBtnStop();
    afx_msg void OnBnClickedBtnVerify();
    afx_msg void OnBnClickedBtnClear();

    // 사용자 정의 메시지 핸들러
    afx_msg LRESULT OnAlertNotify(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};
