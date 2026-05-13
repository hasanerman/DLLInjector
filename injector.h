#pragma once
#include <windows.h>
#include <string>
#include <vector>

namespace Injector {
    DWORD GetProcessIdByName(const std::wstring& processName);
    bool InjectLoadLibrary(DWORD processId, const std::wstring& dllPath);
    bool InjectManualMap(DWORD processId, const std::wstring& dllPath);
    bool AdvancedBypass(DWORD processId, const std::wstring& resourcePath);
    std::wstring GetFullPath(const std::wstring& relativePath);
}
