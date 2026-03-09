#include "pch.h"
#include "AlertManager.h"
#include "HashEngine.h"

#include <fstream>
#include <algorithm>

AlertManager::AlertManager() = default;

void AlertManager::Initialize(BaselineDB* pBaselineDB,
                               AlertCallback uiCallback,
                               const std::wstring& logFilePath)
{
    m_pBaselineDB = pBaselineDB;
    m_uiCallback  = uiCallback;
    m_logFilePath = logFilePath;
}

// ---- 제외 경로 관리 -------------------------------------------------------

void AlertManager::AddExcludedPath(const std::wstring& path)
{
    std::wstring key = path;
    std::transform(key.begin(), key.end(), key.begin(), ::towlower);
    m_excludedPaths.insert(key);
}

void AlertManager::ClearExcludedPaths()
{
    m_excludedPaths.clear();
}

// ---- FileMonitor 콜백 처리 ------------------------------------------------

void AlertManager::OnFileEvent(const FileEvent& evt)
{
    if (m_suppressed || !m_pBaselineDB) return;

    // 제외 경로 확인
    std::wstring key = evt.filePath;
    std::transform(key.begin(), key.end(), key.begin(), ::towlower);
    if (m_excludedPaths.count(key)) return;

    switch (evt.type)
    {
    case FileEventType::Modified:
    {
        const BaselineEntry* entry = m_pBaselineDB->Find(evt.filePath);
        if (!entry)
        {
            // 기준선에 없는 파일이 수정됨
            RaiseAlert(AlertSeverity::Info, L"추가(미등록)", evt.filePath,
                       L"기준선에 등록되지 않은 파일입니다.");
            break;
        }
        std::wstring cur = HashEngine::ComputeFileHash(evt.filePath);
        if (!HashEngine::CompareHash(entry->hash, cur))
        {
            std::wstring detail = L"기준선: " + entry->hash.substr(0, 16) + L"... | "
                                + L"현재값: " + (cur.empty() ? L"(읽기 실패)" : cur.substr(0, 16) + L"...");
            RaiseAlert(AlertSeverity::Critical, L"변조", evt.filePath, detail);
        }
        break;
    }
    case FileEventType::Added:
        if (!m_pBaselineDB->Contains(evt.filePath))
        {
            std::wstring hash = HashEngine::ComputeFileHash(evt.filePath);
            RaiseAlert(AlertSeverity::Warning, L"추가", evt.filePath,
                       L"해시: " + (hash.empty() ? L"(계산 실패)" : hash));
        }
        break;

    case FileEventType::Removed:
        if (m_pBaselineDB->Contains(evt.filePath))
        {
            const BaselineEntry* e = m_pBaselineDB->Find(evt.filePath);
            RaiseAlert(AlertSeverity::Critical, L"삭제", evt.filePath,
                       L"기준선 해시: " + (e ? e->hash.substr(0, 16) + L"..." : L"알 수 없음"));
        }
        break;

    case FileEventType::Renamed:
    {
        AlertSeverity sev = m_pBaselineDB->Contains(evt.oldFilePath)
                            ? AlertSeverity::Warning : AlertSeverity::Info;
        RaiseAlert(sev, L"이름변경", evt.filePath, L"이전 경로: " + evt.oldFilePath);
        break;
    }
    }
}

// ---- VerifyAll() 결과 일괄 처리 -------------------------------------------

void AlertManager::OnDiffResults(const std::vector<DiffResult>& diffs)
{
    if (m_suppressed) return;

    for (const auto& diff : diffs)
    {
        AlertSeverity sev = AlertSeverity::Info;
        std::wstring detail;

        switch (diff.type)
        {
        case DiffType::Modified:
            sev    = AlertSeverity::Critical;
            detail = L"기준선: " + diff.expectedHash.substr(0, 16) + L"... | "
                   + L"현재값: " + (diff.actualHash.empty() ? L"(읽기 실패)" : diff.actualHash.substr(0, 16) + L"...");
            break;
        case DiffType::Added:
            sev    = AlertSeverity::Warning;
            detail = L"해시: " + (diff.actualHash.empty() ? L"(계산 실패)" : diff.actualHash);
            break;
        case DiffType::Removed:
            sev    = AlertSeverity::Critical;
            detail = L"기준선 해시: " + diff.expectedHash.substr(0, 16) + L"...";
            break;
        }
        RaiseAlert(sev, DiffTypeToString(diff.type), diff.filePath, detail);
    }
}

// ---- 이력 -----------------------------------------------------------------

std::vector<AlertRecord> AlertManager::GetAlertHistory() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_history;
}

void AlertManager::ClearHistory()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_history.clear();
}

// ---- 내부 -----------------------------------------------------------------

void AlertManager::RaiseAlert(AlertSeverity severity,
                               const std::wstring& eventType,
                               const std::wstring& filePath,
                               const std::wstring& detail)
{
    AlertRecord record;
    record.severity  = severity;
    record.time      = GetCurrentTimeString();
    record.eventType = eventType;
    record.filePath  = filePath;
    record.detail    = detail;

    { std::lock_guard<std::mutex> lock(m_mutex); m_history.push_back(record); }

    if (!m_logFilePath.empty()) WriteToLog(record);
    if (m_uiCallback) m_uiCallback(record);
}

void AlertManager::WriteToLog(const AlertRecord& record) const
{
    std::wofstream ofs(m_logFilePath, std::ios::app);
    if (!ofs.is_open()) return;

    ofs << L"[" << record.time      << L"] "
        << L"[" << SeverityToString(record.severity) << L"] "
        << L"[" << record.eventType << L"] "
        << record.filePath << L" | " << record.detail << L"\n";
}

std::wstring AlertManager::GetCurrentTimeString()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buf[32];
    swprintf_s(buf, L"%04d-%02d-%02d %02d:%02d:%02d",
               st.wYear, st.wMonth, st.wDay,
               st.wHour, st.wMinute, st.wSecond);
    return buf;
}

std::wstring AlertManager::DiffTypeToString(DiffType type)
{
    switch (type)
    {
    case DiffType::Modified: return L"변조";
    case DiffType::Added:    return L"추가";
    case DiffType::Removed:  return L"삭제";
    default:                 return L"알 수 없음";
    }
}

std::wstring AlertManager::SeverityToString(AlertSeverity sev)
{
    switch (sev)
    {
    case AlertSeverity::Info:     return L"INFO";
    case AlertSeverity::Warning:  return L"WARN";
    case AlertSeverity::Critical: return L"CRIT";
    default:                      return L"UNKN";
    }
}
