#!/bin/sh
set -eu

if ! command -v cppcheck >/dev/null 2>&1; then
	echo "Cppcheck is required; install the cppcheck package." >&2
	exit 2
fi

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
srcdir=$(CDPATH= cd -- "$script_dir/.." && pwd)

echo "Running $(cppcheck --version)" >&2
exec cppcheck \
	--quiet \
	--enable=warning,performance,portability \
	--error-exitcode=1 \
	--library=gnu \
	--library=gtk \
	--library=posix \
	'-DN_(String)=String' \
	'-D_(String)=String' \
	--relative-paths="$srcdir" \
	--suppressions-list="$srcdir/tools/cppcheck-suppressions.txt" \
	"$srcdir/Effect" \
	"$srcdir/General" \
	"$srcdir/Input" \
	"$srcdir/Output" \
	"$srcdir/Visualization" \
	"$srcdir/libxmms" \
	"$srcdir/tests" \
	"$srcdir/wmxmms" \
	"$srcdir/xmms"
