@echo off
echo Building KxHTTP (Windows)
echo.
g++ -s -O2 src\*.cpp -Iinclude -o bin\kxh.exe -L"C:\Program Files\OpenSSL-Win64\lib" -lws2_32 -lssl -lcrypto
echo.
echo Finished Task