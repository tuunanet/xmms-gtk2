# Plan: first-run config defaults pin DoubleSize

Date: 2026-11-28
Status: Implemented on `feat/resolution-skin-scale`.

## Problem

A fresh install without `~/.xmms/config` relies on the automatic
startup signals (display resolution, Xft.dpi). On a smaller screen —
or any display at 96 DPI below the QHD baseline — the first session
opens at the classic 1x size (275x116), which reads as broken on the
modern HiDPI desktops this fork targets. The user must discover
`skin_scale` and hand-edit a config file that does not even exist
until the first clean exit.

## Decision

When `read_config()` finds no `~/.xmms/config`, the first run now
seeds and immediately writes a config file with three keys:

```ini
[xmms]
doublesize=TRUE
skin_autoscale=TRUE
skin_scale=1.5
```

- `skin_scale=1.5` is the startup authority: the coefficient rounds up
  (`util_skin_scale_for_coefficient`) to 2x, so every fresh install
  opens at DoubleSize (550x232) regardless of the display.
- `doublesize=TRUE` keeps the persisted legacy bit consistent with the
  session the user actually sees; it is honored on later starts only
  if both automatic signals are removed (legacy path).
- `skin_autoscale=TRUE` matches the existing in-memory default and is
  written explicitly so the file states all three keys.

Existing configs are never touched by this path: the seeding runs only
in the `cfgfile == NULL` branch of `read_config()`. An upgraded config
without a `skin_scale` key keeps the automatic coefficient default
(0), so no existing user's startup size changes.

### Mechanics

`save_config()` cannot be reused for the first-run write: it
dereferences UI widgets (`mainwin_shuffle->tb_selected`) and the skin
state, none of which exist during `read_config()`. The new
`write_first_run_config()` writes a minimal `ConfigFile` with just the
three keys. `save_config()` opens the existing file on exit and
rewrites every key it manages — including all three — so the file
converges to a complete config after the first session; the seeded
keys survive because `save_config()` writes them unconditionally.

`read_config()` runs before `main()` calls `make_xmms_dir()`, so
`write_first_run_config()` creates `~/.xmms` (plus Skins and Plugins)
itself before `fopen`. `xmms_cfg_write_float` uses `%g` in the C
locale, so the file literally contains `skin_scale=1.5`.

## Constraints honored

- Config-file format unchanged: the `doublesize` key stays boolean on
  disk and the `skin_scale` key keeps its float format; no new keys.
- No plugin ABI, socket framing, or skin-format changes.
- 1x/2x/3x rendering paths untouched; only the startup decision
  input changes for first runs.

## Tests

- `tests/test-hidpi-startup.c` gained
  `/hidpi-startup/first-run-seeds-doublesize-config`: it launches the
  real binary with an isolated `HOME` containing no config, asserts
  the player maps at 550x232 (DoubleSize) on the ambient screen, and
  asserts `~/.xmms/config` exists with `doublesize=TRUE`,
  `skin_autoscale=TRUE`, and a `skin_scale` value parsing to 1.5.
- The launcher was parameterized (`create_config`,
  `config_contents`) so the same harness covers the no-config first
  run; all pre-existing cases still seed a config and are unaffected.
