# Plan: N× integer skin scaling (generalize DoubleSize to 1×/2×/3×)

Date: 2026-11-24
Status: Implemented on `feat/resolution-skin-scale`.
Follow-up to `specs/2026-11-24-resolution-skin-scaling.md`: that feature capped
the skin scale at 2× because the renderer only had two sizes. This removes the
cap.

## Renderer architecture (from the codebase inventory)

Doubling is **window-level**, not widget-level:

- Widgets always draw at 1× into a window-sized backing pixmap
  (`mainwin_bg` / `equalizerwin_bg`, 275×116).
- A second **scaled backing pixmap** (`mainwin_bg_dblsize`, 550×232) is
  maintained: on force redraws the whole 1× background is pixel-replicated
  (`create_dblsize_image`, 2×2), incrementally per-widget rect at `<< 1`
  offsets.
- The X window is resized (275×116 ↔ 550×232 literals); the back pixmap is
  switched between the two.
- Window input handlers divide event coordinates by 2 so widgets keep operating
  in 1× space.
- Shape masks exist as 8 pre-built GdkBitmaps (1×/2× × shade × main/eq) in the
  `Skin` struct; `skin_create_transparent_mask` already multiplies region.txt
  points by `(1 + doublesize)` (arithmetic-ready).
- `vis`/`svis` draw **directly to the window** with hardcoded 2×2 replication
  and strides (vis: 304/152; svis: `DRAW_DS_PIXEL`, 76/152).
- The playlist never scales (always 1×) — unchanged.

## Design

**Scale state.** `Config.doublesize` becomes `gint`: the **extra scale factor**
(0 = 1×, 1 = 2×, 2 = 3×). This preserves every existing site verbatim:
`if (cfg.doublesize)` still means "scaled", `cfg.doublesize + 1` still is the
scale. The config key `doublesize` stays boolean on disk (TRUE = legacy 2×).

**Cap.** `XMMS_SKIN_MAX_SCALE` 2 → 3; `util_skin_scale_for_coefficient()`
already CLAMPs, so 8K-class resolutions (coefficient ≥ 2.5) and
`skin_scale=2.5`/`3.0` configs now select 3×.

**Scaled backing pixmap.** One per window, renamed `*_bg_scaled`, allocated at
`275·s × 116·s` for the active scale `s` (created lazily / re-created on scale
change via `*_ensure_scaled_bg()`; shaded mode reuses the top `14·s` rows).
`create_dblsize_image()` → `create_scaled_image(img, scale)` with generalized
n×n replication (per-bpp, same strategy).

**Input.** Event handlers divide by `cfg.doublesize + 1`. `mainwin_release()`
switches from the `menurow` LED flag to `cfg.doublesize` so there is one source
of truth (same value, always in sync).

**Masks.** `Skin` mask fields become arrays indexed by scale (1..MAX);
`skin_get_mask(mi, gint scale, gboolean shaded)` (CLAMPed);
`skin_create_transparent_mask(..., gint scale)` multiplies points by `scale`;
default rect masks built per scale. Built for all scales at skin load.

**Vis/svis.** Restructured to always compute a 1× 76×16 (vis) / 38×19 (svis)
indexed frame, then either draw it directly (scale 1) or expand n× via
`util_expand_indexed_frame()` and draw at `widget.x·s` with rowstride `width·s`.
`Vis.vs_doublesize` → `gint vs_scale`; `create_vis(..., gint scale)`;
`vis_set_doublesize()` → `vis_set_scale()`. svis keeps its scaled-mode VU
formula (`data·8/37` for scale > 1, `data·7/37` at 1×) and its DITHER_NONE for
scale > 1.

**Equalizer link.** `EQUALIZER_DOUBLESIZE` → `EQUALIZER_SCALE`
(`cfg.doublesize * cfg.eq_doublesize_linked`, value 0..2); mask calls pass
`EQUALIZER_SCALE + 1`. `equalizerwin_set_doublesize(bool)` →
`equalizerwin_set_scale(gint extra_scale)`.

**Runtime toggle (Ctrl+D / DoubleSize menu).** Stays boolean: off = 1×, on =
the session target scale (the startup-decided scale, or 2× if startup was 1×).
The menu checkbox and menurow LED mean "scaled" exactly as before. A legacy
persisted `doublesize=TRUE` still starts at 2× (never 3×).

## Explicitly unchanged

