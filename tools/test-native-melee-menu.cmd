@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\native_melee_menu_tests.cpp /FoRelease\native_melee_menu_tests.obj /FeRelease\native_melee_menu_tests.exe
if errorlevel 1 exit /b 1
Release\native_melee_menu_tests.exe
exit /b %errorlevel%
