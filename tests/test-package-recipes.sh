#!/bin/sh
set -eu

srcdir=${1:-.}
failures=0

ok()
{
	echo "ok - $1"
}

not_ok()
{
	echo "not ok - $1" >&2
	failures=$((failures + 1))
}

require_file()
{
	if test -f "$srcdir/$1"; then
		ok "includes $1"
	else
		not_ok "includes $1"
	fi
}

require_text()
{
	file=$1
	text=$2
	description=$3
	if test -f "$srcdir/$file" && grep -F -- "$text" "$srcdir/$file" >/dev/null; then
		ok "$description"
	else
		not_ok "$description"
	fi
}

require_absent_text()
{
	file=$1
	text=$2
	description=$3
	if test -f "$srcdir/$file" && ! grep -F -- "$text" "$srcdir/$file" >/dev/null; then
		ok "$description"
	else
		not_ok "$description"
	fi
}

for file in \
	tests/test-c-lint.sh \
	tools/cppcheck-suppressions.txt \
	tools/run-c-lint.sh \
	packaging/xmms.desktop \
	packaging/debian/control \
	packaging/debian/copyright \
	packaging/debian/libxmms-dev.install \
	packaging/debian/rules \
	packaging/debian/source/format \
	packaging/debian/xmms.install \
	tools/build-deb.sh \
	.github/workflows/package-linux-mint.yml
do
	require_file "$file"
done

require_text Makefile.am 'deb:' \
	'exposes a top-level make deb target'
require_text Makefile.am '$(MAKE) dist-gzip' \
	'creates a source archive for local Debian builds'
require_text Makefile.am '.PHONY: deb lint' \
	'exposes lint as a phony top-level target'
require_text Makefile.am 'lint:' \
	'exposes the public C lint target'
require_text Makefile.am 'tools/run-c-lint.sh' \
	'runs C lint through the shared helper'
require_text Makefile.in 'lint:' \
	'ships the generated C lint target'
require_text tests/Makefile 'test-c-lint:' \
	'runs C lint contract tests from make check'
require_text Makefile.am 'tools/cppcheck-suppressions.txt' \
	'distributes the C lint baseline'
require_text Makefile.am 'docs/architecture/build-and-test.md' \
	'distributes the C lint architecture guide'
require_text Makefile.am 'tools/build-deb.sh' \
	'builds Debian packages through the shared helper'
require_text Makefile.am '.github/workflows/package-linux-mint.yml' \
	'distributes the Linux Mint package workflow'
require_text Makefile.in '.github/workflows/package-linux-mint.yml' \
	'ships the generated Linux Mint workflow distribution rule'
require_text packaging/debian/control ' cppcheck,' \
	'declares the C analyzer as a Debian build dependency'
require_text CONTRIBUTING.md 'make lint' \
	'documents the local C lint command'
require_text CONTRIBUTING.md 'suppression baseline' \
	'documents controlled lint baseline maintenance'
require_text docs/architecture/build-and-test.md 'Cppcheck' \
	'documents the C lint architecture'
require_text docs/architecture/build-and-test.md 'tools/cppcheck-suppressions.txt' \
	'documents the lint baseline path'
require_text packaging/xmms.desktop 'Name=XMMS GTK2' \
	'uses current branding in the desktop entry'
require_text packaging/xmms.desktop 'Exec=xmms %U' \
	'preserves the xmms executable name'
require_absent_text packaging/xmms.desktop 'Encoding=' \
	'does not use the obsolete desktop Encoding key'
require_text tools/build-deb.sh 'dpkg-buildpackage --build=binary --no-sign' \
	'builds unsigned binary Debian packages'
require_text tools/build-deb.sh "grep 'undefined symbol: .*_ZGV'" \
	'checks packaged MP3 plugin vector math linkage'
require_text tools/build-deb.sh 'lintian --fail-on error' \
	'runs Debian package policy checks from make deb'
require_absent_text tools/build-deb.sh 'sudo' \
	'never elevates privileges from make deb'
require_text .github/workflows/package-linux-mint.yml 'workflow_dispatch:' \
	'builds Linux Mint packages only on manual dispatch'
