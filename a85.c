/*
	HEADER:		CUG267;
	TITLE:		8085 Cross-Assembler (Portable);
	FILENAME:	A85.C;
	VERSION:	0.1;
	DATE:		08/27/1988;
	SEE-ALSO:	A85.H;
	AUTHORS:	William C. Colley III;
*/

/*
		      8085 Cross-Assembler in Portable C

		Copyright (c) 1985,1987 William C. Colley, III

Revision History:

Ver	Date		Description

0.0	AUG 1987	Derived from version 3.4 of my portable 6800/6801
			cross-assembler.  WCC3.

0.1	AUG 1988	Fixed a bug in the command line parser that puts it
			into a VERY long loop if the user types a command line
			like "A85 FILE.ASM -L".  WCC3 per Alex Cameron.

0.1+	Dec 2013	edited for LCC-32 by Herb Johnson
				Mostly by updating function declarations,
				replacing () with (VOID), etc. SOme fixes need
				when char and int were used interchangably,
				unsigned was given signs, etc.
				labels ending with colon now have colon stripped
				All fixes have HRJ in comment lines    HRJ.


This file contains the main program and line assembly routines for the
assembler.  The main program parses the command line, feeds the source lines
to the line assembly routine, and sends the results to the listing and object
file output routines.  It also coordinates the activities of everything.  The
line assembly routines uses the expression analyzer and the lexical analyzer
to parse the source line and convert it into the object bytes that it
represents.
*/

/*  Get global goodies:  */

#include "a85.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* eternal routines HRJ*/

    void asm_line(void);
    void lclose(void), lopen(char *), lputs(void);
    void hclose(void), hopen(char *), hputc(unsigned);
    void error(char), fatal_error(char *), warning(char *);
	void lerror(void); // added to list error count HRJ

    void pops(char *), pushc(int), trash(void);
	void hseek(unsigned);
	void unlex(void);
	int isalph(char); /* was int isalph(int) HRJ */

	/* these are local but used before defined HRJ */
	static void do_label(void),normal_op(void), pseudo_op(void);
	static void flush(void);


/*  Define global mailboxes for all modules:				*/

/* Turbo C has "line" as graphic function, change to lline HRJ */

char errcode, lline[MAXLINE + 1], title[MAXLINE];
int pass = 0;
int eject, filesp, forwd, listhex;
unsigned  address, bytes, errors, listleft, obj[MAXLINE], pagelen, pc;
FILE *filestk[FILES], *source;
TOKEN token;

/*  Mainline routine.  This routine parses the command line, sets up	*/
/*  the assembler at the beginning of each pass, feeds the source text	*/
/*  to the line assembler, feeds the result to the listing and hex file	*/
/*  drivers, and cleans everything up at the end of the run.		*/

static int done, ifsp, off;

int main(argc,argv)
int argc;
char **argv;
{
    SCRATCH unsigned *o;
    int newline(void);

    /* void asm_line(void);
    void lclose(void), lopen(void), lputs(void);
    void hclose(void), hopen(void), hputc(void);
    void error(void), fatal_error(void), warning(char *); */

    printf("8085 Cross-Assembler (Portable) Ver 0.1\n");
    printf("Copyright (c) 1985,1987 William C. Colley, III\n\n");
    printf("fixes for LCC/Windows by HRJ Dec 2013\n\n");

    while (--argc > 0) {
	if (**++argv == '-') {
	    switch (toupper(*++*argv)) {
		case 'L':   if (!*++*argv) {
				if (!--argc) { warning(NOLST);  break; }
				else ++argv;
			    }
			    lopen(*argv);
			    break;

		case 'O':   if (!*++*argv) {
				if (!--argc) { warning(NOHEX);  break; }
				else ++argv;
			    }
			    hopen(*argv);
			    break;

		default:    warning(BADOPT);
	    }
	}
	else if (filestk[0]) warning(TWOASM);
	else if (!(filestk[0] = fopen(*argv,"r"))) fatal_error(ASMOPEN);
    }
    if (!filestk[0]) fatal_error(NOASM);

    while (++pass < 3) {
	fseek(source = filestk[0],0L,0);  done = off = FALSE;
	errors = filesp = ifsp = pagelen = pc = 0;  title[0] = '\0';
	while (!done) {
	    errcode = ' ';
	    if (newline()) {  // reach EOF instead of "END" statement
		error('*');
		strcpy(lline,"\tEND\t ;added by A85\n");
		done = eject = TRUE;  listhex = FALSE;
		bytes = 0;
	    }
	    else asm_line();
	    pc = word(pc + bytes);
	    if (pass == 2) {
		   if (done) lerror();
		   lputs();
		   for (o = obj; bytes--; hputc(*o++));
	    }
	}
    }

	// lerror();  //HRJ list error count
    fclose(filestk[0]);  lclose();  hclose();

    if (errors) printf("%d Error(s)\n",errors);
    else printf("No Errors\n");

    exit(errors);
}

