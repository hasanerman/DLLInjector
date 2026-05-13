#include "mapping.h"
#include "internal.h"
#include <fstream>

typedef NTSTATUS(NTAPI* pNtCreateSection)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PLARGE_INTEGER, ULONG, ULONG, HANDLE);
typedef NTSTATUS(NTAPI* pNtMapViewOfSection)(HANDLE, HANDLE, PVOID*, ULONG_PTR, SIZE_T, PLARGE_INTEGER, PSIZE_T, UINT, ULONG, ULONG);

namespace Mapping {
    bool InjectNtMap(DWORD processId, const std::wstring& dllPath) {
        HANDLE hFile = CreateFileW(dllPath.c_str(), GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        pNtCreateSection NtCreateSection = (pNtCreateSection)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtCreateSection");
        pNtMapViewOfSection NtMapViewOfSection = (pNtMapViewOfSection)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtMapViewOfSection");

        HANDLE hSection;
        NTSTATUS status = NtCreateSection(&hSection, SECTION_ALL_ACCESS, NULL, NULL, PAGE_EXECUTE_READ, 0x01000000, hFile);
        CloseHandle(hFile);

        if (status != 0) return false;

        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        PVOID pBase = NULL;
        SIZE_T viewSize = 0;
        status = NtMapViewOfSection(hSection, hProcess, &pBase, 0, 0, NULL, &viewSize, 1, 0, PAGE_EXECUTE_READ);

        if (status != 0) {
            CloseHandle(hSection);
            CloseHandle(hProcess);
            return false;
        }

        CloseHandle(hSection);
        CloseHandle(hProcess);
        return true;
    }
}
