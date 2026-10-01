@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 /Itests\stubs tests\style_switch_tests.cpp /FoRelease\style_switch_tests.obj /FeRelease\style_switch_tests.exe
if errorlevel 1 exit /b 1
Release\style_switch_tests.exe
