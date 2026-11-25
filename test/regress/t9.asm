* as9 regression test: 6809
        org     $1000
start   lda     #$12
        ldb     <$20
        ldd     #$1234
        ldx     #table
        ldy     #$2000
        ldu     ,s
        lds     #$7fff
        lda     ,x
        lda     5,x
        lda     -1,y
        lda     $100,u
        lda     ,x+
        lda     ,x++
        lda     ,-y
        lda     ,--s
        lda     a,x
        ldb     b,y
        ldd     d,u
        lda     [,x]
        lda     [$1234]
        lda     table,pcr
        leax    1,x
        leay    -2,y
        tfr     a,b
        exg     x,y
        tfr     d,x
        pshs    a,b,x,y,u,pc
        puls    cc,a,dp
        pshu    s,x
        cmpd    #1
        cmpy    #2
        cmpu    #3
        cmps    #4
        swi2
        swi3
        sync
        cwai    #$ef
        andcc   #$fe
        orcc    #1
        bra     start
        lbra    start
        lbsr    sub
        lbeq    far
        beq     start
        jsr     sub
        jmp     [,x]
sub     rts
table   fcb     1,2,3
        fdb     start,table
        fcc     'ABC'
far     equ     *
        rmb     2
        end
