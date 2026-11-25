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
        -e '/^[^*;]/{' -e '/;/!s/$/;c/' -e '}' \
        -e 's/^\([A-Za-z_][A-Za-z0-9_]*\)\([ \t]\)/\1:\2/' \
        "t$n.asm" >"syntax$n.asm"
    rm -f "syntax$n.s19"
    "$BIN/as$n" "syntax$n.asm" - s19 >"syntax$n.out" 2>&1
    if cmp -s "syntax$n.s19" "expected/t$n.s19" && ! grep -q Warning "syntax$n.out"; then
        echo "as$n: comment syntax ok"
    else
        echo "as$n: comment syntax FAIL"; cat "syntax$n.out"; fail=1
    fi
    rm -f "syntax$n.asm" "syntax$n.s19" "syntax$n.out"
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
# .bin: a plain dump of one contiguous block; error if fragmented
[ "$1" = update ] || {
    printf ' org $100\n lda #1\n fcb 2,3\nbuf rmb 4\n' >frag1.asm
    printf ' org $100\n fcb 1\n rmb 2\n fcb 2\n' >frag2.asm
    rm -f frag1.bin frag2.bin frag2.s19
    "$BIN/as9" frag1.asm - bin nos19 >frag1.out 2>&1; r1=$?
    "$BIN/as9" frag2.asm - bin >frag2.out 2>&1; r2=$?
    if [ $r1 = 0 ] && [ ! -s frag1.out ] && [ "$(od -An -tx1 frag1.bin | tr -d ' ')" = 86010203 ] \
       && [ ! -f frag1.s19 ] && [ $r2 != 0 ] && [ ! -f frag2.bin ] && [ -f frag2.s19 ] \
       && grep -q 'bin is fragmented (code jumps to \$0103), use s19tobin' frag2.out; then
        echo "as9: bin ok"
    else
        echo "as9: bin FAIL"; cat frag1.out frag2.out; fail=1
    fi
    rm -f frag1.* frag2.*
}
# s19tobin: padded images, EPROM window, and its error checks
[ "$1" = update ] || {
    S="$BIN/s19tobin"; ok=1
    "$BIN/as9" s19tobin.asm >/dev/null 2>&1
    "$S" s19tobin.s19 o.bin >/dev/null && cmp -s o.bin expected/s19tobin.bin || ok=0
    "$S" -s E000 -e '$FFFF' s19tobin.s19 o.bin >/dev/null \
        && cmp -s o.bin expected/s19tobin_e000.bin || ok=0
    "$S" expected/t0.s19 o.bin >/dev/null && cmp -s o.bin expected/t0.bin || ok=0
    rm -f o.bin
    # each of these must fail and write no file
    printf 'S10501000102F6\nS1040101AA4F\n' >e1.s19     # $0101 twice
    printf 'S10501000102F7\n' >e2.s19                   # bad checksum
    printf 'hello\n' >e3.s19                            # not an S-record
    # options anywhere, "-f FF" or "-fFF"; default name prog.s19 -> prog.bin
    "$S" s19tobin.s19 o.bin -f FF >/dev/null && cmp -s o.bin expected/s19tobin.bin || ok=0
    "$S" -fFF s19tobin.s19 o.bin >/dev/null && cmp -s o.bin expected/s19tobin.bin || ok=0
    rm -f o.bin s19tobin.bin
    "$S" s19tobin.s19 >/dev/null && cmp -s s19tobin.bin expected/s19tobin.bin || ok=0
    rm -f s19tobin.bin
    for e in "e1.s19" "e2.s19" "e3.s19" "-s FF10 s19tobin.s19" "-x 1 s19tobin.s19"; do
        "$S" $e o.bin 2>/dev/null && ok=0
        [ -f o.bin ] && ok=0
    done
    if [ $ok = 1 ]; then echo "s19tobin: ok"; else echo "s19tobin: FAIL"; fail=1; fi
    rm -f o.bin e1.s19 e2.s19 e3.s19 s19tobin.s19
}
# Upper case source must give the same code (no strings in these two).
# Temporary files must not differ from real ones only in case: macOS
# file systems ignore case, so T9.asm would overwrite t9.asm.
[ "$1" = update ] || for n in 9 11; do
    tr a-z A-Z <"t$n.asm" >"upper$n.asm"
    rm -f "upper$n.s19"
    "$BIN/as$n" "upper$n.asm" - s19 >"upper$n.out" 2>&1
    if cmp -s "upper$n.s19" "expected/t$n.s19"; then
        echo "as$n: upper case ok"
    else
        echo "as$n: upper case FAIL"; cat "upper$n.out"; fail=1
    fi
    rm -f "upper$n.asm" "upper$n.s19" "upper$n.out"
done
exit $fail
