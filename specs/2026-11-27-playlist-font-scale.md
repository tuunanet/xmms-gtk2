# Plan: playlist editor font scale config option

Date: 2026-11-27
Status: Implemented on `feat/resolution-skin-scale`. The default
playlist font and default scale were superseded the same day by
`2026-11-27-playlist-font-native-default.md` (default scale is now 100
with a native 12px default font); the scaler mechanics and clamps below
still apply as documented.

## Problem

The playlist editor renders text with a single fixed font,
`XMMS_DEFAULT_PLAYLIST_FONT` (`-misc-fixed-bold-r-semicondensed--13-120-75-75-c-60-iso8859-1`,
13px tall). Users cannot make the text smaller to fit more rows without
hand-editing the font string, and there is no way to nudge the size at
all: bitmap fonts come only in discrete sizes.

## Decision

New config key `playlist_font_scale` in the `[xmms]` section of
`~/.xmms/config`: an integer **percentage** applied to the configured
playlist font's XLFD pixel (field 7) and point (field 8, decipoints)
sizes at load time.

- Default **90** — see the sizing evidence below for why one step
  smaller is 90% and not 95%.
- Clamped to **[25, 400]** on read (`main.c`) and defensively in the
  scaler.
- `100` (and any non-XLFD font name) loads the font unchanged; the
  existing `playlist_font` semantics are untouched.
- The scale is applied in `playlist_list_set_font(font, scale)` via
  `util_font_scale_name()`, so both load sites (playlist window creation
  and Preferences apply) honor it while `cfg.playlist_font` stays
  pristine (the user's chosen font string is never rewritten).

### Sizing evidence (Xvfb + xfonts-base)

Requesting a scaled XLFD makes the X server bitmap-scale the 13px
`fixed` semicondensed source. Measured ascent+descent of what
`XLoadQueryFont` actually returns:

| pattern pixel/point | result pixel | ascent+descent |
| --- | --- | --- |
| 13/120 (current default) | 13 (native) | 13 |
| 12/114 (~95%) | 12 | 13 — visually identical glyphs |
| 12/108 (90%) | 11 (scaled) | 10 — one visible step smaller, clean glyphs |

Two consequences drove the default: the X server derives the bitmap
size from the **point size and resolution**, so the scaler rewrites
both fields together; and 95% is a no-op for this font (the 13→12
bitmap downscale renders identical pixels), so 90% is the first scale
that actually shrinks the text. It yields 11px rows (ascent 9 +
descent 1) against the current 14px row height (`playlist_list_draw`
adds 1px leading), i.e. ~27% more visible rows.

## Constraints honored

- `playlist_font` (the font string itself) keeps its exact config-file
  semantics; at scale 100 nothing changes byte-for-byte, preserving the
  historical compatibility guarantees around the classic fonts.
- Alias fonts (`fixed`) and wildcard patterns (the interim defaults)
  cannot express a scaled size; they load unscaled, as before.

## Rejected alternatives

- **Absolute pixel size option** (`playlist_font_size`): meaningless
  across arbitrary user-selected fonts whose native sizes differ; the
  scale is font-relative by construction.
- **Scaling the main-window title font too**: the request is
  playlist-specific; `mainwin_font` feeds the skinned textbox with its
  own sizing and was left alone.
- **Preferences UI control**: rejected for now (scope decision); the
  config-file key is the requested interface. `prefswin_apply_changes`
  already flows through `playlist_list_set_font`, so adding a control
  later is a UI-only change.

## Tests

`tests/test-font-load.c` (all under the existing harness):

- `/font/scale-default-playlist-font` — 90% rewrites the default font
  to `--12-108-…` exactly.
- `/font/scale-identity-is-null`, `/font/scale-alias-is-null`,
  `/font/scale-wildcard-is-null` — unscaled passthrough cases.
- `/font/scale-keeps-dashed-encoding` — `iso10646-1`-style encodings
  (XLFD field count > 14) survive intact.
- `/font/scale-is-clamped` — 5 → 25, 100000 → 400.
- `/font/scaled-playlist-font-loads-smaller` — integration: the scaled
  default actually loads smaller (ascent+descent strictly less than the
  unscaled font), guarding against silent closest-match regressions.
