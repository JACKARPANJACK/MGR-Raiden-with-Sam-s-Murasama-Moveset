@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\round_trip_tests.cpp /FoRelease\round_trip_tests.obj /FeRelease\round_trip_tests.exe
if errorlevel 1 exit /b 1
Release\round_trip_tests.exe
