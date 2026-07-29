# XMMS Classic Conventions

These rules apply to every human and AI-authored change.
Project compatibility outranks opportunistic modernization.

## Conventional Commits and Versioning

MUST follow Conventional Commits 1.0.0.
MUST use Semantic Versioning for release meaning.

```text
<type>(<scope>): <imperative description>
```

| Type | Purpose | Version effect |
| --- | --- | --- |
| `feat` | New compatible behavior | Minor |
| `fix` | Defect correction | Patch |
| `perf` | Compatible performance improvement | Patch |
| `docs`, `test`, `build`, `ci`, `refactor`, `chore`, `style` | Non-feature maintenance | None |
| `type!` or `BREAKING CHANGE:` | Intentional compatibility break | Major |

MUST keep commit titles concise and imperative.
MUST keep titles within 72 characters.
MUST explain motivation and migration details in the body.
NEVER add AI attribution footers.

## Git Workflow

MUST use `kickoff-branch` before implementation.
MUST NOT implement directly on `main`.
MUST treat remote `main` as protected.
MUST use the `team-pr` mode recorded in `specs/state.yaml`.
MUST merge every change through a pull request.
MUST run release gates before landing changes.
MUST use `commit-message` before creating a commit.
MUST use `release-branch` for integration decisions.
NEVER force-push or run destructive Git commands without explicit human approval.
NEVER create GitHub issues from automated workflows.
Write investigations and plans under `specs/`.

## Agent Workflow

MUST route each task through the matching bigpowers skill.
MUST use `survey-context` when lifecycle state is unclear.
MUST use `scope-work`, `slice-tasks`, and `plan-work` for feature planning.
MUST use `develop-tdd` or `execute-plan` for approved implementation.
MUST use `investigate-bug` for reported defects.
MUST use `verify-work` before review.
MUST run `audit-code` before requesting review.
MUST record planning output under `specs/`.
MUST attach runnable `verify` commands to implementation tasks.
MUST provide manual verification steps for user acceptance.
NEVER generate feature code without an approved plan.

## Always Green and Shift Left

Always Green means Preflight and applicable CI pass before forward work.
Fixing defects during development costs less than fixing them after release.

Run local Preflight:

```sh
make -j"$(nproc)" && xvfb-run --auto-servernum make check
```

Run the stricter distribution gate when release risk warrants it:

```sh
xvfb-run --auto-servernum make distcheck
```

MUST stop forward work when Preflight fails reproducibly.
MUST stop integration when applicable CI fails.
MUST capture command output as verification evidence.
NEVER claim a check passed without running it.

## Discovered Defects

A reproducible gate failure is a discovered defect.
Use this fix-or-log ladder:

1. Use `quick-fix` for trivial data-only corrections within its guardrails.
2. Use `fix-bug` when investigation or behavior changes are required.
3. Write a bug specification when reproduction remains blocked.
4. Stop forward work until the defect receives triage.

MUST separate discovered fixes into distinct Conventional Commits.
MUST include discovered fixes in the active delivery branch.
NEVER narrate a reproducible failure and continue.

### Banned dismissive phrases

| Banned phrase | Required response |
| --- | --- |
| Pre-existing issue | Reproduce, fix, or log the defect |
| Unrelated to this session | Reproduce, fix, or log the defect |
| Not introduced by this change | Prove through isolation, then fix or log |
| Out of scope while ignoring a red gate | Invoke `quick-fix` or `fix-bug` |

## Compatibility Contracts

MUST preserve `xmms/plugin.h` vtable layouts unless scope approves an ABI break.
MUST preserve exported `get_*plugin_info` symbols.
MUST preserve `libxmms` public APIs unless scope approves a break.
MUST preserve existing control-socket command values and packet framing.
MUST preserve `~/.xmms/` paths and established configuration keys.
MUST preserve WinAmp 2 and XMMS skin behavior.
MUST preserve source-tarball, plugin-loading, and Debian packaging workflows.
MUST audit external plugins before changing compatibility facades.
NEVER renumber existing `CMD_*` constants.
NEVER simplify the pseudo-effect facade without auditing output plugins.

