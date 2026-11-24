# Plan: automatic skin scale from display resolution (QHD baseline)

Date: 2026-11-24
Status: Implemented on `feat/resolution-skin-scale`.

## Problem

The skinned UI has a fixed pixel size (main/EQ 275x116). On a 32" 4K
monitor the player looks too small, while the same build on a QHD laptop
looks right. The existing HiDPI autoscale (`skin_autoscale`,
`util_skin_scale_for_dpi()`) only reacts to `Xft.dpi >= 144`; desktops
that report no (or a low) DPI — plain X11, many 4K setups — never scale
up.

## Decision

A continuous **skin size coefficient** relative to a QHD baseline,
mapped onto the only integer scales the skinned renderer supports
(1x / DoubleSize):

```
coefficient = min(screen_w / 2560, screen_h / 1440)     /* QHD = 1.0 */
skin_scale  = round(coefficient), clamped to [1, 2]     /* crisp integer scaling */
```

- 2560x1440 (QHD) → 1.0 → 1x (accepted default; laptops unchanged).
- 3840x2160 (4K) → 1.5 → 2x (doublesize).
- 1920x1080 → 0.75 → 1x. 5120x2880 → 2.0 → 2x.
- The tighter axis (min) prevents ultrawide panels from over-scaling.

Startup decision in `main.c` (`startup_skin_scale()`):

1. `skin_scale` config float ≥ 1.0 → used as the coefficient, wins over
   everything (including `skin_autoscale=FALSE`).
2. Otherwise, when `skin_autoscale` is TRUE: the **strongest** of
   resolution coefficient and `Xft.dpi`/96 coefficient decides.
3. Mapped scale ≥ 2 → same startup path as the persisted DoubleSize
   (`cfg.doublesize = TRUE` before `mainwin_create()`; the existing
   dock-group resnap reconstruction handles geometry).

## Rejected alternatives

- **True fractional rendering (1.5x):** the whole skinned stack
  (`cfg.doublesize` booleans, widget coordinates, `region.txt` masks,
  vis rendering) is integer-scale by construction. Resampling every
  sprite would blur pixel art and violate the skin-compatibility rule.
- **Resolution replaces DPI:** would regress GNOME XWayland (200%
  desktops report real DPI). Max-combining keeps that working and adds
  coverage where DPI is missing.

## Config surface

`[xmms]` in `~/.xmms/config`:

| Key | Type | Default | Meaning |
| --- | --- | --- | --- |
| `skin_scale` | float | 0 | 0 = automatic; ≥ 1.0 = explicit coefficient override (1.0 pins 1x on 4K, 1.5 forces doublesize anywhere) |
| `skin_autoscale` | bool | TRUE | unchanged: persistent opt-out of the automatic signals |

The renderer cap (`XMMS_SKIN_MAX_SCALE = 2`) saturates larger
coefficients (e.g. 8K → 2x) until the renderer supports higher integer
scales.

## Files

- `xmms/util.h` / `xmms/util.c`: `XMMS_SKIN_BASE_WIDTH/HEIGHT`,
  `XMMS_SKIN_MAX_SCALE`, `util_skin_coefficient_for_resolution()`,
  `util_skin_scale_for_coefficient()`, `util_skin_scale_for_display()`;
  `util_skin_scale_for_dpi()` refactored onto the coefficient mapping
  (identical results).
- `xmms/main.h` / `xmms/main.c`: `Config.skin_scale` (read/write),
  `startup_skin_scale()`.
- `tests/test-hidpi-scale.c`: coefficient baseline, integer mapping,
  resolution/DPI combination.
- `tests/test-hidpi-startup.c`: override beats DPI autoscale; override
  forces doublesize without DPI; private-Xvfb 3840x2160 → doublesize and
  2560x1440 → 1x with no DPI signal; ambient tests assert against the
  same rule so they stay valid on any `make check` screen.
- `docs/manual.md`, `docs/architecture/ui-interaction.md`: user and
  architecture docs.

## Verification

- `make -j$(nproc)`, `xvfb-run --auto-servernum make check`, `make lint`
  all green.
- E2E: `/hidpi-startup/resolution-4k-starts-doublesize` spawns a real
  Xvfb at 3840x2160x24 (no `Xft.dpi`), asserts the main window opens at
  550x232; `/hidpi-startup/resolution-qhd-starts-single-size` asserts
  275x116 at 2560x1440.
