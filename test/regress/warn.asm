* Comment warning test: lines 4,6,7,9 must warn, the others not
foo equ 10
bar equ 3
 ldaa #bar - foo
 ldaa #bar-foo ; ok
 ldaa 0,x  index
 fcb 1,2 bytes
 nop ; fine
 rts no semicolon
