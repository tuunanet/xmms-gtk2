# Test Plan

## Local gates

1. Build with `make -j"$(nproc)"`.
2. Run GTK tests with `xvfb-run --auto-servernum make check`.
3. Run Cppcheck with `make lint`.
4. Run `xvfb-run --auto-servernum make distcheck` for distribution-risk changes.

## Test design

- Use focused GLib `g_test` executables for practical regressions.
- Use shell contract tests for packaging, release tools, and generated files.
- Reuse fixture plugins for plugin-discovery behavior.
- Test observable behavior, boundaries, and failure paths.
