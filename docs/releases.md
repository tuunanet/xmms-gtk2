# Release process

XMMS GTK2 uses deliberate, manually tested releases. Merging a pull request
to `main` runs CI but does not publish a release. Short-lived `release/*`
branches produce release candidates, while immutable annotated `v*` tags are
the only source of official GitHub Releases.

Official releases are initially created as drafts. Publishing the draft remains
a manual maintainer decision after its notes and artifacts have been reviewed.

## Release lifecycle

```text
pull requests -> main -> release/VERSION -> candidate artifact -> manual tests
                                                               |
                              annotated vVERSION tag on main <-+
                                                               |
                                              draft GitHub Release
                                                               |
                                              tested DEB packages
                                                               |
                                                 manual publication
```

## 1. Prepare the release branch

Choose a SemVer `MAJOR.MINOR.PATCH` version and create a short-lived branch from
an up-to-date `main`:

```sh
git switch main
git pull --ff-only
git switch -c release/1.2.12
```

Prepare a release commit that:

1. changes the version in `configure.in` (`AM_INIT_AUTOMAKE`);
2. applies the same version to the shipped generated `configure` script;
3. changes `CHANGELOG.md`'s `[Unreleased]` content into a dated
   `[VERSION]` entry and adds a new empty `[Unreleased]` section; and
4. contains no unrelated behavior changes.

The legacy Autotools stack cannot currently be regenerated unchanged with
modern `autoreconf`. Update the narrowly scoped generated version fields rather
than committing a broad Autotools regeneration.

Validate the release metadata locally:

```sh
tools/check-release-version.sh 1.2.12
```

This rejects non-SemVer versions, disagreement between source and generated
package metadata, missing changelog entries, and duplicate version headings.
Submit the release preparation through the normal pull-request and CI process.
Normal CI invokes `make deb`, so Debian packaging regressions must pass before
candidate assembly begins.

## 2. Build and test a candidate

Open **Actions -> Release candidate -> Run workflow** in GitHub. Select the
`release/1.2.12` branch and enter `1.2.12` as the version.

The workflow accepts only `release/*` branches. It runs:

- configuration and compilation;
- the complete Xvfb-backed `make check` suite;
- `make distcheck` from the source distribution; and
- `make deb`, followed by installation and smoke testing of both packages.

A successful run uploads a 30-day workflow artifact containing:

- `xmms-1.2.12.tar.gz`;
- Ubuntu 24.04 `xmms` and `libxmms-dev` DEBs;
- `SHA256SUMS` covering the source archive and both DEBs;
- `release-notes.md`, extracted from the versioned changelog entry; and
- `RELEASE-METADATA.txt`, identifying the candidate commit and workflow run.

Download that workflow artifact and verify it before testing:

```sh
sha256sum -c SHA256SUMS
tar -xzf xmms-1.2.12.tar.gz
sudo apt install './xmms_1.2.12-1~ubuntu24.04_amd64.deb' \
  './libxmms-dev_1.2.12-1~ubuntu24.04_amd64.deb'
```

Manual testing should use the candidate archive rather than an unrelated local
checkout. At minimum, test clean configuration and compilation, startup, MP3
playback and seeking, volume and balance, popup menus, playlist operations, and
preferences. Exercise relevant Linux/BSD and audio/display environments when
available. Record the candidate commit and results in the release pull request.

If testing finds a defect, fix it through a reviewed change, update both the
release branch and `main` as appropriate, then build a new candidate. Do not
reuse an older artifact after the candidate commit changes.

## 3. Finalize and tag

After candidate approval, merge the release preparation into `main` and wait
for required CI to pass. Create an annotated tag on the resulting `main`
commit:

```sh
git switch main
git pull --ff-only
git tag -a v1.2.12 -m "XMMS GTK2 1.2.12"
git push origin v1.2.12
```

A release tag must:

- use the exact `vMAJOR.MINOR.PATCH` form;
- be annotated rather than lightweight;
- point to a commit contained in `main`;
- match `configure.in` and `configure`; and
- have exactly one non-empty `CHANGELOG.md` release entry.

