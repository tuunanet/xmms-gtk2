# Security Plan

## Release boundary

- Pin GitHub Actions and container images for package workflows.
- Grant `contents: write` only to draft-release assembly.
- Re-verify downloaded package checksums before draft assembly.
- Never modify published GitHub Releases.

## Runtime boundary

- Preserve plugin vtable layouts and exported entry symbols.
- Preserve control-socket command values and packet framing.
- Keep GTK calls on the main thread.
- Keep playlist locks outside slow metadata and network I/O.

Current security-review evidence is under `specs/security/`.
