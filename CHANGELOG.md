# Changelog

All notable changes to this fork are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioned entries will follow
[Semantic Versioning](https://semver.org/spec/v2.0.0.html) when the fork begins
publishing tagged releases.

The detailed upstream history (1997–2007) is preserved under
[`docs/history/`](docs/history/README.md). This file is the canonical changelog
for XMMS GTK2.

---

## [Unreleased]

### Added
- N x integer skin scaling: the skinned UI now supports 1x, 2x, and 3x
  (main/equalizer at 825x348 in triple size). The scale selector is the
  same `skin_scale` coefficient — values from 2.5 (and 8K-class
  resolutions, e.g. 7680x4320) select 3x — and the Ctrl+D / DoubleSize
  toggle switches between 1x and the session's chosen scale. Every
  scaled component was generalized from the hardcoded 2x: backing
  pixmaps (`create_scaled_image`), region.txt shape masks (generated
  per scale), event coordinate translation, window hints and dock
  geometry, and the analyzer/vis renderers (now computed at 1x and
  expanded by pixel replication). The playlist follows the player's
  scale too — including its drag-resize steps, geometry hints, shape
  mask, and embedded vis — fixing the mixed-size docked chain at 2x/3x;
  1x and 2x behavior is otherwise unchanged. Covered by unit tests
  (coefficient mapping to 3), end-to-end startup tests (`skin_scale=3.0`
  and a private-Xvfb 8K screen both render 825x348), and dock-resnap
  cases exercising the scaled playlist in the docked chain (600x464 at
  2x, 900x696 at 3x on a private 2560x1440 screen) plus the
  3x-to-1x-to-3x toggle cycle.
- Automatic skin scale from the display resolution: XMMS now picks its
  startup size from a size coefficient relative to the QHD baseline —
  2560x1440 keeps the classic 1x UI, while larger screens scale up
  (a 32" 4K panel at 1.5 rounds up to DoubleSize) even when the desktop
  reports no `Xft.dpi`. The automatic scale is the stronger of the
  resolution ratio and the existing DPI signal. The new `skin_scale`
  float key in `[xmms]` (default 0 = automatic) overrides the
  coefficient explicitly; `skin_autoscale=FALSE` still opts out of the
  automatic signals. A decision signal also replaces a stale
  `doublesize=TRUE` saved by an earlier doubled session, which
  previously masked both keys at startup. Covered by
  `tests/test-hidpi-scale.c` (coefficient
  math) and new end-to-end startup tests on private Xvfb 4K/QHD screens.
- First-run config defaults: when `~/.xmms/config` does not exist yet,
  XMMS now creates it at startup with `doublesize=TRUE`,
  `skin_autoscale=TRUE`, and `skin_scale=1.5`, so fresh installs open
  the skinned UI at DoubleSize on every display instead of the classic
  1x on smaller screens. Existing configs are never rewritten with
  these defaults. Covered by an end-to-end first-run startup test
  asserting both the window geometry and the written keys.

### Fixed
- Restored mouse wheel support dropped by the GTK2 port: the GTK1-era
  handlers listened for the wheel buttons 4/5 as button presses, but
  GTK2 delivers wheel motion as GDK scroll events, so none of them
  ever fired — most visibly, the playlist editor no longer scrolled
  with the wheel. New scroll-event handlers restore the original
  behaviors: the playlist list scrolls three entries per notch, the
  main window adjusts the volume with the vertical wheel and seeks
  with a horizontal wheel, and the equalizer preamp/band sliders step
  under the pointer. Covered by an XTEST end-to-end test that wheels
  the playlist and asserts the list content moves and then restores
  pixel-identically.
- Scaled the playlist's hold-to-popup skinned menus (the submenus that
  pop up while holding the file+, file-, select-all, misc-options, and
  load-list buttons): the popup window was still created at 1x with
  1x skin blits and 1x motion hit-testing, and its anchor mixed the
  window's device origin with logical widget offsets — at 2x/3x the
  menu appeared tiny and detached from the pressed button, and the
  item highlight never tracked the pointer because the popup window
  was not under it. The menu is now composed at 1x and replicated
  into a scaled popup window (25xnum_items*18 at 1x), pointer motion
  is translated by the scale, and the anchors are scaled offsets; 1x
  geometry is pixel-identical to the stock behavior.
- Removed a leftover unconditional 1x visualization draw from the N x
  scaling rework: every vis_draw call painted the analyzer twice — once
  at the 1x device position and once at the scaled position — so at
  2x/3x two spectrum displays animated in the main panel at once. The
  frame is now drawn exactly once, through the scaled path.

## [1.2.14] - 2026-11-27

### Added
- Playlist editor font scale option: `playlist_font_scale` in the
  `[xmms]` section of `~/.xmms/config` scales the playlist text as a
  percentage of the chosen font's size (clamped to 25–400; default
  `100`, which keeps the font's native bitmap pixels untouched). The
  configured font string is never rewritten, and alias/wildcard font
  names load unscaled. Covered by string and load-time tests in
  `tests/test-font-load.c`.
- Robust native default playlist font: the playlist editor now
  defaults to the natively available 6x12 `fixed` bitmap font
  (`-misc-fixed-medium-r-semicondensed--12-110-75-75-c-60-iso8859-1`)
  instead of the 13px bold semicondensed bitmap, whose server-scaled
  sizes rendered thin and broken glyphs. The new default is one pixel
  shorter, keeps the condensed classic look, and renders crisp at the
  default scale. Configs still carrying the 13px bold default are
  migrated to the new default by the existing historical-font
  migration, together with the interim `playlist_font_scale=90`
  default; the 13px bold name keeps loading (as the migrated default)
  through `util_font_load`.

### Fixed
- Decoded UTF-16 ID3v2 text frames to UTF-8 regardless of the process
  locale. With a non-UTF-8 locale (for example C/POSIX, common in
  containers and headless environments), mp3 tag reading and title
  editing silently dropped every non-ASCII character; the ID3v2.3
  write path already assumed UTF-8, so round-trips now preserve
  titles exactly at any locale.
- Declared `xfonts-base` as a runtime dependency of the `xmms`
  package and added it to the release build environment: the playlist
  default font (and every skinned-window bitmap font pattern) resolves
  from that package, and the packaging tests previously ran without
  it, silently font-fallback-loading the playlist text.
- Kept the docked skinned windows flush and fully visible across HiDPI
  autoscale and DoubleSize toggles on XWayland compositors that apply
  shape masks (mutter): the player and equalizer now apply their shape
  mask *after* resizing (both doublesize and shade paths, plus the
  playlist drag-resize path), and the docked group is re-snapped once
  per rescale from pre-rescale geometry — including an autoscaled
  start, where windows previously came up doubled at their saved 1×
  spacing (overlapping), and toggle cycles could land the playlist on
  the equalizer's position.
- Started the skinned windows at the desktop scale on HiDPI desktops whose
  XWayland compositors leave X11 clients unscaled (for example Ubuntu GNOME
  at 200% scaling, where the player previously opened at 275×116 on a
  6144-px-wide screen). `main.c` now reads `Xft.dpi` from the X
  `RESOURCE_MANAGER` property before window creation and starts in
  doublesize when it reports 144 DPI or more. The new `skin_autoscale`
  key in `[xmms]` (default TRUE) opts out persistently, for example under
  KWin, which already upscales XWayland clients itself.
- Covered by `tests/test-hidpi-scale.c` (DPI parsing and threshold),
  `tests/test-hidpi-startup.c` (end-to-end startup geometry under an
  injected `Xft.dpi`), and `tests/test-dock-resnap.c` (dock chain stays
  flush across an autoscaled start and a Ctrl+D cycle).

## [1.2.13] - 2026-07-29

### Added
- Added a manually dispatched Linux Mint 22.3 package workflow that builds and
  install-tests correctly named runtime and development DEBs in a pinned Mint
  image, then uploads source, checksum, and package provenance artifacts.
- Extended package automation with a pinned Ubuntu 26.04 build and a gated,
  least-privilege final job that assembles both verified package sets into a
  draft GitHub Release for a matching annotated tag.

### Fixed
- Preserved source-tree test compiler flags when release validation supplies
  `CFLAGS`, allowing `make distcheck` to verify the complete distribution.
- Treated legacy GTK callback type diagnostics as non-fatal on current GCC so
  clean source-tree builds remain usable.

## [1.2.12] - 2026-07-29

XMMS GTK2 1.2.12 replaces the unpublished 1.3.1 and 1.3.2 draft versions
and is intended to be the first published XMMS GTK2 release. It continues
the original XMMS 1.2 release line and includes the draft changes listed below.

### Added
- Added a top-level `make deb` target and made pull-request and release-candidate
  CI build and verify Debian packages through it. Candidate artifacts now
  include install-tested runtime and development DEBs with checksums.

### Removed
- Removed RPM packaging recipes, the Fedora package workflow job, and the
  legacy Autotools-generated `xmms.spec` files. Official native packages are
  DEB-only for Ubuntu 24.04 LTS.

### Fixed
- Restored GTK keyboard shortcuts by forwarding unhandled main-window key
  events to the accelerator dispatcher.
- Fixed `make deb` source archive assembly after the project-local reviewer and
  CI files were removed.
- Allowed legacy GTK callback pointer conversions in Debian package builds so
  `make deb` also works with GCC 15 on newer Ubuntu systems.
- Linked the MP3 input plugin directly to the math libraries it uses, so native
  packages can load the plugin and add or play MP3 files without relying on
  symbols from the main executable.

## [1.3.1] - 2026-07-27 (unpublished draft)

The unpublished 1.3.1 draft established the maintained, preservation-focused
continuation of XMMS 1.2.11 for current Linux and BSD systems.

### Added
- Added tested Ubuntu 24.04 DEB and Fedora 42 RPM release packages, including
  development packages for plugin authors and checksum metadata.
- Added a credited, GPL-licensed screenshot of the classic XMMS interface to
  the project overview.
- Added GitHub Actions CI for clean configuration, compilation, Xvfb-backed
  regression tests, and source-distribution verification on Ubuntu 24.04.
- Added structured bug and enhancement issue forms, a pull-request template,
  weekly Dependabot updates for GitHub Actions, and contribution, conduct, and
  security policies.
- Added EditorConfig and Git attributes so text encoding, line endings, and
  indentation expectations are explicit.
- Added automated release-candidate builds for short-lived `release/*` branches
  and tag-driven draft GitHub Releases with validated versions, distchecked
  source archives, checksums, and curated changelog notes.

### Changed
- Native packages are attached to a verified draft GitHub Release before
  publication, so the complete source and package release becomes immutable in
  one step.
- Consolidated the fork's release history in `CHANGELOG.md`. Moved the original
  `NEWS`, detailed `ChangeLog`, GTK1-era `FAQ`, and stale developer `TODO` to
  `docs/history/` with an index that clearly marks them as archival.
- Replaced the top-level `AUTHORS` file with the readable Markdown
  `CONTRIBUTORS.md`, preserving all original credits.
- Removed the generic, obsolete Automake `INSTALL` document; current build and
  installation instructions live in `README.md`.
- Integrated the existing regression suite into the conventional top-level
  `make check` target.
- Clarified repository lineage and attribution by distinguishing the original
  XMMS project, the 2015 import and GTK2 port, and current maintenance.
- Adopted the **XMMS GTK2** project name and renamed the GitHub repository to
  `xmms-gtk2`, while retaining compatibility-sensitive
  `xmms` runtime and package identifiers.

### Fixed
- Calculated MP3 playlist durations from MPEG frame headers when no Xing header
  is present, instead of extrapolating a VBR file from its first frame's bitrate;
  accelerated these scans for large local playlists.
- Positioned the playlist's hold-activated bottom-row menus above their buttons
  instead of at the upper-left corner of the screen under XWayland.
- Replaced incomplete Adobe bitmap-font patterns with byte-compatible 6x13
  `fixed` family fonts: bold for playlist text and matching medium weight for
  main-window X text. This preserves the clear classic appearance without GTK2
  character-set warnings; short-lived replacement defaults are migrated.
- Prevented timestamp-dependent source-distribution failures from regenerating
  the bundled gettext plural parser with incompatible modern Bison output.
- Fixed MP3 seeks landing before the requested position and playing beyond the
  reported duration by preserving fractional Xing percentages, anchoring seek
  offsets at the first MPEG frame, and completing finite tracks at their known
  length ([#16](https://github.com/tuunanet/xmms-gtk2/issues/16)).
- Fixed GTK popup menus opening at the screen's upper-left corner instead of
  their requested pointer position
  ([#12](https://github.com/tuunanet/xmms-gtk2/issues/12)).
- Fixed ALSA volume and balance controls when the configured hardware mixer is
  unavailable, as with PipeWire-backed default devices, by falling back to the
  plugin's software volume control
  ([#10](https://github.com/tuunanet/xmms-gtk2/issues/10)).
- Fixed source-distribution (`make distcheck`) builds by correcting the bundled
  gettext implementation's obsolete `getcwd()` declaration and making
  `libxmms` resolve public headers from out-of-tree builds.
- **Clean configure now succeeds.** The pre-generated `configure` script
  requires the `mkinstalldirs` automake auxiliary file, which was never
  committed (not even in the original 1.2.11 import), causing `./configure` to
  fail from a clean checkout with
  `cannot find required auxiliary files: mkinstalldirs`. Added the standard
  automake `mkinstalldirs` helper so the documented `./configure && make` flow
  works without `autoreconf`.
- Removed modern Autoconf `datarootdir` warnings from every generated build
  template while preserving custom `--datarootdir` installation paths. Removed
  the obsolete duplicate `ALL_LINGUAS` declaration; `po/LINGUAS` remains the
  canonical translation list.
- Extended `.gitignore` to cover the `tests/test-alsa-pcm-state` binary, the
  only compiled test not previously ignored.

The work below completed the GTK2 compatibility and modern-build plan: a
17-task effort to restore GTK2 entry navigation, clear the GCC 15 build
blockers, and port the remaining GTK2 callback and X11 API boundaries so XMMS
1.2.11 builds and runs on contemporary Linux and BSD systems.

### GTK2 / GLib2 port
- Migrated the codebase from GTK1/GLib1 to **GTK 2.0 / GLib 2.0**, including
  `libxmms`, the main UI, and the build system (`AM_PATH_GTK_2_0`,
  `AM_PATH_GLIB_2_0`).
- Switched window/dialog types to GTK2 equivalents and removed GTK1-only
  constructs (e.g. `GtkPacker` in the joystick configuration UI, replaced with a
  supported GTK2 container).
- Ported the custom entry widget (`libxmms/xentry.c`) word-navigation to
  GTK2 public visibility/text APIs and UTF-8 character offsets, preserving
  Alt+F/Alt+B word movement for read-only entries.
- Added display-independent GLib regression tests for entry word-boundary
  navigation, including UTF-8 coverage.

### GCC 15 build compatibility
- Renamed the C23-reserved `bool` parameter in `xmms/controlsocket.c`.
- Corrected the `fts_open()` null-comparator type in `xmms/util.c`.
- Restored native callback signatures for the `sbutton` and `pbutton` custom
  widgets instead of `GTK_SIGNAL_FUNC()` field assignments.
- Cast GTK signal handlers at the `gtk_signal_connect()` boundary across the
  main UI, input dialog, playlist, equalizer, about dialogs, and the OSS,
  mpg123, CD-audio, and tone-generator plugins.
- Converted `GdkAtom` values to X11 `Atom` at Xlib boundaries in `xmms/hints.c`
  and `xmms/main.c`, preserving `GdkAtom` for GDK calls.
- Updated changed GTK2 pointer-grab and accelerator APIs
  (`gdk_pointer_grab()`, `gtk_accel_group_activate()`).

### Output and plugin handling
- Defaulted audio output to **ALSA** on modern Linux, migrating unusable OSS
  defaults to ALSA and preferring the available ALSA output plugin.
- Fixed plugin discovery from development build trees so plugins load correctly
  when running from an uninstalled build.
- Fixed ALSA playback dropout after seek by priming the PCM device.
- Added files from the GTK2 file browser.

### Documentation and project hygiene
- Modernized `README` into a Markdown `README.md` with badges, accurate
  GTK2/GLib2 build instructions, contributing and license sections, and a
  prominent section honoring the original XMMS authors.
- Moved the full original user manual to `docs/manual.md` (Markdown), with
  dead `xmms.org` links annotated and redirected to the fork's GitHub Issues.
- Converted the original author credits to UTF-8 (fixing mojibake names),
  preserving every contributor and adding the fork maintainer.

### Verified
- `./configure && make -j"$(nproc)"` succeeds from a clean checkout.
- `make -C tests check` passes (entry navigation, output-plugin discovery, and
  ALSA PCM state tests).

## Original upstream — XMMS 1.2.11 (2007)

The last upstream XMMS release, on which this fork is based. See the archived
[upstream changelog and release notes](docs/history/README.md) for the full
history.

### Original authors
Created by **Peter Alm**, **Håvard Kvålen**, **Thomas Nilsson**, and
**Olle Hällnäs**, with contributions from many others listed in
[`CONTRIBUTORS.md`](CONTRIBUTORS.md).
