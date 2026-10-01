"""Verify native sequence effect references against installed EFF headers."""
import json
import re
import sys
from pathlib import Path
from assets import dat_entries
from inspect_sam_sequences import parse

ROOT = Path(__file__).resolve().parent.parent
PLAYABLE = ROOT / 'local_assets/data107/pl/pl1400.dat.unpacked'
BOSS = ROOT / 'local_assets/data000/em/em0020.dat.unpacked'
CORE = ROOT / 'local_assets/data000/core/coreeff.dat.unpacked'
WOLF = ROOT / 'local_assets/data000/em/em0220.dat.unpacked'
HEATBLADE = ROOT / 'local_assets/data000/wp/wp0372.dat.unpacked'
RAIDEN = ROOT / 'local_assets/data000/pl/pl0010.dat.unpacked'
GRENADE = ROOT / 'local_assets/data000/wp/wp1011.dat.unpacked'

def bank(path):
    files = dict(dat_entries(path.read_bytes()))
    return {int(e.get('id')) for e in parse(files['header.bxm']).find('est')}

def audit(verify=False):
    banks = {0x11400: bank(PLAYABLE / 'pl1400.eff'),
             0x20020: bank(BOSS / 'em0020.eff'),
             0x7C0000: bank(CORE / 'core.eff'),
             0x20220: bank(WOLF / 'em0220.eff'),
             0x30372: bank(HEATBLADE / 'wp0372.eff'),
             0x31011: bank(GRENADE / 'wp1011.eff')}
    codes = set(re.findall(r'"([0-9a-f]{4})"', (ROOT / 'SamUltimatePolicy.h').read_text()))
    moves = []
    missing = []
    stock_missing = []
    repaired = []
    raiden_codes = {'2400','3501','2420','2422'}
    for folder in (PLAYABLE, BOSS, WOLF, HEATBLADE, RAIDEN):
        for path in sorted(folder.glob('*seq.bxm')):
            if folder == RAIDEN and path.name.split('_')[1] not in raiden_codes:
                continue
            if folder == BOSS and path.name.split('_')[1] not in codes:
                continue
            effects = []
            original = parse(path.read_bytes())
            actual = original
            adapted = ROOT / 'Release/effect-sequences' / path.name
            if verify and (folder == BOSS or path.name == 'pl1400_242b_2_seq.bxm'):
                assert adapted.exists(), f'Missing actual adapter output: {path.name}'
                actual = parse(adapted.read_bytes())
                old_fx = [dict(e.attrib) for e in original.iter('Seq') if e.get('EffNo') is not None]
                new_fx = [dict(e.attrib) for e in actual.iter('Seq') if e.get('EffNo') is not None]
                assert len(old_fx) == len(new_fx)
                for old, new in zip(old_fx, new_fx):
                    expected = dict(old)
                    if folder == BOSS:
                        expected['LayerFlag'] = '4294967295'
                    elif old['EffDataId'] == '70656' and old['EffNo'] == '146':
                        expected['EffNo'] = '119'
                    assert new == expected, (path.name, old, new)
                repaired.append(path.name)
            for e in actual.iter('Seq'):
                if e.get('EffNo') is None:
                    continue
                source, number = int(e.get('EffDataId')), int(e.get('EffNo'))
                # Native player bank 10010 is resolved by the engine alias table;
                # retain its references and report separately rather than guess.
                resolved = number in banks[source] if source in banks else None
                effects.append(dict(e.attrib, assetPresent=resolved))
                if resolved is False:
                    failure = {'sequence': path.name, 'bank': source, 'number': number}
                    # Wolf animation sequences are inspected as stock fixtures;
                    # the player uses the native projectile, never Wolf's graph.
                    # Report stock-only gaps separately from executable mod clips.
                    (stock_missing if folder == WOLF else missing).append(failure)
            moves.append({'sequence': path.name, 'effects': effects})
    alias_refs = [{'sequence': m['sequence'], 'bank': e['EffDataId'], 'number': e['EffNo']}
                  for m in moves for e in m['effects'] if e['assetPresent'] is None]
    report = {'sequences': len(moves), 'effects': sum(len(m['effects']) for m in moves),
              'missing': missing, 'runtimeAdaptersVerified': repaired,
              'engineAliasReferences': alias_refs,
              'registeredBankAssets': {hex(k): len(v) for k,v in banks.items()},
              'stockOnlyMissingReferences': stock_missing,
              'moves': moves, 'gameplayVerified': False}
    out = ROOT / 'Release/effect-coverage.json'
    out.parent.mkdir(exist_ok=True)
    out.write_text(json.dumps(report, indent=2))
    for n in (117, 118, 119, 121):
        assert n in banks[0x11400], f'Missing charge stage EST {n}'
    assert 316 in banks[0x7C0000], 'Missing electric impact EST'
    print(json.dumps({k: v for k, v in report.items() if k not in ('moves','runtimeAdaptersVerified','engineAliasReferences')},
                     indent=2))
    print(f'{len(alias_refs)} native alias references preserved; their engine alias resolution requires gameplay verification')
    if verify:
        assert not missing, missing
        print(f'PASS: {len(repaired)} runtime sequence adapters preserve effect timing, banks, offsets and controllers')
    return report

if __name__ == '__main__':
    audit('--verify' in sys.argv)
