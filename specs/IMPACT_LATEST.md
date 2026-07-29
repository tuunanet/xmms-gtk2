## Target

`.github/workflows/package-linux-mint.yml`, its artifact contract, and the GitHub Release publication boundary.

## Dependents (8)

- `Makefile.am` and generated `Makefile.in`: distribute the workflow in source archives.
- `tools/build-deb.sh`: supplies the shared distribution/revision-aware package build boundary.
- `packaging/debian/*`: defines package contents, dependencies, and install paths for both targets.
- `tests/test-package-recipes.sh`: guards trigger, image, package naming, permissions, and artifact behavior.
- `README.md`: documents downloadable package targets and filenames.
- `docs/releases.md`: defines tag, draft-release, immutability, and maintainer review policy.
- `docs/architecture/build-and-test.md`: documents package automation architecture.
- `specs/epics/e01-linux-mint-package/*`: owns the existing Linux Mint behavior that must remain intact.

## Affected Stories

- Story e01s01: Produce Linux Mint 22.3 release artifacts (regression surface).
- Story e02s01: Produce Ubuntu 26.04 release artifacts.
- Story e02s02: Assemble verified package artifacts into a draft GitHub Release.

## Test Coverage

- `tests/test-package-recipes.sh`: static workflow contract and regression coverage.
- `make deb`: real Debian package build, lintian, linkage, and Xvfb-backed test boundary.
- Each workflow build job: target identity, package metadata, installation, version, and checksum checks.
- Final workflow job: re-verifies downloaded checksums before release assembly.
- Gap: local tests cannot exercise GitHub-hosted artifact transport or create a real draft Release.
- Gap: end-to-end release creation requires a new matching annotated version tag; the immutable `v1.2.12` release cannot be reused for a test run.

## Risk: High

The runtime code is unaffected, but the workflow gains `contents: write` in a final job and can create immutable release history; strict tag validation, draft-only creation, pinned dependencies, and artifact re-verification are required.

## Recommended action

Proceed with a digest-pinned Ubuntu 26.04 amd64 image, preserve the tested Mint path, require a matching annotated tag contained in `main`, grant write permission only to the final job, create or update drafts only, and block mutation of published releases.
