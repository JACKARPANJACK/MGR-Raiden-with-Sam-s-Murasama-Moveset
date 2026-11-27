@echo off
setlocal
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
if not exist Release mkdir Release
cl /nologo /EHsc /std:c++20 /W4 tests\smg_control_regression_tests.cpp /FoRelease\smg_control_regression_tests.obj /FeRelease\smg_control_regression_tests.exe || exit /b 1
Release\smg_control_regression_tests.exe
exit /b %errorlevel%
