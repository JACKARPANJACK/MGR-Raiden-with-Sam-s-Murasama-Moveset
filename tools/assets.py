"""Read-only MGR CPK/DAT inspection and selective extraction (Python 3).

CPK/UTF reference: https://gist.github.com/unknownbrackets/78c4631a4091044d381432ffb7f1bae4
Never modifies a source archive. Extracted game assets are for local use only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


def utf(data):
    if data[:4] != b'@UTF':
        raise ValueError('Expected unencrypted @UTF table')
    size, rows, strings, blobs, name, columns, width, count = struct.unpack_from('>5I2HI', data, 4)
    if size + 8 > len(data):
        raise ValueError('Truncated UTF')
    rows, strings, blobs = rows + 8, strings + 8, blobs + 8
    def string(offset):
        start = strings + offset
        return data[start:data.index(0, start)].decode('utf-8')
    def value(kind, at):
        formats = {0:'B', 1:'b', 2:'H', 3:'h', 4:'I', 5:'i', 6:'Q', 7:'q', 8:'f', 9:'d', 10:'I', 11:'II'}
        fmt = '>' + formats[kind]
        values = struct.unpack_from(fmt, data, at)
        result = values[0]
        if kind == 10:
            result = string(result)
        elif kind == 11:
            result = data[blobs + result:blobs + result + values[1]].hex()
        return result, at + struct.calcsize(fmt)
    schema, at = [], 32
    for _ in range(columns):
        flags, offset = struct.unpack_from('>BI', data, at)
        at += 5
        storage, kind = flags & 0xf0, flags & 15
        constant = 0
        if storage == 0x30:
            constant, at = value(kind, at)
        elif storage not in (0x10, 0x50):
            raise ValueError(f'Unsupported UTF storage {storage:#x}')
        schema.append((string(offset), storage, kind, constant))
    result = []
    for row in range(count):
        at, record = rows + row * width, {}
        for key, storage, kind, constant in schema:
            if storage == 0x50:
                record[key], at = value(kind, at)
            else:
                record[key] = constant
        if at != rows + (row + 1) * width:
            raise ValueError('UTF row size mismatch')
        result.append(record)
    return result


def packet(stream, offset, magic):
    stream.seek(offset)
    header = stream.read(16)
    if header[:4] != magic:
        raise ValueError(f'Expected {magic!r} at {offset:#x}')
    length = struct.unpack_from('<Q', header, 8)[0]
    if length > 128 * 1024 * 1024:
        raise ValueError('Unexpectedly large metadata packet')
    return utf(stream.read(length))


def cpk_index(path):
    with path.open('rb') as stream:
        header = packet(stream, 0, b'CPK ')[0]
        toc = packet(stream, header['TocOffset'], b'TOC ')
    base = min(header['TocOffset'], header['ContentOffset'])
    result = []
    for row in toc:
        name = str(row.get('DirName') or '').strip('/')
        name = (name + '/' if name else '') + row['FileName']
        offset, length = base + row['FileOffset'], row['FileSize']
        if offset < 0 or offset + length > path.stat().st_size:
            raise ValueError('CPK entry outside archive')
        result.append(dict(archive=path.name, path=name, offset=offset,
                           size=length, extracted_size=row['ExtractSize']))
    if len(result) != header['Files']:
        raise ValueError('CPK file count mismatch')
    return result


def decompress(data):
    if data[:8] != b'CRILAYLA':
        return data
    size, packed = struct.unpack_from('<II', data, 8)
    if len(data) < 16 + packed + 256:
        raise ValueError('Truncated CRILAYLA')
    at, bits, buffer = 15 + packed, 0, 0
    def read(n):
        nonlocal at, bits, buffer
        while bits < n:
            if at < 16:
                raise ValueError('CRILAYLA bitstream exhausted')
            buffer = (buffer << 8) | data[at]
            at -= 1
            bits += 8
        bits -= n
        result = (buffer >> bits) & ((1 << n) - 1)
        buffer &= (1 << bits) - 1
        return result
    out = bytearray()
    while len(out) < size:
        if not read(1):
            out.append(read(8))
            continue
        distance, length = read(13) + 3, 3
        for width in (2, 3, 5, 8):
            part = read(width)
            length += part
            if part != (1 << width) - 1:
                break
        else:
            while True:
                part = read(8)
                length += part
                if part != 255:
                    break
        if distance > len(out) or len(out) + length > size:
            raise ValueError('Invalid CRILAYLA backreference')
        for _ in range(length):
            out.append(out[-distance])
    return data[16 + packed:16 + packed + 256] + out[::-1]


def dat_entries(data):
    if data[:4] != b'DAT\0':
        raise ValueError('Expected DAT archive')
    count, offsets, extensions, names, sizes, hashes = struct.unpack_from('<6I', data, 4)
    name_width = struct.unpack_from('<I', data, names)[0]
    if count > 100000 or not 0 < name_width < 4096:
        raise ValueError('Invalid DAT header')
    result = []
    for i in range(count):
        start = names + 4 + i * name_width
        name = data[start:start + name_width].split(b'\0')[0].decode('ascii')
        offset = struct.unpack_from('<I', data, offsets + 4*i)[0]
        size = struct.unpack_from('<I', data, sizes + 4*i)[0]
        if offset + size > len(data):
            raise ValueError(f'DAT entry outside archive: {name}')
        result.append((name, data[offset:offset + size]))
    return result


def write_local(root, name, data):
    dest = (root / name).resolve()
    if not dest.is_relative_to(root.resolve()) or ':' in name or '\\' in name:
        raise ValueError(f'Unsafe archive path: {name}')
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists():
        if dest.read_bytes() != data:
            raise FileExistsError(f'Refusing to replace different content: {dest}')
    else:
        dest.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_data', type=Path)
    parser.add_argument('--output', type=Path, default=Path('local_assets'))
    parser.add_argument('--select', help='Regex for archive member paths; omit to index only')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    records, extracted = [], []
    for archive in sorted(args.game_data.glob('*.cpk')):
        rows = cpk_index(archive)
        records.extend(rows)
        print(f'{archive.name}: {len(rows)} entries', flush=True)
        if not args.select:
            continue
        with archive.open('rb') as stream:
            for row in rows:
                if not re.search(args.select, row['path'], re.I):
                    continue
                stream.seek(row['offset'])
                data = decompress(stream.read(row['size']))
                if len(data) != row['extracted_size']:
                    raise ValueError(f'Extracted size mismatch: {row}')
                relative = archive.stem + '/' + row['path']
                write_local(args.output, relative, data)
                record = dict(row, sha256=hashlib.sha256(data).hexdigest())
                if data[:4] == b'DAT\0':
                    entries = dat_entries(data)
                    record['members'] = [dict(name=n, size=len(b), sha256=hashlib.sha256(b).hexdigest()) for n,b in entries]
                    for name, blob in entries:
                        write_local(args.output, relative + '.unpacked/' + name, blob)
                extracted.append(record)
                print(f'  {relative}: {len(data):,} bytes', flush=True)
    (args.output / 'cpk-index.json').write_text(json.dumps(records, indent=2), encoding='utf-8')
    if args.select:
        (args.output / 'extraction-manifest.json').write_text(json.dumps(extracted, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
