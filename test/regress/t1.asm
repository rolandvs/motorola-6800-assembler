* as1 regression test: 6801/6803
        org     $8000
start   ldd     #$1234
        std     $80
        addd    1,x
        subd    #1
        mul
        abx
        pshx
        pulx
        lsrd
        asld
        brn     start
        ldaa    #-1
        staa    var
        jsr     $f000
        bra     start
tab     fdb     start,var
        fcb     $10,$20
var     rmb     1
        end
