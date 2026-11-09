#!/usr/bin/env python3
"""Freeze an existing port --watch 06000708:0600072F log as a scenario.

Recording may use STV_AUTOPLAY. Replays never do: both machines get this same
fixed sequence. The P41 line is after raw-input publication in each VBlank.
"""
import argparse
import hashlib
from pathlib import Path
import re


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('log')
    ap.add_argument('output')
    ap.add_argument('--frames', type=int, default=10000)
    args = ap.parse_args()
    raw = [0,0,0]
    rows = []
    store = re.compile(r'store16\s+(06000708|06000710|06000728) = ([0-9A-F]+)')
    frame = re.compile(r'P41 VB=(\d+) ')
    with Path(args.log).open() as stream:
        for line in stream:
            m = store.search(line)
            if m:
                raw[{'06000708':0,'06000710':1,'06000728':2}[m[1]]] = int(m[2],16)
            m = frame.search(line)
            if m and int(m[1]) <= args.frames:
                if int(m[1]) != len(rows)+1:
                    raise ValueError('incomplete input recording')
                rows.append(tuple(raw))
    if len(rows) != args.frames:
        raise ValueError('input recording ended early')
    lines = ['# v1.00; fixed physical input recorded from STV_AUTOPLAY=100.',
             f'# Source: {args.log}', f'# Source SHA256: {hashlib.sha256(Path(args.log).read_bytes()).hexdigest()}',
             '# Third combat area is director scene 9 (Scene 3+); continues are panel events.',
             f'frames {args.frames}', 'expect paid', 'expect scene=9']
    start = 1
    for end in range(1,len(rows)+1):
        if end == len(rows) or rows[end] != rows[start-1]:
            row = rows[start-1]
            if any(row):
                lines.append(f'{start} {end} {row[0]:x} {row[1]:x} {row[2]:x}')
            start = end+1
    Path(args.output).write_text('\n'.join(lines)+'\n')

if __name__ == '__main__':
    main()
