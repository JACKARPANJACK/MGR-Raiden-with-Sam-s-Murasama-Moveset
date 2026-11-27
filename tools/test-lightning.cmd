@echo off
setlocal
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1
python "%~dp0build_lightning_effects.py"
if errorlevel 1 exit /b 1
python "%~dp0..\tests\lightning_effect_assets_tests.py"
if errorlevel 1 exit /b 1
for %%t in (lightning_special sam_config sam_electric) do (
    cl /nologo /EHsc /std:c++20 /W4 tests\%%t_tests.cpp /FoRelease\%%t_tests.obj /FeRelease\%%t_tests.exe
    if errorlevel 1 exit /b 1
    Release\%%t_tests.exe
    if errorlevel 1 exit /b 1
)
