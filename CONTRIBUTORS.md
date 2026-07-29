# Contributors

XMMS was created in 1997, and upstream development ended with version 1.2.11
in 2007. This repository preserves the original work and later modernization
efforts, but it is not actively maintained.

Historical `@xmms.org` addresses are no longer active. Issues and pull requests
in this repository may not be reviewed. Anyone interested in continuing the
project is encouraged to create and maintain an independent open-source fork.

## Repository import and GTK2 port (2015)

- Oleg Pudeyev — imported XMMS 1.2.11 into Git, applied downstream FreeBSD
  compatibility patches, and created the initial GTK2 / GLib2 port preserved
  in this repository's history.

## Repository maintenance (2026)

- **[Tuomo Tuunanen](https://github.com/tuunanet)** — completed GTK2
  compatibility work; fixed modern compiler, build, plugin, and ALSA runtime
  issues; and maintained the regression suite, source distributions,
  documentation, and GitHub project infrastructure.

### July 2026 contributions

Tuomo Tuunanen:

- Restored GTK keyboard shortcuts by forwarding unhandled main-window key
  events to the accelerator dispatcher.
- Fixed `make deb` source archive assembly after project-local reviewer and CI
  files were removed.
- Made Debian package builds work with GCC 15 by allowing the legacy GTK
  callback conversions required by XMMS's GTK2 code.
- Linked the MP3 input plugin directly to its required math libraries so it
  loads reliably from native packages.
- Corrected playlist durations for VBR MP3 files without Xing headers by
  scanning MPEG frames instead of extrapolating from the first frame's bitrate.
- Fixed MP3 seeking near the end of tracks by preserving fractional Xing seek
  percentages, anchoring offsets at the first MPEG frame, and honoring known
  stream lengths.
- Fixed ALSA playback dropping out after a seek by priming prepared PCM devices
  before polling resumes.
- Restored volume and balance controls when an ALSA hardware mixer is
  unavailable, including PipeWire-backed defaults, by falling back to software
  volume control.
- Migrated unusable OSS defaults to ALSA, preferred an available ALSA output
  plugin, and fixed plugin discovery when running from an uninstalled build.
- Fixed GTK popup and playlist hold-menu positioning so menus open at the
  requested pointer or button coordinates instead of the screen's upper-left
  corner under XWayland.
- Restored clear, byte-compatible classic playlist and main-window fonts using
  dependable `fixed`-family bitmap fonts, including migration from short-lived
  replacement defaults.
- Prevented source-distribution failures caused by modern Bison regenerating
  the bundled gettext plural parser with incompatible output.
- Fixed clean-checkout configuration by shipping the required `mkinstalldirs`
  helper and removed obsolete Autoconf `datarootdir` warnings.
- Fixed `make distcheck` and out-of-tree builds by correcting the bundled
  gettext `getcwd()` declaration and public-header resolution in `libxmms`.
- Restored GTK2 entry word navigation, including UTF-8-aware Alt+F and Alt+B
  movement in read-only entry widgets.
- Corrected remaining GTK2 and GCC 15 compatibility boundaries, including
  callback signatures, C23's reserved `bool` identifier, `fts_open()` typing,
  X11 atom conversions, pointer grabs, and accelerator activation.
- Restored adding files through the GTK2 file browser and replaced the removed
  GTK1 joystick layout container with its GTK2 equivalent.

The GitHub fork relationship records repository hosting lineage. The 2026
maintenance work, historical GTK2 porting, and original XMMS authorship are
separate contributions, all preserved here and in the git history.

## Original XMMS authors and contributors (1997–2007)

| Contribution | People |
| --- | --- |
| Main programming | Peter Alm |
| Additional programming | Håvard Kvålen; Derrik Pates |
| Default skin | Leonard “Blayde” Tan; Robin Sylvestre (equalizer and playlist); Thomas Nilsson (new titles and cleanups) |
| Homepage and graphics | Thomas Nilsson |
| Support and documentation | Olle Hällnäs |

### Additional contributors

- Sean Atkinson
- Jorn Baayen
- James M. Cape
- Anders Carlsson — effect plugins
- Chun-Chung Chen — X font patch
- Tim Ferguson — joystick plugin
- Ben Gertzfield
- Vesa Halttunen
- Logan Hanks
- Eric L. Hernes — FreeBSD patches
- Ville Herva
- Ian “Hixie” Hickson
- higway — MMX
- Michael Hipp and others — MPG123 engine
- Olle Hällnäs — compilation fixes
- David Jacoby
- Osamu Kayasono — 3DNow!
- Lyle B Kempler
- J. Nick Koston — MikMod plugin
- Aaron Lehmann
- Johan Levin — echo and stereo plugins
- Eric Lindvall
- Colin Marquardt
- Willem Monsuwe
- John Riddoch — Solaris plugin
- Josip Rodin
- Pablo Saratxaga — internationalization
- Carl van Schaik — Pro Logic plugin
- Jörg Schuler
- Charles Sielski — IRman plugin
- Espen Skoglund
- Matthieu Sozeau — ALSA plugin
- Kimura Takuhiro — 3DNow!
- Zinx Verituse
- Ryan Weaver — RPMs and other work
- Chris Wilson
- Dave Yearke
- Stephan K. Zitz
