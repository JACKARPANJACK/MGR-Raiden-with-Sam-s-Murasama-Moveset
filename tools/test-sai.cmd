@echo off
setlocal
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\sai_moveset_tests.cpp /FoRelease\sai_moveset_tests.obj /FeRelease\sai_moveset_tests.exe
if errorlevel 1 exit /b 1
Release\sai_moveset_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\sai_pull_tests.cpp /FoRelease\sai_pull_tests.obj /FeRelease\sai_pull_tests.exe
if errorlevel 1 exit /b 1
Release\sai_pull_tests.exe
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\monsoon_kick_tests.cpp /FoRelease\monsoon_kick_tests.obj /FeRelease\monsoon_kick_tests.exe
if errorlevel 1 exit /b 1
Release\monsoon_kick_tests.exe
exit /b %errorlevel%
