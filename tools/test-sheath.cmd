@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\sheath_controller_tests.cpp /FoRelease\sheath_controller_tests.obj /FeRelease\sheath_controller_tests.exe
if errorlevel 1 exit /b 1
Release\sheath_controller_tests.exe
