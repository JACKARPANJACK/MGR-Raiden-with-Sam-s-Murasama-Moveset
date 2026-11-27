"""Package moveset, guns, CQC and stealth for an existing MGR ModLoader install."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game',type=Path)
    args=parser.parse_args();game=args.game.resolve()
    stage=ROOT/'Release/RaidenMoveset-Installer'
    stage.mkdir(parents=True,exist_ok=True)
    assert not list(stage.iterdir()), 'Use a fresh package staging directory'
    files=[]
    def include(source,relative):
        dest=stage/relative;dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source,dest)
        with dest.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
        files.append(dict(path=relative,sha256=digest,size=dest.stat().st_size))
    excluded={'pl/pl1300.dat','pl/pl1307.dat','pl/pl1307.dtt','pl/pl10a5.dat'}
    profile=game/'mods/RaidenMoveset'
    for path in sorted(profile.rglob('*')):
        if not path.is_file() or path.suffix.lower() not in ('.dat','.dtt'):continue
        relative=path.relative_to(profile).as_posix()
        if relative in excluded:continue
        assert path.parent.name in ('pl','wp','ui','ckmsg','core','dlc0','dlc1','dlc2','dlc3')
        include(path,path.relative_to(game).as_posix())
    include(game/'Scripts/Raiden Moveset.asi','Scripts/Raiden Moveset.asi')
    include(ROOT/'SamMoveset.ini','SamMoveset.ini')
    assert all(f['path'].startswith('mods/RaidenMoveset/') or f['path'] in
               ('Scripts/Raiden Moveset.asi','SamMoveset.ini') for f in files)
    assert [f['path'] for f in files if f['path'].endswith('.asi')]==['Scripts/Raiden Moveset.asi']
    metadata=dict(files=files,profiles=['RaidenMoveset'],
                  requires='An existing working MGR ModLoader installation',
                  features=['Moveset','Guns','CQC','Stealth'])
    (stage/'package-manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
    shutil.copy2(ROOT/'tools/package_install.ps1',stage/'Install.ps1')
    (stage/'Install.cmd').write_text('@echo off\nsetlocal\npowershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install.ps1" %*\nif errorlevel 1 (\n  echo Installation failed. Read the error above.\n  pause\n  exit /b 1\n)\npause\n')
    (stage/'README-INSTALL.txt').write_text(r'''RAIDEN MOVESET / GUNS / CQC / STEALTH

Requires an existing working MGR ModLoader installation.

1. Close Metal Gear Rising.
2. Extract this ZIP into a separate folder.
3. Run Install.cmd. It detects the usual Steam folder or asks for the game folder.
   Or run Install.cmd -GameDirectory "D:\SteamLibrary\steamapps\common\METAL GEAR RISING REVENGEANCE"
4. Launch normally. 0 opens the menu. G / R3 cycles standing, crouch and prone.
   Select Murasama / Iaido katana for Sam's moveset.

Contents: Scripts/Raiden Moveset.asi, SamMoveset.ini, and the required animation,
weapon, gun, sound, effect and inventory UI resources in mods/RaidenMoveset.
Animation banks: pl1c00.dat, pl1c10.dat, pl1c20.dat and wp2062.dat.
Animation members use plxxxx_yyyy / wpxxxx_yyyy naming.

No custom skins, suit framework, unrelated ASI plugins, ModLoader binaries,
source code or developer files are included. Install.cmd/Install.ps1 are only
installation helpers. No Python or SDK is required to install.

Existing settings, other mods and their order are preserved. Only RaidenMoveset
is registered/enabled. Replaced files are backed up in RaidenMoveset-Backups.
File and script loading are enabled in your existing ModLoader settings.

Manual setup: copy mods/RaidenMoveset into mods, the ASI into Scripts, and put
SamMoveset.ini beside the game executable. Enable RaidenMoveset in ModLoader.
Use the installer for automatic registration, backups and file verification.
Do not load a separate weapon-switcher plugin alongside the integrated moveset.
''')
    print(f'Prepared {len(files)} payload files, {sum(f["size"] for f in files)/1024**2:.1f} MiB in {stage}',flush=True)
    output=ROOT/'Release/RaidenMoveset-Installer.zip'
    with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=3,allowZip64=True) as z:
        for path in sorted(stage.rglob('*')):
            if path.is_file():z.write(path,path.relative_to(stage).as_posix())
    print(f'ZIP built: {output} ({output.stat().st_size/1024**2:.1f} MiB)',flush=True)

if __name__=='__main__':main()
