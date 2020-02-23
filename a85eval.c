/*
	HEADER:		CUG267;
	TITLE:		8085 Cross-Assembler (Portable);
	FILENAME:	A85EVAL.C;
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

	DEC 2013	fixed as per fixed a68eval.c for lcc-win32 HRJ

This file contains the assembler's expression evaluator and lexical analyzer.
The lexical analyzer chops the input character stream up into discrete tokens
that are processed by the expression analyzer and the line assembler.  The
expression analyzer processes the token stream into unsigned results of
arithmetic expressions.
*/

/*  Get global goodies:  */

#include "a85.h"
#include <ctype.h>
#include <string.h>

/* from A18eval.c HRJ */
/* local  prototypes HRJ*/
static unsigned eval(unsigned);
static void exp_error(char);
void unlex(void);
TOKEN *lex(void);
static void make_number(unsigned);
int popc(void);
void pushc(char);
int isalph(char); /* was isalph(int) HRJ */
static int isnum(char), ischar(char), ishex(char);
static int isalpnum(char c);

/* external prototypes HRJ*/
void error(char);
void pops(char *), trash(void);
OPCODE *find_operator(char *);
SYMBOL *find_symbol(char *);

void asm_line(void);
void lclose(void), lopen(char *), lputs(void);
void hclose(void), hopen(char *), hputc(unsigned);
void error(char), fatal_error(char *), warning(char *);
void hseek(unsigned);
void unlex(void);
/* above from A68eval.c HRJ */



/*  Get access to global mailboxes defined in A85.C:			*/

extern char lline[]; //HRJ was line[] in A85.c
extern int filesp, forwd, pass;
extern unsigned pc;
extern FILE *filestk[], *source;
extern TOKEN token;

/*  Expression analysis routine.  The token stream from the lexical	*/
/*  analyzer is processed as an arithmetic expression and reduced to an	*/
/*  unsigned value.  If an error occurs during the evaluation, the	*/
/*  global flag	forwd is set to indicate to the line assembler that it	*/
/*  should not base certain decisions on the result of the evaluation.	*/

static int bad;

unsigned expr()
{
    SCRATCH unsigned u;
    // unsigned eval();

    bad = FALSE;
    u = eval(START);
    return bad ? 0 : u;
}

static unsigned eval(pre)
unsigned pre;
{
    register unsigned op, u, v;
    // TOKEN *lex();
    // void exp_error(), unlex();

    for (;;) {
	u = op = lex() -> valu;
	switch (token.attr & TYPE) {
	    case REG:	exp_error('S');  break;

	    case SEP:   // HRJ in a68eval.c  if (pre != START) unlex();
	    case EOL:	unlex();  exp_error('E');  return (u); /* return had no value HRJ*/

	    case OPR:	if (!(token.attr & UNARY)) { exp_error('E');  break; }
			// HRJ was u = eval((op == '+' || op == '-') ?
                        //      (unsigned) UOP1 : token.attr & PREC);
			u = (op == '*' ? pc :
			    eval((op == '+' || op == '-') ?
				(unsigned) UOP1 : token.attr & PREC)); //HRJ from a68eval.c
			switch (op) {
			    case '-':	u = word(0-u);  break; /* had (-u) HRJ */

			    case NOT:	u ^= 0xffff;  break;

			    case HIGH:	u = high(u);  break;

			    case LOW:	u = low(u);  break;
			}

	    case VAL:
	    case STR:	for (;;) {
			    op = lex() -> valu;
			    switch (token.attr & TYPE) {
				case REG:   exp_error('S');  break;

				case SEP:   //HRJ in a68eval.c  if (pre != START) unlex();
				case EOL:   if (pre == LPREN) exp_error('(');
					    unlex();  return u; //HRJ no unlex() in a68

				case STR:
				case VAL:   exp_error('E');  break;

				case OPR:   if (!(token.attr & BINARY)) {
						exp_error('E');  break;
					    }
					    if ((token.attr & PREC) >= pre) {
						unlex();  return u;
					    }
					    if (op != ')')
						v = eval(token.attr & PREC);
					    switch (op) {
						case '+':   u += v;  break;

						case '-':   u -= v;  break;

						case '*':   u *= v;  break;

						case '/':   u /= v;  break;

						case MOD:   u %= v;  break;

						case AND:   u &= v;  break;

						case OR:    u |= v;  break;

						case XOR:   u ^= v;  break;

						case '<':   u = u < v;  break;

						case LE:    u = u <= v;  break;

						case '=':   u = u == v;  break;

						case GE:    u = u >= v;  break;

						case '>':   u = u > v;  break;

						case NE:    u = u != v;  break;

						case SHL:   if (v > 15)
								exp_error('E');
							    else u <<= v;
							    break;

						case SHR:   if (v > 15)
								exp_error('E');
							    else u >>= v;
							    break;

						case ')':   if (pre == LPREN)
								return u;
							    exp_error('(');
							    break;
					    }
					    clamp(u);
					    break;
			    }
			}
			break;
	}
    }
}

