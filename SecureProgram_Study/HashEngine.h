#pragma once

#include <string>
#include <vector>

// Windows BCrypt API를 사용한 SHA-256 해시 엔진
// bcrypt.lib 링크 필요: #pragma comment(lib, "bcrypt.lib")

class HashEngine
{
public:
    // 파일 경로를 받아 SHA-256 해시를 16진수 문자열로 반환
    // 실패 시 빈 문자열 반환
    static std::wstring ComputeFileHash(const std::wstring& filePath);

    // 메모리 버퍼의 SHA-256 해시를 16진수 문자열로 반환
    static std::wstring ComputeBufferHash(const std::vector<BYTE>& data);

    // 두 해시 문자열이 동일한지 비교
    static bool CompareHash(const std::wstring& hash1, const std::wstring& hash2);

private:
    // 바이트 배열을 16진수 문자열로 변환
    static std::wstring BytesToHexString(const std::vector<BYTE>& bytes);
};
