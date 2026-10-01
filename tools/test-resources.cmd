@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
if errorlevel 1 exit /b 1
if not exist Release mkdir Release
cl /nologo /EHsc /std:c++20 /W4 tests\sam_archive_lookup_tests.cpp /FoRelease\sam_archive_lookup_tests.obj /FeRelease\sam_archive_lookup_tests.exe
if errorlevel 1 exit /b 1
Release\sam_archive_lookup_tests.exe
