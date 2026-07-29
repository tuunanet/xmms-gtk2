# Changelog

All notable changes to this fork are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioned entries will follow
[Semantic Versioning](https://semver.org/spec/v2.0.0.html) when the fork begins
publishing tagged releases.

The detailed upstream history (1997–2007) is preserved under
[`docs/history/`](docs/history/README.md). This file is the canonical changelog
for XMMS Classic.

---

## [Unreleased]

## [1.2.12] - 2026-07-29

XMMS Classic 1.2.12 replaces the unpublished 1.3.1 and 1.3.2 draft versions
and is intended to be the first published XMMS Classic release. It continues
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
- Adopted the toolkit-neutral **XMMS Classic** project name and renamed the
  GitHub repository to `xmms-classic`, while retaining compatibility-sensitive
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
  length ([#16](https://github.com/tuunanet/xmms-classic/issues/16)).
- Fixed GTK popup menus opening at the screen's upper-left corner instead of
  their requested pointer position
  ([#12](https://github.com/tuunanet/xmms-classic/issues/12)).
- Fixed ALSA volume and balance controls when the configured hardware mixer is
  unavailable, as with PipeWire-backed default devices, by falling back to the
  plugin's software volume control
  ([#10](https://github.com/tuunanet/xmms-classic/issues/10)).
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
