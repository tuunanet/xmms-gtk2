AUTOMAKE_OPTIONS = foreign

SUBDIRS = intl libxmms xmms Output Input Effect General Visualization wmxmms po

bin_SCRIPTS = xmms-config

EXTRA_DIST = \
	.editorconfig \
	.gitattributes \
	.gitignore \
	.github/dependabot.yml \
	.github/ISSUE_TEMPLATE/bug_report.yml \
	.github/ISSUE_TEMPLATE/config.yml \
	.github/ISSUE_TEMPLATE/feature_request.yml \
	.github/pull_request_template.md \
	CHANGELOG.md \
	CODE_OF_CONDUCT.md \
	CONTRIBUTING.md \
	CONVENTIONS.md \
	CONTRIBUTORS.md \
	README.md \
	SECURITY.md \
	docs/architecture/build-and-test.md \
	docs/history/README.md \
	docs/history/upstream-changelog.txt \
	docs/history/upstream-faq.txt \
	docs/history/upstream-release-notes.txt \
	docs/history/upstream-todo.txt \
	docs/images/README.md \
	docs/images/xmms.png \
	docs/manual.md \
	docs/releases.md \
	mkinstalldirs \
	packaging/debian/control \
	packaging/debian/copyright \
	packaging/debian/libxmms-dev.install \
	packaging/debian/rules \
	packaging/debian/source/format \
	packaging/debian/xmms.docs \
	packaging/debian/xmms.install \
	packaging/xmms.desktop \
	tests/Makefile \
	tests/fixture-input-plugin.c \
	tests/fixture-output-plugin.c \
	tests/test-alsa-pcm-state.c \
	tests/test-alsa-volume.c \
	tests/test-filebrowser.c \
	tests/test-font-load.c \
	tests/test-c-lint.sh \
	tests/test-intl-generated-sources.sh \
	tests/test-keyboard-shortcuts.c \
	tests/test-mpg123-file-duration.c \
	tests/test-mpg123-stream-position.c \
	tests/test-outputplugin.c \
	tests/test-package-recipes.sh \
	tests/test-plugin-linkage.sh \
	tests/test-pluginenum.c \
	tests/test-popup-position.c \
	tests/test-release-tools.sh \
	tests/test-xentry.c \
	tools/build-deb.sh \
	tools/check-release-version.sh \
	tools/cppcheck-suppressions.txt \
	tools/run-c-lint.sh \
	tools/extract-release-notes.sh \
	xmms.m4

m4datadir = $(datadir)/aclocal
m4data_DATA = xmms.m4

man_MANS = xmms.1 wmxmms.1

.PHONY: deb lint

lint:
	$(SHELL) '$(top_srcdir)/tools/run-c-lint.sh'

deb:
	@archive='$(DEB_SOURCE_ARCHIVE)'; \
	if test -z "$$archive"; then \
		$(MAKE) dist-gzip; \
		archive='$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz'; \
	fi; \
	$(SHELL) '$(top_srcdir)/tools/build-deb.sh' '$(VERSION)' \
		"$$archive" '$(DEB_OUTPUT_DIR)' '$(top_srcdir)/packaging'

DEB_OUTPUT_DIR = $(top_builddir)/deb-artifacts

check-local:
	$(mkdir_p) tests
	@top_srcdir=`cd $(top_srcdir) && pwd`; top_builddir=`pwd`; \
	$(MAKE) -C tests -f "$$top_srcdir/tests/Makefile" \
		srcdir="$$top_srcdir/tests" top_srcdir="$$top_srcdir" \
		top_builddir="$$top_builddir" check

clean-local:
	@if test -d tests; then \
		top_srcdir=`cd $(top_srcdir) && pwd`; \
		$(MAKE) -C tests -f "$$top_srcdir/tests/Makefile" \
			srcdir="$$top_srcdir/tests" top_srcdir="$$top_srcdir" clean; \
	fi
