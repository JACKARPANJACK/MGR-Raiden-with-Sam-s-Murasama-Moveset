"""Read local BXM sequence tracks for native Sam move verification."""
import struct
from pathlib import Path
import xml.etree.ElementTree as ET

def parse(blob):
    nodes, pairs = struct.unpack_from('>HH', blob, 8)
    pair_offset = 16 + nodes * 8
    text_offset = pair_offset + pairs * 4
    def string(index):
        if index == 65535:
            return ''
        start = text_offset + index
        return blob[start:blob.index(0, start)].decode('utf-8')
    def pair(index):
        return tuple(string(x) for x in struct.unpack_from('>HH', blob, pair_offset + index * 4))
    def node(index):
        count, first, attrs, data = struct.unpack_from('>4H', blob, 16 + index * 8)
        name, value = pair(data)
        el = ET.Element(name, dict(pair(data + a + 1) for a in range(attrs)))
        el.text = value
        for child in range(first, first + count):
            el.append(node(child))
        return el
    return node(0)

if __name__ == '__main__':
    import sys
    for name in sys.argv[1:]:
        p = Path(name)
        print(p.name)
        print(ET.tostring(parse(p.read_bytes()), encoding='unicode'))
