#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// 단일 파일의 기준선 정보
struct BaselineEntry
{
    std::wstring filePath;
    std::wstring hash;          // SHA-256 16진수 문자열
    std::wstring lastModified;  // 기준선 생성 시각 "YYYY-MM-DD HH:MM:SS"
};

// 기준선 비교 결과 종류
enum class DiffType
{
    Modified,   // 해시 변경 (파일 변조)
    Added,      // 기준선에 없던 파일
    Removed,    // 기준선 파일이 삭제됨
};

struct DiffResult
{
    DiffType        type;
    std::wstring    filePath;
    std::wstring    expectedHash;   // 기준선 해시 (Modified/Removed)
    std::wstring    actualHash;     // 현재 해시  (Added/Modified)
};

// 파일 무결성 기준선 DB
// 탭 구분 텍스트 형식으로 저장: HASH\t타임스탬프\t파일경로
class BaselineDB
{
public:
    explicit BaselineDB(const std::wstring& dbFilePath);

    // --- 기준선 구축 ---
    void AddOrUpdate(const std::wstring& filePath, const std::wstring& hash);
    bool BuildFromDirectory(const std::wstring& dirPath, bool watchSubtree = true);
    void Remove(const std::wstring& filePath);
    void Clear();

    // --- 영속성 ---
    bool Save() const;
    bool Load();

    // --- 무결성 검사 ---
    bool VerifyFile(const std::wstring& filePath) const;
    std::vector<DiffResult> VerifyAll() const;

    // --- 조회 ---
    bool Contains(const std::wstring& filePath) const;
    const BaselineEntry* Find(const std::wstring& filePath) const;
    size_t Count() const { return m_entries.size(); }
    const std::wstring& GetDbFilePath() const { return m_dbFilePath; }

private:
    static std::wstring GetCurrentTimeString();
    static void CollectFiles(const std::wstring& dirPath,
                             bool recursive,
                             std::vector<std::wstring>& outFiles);

    std::wstring m_dbFilePath;
    std::unordered_map<std::wstring, BaselineEntry> m_entries; // key: 소문자 경로
};
