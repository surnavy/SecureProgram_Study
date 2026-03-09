#pragma once

#include "BaselineDB.h"
#include "FileMonitor.h"

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <unordered_set>

// 경보 심각도
enum class AlertSeverity
{
    Info,       // 일반 정보 (파일 추가 등)
    Warning,    // 주의 (예상치 못한 변경)
    Critical,   // 위험 (파일 변조/삭제)
};

// 경보 항목
struct AlertRecord
{
    AlertSeverity   severity;
    std::wstring    time;       // 발생 시각 "YYYY-MM-DD HH:MM:SS"
    std::wstring    eventType;  // "변조", "추가", "삭제", "이름변경"
    std::wstring    filePath;
    std::wstring    detail;
};

// UI 스레드로 경보를 전달하는 콜백 타입
using AlertCallback = std::function<void(const AlertRecord&)>;

// 파일 무결성 경보 관리자
// FileMonitor 콜백에서 무결성 검사 후 경보 발생 + 로그 기록
class AlertManager
{
public:
    AlertManager();
    ~AlertManager() = default;

    // 초기화: BaselineDB와 UI 콜백 연결
    // logFilePath: 빈 문자열이면 파일 저장 안 함
    void Initialize(BaselineDB* pBaselineDB,
                    AlertCallback uiCallback,
                    const std::wstring& logFilePath = L"");

    // FileMonitor의 FileEventCallback으로 등록 가능 (백그라운드 스레드 호출)
    void OnFileEvent(const FileEvent& evt);

    // BaselineDB::VerifyAll() 결과 일괄 처리
    void OnDiffResults(const std::vector<DiffResult>& diffs);

    std::vector<AlertRecord> GetAlertHistory() const;
    void ClearHistory();

    void SetLogFilePath(const std::wstring& path) { m_logFilePath = path; }
    void SetSuppressed(bool suppress) { m_suppressed = suppress; }

    // 감시에서 제외할 경로 등록 (DB, 로그 파일 등 내부 파일)
    void AddExcludedPath(const std::wstring& path);
    void ClearExcludedPaths();

    // 제외 패턴 등록 (예: *.tmp, ~$*, *.bak)
    // 패턴에 '\'가 없으면 파일명에만 매칭, 있으면 전체 경로에 매칭
    void SetExcludedPatterns(const std::vector<std::wstring>& patterns);
    const std::vector<std::wstring>& GetExcludedPatterns() const { return m_excludedPatterns; }

private:
    void RaiseAlert(AlertSeverity severity,
                    const std::wstring& eventType,
                    const std::wstring& filePath,
                    const std::wstring& detail);

    void WriteToLog(const AlertRecord& record) const;

    static std::wstring GetCurrentTimeString();
    static std::wstring DiffTypeToString(DiffType type);
    static std::wstring SeverityToString(AlertSeverity sev);

    BaselineDB*              m_pBaselineDB = nullptr;
    AlertCallback            m_uiCallback;
    std::wstring             m_logFilePath;
    bool                     m_suppressed  = false;

    mutable std::mutex       m_mutex;
    std::vector<AlertRecord> m_history;

    // 제외 경로 목록 (소문자 정규화)
    std::unordered_set<std::wstring> m_excludedPaths;

    // 제외 패턴 목록 (소문자 정규화)
    std::vector<std::wstring> m_excludedPatterns;

    // 와일드카드 패턴 매칭 (* 와 ? 지원)
    static bool MatchPattern(const std::wstring& str, const std::wstring& pattern);
};