Pushing the tag does not publish automatically. Manually dispatch **Linux
packages and release** on that tag and provide the matching version. Its first
job rejects a branch, lightweight or mismatched tag, a tag outside `main`, or
an existing published Release before package work begins.

## 4. Build packages and assemble the draft Release

The workflow has only a `workflow_dispatch` trigger. After release validation,
a two-target build matrix runs in digest-pinned Linux Mint 22.3 Zena and
Ubuntu 26.04 Resolute amd64 container images. Build jobs retain read-only
repository permissions and independently verify their target identity.

Each target then:

- creates the matching source archive for `make deb`;
- builds `xmms` and `libxmms-dev` with revision `1~linuxmint22.3` or
  `1~ubuntu26.04`;
- runs the Xvfb-backed package tests and linkage checks through `make deb`;
- checks package identity, control version, and architecture;
- installs both packages in the build environment and checks `xmms --version`
  plus the development headers; and
- uploads target-specific DEBs, checksums, and package metadata as a 30-day
  Actions artifact.

Downloadable package names use `.linuxmint22.3_amd64.deb` or
`.ubuntu26.04_amd64.deb`. The Mint artifact also carries the canonical source
archive.

Only after release validation and both matrix variants succeed does the final
job receive `actions: read` and `contents: write`. It downloads artifacts from
the same run, re-verifies both `PACKAGES-SHA256SUMS` and `SHA256SUMS` files,
and assembles four DEBs, one source archive, release notes, target metadata,
package checksum manifests, `RELEASE-METADATA.txt`, and a complete
`SHA256SUMS`. It then creates a draft GitHub Release for the existing tag.

A rerun may replace assets only while that Release remains a draft. The
workflow fails rather than modify a published release, and it never publishes
a draft automatically. Never replace the existing v1.2.12 assets with
different bytes; use a new patch version. LMDE, other Ubuntu/Mint releases,
and non-amd64 architectures require separate builds and testing.

## 5. Review and publish

Before publishing the draft in GitHub:

1. confirm the tag and displayed version;
2. inspect the extracted release notes;
3. download the source archive and verify `SHA256SUMS`;
4. verify the release-level `SHA256SUMS` and both target package checksum
   manifests;
5. confirm release validation and both Linux Mint and Ubuntu package jobs
   succeeded for the tagged commit on `main`;
6. install and smoke-test each package pair on its named distribution; and
7. perform any other final manual checks required for the release.

Then publish the complete draft manually. With immutable releases enabled,
publication permanently locks its tag, notes, and all source and package
assets. Delete the short-lived release branch after publication. Never move,
delete, or recreate a published version tag; prepare a new patch release
instead.

Rerunning the manual workflow may repair an existing draft and replace its
assets. It fails closed once the Release is published.

## Rollback and hotfixes

A GitHub Release is immutable release history, not a deployment that can be
silently rolled back. If a published artifact is defective:

1. mark the affected release clearly in its notes if users must avoid it;
2. branch from the appropriate maintained commit;
3. prepare and test a new patch version; and
4. publish a new tag and release through the same process.

Do not replace assets on an already published release with different bytes.

## Implementation references

The workflows follow GitHub's documented mechanisms for
[manual workflow dispatch](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow),
[tag push filters](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#onpushbranchestagsbranches-ignoretags-ignore),
least-privilege
[`GITHUB_TOKEN` permissions](https://docs.github.com/en/actions/security-for-github-actions/security-guides/automatic-token-authentication#modifying-the-permissions-for-the-github_token),
and [container jobs](https://docs.github.com/en/actions/how-tos/write-workflows/choose-where-workflows-run/run-jobs-in-a-container).
The package workflow pins the
[`linuxmintd/mint22.3-amd64`](https://hub.docker.com/r/linuxmintd/mint22.3-amd64)
image family used by Linux Mint's own CI and the official
[`ubuntu:26.04`](https://hub.docker.com/_/ubuntu) amd64 image by manifest
digest. Draft assembly uses the pinned GitHub artifact actions and the GitHub
CLI's `release create --verify-tag --draft` boundary. Package recipes follow
the official [Debian maintainer
reference](https://www.debian.org/doc/manuals/debmake-doc/).
