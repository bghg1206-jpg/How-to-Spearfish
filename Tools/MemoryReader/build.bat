@echo off
rem Run from "x64 Native Tools Command Prompt for VS" (MSVC) -> creates memreader.exe
cl /nologo /EHsc /std:c++17 /W4 /utf-8 /D_CRT_SECURE_NO_WARNINGS main.cpp /Fe:memreader.exe
