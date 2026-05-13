#include "injector.h"
#include <tlhelp32.h>
#include <iostream>
#include <fstream>

namespace Injector {

    DWORD GetProcessIdByName(const std::wstring& processName) {
        DWORD processId = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        
        if (snapshot == INVALID_HANDLE_VALUE) {
            return 0;
        }

        PROCESSENTRY32W processEntry;
        processEntry.dwSize = sizeof(processEntry);

        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (_wcsicmp(processName.c_str(), processEntry.szExeFile) == 0) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }

        CloseHandle(snapshot);
        return processId;
    }

    bool InjectLoadLibrary(DWORD processId, const std::wstring& dllPath) {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (hProcess == NULL) return false;

        size_t pathSize = (dllPath.length() + 1) * sizeof(wchar_t);
        void* remoteMemory = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        
        if (remoteMemory == NULL) {
            CloseHandle(hProcess);
            return false;
        }

        if (!WriteProcessMemory(hProcess, remoteMemory, dllPath.c_str(), pathSize, NULL)) {
            VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        FARPROC loadLibraryAddr = GetProcAddress(hKernel32, "LoadLibraryW");

        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, remoteMemory, 0, NULL);
        if (hThread == NULL) {
            VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        VirtualFreeEx(hProcess, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return true;
    }

    struct MANUAL_MAPPING_DATA {
        LPVOID pLoadLibraryA;
        LPVOID pGetProcAddress;
        LPVOID pBase;
    };

    void __stdcall Shellcode(MANUAL_MAPPING_DATA* pData) {
        if (!pData) return;

        BYTE* pBase = (BYTE*)pData->pBase;
        auto* pOpt = &((IMAGE_NT_HEADERS*)(pBase + ((IMAGE_DOS_HEADER*)pBase)->e_lfanew))->OptionalHeader;

        auto _LoadLibraryA = (decltype(LoadLibraryA)*)pData->pLoadLibraryA;
        auto _GetProcAddress = (decltype(GetProcAddress)*)pData->pGetProcAddress;
        auto _DllMain = (BOOL(WINAPI*)(HINSTANCE, DWORD, LPVOID))(pBase + pOpt->AddressOfEntryPoint);

        auto* pImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
        while (pImportDescriptor->Name) {
            char* szModule = (char*)(pBase + pImportDescriptor->Name);
            HINSTANCE hDll = _LoadLibraryA(szModule);

            auto* pThunkRef = (IMAGE_THUNK_DATA*)(pBase + pImportDescriptor->OriginalFirstThunk);
            auto* pFuncRef = (IMAGE_THUNK_DATA*)(pBase + pImportDescriptor->FirstThunk);

            if (!pThunkRef) pThunkRef = pFuncRef;

            for (; pThunkRef->u1.AddressOfData; ++pThunkRef, ++pFuncRef) {
                if (IMAGE_SNAP_BY_ORDINAL(pThunkRef->u1.Ordinal)) {
                    pFuncRef->u1.Function = (ULONGLONG)_GetProcAddress(hDll, (char*)(pThunkRef->u1.Ordinal & 0xFFFF));
                } else {
                    auto* pImportByName = (IMAGE_IMPORT_BY_NAME*)(pBase + pThunkRef->u1.AddressOfData);
                    pFuncRef->u1.Function = (ULONGLONG)_GetProcAddress(hDll, pImportByName->Name);
                }
            }
            pImportDescriptor++;
        }

        if (pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
            auto* pBaseReloc = (IMAGE_BASE_RELOCATION*)(pBase + pOpt->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
            ptrdiff_t delta = (ptrdiff_t)(pBase - pOpt->ImageBase);

            while (pBaseReloc->VirtualAddress) {
                UINT count = (pBaseReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                WORD* pRelativeInfo = (WORD*)(pBaseReloc + 1);

                for (UINT i = 0; i < count; ++i) {
                    if ((pRelativeInfo[i] >> 12) == IMAGE_REL_BASED_DIR64) {
                        ULONGLONG* pPatch = (ULONGLONG*)(pBase + pBaseReloc->VirtualAddress + (pRelativeInfo[i] & 0xFFF));
                        *pPatch += delta;
                    }
                }
                pBaseReloc = (IMAGE_BASE_RELOCATION*)((BYTE*)pBaseReloc + pBaseReloc->SizeOfBlock);
            }
        }

        _DllMain((HINSTANCE)pBase, DLL_PROCESS_ATTACH, nullptr);
    }

    bool InjectManualMap(DWORD processId, const std::wstring& dllPath) {
        std::ifstream file(dllPath, std::ios::binary | std::ios::ate);
        if (file.fail()) return false;

        auto fileSize = file.tellg();
        BYTE* pSrcData = new BYTE[(UINT_PTR)fileSize];
        file.seekg(0, std::ios::beg);
        file.read((char*)pSrcData, fileSize);
        file.close();

        auto* pDosHeader = (IMAGE_DOS_HEADER*)pSrcData;
        auto* pOldNtHeader = (IMAGE_NT_HEADERS*)(pSrcData + pDosHeader->e_lfanew);
        auto* pOldOptHeader = &pOldNtHeader->OptionalHeader;
        auto* pOldFileHeader = &pOldNtHeader->FileHeader;

        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        BYTE* pTargetBase = (BYTE*)VirtualAllocEx(hProcess, nullptr, pOldOptHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

        WriteProcessMemory(hProcess, pTargetBase, pSrcData, pOldOptHeader->SizeOfHeaders, nullptr);
        auto* pSectionHeader = IMAGE_FIRST_SECTION(pOldNtHeader);
        for (UINT i = 0; i < pOldFileHeader->NumberOfSections; ++i, ++pSectionHeader) {
            WriteProcessMemory(hProcess, pTargetBase + pSectionHeader->VirtualAddress, pSrcData + pSectionHeader->PointerToRawData, pSectionHeader->SizeOfRawData, nullptr);
        }

        MANUAL_MAPPING_DATA data;
        data.pLoadLibraryA = LoadLibraryA;
        data.pGetProcAddress = GetProcAddress;
        data.pBase = pTargetBase;

        LPVOID pDataAddr = VirtualAllocEx(hProcess, nullptr, sizeof(data), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        WriteProcessMemory(hProcess, pDataAddr, &data, sizeof(data), nullptr);

        LPVOID pShellcodeAddr = VirtualAllocEx(hProcess, nullptr, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        WriteProcessMemory(hProcess, pShellcodeAddr, Shellcode, 1024, nullptr);

        HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)pShellcodeAddr, pDataAddr, 0, nullptr);
        if (!hThread) return false;

        WaitForSingleObject(hThread, INFINITE);
        delete[] pSrcData;
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return true;
    }

    bool AdvancedBypass(DWORD processId, const std::wstring& resourcePath) {
        size_t lastSlash = resourcePath.find_last_of(L"\\/");
        std::wstring fileName = (lastSlash == std::wstring::npos) ? resourcePath : resourcePath.substr(lastSlash + 1);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
        if (hSnapshot == INVALID_HANDLE_VALUE) return false;

        MODULEENTRY32W me;
        me.dwSize = sizeof(me);
        bool found = false;
        if (Module32FirstW(hSnapshot, &me)) {
            do {
                if (_wcsicmp(me.szModule, fileName.c_str()) == 0) {
                    found = true;
                    break;
                }
            } while (Module32NextW(hSnapshot, &me));
        }
        CloseHandle(hSnapshot);
        if (!found) return false;

        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!hProcess) return false;

        BYTE patch[] = { 0xC3 };
        DWORD oldProtect;
        if (VirtualProtectEx(hProcess, me.modBaseAddr, sizeof(patch), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            WriteProcessMemory(hProcess, me.modBaseAddr, patch, sizeof(patch), nullptr);
            VirtualProtectEx(hProcess, me.modBaseAddr, sizeof(patch), oldProtect, &oldProtect);
            CloseHandle(hProcess);
            return true;
        }
        CloseHandle(hProcess);
        return false;
    }

    std::wstring GetFullPath(const std::wstring& relativePath) {
        wchar_t fullPath[MAX_PATH];
        if (GetFullPathNameW(relativePath.c_str(), MAX_PATH, fullPath, NULL)) {
            return std::wstring(fullPath);
        }
        return relativePath;
    }
}
