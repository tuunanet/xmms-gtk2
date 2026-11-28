# Plan: playlist editor text follows the skin scale

Date: 2026-11-28
Status: Implemented on `feat/resolution-skin-scale`.

## Problem

After the N x skin scaling merged, a first run at DoubleSize renders
the playlist window, skin graphics, and selection bars doubled — but
the list text stays at the font's native pixel size (~13px rows inside
a doubled window, ~53 tiny rows where the design target shows ~29).
The user set `playlist_font_scale=200`, which helps, but that is a
manual patch for what should be the default: everything else in the
playlist follows the player's scale, so the text must too.

## Cause

`playlist_list_set_font(font, scale)` applied only the
`playlist_font_scale` percentage via `util_font_scale_name` (100 =
native pixels untouched); no code path multiplies by the skin scale,
and `playlistwin_set_scale` (Ctrl+D) did not reload the font at all.

## Decision

- `playlist_list_set_font` computes the effective scale as
  `playlist_font_scale * PLAYLIST_SCALE`, i.e. a percentage on top of
  the skin scale; 100 now means "match the scaled skin".
- `playlistwin_set_scale` reloads the list font before the forced
  redraw so Ctrl+D and autoscaled starts track the new scale.
- Call sites stay unchanged (`cfg.playlist_font_scale` keeps its
  meaning as the user-facing percentage). The combined scale passes
  through `util_font_scale_name`'s existing 25–400 clamp, so extreme
  combinations saturate at 4x bitmap scaling.

## Constraints honored

- Config format unchanged; `playlist_font_scale` keeps its percentage
  semantics and clamp, now relative to the skin scale.
- At 1x nothing changes (percent × 1 = percent; 100 = native).
- The font string itself is still never rewritten in `cfg`.

## Tests

- `tests/test-playlist-wheel.c` gained
  `/playlist-wheel/text-follows-skin-scale`: measures the rendered
  playlist row pitch from frame captures on a private Xvfb at 1x and
  at a doubled start (persisted `doublesize=TRUE` with autoscale off).
  Asserts the 1x pitch is a sane font row (~13px) and the doubled
  pitch is at least 1.6x the 1x pitch (measured 13px vs 28px with the
  fix; the unfixed build renders ~13px at 2x and fails).
