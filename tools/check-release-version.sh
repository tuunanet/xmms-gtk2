#!/bin/sh
set -eu

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
	echo "usage: $0 VERSION [REPOSITORY_ROOT]" >&2
	exit 2
fi

expected=$1
root=${2:-.}

if ! printf '%s\n' "$expected" | grep -Eq '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$'; then
	echo "error: '$expected' is not a MAJOR.MINOR.PATCH version" >&2
	exit 1
fi

for file in configure.in configure CHANGELOG.md; do
	if [ ! -f "$root/$file" ]; then
		echo "error: required release file is missing: $file" >&2
		exit 1
	fi
done

source_versions=$(sed -n 's/^AM_INIT_AUTOMAKE(\[xmms\], \[\([^]]*\)\])$/\1/p' \
	"$root/configure.in")
source_count=$(printf '%s\n' "$source_versions" | grep -c . || true)
if [ "$source_count" -ne 1 ]; then
	echo "error: configure.in must contain exactly one XMMS package version" >&2
	exit 1
fi
if [ "$source_versions" != "$expected" ]; then
	echo "error: requested version $expected does not match configure.in version $source_versions" >&2
	exit 1
fi

generated_versions=$(sed -n 's/^[[:space:]]*VERSION=\([^[:space:]]*\)[[:space:]]*$/\1/p' \
	"$root/configure")
generated_count=$(printf '%s\n' "$generated_versions" | grep -c . || true)
if [ "$generated_count" -ne 1 ]; then
	echo "error: configure must contain exactly one generated package version" >&2
	exit 1
fi
if [ "$generated_versions" != "$expected" ]; then
	echo "error: generated configure version $generated_versions does not match $expected" >&2
	exit 1
fi

changelog_count=$(awk -v heading="## [$expected]" '
	index($0, heading) == 1 {
		suffix = substr($0, length(heading) + 1)
		if (suffix == "" || index(suffix, " - ") == 1 ||
		    index(suffix, " — ") == 1)
			count++
	}
	END { print count + 0 }
' "$root/CHANGELOG.md")
if [ "$changelog_count" -ne 1 ]; then
	echo "error: CHANGELOG.md must contain exactly one [$expected] release heading" >&2
	exit 1
fi

printf '%s\n' "$expected"
