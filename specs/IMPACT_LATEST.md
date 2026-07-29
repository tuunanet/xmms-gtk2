## Target

GitHub Actions package automation, `tools/build-deb.sh` environment contract, and packaging contract tests.

## Dependents (5)

- `Makefile.am`: invokes `tools/build-deb.sh` through `make deb`.
- `packaging/debian/*`: defines package contents and dependencies.
- `tests/test-package-recipes.sh`: guards package helper and workflow expectations.
- `README.md`: documents downloadable package names and targets.
- `docs/releases.md`: documents native package production and artifacts.

## Affected Stories

- Story e01s01: Produce Linux Mint 22.3 release artifacts.

## Test Coverage

- `tests/test-package-recipes.sh`: static contract for workflow trigger, target image, package revision, verification, and artifact contents.
- `make deb`: builds and tests both packages through the real Debian packaging boundary.
- Workflow smoke test: installs generated packages in the same Linux Mint target userspace.
- Gap: a local checkout cannot fully execute GitHub-hosted artifact upload behavior.

## Risk: Medium

The change is isolated from runtime code but affects release provenance and depends on an external container image and GitHub Actions execution.

## Recommended action

Proceed with pinned dependencies, explicit target verification, package installation, checksums, metadata, and a manual-only trigger.
