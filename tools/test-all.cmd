@echo off
setlocal
call "%~dp0vc-env.cmd"
if errorlevel 1 exit /b 1

echo Runtime timing and damage repair tests...
cl /nologo /EHsc /std:c++20 /W4 tests\kunai_tests.cpp /FoRelease\kunai_tests.obj /FeRelease\kunai_tests.exe || exit /b 1
Release\kunai_tests.exe || exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\weapon_dlc_tests.cpp /FoRelease\weapon_dlc_tests.obj /FeRelease\weapon_dlc_tests.exe || exit /b 1
Release\weapon_dlc_tests.exe || exit /b 1
cl /nologo /EHsc /std:c++20 /W4 tests\sam_runtime_repair_tests.cpp /FoRelease\sam_runtime_repair_tests.obj /FeRelease\sam_runtime_repair_tests.exe || exit /b 1
Release\sam_runtime_repair_tests.exe || exit /b 1

echo Electric combat policy tests...
cl /nologo /EHsc /std:c++20 /W4 tests\sam_electric_tests.cpp /FoRelease\sam_electric_tests.obj /FeRelease\sam_electric_tests.exe || exit /b 1
Release\sam_electric_tests.exe || exit /b 1

echo Directional add-on and effect policy tests...
cl /nologo /EHsc /std:c++20 /W4 tests\sam_directional_tests.cpp /FoRelease\sam_directional_tests.obj /FeRelease\sam_directional_tests.exe || exit /b 1
Release\sam_directional_tests.exe || exit /b 1

echo Native effect coverage tests...
cl /nologo /EHsc /std:c++20 /W4 tests\sam_effect_coverage_tests.cpp /FoRelease\sam_effect_coverage_tests.obj /FeRelease\sam_effect_coverage_tests.exe || exit /b 1
Release\sam_effect_coverage_tests.exe || exit /b 1
python tools\audit_effects.py --verify || exit /b 1

echo [1/7] Round Trip tests...
cl /nologo /EHsc /std:c++20 /W4 tests\round_trip_tests.cpp /FoRelease\round_trip_tests.obj /FeRelease\round_trip_tests.exe || exit /b 1
Release\round_trip_tests.exe || exit /b 1

echo [2/7] Sheath tests...
cl /nologo /EHsc /std:c++20 /W4 tests\sheath_controller_tests.cpp /FoRelease\sheath_controller_tests.obj /FeRelease\sheath_controller_tests.exe || exit /b 1
Release\sheath_controller_tests.exe || exit /b 1

echo [3/7] Ultimates tests...
cl /nologo /EHsc /std:c++20 /W4 tests\sam_ultimate_tests.cpp /FoRelease\sam_ultimate_tests.obj /FeRelease\sam_ultimate_tests.exe || exit /b 1
Release\sam_ultimate_tests.exe || exit /b 1

echo [4/7] Combat routing tests...
cl /nologo /EHsc /std:c++20 /W4 tests\sam_combat_routing_tests.cpp /FoRelease\sam_combat_routing_tests.obj /FeRelease\sam_combat_routing_tests.exe || exit /b 1
Release\sam_combat_routing_tests.exe || exit /b 1

echo [5/7] Resource lookup tests...
cl /nologo /EHsc /std:c++20 /W4 tests\sam_archive_lookup_tests.cpp /FoRelease\sam_archive_lookup_tests.obj /FeRelease\sam_archive_lookup_tests.exe || exit /b 1
Release\sam_archive_lookup_tests.exe || exit /b 1

echo [6/7] Style switch tests...
cl /nologo /EHsc /std:c++20 /W4 /Itests\stubs tests\style_switch_tests.cpp /FoRelease\style_switch_tests.obj /FeRelease\style_switch_tests.exe || exit /b 1
Release\style_switch_tests.exe || exit /b 1

echo [7/7] Toggle policy tests...
cl /nologo /EHsc /std:c++20 /W4 tests\sam_toggle_policy_tests.cpp /FoRelease\sam_toggle_policy_tests.obj /FeRelease\sam_toggle_policy_tests.exe || exit /b 1
Release\sam_toggle_policy_tests.exe || exit /b 1

echo.
echo ========================================================
echo SUCCESS: ALL 13 TEST SUITES PASSED CLEANLY WITH ZERO FAILS!
echo ========================================================
