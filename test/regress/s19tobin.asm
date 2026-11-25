* s19tobin test: vectors first, code below them, RMB gap
 org $fffe
 fdb start
 org $ff00
start lda #$12
 rmb 3
 sta $20
 bra start
