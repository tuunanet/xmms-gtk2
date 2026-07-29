# Story e01s01: Produce Linux Mint 22.3 release artifacts

## 1. Type

Feature

## 2. Status

Implemented and locally verified

## 3. Context

The published Ubuntu-labelled package was linked against a newer userspace than its name claimed. The repository also lacks the release workflow described by its documentation.

## 4. User

A maintainer preparing a Linux Mint-compatible XMMS package.

## 5. Need

Build and identify packages using the actual target distribution userspace.

## 6. Outcome

A manual workflow run returns reviewable Linux Mint 22.3 package artifacts.

## 7. Scope

Workflow, packaging contract test, and release/install documentation.

## 8. Dependencies

GitHub Actions, the Linux Mint project's Mint 22.3 container image, Debian packaging tools, and the existing `make deb` target.

## 9. Purpose / Callers / Contracts

`tools/build-deb.sh` turns a source archive and Debian recipes into two binary packages. It is called by `Makefile.am` and the new workflow. Its contracts are exact source versioning, two-package output, lintian success, and MP3 plugin linkage validation.

## 10. Prior Art

| Candidate | Source | Verdict | Notes |
|---|---|---|---|
| Linux Mint CI image | `linuxmintd/mint22.3-amd64` | adopt | Used by Linux Mint's own Cinnamon CI; pin by digest. |
| Linux Mint reusable build workflows | `linuxmint/github-actions` | extend | Confirms container-job pattern, but is broader than this project needs. |
| Existing `make deb` path | `Makefile.am`, `tools/build-deb.sh` | compose | Reuse with Mint distribution and revision environment variables. |
| Obsolete community Mint image | `vcatechnology/linux-mint` | reject | Last current tag targets Mint 18. |

External package slopcheck: Linux Mint image [OK], `actions/checkout` [OK], `actions/upload-artifact` [OK].

## 11. Reason for Depth

No new abstraction is needed; the workflow composes existing package commands directly.

## 12. Security

Use read-only repository permissions and immutable pins for the container image and actions.

## 13. Failure Handling

Fail before upload if the target identity, project version, package metadata, installation, smoke test, or checksum generation fails.

## 14. Observability

`PACKAGE-METADATA.txt` records image, target release, source commit, and Debian control fields.

## 15. Compatibility

The runtime package remains `xmms`, the development package remains `libxmms-dev`, and historical runtime paths are unchanged.

## 16. Requirements

### ADDED: Manual Linux Mint package build

A `workflow_dispatch` workflow builds amd64 packages in Linux Mint 22.3 Zena and uploads reviewable release artifacts.

### ADDED: Honest artifact identity

The Debian revision and uploaded DEB filenames identify Linux Mint 22.3 rather than Ubuntu.

### ADDED: Provenance and integrity

The result includes source, SHA-256 sums, and package/build metadata.

## 17. Acceptance Criteria

1. The workflow has no automatic trigger.
2. It runs in the pinned Linux Mint 22.3 image and verifies the target identity.
3. The requested SemVer matches `configure.in`, `configure`, and `CHANGELOG.md`.
4. Both packages build, install, and pass an XMMS version smoke test.
5. Artifact filenames contain `linuxmint22.3_amd64`.
6. The upload contains two DEBs, one source tarball, checksums, and metadata.

## 18. Implementation Steps

1. Add failing packaging contract checks for the workflow → verify: `tests/test-package-recipes.sh .`
2. Add the pinned manual workflow and artifact verification → verify: `tests/test-package-recipes.sh . && python3 -c "import yaml; yaml.safe_load(open('.github/workflows/release-linux-packages.yml'))"`
3. Update public package documentation → verify: `grep -q 'Linux Mint 22.3' README.md && grep -q 'Linux Mint package' docs/releases.md`

## 19. Manual Verification

1. Open Actions and select “Linux Mint package”.
2. Run it for a version matching the selected branch.
3. Download `xmms-VERSION-linuxmint22.3-amd64`.
4. Run `sha256sum -c PACKAGES-SHA256SUMS`.
5. Inspect `PACKAGE-METADATA.txt` and install both DEBs on Linux Mint 22.3.

## 20. Out of Scope / Risks

No LMDE, Mint 21, automatic release publication, signing, or replacement of published v1.2.12 assets. The main risk is upstream image drift, mitigated by a digest pin and explicit target check.
