@echo off
where cl >nul 2>&1
if not errorlevel 1 exit /b 0
set "mgr_vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%mgr_vswhere%" exit /b 1
for /f "usebackq delims=" %%i in (`"%mgr_vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "mgr_vsroot=%%i"
if not defined mgr_vsroot exit /b 1
call "%mgr_vsroot%\VC\Auxiliary\Build\vcvars32.bat" >nul
exit /b %errorlevel%