- `dock.c` (takes explicit pixel sizes), `resnap_dock_group` (`ms`
  arithmetic — the playlist's scaled size feeds it via
  `PLAYLIST_WIDTH`/`PLAYLIST_HEIGHT`).
- Widgets themselves (draw at 1×, receive pre-translated coordinates).
- Config key names and formats.
- Mask-after-resize ordering (XWayland workaround).

## Playlist scaling (added during review)

The first N× cut kept the playlist at 1× (its historical behavior), but
at 2×/3× the docked chain came up with mixed window sizes, which reads
as broken. The playlist now follows the player's scale:

- `PLAYLIST_SCALE`/`PLAYLIST_WIDTH`/`PLAYLIST_HEIGHT` macros multiply
  the 1× config dimensions; the config values themselves stay 1×.
- Same backing-pixmap model as main/EQ: a 1× working pixmap plus a
  scaled pixmap (`playlistwin_update_scaled_bg()`), maintained on
  interactive resize (each quantized 25/29-px 1× step), on skin change
  and on shade/focus force redraws.
- Drag-resize, popup hit regions and keyboard handling operate on
  event coordinates divided by the scale; WM-driven resizes are divided
  in the configure handler. Geometry hints and the shade height scale.
- `playlistwin_set_scale()` is called from `set_doublesize()`; the
  dock resnap uses the scaled playlist geometry.
- The embedded vis switches scale with the window (`vis_set_scale`).

## Known gap retained

`equalizerwin_release()` does not translate event coordinates (historical,
harmless today because release callbacks ignore coordinates).

## Post-review fix: duplicate analyzer draw

After the playlist was scaled, UAT showed **two visualizations animating
in the main panel**. Root cause (confirmed by instrumenting vis_draw and
stub experiments): the N x rewrite of `vis_draw()` kept the ORIGINAL
unconditional 1x `gdk_draw_indexed_image(... frame ...)` inside the
frame-computation block, so every call painted the analyzer at the 1x
device rect (24,43 76x16) *and* at the scaled rect — the 1x copy looked
like a second, small visualization animating above the menurow. The fix
removes the leftover 1x draw; the scaled path is the only painter.
Verified on a private Xvfb at 3x with playback: per-region frame diffs
show animation only in the scaled vis rect (891-3456 changing pixels)
and zero changes at the 1x rect (previously 99-372).

## Post-review fix: hold-to-popup skinned menus

UAT found the playlist's hold-to-popup skinned menus (file+, file-,
select-all, misc-options, load-list) tiny and misplaced at scaled
sizes, with no item highlight while holding. Root cause:
`playlist_popup()` was wholly 1x — the popup window was sized
`25 x num_items*18` device pixels, `playlist_popup_draw()` blitted 1x
skin rects straight to the screen, `playlist_popup_motion()` hit-tested
raw device coordinates, and `playlistwin_press()` anchored the popup by
mixing the dock's device-space window origin (`dock_get_widget_pos`)
with logical widget offsets. With the menu detached from the pressed
button, the pointer never crossed the grab window, so the selected-item
tracking ("hold down animation") never ran either.

The fix keeps the 1x contract and scales everything around it: the menu
is still composed at 1x, then replicated into the popup window through
`create_scaled_image()` (the same backing-pixmap model as the skinned
windows); window size, the -1px shift, and the motion hit-test all
follow the scale; the five anchors in `playlistwin_press()` now scale
their logical offsets (`xpos + 41 * scale`,
`ypos + (h - 4*18 - 11) * scale`, and the REGION_R variant
`(w - 46) * scale`). 1x geometry is pixel-identical to stock.

Verified on a private Xvfb (uat-home `skin_scale=3.0`): holding
misc-options pops `75x216` at the exact predicted anchor over the
pressed button, the highlight follows XTEST pointer motion across the
items, clears when the pointer leaves, and the window is destroyed on
release. The load-list (REGION_R) and select-all popups land at their
predicted anchors too. At 1x the popup is `25x72` at the stock
position, unchanged.

## Tests

- `tests/test-hidpi-scale.c`: mapping cap moves to 3 (3.0 → 3, 2.5 → 3,
  2.4 → 2); display combination at 7680×4320 → 3.
- `tests/test-hidpi-startup.c`: `skin_scale=3.0` (ambient) → 825×348; private
  Xvfb 7680×4320 with no DPI → 825×348; 4K/QHD expectations unchanged.
- `tests/test-dock-resnap.c`: new 3× autoscaled-start case with a matching
  playlist height (348) so the docked stack stays flush.
