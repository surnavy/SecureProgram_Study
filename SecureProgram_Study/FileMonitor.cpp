#include "pch.h"
#include "FileMonitor.h"

// ReadDirectoryChangesW 버퍼 크기 (64KB)
static constexpr DWORD NOTIFY_BUFFER_SIZE = 64 * 1024;

// 감시할 변경 종류 플래그
static constexpr DWORD NOTIFY_FILTER =
    FILE_NOTIFY_CHANGE_FILE_NAME  |   // 파일 생성/삭제/이름변경
    FILE_NOTIFY_CHANGE_DIR_NAME   |   // 디렉토리 생성/삭제
    FILE_NOTIFY_CHANGE_SIZE       |   // 파일 크기 변경
    FILE_NOTIFY_CHANGE_LAST_WRITE |   // 마지막 수정 시간 변경
    FILE_NOTIFY_CHANGE_CREATION;      // 생성 시간 변경

FileMonitor::FileMonitor() = default;

FileMonitor::~FileMonitor()
{
    Stop();
}

bool FileMonitor::Start(const std::wstring& watchPath,
                        FileEventCallback callback,
                        bool watchSubtree)
{
    if (m_running.load()) return false;

    // FILE_FLAG_BACKUP_SEMANTICS: 디렉토리 핸들 열기에 필요
    // FILE_FLAG_OVERLAPPED: 비동기 I/O 사용
    m_hDir = CreateFileW(
        watchPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );
    if (m_hDir == INVALID_HANDLE_VALUE) return false;

    // 스레드 종료 신호용 이벤트
    m_hStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (m_hStopEvent == INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hDir);
        m_hDir = INVALID_HANDLE_VALUE;
        return false;
    }

    m_watchPath    = watchPath;
    m_callback     = callback;
    m_watchSubtree = watchSubtree;
    m_running      = true;

    m_hThread = CreateThread(nullptr, 0, MonitorThreadProc, this, 0, nullptr);
    if (m_hThread == INVALID_HANDLE_VALUE)
    {
        m_running = false;
        CloseHandle(m_hStopEvent);
        CloseHandle(m_hDir);
        m_hStopEvent = m_hDir = INVALID_HANDLE_VALUE;
        return false;
    }
    return true;
}

void FileMonitor::Stop()
{
    if (!m_running.load()) return;
    m_running = false;

    if (m_hStopEvent != INVALID_HANDLE_VALUE)
        SetEvent(m_hStopEvent);

    if (m_hThread != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(m_hThread, 5000);
        CloseHandle(m_hThread);
        m_hThread = INVALID_HANDLE_VALUE;
    }
    if (m_hStopEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hStopEvent);
        m_hStopEvent = INVALID_HANDLE_VALUE;
    }
    if (m_hDir != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hDir);
        m_hDir = INVALID_HANDLE_VALUE;
    }
}

// 스레드 진입점 (static) -> 인스턴스 메서드로 위임
DWORD WINAPI FileMonitor::MonitorThreadProc(LPVOID lpParam)
{
    static_cast<FileMonitor*>(lpParam)->MonitorLoop();
    return 0;
}

void FileMonitor::MonitorLoop()
{
    std::vector<BYTE> buffer(NOTIFY_BUFFER_SIZE);
    OVERLAPPED overlapped{};
    overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (overlapped.hEvent == INVALID_HANDLE_VALUE) { m_running = false; return; }

    // 종료 신호와 I/O 완료를 동시에 대기
    HANDLE waitHandles[2] = { m_hStopEvent, overlapped.hEvent };

    while (m_running.load())
    {
        DWORD bytesReturned = 0;
        ResetEvent(overlapped.hEvent);

        BOOL result = ReadDirectoryChangesW(
            m_hDir,
            buffer.data(), NOTIFY_BUFFER_SIZE,
            m_watchSubtree ? TRUE : FALSE,
            NOTIFY_FILTER,
            &bytesReturned,
            &overlapped,
            nullptr
        );
        if (!result) break;

        DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);

        if (waitResult == WAIT_OBJECT_0)    // 종료 이벤트
        {
            CancelIo(m_hDir);
            break;
        }
        if (waitResult != WAIT_OBJECT_0 + 1) break;

        if (!GetOverlappedResult(m_hDir, &overlapped, &bytesReturned, FALSE)) break;
        if (bytesReturned == 0) continue;

        // 버퍼에서 이벤트 파싱
        const BYTE* ptr = buffer.data();
        std::wstring prevRenamedPath;   // RENAMED_OLD_NAME 임시 저장

        while (true)
        {
            const auto* info = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(ptr);
            std::wstring fileName(info->FileName, info->FileNameLength / sizeof(WCHAR));
            std::wstring fullPath = m_watchPath + L"\\" + fileName;

            FileEvent evt;
            evt.filePath = fullPath;

            switch (info->Action)
            {
            case FILE_ACTION_ADDED:
                evt.type = FileEventType::Added;
                if (m_callback) m_callback(evt);
                break;
            case FILE_ACTION_REMOVED:
                evt.type = FileEventType::Removed;
                if (m_callback) m_callback(evt);
                break;
            case FILE_ACTION_MODIFIED:
                evt.type = FileEventType::Modified;
                if (m_callback) m_callback(evt);
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                prevRenamedPath = fullPath;
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                evt.type        = FileEventType::Renamed;
                evt.oldFilePath = prevRenamedPath;
                if (m_callback) m_callback(evt);
                prevRenamedPath.clear();
                break;
            }

            if (info->NextEntryOffset == 0) break;
            ptr += info->NextEntryOffset;
        }
    }

    CloseHandle(overlapped.hEvent);
    m_running = false;
}
