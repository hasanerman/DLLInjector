@echo off
echo [*] Compiling Test DLL...
cl.exe /LD test_dll.cpp /Fe:test.dll /link /INCREMENTAL:NO

echo.
echo [*] Compiling Injector...
cl.exe main.cpp injector.cpp /Fe:injector.exe /link /INCREMENTAL:NO

echo.
echo [+] Done! Run injector.exe to start.
pause
