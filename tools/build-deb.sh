#!/bin/sh
set -eu

if test "$#" -ne 4; then
	echo "usage: $0 VERSION SOURCE_ARCHIVE OUTPUT_DIRECTORY PACKAGING_DIRECTORY" >&2
	exit 2
fi

version=$1
source_archive=$2
output_dir=$3
packaging_dir=$4

if ! printf '%s\n' "$version" | grep -Eq \
    '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$'; then
	echo "error: invalid package version: $version" >&2
	exit 1
fi

for command in dpkg-architecture dpkg-buildpackage dpkg-deb ldd lintian readelf tar; do
	if ! command -v "$command" >/dev/null 2>&1; then
		echo "error: make deb requires $command" >&2
		exit 1
	fi
done

if test ! -f "$source_archive"; then
	echo "error: source archive not found: $source_archive" >&2
	exit 1
fi
if test ! -d "$packaging_dir/debian"; then
	echo "error: Debian packaging recipes not found: $packaging_dir/debian" >&2
	exit 1
fi

source_archive=$(cd "$(dirname "$source_archive")" && pwd)/$(basename "$source_archive")
packaging_dir=$(cd "$packaging_dir" && pwd)
mkdir -p "$output_dir"
output_dir=$(cd "$output_dir" && pwd)
source_date_epoch=${SOURCE_DATE_EPOCH:-$(stat -c %Y "$source_archive")}
distribution=${DEB_DISTRIBUTION:-noble}
revision=${DEB_REVISION:-1~ubuntu24.04}
maintainer=${DEB_MAINTAINER:-XMMS Classic contributors <47913151+tuunanet@users.noreply.github.com>}

build_root=$(mktemp -d "${TMPDIR:-/tmp}/xmms-deb.XXXXXX")
trap 'rm -rf "$build_root"' EXIT HUP INT TERM

tar -xzf "$source_archive" -C "$build_root"
source_dir=$build_root/xmms-$version
if test ! -d "$source_dir"; then
	echo "error: archive does not contain xmms-$version" >&2
	exit 1
fi
rm -rf "$source_dir/packaging" "$source_dir/debian"
cp -a "$packaging_dir" "$source_dir/packaging"
cp -a "$packaging_dir/debian" "$source_dir/debian"
release_date=$(date --date="@$source_date_epoch" --rfc-email)
cat > "$source_dir/debian/changelog" <<EOF
xmms (1:$version-$revision) $distribution; urgency=medium

  * Package XMMS Classic $version.

 -- $maintainer  $release_date
EOF

(
	cd "$source_dir"
	dpkg-buildpackage --build=binary --no-sign
)

set -- "$build_root"/*.deb
if test "$#" -ne 2 || test ! -f "$1" || test ! -f "$2"; then
	echo "error: expected exactly two Debian packages" >&2
	exit 1
fi

runtime=$(find "$build_root" -maxdepth 1 -name 'xmms_*.deb' -type f -print)
devel=$(find "$build_root" -maxdepth 1 -name 'libxmms-dev_*.deb' -type f -print)
test -n "$runtime" && test -n "$devel"
test "$(dpkg-deb --field "$runtime" Package)" = xmms
test "$(dpkg-deb --field "$devel" Package)" = libxmms-dev
test "$(dpkg-deb --field "$runtime" Version)" = "1:$version-$revision"
lintian --fail-on error "$runtime" "$devel"

extract_dir=$build_root/installed
mkdir "$extract_dir"
dpkg-deb -x "$runtime" "$extract_dir"
plugin=$(find "$extract_dir/usr/lib" -path '*/xmms/Input/libmpg123.so' \
	-type f -print -quit)
if test -z "$plugin"; then
	echo "error: runtime package does not contain libmpg123.so" >&2
	exit 1
fi
readelf -d "$plugin" | grep -F 'libm.so' >/dev/null
if ldd -r "$plugin" 2>&1 | grep 'undefined symbol: .*_ZGV'; then
	echo "error: packaged MP3 plugin has unresolved vector math symbols" >&2
	exit 1
fi

rm -f "$output_dir"/xmms_*.deb "$output_dir"/libxmms-dev_*.deb
cp "$runtime" "$devel" "$output_dir"/
runtime=$output_dir/$(basename "$runtime")
devel=$output_dir/$(basename "$devel")
printf '%s\n' "Built Debian packages:"
printf '  %s\n' "$runtime" "$devel"
