
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "as.h"
#include "output.h"
#include "util.h"
#include "ffwd.h"
#include "do.h"
#include "ffwd.h"
#include "pseudo.h"
#include "symtab.h"
#include "globals.h"

void initialize(void);
void make_pass(void);
void re_init(void);
void process(void);
int parse_line(void);

/*
	as ---	cross assembler main program
 */
int main(int argc, char *argv[])
{
	char	**np;
	char	*i;
	FILE	*fopen();
	int	j = 0;

	if(argc < 2){
		printf("Usage: %s [files]\n",argv[j]);
		exit(1);
		}
	  Argv = argv;
	  root = NULL;	/* before initialize(): localinit() may install symbols */
	  initialize();
	  while ((*argv[j] != '-') && (j<argc))
	  {
	   j++;
	   if (j == argc) break;
	  }
	  N_files = j-1;
	 if (j < argc )
	  {
	  argv[j]++;
	  while (j<argc)
	   {
	   for (i = argv[j]; *i != 0; i++)
	     if ((*i <= 'Z') && (*i >= 'A'))
	       *i = *i + 32;
	   if (strcmp(argv[j],"l")==0)
	     Lflag = 1;
	   else if (strcmp(argv[j],"nol")==0)
	     Lflag = 0;
	   else if (strcmp(argv[j],"c")==0)
	      Cflag = 1;
	   else if (strcmp(argv[j],"noc")==0)
	     Cflag = 0;
	   else if (strcmp(argv[j],"s")==0)
	     Sflag = 1;
	   else if (strcmp(argv[j],"cre")==0)
	     CREflag = 1;
	    j++;
	   }
	  }

	Cfn = 0;
	np = argv;
	Line_num = 0; /* reset line number */
	while( ++Cfn <= N_files )
		if((Fd = fopen(*++np,"r")) == NULL)
			printf("as: can't open %s\n",*np);
		else{
			make_pass();
			fclose(Fd);
		}
	if( Err_count == 0 ){
		Pass++;
		re_init();
		Cfn = 0;
		np = argv;
		Line_num = 0;
		while( ++Cfn <= N_files)
			if((Fd = fopen(*++np,"r")) != NULL)
			    {
				make_pass();
				fclose(Fd);
			     }
			if (Sflag == 1)
			  {
			    printf ("\f");
			    stable (root);
			  }
			if (CREflag == 1)
			  {
			    printf ("\f");
			    cross (root);
			  }
		fprintf(Objfil,"S9030000FC\n"); /* at least give a decent ending */
		}
	exit(Err_count);
}

void initialize(void)
{
	FILE	*fopen();
	int	i = 0;

#ifdef DEBUG
	printf("Initializing\n");
#endif
	Err_count = 0;
	Pc	  = 0;
	Pass	  = 1;
	Lflag	  = 0;
	Cflag	  = 0;
	Ctotal	  = 0;
	Sflag	  = 0;
	CREflag   = 0;
	N_page	  = 0;

	if( strlen(Argv[1]) + 5 > MAXPATH )  /* room for ".s19" */
		fatal("File name too long");
	strcpy(Obj_name,Argv[1]); /* copy first file name into array */
	do {
	    if (Obj_name[i]=='.')
	       Obj_name[i]=0;
	}
	while (Obj_name[i++] != 0);
	strcat(Obj_name,".s19");  /* append .out to file name. */
	if( (Objfil = fopen(Obj_name,"w")) == NULL)
		fatal("Can't create object file");
	fwdinit();	/* forward ref init */
	localinit();	/* target machine specific init. */
}

void re_init(void)
{
#ifdef DEBUG
	printf("Reinitializing\n");
#endif
	Pc	= 0;
	E_total = 0;
	P_total = 0;
	Ctotal	= 0;
	N_page	= 0;
	fwdreinit();
}

void make_pass(void)
{
	size_t	n;
	int	c;
	int	toolong;

#ifdef DEBUG
	printf("Pass %d\n",Pass);
#endif
	while( fgets(Line,MAXBUF,Fd) != (char *)NULL ){
		/* strip LF or CR/LF; the last line need not have one */
		n = strlen(Line);
		toolong = NO;
		if( n > 0 && Line[n-1] == NEWLINE )
			Line[--n] = EOS;
		else if( (c = getc(Fd)) != EOF && c != NEWLINE ){
			toolong = YES;	/* longer than MAXBUF-1: drop the rest */
			while( (c = getc(Fd)) != EOF && c != NEWLINE )
				;
			}
		if( n > 0 && Line[n-1] == '\r' )
			Line[--n] = EOS;
		Line_num++;
		if( toolong )
			error("Line too long, truncated");
		P_force = 0;	/* No force unless bytes emitted */
		N_page = 0;
		   if(parse_line())
			process();
		if(Pass == 2 && Lflag && !N_page)
			print_line();
		P_total = 0;	/* reset byte count */
		Cycles = 0;	/* and per instruction cycle count */
		}
	f_record();
}


/*
 *	parse_line --- split input line into label, op and operand
 */
int parse_line(void)
{
	register char *ptrfrm = Line;
	register char *ptrto = Label;
	char	*skip_white();

	if( *ptrfrm == '*' || *ptrfrm == ';' || *ptrfrm == EOS )
		return(0);	/* a comment line */

	while( delim(*ptrfrm)== NO ){
		if( ptrto < Label+MAXLAB-1 )
			*ptrto++ = *ptrfrm;
		ptrfrm++;
		}
	if( *ptrfrm == ':' )	/* allow trailing : */
		ptrfrm++;
	*ptrto = EOS;

	ptrfrm = skip_white(ptrfrm);

	ptrto = Op;
	while( delim(*ptrfrm) == NO){
		if( ptrto < Op+MAXOP-1 )
			*ptrto++ = mapdn(*ptrfrm);
		ptrfrm++;
		}
	*ptrto = EOS;

	ptrfrm = skip_white(ptrfrm);

	ptrto = Operand;
	while( *ptrfrm != EOS )
		*ptrto++ = *ptrfrm++;
	*ptrto = EOS;

#ifdef DEBUG
	printf("Label-%s-\n",Label);
	printf("Op----%s-\n",Op);
	printf("Operand-%s-\n",Operand);
#endif
	return(1);
}

/*
 *	process --- determine mnemonic class and act on it
 */
void process(void)
{
	register struct oper *i;
	struct oper *mne_look();

	Old_pc = Pc;		/* setup `old' program counter */
	Optr = Operand; 	/* point to beginning of operand field */

	if(*Op==EOS){		/* no mnemonic */
		if(*Label != EOS)
			install(Label,Pc);
		}
	else if( (i = mne_look(Op))== NULL)
		errors("Unrecognized Mnemonic", Op);
	else if( i->class == PSEUDO )
		do_pseudo(i->opcode);
	else{
		if( *Label )install(Label,Pc);
		if(Cflag)Cycles = i->cycles;
		do_op(i->opcode,i->class);
		if(Cflag)Ctotal += Cycles;
		}
}

