#!/bin/sh
set -eu

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
	echo "usage: $0 VERSION OUTPUT_FILE [CHANGELOG]" >&2
	exit 2
fi

version=$1
output=$2
changelog=${3:-CHANGELOG.md}

if ! printf '%s\n' "$version" | grep -Eq '^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$'; then
	echo "error: '$version' is not a MAJOR.MINOR.PATCH version" >&2
	exit 1
fi
if [ ! -f "$changelog" ]; then
	echo "error: changelog is missing: $changelog" >&2
	exit 1
fi

output_dir=$(dirname "$output")
if [ ! -d "$output_dir" ]; then
	echo "error: output directory is missing: $output_dir" >&2
	exit 1
fi

temporary="$output.tmp.$$"
trap 'rm -f "$temporary"' EXIT HUP INT TERM

awk -v heading="## [$version]" '
	index($0, heading) == 1 {
		suffix = substr($0, length(heading) + 1)
		if (suffix == "" || index(suffix, " - ") == 1 ||
		    index(suffix, " — ") == 1) {
			found = 1
			next
		}
	}
	found && /^## / { exit }
	found { lines[++count] = $0 }
	END {
		first = 1
		while (first <= count && lines[first] ~ /^[[:space:]]*$/)
			first++
		while (count >= first && lines[count] ~ /^[[:space:]]*$/)
			count--
		for (i = first; i <= count; i++)
			print lines[i]
	}
' "$changelog" > "$temporary"

if ! grep -q '[^[:space:]]' "$temporary"; then
	echo "error: changelog entry [$version] has no release notes" >&2
	exit 1
fi

mv "$temporary" "$output"
trap - EXIT HUP INT TERM
