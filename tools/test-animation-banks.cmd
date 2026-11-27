@echo off
setlocal
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
python "%~dp0pack_animation_banks.py"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\animation_archive_tests.cpp /FoRelease\animation_archive_tests.obj /FeRelease\animation_archive_tests.exe
if errorlevel 1 exit /b 1
Release\animation_archive_tests.exe
exit /b %errorlevel%
