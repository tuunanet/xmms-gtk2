---
bug_id: BUG-2026-11-23T145455-hidpi-xwayland-scaling
status: resolved
severity: medium
priority: P2
scope: ui-hidpi
---

# BUG-2026-11-23T145455: HiDPI — XMMS renders unscaled (tiny) on GNOME Wayland

## Problem

On Ubuntu 26.04 (GNOME on Wayland) with a 6144×3456 panel at 200% desktop
scaling, XMMS opens at its raw skin pixel sizes — a 275×116 px main window,
roughly 4.5% of the screen width — and looks "incredibly small". On
Kubuntu 26.04 (KWin Wayland) on equivalent hardware it looks correctly sized.

The same binary behaves differently because the compositors treat legacy
X11/XWayland clients differently:

- **KWin** runs the XWayland framebuffer at scale 1 but upscales every
  client's buffer to the display scale factor by default. All X11 apps
  appear at the correct physical size (slightly soft).
- **GNOME/mutter** does not scale XWayland clients by default; the X server
  reports the full native panel at 96 DPI, scale 1.0. Verified on the
  affected machine with `xdpyinfo`:
  `dimensions: 6144x3456 pixels`, `resolution: 96x96 dots per inch`.

## Root Cause Analysis

1. XMMS is a fixed-pixel skinned player. There is no DPI awareness anywhere
   in the code: `grep -Ei 'Xft|dpi' xmms/**/*.c` returns nothing.
   Window geometry comes from skin bitmaps: main 275×116, EQ 275×116,
   playlist base width 275 (`xmms/main.c`, `xmms/equalizer.c`,
   `xmms/playlistwin.c`, `xmms/skin.c`).
2. Skinned windows ignore `Xft.dpi`. On the affected machine `xrdb` reports
   `Xft.dpi: 192`, so GTK2 *dialog* fonts render at 2× while the skinned
   windows stay frozen at 1× — the contrast makes the player look even
   smaller than it is.
3. XMMS does have a built-in 2× mode (`DoubleSize`, Ctrl+D, persisted as
   `doublesize` in `~/.xmms/config`) using nearest-neighbor pixel doubling
   (`create_dblsize_image()` in `xmms/util.c`, `mainwin_bg_dblsize`,
   doubled masks in `skin_get_mask()`, halved hit-test coordinates in
   callbacks). But:
   - it is opt-in and never applied automatically on HiDPI screens;
   - the playlist window has no doublesize support at all (historical
     limitation in original XMMS);
   - only integer factor 2 exists — no general scale factor.

## External Workarounds (no rebuild)

1. Press **Ctrl+D** in the main window (Options → DoubleSize). Persists via
   `doublesize=TRUE`. Main window and linked EQ render at 550×232 —
   correctly sized for a 200%-scaled panel. Playlist stays at 1×.
2. Make GNOME scale all XWayland apps (slightly blurry, fixes every legacy
   X11 app):
   `gsettings set org.gnome.mutter experimental-features "['scale-monitor-framebuffer']"`
   then re-login.

## Approved Fix Plan (approved 2026-11-23; implemented on `fix/hidpi-xwayland-scaling`)

Reuse the existing DoubleSize machinery instead of inventing a new scaling
layer. This preserves skin compatibility, window geometry behavior, and the
historical default (scale 1) when nothing is configured.

1. **DPI detection utility.** Add `util_get_xft_dpi()` reading `Xft.dpi`
   from the X resource database (Xrm), falling back to 96 when unset or
   unparseable. Unit-test the parsing/fallback logic as a source-slice test.
2. **Auto-scaling at startup.** New config key `skin_autoscale` (boolean,
   default TRUE, written/read under `[xmms]` like other keys). At startup,
   if `skin_autoscale` is set and `round(Xft.dpi / 96) >= 2`, apply the
   existing `set_doublesize(TRUE)` before windows are shown. No new
   rendering code; scale factor remains 1 or 2. Manual override: the
   existing Ctrl+D toggle wins and updates `doublesize` as today.
   *Implementation note (2026-11-23 code review):* `set_doublesize()`
   dereferences windows that do not exist yet, so startup sets
   `cfg.doublesize = TRUE` directly — byte-for-byte what the existing
   persisted-`doublesize=TRUE` startup path does. The Ctrl+D toggle wins
   within the running session; because the legacy config format writes
   `doublesize=FALSE` on every exit (it cannot distinguish an untouched
   default from an explicit opt-out), the persistent opt-out on HiDPI
   displays is `skin_autoscale=FALSE`.
