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

output_file=$(mktemp)
tmpdir=$(mktemp -d)
trap 'rm -f "$output_file"; rm -rf "$tmpdir"' EXIT HUP INT TERM

if PATH=/nonexistent "$srcdir/tools/run-c-lint.sh" >"$output_file" 2>&1; then
	not_ok "fails when Cppcheck is unavailable"
elif grep -F 'Cppcheck is required' "$output_file" >/dev/null; then
	ok "explains the missing Cppcheck prerequisite"
else
	not_ok "explains the missing Cppcheck prerequisite"
fi

mkdir -p "$tmpdir/bin"
cat >"$tmpdir/bin/cppcheck" <<'EOF'
#!/bin/sh
printf '%s\n' "$@" >"$CPPCHECK_ARGS_FILE"
EOF
chmod +x "$tmpdir/bin/cppcheck"

args_file="$tmpdir/cppcheck-args"
if CPPCHECK_ARGS_FILE="$args_file" PATH="$tmpdir/bin:$PATH" \
	"$srcdir/tools/run-c-lint.sh"; then
	ok "runs Cppcheck for the maintained source tree"
else
	not_ok "runs Cppcheck for the maintained source tree"
fi

for argument in \
	'--enable=warning,performance,portability' \
	'--error-exitcode=1' \
	'--library=gnu' \
	'--library=gtk' \
	'--library=posix' \
	'-DN_(String)=String' \
	'-D_(String)=String' \
	"--suppressions-list=$srcdir/tools/cppcheck-suppressions.txt" \
	"$srcdir/xmms" \
	"$srcdir/tests"
do
	if grep -Fx -- "$argument" "$args_file" >/dev/null; then
		ok "passes $argument"
	else
		not_ok "passes $argument"
	fi
done

if grep -Fx -- "$srcdir/intl" "$args_file" >/dev/null; then
	not_ok "excludes generated intl sources"
else
	ok "excludes generated intl sources"
fi

if "$srcdir/tools/run-c-lint.sh" >"$output_file" 2>&1; then
	ok "accepts the reviewed legacy baseline"
else
	cat "$output_file" >&2
	not_ok "accepts the reviewed legacy baseline"
fi

fixture_root="$tmpdir/fixture-project"
mkdir -p "$fixture_root/tools"
for directory in Effect General Input Output Visualization libxmms tests wmxmms xmms
do
	mkdir -p "$fixture_root/$directory"
done
cp "$srcdir/tools/run-c-lint.sh" "$fixture_root/tools/run-c-lint.sh"
: >"$fixture_root/tools/cppcheck-suppressions.txt"
cat >"$fixture_root/xmms/new-diagnostic.c" <<'EOF'
int main(void)
{
	int value;
	return value;
}
EOF

if "$fixture_root/tools/run-c-lint.sh" >"$output_file" 2>&1; then
	not_ok "rejects a representative new diagnostic"
elif grep -F '[uninitvar]' "$output_file" >/dev/null; then
	ok "rejects a representative new diagnostic"
else
	cat "$output_file" >&2
	not_ok "reports the representative new diagnostic"
fi

if test "$failures" -ne 0; then
	echo "$failures C lint checks failed" >&2
	exit 1
fi
