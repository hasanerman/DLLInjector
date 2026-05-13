#pragma once
#include <windows.h>
#include <string>

namespace Mapping {
    bool InjectNtMap(DWORD processId, const std::wstring& dllPath);
}
