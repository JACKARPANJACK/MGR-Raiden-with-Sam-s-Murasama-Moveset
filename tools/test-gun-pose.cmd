@echo off
setlocal
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
python tests\prepare_gun_pose_tests.py
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 /Itests\gun_pose_stubs /I"%MGR_PLUGIN_SDK%shared\dxsdk" tests\gun_pose_runtime_tests.cpp /FoRelease\gun_pose_runtime_tests.obj /FeRelease\gun_pose_runtime_tests.exe /link /LIBPATH:"%MGR_PLUGIN_SDK%shared\dxsdk" d3dx9.lib
if errorlevel 1 exit /b 1
Release\gun_pose_runtime_tests.exe
if errorlevel 1 exit /b 1
if errorlevel 1 exit /b 1
cl /nologo /EHsc /std:c++20 /W4 /I"%MGR_PLUGIN_SDK%shared\dxsdk" tests\pistol_ik_tests.cpp /FoRelease\pistol_ik_tests.obj /FeRelease\pistol_ik_tests.exe /link /LIBPATH:"%MGR_PLUGIN_SDK%shared\dxsdk" d3dx9.lib
if errorlevel 1 exit /b 1
Release\pistol_ik_tests.exe
exit /b %errorlevel%
