# Story e02s02: Assemble package artifacts into a draft GitHub Release

## 1. Type

Feature

## 2. Status

Planned

## 3. Context

The existing package workflow stops at expiring Actions artifacts. Maintainers need one reviewable GitHub Release containing both target package sets and the source/provenance assets, without allowing the workflow to rewrite published history.

## 4. User

A maintainer finalizing an annotated XMMS version tag.

## 5. Need

Receive a complete draft Release only after every distribution package gate passes.

## 6. Outcome

The matching version tag gains a draft GitHub Release with verified multi-distribution assets ready for manual publication.

## 7. Scope

Tag validation, artifact download and integrity verification, release-asset assembly, least-privilege draft creation, rerun behavior, and release documentation.

## 8. Dependencies

GitHub Actions artifact service, `actions/download-artifact`, the GitHub CLI release API, changelog release tooling, and both package variants from e02s01.

## 9. Purpose / Callers / Contracts

The final job is the sole release-write boundary. It is called only by successful validation and package jobs. It must consume artifacts from the same run, verify checksums again, bind the release to a matching annotated tag contained in `main`, remain draft-only, and reject any attempt to mutate a published release.

## 10. Prior Art

| Candidate | Source | Verdict | Notes |
|---|---|---|---|
| GitHub CLI `release create` | `gh release create --help` | adopt | `--verify-tag` aborts if the tag is absent; `--draft` creates a reviewable release; `--notes-file` supplies curated notes. |
| `actions/download-artifact` v8 | `actions/download-artifact`, commit `3e5f45b2cfb9172054b4087a40e8e0b5a5461e7c` | adopt | Downloads named artifacts from the current workflow run and is pinned to the v8 commit. |
| Existing changelog extraction | `tools/extract-release-notes.sh` | compose | Produces the exact version section used as draft notes and an attached asset. |
| Existing release policy | `docs/releases.md` | extend | Preserve annotated tags, drafts, manual publication, and immutable published releases. |

External dependency slopcheck: GitHub CLI [OK], `actions/download-artifact` [OK]. No third-party release action is needed.

## 11. Reason for Depth

A separate final job creates a narrow permission boundary: build code remains read-only and only verified fan-in receives release-write access.

## 12. Security

Grant `contents: write` and `actions: read` only to the final job. Validate the selected annotated tag and main ancestry before any build. Re-check artifact checksums after download and reject non-draft existing releases.

## 13. Failure Handling

The final job is skipped if validation or either package variant fails. It fails closed on missing files, checksum mismatches, incorrect tags, published releases, or GitHub API errors. Reruns may replace assets only on the same draft.

## 14. Observability

A consolidated release metadata file records the source SHA, selected tag, workflow URL, and both build image digests. Draft notes come from the matching changelog entry.

## 15. Compatibility

The release uses the existing `vMAJOR.MINOR.PATCH` identity and preserves manual publication. Existing published releases, especially `v1.2.12`, are never modified.

## 16. Requirements

### MODIFIED: Package workflow publication boundary

**Before:** A successful manual run uploads one expiring Linux Mint Actions artifact and has repository contents read permission only.

**After:** A matching annotated tag triggers two package variants; after both succeed, a separate least-privilege job verifies their downloaded outputs and creates or resumes a draft GitHub Release containing all release-relevant assets.

### ADDED: Published-release immutability guard

The workflow may repair a draft on rerun but must fail before uploading when a release for the tag is already published.

## 17. Acceptance Criteria

1. Dispatch rejects branches, lightweight tags, mismatched versions, tags outside `main`, and published existing releases before package builds.
2. The final job depends on validation and the complete two-target package matrix.
3. Both downloaded target checksum manifests are verified before copying files.
4. Final `SHA256SUMS` covers four DEBs, one source archive, target metadata, package checksum manifests, release notes, and consolidated release metadata.
5. Only the final job receives `contents: write`; build jobs remain read-only.
6. The workflow creates a draft for a new tag, safely updates the same draft on rerun, and never publishes it.

## 18. Implementation Steps

1. Add failing contracts for annotated-tag validation, fan-in dependencies, write-permission isolation, checksum re-verification, and draft-only release commands → verify: `tests/test-package-recipes.sh .`
2. Add validation and final assembly jobs using pinned download action and GitHub CLI → verify: `tests/test-package-recipes.sh . && python3 -c "import yaml; yaml.safe_load(open('.github/workflows/package-linux-mint.yml'))" && actionlint .github/workflows/package-linux-mint.yml`
3. Synchronize release architecture and maintainer instructions → verify: `grep -q 'draft GitHub Release' docs/releases.md && grep -q 'SHA256SUMS' README.md && grep -q 'contents: write' docs/architecture/build-and-test.md`

## 19. Manual Verification

1. Prepare a new version in `configure.in`, generated `configure`, and `CHANGELOG.md`; merge it to `main`.
2. Create and push an annotated `vVERSION` tag on that main commit.
3. Dispatch this workflow for the tag and matching version.
4. Confirm validation runs first, both package variants pass, and final release assembly runs last.
5. Open the new draft Release, download all assets, and run `sha256sum --check SHA256SUMS`.
6. Inspect notes and metadata, test both package pairs, then publish manually.

## 20. Out of Scope / Risks

No automatic publication, tag creation, signing, or mutation of published assets. Full GitHub Release API behavior can only be proven on the next new version tag, so local validation and the first live run require maintainer scrutiny.
