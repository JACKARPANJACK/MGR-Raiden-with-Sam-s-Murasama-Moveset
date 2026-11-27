param([string]$GameDirectory, [switch]$VerifyOnly)
$ErrorActionPreference = 'Stop'
$packageRoot = $PSScriptRoot
$packageManifest = Get-Content -LiteralPath (Join-Path $packageRoot 'package-manifest.json') -Raw | ConvertFrom-Json

function Get-SafeChild([string]$Parent, [string]$Relative) {
    $full = [IO.Path]::GetFullPath((Join-Path $Parent $Relative))
    if (!$full.StartsWith($Parent.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Unsafe package path: $Relative"
    }
    return $full
}
Write-Host 'Checking package files...'
foreach ($entry in $packageManifest.files) {
    $source = Get-SafeChild $packageRoot $entry.path
    if (!(Test-Path -LiteralPath $source -PathType Leaf) -or
        (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ne $entry.sha256) {
        throw "Missing or damaged package file: $($entry.path). Extract the complete ZIP again."
    }
}
if ($VerifyOnly) { Write-Host "Package verified: $($packageManifest.files.Count) files."; exit 0 }
if (!$GameDirectory) {
    $candidates = @($packageRoot)
    $steamLocation = (Get-ItemProperty -LiteralPath 'HKCU:\Software\Valve\Steam' -ErrorAction SilentlyContinue).SteamPath
    if ($steamLocation) { $candidates += Join-Path $steamLocation 'steamapps/common/METAL GEAR RISING REVENGEANCE' }
    $candidates += Join-Path ${env:ProgramFiles(x86)} 'Steam/steamapps/common/METAL GEAR RISING REVENGEANCE'
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath (Join-Path $candidate 'METAL GEAR RISING REVENGEANCE.exe')) { $GameDirectory = $candidate; break }
    }
    if (!$GameDirectory) { $GameDirectory = (Read-Host 'Paste the game folder containing METAL GEAR RISING REVENGEANCE.exe').Trim('"') }
}
$game = (Resolve-Path -LiteralPath $GameDirectory).Path.TrimEnd('\')
if (!(Test-Path -LiteralPath (Join-Path $game 'METAL GEAR RISING REVENGEANCE.exe'))) { throw 'The selected folder does not contain the game executable.' }
if (Get-Process -Name '*REVENGEANCE*' -ErrorAction SilentlyContinue) { throw 'Close Metal Gear Rising before installing.' }
foreach($required in @('MGRModLoader.asi','MGRModLoaderSettings.ini')) {
    if(!(Test-Path -LiteralPath (Join-Path $game $required))) { throw 'Install MGR ModLoader first, then run this moveset installer.' }
}
if ($game -eq $packageRoot.TrimEnd('\')) { throw 'Extract this ZIP into a separate folder, then run Install.cmd from there so existing files can be backed up.' }
$backup = Join-Path $game ('RaidenMoveset-Backups/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $backup -Force | Out-Null
$installed = @()
foreach ($entry in $packageManifest.files) {
    $source = Get-SafeChild $packageRoot $entry.path
    $target = Get-SafeChild $game $entry.path
    # Keep user preferences; supplied INIs are defaults for a fresh install.
    $preserve = $entry.path -in @('SamMoveset.ini', 'Scripts/CustomSuitSlot.ini')
    if ($preserve -and (Test-Path -LiteralPath $target)) { continue }
    $hadOriginal = Test-Path -LiteralPath $target -PathType Leaf
    if ($hadOriginal -and (Get-FileHash -LiteralPath $target).Hash -eq $entry.sha256) { continue }
    if ($hadOriginal) {
        $saved = Get-SafeChild $backup $entry.path
        New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force | Out-Null
        Copy-Item -LiteralPath $target -Destination $saved
        if ((Get-FileHash -LiteralPath $target).Hash -ne (Get-FileHash -LiteralPath $saved).Hash) { throw "Backup failed: $($entry.path)" }
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $target -Force
    if ((Get-FileHash -LiteralPath $target).Hash -ne $entry.sha256) { throw "Installation verification failed: $($entry.path)" }
    $installed += [pscustomobject]@{Path=$entry.path; HadOriginal=$hadOriginal; SHA256=$entry.sha256}
}

function Save-Config([string]$Relative, [string]$Content) {
    $target = Get-SafeChild $game $Relative
    $saved = Get-SafeChild $backup $Relative
    if ((Test-Path -LiteralPath $target) -and !(Test-Path -LiteralPath $saved)) {
        New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force | Out-Null
        Copy-Item -LiteralPath $target -Destination $saved
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    [IO.File]::WriteAllText($target,$Content)
}
function Set-IniFlag([string]$Text, [string]$Section, [string]$Key) {
    $pattern = '(?ms)(^\[' + [regex]::Escape($Section) + '\]\r?\n)(.*?)(?=^\[|\z)'
    $match = [regex]::Match($Text,$pattern)
    if (!$match.Success) { return $Text.TrimEnd() + "`r`n[$Section]`r`n$Key=true`r`n" }
    $body = $match.Groups[2].Value
    $keyPattern = '(?m)^\s*' + [regex]::Escape($Key) + '\s*=[^\r\n]*'
    if ([regex]::IsMatch($body,$keyPattern)) { $body = [regex]::Replace($body,$keyPattern,"$Key=true") }
    else { $body = "$Key=true`r`n$body" }
    return $Text.Substring(0,$match.Index) + $match.Groups[1].Value + $body + $Text.Substring($match.Index+$match.Length)
}
$profilePath = Join-Path $game 'mods/profiles.ini'
$profiles = if (Test-Path -LiteralPath $profilePath) { Get-Content -LiteralPath $profilePath -Raw } else { '' }
$places = @([regex]::Matches($profiles,'(?m)^MyPlace\s*=\s*(\d+)') | ForEach-Object { [int]$_.Groups[1].Value })
$nextPlace = if ($places.Count) { ($places | Measure-Object -Maximum).Maximum + 1 } else { 0 }
foreach ($name in $packageManifest.profiles) {
    if ($profiles -notmatch ('(?m)^\[' + [regex]::Escape($name) + '\]')) {
        $profiles = $profiles.TrimEnd() + "`r`n[$name]`r`nEnabled=true`r`nMyPlace=$nextPlace`r`n"
        $nextPlace++
    } else { $profiles = Set-IniFlag $profiles $name 'Enabled' }
}
Save-Config 'mods/profiles.ini' $profiles
$loaderSettings = Get-Content -LiteralPath (Join-Path $game 'MGRModLoaderSettings.ini') -Raw
foreach ($flag in @('LoadMods','LoadFiles','LoadScripts')) { $loaderSettings = Set-IniFlag $loaderSettings 'ModLoader' $flag }
Save-Config 'MGRModLoaderSettings.ini' $loaderSettings

# Retire previous descriptive archive names after native-ID banks are installed.
foreach($relative in @('mods/RaidenMoveset/pl/stealth_anims.dat','mods/RaidenMoveset/pl/sam_template_anims.dat','mods/RaidenMoveset/pl/base_moveset_anims.dat','mods/RaidenMoveset/wp/pistol_anims.dat')) {
    $legacy=Get-SafeChild $game $relative
    if(!(Test-Path -LiteralPath $legacy -PathType Leaf)){continue}
    $saved=Get-SafeChild $backup "legacy/$relative"
    $oldHash=(Get-FileHash -LiteralPath $legacy).Hash
    New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force | Out-Null
    Move-Item -LiteralPath $legacy -Destination $saved
    if((Get-FileHash -LiteralPath $saved).Hash -ne $oldHash){throw "Archive backup failed: $relative"}
}
# Retire the previous loose bundles; packed DAT readers no longer use them.
foreach ($bundle in @('StealthAnims','SamTemplateAnims','BaseMoveset')) {
    foreach ($parent in @('Scripts','mods/RaidenMoveset')) {
        $relative = "$parent/$bundle"
        $legacy = Get-SafeChild $game $relative
        if (!(Test-Path -LiteralPath $legacy)) { continue }
        $item = Get-Item -LiteralPath $legacy -Force
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            if ($item.LinkType -ne 'Junction' -or
                [IO.Path]::GetFullPath([string]$item.Target) -ne (Get-SafeChild $game "mods/RaidenMoveset/$bundle")) {
                throw "Unexpected old animation link: $legacy"
            }
            [IO.Directory]::Delete($legacy)
        } else {
            $saved = Get-SafeChild $backup "legacy/$relative"
            New-Item -ItemType Directory -Path (Split-Path -Parent $saved) -Force | Out-Null
            Move-Item -LiteralPath $legacy -Destination $saved
        }
    }
}
$installed | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $backup 'installed-files.json')
Write-Host "Installed Raiden Moveset and $($packageManifest.profiles.Count) ModLoader profiles."
Write-Host 'Installed package payload and registered its ModLoader profiles. Animation assets are packed in pl/wp DAT files.'
Write-Host "Backups: $backup"
Write-Host 'Launch the game normally. G / R3 cycles standing, crouch and prone; 0 opens the moveset menu.'
