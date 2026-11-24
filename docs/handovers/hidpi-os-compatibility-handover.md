# Handover: OS-specific HiDPI/window fixes (Ubuntu GNOME vs Kubuntu KDE)

Scope: **only the changes required because Ubuntu (GNOME/mutter) and Kubuntu
(KDE/KWin) treat X11/XWayland clients differently.** Window-size decisions
(playlist scale, player width matching, etc.) are intentionally excluded —
see the final section for the boundary.

Implemented on branch `fix/hidpi-xwayland-scaling`; specs and evidence:
`specs/bugs/BUG-2026-11-23T145455-hidpi-xwayland-scaling.md`.

---

## 1. The OS difference being handled

XMMS is a fixed-pixel skinned application: the player draws from 275×116 px
skin bitmaps. Whether that appears correctly sized depends entirely on the
desktop's policy for legacy X11/XWayland clients:

| Desktop | XWayland policy | Result before the fix |
| --- | --- | --- |
| Kubuntu 26.04 (KWin) | KWin upscales the X11 window to the desktop scale by default | looked correct (soft) |
| Ubuntu 26.04 (GNOME/mutter) | XWayland is left at scale 1.0; the X screen is the full native panel | "incredibly small" (275 px on a 6144 px-wide screen) |

Measured on the affected machines:

```
xdpyinfo: dimensions 6144x3456, resolution 96x96 dots per inch
xrdb -query: Xft.dpi: 192
```

So the X surface is unscaled (96 DPI) while the desktop scale is 2×. A
portable XMMS must therefore decide for itself whether to scale — it cannot
rely on the compositor.

---

## 2. Required change A — read the desktop scale and self-scale

This is the change that makes the same binary usable on both desktops.

### 2.1 DPI utilities (`xmms/util.c`, `xmms/util.h`)

```c
gint util_xft_dpi_parse(const gchar *value);      /* strict, 96 fallback  */
gint util_xft_dpi_from_rdb(const gchar *rdb);     /* Xrm "Xft.dpi" lookup */
gint util_get_xft_dpi(void);                      /* RESOURCE_MANAGER     */
gint util_skin_scale_for_dpi(gint dpi);           /* >=144 -> 2, else 1   */
```

- `util_get_xft_dpi()` opens the display, reads
  `XResourceManagerString()`, parses `Xft.dpi` with Xrm; returns 96 when
  there is no display or no resource.
- Threshold: `XMMS_HIDPI_DOUBLE_DPI 144` (1.5× or more counts as HiDPI).
- Guard against unusable values (NULL, empty, non-numeric, <=0, >4096 →
  96). Round fractional DPI.

### 2.2 Startup decision (`xmms/main.c`, `xmms/main.h`)

New config field and key:

```c
/* main.h */  gboolean skin_autoscale;   /* default TRUE */
```

Read/written in `read_config()` / `save_config()` under `[xmms]` as
`skin_autoscale`.

In `main()`, after `gtk_init_check()` succeeded and **before**
`mainwin_create()`:

```c
if (cfg.skin_autoscale &&
    util_skin_scale_for_dpi(util_get_xft_dpi()) >= 2)
    cfg.doublesize = TRUE;
```

Set the flag rather than calling `set_doublesize()`: pre-create, that
function dereferences windows that do not exist yet. Setting
`cfg.doublesize` before window creation is exactly the existing
"persisted DoubleSize" startup path.

### 2.3 Contract

- `skin_autoscale=TRUE` (default): HiDPI displays start in DoubleSize.
- `skin_autoscale=FALSE`: historical always-1× startup (the pre-fix
  behaviour).
- The Ctrl+D toggle still wins for the running session. Because the legacy
  config format writes `doublesize=FALSE` on every exit, it cannot
  distinguish "untouched default" from "explicit opt-out";
  `skin_autoscale=FALSE` is therefore the persistent opt-out.
- No existing config key is removed or reinterpreted; this is a new key
  only.

Tests: `tests/test-hidpi-scale.c` (parsing, resource lookup, thresholds).

---

## 3. Required change B — apply shape masks after resizing

This is a compositor-behaviour fix, not a styling choice.

### Symptom

After a DoubleSize or shade change, a window could appear **cropped to its
previous size** even though:

- `XGetWindowAttributes` reported the new geometry,
- `XGetImage` of the window returned complete, correct content,
- the mask bitmap itself had the correct dimensions.

The clip existed only in the compositor.

### Cause

