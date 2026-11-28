#!/usr/bin/env python3
"""Regenerate every icon derived from xmms/app-icon.png.

Usage: tools/generate-app-icon.py [--check] [SOURCE_ROOT]

The build never runs this script. It is a maintainer tool: after editing
xmms/app-icon.png, run it and commit the rewritten files together with the
master artwork. --check regenerates in memory, compares with the source tree,
and exits non-zero when a derived icon is missing or stale. Exact bytes depend
on the Pillow version; regenerate when a different one reports drift.

Requires Python 3 and Pillow (Debian/Ubuntu: python3-pil).
"""

import argparse
import io
import sys
from pathlib import Path

from PIL import Image

MASTER = "xmms/app-icon.png"

# Renditions mainwin_set_icon() publishes through _NET_WM_ICON. The 16x16 one
# doubles as the legacy WM_HINTS pixmap.
WINDOW_ICONS = (
    ("xmms/xmms_mini.xpm", "xmms_mini_xpm", 16),
    ("xmms/xmms_icon_32.xpm", "xmms_icon_32_xpm", 32),
    ("xmms/xmms_icon_48.xpm", "xmms_icon_48_xpm", 48),
    ("xmms/xmms_icon_64.xpm", "xmms_icon_64_xpm", 64),
    ("xmms/xmms_icon_128.xpm", "xmms_icon_128_xpm", 128),
)

# About dialog artwork, and the wmxmms launch tile (centred on its 64x64 dock app).
OTHER_ICONS = (
    ("xmms/xmms_logo.xpm", "xmms_logo", 128),
    ("wmxmms/wmxmms.xpm", "wmxmms", 48),
)

# Idle backgrounds of the visualization plugins: the icon centred on a black
# canvas. Each canvas matches the WIDTH and HEIGHT of the plugin window.
VIS_BACKGROUNDS = (
    ("Visualization/sanalyzer/xmms_logo.xpm", "sanalyzer_xmms_logo_xpm", 250, 100),
    ("Visualization/blur_scope/xmms_logo.xpm", "bscope_xmms_logo_xpm", 256, 128),
)

# Themed launcher icons for packaging/xmms.desktop (Icon=xmms), installed by
# packaging/debian/rules.
HICOLOR_SIZES = (16, 22, 24, 32, 36, 48, 64, 72, 96, 128, 192, 256, 512)

MAX_COLORS = 256
# XPM pixel codes: printable ASCII without the string delimiters.
CODES = "".join(chr(c) for c in range(33, 127) if chr(c) not in '"\\')


def render(master, size):
    """The artwork resampled to size x size."""
    return master.resize((size, size), Image.Resampling.LANCZOS)


def palettize(image):
    """Reduce to the palette XPM and themed icons use."""
    return image.quantize(colors=MAX_COLORS, method=Image.Quantize.MAXCOVERAGE,
                          dither=Image.Dither.NONE)


def xpm(image, symbol):
    """Serialize a palettized image as the C array the sources #include."""
    width, height = image.size
    palette = image.getpalette()
    pixels = image.tobytes()
    # Only the palette entries the image uses are written out, so map each
    # used entry to its two-character code explicitly.
    codes = [None] * (len(palette) // 3)
    colors = []
    for number, entry in enumerate(sorted(set(pixels))):
        codes[entry] = CODES[number // len(CODES)] + CODES[number % len(CODES)]
        red, green, blue = palette[entry * 3:entry * 3 + 3]
        colors.append('"%s c #%02X%02X%02X"' % (codes[entry], red, green, blue))

    strings = ['"%d %d %d 2"' % (width, height, len(colors))] + colors
    strings += ['"%s"' % "".join(codes[pixels[y * width + x]]
                                 for x in range(width))
                for y in range(height)]
    return "/* XPM */\nstatic char *%s[] = {\n%s\n};\n" % (
        symbol, ",\n".join(strings))


def png(image):
    buffer = io.BytesIO()
    image.save(buffer, format="PNG", optimize=True)
    return buffer.getvalue()


def derive(master):
    """Yield (relative path, bytes) of every icon generated from the master."""
    icons = WINDOW_ICONS + OTHER_ICONS
    for path, symbol, size in icons:
        yield path, xpm(palettize(render(master, size)), symbol).encode("ascii")
    for path, symbol, width, height in VIS_BACKGROUNDS:
        icon = render(master, min(width, height))
        canvas = Image.new("RGB", (width, height), (0, 0, 0))
        canvas.paste(icon, ((width - icon.width) // 2, (height - icon.height) // 2))
        yield path, xpm(palettize(canvas), symbol).encode("ascii")
    for size in HICOLOR_SIZES:
        path = "packaging/icons/hicolor/%dx%d/apps/xmms.png" % (size, size)
        yield path, png(palettize(render(master, size)))


def main(argv):
    default_root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true",
                        help="report stale or missing icons instead of writing")
    parser.add_argument("source_root", nargs="?", type=Path, default=default_root)
    args = parser.parse_args(argv)

    master_path = args.source_root / MASTER
    if not master_path.is_file():
        parser.error("master artwork not found: %s" % master_path)
    master = Image.open(master_path).convert("RGB")
    stale = []
    for path, content in derive(master):
        target = args.source_root / path
        if args.check:
            if not target.is_file() or target.read_bytes() != content:
                stale.append(path)
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(content)
            print("wrote %s" % path)

    for path in stale:
        print("stale or missing: %s" % path, file=sys.stderr)
    return 1 if stale else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
