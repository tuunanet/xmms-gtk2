# Plan: native default playlist font (6x12) for robust rendering

Date: 2026-11-27
Status: Implemented on `feat/resolution-skin-scale`.
Follow-up to `2026-11-27-playlist-font-scale.md`.

## Problem

The playlist's default font,
`-misc-fixed-bold-r-semicondensed--13-120-75-75-c-60-iso8859-1`
(6x13 bold), is the **only** bold semicondensed size shipped by
xfonts-base. `playlist_font_scale` values below 100 make the X server
bitmap-scale that 13px source, and the 13→11px downscale renders
thin, broken strokes (stems dropped, gaps inside glyphs) — playlist
text looked bad at any non-default scale.

## Measured evidence (Xvfb + xfonts-base)

Native `misc` fixed inventory below 13px (via `xlsfonts`): 6, 7, 8,
9, 10px medium-normal and **12px medium-semicondensed** (6x12). No
bold exists below 13px, so "smaller and bold" is impossible without
server scaling.

Glyph renders of `14. Pierrcified - Hertz - Electronic Fridge 60:42`:

| font | source → rendered | quality |
| --- | --- | --- |
| 6x13 bold (old default) | native 13px | crisp, tall, heavy |
| old default @90% | 13 → 11 | **broken glyphs** (the complaint) |
| **6x12 medium semicondensed** | native 12px | crisp, clean, same 6px cell |
| 6x12 @90% | 12 → 11 | clean (near-crop, legible) |
| 6x10 medium | native 10px | clean but cramped |

## Decision

- **New default playlist font:**
  `-misc-fixed-medium-r-semicondensed--12-110-75-75-c-60-iso8859-1`
  (native 6x12, medium). One pixel shorter than before, same 6px
  character cell (no reflow), crisp at the default scale, and its
  scaled variants degrade gracefully because the 12px source is
  closer to every requested size.
- **Default `playlist_font_scale` is now 100.** With a natively
  sized default font, the robust default is to draw exactly the
  bitmap pixels; server bitmap-scaling becomes opt-in (`90` remains
  one clean visible step smaller).
- **Migration:** the 13px bold semicondensed default joined the
  historical-font migration (`XMMS_INTERIM_SEMICONDENSED_PLAYLIST_FONT`
  in `util_font_resolve_default()` and the `read_config()` playlist
  list), so existing configs carrying it land on the new default,
  consistent with how legacy/interim defaults were migrated before.
  Configs migrated this way that still carry `playlist_font_scale=90`
  (which shipped as the default with the interim font, and renders
  12px source bitmaps at a soft 10px via the X server) are moved to
  `100` as well, so migrated sessions land on native-size rendering.
  Deliberate choices — any custom font string or any other scale
  value — are untouched. The main-window font is unchanged.

## Rejected alternatives

- **Keep 13px bold + server scaling:** measured broken glyphs; the
  exact complaint.
- **A bold smaller font:** none exists in xfonts-base; depending on
  optional font packages (xfonts-75dpi/100dpi Adobe fonts) would
  reintroduce the charset-warning problems the fixed defaults fixed.
- **6x10 medium default:** crisp but visibly cramped at 10px; kept as
  a user choice via a custom font or low scale values.
- **Xft/Pango rendering for the playlist:** a new text pipeline over
  the skinned core-drawing path; too invasive for now and departs
  from the classic bitmap look.

## Tests

`tests/test-font-load.c`:

- `/font/preferred-playlist-default` — new default asserts
  `-medium-`/`-semicondensed-`, native metrics (ascent+descent == 12,
  6px charcell), i.e. the X server returns the unscaled bitmap.
- `/font/interim-semicondensed-playlist-migrates` — the 13px bold
  name loads with identical metrics to the new default (migration).
- Scale expectations updated for the 12px/110-decipoint base
  (90% → `--11-99-…`); clamps re-checked (`25` → `--3-28-…`,
  `400` → `--48-440-…`); loads-smaller test uses an explicit 90.
