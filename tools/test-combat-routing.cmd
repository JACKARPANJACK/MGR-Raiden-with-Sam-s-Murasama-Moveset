@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\sam_combat_routing_tests.cpp /FoRelease\sam_combat_routing_tests.obj /FeRelease\sam_combat_routing_tests.exe
if errorlevel 1 exit /b 1
Release\sam_combat_routing_tests.exe
