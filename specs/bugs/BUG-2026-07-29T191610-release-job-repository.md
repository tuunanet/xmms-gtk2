---
bug_id: BUG-2026-07-29T191610-release-job-repository
status: resolved
severity: high
priority: P1
scope: release
---

# BUG-2026-07-29T191610: release assembly cannot select its repository

## Problem

The hosted v1.2.13 workflow successfully built and verified both package targets, but
`Create draft GitHub Release` failed before creating a draft. The artifact-only job
intentionally does not check out the repository, and `gh release` attempted to infer
its repository from a non-existent `.git` directory.

**Security impact: NONE.** The failure occurred before any release mutation.

## Root Cause Analysis

1. **Reproduce:** Run `gh release view` from a directory without `.git`; it fails
   unless a repository is supplied.
2. **Isolate:** `create-release` downloads artifacts only and has no checkout step,
   while all `gh release` commands rely on implicit repository discovery.
3. **Hypothesize:** Setting `GH_REPO` from GitHub's trusted `github.repository`
   context lets `gh` target the correct repository without a checkout.
4. **Verify:** From a temporary non-Git directory,
   `GH_REPO=tuunanet/xmms-gtk2 gh release view v1.2.12` succeeded. The workflow
   contract test failed before the environment setting and passed afterward.

**Risk level: High.** The defect blocks every first draft-release creation after
otherwise successful package builds.

## TDD Fix Plan

1. **RED:** Require the release workflow to declare a checkout-independent GitHub
   repository selection setting.
2. **GREEN:** Add `GH_REPO: ${{ github.repository }}` to the `create-release` job
   environment so every `gh release` invocation uses the intended repository.
3. **VERIFY:** Run package-workflow contract checks, YAML parsing, `actionlint`, and
   dispatch the tagged v1.2.13 workflow end to end.

## Acceptance Criteria

- [x] The release job explicitly selects its GitHub repository without checking out source.
- [x] The workflow contract test covers that selection.
- [x] `gh release` works from a directory with no `.git` when `GH_REPO` is set.
- [ ] The rerun creates v1.2.13 as a draft release after artifact verification.