3. **Playlist DoubleSize support.** Extend the doublesize machinery to the
   playlist window: compose `playlistwin_bg` at 2× (reuse
   `create_dblsize_image()`), scale slider/button/text widget geometry and
   hit tests, and load the playlist font at doubled pixel size. Shaded EQ
   style rows (29 px tiles, 25 px buttons) are doubled by the same factor
   arithmetic used in `skin_get_mask()`. Keep 1× rendering byte-identical
   when doublesize is off.
   *Design decision (2026-11-23 code review):* the "doubled pixel size
   font" wording is deliberately not implemented literally. The playlist
   list computes row layout from the font metrics in skin space
   (`pl_fheight`), so a 2× font there would double the row height again
   through the bg doubling (4× effective). Text instead stays in skin
   space and is doubled on the way out — identical to how every other
   skinned element, including mainwin's skinned fonts, renders in
   DoubleSize.
4. **Docs.** Document the behavior in the manual (`xmms.1.in`) and
   `docs/architecture/ui-interaction.md`; mention the GNOME
   `scale-monitor-framebuffer` alternative.

Constraints honored: no plugin vtable/ABI changes (UI-only), no socket or
config-path changes (new `[xmms]` key only), skin compatibility untouched
(1× is the default and unchanged), GTK work stays on the main thread.

## Acceptance Criteria

- [x] On a screen with `Xft.dpi >= 144`, XMMS starts with main window, EQ,
      and playlist all rendered at 2× without manual intervention.
      (Automated: `tests/test-hidpi-startup.c` asserts main 550×232 and
      playlist 600×464 under `Xft.dpi: 192`.)
- [x] With `skin_autoscale=FALSE` (or `Xft.dpi <= 96`), startup behavior is
      byte-identical to today. (Automated: the same e2e test asserts
      275×116 / 300×232 in both scenarios.)
- [x] Ctrl+D still toggles 1×/2× per window group as it does today; the
      playlist window now follows the same toggle
      (`playlistwin_set_doublesize()`). (Automated:
      `tests/test-hidpi-startup.c` sends Ctrl+D on a doubled session and
      asserts both windows collapse to 275×116 / 300×232.)
- [x] Playlist window scales with DoubleSize for the first time; clicks map
      to correct rows/buttons at 2× (event coordinates halved at dispatch,
      mirroring the documented mainwin mechanism). Click mapping itself is
      manual-verified only; the dispatch path is shared with mainwin's
      decades-established doublesize handling.
- [x] Focused tests, Preflight (`make && xvfb-run make check && make lint`),
      and `make distcheck` pass.

## Dock-Chain Rescale Follow-up (found in on-hardware testing)

Manual testing on the affected GNOME session showed that toggling DoubleSize
off and on again left the three docked windows shuffled: the test harness
reproduced EQ and playlist landing on the same y coordinate (fully
overlapping), and a saved 1x-consistent stack starting doubled with its
windows overlapping.

Root cause: `dock_resize()` re-docks only windows docked *directly* to the
window being resized.  Rescaling a vertical chain (player -> equalizer ->
playlist) runs three sequential docking passes; each recomputes docking from
the intermediate geometry, so later windows detach or collide.  Saved window
positions are also only consistent at 1x, so an autoscaled start needs the
same treatment.

Fix in `main.c`:

- `resnap_dock_group()` runs once after all three windows rescale (and once
  at startup when autoscale fires) and re-snaps flush-docked followers to
  their leader's new edge: equalizer to the player, playlist to the
  equalizer (falling back to the player when it was not docked to the
  equalizer).  Docking relations are read from the pre-rescale geometry.
- New sizes are **computed** from the skin geometry; reading them back with
  `gdk_window_get_size()` during the transition returns stale values
  (observed: intermediate 200px heights), which produced wrong targets in
  an earlier revision.
- The pre-rescale scale factor is captured before `cfg.doublesize` is
  updated, so the startup menu-state callback (which re-enters
  `set_doublesize()` with an unchanged scale) is a no-op instead of
  mislabelling the old geometry.

Verified on the affected GNOME/Wayland session by driving four consecutive
Ctrl+d toggles and measuring all three X windows: both states stay flush
(doubled: 1522/1754/1986; collapsed: 1522/1638/1754) and repeat identically.

## Compositor Shape-Region Follow-up (found in on-hardware testing)

