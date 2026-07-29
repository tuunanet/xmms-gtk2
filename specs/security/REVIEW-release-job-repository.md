# Security Review: checkout-independent release repository selection

- Reviewed branch: `fix/release-job-repository`
- Diff base: `origin/main`
- Result: **PASS** — no reportable findings.

`GH_REPO` receives GitHub's trusted `github.repository` context, not a user-provided
workflow input. It only gives the existing authenticated `gh release` commands an
explicit repository in the artifact-only job; it adds no command interpolation,
permission, secret, or artifact-integrity change.
