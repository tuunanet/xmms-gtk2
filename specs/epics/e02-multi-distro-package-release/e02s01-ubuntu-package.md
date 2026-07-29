# Story e02s01: Produce Ubuntu 26.04 and Linux Mint 22.3 package artifacts

## 1. Type

Feature

## 2. Status

Planned

## 3. Context

The manual workflow currently proves only Linux Mint 22.3 compatibility. A release needs independently built packages for Ubuntu 26.04 without weakening the established Mint package contract.

## 4. User

A maintainer preparing native XMMS packages for current Mint and Ubuntu systems.

## 5. Need

Receive install-tested, honestly named package artifacts from each target userspace.

## 6. Outcome

One workflow run produces verified Linux Mint 22.3 and Ubuntu 26.04 artifact sets.

## 7. Scope

The existing workflow, package contract tests, target documentation, and distribution-specific metadata.

## 8. Dependencies

GitHub Actions, the existing Mint image, the official Ubuntu 26.04 image, Debian package tooling, and `make deb`.

## 9. Purpose / Callers / Contracts

The workflow is the hosted package-build orchestrator. Maintainers call it through `workflow_dispatch`; release assembly consumes its uploaded artifacts. It must preserve manual triggering, immutable environment pins, exact target naming, package installability, checksums, and the existing `xmms`/`libxmms-dev` package identities.

## 10. Prior Art

| Candidate | Source | Verdict | Notes |
|---|---|---|---|
| Existing Mint package job | `.github/workflows/release-linux-packages.yml` | extend | Reuse its build, package inspection, install, smoke-test, and checksum gates. |
| Existing package helper | `tools/build-deb.sh` | compose | Already accepts `DEB_DISTRIBUTION` and `DEB_REVISION`. |
| Official Ubuntu image | `ubuntu:26.04`, amd64 manifest `sha256:7c2884fd32770fc6c173b78e0dc2278a2851d89f5447919edbc45475ac55dd6a` | adopt | Image identifies Ubuntu 26.04 LTS Resolute and ships glibc 2.43. |
| Hosted `ubuntu-26.04` runner | GitHub-hosted runners | reject | Container pinning gives an explicit target userspace independent of runner-label rollout. |

External dependency slopcheck: official Ubuntu image [OK], `actions/checkout` [OK], `actions/upload-artifact` [OK].

## 11. Reason for Depth

A two-entry build matrix removes duplicated release logic while retaining explicit target identity and naming parameters.

## 12. Security

Pin the Ubuntu amd64 image by manifest digest, retain read-only repository permissions in build jobs, and verify `/etc/os-release` before installing dependencies.

## 13. Failure Handling

Either target fails before upload when image identity, release metadata, package build, package fields, installation, smoke test, or checksums disagree.

## 14. Observability

Each target emits distinct package metadata recording image, source SHA, target release, architecture, and Debian control fields.

## 15. Compatibility

The Mint package bytes and naming contract remain target-specific. Ubuntu packages use their own revision so package-manager ordering remains valid while downloadable names remain filesystem-friendly.

## 16. Requirements

### MODIFIED: Manual package build targets

**Before:** The workflow builds only Linux Mint 22.3 Zena amd64 packages.

**After:** The workflow builds both Linux Mint 22.3 Zena amd64 and Ubuntu 26.04 Resolute amd64 packages, and both variants must pass before downstream release assembly.

### ADDED: Honest Ubuntu package identity

Ubuntu package control versions use `1~ubuntu26.04`, while downloadable DEB filenames use `.ubuntu26.04_amd64` and metadata identifies the digest-pinned Ubuntu image.

## 17. Acceptance Criteria

1. The existing Mint variant continues to produce and test both packages.
2. The Ubuntu variant runs in the pinned official Ubuntu 26.04 amd64 image and verifies Resolute identity.
3. Ubuntu control versions end in `1~ubuntu26.04` and downloadable names contain `.ubuntu26.04_amd64`.
4. Both variants install runtime and development packages and verify `xmms --version` and `plugin.h`.
5. Both artifact sets include package checksums and target metadata; the Mint set remains the canonical source-archive carrier.

## 18. Implementation Steps

1. Add failing static contracts for Ubuntu image, identity, revision, names, and two-target gating → verify: `tests/test-package-recipes.sh .`
2. Extend the package job into two pinned target variants with shared validation → verify: `tests/test-package-recipes.sh . && python3 -c "import yaml; yaml.safe_load(open('.github/workflows/release-linux-packages.yml'))" && actionlint .github/workflows/release-linux-packages.yml`
3. Document the new Ubuntu package target → verify: `grep -q 'Ubuntu 26.04' README.md && grep -q 'Ubuntu 26.04' docs/releases.md && grep -q 'Ubuntu 26.04' docs/architecture/build-and-test.md`

## 19. Manual Verification

1. Dispatch the workflow on a new annotated `vVERSION` tag contained in `main`.
2. Confirm separate Mint and Ubuntu package jobs complete.
3. Download both intermediate artifacts before publishing the draft.
4. Run each target's `sha256sum --check SHA256SUMS`.
5. Inspect control versions and install each package pair on its target distribution.

## 20. Out of Scope / Risks

No cross-distribution claim is inferred from one build. The principal risk is image or package-repository drift; immutable image pins and explicit target checks fail closed.
