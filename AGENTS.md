# XMMS GTK2 — AI Agents

Read `CONVENTIONS.md` before Git, GitHub, or implementation work.

<!-- BEGIN bigpowers:project -->
## Project

XMMS is a Linux multimedia audio player built with C and GTK2.
It supports classic WinAmp skins, playlists, equalization, and dynamically loaded plugins.
Stack: C, GTK2, GLib2, GNU Autotools, libtool, and GitHub Actions.

## Commands

| Action | Command |
| --- | --- |
| Configure | `./configure` |
| Run | `./xmms/xmms` |
| Build | `make -j"$(nproc)"` |
| Test | `xvfb-run --auto-servernum make check` |
| Lint | `make lint` |
| Preflight | `make -j"$(nproc)" && xvfb-run --auto-servernum make check && make lint` |
| Distribution gate | `xvfb-run --auto-servernum make distcheck` |
| Package build | `make deb` |
| CI checks | `gh pr checks` |

## Architecture

`xmms/` contains GTK2 UI code and playback glue.
`libxmms/` provides shared helpers and Unix-socket remote control.
Input, Output, Effect, Visualization, and General plugins load dynamically.
`tests/`, `packaging/`, and `tools/` protect build and release boundaries.

## Conventions

- Use Conventional Commits: `type(scope): imperative description`.
- Follow nearby C style and subsystem-prefixed `snake_case` names.
- Preserve historical compatibility before opportunistic modernization.
- Record plans, decisions, and investigations under `specs/`.
- Run the relevant checks after every change.

## Never

- Never commit directly to `main`.
- Never force-push, rewrite shared history, delete release tags, or alter published releases.
- Never break plugin vtable ABI, exported symbols, socket framing, configuration paths, or skin compatibility.
- Never replace GTK2, Autotools, or historical modules without approved scope.
- Never perform GTK work from background threads.
- Never hold playlist locks across slow I/O.
- Never ignore reproducible build, lint, test, CI, or distribution-gate failures.
- Never commit credentials, binaries, package artifacts, or generated local build output.
- Never publish GitHub Releases automatically.

## Agent Rules

- MUST use the matching bigpowers skill for every task.
- MUST run `survey-context` when lifecycle state is unclear.
- MUST use `kickoff-branch` before implementation.
- MUST write an approved plan before feature code.
- MUST use `develop-tdd` or `execute-plan` for approved implementation.
- MUST run `verify-work` before review.
- MUST run `audit-code` before requesting review.
- MUST use `commit-message` before committing.
- MUST use `release-branch` for integration decisions.
- MUST stop forward work on reproducible red Preflight or CI.
<!-- END bigpowers:project -->

<!-- BEGIN bigpowers:context-routing -->
## Context Routing

| Path | Read first |
| --- | --- |
| `specs/` | `specs/state.yaml` and relevant epic or bug specification |
| `xmms/` | `docs/architecture/ui-interaction.md` |
| `Input/`, `Output/`, `Effect/`, `Visualization/`, `General/` | `docs/architecture/plugin-system.md` and `docs/architecture/processing-pipeline.md` |
| `libxmms/`, `wmxmms/` | `docs/architecture/external-control.md` |
| `packaging/`, `tools/`, `.github/` | `docs/architecture/build-and-test.md` and `docs/releases.md` |
<!-- END bigpowers:context-routing -->

<!-- BEGIN bigpowers:learned-preferences -->
## Learned User Preferences

## Workspace Facts
<!-- END bigpowers:learned-preferences -->
