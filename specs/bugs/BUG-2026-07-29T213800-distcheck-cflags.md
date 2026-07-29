---
bug_id: BUG-2026-07-29T213800-distcheck-cflags
status: resolved
severity: medium
priority: P1
scope: build
---

# BUG-2026-07-29T213800: distcheck drops test compiler flags

## Problem

A release `make distcheck` invocation that supplies compatibility `CFLAGS`
failed while compiling source-tree GTK tests. The distribution build must accept
caller-supplied compiler flags while retaining the test-specific include and
compile options required by those tests.

**Security impact: NONE.** No security exploit path was identified.

## Root Cause Analysis

1. **Reproduce:** `xvfb-run --auto-servernum make distcheck CFLAGS='-g -O2 -Wno-error=incompatible-pointer-types'` failed to locate GTK/GLib headers when compiling tests.
2. **Isolate:** The source-tree test Makefile appended GTK flags with ordinary make assignments.
3. **Hypothesize:** GNU make gives a command-line `CFLAGS` assignment precedence over ordinary Makefile assignments, so the required GTK flags were omitted.
4. **Verify:** A characterization test inspected the test compile command with caller-supplied `CFLAGS` and failed before the change. Applying override appends restored GTK and target-specific flags; the characterization test and complete distcheck passed.

**Risk level: Medium.** The defect blocks release-quality source distribution verification but does not change runtime behavior.

## TDD Fix Plan

1. **RED:** Assert that a source-tree test compile command retains GTK compiler flags when `CFLAGS` is provided by the caller.
   **GREEN:** Make the shared GTK flag append override a command-line `CFLAGS` assignment.
   **verify:** `tests/test-release-tools.sh .`

2. **RED:** Exercise distribution validation with compatibility `CFLAGS`.
   **GREEN:** Apply the same override behavior to target-specific test compiler options.
   **verify:** `xvfb-run --auto-servernum make distcheck CFLAGS='-g -O2 -Wno-error=incompatible-pointer-types'`

**REFACTOR:** No new abstraction is needed; this preserves the existing flat test Makefile while making its compiler-flag contract explicit.

## Acceptance Criteria

- [x] Source-tree GTK tests retain their required compiler flags with caller-supplied `CFLAGS`.
- [x] Target-specific test flags are also preserved.
- [x] The characterization test passes.
- [x] The compatibility-flag distcheck passes.

## Resolution

Resolved by changing the relevant test Makefile appends to explicit override appends and adding the release-tools characterization test. Verified in release branch commits `2c011a4` and `1e3312a`.