require_absent_text .github/workflows/package-linux-mint.yml 'pull_request:' \
	'does not build release artifacts for pull requests'
require_absent_text .github/workflows/package-linux-mint.yml 'push:' \
	'does not build release artifacts on push'
require_text .github/workflows/package-linux-mint.yml \
	'linuxmintd/mint22.3-amd64@sha256:f71f1a261ef2957022ae74ad2b89ebbc8fcb2f25e40d8d7cdb599aa9e2748a8e' \
	'pins the Linux Mint 22.3 build image'
require_text .github/workflows/package-linux-mint.yml \
	'ubuntu@sha256:7c2884fd32770fc6c173b78e0dc2278a2851d89f5447919edbc45475ac55dd6a' \
	'pins the Ubuntu 26.04 amd64 build image'
require_text .github/workflows/package-linux-mint.yml 'target_id: ubuntu' \
	'adds an Ubuntu package target'
require_text .github/workflows/package-linux-mint.yml 'target_release: "26.04"' \
	'identifies Ubuntu 26.04'
require_text .github/workflows/package-linux-mint.yml 'target_codename: resolute' \
	'identifies Ubuntu Resolute'
require_text .github/workflows/package-linux-mint.yml \
	'deb_revision: 1~ubuntu26.04' \
	'uses an Ubuntu 26.04 package revision'
require_text .github/workflows/package-linux-mint.yml \
	'.ubuntu26.04_amd64.deb' \
	'checks Ubuntu 26.04 downloadable package names'
require_text .github/workflows/package-linux-mint.yml \
	'ubuntu26.04-amd64' \
	'uploads a distinct Ubuntu 26.04 artifact'
require_text .github/workflows/package-linux-mint.yml \
	'official-package-repositories.list' \
	'verifies the Linux Mint package repository configured by the image'
require_text .github/workflows/package-linux-mint.yml \
	'codename="${MINT_CODENAME}"' \
	'verifies the Linux Mint image codename'
require_absent_text .github/workflows/package-linux-mint.yml \
	'/etc/linuxmint/info' \
	'does not require desktop-only Linux Mint identity metadata'
require_text .github/workflows/package-linux-mint.yml 'DEB_DISTRIBUTION: zena' \
	'marks package metadata for Linux Mint Zena'
require_text .github/workflows/package-linux-mint.yml \
	'DEB_REVISION: 1~linuxmint22.3' \
	'uses a Linux Mint package revision'
require_text .github/workflows/package-linux-mint.yml \
	'PACKAGES-SHA256SUMS' \
	'publishes package checksums'
require_text .github/workflows/package-linux-mint.yml \
	'PACKAGE-METADATA.txt' \
	'publishes package provenance and control metadata'
require_text .github/workflows/package-linux-mint.yml \
	'actions/upload-artifact' \
	'uploads Linux Mint release artifacts'
require_text packaging/debian/control 'Package: xmms' \
	'defines the Debian runtime package'
require_text packaging/debian/control 'Package: libxmms-dev' \
	'defines the Debian development package'
require_text packaging/debian/rules './configure' \
	'configures the Debian build explicitly'
require_text packaging/debian/rules 'override_dh_autoreconf:' \
	'preserves the shipped legacy Autotools files'
require_text packaging/debian/rules 'optimize=-lto' \
	'disables LTO for the legacy bundled libtool'
require_text packaging/debian/rules '-Wno-error=incompatible-pointer-types' \
	'permits legacy GTK callbacks with newer Ubuntu GCC'
require_text packaging/debian/rules 'DEB_BUILD_OPTIONS' \
	'honors Debian package test controls'
require_text packaging/debian/rules '--disable-esd' \
	'disables the obsolete ESD plugin in Debian builds'
require_absent_text Makefile.am 'xmms.spec' \
	'does not ship legacy RPM package metadata'
require_absent_text Makefile.am 'packaging/rpm' \
	'does not ship modern RPM package recipes'

if test "$failures" -ne 0; then
	echo "$failures package recipe checks failed" >&2
	exit 1
fi
