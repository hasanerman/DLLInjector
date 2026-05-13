#include <windows.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <algorithm>
#include <dwmapi.h>
#include "injector.h"
#include "stealth.h"
#include "mapping.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern "C" HRESULT WINAPI SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);

HWND hProcessList, hDllPath, hMethodCombo, hLogBox, hSearchEdit;
HWND hBypassCheck, hResourceBtn, hHideCheck, hEraseCheck, hHijackCheck;
std::wstring selectedDllPath, selectedResourcePath, currentSearch;

HBRUSH hbrBkg = CreateSolidBrush(RGB(20, 20, 20));
HBRUSH hbrEdit = CreateSolidBrush(RGB(35, 35, 35));
HBRUSH hbrBtn = CreateSolidBrush(RGB(55, 55, 55));
HBRUSH hbrBtnHover = CreateSolidBrush(RGB(75, 75, 75));

LRESULT CALLBACK HeaderSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_PAINT) {
        LRESULT res = DefSubclassProc(hwnd, msg, wParam, lParam);
        HDC hdc = GetDC(hwnd);
        RECT rc; GetClientRect(hwnd, &rc);
        HBRUSH hHeaderBr = CreateSolidBrush(RGB(55, 55, 55));
        FillRect(hdc, &rc, hHeaderBr);
        SetTextColor(hdc, RGB(220, 220, 220));
        SetBkMode(hdc, TRANSPARENT);
        int count = Header_GetItemCount(hwnd);
        for (int i = 0; i < count; i++) {
            RECT itemRc; Header_GetItemRect(hwnd, i, &itemRc);
            wchar_t text[64]; HDITEMW hdi = { HDI_TEXT }; hdi.pszText = text; hdi.cchTextMax = 64;
            SendMessageW(hwnd, HDM_GETITEMW, i, (LPARAM)&hdi);
            DrawTextW(hdc, text, -1, &itemRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        DeleteObject(hHeaderBr);
        ReleaseDC(hwnd, hdc);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void AddToolTip(HWND hP, HWND hC, LPCWSTR t) {
    HWND hT = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hP, NULL, GetModuleHandle(NULL), NULL);
    TOOLINFOW ti = { 0 }; ti.cbSize = sizeof(ti); ti.hwnd = hP; ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS; ti.uId = (UINT_PTR)hC; ti.lpszText = (LPWSTR)t;
    SendMessage(hT, TTM_ADDTOOLW, 0, (LPARAM)&ti);
}

void RefreshProcessList() {
    SendMessage(hProcessList, LVM_DELETEALLITEMS, 0, 0);
    HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (s != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe; pe.dwSize = sizeof(pe);
        if (Process32FirstW(s, &pe)) {
            int i = 0;
            do {
                std::wstring name = pe.szExeFile;
                if (!currentSearch.empty()) {
                    std::wstring ln = name, ls = currentSearch;
                    std::transform(ln.begin(), ln.end(), ln.begin(), ::towlower);
                    std::transform(ls.begin(), ls.end(), ls.begin(), ::towlower);
                    if (ln.find(ls) == std::wstring::npos) continue;
                }
                LVITEMW item = { 0 }; item.mask = LVIF_TEXT; item.iItem = i; item.pszText = pe.szExeFile;
                int idx = SendMessage(hProcessList, LVM_INSERTITEMW, 0, (LPARAM)&item);
                std::wstring pid = std::to_wstring(pe.th32ProcessID);
                LVITEMW sub = { 0 }; sub.mask = LVIF_TEXT; sub.iItem = idx; sub.iSubItem = 1; sub.pszText = (LPWSTR)pid.c_str();
                SendMessage(hProcessList, LVM_SETITEMW, 0, (LPARAM)&sub);
                i++;
            } while (Process32NextW(s, &pe));
        }
        CloseHandle(s);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFont, hFontBold;
    switch (msg) {
    case WM_CREATE: {
        BOOL d = TRUE; DwmSetWindowAttribute(hwnd, 20, &d, sizeof(d));
        hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        hFontBold = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        HWND hT = CreateWindow(L"STATIC", L"DLL INJECTOR", WS_VISIBLE | WS_CHILD | SS_CENTER, 0, 15, 465, 35, hwnd, NULL, NULL, NULL);
        SendMessage(hT, WM_SETFONT, (WPARAM)hFontBold, TRUE);

        hSearchEdit = CreateWindowEx(0, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 20, 60, 410, 30, hwnd, (HMENU)10, NULL, NULL);
        SendMessage(hSearchEdit, EM_SETCUEBANNER, FALSE, (LPARAM)L"Search process name...");

        hProcessList = CreateWindowEx(0, WC_LISTVIEW, L"", WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL, 20, 100, 410, 180, hwnd, (HMENU)1, NULL, NULL);
        ListView_SetExtendedListViewStyle(hProcessList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        ListView_SetBkColor(hProcessList, RGB(30, 30, 30));
        ListView_SetTextBkColor(hProcessList, RGB(30, 30, 30));
        ListView_SetTextColor(hProcessList, RGB(220, 220, 220));
        SetWindowTheme(hProcessList, L"DarkMode_Explorer", NULL);
        SetWindowSubclass(ListView_GetHeader(hProcessList), HeaderSubclass, 0, 0);
        
        LVCOLUMNW col; col.mask = LVCF_TEXT | LVCF_WIDTH;
        col.pszText = (LPWSTR)L"Process"; col.cx = 280; SendMessage(hProcessList, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);
        col.pszText = (LPWSTR)L"PID"; col.cx = 100; SendMessage(hProcessList, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);
        RefreshProcessList();

        hDllPath = CreateWindowEx(0, L"EDIT", L"", WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_READONLY, 20, 295, 330, 30, hwnd, NULL, NULL, NULL);
        CreateWindow(L"BUTTON", L"Browse", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 360, 295, 70, 30, hwnd, (HMENU)2, NULL, NULL);

        hMethodCombo = CreateWindow(WC_COMBOBOX, L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST, 20, 335, 410, 200, hwnd, NULL, NULL, NULL);
        SendMessage(hMethodCombo, CB_ADDSTRING, 0, (LPARAM)L"LoadLibrary");
        SendMessage(hMethodCombo, CB_ADDSTRING, 0, (LPARAM)L"Manual Map");
        SendMessage(hMethodCombo, CB_ADDSTRING, 0, (LPARAM)L"NtMapView");
        SendMessage(hMethodCombo, CB_SETCURSEL, 1, 0);

        hBypassCheck = CreateWindow(L"BUTTON", L"Advanced Bypass", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 375, 140, 25, hwnd, (HMENU)5, NULL, NULL);
        SetWindowTheme(hBypassCheck, L"", L"");
        hResourceBtn = CreateWindow(L"BUTTON", L"Select Resource", WS_CHILD | BS_OWNERDRAW, 170, 375, 260, 25, hwnd, (HMENU)6, NULL, NULL);
        hHideCheck = CreateWindow(L"BUTTON", L"Hide Module", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 410, 110, 25, hwnd, (HMENU)7, NULL, NULL);
        SetWindowTheme(hHideCheck, L"", L"");
        hEraseCheck = CreateWindow(L"BUTTON", L"Erase Headers", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 140, 410, 120, 25, hwnd, (HMENU)8, NULL, NULL);
        SetWindowTheme(hEraseCheck, L"", L"");
        hHijackCheck = CreateWindow(L"BUTTON", L"Thread Hijack", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 270, 410, 110, 25, hwnd, (HMENU)9, NULL, NULL);
        SetWindowTheme(hHijackCheck, L"", L"");

        CreateWindow(L"BUTTON", L"PERFORM INJECTION", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, 20, 450, 410, 45, hwnd, (HMENU)3, NULL, NULL);
        HWND hF = CreateWindow(L"STATIC", L"hasanerman", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_NOTIFY, 0, 505, 465, 20, hwnd, (HMENU)4, NULL, NULL);
        hLogBox = CreateWindowEx(0, L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 20, 530, 410, 60, hwnd, NULL, NULL, NULL);

        EnumChildWindows(hwnd, [](HWND c, LPARAM f) { SendMessage(c, WM_SETFONT, f, TRUE); return TRUE; }, (LPARAM)hFont);
        break;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT di = (LPDRAWITEMSTRUCT)lParam;
        if (di->CtlType == ODT_BUTTON) {
            FillRect(di->hDC, &di->rcItem, (di->itemState & ODS_SELECTED) ? hbrBtnHover : hbrBtn);
            SetTextColor(di->hDC, RGB(240, 240, 240)); SetBkMode(di->hDC, TRANSPARENT);
            wchar_t t[128]; GetWindowTextW(di->hwndItem, t, 128);
            DrawTextW(di->hDC, t, -1, &di->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam; SetTextColor(hdc, RGB(240, 240, 240)); SetBkColor(hdc, RGB(20, 20, 20));
        if (GetDlgCtrlID((HWND)lParam) == 4) SetTextColor(hdc, RGB(0, 160, 255));
        return (LRESULT)hbrBkg;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam; SetTextColor(hdc, RGB(240, 240, 240)); SetBkColor(hdc, RGB(35, 35, 35));
        return (LRESULT)hbrEdit;
    }
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam; SetTextColor(hdc, RGB(240, 240, 240)); SetBkColor(hdc, RGB(35, 35, 35));
        return (LRESULT)hbrEdit;
    }
    case WM_COMMAND: {
        if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == 10) { wchar_t b[256]; GetWindowTextW(hSearchEdit, b, 256); currentSearch = b; RefreshProcessList(); }
        if (LOWORD(wParam) == 2) { OPENFILENAMEW ofn = { 0 }; wchar_t sz[MAX_PATH] = { 0 }; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd; ofn.lpstrFile = sz; ofn.nMaxFile = MAX_PATH; ofn.lpstrFilter = L"DLL Files\0*.dll\0All Files\0*.*\0"; if (GetOpenFileNameW(&ofn)) { selectedDllPath = sz; SetWindowTextW(hDllPath, sz); } }
        if (LOWORD(wParam) == 5) ShowWindow(hResourceBtn, IsDlgButtonChecked(hwnd, 5) ? SW_SHOW : SW_HIDE);
        if (LOWORD(wParam) == 6) { OPENFILENAMEW ofn = { 0 }; wchar_t sz[MAX_PATH] = { 0 }; ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd; ofn.lpstrFile = sz; ofn.nMaxFile = MAX_PATH; if (GetOpenFileNameW(&ofn)) selectedResourcePath = sz; }
        if (LOWORD(wParam) == 3) {
            int sel = SendMessage(hProcessList, LVM_GETNEXTITEM, -1, LVNI_SELECTED); if (sel == -1 || selectedDllPath.empty()) break;
            wchar_t pt[16]; ListView_GetItemText(hProcessList, sel, 1, pt, 16); DWORD pid = std::stoul(pt); int m = SendMessage(hMethodCombo, CB_GETCURSEL, 0, 0);
            if (IsDlgButtonChecked(hwnd, 5) && !selectedResourcePath.empty()) Injector::AdvancedBypass(pid, selectedResourcePath);
            bool s = (m == 0) ? Injector::InjectLoadLibrary(pid, selectedDllPath) : (m == 1 ? Injector::InjectManualMap(pid, selectedDllPath) : Mapping::InjectNtMap(pid, selectedDllPath));
            if (s) { if (IsDlgButtonChecked(hwnd, 7)) Stealth::HideModule(pid, selectedDllPath.substr(selectedDllPath.find_last_of(L"\\/") + 1)); SendMessage(hLogBox, EM_REPLACESEL, 0, (LPARAM)L"[+] Success\r\n"); }
            else SendMessage(hLogBox, EM_REPLACESEL, 0, (LPARAM)L"[!] Failed\r\n");
        }
        if (LOWORD(wParam) == 4) ShellExecuteW(NULL, L"open", L"https://github.com/hasanerman", NULL, NULL, SW_SHOWNORMAL);
        break;
    }
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE h, HINSTANCE hp, PWSTR c, int s) {
    InitCommonControls();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, h, 
                      LoadIcon(h, MAKEINTRESOURCE(101)), 
                      LoadCursor(NULL, IDC_ARROW), hbrBkg, NULL, L"InjectorGUI", 
                      LoadIcon(h, MAKEINTRESOURCE(101)) };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST, L"InjectorGUI", L"DLL Injector", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 465, 650, NULL, NULL, h, NULL);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)wc.hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)wc.hIconSm);
    ShowWindow(hwnd, s); UpdateWindow(hwnd);
    MSG m; while (GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
    return (int)m.wParam;
}
