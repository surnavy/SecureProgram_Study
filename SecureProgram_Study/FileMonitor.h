#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>

// 파일 변경 이벤트 종류
enum class FileEventType
{
    Added,      // 파일 추가
    Removed,    // 파일 삭제
    Modified,   // 파일 수정
    Renamed,    // 파일 이름 변경
};

// 파일 변경 이벤트 정보
struct FileEvent
{
    FileEventType   type;
    std::wstring    filePath;       // 전체 경로
    std::wstring    oldFilePath;    // 이름 변경 시 이전 경로
};

// 이벤트 콜백 타입
using FileEventCallback = std::function<void(const FileEvent&)>;

// 실시간 파일 감시 클래스
// ReadDirectoryChangesW API를 백그라운드 스레드에서 실행
class FileMonitor
{
public:
    FileMonitor();
    ~FileMonitor();

    // 감시 시작 (watchSubtree: 하위 디렉토리 포함 여부)
    bool Start(const std::wstring& watchPath,
               FileEventCallback callback,
               bool watchSubtree = true);

    // 감시 중지 (블로킹: 스레드 종료 대기)
    void Stop();

    bool IsRunning() const { return m_running.load(); }
    const std::wstring& GetWatchPath() const { return m_watchPath; }

private:
    static DWORD WINAPI MonitorThreadProc(LPVOID lpParam);
    void MonitorLoop();

    std::wstring        m_watchPath;
    FileEventCallback   m_callback;
    bool                m_watchSubtree = true;

    HANDLE              m_hThread       = INVALID_HANDLE_VALUE;
    HANDLE              m_hDir          = INVALID_HANDLE_VALUE;
    HANDLE              m_hStopEvent    = INVALID_HANDLE_VALUE;  // 종료 신호용

    std::atomic<bool>   m_running{ false };
};
