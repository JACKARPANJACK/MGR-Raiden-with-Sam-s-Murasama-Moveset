@echo off
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
python tools\prepare_pistol_full_body_tests.py
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 /I"%MGR_PLUGIN_SDK%shared\dxsdk" tests\pistol_full_body_tests.cpp /FoRelease\pistol_full_body_tests.obj /FeRelease\pistol_full_body_tests.exe /link /LIBPATH:"%MGR_PLUGIN_SDK%shared\dxsdk" d3dx9.lib
if errorlevel 1 exit /b 1
Release\pistol_full_body_tests.exe
exit /b %errorlevel%
