# Plan: restore mouse wheel support via GDK scroll events

Date: 2026-11-28
Status: Implemented on `feat/resolution-skin-scale`.

## Problem

The playlist editor does not scroll with the mouse wheel; only the
arrow keys and dragging the skinned scrollbar work. The same silence
covers the main window (wheel volume/seek) and the equalizer sliders.

## Cause

The GTK1-era code handles wheel motion as `event->button == 4/5` (and
6/7 for horizontal) inside `button_press` handlers — three sites:

- `playlistwin_press` — scrolls the list three entries per notch
- `mainwin_press` — vertical wheel adjusts volume by `mouse_change`,
  horizontal wheel seeks by `mouse_change` seconds
- `eqslider_button_press_cb` — steps the slider under the pointer two
  pixels per notch

GTK1's GDK delivered X wheel buttons as button-press events, so those
handlers worked. The GTK2 port kept the code: GDK2 translates X
buttons 4-7 into `GDK_SCROLL` events instead, and `button_press` never
sees them. No `scroll-event` handler exists anywhere, so the wheel has
been dead since the port.

## Decision

Restore the three paths with window-level `scroll-event` handlers that
map `GdkEventScroll.direction` onto the same operations as the dead
GTK1 code, leaving the historical button 4/5 branches untouched:

- `playlistwin_scroll_event` — `UP`/`DOWN` call `playlistwin_scroll`
  (±3 rows); ignored in shaded mode. The list and slider widgets are
  redrawn by `playlistwin_update_list`, same as PageUp/PageDown.
- `mainwin_scroll_event` — vertical adjusts volume, horizontal seeks,
  identical deltas and clamps to the GTK1 branches.
- `equalizerwin_scroll_event` — divides the event coordinates by the
  equalizer scale like `equalizerwin_press`, hit-tests the preamp and
  ten band sliders (`inside_widget` respects visibility, so shaded
  mode and hidden sliders cannot be hit), and calls the new
  `eqslider_scroll` helper, which steps `es_position` by two pixels
  with the same clamps and center snap as the rest of the widget.

No event-mask changes are required: each window already selects
`GDK_BUTTON_PRESS_MASK`, which is what makes the X server deliver the
wheel button presses that GDK converts into scroll events.

## Constraints honored

- Config keys, plugin ABI, socket framing, and skin formats untouched.
- The dead GTK1 branches stay; the new handlers are purely additive
  and return FALSE for directions they do not handle, leaving
  propagation semantics unchanged.

## Tests

- New `tests/test-playlist-wheel.c`: spawns the real binary on a
  private Xvfb with a 40-entry `~/.xmms/xmms.m3u`, injects Button5
  (wheel down) and Button4 (wheel up) through XTEST over the playlist,
  and asserts the list area changes on scroll and returns
  pixel-identical to the baseline after scrolling back (compared via
  `XGetPixel` below the title strip, because the title strip repaints
  continuously even without input).
- `tests/Makefile` gained the `test-playlist-wheel` target (links
  X11 + Xtst like `test-dock-resnap`).
- Two cppcheck suppression line anchors moved (`eq_slider.c` 70 → 86,
  `equalizer.c` 1217 → 1262) because the insertions shifted the
  flagged legacy code; the diagnostics themselves are unchanged.
