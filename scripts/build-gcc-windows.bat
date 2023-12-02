:: This script is set to run using the root project directory
:: You might want to change the relative paths if you're running it outside an IDE
:: Also, don't forget to change the paths for the OpenSSL libraries and include files

@echo off
echo Building KxHTTP (Windows)
g++ -s -O2 src\*.cpp -Iinclude -I"C:\Program Files\OpenSSL-Win64\include" -o bin\kxh.exe -L"C:\Program Files\OpenSSL-Win64\lib" -lws2_32 -lssl -lcrypto -lcrypt32
echo Finished Task