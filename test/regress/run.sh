#!/bin/sh
# Regression test: assemble tN.asm with bin/asN and compare the S-record
# output byte-for-byte with expected/tN.s19.
#   ./run.sh           compare against expected/
#   ./run.sh update    regenerate expected/ (only when a change is intended)
cd "$(dirname "$0")" || exit 2
BIN=../../bin
fail=0
mkdir -p expected
for n in 0 1 4 5 9 11; do
    rm -f "t$n.s19"
    # 's19' is ignored by older versions and required by newer ones
    "$BIN/as$n" "t$n.asm" - s19 >"t$n.out" 2>&1
    if [ "$1" = update ]; then
        cp "t$n.s19" "expected/t$n.s19"
        echo "as$n: expected/t$n.s19 updated"
    elif ! cmp -s "t$n.s19" "expected/t$n.s19"; then
        echo "as$n: FAIL (diff t$n.s19 expected/t$n.s19)"; cat "t$n.out"
        fail=1
    elif grep -q Warning "t$n.out"; then
        echo "as$n: FAIL (unexpected warning)"; cat "t$n.out"
        fail=1
    else
        echo "as$n: ok"
    fi
done
# Same source with vdH comment syntax must give the same code:
# "label:", ";comment" right after the operand, ";" comment lines.
[ "$1" = update ] || for n in 0 1 4 5 9 11; do
    sed -e '/^\*/s/^\*/;/' \
        -e '/^[^*;]/{/;/!s/$/;c/}' \
        -e 's/^\([A-Za-z_][A-Za-z0-9_]*\)\([ \t]\)/\1:\2/' \
        "t$n.asm" >"S$n.asm"
    rm -f "S$n.s19"
    "$BIN/as$n" "S$n.asm" - s19 >"S$n.out" 2>&1
    if cmp -s "S$n.s19" "expected/t$n.s19" && ! grep -q Warning "S$n.out"; then
        echo "as$n: comment syntax ok"
    else
        echo "as$n: comment syntax FAIL"; cat "S$n.out"; fail=1
    fi
    rm -f "S$n.asm" "S$n.s19" "S$n.out"
done
# Comment warning: exactly lines 4 6 7 9 of warn.asm, none with 'now'
[ "$1" = update ] || {
    w=$("$BIN/as0" warn.asm | sed -n 's/^\([0-9]*\): Warning.*/\1/p' | tr '\n' ' ')
    n=$("$BIN/as0" warn.asm - now | grep -c Warning)
    if [ "$w" = "4 6 7 9 " ] && [ "$n" = 0 ]; then
        echo "as0: comment warning ok"
    else
        echo "as0: comment warning FAIL (lines '$w', with now: $n)"; fail=1
    fi
    rm -f warn.s19
}
# Upper case source must give the same code (no strings in these two)
[ "$1" = update ] || for n in 9 11; do
    tr a-z A-Z <"t$n.asm" >"T$n.asm"
    rm -f "T$n.s19"
    "$BIN/as$n" "T$n.asm" - s19 >"T$n.out" 2>&1
    if cmp -s "T$n.s19" "expected/t$n.s19"; then
        echo "as$n: upper case ok"
    else
        echo "as$n: upper case FAIL"; cat "T$n.out"; fail=1
    fi
    rm -f "T$n.asm" "T$n.s19" "T$n.out"
done
exit $fail
