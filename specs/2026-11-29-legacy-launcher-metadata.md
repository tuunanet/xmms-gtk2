# Plan: retire the legacy launcher metadata

Date: 2026-11-29
Status: Implemented on `feat/app-icon`.

## Problem

`xmms/xmms.desktop` and `xmms/xmms.wmconfig` ship in every source tarball but
serve no system this fork targets. Both still point at the previous icon by
file name (`Icon=xmms_mini.xpm` and `xmms icon "mini/xmms_mini.xpm"`).

## Findings

- Neither file is installed. They are listed in `xmms/Makefile.am`
  `EXTRA_DIST` only, and the Debian package installs `packaging/xmms.desktop`
  instead.
- `xmms/xmms.desktop` fails `desktop-file-validate` with 12 errors. It is not
  UTF-8; its translated comments use the invalid key `comment[xx]`, so they
  were never read; `Icon` carries a file extension; and `Encoding`,
  `Terminal=0`, and `application/x-ogg` are obsolete.
- `xmms/xmms.wmconfig` is input for Red Hat's `wmconfig` menu generator, which
  read `/etc/X11/wmconfig`. No current distribution ships it.
- No source, test, tool, workflow, or document depends on either file.
- The fork dropped `xmms.spec` the same way at import, and
  `tests/test-package-recipes.sh` already asserts that legacy packaging
  metadata is not shipped.

## Decision

- Remove both files and their `EXTRA_DIST` entries, mirrored by hand in
  `xmms/Makefile.in` as `CONTRIBUTING.md` requires.
- Keep `packaging/xmms.desktop` as the single desktop entry. GNOME and Plasma
  both consume it, so nothing desktop-specific is needed.
- Declare `StartupWMClass=xmms` there. Every XMMS window sets the WM class
  `xmms` (`gtk_window_set_wmclass` for the player, playlist, and equalizer),
  so the launcher association becomes explicit instead of depending on each
  shell's name heuristics, and all three windows group under the launcher.
- Leave the `#if 0` block in `xmms/main.c` that held a translated comment for
  the old entry. It is inert.

## Constraints honored

- No ABI, socket framing, configuration path, or skin change. The files are
  data that nothing reads.
- Window Maker integration is unaffected: `wmxmms` is the dock app, and
  `wmconfig` was never part of it.

## Tests

- `tests/test-package-recipes.sh` asserts that the two files are absent and
  not listed in `xmms/Makefile.am`, that `packaging/xmms.desktop` declares
  `StartupWMClass=xmms`, and that the player window still sets the matching
  WM class.
- `desktop-file-validate packaging/xmms.desktop` passes.
- Not exercised on a live Plasma session; the KDE behaviour follows the
  freedesktop.org desktop entry specification.
