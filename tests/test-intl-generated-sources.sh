#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
	echo "usage: $0 REPOSITORY_ROOT" >&2
	exit 2
fi

root=$1
tmp=${TMPDIR:-/tmp}/xmms-intl-generated-sources.$$
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

mkdir "$tmp"
cp "$root/intl/Makefile.in" "$root/intl/plural.c" \
	"$root/intl/plural.y" "$tmp"/
cp "$tmp/plural.c" "$tmp/plural.c.expected"

# Git checkouts do not preserve mtimes. Make plural.y unambiguously newer to
# reproduce the ordering that previously made distcheck invoke modern Bison.
touch -t 200001010000 "$tmp/plural.c"
touch -t 200101010000 "$tmp/plural.y"

if ! make -C "$tmp" -f Makefile.in plural.c YACC=false srcdir=. \
	>"$tmp/make.log" 2>&1; then
	cat "$tmp/make.log" >&2
	echo "not ok - shipped intl/plural.c must not be regenerated" >&2
	exit 1
fi

if ! cmp -s "$tmp/plural.c.expected" "$tmp/plural.c"; then
	echo "not ok - shipped intl/plural.c changed" >&2
	exit 1
fi

printf '%s\n' "ok - preserves shipped intl/plural.c when plural.y is newer"