static void exp_error(char c)

{
    forwd = bad = TRUE;  error(c);
}

/*  Lexical analyzer.  The source input character stream is chopped up	*/
/*  into its component parts and the pieces are evaluated.  Symbols are	*/
/*  looked up, operators are looked up, etc.  Everything gets reduced	*/
/*  to an attribute word, a numeric value, and (possibly) a string	*/
/*  value.								*/

static int oldt = FALSE;
static int quote = FALSE;

TOKEN *lex(void)
{
    SCRATCH char c, *p; // HRJ d not needed
    SCRATCH unsigned b;
    SCRATCH OPCODE *o;
    SCRATCH SYMBOL *s;

    /* OPCODE *find_operator();
    SYMBOL *find_symbol();
    void exp_error(), make_number(), pops(), pushc(), trash(); */

    if (oldt) { oldt = FALSE;  return &token; }
    trash();
    if (isalph(c = popc())) {
	pushc(c);  pops(token.sval);
	if (!strcmp(token.sval,"$")) {
	    token.attr = VAL;
	    token.valu = pc;
	}
	else if ((o = find_operator(token.sval))) {
	    token.attr = o -> attr;
	    token.valu = o -> valu;
	}
	else {
	    token.attr = VAL;  token.valu = 0;

	    if ((s = find_symbol(token.sval))) {
		token.valu = s -> valu;
		if (pass == 2 && s -> attr & FORWD) forwd = TRUE;
	    }
	    else exp_error('U');
	}
    }
    else if (isnum(c)) {
	pushc(c);  pops(token.sval);
	for (p = token.sval; *p; ++p);
	switch (toupper(*--p)) {
	    case 'B':	b = 2;  break;

	    case 'O':
	    case 'Q':	b = 8;  break;

	    default:	++p;
	    case 'D':	b = 10;  break;

	    case 'H':	b = 16;  break;
	}
	*p = '\0';  make_number(b);
    }
    else switch (c) {
        //HRJ a68 has %, @, $, #  cases

	case '(':   token.attr = UNARY + LPREN + OPR;
		    goto opr1;

	case ')':   token.attr = BINARY + RPREN + OPR;
		    goto opr1;

	case '+':   token.attr = BINARY + UNARY + ADDIT + OPR;
		    goto opr1;

	case '-':   token.attr = BINARY + UNARY + ADDIT + OPR;
		    goto opr1;

	case '*':   token.attr = BINARY + UNARY + MULT + OPR;
		    goto opr1;

	case '/':   token.attr = BINARY + MULT + OPR;
opr1:		    token.valu = c;
		    break;

	case '<':   token.valu = c;
		    if ((c = popc()) == '=') token.valu = LE;
		    else if (c == '>') token.valu = NE;
		    else pushc(c);
		    goto opr2;

	case '=':   token.valu = c;
		    if ((c = popc()) == '<') token.valu = LE;
		    else if (c == '>') token.valu = GE;
		    else pushc(c);
		    goto opr2;

	case '>':   token.valu = c;
		    if ((c = popc()) == '<') token.valu = NE;
		    else if (c == '=') token.valu = GE;
		    else pushc(c);
opr2:		    token.attr = BINARY + RELAT + OPR;
		    break;

	case '\'':
	case '"':   quote = TRUE;  token.attr = STR;
	        //HRJ following different in a68eval.c
		    // for (p = token.sval; ; ++p) {
			// if ((d = popc()) == '\n') { exp_error('"');  break; }
			// if ((*p = d) == c && (d=popc()) != c) break;
		    // }
		    // pushc(d); //HRJ end of difference
                    //HRJ I think this is a problem because popc() is evoked twice per FOR loop
                    // but only pushc() once after breaking loop. May miss newline.
			//HRJ replaced above with following from a68eval.c
		    for (p = token.sval; (*p = popc()) != c; ++p)
			   if (*p == '\n') { exp_error('"');  break; }
			//HRJ end of replacement. This fixed problem where expressions like
			//	DB<tab>"B";<crlf> or DB<tab>"B"<tab>;comment were "E" errors
	        *p = '\0';  quote = FALSE;
		    if ((token.valu = token.sval[0]) && token.sval[1])
			token.valu = (token.valu << 8) + token.sval[1];
		    break;

	case ',':   token.attr = SEP;
		    break;

        case '\n':  token.attr = EOL;
		    break;
    }
    return &token;
}

