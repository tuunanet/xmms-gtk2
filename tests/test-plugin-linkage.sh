#!/bin/sh
set -eu

top_builddir=${1:-..}
plugin=$top_builddir/Input/mpg123/.libs/libmpg123.so
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

if test -f "$plugin"; then
	ok "built MP3 input plugin exists"
else
	not_ok "built MP3 input plugin exists"
fi

if readelf -d "$plugin" | grep 'NEEDED' | grep -F 'libm.so' >/dev/null; then
	ok "MP3 input plugin links directly to libm"
else
	not_ok "MP3 input plugin links directly to libm"
fi

# glibc vector math entry points such as _ZGVbN2v_cos must resolve from the
# plugin's own dependencies, not accidentally from the main executable.
if ldd -r "$plugin" 2>&1 | grep 'undefined symbol: .*_ZGV' \
	>/tmp/xmms-plugin-linkage-vector-undefined.$$; then
	cat /tmp/xmms-plugin-linkage-vector-undefined.$$ >&2
	not_ok "MP3 input plugin resolves vector math symbols"
else
	ok "MP3 input plugin resolves vector math symbols"
fi
rm -f /tmp/xmms-plugin-linkage-vector-undefined.$$

if test "$failures" -ne 0; then
	echo "$failures plugin linkage checks failed" >&2
	exit 1
fi
