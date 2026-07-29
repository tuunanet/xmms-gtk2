**Must-fix:** None. **Zero must-fix findings.**

**Should-fix**
- `AGENTS.md:21` defines Preflight as build + Xvfb tests + lint, while `CONVENTIONS.md:64-68` defines local Preflight as build + Xvfb tests only. Align the commands or name the stricter command separately; the current wording makes the mandatory gate ambiguous.

**Consider**
- `AGENTS.md:72` newly routes agents to `specs/state.yaml`, but `specs/state.yaml:28` names `feat/ubuntu-mint-github-release` while the verified/current branch is `heal-bigpowers-initialization` (`specs/verifications/initialization-verify.yaml:8`). Refresh or clarify this transient field to avoid misleading lifecycle handoffs.
- `specs/execution-status.yaml` is empty although `state.yaml` and `release-plan.yaml` record active/implemented e02 lifecycle data. Clarify the intended source of truth before agents begin consuming the new scaffold, consistent with `CONVENTIONS.md:191`.

**Fowler smells:** None identified.

**Quality score:** 93/100 — configuration is valid, OpenCode resolves `AGENTS.md`, policies are safe and draft-only, and no secrets or unsafe automation were added. Minor operational-documentation inconsistencies remain.
