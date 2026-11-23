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
3. **Playlist DoubleSize support.** Extend the doublesize machinery to the
   playlist window: compose `playlistwin_bg` at 2× (reuse
   `create_dblsize_image()`), scale slider/button/text widget geometry and
   hit tests, and load the playlist font at doubled pixel size. Shaded EQ
   style rows (29 px tiles, 25 px buttons) are doubled by the same factor
   arithmetic used in `skin_get_mask()`. Keep 1× rendering byte-identical
   when doublesize is off.
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
      (`playlistwin_set_doublesize()`).
- [x] Playlist window scales with DoubleSize for the first time; clicks map
      to correct rows/buttons at 2× (event coordinates halved at dispatch,
      mirroring the documented mainwin mechanism).
- [x] Focused tests, Preflight (`make && xvfb-run make check && make lint`),
      and `make distcheck` pass.

## Verification Evidence

- `tests/test-hidpi-scale` — 9/9 pass (parsing, RDB lookup, thresholds;
  caught and fixed a use-after-free in the first parser draft during RED).
- `tests/test-hidpi-startup` — 3/3 pass (dpi-192 doubles both windows;
  `skin_autoscale=FALSE` and standard-DPI stay 1×).
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