XWayland compositors (observed with mutter) retain the **pre-resize shape
region** when a window's shape mask is changed while the window still has
its old size. XMMS applied the new mask, then resized; the compositor kept
clipping to the smaller mask and never recovered.

### Minimal fix

Apply the mask **after** the window resize, never before:

- `xmms/main.c`, `set_doublesize()`: move `mainwin_set_shape_mask()` after
  `dock_resize()`.
- `xmms/main.c`, `mainwin_set_shade_menu_cb()`: move it after
  `dock_shade()`.
- `xmms/equalizer.c`, `equalizerwin_set_doublesize()` and
  `equalizerwin_set_shade_menu_cb()`: same reordering.
- `xmms/playlistwin.c`, `playlistwin_motion()`: after the interactive
  resize (`gdk_window_resize()` / `gtk_widget_set_usize()`), re-create the
  mask and re-set the back pixmap:

  ```c
  playlistwin_create_mask();
  playlistwin_set_back_pixmap();
  draw_playlist_window(TRUE);
  ```

  Without this, dragging the playlist larger left the newly grown strip
  invisible until a later re-mask.

Note: the playlist's WM-driven configure path already resized first and
masked afterwards, which is why only the drag path clipped.

Masks must also always match the window size exactly; a mask smaller than
the window is the same clip from the app side.

---

## 4. Optional (cosmetic parity, not correctness)

KWin upscales the window with filtering. XMMS's own DoubleSize used
nearest-neighbour pixel doubling, which is visibly chunkier. To match the
Kubuntu look on GNOME, `create_dblsize_image()` now performs a bilinear
scale (destination *d* samples source at `d/2 − 0.25`, i.e. 1/4–3/4 weights,
clamped edges; 8/16-bit visuals keep the old path). Generalised as
`scale_image(img, out_w, out_h)` in `xmms/util.c`.

This is presentation only: omitting it does not affect correct sizing or
window behaviour.

---

## 5. Verification

### 5.1 On each desktop

```sh
xrdb -query | grep -i dpi                      # expect Xft.dpi: 192 at 200%
xdpyinfo | grep -E 'dimensions|resolution'     # expect the native panel
./xmms/xmms                                    # expect correctly sized windows
```

Then, on both machines:

1. `Ctrl+D` several times — no cropping, no stuck layout.
2. Shade/expand the main window and the equalizer — no cropping.
3. Drag-resize the playlist wider and taller — newly grown area renders
   immediately.
4. Set `skin_autoscale=FALSE`, restart — windows come up at 1× exactly as
   the pre-fix build did.

### 5.2 Automated (headless)

`tests/test-hidpi-startup.c` spawns the real binary under an isolated HOME,
injects `Xft.dpi` through the X server's `RESOURCE_MANAGER` property
(`XChangeProperty` on `XA_RESOURCE_MANAGER`), and asserts window geometry:

```sh
xvfb-run --auto-servernum make check
```

The property is restored afterwards so the run does not affect other tests.

---

## 6. Files in the minimal OS-specific change set

| File | Change |
| --- | --- |
| `xmms/util.c` / `xmms/util.h` | DPI read helpers; `scale_image()` (optional smoothing) |
| `xmms/main.c` / `xmms/main.h` | `skin_autoscale` key; startup decision; mask ordering |
| `xmms/equalizer.c` | mask ordering |
| `xmms/playlistwin.c` | re-mask after interactive resize |
| `tests/test-hidpi-scale.c`, `tests/test-hidpi-startup.c` | unit + end-to-end coverage |
| `tests/Makefile`, `Makefile.am`, `Makefile.in` | test registration (`Makefile.in` mirrors `Makefile.am`; the checked-in Automake metadata predates the local Automake) |
| `xmms.1.in`, `docs/manual.md`, `docs/architecture/ui-interaction.md`, `CHANGELOG.md` | documentation |

Compatibility notes: UI-only; no plugin vtable, exported symbol, socket
framing or skin-format change; one new config key.

---

## 7. Explicitly out of scope

These belong to the same branch but are **not** OS-specific — they are
window-size/app-logic decisions and are excluded from this handover:

- Playlist editor fixed 3/2 scale, player/EQ width matching, arbitrary
  player scale, the generalized `scale_image()` sizing use.
- Dock-chain re-snapping after a rescale (`resnap_dock_group()`), which
  fixes window shuffling when the three docked windows change size — an
  application-logic bug reproducible independently of the desktop.