static void make_number(base)
unsigned base;
{
    SCRATCH char *p;
    SCRATCH unsigned d;
    /* void exp_error(); */

    token.attr = VAL;
    token.valu = 0;
    for (p = token.sval; *p; ++p) {
	d = toupper(*p) - (isnum(*p) ? '0' : 'A' - 10);
	token.valu = token.valu * base + d;
	if (!ishex(*p) || d >= base) { exp_error('D');  break; }
    }
    clamp(token.valu);
    return;
}

int isalph(char c) // HRJ slightly different from a68eval.c

{
    return (c >= '@' && c <= '~') || (c >= '#' && c <= '&') ||
	c == '!' || c == '.' || c == ':' || c == '?';
}

static int isnum(char c)

{
    return c >= '0' && c <= '9';
}

static int ishex(char c)

{
    return isnum(c) || ((c = toupper(c)) >= 'A' && c <= 'F');
}

static int isalpnum(char c) //HRJ changed to avoid ctype.h conflict

{
    return isalph(c) || isnum(c);
}

/*  Push back the current token into the input stream.  One level of	*/
/*  pushback is supported.						*/

void unlex()
{
    oldt = TRUE;
    return;
}

/*  Get an alphanumeric string into the string value part of the	*/
/*  current token.  Leading blank space is trashed.			*/

void pops(s)
char *s;
{
    // void pushc(), trash();

    trash();
    for (; isalpnum(*s = popc()); ++s);
    pushc(*s);  *s = '\0';
    return;
}

/*  Trash blank space and push back the character following it.		*/

void trash()
{
    SCRATCH char c;
    // void pushc();

    while ((c = popc()) == ' ');
    pushc(c);
    return;
}

/*  Get character from input stream.  This routine does a number of	*/
/*  other things while it's passing back characters.  All control	*/
/*  characters except \t and \n are ignored.  \t is mapped into ' '.	*/
/*  Semicolon is mapped to \n.  In addition, a copy of all input is set	*/
/*  up in a line buffer for the benefit of the listing.			*/

static int oldc, eol;
static char *lptr;

int popc(void)
{
    SCRATCH int c;

    if (oldc) { c = oldc;  oldc = '\0';  return c; }
    if (eol) return '\n';
    for (;;) {
	if ((c = getc(source)) != EOF && (c &= 0377) == ';' && !quote) {
	    do *lptr++ = c;
	    while ((c = getc(source)) != EOF && (c &= 0377) != '\n');
	}
	if (c == EOF) c = '\n';
	// HRJ could try if (c == '!' && !quote) { lptr ="!\n\0"
	// to treat ! as instruction seperator, force scan to break line
	// but too hard to force scanner to deal with opr!opr
	if ((*lptr++ = c) >= ' ' && c <= '~') return c;
	if (c == '\n') { eol = TRUE;  *lptr = '\0';  return '\n'; }
	if (c == '\t') return quote ? '\t' : ' ';
    }
}

/*  Push character back onto input stream.  Only one level of push-back	*/
/*  supported.  \0 cannot be pushed back, but nobody would want to.	*/

void pushc(char c)
{
    oldc = c;
    return;
}

/*  Begin new line of source input.  This routine returns non-zero if	*/
/*  EOF	has been reached on the main source file, zero otherwise.	*/

int newline()
{
    // void fatal_error();

    oldc = '\0';  lptr = lline;
    oldt = eol = FALSE;
    while (feof(source)) {
	if (ferror(source)) fatal_error(ASMREAD);
	if (filesp) {
	    fclose(source);
	    source = filestk[--filesp];
	}
	else return TRUE;
    }
    return FALSE;
}

