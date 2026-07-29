## Summary

<!-- What problem does this solve, and why is this approach appropriate? -->

## Verification

- [ ] `./configure --disable-esd`
- [ ] `make -j"$(nproc)"`
- [ ] `make check`
- [ ] Relevant manual runtime testing (describe below), or not applicable

## Project checks

- [ ] The change is focused and does not reformat unrelated code.
- [ ] Bug fixes include a regression test where practical.
- [ ] User-visible changes are documented under `CHANGELOG.md` → `Unreleased`.
- [ ] No generated build artifacts or local configuration files are included.

## Compatibility notes

<!-- Platforms, plugins, formats, packaging, or classic behavior affected. -->
