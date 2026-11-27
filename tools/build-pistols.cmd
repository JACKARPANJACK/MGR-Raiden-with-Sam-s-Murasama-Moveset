@echo off
setlocal
if not defined BLENDER_EXE if exist "%ProgramFiles%\Blender Foundation\Blender 4.5\blender.exe" set "BLENDER_EXE=%ProgramFiles%\Blender Foundation\Blender 4.5\blender.exe"
if not defined BLENDER_EXE if exist "E:\MGR-Blender-4.5.11\Blender Foundation\Blender 4.5\blender.exe" set "BLENDER_EXE=E:\MGR-Blender-4.5.11\Blender Foundation\Blender 4.5\blender.exe"
if not defined BLENDER_EXE if exist "%ProgramFiles(x86)%\Steam\steamapps\common\Blender\blender.exe" set "BLENDER_EXE=%ProgramFiles(x86)%\Steam\steamapps\common\Blender\blender.exe"
if not defined BLENDER_EXE exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0prepare_mgsv_pistols.ps1"
if errorlevel 1 exit /b 1
"%BLENDER_EXE%" -b --factory-startup --python-exit-code 1 --python "%~dp0build_stealth_animations_blender.py" -- --repo "%~dp0.." --pistols
if errorlevel 1 exit /b 1
"%BLENDER_EXE%" -b --factory-startup --python-exit-code 1 --python "%~dp0build_pistol_models_blender.py" -- --repo "%~dp0.."
if errorlevel 1 exit /b 1
if not defined PISTOL_ATTACK_PACKAGE set "PISTOL_ATTACK_PACKAGE=%USERPROFILE%\Desktop\InsanePistolAction AnimSet v1.0\InsanePistolAction AnimSet v1.0.unitypackage"
if exist "%PISTOL_ATTACK_PACKAGE%" (
    "%BLENDER_EXE%" -b --factory-startup --python-exit-code 1 --python "%~dp0build_pistol_attacks_blender.py" -- --repo "%~dp0.." --package "%PISTOL_ATTACK_PACKAGE%"
    if errorlevel 1 exit /b 1
    python "%~dp0tune_pistol_combat.py"
    if errorlevel 1 exit /b 1
    "%BLENDER_EXE%" -b -t 1 --factory-startup --python-exit-code 1 --python "%~dp0verify_pistol_sources_blender.py"
    if errorlevel 1 exit /b 1
    python "%~dp0pistol_attack_catalog.py"
    if errorlevel 1 exit /b 1
)
python "%~dp0verify_pistol_assets.py"
if errorlevel 1 exit /b 1
"%BLENDER_EXE%" -b --factory-startup --python-exit-code 1 --python "%~dp0verify_pistol_models_blender.py" -- --repo "%~dp0.."
if errorlevel 1 exit /b 1
"%BLENDER_EXE%" -b --factory-startup --python-exit-code 1 --python "%~dp0render_pistol_icons_blender.py" -- --repo "%~dp0.."
if errorlevel 1 exit /b 1
if "%~1"=="assets" exit /b 0
python "%~dp0build_custom_mcd.py"
if errorlevel 1 exit /b 1
python "%~dp0build_custom_uvd.py"
if errorlevel 1 exit /b 1
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
msbuild /nologo /m /p:Configuration=Release /p:Platform=Win32 "%~dp0..\Raiden Moveset.vcxproj"
exit /b %errorlevel%
