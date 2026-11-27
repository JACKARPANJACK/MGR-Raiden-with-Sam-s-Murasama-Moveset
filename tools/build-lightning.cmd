@echo off
setlocal
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
python "%~dp0build_lightning_effects.py"
if errorlevel 1 exit /b 1
msbuild /nologo /m /p:Configuration=Release /p:Platform=Win32 "%~dp0..\Raiden Moveset.vcxproj"
exit /b %errorlevel%
