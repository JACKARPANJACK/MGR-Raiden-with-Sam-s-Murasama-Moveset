@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\pistol_tests.cpp /FoRelease\pistol_tests.obj /FeRelease\pistol_tests.exe
if errorlevel 1 exit /b 1
Release\pistol_tests.exe
if errorlevel 1 exit /b 1
python tools\verify_pistol_native.py
exit /b %errorlevel%
