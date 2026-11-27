@echo off
setlocal EnableDelayedExpansion
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
if exist "%~dp0..\DualBladeMoveset\pl0010.dat" (
    if not defined BLENDER_EXE if exist "%ProgramFiles%\Blender Foundation\Blender 4.5\blender.exe" set "BLENDER_EXE=%ProgramFiles%\Blender Foundation\Blender 4.5\blender.exe"
    if not defined BLENDER_EXE if exist "E:\MGR-Blender-4.5.11\Blender Foundation\Blender 4.5\blender.exe" set "BLENDER_EXE=E:\MGR-Blender-4.5.11\Blender Foundation\Blender 4.5\blender.exe"
    if not defined BLENDER_EXE (
        echo Blender 4.5+ not found. Set BLENDER_EXE to blender.exe.
        exit /b 1
    )
    if exist "%~dp0..\Release\BlenderRetarget\retarget-report.json" del /q "%~dp0..\Release\BlenderRetarget\retarget-report.json"
    "!BLENDER_EXE!" -b --factory-startup --python-exit-code 1 --python "%~dp0retarget_sundowner_blender.py" -- --repo "%~dp0.." --output "%~dp0..\Release\BlenderRetarget"
    if errorlevel 1 exit /b 1
    if not exist "%~dp0..\Release\BlenderRetarget\retarget-report.json" exit /b 1
    python "%~dp0build_dual_blade_animations.py" --source "%~dp0..\DualBladeMoveset\pl0010.dat" --blade-source "%~dp0..\local_assets\dual-blade\wp0050.dat" --retargeted-motions "%~dp0..\Release\BlenderRetarget" --output "%~dp0..\Release\GameData\wp\wp2050.dat"
    if errorlevel 1 exit /b 1
    python "%~dp0build_gun_animations.py" --base "%~dp0..\local_assets\data000\pl\pl0010.dat" --skeleton "%~dp0..\local_assets\smg\raiden.wmb" --output "%~dp0..\Release\GunMoveset\pl\pl0010.dat"
    if errorlevel 1 exit /b 1
)
if exist "%~dp0..\local_assets\data000\em\em01a0.dat" (
    python "%~dp0build_monsoon_kick.py" --prepare
    if errorlevel 1 exit /b 1
    "!BLENDER_EXE!" -b --factory-startup --python-exit-code 1 --python "%~dp0retarget_sundowner_blender.py" -- --repo "%~dp0.." --output "%~dp0..\Release\MonsoonRetarget" --monsoon-kick
    if errorlevel 1 exit /b 1
    python "%~dp0build_monsoon_kick.py"
    if errorlevel 1 exit /b 1
)
call "%~dp0build-pistols.cmd" assets
if errorlevel 1 exit /b 1
python "%~dp0build_custom_mcd.py"
if errorlevel 1 exit /b 1
python "%~dp0build_custom_uvd.py"
if errorlevel 1 exit /b 1
python "%~dp0pack_animation_banks.py"
if errorlevel 1 exit /b 1
python "%~dp0build_lightning_effects.py"
if errorlevel 1 exit /b 1
msbuild /nologo /m /p:Configuration=Release /p:Platform=Win32 "%~dp0..\Raiden Moveset.vcxproj"
exit /b %errorlevel%
