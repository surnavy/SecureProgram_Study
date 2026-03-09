#include "pch.h"
#include "BaselineDB.h"
#include "HashEngine.h"

#include <fstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

static const std::wstring DB_HEADER = L"#FIM_BASELINE_V1";
static const wchar_t DELIMITER = L'\t';

BaselineDB::BaselineDB(const std::wstring& dbFilePath)
    : m_dbFilePath(dbFilePath)
{
}

// ---- 기준선 구축 ----------------------------------------------------------

void BaselineDB::AddOrUpdate(const std::wstring& filePath, const std::wstring& hash)
{
    // 경로를 소문자로 정규화 (대소문자 구분 없이 비교)
    std::wstring key = filePath;
    std::transform(key.begin(), key.end(), key.begin(), ::towlower);

    BaselineEntry entry;
    entry.filePath     = filePath;
    entry.hash         = hash;
    entry.lastModified = GetCurrentTimeString();
    m_entries[key]     = std::move(entry);
}

bool BaselineDB::BuildFromDirectory(const std::wstring& dirPath, bool watchSubtree)
{
    std::vector<std::wstring> files;
    CollectFiles(dirPath, watchSubtree, files);
    if (files.empty()) return false;

    for (const auto& path : files)
    {
        std::wstring hash = HashEngine::ComputeFileHash(path);
        if (!hash.empty())
            AddOrUpdate(path, hash);
    }
    return true;
}

void BaselineDB::Remove(const std::wstring& filePath)
{
    std::wstring key = filePath;
    std::transform(key.begin(), key.end(), key.begin(), ::towlower);
    m_entries.erase(key);
}

void BaselineDB::Clear() { m_entries.clear(); }

// ---- 영속성 ---------------------------------------------------------------

bool BaselineDB::Save() const
{
    std::wofstream ofs(m_dbFilePath, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) return false;

    ofs << DB_HEADER << L"\n";
    for (const auto& [key, entry] : m_entries)
        ofs << entry.hash << DELIMITER << entry.lastModified << DELIMITER << entry.filePath << L"\n";

    return ofs.good();
}

bool BaselineDB::Load()
{
    std::wifstream ifs(m_dbFilePath);
    if (!ifs.is_open()) return false;

    m_entries.clear();
    std::wstring line;

    // 헤더 확인
    if (!std::getline(ifs, line) || line != DB_HEADER)
        return false;

    while (std::getline(ifs, line))
    {
        if (line.empty() || line[0] == L'#') continue;

        size_t pos1 = line.find(DELIMITER);
        if (pos1 == std::wstring::npos) continue;
        size_t pos2 = line.find(DELIMITER, pos1 + 1);
        if (pos2 == std::wstring::npos) continue;

        BaselineEntry entry;
        entry.hash         = line.substr(0, pos1);
        entry.lastModified = line.substr(pos1 + 1, pos2 - pos1 - 1);
        entry.filePath     = line.substr(pos2 + 1);

        std::wstring key = entry.filePath;
        std::transform(key.begin(), key.end(), key.begin(), ::towlower);
        m_entries[key] = std::move(entry);
    }
    return true;
}

// ---- 무결성 검사 ----------------------------------------------------------

bool BaselineDB::VerifyFile(const std::wstring& filePath) const
{
    std::wstring key = filePath;
    std::transform(key.begin(), key.end(), key.begin(), ::towlower);

    auto it = m_entries.find(key);
    if (it == m_entries.end()) return false;

    std::wstring currentHash = HashEngine::ComputeFileHash(filePath);
    return HashEngine::CompareHash(it->second.hash, currentHash);
}

std::vector<DiffResult> BaselineDB::VerifyAll() const
{
    std::vector<DiffResult> diffs;

    // 기준선 파일 검사: Modified / Removed 탐지
    for (const auto& [key, entry] : m_entries)
    {
        if (!fs::exists(entry.filePath))
        {
            diffs.push_back({ DiffType::Removed, entry.filePath, entry.hash, L"" });
        }
        else
        {
            std::wstring cur = HashEngine::ComputeFileHash(entry.filePath);
            if (!HashEngine::CompareHash(entry.hash, cur))
                diffs.push_back({ DiffType::Modified, entry.filePath, entry.hash, cur });
        }
    }

    // 현재 파일 중 기준선에 없는 파일: Added 탐지
    if (!m_entries.empty())
    {
        std::wstring rootDir = fs::path(m_entries.begin()->second.filePath).parent_path().wstring();
        for (const auto& [k, e] : m_entries)
        {
            while (!rootDir.empty() && e.filePath.substr(0, rootDir.size()) != rootDir)
                rootDir = fs::path(rootDir).parent_path().wstring();
        }

        if (!rootDir.empty() && fs::exists(rootDir))
        {
            std::vector<std::wstring> currentFiles;
            CollectFiles(rootDir, true, currentFiles);
            for (const auto& path : currentFiles)
            {
                std::wstring key = path;
                std::transform(key.begin(), key.end(), key.begin(), ::towlower);
                if (m_entries.find(key) == m_entries.end())
                    diffs.push_back({ DiffType::Added, path, L"", HashEngine::ComputeFileHash(path) });
            }
        }
    }
    return diffs;
}

// ---- 조회 ----------------------------------------------------------------

bool BaselineDB::Contains(const std::wstring& filePath) const
{
    std::wstring key = filePath;
    std::transform(key.begin(), key.end(), key.begin(), ::towlower);
    return m_entries.count(key) > 0;
}

const BaselineEntry* BaselineDB::Find(const std::wstring& filePath) const
{
    std::wstring key = filePath;
    std::transform(key.begin(), key.end(), key.begin(), ::towlower);
    auto it = m_entries.find(key);
    return (it != m_entries.end()) ? &it->second : nullptr;
}

// ---- 유틸리티 ------------------------------------------------------------

std::wstring BaselineDB::GetCurrentTimeString()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t buf[32];
    swprintf_s(buf, L"%04d-%02d-%02d %02d:%02d:%02d",
               st.wYear, st.wMonth, st.wDay,
               st.wHour, st.wMinute, st.wSecond);
    return buf;
}

void BaselineDB::CollectFiles(const std::wstring& dirPath,
                              bool recursive,
                              std::vector<std::wstring>& outFiles)
{
    std::error_code ec;
    if (recursive)
    {
        for (const auto& e : fs::recursive_directory_iterator(dirPath, ec))
            if (!ec && e.is_regular_file(ec)) outFiles.push_back(e.path().wstring());
    }
    else
    {
        for (const auto& e : fs::directory_iterator(dirPath, ec))
            if (!ec && e.is_regular_file(ec)) outFiles.push_back(e.path().wstring());
    }
}
