#include <windows.h>
#include <iostream>
#include <stdio.h>

void MainThread(HMODULE hModule) {
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    std::cout << "========================================" << std::endl;
    std::cout << "      DLL INJECTION SUCCESSFUL          " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Module: TestDLL.dll" << std::endl;
    std::cout << "Status: LOADED" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\n[!] Press 'END' to unload module..." << std::endl;

    while (true) {
        if (GetAsyncKeyState(VK_END) & 0x8000) break;
        Sleep(100);
    }

    if (f) fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, NULL);
        if (hThread) CloseHandle(hThread);
    }
    return TRUE;
}