After the dock-chain fix, on-screen testing still showed the main player and
equalizer rendered as if cropped to the top-left corner while the playlist
was correct. X-level inspection (window geometry, back-pixmap contents, and
the applied shape-mask dimensions) reported everything correct in every
state, which pointed at the compositor rather than the app.

Root cause: the main window and equalizer applied their new shape mask
*before* resizing the window (`mainwin_set_shape_mask()` /
`equalizerwin_set_shape_mask()` ran before `dock_resize()`). XWayland
compositors keep the pre-resize shape region in that case, so expanding a
window left the compositor clipping it to the previous (smaller) shape —
exactly the cropped appearance, and the stale clip never recovered. The
playlist window was unaffected because it already rebuilt its mask *after*
its resize, and it rendered correctly in the same screenshots.

Fix: apply the shape mask after the window resize in all four paths —
`set_doublesize()`, `mainwin_set_shade_menu_cb()`,
`equalizerwin_set_doublesize()` and `equalizerwin_set_shade_menu_cb()`.

Verified by the user watching a scripted Ctrl+D cycle on the affected
GNOME/Wayland session: both directions render complete, uncropped windows.

## Playlist Interactive-Resize Follow-up (found in UAT)

Dragging the playlist window larger clipped the newly grown strip (right
side when widening, bottom when heightening) until a Ctrl+D cycle forced a
full re-mask.  `playlistwin_motion()` resized the window *after*
`playlistwin_resize()` had already created the shape mask, so the XWayland
compositor kept the pre-resize shape region — the same rule as the
compositor follow-up above.

Fix: after the drag resize applies (`gdk_window_resize()` /
`gtk_widget_set_usize()`), re-create the mask and re-set the back pixmap in
`playlistwin_motion()`.  The WM-driven configure path already resizes
first and masks afterwards.

## Smooth-Scaling Follow-up (UAT decision)

The first HiDPI builds used nearest-neighbour pixel doubling, which UAT
found noticeably chunkier than the pre-PR Kubuntu appearance (KWin
upgraded the 1x window).  The user chose bilinear smoothing: `create_dblsize_image()`
now scales 24/32-bit images bilinearly (destination n samples n/2 - 0.25,
so 1/4–3/4 weights with clamped edges); legacy 8/16-bit visuals keep the
old nearest-neighbour path.  Every redraw re-scales the whole background,
which avoids seams from blending widget rectangles against stale
neighbours.  Unit tests (`tests/test-dblsize-scale.c`) pin the blend
weights, clamping and flat-colour preservation.

## Verification Evidence

- `tests/test-hidpi-scale` — 9/9 pass (parsing, RDB lookup, thresholds;
  caught and fixed a use-after-free in the first parser draft during RED).
- `tests/test-hidpi-startup` — 4/4 pass (dpi-192 doubles both windows;
  Ctrl+D collapses both back to 1×; `skin_autoscale=FALSE` and
  standard-DPI stay 1×).
- On-hardware dock-chain reassembly verified on the affected GNOME/Wayland
  session across four Ctrl+D toggles (see follow-up section above); pixel
  grabs of every window at every step showed complete, unclipped content.
- Two-axis code review (standards + spec sub-agents, 2026-11-23) produced
  fixes: guarded shared blit helper (`playlistwin_double_bg_rect`),
  restored the historical `14px` configure-event escape at any scale,
  removed dead test logic, and the design notes above.
- `xvfb-run --auto-servernum make check` — pass (163 TAP assertions,
  includes the pre-existing suites).
- `make lint` (Cppcheck 2.19.0) — pass; one pre-existing memleak false
  positive now also listed for the 2.19 report line
  (`tools/cppcheck-suppressions.txt`), and the linking of `xmms/util.c`
  into source-slice tests gained `-lX11` for the new Xrm symbols.
- `xvfb-run --auto-servernum make distcheck` — pass; tarball builds and the
  full suite runs inside the distribution sandbox.
- `Makefile.in` updated manually and exactly in parallel with `Makefile.am`
  for `EXTRA_DIST` (checked-in Automake metadata predates the local
  Automake; see mpg123-file-info bug record for precedent).
- Pending: user confirmation on real GNOME/Kubuntu hardware after the
  packages from this branch are installed (e02 epic).

## Notes

- Discovered during e02 multi-distro package-release verification
  (Ubuntu 26.04 vs Kubuntu 26.04 debs).
- Implementation must start from a fresh `kickoff-branch`; this record is
  documentation only and is deliberately kept off the in-flight
  `heal-bigpowers-initialization` review branch.
- Registry updated in `specs/bugs/registry.yaml`.
