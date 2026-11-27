@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
python tools\prepare_pistol_controller_tests.py
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\pistol_controller_tests.cpp /FoRelease\pistol_controller_tests.obj /FeRelease\pistol_controller_tests.exe
if errorlevel 1 exit /b 1
Release\pistol_controller_tests.exe
exit /b %errorlevel%
