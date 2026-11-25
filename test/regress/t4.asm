* as4 regression test: 6804
        org     $80
start   lda     #$12
        lda     $81
        sta     $83
        lda     [$80]
        inc     $80
        dec     $83
        add     #1
        sub     $10
        cmp     [$81]
        ldxi    #$90
        ldyi    #$91
        clrx
        clry
        mvi     $82,5
        incx
        decy
        bset    3,$10
        bclr    2,$11
        brset   1,$12,start
        brclr   0,$13,start
        bne     sub
        jsr     sub
        jmp     start
sub     rts
        fcb     1,2,3
        end
