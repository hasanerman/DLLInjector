#include "stealth.h"
#include "internal.h"
#include <tlhelp32.h>

typedef NTSTATUS(NTAPI* pNtQueryInformationProcess)(HANDLE, UINT, PVOID, ULONG, PULONG);

namespace Stealth {
    bool HideModule(DWORD processId, const std::wstring& moduleName) {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!hProcess) return false;

        pNtQueryInformationProcess NtQueryInfo = (pNtQueryInformationProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");
        PROCESS_BASIC_INFORMATION pbi;
        ULONG returnLength;
        if (NtQueryInfo(hProcess, 0, &pbi, sizeof(pbi), &returnLength) != 0) {
            CloseHandle(hProcess);
            return false;
        }

        PEB peb;
        SIZE_T bytesRead;
        if (!ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), &bytesRead)) {
            CloseHandle(hProcess);
            return false;
        }

        PEB_LDR_DATA ldr;
        if (!ReadProcessMemory(hProcess, peb.Ldr, &ldr, sizeof(ldr), &bytesRead)) {
            CloseHandle(hProcess);
            return false;
        }

        LIST_ENTRY* pListHead = (LIST_ENTRY*)((BYTE*)peb.Ldr + FIELD_OFFSET(PEB_LDR_DATA, InLoadOrderModuleList));
        LIST_ENTRY* pCurrent = ldr.InLoadOrderModuleList.Flink;

        while (pCurrent != pListHead) {
            LDR_DATA_TABLE_ENTRY entry;
            if (!ReadProcessMemory(hProcess, CONTAINING_RECORD(pCurrent, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks), &entry, sizeof(entry), &bytesRead)) break;

            wchar_t name[MAX_PATH];
            if (ReadProcessMemory(hProcess, entry.BaseDllName.Buffer, name, entry.BaseDllName.Length, &bytesRead)) {
                name[entry.BaseDllName.Length / 2] = L'\0';
                if (_wcsicmp(name, moduleName.c_str()) == 0) {
                    LIST_ENTRY prev, next;
                    ReadProcessMemory(hProcess, pCurrent->Blink, &prev, sizeof(prev), &bytesRead);
                    ReadProcessMemory(hProcess, pCurrent->Flink, &next, sizeof(next), &bytesRead);

                    next.Blink = pCurrent->Blink;
                    prev.Flink = pCurrent->Flink;

                    WriteProcessMemory(hProcess, pCurrent->Blink, &prev, sizeof(prev), &bytesRead);
                    WriteProcessMemory(hProcess, pCurrent->Flink, &next, sizeof(next), &bytesRead);
                    break;
                }
            }
            pCurrent = entry.InLoadOrderLinks.Flink;
        }

        CloseHandle(hProcess);
        return true;
    }

    bool EraseHeaders(HANDLE hProcess, LPVOID pBase) {
        DWORD old;
        if (VirtualProtectEx(hProcess, pBase, 4096, PAGE_READWRITE, &old)) {
            BYTE zero[4096] = { 0 };
            SIZE_T bytesWritten;
            WriteProcessMemory(hProcess, pBase, zero, 4096, &bytesWritten);
            VirtualProtectEx(hProcess, pBase, 4096, old, &old);
            return true;
        }
        return false;
    }

    bool HijackThread(HANDLE hProcess, LPVOID pEntryPoint, LPVOID pData) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return false;

        THREADENTRY32 te;
        te.dwSize = sizeof(te);
        DWORD targetPid = GetProcessId(hProcess);
        DWORD threadId = 0;

        if (Thread32First(hSnapshot, &te)) {
            do {
                if (te.th32OwnerProcessID == targetPid) {
                    threadId = te.th32ThreadID;
                    break;
                }
            } while (Thread32Next(hSnapshot, &te));
        }
        CloseHandle(hSnapshot);

        if (threadId == 0) return false;

        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
        if (!hThread) return false;

        SuspendThread(hThread);

        CONTEXT ctx;
        ctx.ContextFlags = CONTEXT_FULL;
        GetThreadContext(hThread, &ctx);

#ifdef _WIN64
        BYTE shellcode[] = {
            0x48, 0x83, 0xEC, 0x28,                 
            0x48, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0x48, 0xBA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0xFF, 0xD2,                             
            0x48, 0x83, 0xC4, 0x28,                 
            0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
            0xFF, 0xE0                              
        };

        *(LPVOID*)&shellcode[6] = pData;
        *(LPVOID*)&shellcode[16] = pEntryPoint;
        *(ULONGLONG*)&shellcode[34] = ctx.Rip;

        LPVOID pShellcode = VirtualAllocEx(hProcess, NULL, sizeof(shellcode), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        SIZE_T bytesWritten;
        WriteProcessMemory(hProcess, pShellcode, shellcode, sizeof(shellcode), &bytesWritten);

        ctx.Rip = (DWORD64)pShellcode;
#endif

        SetThreadContext(hThread, &ctx);
        ResumeThread(hThread);
        CloseHandle(hThread);
        return true;
    }
}
