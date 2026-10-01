@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\sam_toggle_policy_tests.cpp /FoRelease\sam_toggle_policy_tests.obj /FeRelease\sam_toggle_policy_tests.exe
if errorlevel 1 exit /b 1
Release\sam_toggle_policy_tests.exe
