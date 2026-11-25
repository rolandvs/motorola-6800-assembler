* as11 regression test: 68HC11
        org     $e000
start   lds     #$00ff
        ldx     #$1000
        ldy     #$1000
        ldaa    #$12
        ldab    $20
        ldd     $1234
        ldaa    5,x
        ldaa    5,y
        staa    0,y
        std     2,x
        sty     $30
        stx     var
        cpd     #1
        cpy     #2
        cmpx    3,y
        cpx     4,x
        iny
        dey
        pshy
        puly
        xgdy
        aby
        idiv
        fdiv
        bset    $10 $55
        bclr    1,x $aa
        bset    2,y $0f
        brset   $10 $01 start
        brclr   3,x $80 start
        brset   4,y $40 fwd
fwd     bne     start
        jsr     0,y
        jmp     start
var     rmb     2
        fdb     start,var
        end
