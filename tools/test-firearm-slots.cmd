@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\firearm_slots_tests.cpp /FoRelease\firearm_slots_tests.obj /FeRelease\firearm_slots_tests.exe
if errorlevel 1 exit /b 1
Release\firearm_slots_tests.exe
exit /b %errorlevel%
