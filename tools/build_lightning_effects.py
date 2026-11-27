"""Author a dedicated native EFF/EST/EFT lightning bank (pl1c30).

Custom layouts, colours, delays and lifetimes are authored here. The particle
record's renderer flags come from a complete retail sprite for engine
compatibility. Custom DXT5 textures, native delays and lifetimes provide the
lightning; no overlay or custom graphics drawing path is involved.
Format field reference (not vendored code): ArthurHeitmann/F-SERVO,
lib/fileTypeUtils/effects/{estIO,estEntryTypes}.dart (EST/MOVE/PART/EMIF).
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import xml.etree.ElementTree as ET
import io
from PIL import Image, ImageDraw, ImageFilter
from assets import cpk_index, dat_entries, decompress
from build_dual_blade_animations import encode_bxm
from inspect_sam_sequences import parse
from merge_sam_bodies import pack_members

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_GAME = Path(r'C:\Program Files (x86)\Steam\steamapps\common\METAL GEAR RISING REVENGEANCE')


def records(blob):
    if len(blob) < 28:
        raise ValueError('Truncated EST')
    magic, count, offsets, types, start, stride, columns = struct.unpack_from('<4s6I', blob)
    if magic != b'EFF\0' or stride != 16 or not 0 < count < 4096 or not 0 < columns < 64:
        raise ValueError('Invalid EST header')
    if offsets + count*4 > len(blob) or types + count*columns*stride > len(blob):
        raise ValueError('Invalid EST tables')
    result = []
    names = []
    for row in range(count):
        base = struct.unpack_from('<I', blob, offsets+row*4)[0]
        chunks = {}
        for col in range(columns):
            flags, tag, size, local = struct.unpack_from('<I4sII', blob, types+(row*columns+col)*stride)
            if not row:
                names.append(tag)
            if size:
                if base < start or base+local+size > len(blob):
                    raise ValueError('EST record outside file')
                chunks[tag] = (flags, blob[base+local:base+local+size])
        result.append(chunks)
    return names, result


def encode(names, rows):
    align = lambda x: (x+15) & ~15
    count = len(rows)
    offsets = 32
    types = align(offsets+count*4)
    start = align(types+count*len(names)*16)
    out = bytearray(start)
    struct.pack_into('<4s6I', out, 0, b'EFF\0', count, offsets, types, start, 16, len(names))
    for i, row in enumerate(rows):
        out += b'\0' * (-len(out) % 16)
        base = len(out)
        struct.pack_into('<I', out, offsets+i*4, base)
        for j, tag in enumerate(names):
            flags, data = row.get(tag, (0, b''))
            struct.pack_into('<I4sII', out, types+(i*len(names)+j)*16,
                             flags, tag, len(data), len(out)-base if data else 0)
            out += data
    assert records(out)[1] == rows
    return bytes(out)


def particle(template, point, *, colour, life, delay=0, bone=0, width=0.10, variant=0, angle=0, texture_id=900):
    # Preserve native renderer options; replace every authored motion field.
    row = {tag: (flags, bytes(data)) for tag, (flags, data) in template.items()}
    for tag, minimum in ((b'PART', 64), (b'MOVE', 384), (b'EMIF', 48)):
        if len(row[tag][1]) != minimum:
            raise ValueError('Unsupported MGR particle layout')
    part = bytearray(row[b'PART'][1])
    struct.pack_into('<h', part, 2, life)
    struct.pack_into('<h', part, 12, bone)
    move = bytearray(row[b'MOVE'][1])
    struct.pack_into('<3f', move, 4, *point)
    struct.pack_into('<9f', move, 16, *([0.0]*9))  # spread, speed, random speed
    struct.pack_into('<f', move, 76, angle)
    struct.pack_into('<3f', move, 128, width, width*3.0, 0.0)
    struct.pack_into('<4f', move, 200, *colour)
    emitter = bytearray(row[b'EMIF'][1])
    struct.pack_into('<h', emitter, 0, 1)
    struct.pack_into('<h', emitter, 8, delay)
    struct.pack_into('<h', emitter, 10, 1)
    for tag, data in ((b'PART', part), (b'MOVE', move), (b'EMIF', emitter)):
        row[tag] = (row[tag][0], bytes(data))
    tex=bytearray(row[b'TEX '][1])
    struct.pack_into('<hh',tex,4,texture_id,0)
    struct.pack_into('<hBB',tex,28,100,variant,1)
    struct.pack_into('<h',tex,36,0)
    row[b'TEX ']=(row[b'TEX '][0],bytes(tex))
    return row


def bolt(a, b, rng, steps=16, jitter=0.12):
    for i in range(steps+1):
        t = i/steps
        p = [a[k]+(b[k]-a[k])*t for k in range(3)]
        if i and i < steps:
            p = [x+rng.uniform(-jitter, jitter) for x in p]
        yield tuple(p)


def layout(shape, reach, seed):
    """Yield (position, delay, hand bone, sprite width) in actor-local metres."""
    rng = random.Random(seed)
    if shape == 'charge':
        for arm in (13,):
            for branch in range(7):
                a = branch*math.tau/7
                end = (math.cos(a)*0.55, math.sin(a)*0.55, rng.uniform(-0.3, 0.3))
                for p in bolt((0, 0, 0), end, rng, 6, 0.07):
                    yield p, branch % 3, arm, 0.075
    elif shape in ('thrust', 'spear', 'double'):
        for side in ((-0.45, 0.45) if shape == 'double' else (0,)):
            for p in bolt((side, 1.15, 0.4), (side, 1.15, reach), rng, 48, 0.12):
                yield p, 0, 0, 0.13 if shape == 'spear' else 0.18
    elif shape in ('radial', 'storm'):
        for branch in range(12):
            a = branch*math.tau/12
            end = (math.sin(a)*reach, 0.35+(branch % 3)*0.4, math.cos(a)*reach)
            for p in bolt((0, 1.0, 0), end, rng, 15, 0.16):
                yield p, branch % 3, 0, 0.12
    elif shape == 'needles':
        for i in range(9):
            a = (i-4)*0.105
            for j, p in enumerate(bolt((0, 1.3, 0.6), (math.sin(a)*reach, 1.3, math.cos(a)*reach), rng, 20, 0.035)):
                yield p, j//2, 0, 0.065
    elif shape == 'wave':
        for layer in range(3):
            z = 1.0+layer*4.2
            for i in range(49):
                x = (i-24)/24*(0.6+z*0.32)
                yield (x, 1.0+0.25*math.cos(x), z+0.1*rng.uniform(-1, 1)), layer*6, 0, 0.14
    elif shape == 'kirin':
        # Main descending bolt, branching crown/horns and forked ground arcs.
        for p in bolt((0, reach, 0), (0, 0.25, 0), rng, 64, 0.27):
            yield p, 0, 0, 0.23
        for side in (-1, 1):
            for a, b in [((0, 9, 0), (side*1.8, 11.4, 0)),
                         ((0, 8, 0), (side*1.3, 7, 1.4)),
                         ((0, 0.4, 0), (side*3, 0.2, 1.8))]:
                for p in bolt(a, b, rng, 12, 0.12):
                    yield p, 1, 0, 0.16
    elif shape == 'impact':
        for i in range(6):
            a = i*math.tau/6
            for p in bolt((0, 1, 0), (math.sin(a)*0.7, 1+math.cos(a)*0.7, 0), rng, 4, 0.05):
                yield p, 0, 0, 0.095
    else:
        raise ValueError(shape)


def lightning_textures():
    """Four authored lightning sprite variants, stored as native DXT5 DDSs."""
    textures=[]
    for variant in range(4):
        rng=random.Random(4100+variant)
        mask=Image.new('L',(256,256),0);draw=ImageDraw.Draw(mask)
        points=[(128+rng.randint(-38,38),y) for y in range(12,245,14)]
        draw.line(points,fill=255,width=3)
        for i in (3,7,11):
            x,y=points[i];side=-1 if i%2 else 1
            draw.line([(x,y),(x+side*30,y+8),(x+side*46,y+30),(x+side*64,y+18)],fill=220,width=2)
        halo=mask.filter(ImageFilter.GaussianBlur(5))
        from PIL import ImageChops
        alpha=ImageChops.add(mask,ImageChops.multiply(halo,Image.new('L',halo.size,220)))
        sprite=Image.new('RGBA',mask.size,(255,255,255,0));sprite.putalpha(alpha)
        output=io.BytesIO();sprite.save(output,format='DDS',pixel_format='DXT5')
        blob=output.getvalue();assert blob[84:88]==b'DXT5'
        textures.append(blob)
    return textures


def build(source, textures, destination, config):
    # Source assets remain read-only. Only a complete, simple native leaf
    # emitter is used as a renderer-compatible record template.
    retail=dict(dat_entries(dict(dat_entries(source))['core.eff']))
    # 316/3 carries a mesh/material selector (TEX 44:48 = 1,2) and
    # conditional PART data. Those dependencies are absent in this bank.
    # 118/0 is a self-contained native blue billboard, with no model binding.
    names,donor=records(retail['118.est'])
    template=dict(donor[0])
    # Use the native neutral billboard sampler: no UV animation/frame offset,
    # external mesh selector or dimming inherited from a particular effect.
    template[b'TEX ']=records(retail['316.est'])[1][0][b'TEX ']
    assert set(template)=={b'PART',b'MOVE',b'EMIF',b'TEX '}
    atlas=config['texture_id']
    if not 0<=atlas<1000 or f'{atlas:03}.wta' in retail:
        raise ValueError('Custom atlas must not replace a retail effect atlas')
    header=ET.Element('root');est=ET.SubElement(header,'est');tex=ET.SubElement(header,'tex');ET.SubElement(header,'model')
    members={};report=[]
    specs=[]
    for i,move in enumerate(config['moves']):
        specs.extend([(config['first_charge_id']+i,move,'charge',65),
                      (config['first_release_id']+i,move,move['shape'],move['life'])])
    specs.extend([(config['impact_id'],dict(name='Impact',reach=1),'impact',18),
                  (config['miss_id'],dict(name='Kirin miss',reach=12),'kirin',44)])
    for effect_id,move,shape,life in specs:
        points=list(layout(shape,move['reach'],effect_id))
        # Three short, staggered particle layers cycle different bolt textures.
        # This animation belongs to native emitter delays and particle lifetime.
        sampled=points[::3]
        rows=[]
        for frame in range(10 if shape=='charge' else 3):
            for j,(pos,delay,bone,width) in enumerate(sampled):
                if effect_id==config['miss_id']:pos=(pos[0],pos[1],pos[2]+8)
                rows.append(particle(template,pos,colour=config['colour'],life=12,
                    delay=delay+frame*6,bone=0,width=max(width,0.08),
                    variant=(j+frame)%4,angle=((j%7)-3)*0.25,texture_id=atlas))
        assert len(rows)<=256
        blob=encode(names,rows);members[f'{effect_id:03}.est']=blob
        ET.SubElement(est,'data',id=str(effect_id),sst='0')
        report.append(dict(id=effect_id,name=move['name'],phase='charge' if shape=='charge' else 'release',
            shape=shape,emitters=len(rows),sha256=hashlib.sha256(blob).hexdigest()))
    images=lightning_textures();payload=bytearray();offsets=[]
    for image in images:
        payload+=b'\0'*(-len(payload)%4096);offsets.append(len(payload));payload+=image
    # Retail WTB v0: independent offsets, sizes and flags tables. No global IDs.
    wta=bytearray(128);struct.pack_into('<4s7I',wta,0,b'WTB\0',0,4,32,64,96,0,128)
    for i,image in enumerate(images):
        struct.pack_into('<I',wta,32+i*4,offsets[i]);struct.pack_into('<I',wta,64+i*4,len(image));struct.pack_into('<I',wta,96+i*4,2)
    members[f'{atlas:03}.wta']=bytes(wta);ET.SubElement(tex,'data',id=f'{atlas:03}')
    members['header.bxm']=encode_bxm(header)
    output=pack_members({'pl1c30.eff':pack_members(members)})
    texture_output=pack_members({'pl1c30.eft':pack_members({f'{atlas:03}.wtp':bytes(payload)})})
    destination.parent.mkdir(parents=True,exist_ok=True)
    destination.write_bytes(output);destination.with_suffix('.dtt').write_bytes(texture_output)
    manifest_path=ROOT/'Release/lightning-effects-manifest.json'
    previous=json.loads(manifest_path.read_text()) if manifest_path.exists() else {}
    retired=previous.get('retired_core',{})
    if 'texture_id' in previous and previous.get('bank')==0x7c0000:
        retired={'core/coreeff.dat':previous['output_sha256'],'core/coreeff.dtt':previous['texture_output_sha256']}
    manifest=dict(bank=config['bank'],archive='pl/pl1c30.dat',effects=report,
        impact_id=config['impact_id'],miss_id=config['miss_id'],texture_id=atlas,
        source_sha256=hashlib.sha256(source).hexdigest(),
        output_sha256=hashlib.sha256(output).hexdigest(),
        texture_output_sha256=hashlib.sha256(texture_output).hexdigest(),
        retired_core=retired,gameplay_verified=False)
    manifest_path.write_text(json.dumps(manifest,indent=2)+'\n')
    # Retire only our own old generated core overrides, matched by ownership
    # hashes. The source retail cache and unrelated staging files stay intact.
    for relative in ('core/coreeff.dat','core/coreeff.dtt'):
        old=ROOT/'Release/GameData'/relative
        if old.exists() and hashlib.sha256(old.read_bytes()).hexdigest()==retired.get(relative):
            old.unlink()
    return manifest


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--game', type=Path, default=DEFAULT_GAME)
    args = ap.parse_args()
    cache = ROOT/'local_assets/lightning/coreeff-source.dat'
    for suffix in ('.dat', '.dtt'):
        cached = cache.with_suffix(suffix)
        if not cached.exists():
            cpk = args.game/'GameData/data000.cpk'
            entry = next(e for e in cpk_index(cpk) if e['path'].lower() == 'core/coreeff'+suffix)
            with cpk.open('rb') as stream:
                stream.seek(entry['offset'])
                source = decompress(stream.read(entry['size']))
            cached.parent.mkdir(parents=True, exist_ok=True)
            cached.write_bytes(source)
    config = json.loads((ROOT/'assets/lightning/effects.json').read_text())
    manifest = build(cache.read_bytes(), cache.with_suffix('.dtt').read_bytes(), ROOT/'Release/GameData/pl/pl1c30.dat', config)
    print("Built dedicated native lightning bank: 18 EST effects, four DXT5 lightning textures; retail banks unchanged.")


if __name__ == '__main__':
    main()
