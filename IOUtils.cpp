#include "windows.h"
#include <string>
#include <chrono>
#include <filesystem>
#include <array>
constexpr size_t ArraySize = 256;

consteval std::array<bool, ArraySize> getEncodeTable()
{
    std::array<bool, ArraySize> result = {};

    // Set characters that don't need URL encoding to true
    for (char c = 'A'; c <= 'Z'; ++c) {
        result[c] = true;
    }

    for (char c = 'a'; c <= 'z'; ++c) {
        result[c] = true;
    }

    for (char c = '0'; c <= '9'; ++c) {
        result[c] = true;
    }

    result['-'] = true;
    result['_'] = true;
    result['.'] = true;
    result['~'] = true;
    return result;
}

constexpr std::array<bool, ArraySize> EncodeTable = getEncodeTable();

std::string urlEncode(const std::string& url)
{
    std::string result;

    constexpr std::string_view hex{ "0123456789ABCDEF" };


    for (auto ch : url)
    {
        unsigned char idx = ch;
        if (EncodeTable[idx])
        {
            result.push_back(ch);
        }
        else
        {
            char high = hex[idx >> 4];
            char low = hex[idx & 0x0F];
            result.push_back('%');
            result.push_back(high);
            result.push_back(low);

        }

    }
    return result;
}





std::string getProcessOutput(const std::string& cmdline)
{
    BOOL ok = TRUE;
    HANDLE hStdInPipeRead = NULL;
    HANDLE hStdInPipeWrite = NULL;
    HANDLE hStdOutPipeRead = NULL;
    HANDLE hStdOutPipeWrite = NULL;

    // Create two pipes.
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    ok = CreatePipe(&hStdInPipeRead, &hStdInPipeWrite, &sa, 0);
    if (ok == FALSE) return std::string();
    ok = CreatePipe(&hStdOutPipeRead, &hStdOutPipeWrite, &sa, 0);
    if (ok == FALSE) return std::string();

    // Create the process.
    STARTUPINFO si = { };
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdError = hStdOutPipeWrite;
    si.hStdOutput = hStdOutPipeWrite;
    si.hStdInput = hStdInPipeRead;
    PROCESS_INFORMATION pi = { };
    LPCSTR lpApplicationName = 0;//appname.c_str();
    LPSTR lpCommandLine = (LPSTR)cmdline.c_str();
    LPSECURITY_ATTRIBUTES lpProcessAttributes = NULL;
    LPSECURITY_ATTRIBUTES lpThreadAttribute = NULL;
    BOOL bInheritHandles = TRUE;
    DWORD dwCreationFlags = 0;
    LPVOID lpEnvironment = NULL;
    LPCSTR lpCurrentDirectory = NULL;
    ok = CreateProcessA(
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttribute,
        bInheritHandles,
        dwCreationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        &si,
        &pi);
    if (ok == FALSE)
    {
        int err = GetLastError();
        return std::string();
    }

    // Close pipes we do not need.
    CloseHandle(hStdOutPipeWrite);
    CloseHandle(hStdInPipeRead);

    // The main loop for reading output from the DIR command.
    char buf[1024 + 1] = { };
    DWORD dwRead = 0;
    DWORD dwAvail = 0;
    std::string result;
//    ok = ReadFile(hStdOutPipeRead, buf, 1024, &dwRead, NULL);
    ok = true;
    do 
    {
        ok = ReadFile(hStdOutPipeRead, buf, 1024, &dwRead, NULL);
        std::string tmp(buf, buf + dwRead);
        result += tmp;
    } while (ok);
        
    // Clean up and exit.
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hStdOutPipeRead);
    CloseHandle(hStdInPipeWrite);
    DWORD dwExitCode = 0;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);
    return result;
}

std::string getUrlContent(const std::string& url)
{
    
    std::string call = "curl -s ";
    call += char(34);
    call += url;
    call += char(34);
    return getProcessOutput(call);
}

std::chrono::minutes getFileAge(const std::string& filename)
{
    if (!std::filesystem::exists(filename))
    {
        return std::chrono::minutes(0);
    }
    auto writetime = std::filesystem::last_write_time(filename);
    auto now = std::chrono::file_clock::now();
    auto dura = std::chrono::duration_cast<std::chrono::minutes>(now - writetime);
    return dura;
}