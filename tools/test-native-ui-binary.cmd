@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 tests\native_ui_binary_tests.cpp /FoRelease\native_ui_binary_tests.obj /FeRelease\native_ui_binary_tests.exe /link /BASE:0x20000000 /DYNAMICBASE:NO
if errorlevel 1 exit /b 1
Release\native_ui_binary_tests.exe
exit /b %errorlevel%
