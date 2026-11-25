* listing test: tabs in the source must line up in the .lst
	NAM	LISTTEST
PORT	EQU	$FD00
	ORG	$F800
start	LDAA	#$41	; tab before comment
lp	STAA	PORT
	BRA	lp
;
msg	fcc	/a long string, two lines/
x  ldaa  #1  ; spaces, not tabs
	END
