@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\weapon_selection_tests.cpp /FoRelease\weapon_selection_tests.obj /FeRelease\weapon_selection_tests.exe
if errorlevel 1 exit /b 1
Release\weapon_selection_tests.exe
exit /b %errorlevel%
