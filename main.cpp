#include <iostream>
#include <string>
#include "injector.h"

void PrintBanner() {
    std::wcout << L"========================================" << std::endl;
    std::wcout << L"      UNIVERSAL DLL INJECTOR (CLI)      " << std::endl;
    std::wcout << L"========================================" << std::endl;
}

int main() {
    PrintBanner();

    std::wstring processName;
    std::wstring dllName;

    std::wcout << L"[?] Target Process Name (e.g., notepad.exe): ";
    std::getline(std::wcin, processName);

    std::wcout << L"[?] DLL File Path: ";
    std::getline(std::wcin, dllName);

    std::wcout << L"\n[*] Searching for " << processName << L"..." << std::endl;
    DWORD pid = Injector::GetProcessIdByName(processName);

    if (pid == 0) {
        std::wcout << L"[!] Process not found! Make sure it's running." << std::endl;
        system("pause");
        return 1;
    }

    std::wcout << L"[+] Process found! PID: " << pid << std::endl;
    
    std::wcout << L"\n[1] LoadLibrary (Basic)" << std::endl;
    std::wcout << L"[2] Manual Mapping (Advanced/Stealth)" << std::endl;
    std::wcout << L"[?] Choose Method: ";
    
    int choice;
    std::cin >> choice;

    std::wstring fullDllPath = Injector::GetFullPath(dllName);
    std::wcout << L"[*] Attempting to inject: " << fullDllPath << std::endl;

    bool success = false;
    if (choice == 1) {
        success = Injector::InjectLoadLibrary(pid, fullDllPath);
    } else {
        success = Injector::InjectManualMap(pid, fullDllPath);
    }

    if (success) {
        std::wcout << L"\n[SUCCESS] DLL successfully injected!" << std::endl;
    } else {
        std::wcout << L"\n[FAILED] Injection failed." << std::endl;
    }

    std::wcout << L"\n========================================" << std::endl;
    system("pause");
    return 0;
}