/*  Line assembly routine.  This routine gets expressions and tokens	*/
/*  from the source file using the expression evaluator and lexical	*/
/*  analyzer, respectively.  It fills a buffer with the machine code	*/
/*  bytes and returns nothing.						*/

static char label[MAXLINE];
static int ifstack[IFDEPTH] = { ON };

static OPCODE *opcod;

void asm_line(void)
{
    SCRATCH char *p;
    SCRATCH int i;
    int popc(void);
    OPCODE *find_code(char *), *find_operator(char *);



    address = pc;  bytes = 0;  eject = forwd = listhex = FALSE;
    for (i = 0; i < BIGINST; obj[i++] = NOP);

    label[0] = '\0';
    if ((i = popc()) != ' ' && i != '\n') {
	if (isalph((char) i)) { //HRJ
	    pushc(i);  pops(label);
	/*HRJ need to remove colon from label? */
	    for (p = label; *p; ++p);
	    if (*--p == ':') *p = '\0';
	    if (find_operator(label)) { label[0] = '\0';  error('L'); }
	}
	else {
	    error('L');
	    while ((i = popc()) != ' ' && i != '\n');
	}
    }

    trash(); opcod = NULL;
    if ((i = popc()) != '\n') {
	if (!isalph((char) i)) error('S');
	else {
	    pushc(i);  pops(token.sval);
	    if (!(opcod = find_code(token.sval))) error('O');
	}
	if (!opcod) { listhex = TRUE;  bytes = BIGINST; }
    }

    if (opcod && opcod -> attr & ISIF) { if (label[0]) error('L'); }
    else if (off) { listhex = FALSE;  flush();  return; }

    if (!opcod) { do_label();  flush(); }
    else {
	listhex = TRUE;
	if (opcod -> attr & PSEUDO) pseudo_op();
	else normal_op();
	// HRJ this is where ! operator would be seen
	while ((i = popc()) != '\n') if (i != ' ') error('T');
    }

    source = filestk[filesp];
    return;
}

static void flush(void)
{
	int popc(void);

    while (popc() != '\n');
}

static void do_label(void)
{
    SCRATCH SYMBOL *l;
    SYMBOL *find_symbol(char *), *new_symbol(char *);

    if (label[0]) {
	listhex = TRUE;
	if (pass == 1) {
	    if (!((l = new_symbol(label)) -> attr)) {
		l -> attr = FORWD + VAL;
		l -> valu = pc;
	    }
	}
	else {
	    if ((l = find_symbol(label))) {
		l -> attr = VAL;
		if (l -> valu != pc) error('M');
	    }
	    else error('P');
	}
    }
}

static void normal_op(void)
{
    SCRATCH unsigned attrib, u;
    unsigned expr(void);
    TOKEN *lex(void);
    void do_label(void), unlex(void);

    do_label();  bytes = (attrib = opcod -> attr) & BYTES;
    if (pass < 2) return;
    obj[0] = opcod -> valu;  obj[1] = obj[2] = 0;

    while (attrib & ARG1) {
	lex();
	switch (attrib & ARG1) {
	    case DATA_16:   unlex();  obj[1] = low(u = expr());
			    obj[2] = high(u);  break;

	    case DATA_8:    unlex();
			    if ((u = expr()) > 0xff && u < 0xff80) {
				error('V');  u = 0;
			    }
			    obj[1] = low(u);  break;

	    case PORT:	    unlex();
			    if ((u = expr()) > 0xff) { error('V');  u = 0; }
			    obj[1] = low(u);  break;

	    case RST_NUM:   unlex();
			    if ((u = expr()) > 7) { error('V');  u = 0; }
			    obj[0] |= u << 3;  break;

	    case LDAX_REG:  u = BD;  goto do_reg;

	    case DAD_REG:   u = BDHSP;  goto do_reg;

	    case POP_REG:   u = BDHPSW;  goto do_reg;

	    case SRC_REG:   token.valu >>= 3;  u = BCDEHLMA;  goto do_reg;

	    case DST_REG:   u = BCDEHLMA;
do_reg:			    if ((token.attr & TYPE) != REG) {
				error('S');  break;
			    }
			    if (!(token.attr & u)) { error('R');  break; }
			    obj[0] |= token.valu;  break;
	}
	if (((attrib >>= 4) & ARG1) && (lex() -> attr & TYPE) != SEP) {
	    error('S');  break;
	}
	if (obj[0] == 0x76) error('R');
    }
}

