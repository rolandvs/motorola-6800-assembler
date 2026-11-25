* as5 regression test: 6805
        org     $100
start   lda     #$12
        lda     $50
        lda     $1234
        lda     ,x
        lda     5,x
        lda     $300,x
        sta     $51
        ldx     #$10
        stx     var
        inc     $20
        clr     3,x
        incx
        bset    3,$10
        bclr    2,$11
        brset   1,$12,start
        brclr   0,$13,fwd
fwd     bne     start
        bih     start
        jsr     sub
        jmp     ,x
sub     rts
        mul
var     rmb     2
        fdb     start,var
        end
