# Project Context

## Stack

- C application and plugins built with legacy GNU Autotools.
- GTK2/GLib2 desktop UI, ALSA output, and optional codec/visualization libraries.
- Debian packaging under `packaging/debian/`, driven by `make deb` and `tools/build-deb.sh`.
- Shell-based regression and packaging contract tests under `tests/`.
- GitHub Actions is the intended CI and release automation platform.

## Architecture

Runtime code is organized around the XMMS executable, `libxmms`, and dynamically loaded plugin families. Packaging is a distribution boundary: `make deb` creates a source archive, overlays the Debian recipes, runs `dpkg-buildpackage`, and emits runtime and development packages.

## Conventions (Observed)

- Preserve shipped generated Autotools files and historical runtime identifiers.
- Keep distribution-specific package metadata outside runtime code.
- Validate packaging recipes with `tests/test-package-recipes.sh`.
- Run GTK tests under Xvfb.
- Release artifacts are unsigned and accompanied by checksums and metadata.

## Signals / Active Considerations

- The published Ubuntu-labelled v1.2.12 package was built against libraries newer than its stated target and remains immutable.
- Distribution packages must be built inside the claimed target userspace and smoke-tested there.
- Manual release automation validates an annotated version tag, builds in pinned Linux Mint 22.3 and Ubuntu 26.04 amd64 containers, and grants `contents: write` only to draft Release assembly.
