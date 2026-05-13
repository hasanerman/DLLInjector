#pragma once
#include <windows.h>
#include <string>

namespace Stealth {
    bool HideModule(DWORD processId, const std::wstring& moduleName);
    bool EraseHeaders(HANDLE hProcess, LPVOID pBase);
    bool HijackThread(HANDLE hProcess, LPVOID pEntryPoint, LPVOID pData);
}
