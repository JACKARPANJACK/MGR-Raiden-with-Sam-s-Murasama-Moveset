"""Build a versioned release with audited Sam-compatible suits and CustomSuitSlot."""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

from patch_installed_skin_bones import audit

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game', type=Path)
    parser.add_argument('--version', default='v0.6.0-beta')
    parser.add_argument('--reuse-staging', action='store_true', help='Reuse this version staging folder after an interrupted build')
    args = parser.parse_args()
    if not args.version or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-' for c in args.version):
        parser.error('Version must contain only letters, digits, dots and hyphens')
    game = args.game.resolve()
    report = audit(game)
    if report['patchedArchiveCount']:
        raise RuntimeError('Suit archives need Sam bones; adapt them before packaging')
    selected = set(report['bodyProfiles'] + report['inheritedBodyProfiles'])
    config = configparser.ConfigParser()
    config.read(game / 'mods/profiles.ini')
    profiles = sorted(selected, key=lambda name: config.getint(name, 'MyPlace'))
    stage = ROOT / 'Release' / f'RaidenMoveset-{args.version}-Complete'
    stage.mkdir(parents=True, exist_ok=args.reuse_staging)
    files = []

    def include(source, relative):
        target = stage / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        files.append(dict(path=relative, sha256=digest(target), size=target.stat().st_size))

    for profile in profiles:
        folder = game / 'mods' / profile
        for source in sorted(folder.rglob('*')):
            if source.is_file() and (source.suffix.lower() in ('.dat', '.dtt') or source.name in ('suit.ini', 'mod.ini')):
                include(source, source.relative_to(game).as_posix())
    include(ROOT / 'Release/Raiden Moveset.asi', 'Scripts/Raiden Moveset.asi')
    for name in ('CustomSuitSlot.asi', 'CustomSuitSlot.ini', 'MGR-Unlimiter.asi'):
        include(game / 'Scripts' / name, 'Scripts/' + name)
    include(ROOT / 'SamMoveset.ini', 'SamMoveset.ini')
    # Keep portable structural evidence with the payload, without machine paths.
    report.pop('backup', None)
    (stage / 'suit-bones-audit.json').write_text(json.dumps(report, indent=2) + '\n')
    metadata = dict(version=args.version, files=files, profiles=profiles,
                    requires='An existing working MGR ModLoader installation',
                    features=['Moveset', 'Guns', 'CQC', 'Stealth', 'Lightning', 'Sam-compatible suits', 'CustomSuitSlot'])
    (stage / 'package-manifest.json').write_text(json.dumps(metadata, indent=2) + '\n')
    shutil.copy2(ROOT / 'tools/package_install.ps1', stage / 'Install.ps1')
    (stage / 'Install.cmd').write_text('@echo off\nsetlocal\npowershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install.ps1" %*\nif errorlevel 1 (\n  echo Installation failed. Read the error above.\n  pause\n  exit /b 1\n)\npause\n')
    (stage / 'README-INSTALL.txt').write_text(
        f'RAIDEN MOVESET {args.version} - COMPLETE SUIT BUNDLE\n\n'
        'Requires an existing working MGR ModLoader installation. Close the game,\n'
        'extract this archive into a separate folder and run Install.cmd.\n'
        'For a custom location: Install.cmd -GameDirectory "D:\\SteamLibrary\\steamapps\\common\\METAL GEAR RISING REVENGEANCE"\n\n'
        'Includes Raiden Moveset.asi, CustomSuitSlot.asi, MGR-Unlimiter.asi\n'
        '(required by RaidenRemaster), default INIs, packed\n'
        'moveset resources and these profiles in their tested source order:\n'
        + '\n'.join(profiles) + '\n\n'
        '23 body models passed the Sam bone lookup/sheath audit. Sunglasses use\n'
        'the shared body. Geometry and existing skin bones were preserved during\n'
        'adaptation. Structural verification does not replace in-game testing.\n\n'
        'Existing SamMoveset.ini and CustomSuitSlot.ini preferences are preserved.\n'
        'Replaced files and updated ModLoader configuration are backed up under\n'
        'RaidenMoveset-Backups. Existing profiles retain their priority order.\n'
        'CustomSuitSlot detects the included suit.ini registrations. RaidenRemaster\n'
        'is a shared base-body replacement. Disable conflicting costume overrides\n'
        'if your existing mod priority changes their appearance.\n\n'
        '0 opens the moveset menu; G / R3 cycles standing, crouch and prone.\n'
        'Select Murasama / Iaido katana to enable Sam combat.\n'
        'No ModLoader binaries, logs, debug symbols or unrelated ASIs are bundled.\n')
    output = stage.parent / (stage.name + '.zip')
    print(f'Packing {len(files)} payload files ({sum(f["size"] for f in files) / 1024**2:.1f} MiB)', flush=True)
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=3, allowZip64=True) as archive:
        for path in sorted(stage.rglob('*')):
            if path.is_file():
                archive.write(path, path.relative_to(stage).as_posix())
    checksum = output.with_name(output.name + '.sha256')
    checksum.write_text(f'{digest(output)}  {output.name}\n')
    print(f'Built {output} ({output.stat().st_size / 1024**2:.1f} MiB)', flush=True)


if __name__ == '__main__':
    main()
