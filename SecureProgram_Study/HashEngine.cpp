#include "pch.h"
#include "HashEngine.h"

#include <windows.h>
#include <bcrypt.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "bcrypt.lib")

// 파일 읽기 버퍼 크기: 4MB
static constexpr DWORD READ_BUFFER_SIZE = 4 * 1024 * 1024;

std::wstring HashEngine::ComputeFileHash(const std::wstring& filePath)
{
    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return L"";

    // SHA-256 알고리즘 프로바이더 열기
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);

    if (!BCRYPT_SUCCESS(status))
    {
        CloseHandle(hFile);
        return L"";
    }

    DWORD cbHashObject = 0, cbData = 0;
    BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&cbHashObject), sizeof(DWORD), &cbData, 0);

    std::vector<BYTE> hashObject(cbHashObject);

    BCRYPT_HASH_HANDLE hHash = nullptr;
    status = BCryptCreateHash(hAlg, &hHash,
        hashObject.data(), cbHashObject, nullptr, 0, 0);

    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        CloseHandle(hFile);
        return L"";
    }

    // 파일을 청크 단위로 읽어 해시 업데이트
    std::vector<BYTE> buffer(READ_BUFFER_SIZE);
    DWORD bytesRead = 0;
    bool success = true;

    while (true)
    {
        if (!ReadFile(hFile, buffer.data(), READ_BUFFER_SIZE, &bytesRead, nullptr))
        {
            success = false;
            break;
        }
        if (bytesRead == 0) break;

        status = BCryptHashData(hHash, buffer.data(), bytesRead, 0);
        if (!BCRYPT_SUCCESS(status)) { success = false; break; }
    }

    std::wstring result;
    if (success)
    {
        DWORD cbHash = 0;
        BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(&cbHash), sizeof(DWORD), &cbData, 0);

        std::vector<BYTE> hashBytes(cbHash);
        status = BCryptFinishHash(hHash, hashBytes.data(), cbHash, 0);
        if (BCRYPT_SUCCESS(status))
            result = BytesToHexString(hashBytes);
    }

    // 리소스 정리
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    CloseHandle(hFile);
    return result;
}

std::wstring HashEngine::ComputeBufferHash(const std::vector<BYTE>& data)
{
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(
        &hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) return L"";

    DWORD cbHashObject = 0, cbData = 0;
    BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&cbHashObject), sizeof(DWORD), &cbData, 0);

    std::vector<BYTE> hashObject(cbHashObject);
    BCRYPT_HASH_HANDLE hHash = nullptr;
    BCryptCreateHash(hAlg, &hHash, hashObject.data(), cbHashObject, nullptr, 0, 0);

    std::wstring result;
    status = BCryptHashData(hHash,
        const_cast<PUCHAR>(data.data()), static_cast<ULONG>(data.size()), 0);

    if (BCRYPT_SUCCESS(status))
    {
        DWORD cbHash = 0;
        BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(&cbHash), sizeof(DWORD), &cbData, 0);

        std::vector<BYTE> hashBytes(cbHash);
        if (BCRYPT_SUCCESS(BCryptFinishHash(hHash, hashBytes.data(), cbHash, 0)))
            result = BytesToHexString(hashBytes);
    }

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return result;
}

bool HashEngine::CompareHash(const std::wstring& hash1, const std::wstring& hash2)
{
    if (hash1.empty() || hash2.empty()) return false;
    return hash1 == hash2;
}

std::wstring HashEngine::BytesToHexString(const std::vector<BYTE>& bytes)
{
    std::wostringstream oss;
    oss << std::hex << std::setfill(L'0');
    for (BYTE b : bytes)
        oss << std::setw(2) << static_cast<unsigned int>(b);
    return oss.str();
}
