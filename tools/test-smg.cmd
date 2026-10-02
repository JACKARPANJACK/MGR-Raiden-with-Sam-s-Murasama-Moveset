@echo off
setlocal
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\smg_tests.cpp /FoRelease\smg_tests.obj /FeRelease\smg_tests.exe || exit /b 1
Release\smg_tests.exe
exit /b %errorlevel%