## C Style and Ownership

MUST follow nearby C formatting and naming.
MUST use subsystem-prefixed `snake_case` names.
MUST make ownership and lifetime visible near allocations.
MUST pair GLib allocation with the matching release operation.
MUST initialize structs and shared state explicitly.
MUST check optional plugin callbacks before invocation.
MUST use named constants for non-obvious protocol and timing values.
MUST prefer early returns over deeper nesting.
MUST keep changes narrow within historical modules.
NEVER perform unrelated formatting across legacy files.
NEVER delete comments that explain historical compatibility or intent.
NEVER edit generated Autotools files without updating their source definitions.

Existing large historical modules are documented architecture constraints.
The generic 300-line file rule does not apply retroactively.
New modules MUST remain focused and reasonably sized.

## Error Handling

MUST follow each subsystem's established return-value convention.
MUST include actionable context in new diagnostics.
MUST use GLib logging consistently within GTK-facing code.
MUST report user-facing GTK errors on the main thread.
MUST release resources on every new error path.
MUST preserve established EOF and output-failure sentinels.
NEVER introduce fatal `g_error` for recoverable runtime failures.
NEVER hide device, decoder, network, or plugin-load failures silently.

## Threading

MUST keep GTK operations on the GTK main thread.
MUST document ownership for new worker-thread state.
MUST guard shared mutable lists with their established locks.
MUST release `PL_LOCK` before slow metadata or network I/O.
MUST reacquire locks before mutating shared playlist entries.
MUST verify entry lifetime after unlocked work.
MUST keep visualization callbacks fast.
MUST define bounded shutdown behavior for new worker threads.
NEVER call GTK from decoder, output, metadata, or socket-reader threads.
NEVER add blocking I/O to `idle_func`.

## Tests

Tests MUST remain Fast, Independent, Repeatable, Self-Validating, and Timely.
MUST add a regression test for each practical bug fix.
MUST test new behavior through observable interfaces.
MUST prefer focused GLib `g_test` executables.
MUST reuse the source-slice test pattern when full UI startup adds no value.
MUST use fixture plugins for plugin-discovery behavior.
MUST cover boundary values and failure paths.
MUST run GTK tests under Xvfb in headless environments.
NEVER skip tests without a documented unresolved ambiguity.
NEVER depend on test execution order.

## Build and Generated Files

MUST treat `configure.in` and `Makefile.am` as source definitions.
MUST regenerate expected distribution artifacts through established tools.
MUST distinguish tracked generated sources from local build products.
MUST run plugin-linkage checks after plugin build changes.
MUST run packaging checks after install-path or package changes.
MUST run release-tool checks after version or changelog changes.
NEVER commit local binaries, object files, package artifacts, or Repomix output.

## Documentation

MUST update `docs/architecture/` when public architecture changes.
MUST update `specs/tech-architecture/tech-stack.md` when stack facts change.
MUST write comments that explain why.
MUST reference an ADR, issue, or commit for surprising compatibility logic.
MUST keep public contract documentation synchronized with code.
NEVER duplicate lifecycle status across specification files.

## Defensive Code

Only these defensive categories are enabled:

### Timeout

MUST bound new control-socket and network waits.
MUST expose timeout failures through existing diagnostics.

### Retry

MUST use bounded retries for transient device and network failures.
MUST stop retrying after a defined limit or deadline.
MUST avoid busy loops between attempts.

### Graceful degradation

MUST disable unavailable optional plugins or features without crashing.
MUST preserve core playback when an optional integration fails.
MUST explain degraded behavior through existing diagnostics.

Rate limiting and circuit breakers are not project-wide requirements.
Add them only through explicit approved scope.

## Hard Stops

NEVER break historical compatibility unintentionally.
NEVER replace GTK2 or Autotools without an approved migration plan.
NEVER edit historical codec code without explicit task scope.
NEVER perform GTK work from worker threads.
NEVER hold playlist locks across slow I/O.
NEVER ignore reproducible Preflight or CI failures.
NEVER expose credentials, private data, or signing material.
