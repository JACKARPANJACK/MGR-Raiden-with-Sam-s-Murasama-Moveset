param([Parameter(Mandatory=$true)][string]$GameDirectory)
$ErrorActionPreference = 'Stop'
$installRoot = Split-Path -Parent $PSScriptRoot
$installGame = (Resolve-Path -LiteralPath $GameDirectory).Path
if (!(Test-Path -LiteralPath (Join-Path $installGame 'METAL GEAR RISING REVENGEANCE.exe'))) { throw 'Game executable not found in the supplied directory.' }
if (Get-Process -Name '*REVENGEANCE*' -ErrorAction SilentlyContinue) { throw 'Close the game before installing the plugin.' }
$installTarget = Join-Path $installGame 'scripts'
$installBackup = Join-Path $installRoot ('backups/installed-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $installTarget,$installBackup -Force | Out-Null
foreach ($installName in @('Raiden Moveset.asi','Raiden Moveset.pdb')) {
    $installSource = Join-Path $installRoot ('Release/' + $installName)
    if (!(Test-Path -LiteralPath $installSource)) { throw "Build output missing: $installName" }
    $installDestination = Join-Path $installTarget $installName
    if (Test-Path -LiteralPath $installDestination) { Copy-Item -LiteralPath $installDestination -Destination $installBackup }
    Copy-Item -LiteralPath $installSource -Destination $installDestination -Force
    if ((Get-FileHash -LiteralPath $installSource).Hash -ne (Get-FileHash -LiteralPath $installDestination).Hash) { throw 'Installed file verification failed.' }
}
Write-Output "Installed and hash verified. Backup: $installBackup"
