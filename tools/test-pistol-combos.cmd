@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\pistol_combo_tests.cpp /FoRelease\pistol_combo_tests.obj /FeRelease\pistol_combo_tests.exe
if errorlevel 1 exit /b 1
Release\pistol_combo_tests.exe
if errorlevel 1 exit /b 1
call "%~dp0test-pistol-controller.cmd"
if errorlevel 1 exit /b 1
python tools\verify_pistol_combos.py
if errorlevel 1 exit /b 1
python tools\verify_pistol_playback_native.py
if errorlevel 1 exit /b 1
call "%~dp0test-pistol-full-body.cmd"
exit /b %errorlevel%
