#!/usr/bin/env python3
"""Inventory captured fields and make a contact sheet for the ending investigation."""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('directory', type=Path)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
rows = []
for path in sorted(a.directory.glob('shot-*.png'), key=lambda p: int(p.stem.split('-')[-1])):
    with Image.open(path) as source:
        im = source.convert('RGB')
        r, g, b = im.split()
        threshold = [0 if i <= 40 else 255 for i in range(256)]
        mask = ImageChops.darker(ImageChops.subtract(g, r).point(threshold),
                                 ImageChops.subtract(g, b).point(threshold))
        mask = ImageChops.darker(mask, g.point([0 if i < 80 else 255 for i in range(256)]))
        area = im.width * im.height
        dominant = max(im.getcolors(area))
        rows.append(dict(frame=int(path.stem.split('-')[-1]), path=path.as_posix(),
                         width=im.width, height=im.height,
                         png_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                         rgb_sha256=hashlib.sha256(im.tobytes()).hexdigest(),
                         green_fraction=round(mask.histogram()[255] / area, 6),
                         dominant_rgb=dominant[1], dominant_fraction=round(dominant[0] / area, 6)))
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.with_suffix('.json').write_text(json.dumps(rows, indent=2) + '\n')
if rows:
    selected = rows[::max(1, len(rows) // 12)][:12]
    selected += sorted(rows, key=lambda r: r['green_fraction'], reverse=True)[:12]
    canvas = Image.new('RGB', (352 * 4, 144 * ((len(selected) + 3) // 4)), '#15191c')
    draw = ImageDraw.Draw(canvas)
    for index, row in enumerate(selected):
        x, y = index % 4 * 352, index // 4 * 144
        with Image.open(row['path']) as im:
            canvas.paste(im.convert('RGB').resize((352, 120)), (x, y + 24))
        draw.text((x + 4, y + 4), f"{row['frame']}  green {row['green_fraction']:.1%}", fill='white')
    canvas.save(a.output.with_suffix('.png'))
    print(json.dumps(dict(frames=len(rows), greenest=sorted(rows, key=lambda r: r['green_fraction'], reverse=True)[:3]), indent=2))
else:
    print('No captured PNGs yet.')
