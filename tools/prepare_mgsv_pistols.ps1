param([string]$OutputRoot=(Join-Path $PSScriptRoot '../local_assets/mgsv/pistols'))
$ErrorActionPreference='Stop'
$mgsvRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../local_assets/mgsv'))
$toolRoot=Join-Path $mgsvRoot 'tools/GzsTool'
foreach($dll in 'CityHash.dll','Zlib.Portable.dll','GzsTool.Core.dll') {
    [Reflection.Assembly]::LoadFrom((Join-Path $toolRoot $dll)) | Out-Null
}
[GzsTool.Core.Utility.Hashing]::ReadDictionary((Join-Path $mgsvRoot 'tools/mgsv-lookup-strings/GzsTool/qar_dictionary.txt'))
[GzsTool.Core.Utility.Hashing]::ReadMd5Dictionary((Join-Path $toolRoot 'fpk_dictionary.txt'))
$index=Get-Content -LiteralPath (Join-Path $mgsvRoot 'qar-index.json') -Raw | ConvertFrom-Json
$selected=$index | Where-Object {$_.path -match '/chimera/(receiver/hg0[012]_main0_def_v00|magazine/hg01_ammo0_def|barrel/hg02_barl0_def_v00|suppressor/[^/]+)\.fpk$'}
$manifest=@()
foreach($group in ($selected | Group-Object archive)) {
    $stream=[IO.File]::OpenRead($group.Name)
    try {
        $archive=[GzsTool.Core.Qar.QarFile]::new();$archive.Read($stream)
        foreach($wanted in $group.Group) {
            $entry=$archive.Entries | Where-Object {$_.Hash.ToString('X16') -eq $wanted.hash} | Select-Object -First 1
            if(!$entry) {throw "Missing indexed entry: $($wanted.path)"}
            $dest=Join-Path $OutputRoot ([IO.Path]::GetFileName($wanted.path))
            New-Item -ItemType Directory -Force $OutputRoot | Out-Null
            $data=$entry.Export($stream).DataStream.Invoke()
            try {$out=[IO.File]::Create($dest);try {$data.CopyTo($out)}finally {$out.Dispose()}}finally {$data.Dispose()}
            & (Join-Path $toolRoot 'GzsTool.exe') $dest | Out-Null
            if($LASTEXITCODE -ne 0) {throw "FPK unpack failed: $dest"}
            $sha=[Security.Cryptography.SHA256]::Create()
            try {$digest=[BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes($dest))).Replace('-','')}finally {$sha.Dispose()}
            $manifest+=@{archive=$group.Name;source=$wanted.path;output=[IO.Path]::GetFullPath($dest);sha256=$digest}
            Write-Output "Extracted $($wanted.path)"
        }
    }finally {$stream.Dispose()}
}
$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $OutputRoot 'extraction-manifest.json')