static void pseudo_op(void)
{
    SCRATCH char *s;
    SCRATCH int c;
    SCRATCH unsigned *o, u;
    SCRATCH SYMBOL *l;
    unsigned expr(void);
    SYMBOL *find_symbol(char *), *new_symbol(char *);
    TOKEN *lex(void);

	int popc(void);


    /* void do_label(void), error(char), fatal_error(void), hseek(unsigned);
    void pushc(int), trash(void), unlex(void); */

    o = obj;
    switch (opcod -> valu) {
		case DB:    do_label();
		    do {
			switch (lex() -> attr & TYPE) {
			    case SEP:	unlex();  u = 0;  goto save_byte;

			    case STR:	trash();  pushc(c = popc());
					if (c == ',' || c == '\n') {
						//hrj wanted to add tab, semicolon
					    for (s = token.sval; *s;
						*o++ = *s++) ++bytes;
					    break;
					}

			    default:	unlex();
					if ((u = expr()) > 0xff &&
					    u < 0xff80) {
					    u = 0;  error('V');
					}
save_byte:				*o++ = low(u);  ++bytes;  break;
			}
		    } while ((lex() -> attr & TYPE) == SEP);
		    break;

	case DS:    do_label();
		    u = word(pc + expr());
		    if (forwd) error('P');
		    else {
			pc = u;
			if (pass == 2) hseek(pc);
		    }
		    break;

	case DW:    do_label();
		    do {
			lex();  unlex();
			u = ((token.attr & TYPE) == SEP) ? 0 : expr();
			*o++ = low(u);  *o++ = high(u);  bytes += 2;
		    } while ((lex() -> attr & TYPE) == SEP);
		    break;

	case ELSE:  listhex = FALSE;
		    if (ifsp) off = (ifstack[ifsp] = -ifstack[ifsp]) != ON;
		    else error('I');
		    break;

	case END:   do_label();
		    if (filesp) { listhex = FALSE;  error('*'); }
		    else {
			done = eject = TRUE;
			if (pass == 2) {
			    if ((lex() -> attr & TYPE) != EOL) {
				unlex();  hseek(address = expr());
			    }
			}
			if (ifsp) error('I');
		    }
		    break;

	case ENDIF: listhex = FALSE;
		    if (ifsp) off = ifstack[--ifsp] != ON;
		    else error('I');
		    break;

	case EQU:   if (label[0]) {
			if (pass == 1) {
			    if (!((l = new_symbol(label)) -> attr)) {
				l -> attr = FORWD + VAL;
				address = expr();
				if (!forwd) l -> valu = address;
			    }
			}
			else {
			    if ((l = find_symbol(label))) {
				l -> attr = VAL;
				address = expr();
				if (forwd) error('P');
				if (l -> valu != address) error('M');
			    }
			    else error('P');
			}
		    }
		    else error('L');
		    break;

	case IF:    if (++ifsp == IFDEPTH) fatal_error(IFOFLOW);
		    address = expr();
		    if (forwd) { error('P');  address = TRUE; }
		    if (off) { listhex = FALSE;  ifstack[ifsp] = ZERO; } /* was NULL but error HRJ*/
		    else {
			ifstack[ifsp] = address ? ON : OFF;
			if (!address) off = TRUE;
		    }
		    break;

	case INCL:  listhex = FALSE;  do_label();
		    if ((lex() -> attr & TYPE) == STR) {
			if (++filesp == FILES) fatal_error(FLOFLOW);
			if (!(filestk[filesp] = fopen(token.sval,"r"))) {
			    --filesp;  error('V');
			}
		    }
		    else error('S');
		    break;

	case ORG:   u = expr();
		    if (forwd) error('P');
		    else {
			pc = address = u;
			if (pass == 2) hseek(pc);
		    }
		    do_label();
		    break;

	case PAGE:  listhex = FALSE;  do_label();
		    if ((lex() -> attr & TYPE) != EOL) {
			unlex();
			if ((pagelen = expr()) > 0 && pagelen < 3) {
			    pagelen = 0;  error('V');
			}
		    }
		    eject = TRUE;
		    break;

	case SET:   if (label[0]) {
			if (pass == 1) {
			    if (!((l = new_symbol(label)) -> attr)
				|| (l -> attr & SOFT)) {
				l -> attr = FORWD + SOFT + VAL;
				address = expr();
				if (!forwd) l -> valu = address;
			    }
			}
			else {
			    if ((l = find_symbol(label))) {
				address = expr();
				if (forwd) error('P');
				else if (l -> attr & SOFT) {
				    l -> attr = SOFT + VAL;
				    l -> valu = address;
				}
				else error('M');
			    }
			    else error('P');
			}
		    }
		    else error('L');
		    break;

	case TITLE: listhex = FALSE;  do_label();
		    if ((lex() -> attr & TYPE) == EOL) title[0] = '\0';
		    else if ((token.attr & TYPE) != STR) error('S');
		    else strcpy(title,token.sval);
		    break;
    }
    return;
}
