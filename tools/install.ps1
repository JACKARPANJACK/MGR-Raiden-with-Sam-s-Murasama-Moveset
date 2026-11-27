param([Parameter(Mandatory=$true)][string]$GameDirectory)
$ErrorActionPreference = 'Stop'
$installRoot = Split-Path -Parent $PSScriptRoot
$installGame = (Resolve-Path -LiteralPath $GameDirectory).Path
if (!(Test-Path -LiteralPath (Join-Path $installGame 'METAL GEAR RISING REVENGEANCE.exe'))) { throw 'Game executable not found in the supplied directory.' }
if (Get-Process -Name '*REVENGEANCE*' -ErrorAction SilentlyContinue) { throw 'Close the game before installing the plugin.' }
$installTarget = Join-Path $installGame 'scripts'
# Preflight every runtime dependency before replacing installed files. The
# plugin opens these bundles directly; Mod Loader cannot supply missing files.
$installPlan = @()
function Add-InstallFile([string]$Source, [string]$Relative) {
    if (!(Test-Path -LiteralPath $Source -PathType Leaf)) { throw "Required runtime file missing: $Source" }
    $script:installPlan += [pscustomobject]@{Source=$Source; Relative=$Relative}
}
foreach ($installName in @('Raiden Moveset.asi','Raiden Moveset.pdb')) {
    Add-InstallFile (Join-Path $installRoot "Release/$installName") "scripts/$installName"
}
$installBinary = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes((Join-Path $installRoot 'Release/Raiden Moveset.asi')))
foreach ($installBank in @('pl1c00','pl1c10','pl1c20','pl1c30')) {
    if (!$installBinary.Contains($installBank)) { throw 'Rebuild the ASI with packed animation support before installing.' }
}
# Packing validates required members and byte-for-byte DAT round trips.
& python (Join-Path $PSScriptRoot 'build_lightning_effects.py') --game $installGame
if ($LASTEXITCODE -ne 0) { throw 'Custom lightning effect build failed.' }
& python (Join-Path $PSScriptRoot 'pack_animation_banks.py')
if ($LASTEXITCODE -ne 0) { throw 'Animation archive packing failed.' }
foreach ($installBank in @('pl1c00','pl1c10','pl1c20','pl1c30')) {
    if (!(Test-Path -LiteralPath (Join-Path $installRoot "Release/GameData/pl/$installBank.dat"))) {
        throw "Required animation archive missing: $installBank"
    }
}
# Preserve existing user settings, including installs that placed the INI in
# scripts even though SamConfig reads it alongside the game executable.
if (!(Test-Path -LiteralPath (Join-Path $installGame 'SamMoveset.ini'))) {
    $installConfig = Join-Path $installTarget 'SamMoveset.ini'
    if (!(Test-Path -LiteralPath $installConfig)) { $installConfig = Join-Path $installRoot 'SamMoveset.ini' }
    Add-InstallFile $installConfig 'SamMoveset.ini'
}
$installMod = Join-Path $installGame 'mods/RaidenMoveset'
foreach ($installBase in @('pl/pl0010.dat','pl/pl1400.dat')) {
    if (!(Test-Path -LiteralPath (Join-Path $installMod $installBase))) { throw "Base Mod Loader resource missing: $installBase" }
}
$installStaging = Join-Path $installRoot 'Release/GameData'
foreach ($installResource in Get-ChildItem -LiteralPath $installStaging -Recurse -File | Where-Object { $_.Extension -in '.dat','.dtt' }) {
    $installRelative = $installResource.FullName.Substring($installStaging.Length + 1)
    Add-InstallFile $installResource.FullName "mods/RaidenMoveset/$installRelative"
}
foreach ($installLoader in @('MGRModLoader.asi','MGRModLoaderSettings.ini','mods/profiles.ini')) {
    if (!(Test-Path -LiteralPath (Join-Path $installGame $installLoader))) { throw "Mod Loader installation missing: $installLoader" }
}
$installBundles = @('StealthAnims','SamTemplateAnims','BaseMoveset')
foreach ($installBundle in $installBundles) {
    $linkPath = Join-Path $installTarget $installBundle
    $bundlePath = Join-Path $installMod $installBundle
    if (Test-Path -LiteralPath $linkPath) {
        $linkItem = Get-Item -LiteralPath $linkPath -Force
        if (($linkItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -and
            ($linkItem.LinkType -ne 'Junction' -or [IO.Path]::GetFullPath([string]$linkItem.Target) -ne [IO.Path]::GetFullPath($bundlePath))) {
            throw "Unexpected existing animation link: $linkPath"
        }
    }
}
$installBackup = Join-Path $installRoot ('backups/installed-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $installTarget,$installBackup -Force | Out-Null
function Save-InstalledFile([string]$Path, [string]$Relative) {
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { return }
    $savedPath = Join-Path $installBackup $Relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $savedPath) -Force | Out-Null
    Copy-Item -LiteralPath $Path -Destination $savedPath
    if ((Get-FileHash -LiteralPath $Path).Hash -ne (Get-FileHash -LiteralPath $savedPath).Hash) { throw "Backup verification failed: $Relative" }
}
function Disable-DuplicateMovesetPlugin([string]$GamePath, [string]$BackupPath) {
    # ASI loaders discover both the game root and Scripts. Two copies install
    # competing action hooks, so an unloaded controller cancels the other's move.
    $canonicalPlugin = Join-Path $GamePath 'scripts/Raiden Moveset.asi'
    if (!(Test-Path -LiteralPath $canonicalPlugin -PathType Leaf)) { throw 'Canonical Scripts plugin must be installed before disabling a duplicate.' }
    $duplicatePlugin = Join-Path $GamePath 'Raiden Moveset.asi'
    if (!(Test-Path -LiteralPath $duplicatePlugin -PathType Leaf)) { return }
    $duplicateHash = (Get-FileHash -LiteralPath $duplicatePlugin).Hash
    $savedPlugin = Join-Path $BackupPath 'duplicate-root/Raiden Moveset.asi'
    New-Item -ItemType Directory -Path (Split-Path -Parent $savedPlugin) -Force | Out-Null
    Copy-Item -LiteralPath $duplicatePlugin -Destination $savedPlugin
    if ((Get-FileHash -LiteralPath $savedPlugin).Hash -ne $duplicateHash) { throw 'Duplicate plugin backup verification failed.' }
    $disabledName = 'Raiden Moveset.asi.disabled-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff')
    Rename-Item -LiteralPath $duplicatePlugin -NewName $disabledName
    if ((Get-FileHash -LiteralPath (Join-Path $GamePath $disabledName)).Hash -ne $duplicateHash) { throw 'Disabled plugin hash mismatch.' }
    Write-Output 'Disabled and backed up duplicate root plugin; the current Scripts copy is the only active copy.'
}
$installManifest = @()
foreach ($installFile in $installPlan) {
    $installDestination = Join-Path $installGame $installFile.Relative
    Save-InstalledFile $installDestination $installFile.Relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $installDestination) -Force | Out-Null
    Copy-Item -LiteralPath $installFile.Source -Destination $installDestination -Force
    $installHash = (Get-FileHash -LiteralPath $installFile.Source).Hash
    if ($installHash -ne (Get-FileHash -LiteralPath $installDestination).Hash) { throw "Installed file verification failed: $($installFile.Relative)" }
    $installManifest += [pscustomobject]@{Path=$installFile.Relative; SHA256=$installHash}
}
Disable-DuplicateMovesetPlugin $installGame $installBackup
# Retire the old lightning core overrides only when they match our manifest.
$lightningManifestPath = Join-Path $installRoot 'Release/lightning-effects-manifest.json'
if (Test-Path -LiteralPath $lightningManifestPath) {
    $lightningOwnership = Get-Content -LiteralPath $lightningManifestPath -Raw | ConvertFrom-Json
    foreach ($lightningOldRelative in @('core/coreeff.dat','core/coreeff.dtt')) {
        $lightningOldPath = Join-Path $installMod $lightningOldRelative
        $lightningOwnedHash = $lightningOwnership.retired_core.$lightningOldRelative
        if ($lightningOwnedHash -and (Test-Path -LiteralPath $lightningOldPath -PathType Leaf) -and
            (Get-FileHash -LiteralPath $lightningOldPath).Hash -eq $lightningOwnedHash) {
            Save-InstalledFile $lightningOldPath "mods/RaidenMoveset/$lightningOldRelative"
            Remove-Item -LiteralPath $lightningOldPath
            Write-Output "Retired owned core override: $lightningOldRelative"
        }
    }
}
# Retire the former descriptive archives after installing the native-ID banks.
foreach ($installOldBank in @('pl/stealth_anims.dat','pl/sam_template_anims.dat','pl/base_moveset_anims.dat','wp/pistol_anims.dat')) {
    $installOldPath = [IO.Path]::GetFullPath((Join-Path $installMod $installOldBank))
    if (!$installOldPath.StartsWith($installMod + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected archive migration path.' }
    if (!(Test-Path -LiteralPath $installOldPath -PathType Leaf)) { continue }
    $installOldHash = (Get-FileHash -LiteralPath $installOldPath).Hash
    $installRetired = Join-Path $installBackup "retired-archives/$installOldBank"
    New-Item -ItemType Directory -Path (Split-Path -Parent $installRetired) -Force | Out-Null
    Move-Item -LiteralPath $installOldPath -Destination $installRetired
    if ((Get-FileHash -LiteralPath $installRetired).Hash -ne $installOldHash) { throw 'Retired archive backup verification failed.' }
}
# Retire old directory links and loose bundles only after the new archives
# and updated ASI have been installed and hash verified.
foreach ($installBundle in $installBundles) {
    $linkPath = Join-Path $installTarget $installBundle
    if (Test-Path -LiteralPath $linkPath) {
        $linkItem = Get-Item -LiteralPath $linkPath -Force
        if ($linkItem.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            # Nonrecursive deletion removes the junction itself, never its target.
            [IO.Directory]::Delete($linkPath)
        } else {
            $resolvedLink = (Resolve-Path -LiteralPath $linkPath).Path
            if (!$resolvedLink.StartsWith($installGame + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
                throw 'Animation directory is outside the game directory.'
            }
            $bundleBackup = Join-Path $installBackup "previous-script-bundles/$installBundle"
            New-Item -ItemType Directory -Path (Split-Path -Parent $bundleBackup) -Force | Out-Null
            Move-Item -LiteralPath $resolvedLink -Destination $bundleBackup
        }
    }
    $loosePath = Join-Path $installMod $installBundle
    if (Test-Path -LiteralPath $loosePath) {
        $looseItem = Get-Item -LiteralPath $loosePath -Force
        $resolvedLoose = (Resolve-Path -LiteralPath $loosePath).Path
        if (($looseItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -or
            !$resolvedLoose.StartsWith($installMod + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
            throw 'Unexpected loose animation path; cleanup stopped.'
        }
        $bundleBackup = Join-Path $installBackup "previous-mod-bundles/$installBundle"
        New-Item -ItemType Directory -Path (Split-Path -Parent $bundleBackup) -Force | Out-Null
        Move-Item -LiteralPath $resolvedLoose -Destination $bundleBackup
    }
}
# Enable the resource profile and its loader paths without changing mod order.
function Enable-IniFlag([string]$Relative, [string]$Section, [string]$Key) {
    $iniPath = Join-Path $installGame $Relative
    $iniText = Get-Content -LiteralPath $iniPath -Raw
    $sectionPattern = '(?ms)(^\[' + [regex]::Escape($Section) + '\]\r?\n)(.*?)(?=^\[|\z)'
    $sectionMatch = [regex]::Match($iniText,$sectionPattern)
    if (!$sectionMatch.Success) { throw "Missing INI section: $Relative [$Section]" }
    $body = $sectionMatch.Groups[2].Value
    $flagPattern = '(?m)^' + [regex]::Escape($Key) + '=[^\r\n]*'
    if ([regex]::IsMatch($body,$flagPattern)) { $body = [regex]::Replace($body,$flagPattern,"$Key=true") }
    else { $body = "$Key=true`r`n$body" }
    $updated = $iniText.Substring(0,$sectionMatch.Index) + $sectionMatch.Groups[1].Value + $body + $iniText.Substring($sectionMatch.Index+$sectionMatch.Length)
    if ($updated -ne $iniText) {
        Save-InstalledFile $iniPath $Relative
        [System.IO.File]::WriteAllText($iniPath,$updated)
    }
}
Enable-IniFlag 'mods/profiles.ini' 'RaidenMoveset' 'Enabled'
foreach ($installFlag in @('LoadMods','LoadScripts','LoadFiles')) { Enable-IniFlag 'MGRModLoaderSettings.ini' 'ModLoader' $installFlag }
# Migrate only resources already owned by the enabled Mod Loader profile.
$installMod = Join-Path $installGame 'mods/RaidenMoveset'
if (!(Test-Path -LiteralPath $installMod)) { throw 'Install the RaidenMoveset Mod Loader resource profile first.' }
$installData = Join-Path $installGame 'GameData'
$installMigrations = @()
foreach ($installResource in Get-ChildItem -LiteralPath $installMod -Recurse -File) {
    $installRelative = $installResource.FullName.Substring($installMod.Length + 1)
    $installLoose = Join-Path $installData $installRelative
    if (!(Test-Path -LiteralPath $installLoose -PathType Leaf)) { continue }
    if ((Get-FileHash -LiteralPath $installLoose).Hash -ne (Get-FileHash -LiteralPath $installResource.FullName).Hash) {
        throw "Conflicting loose resource; migration stopped: $installRelative"
    }
    $installMigrations += [pscustomobject]@{Source=$installLoose; Relative=$installRelative}
}
foreach ($installMigration in $installMigrations) {
    $installSaved = Join-Path (Join-Path $installBackup 'GameData') $installMigration.Relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $installSaved) -Force | Out-Null
    Copy-Item -LiteralPath $installMigration.Source -Destination $installSaved
    if ((Get-FileHash -LiteralPath $installMigration.Source).Hash -ne (Get-FileHash -LiteralPath $installSaved).Hash) { throw 'Resource backup verification failed.' }
    Remove-Item -LiteralPath $installMigration.Source
    Write-Output "Moved to Mod Loader (existing copy verified): $($installMigration.Relative)"
}
Write-Output "Installed and hash verified. Backup: $installBackup"
$installManifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $installBackup 'installed-manifest.json')
Write-Output "Verified $($installManifest.Count) runtime files, including 3 packed animation DAT archives (34 stealth poses, 16 CQC sets, Sam template and base moveset clips). RaidenMoveset profile enabled."
