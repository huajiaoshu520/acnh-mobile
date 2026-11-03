#!/usr/bin/env python3
"""Bundle the upstream companion manifest and its authored MSBT flavor strings.

These files are companion source data, not extracted game assets. Native game
names, pictures, fonts and item labels continue to come from the user's RomFS.
"""
import json
from pathlib import Path
import struct
import sys


def msbt(data):
    if len(data) < 32 or data[:8] != b'MsgStdBn':
        raise ValueError('invalid companion MSBT header')
    endian = '<' if data[8:10] == b'\xff\xfe' else '>'
    def u32(offset):
        return struct.unpack_from(endian + 'I', data, offset)[0]
    labels, texts, off = {}, [], 32
    sections = struct.unpack_from(endian + 'H', data, 14)[0]
    for _ in range(sections):
        tag, size = data[off:off+4], u32(off+4)
        body, end = off+16, off+16+size
        if end > len(data):
            raise ValueError('truncated companion MSBT section')
        if tag == b'LBL1':
            buckets = u32(body)
            if buckets > (size-4)//8:
                raise ValueError('invalid companion MSBT label table')
            for i in range(buckets):
                count, at = u32(body+4+i*8), body+u32(body+8+i*8)
                for _ in range(count):
                    if at >= end:
                        raise ValueError('truncated companion MSBT label')
                    length = data[at]
                    if at+length+5 > end:
                        raise ValueError('truncated companion MSBT label')
                    labels[u32(at+length+1)] = data[at+1:at+length+1].decode('utf-8')
                    at += length+5
        if tag == b'TXT2':
            count = u32(body)
            if count > (size-4)//4:
                raise ValueError('invalid companion MSBT text table')
            for i in range(count):
                start = body+u32(body+4+i*4)
                stop = body+u32(body+8+i*4) if i+1 < count else end
                if not body+4+count*4 <= start <= stop <= end:
                    raise ValueError('invalid companion MSBT text range')
                if data[12] != 0:
                    raise ValueError('companion flavor must be plain UTF-8')
                text = data[start:stop].split(b'\0', 1)[0]
                if b'\x0e' in text or b'\x0f' in text:
                    raise ValueError('unexpected companion MSBT format tags')
                texts.append(text.decode('utf-8'))
        off = (end+15) & ~15
    return {label: texts[index] for index, label in labels.items() if index < len(texts)}


def generate(source):
    root = json.loads((source/'manifest.json').read_text())
    root['apple_flavor'] = {p.stem: json.loads(p.read_text()) for p in sorted((source/'flavor').glob('*.json')) if '{' not in p.name}
    return root


if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit('usage: gen_companion.py VENDOR_ACNH OUTPUT_JSON')
    Path(sys.argv[2]).write_text(json.dumps(generate(Path(sys.argv[1])), ensure_ascii=False, separators=(',', ':'))+'\n')
