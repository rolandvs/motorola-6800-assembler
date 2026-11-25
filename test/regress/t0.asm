* as0 regression test: 6800/6802
        org     $100
start   ldaa    #$12            ; immediate
        ldab    <$20            ; forced direct
        ldaa    $1234           ; extended
        ldaa    5,x             ; indexed
        staa    data            ; forward ref -> extended
        stab    $40             ; direct
        ldx     #table
        lds     #$01ff
        cpx     #$1234
        jsr     sub
        jmp     0,x
        asl     $1000
        clr     3,x
        inca
        aba
        bne     start
        bra     *+2
loop    dex
        bne     loop
        swi
sub     rts
table   fcb     1,2,$ff,'A,%1010,@17
        fdb     start,$1234,table+2
        fcc     /Hello/
        fcc     "x"
        rmb     3
        zmb     2
        fill    $55,3
data    equ     *
size    equ     data-start
        fdb     size*2,size/2,size&$f,size|1,size^3,size%7
        end
