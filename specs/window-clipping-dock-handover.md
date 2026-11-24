# Handover: cropped skinned windows and docked-window shuffle

Scope: what caused, and what fixes, the state in the UAT screenshot where
the **player and equalizer appear cropped to their top-left corner while the
playlist editor renders completely**, with **gaps between the docked
windows** instead of a flush stack.

Implemented on branch `fix/hidpi-xwayland-scaling`. Full investigation and
evidence: `specs/bugs/BUG-2026-11-23T145455-hidpi-xwayland-scaling.md`.
Window-size decisions (playlist scale, player width matching) are out of
scope here; see the final section.

---

## 1. What the screenshot shows

- Player window: correct doubled pixel content (large time digits, large
  title text) but only its left portion is visible; the right side — and the
  lower part — is missing.
- Equalizer: same pattern; roughly the left third of the sliders visible.
- Playlist editor: complete frame and content.
- All three windows: separated by gaps instead of the usual flush stack.

Two independent defects produce this combination.

---

## 2. Why it was hard to see from inside XMMS

Everything XMMS could inspect looked correct in the broken state:

```
XGetWindowAttributes  -> the new (doubled) window size
XGetImage(window)     -> complete, correct window content
mask bitmap size      -> the new (scaled) size
```

The clipping existed only in the compositor's presentation. Two diagnostic
traps to remember:

- **`XGetImage` does not apply the window shape.** A shaped window returns
  full contents even when the compositor shows only the shaped region.
- **`gdk_window_get_size()` during a transition is asynchronous.** Read
  immediately after a resize request it can return intermediate values
  (observed: 200 px heights while animating between 116 and 232). Geometry
  must be computed (or read after the window has settled), never sampled
  mid-rescale.

The decisive confirmation came from the user watching a scripted toggle run
on the affected session: the X-level state was correct at every step while
the screen showed the cropped windows.

---

## 3. Defect 1 — shape mask applied before resizing (the cropping)

### Cause

The player and equalizer applied their new shape mask *before* resizing the
window (`mainwin_set_shape_mask()` / `equalizerwin_set_shape_mask()` ran
ahead of `dock_resize()` / `dock_shade()`). XWayland compositors (observed
with mutter) retain the **pre-resize shape region** in that case: the
expanded window is composited clipped to the previous, smaller mask, and it
never recovers on its own.

The playlist was unaffected because it already rebuilt its mask *after* its
resize — the strongest clue in the screenshot.

### Minimal fix

Apply the mask **after** the resize in every path that changes size:

| Function | Change |
| --- | --- |
| `mainwin_set_doublesize()` (`xmms/main.c`) | move `mainwin_set_shape_mask()` after `dock_resize()` |
| `mainwin_set_shade_menu_cb()` (`xmms/main.c`) | move it after `dock_shade()` |
| `equalizerwin_set_doublesize()` (`xmms/equalizer.c`) | move it after `dock_resize()` |
| `equalizerwin_set_shade_menu_cb()` (`xmms/equalizer.c`) | move it after `dock_shade()` |
| `playlistwin_motion()` (`xmms/playlistwin.c`) | after the drag resize, re-create the mask and re-set the back pixmap before drawing |

Rule to keep: **the shape mask must always match the window size exactly.**
A mask smaller than the window is the same clip from the application side.
(Any feature that changes the double-mode scale must therefore rebuild the
mask cache for that scale; see `skin_set_scale()` in `xmms/skin.c`.)

Commit: `41be268 fix(ui): apply shape masks after resizing, never before`,
plus `144d2c0 fix(ui): re-mask the playlist after interactive resize`.

---

## 4. Defect 2 — dock chain shuffle (the gaps)

### Cause

`dock_resize()` re-docks only the windows docked **directly** to the window
being resized. Rescaling a vertical chain (player → equalizer → playlist)
runs three sequential docking passes; each recomputes docking from the
intermediate geometry. Windows that were flush below a resized window get
pulled to its new edge, while windows further down the chain keep their old
offsets — producing detached or overlapping windows (measured: EQ and
playlist both landing on the same y coordinate). Saved window positions are
also only consistent at 1×, so an autoscaled start needs the same treatment.

### Minimal fix

One coordinated re-snap after the whole group has been rescaled
(`resnap_dock_group()` in `xmms/main.c`):

- Run it once at the end of `set_doublesize()`, and once at startup when
  autoscale enabled DoubleSize before the windows were shown.
- Relations come from the **pre-rescale** geometry: a window that was flush
  below/above/beside its leader moves to the leader's new edge; unrelated
  windows are left alone.
- Leader chain: equalizer relative to player, playlist relative to
  equalizer, falling back to the player when the playlist was not docked to
  the equalizer.
- New sizes are **computed** from the skin geometry, never read back
  mid-transition (see §2).
- The pre-rescale scale factor must be captured **before** `cfg.doublesize`
  is updated. Otherwise the startup menu-state callback — which re-enters
  `set_doublesize()` with an unchanged scale — mislabels the old geometry
  and corrupts the snap.
- Position bookkeeping that must survive: `dock_get_widget_pos()` reads
  stored data synchronously, which is safe for unmapped windows; only the
  *sizes* were the async hazard.

Commit: `e0caccc fix(ui): re-snap dock chain when the skinned group rescales`.

---

## 5. Verification procedure

X-level assertions alone are insufficient for defect 1 — the acceptance
test is visual on a compositor that applies shapes.

Manual, on the affected desktop:

1. `Ctrl+D` four times; after each state the three windows must be flush
   and fully visible (no cropped strips).
2. Collapse/expand main window and equalizer (shade paths share the mask
   ordering).
3. Drag-resize the playlist wider and taller; the newly grown area must
   render immediately, without a later Ctrl+D to "repair" it.
4. Restart with a saved layout: the stack must come up flush.

Automated (headless, no compositor shapes applied):

```sh
xvfb-run --auto-servernum make check
```

- `tests/test-hidpi-startup.c` spawns the real binary with an isolated
  HOME, injects `Xft.dpi` through the server's `RESOURCE_MANAGER`
  property, and asserts window geometry including the Ctrl+D collapse.
- Geometry tests prove the dock/size logic but **cannot** catch shape
  clipping; keep the visual checklist above for that.

---

## 6. Files in this change set

| File | Change |
| --- | --- |
| `xmms/main.c` | mask ordering in doublesize/shade; `resnap_dock_group()` and its call sites |
| `xmms/equalizer.c` | mask ordering in doublesize/shade |
| `xmms/playlistwin.c` | mask + back-pixmap re-apply after drag resize; configure-height escape preserved |
| `xmms/skin.c` / `xmms/skin.h` | mask cache rebuild support (`skin_set_scale()`) |
| `tests/test-hidpi-startup.c` | end-to-end geometry coverage |
| `specs/bugs/BUG-2026-11-23T145455-hidpi-xwayland-scaling.md` | investigation and evidence |

No plugin vtable, exported symbol, socket-framing or skin-format changes.

---

## 7. Out of scope (do not confuse with these fixes)

- Playlist fixed 3/2 scale, player/equalizer width matching, arbitrary
  player scale, generalized bilinear scaling — window-size/appearance
  decisions.
- The HiDPI `Xft.dpi` decision itself (`skin_autoscale`) — see
  `specs/hidpi-os-compatibility-handover.md`.
