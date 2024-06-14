/* A85 Cross Assembler in Portable C
 *
 * Copyright (c) 1985,1987 William C. Colley, III
 * Copyright (c) 2013 Herb Johnson
 * Copyright (c) 2020 The Glitch Works
 *
 * This is a modified version of William C. Colley III's A85 cross assembler
 * in "portable C." Modifications included from Herb Johnson and The Glitch
 * Works. See README in project root for more information.
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
This module contains the following utility packages:

	1)  symbol table building and searching

	2)  opcode and operator table searching

	3)  listing file output

	4)  hex file output

	5)  error flagging
*/

/*  Get global goodies:  */
#include "a85.h"
#include <string.h> /* HRJ */
#include <ctype.h>
// #include <malloc.h> /* for lcc-32 HRJ */
/* #include <alloc.h> for Turbo C HRJ */
#include <stdlib.h>

/*  Make sure that MSDOS compilers using the large memory model know	*/
/*  that calloc() returns pointer to char as an MSDOS far pointer is	*/
/*  NOT compatible with the int type as is usually the case.		*/

/* char *calloc(); HRJ */

/*HRJ local declarations */

static OPCODE *bccsearch(OPCODE *, OPCODE *, char *);
static void list_sym(SYMBOL *);
static void record(unsigned);
static void putb(unsigned);
static int ustrcmp(char *, char*);
static void check_page(void);
void warning(char *);
void fatal_error(char *);


/*  Get access to global mailboxes defined in A85.C:			*/

extern char errcode, lline[], title[];
extern int eject, listhex;
extern unsigned address, bytes, errors, listleft, obj[], pagelen;

/*  The symbol table is a binary tree of variable-length blocks drawn	*/
/*  from the heap with the calloc() function.  The root pointer lives	*/
/*  here:								*/

static SYMBOL *sroot = NULL;

/*  Add new symbol to symbol table.  Returns pointer to symbol even if	*/
/*  the symbol already exists.  If there's not enough memory to store	*/
/*  the new symbol, a fatal error occurs.				*/

SYMBOL *new_symbol(char *nam)

{
    SCRATCH int i;
    SCRATCH SYMBOL **p, *q;
    void fatal_error(char *);

    /* printf("new_symbol>>%s<<\n",nam);  HRJ diagnostic*/

    for (p = &sroot; (q = *p) && (i = strcmp(nam,q -> sname)); )
	p = i < 0 ? &(q -> left) : &(q -> right);
    if (!q) {
	if (!(*p = q = (SYMBOL *)calloc(1,sizeof(SYMBOL) + strlen(nam))))
	    fatal_error(SYMBOLS);
	strcpy(q -> sname,nam);
    }
    return q;
}

/*  Look up symbol in symbol table.  Returns pointer to symbol or NULL	*/
/*  if symbol not found.						*/

SYMBOL *find_symbol(char *nam)

{
    SCRATCH int i;
    SCRATCH SYMBOL *p;

    for (p = sroot; p && (i = strcmp(nam,p -> sname));
	p = i < 0 ? p -> left : p -> right);
    return p;
}

/*  Opcode table search routine.  This routine pats down the opcode	*/
/*  table for a given opcode and returns either a pointer to it or	*/
/*  NULL if the opcode doesn't exist.					*/

OPCODE *find_code(char *nam)

{
    /* OPCODE *bsearch(); */

    static OPCODE opctbl[] = {
	{ DATA_8 + 2,				0xce,	"ACI"	},
	{ SRC_REG + 1,				0x88,	"ADC"	},
	{ SRC_REG + 1,				0x80,	"ADD"	},
	{ DATA_8 + 2,				0xc6,	"ADI"	},
	{ SRC_REG + 1,				0xa0,	"ANA"	},
	{ DATA_8 + 2,				0xe6,	"ANI"	},
	{ DATA_16 + 3,				0xcd,	"CALL"	},
	{ DATA_16 + 3,				0xdc,	"CC"	},
	{ DATA_16 + 3,				0xfc,	"CM"	},
	{ NONE + 1,				0x2f,	"CMA"	},
	{ NONE + 1,				0x3f,	"CMC"	},
	{ SRC_REG + 1,				0xb8,	"CMP"	},
	{ DATA_16 + 3,				0xd4,	"CNC"	},
	{ DATA_16 + 3,				0xc4,	"CNZ"	},
	{ DATA_16 + 3,				0xf4,	"CP"	},
	{ DATA_16 + 3,				0xec,	"CPE"	},
	{ DATA_8 + 2,				0xfe,	"CPI"	},
	{ DATA_16 + 3,				0xe4,	"CPO"	},
	{ DATA_16 + 3,				0xcc,	"CZ"	},
	{ NONE + 1,				0x27,	"DAA"	},
	{ DAD_REG + 1,				0x09,	"DAD"	},
	{ PSEUDO,				DB,	"DB"	},
	{ DST_REG + 1,				0x05,	"DCR"	},
	{ DAD_REG + 1,				0x0b,	"DCX"	},
	{ NONE + 1,				0xf3,	"DI"	},
	{ PSEUDO,				DS,	"DS"	},
	{ PSEUDO,				DW,	"DW"	},
	{ NONE + 1,				0xfb,	"EI"	},
	{ PSEUDO + ISIF,			ELSE,	"ELSE"	},
	{ PSEUDO,				END,	"END"	},
	{ PSEUDO + ISIF,			ENDIF,	"ENDIF"	},
	{ PSEUDO,				EQU,	"EQU"	},
	{ NONE + 1,				0x76,	"HLT"	},
	{ PSEUDO + ISIF,			IF,	"IF"	},
	{ PSEUDO + ISIF,			IFDEF,	"IFDEF"	},
	{ PSEUDO + ISIF,			IFNDEF,	"IFNDEF"},
	{ PORT + 2,				0xdb,	"IN"	},
	{ PSEUDO,				INCL,	"INCL"	},
	{ PSEUDO,				INCL,	"INCLUDE"},
	{ DST_REG + 1,				0x04,	"INR"	},
	{ DAD_REG + 1,				0x03,	"INX"	},
	{ DATA_16 + 3,				0xda,	"JC"	},
	{ DATA_16 + 3,				0xfa,	"JM"	},
	{ DATA_16 + 3,				0xc3,	"JMP"	},
	{ DATA_16 + 3,				0xd2,	"JNC"	},
	{ DATA_16 + 3,				0xc2,	"JNZ"	},
	{ DATA_16 + 3,				0xf2,	"JP"	},
	{ DATA_16 + 3,				0xea,	"JPE"	},
	{ DATA_16 + 3,				0xe2,	"JPO"	},
	{ DATA_16 + 3,				0xca,	"JZ"	},
	{ DATA_16 + 3,				0x3a,	"LDA"	},
	{ LDAX_REG + 1,				0x0a,	"LDAX"	},
	{ DATA_16 + 3,				0x2a,	"LHLD"	},
	{ DAD_REG + (DATA_16 << 4) + 3,		0x01,	"LXI"	},
	{ DST_REG + (SRC_REG << 4) + 1,		0x40,	"MOV"	},
	{ DST_REG + (DATA_8 << 4) + 2,		0x06,	"MVI"	},
	{ NONE + 1,				0x00,	"NOP"	},
	{ SRC_REG + 1,				0xb0,	"ORA"	},
	{ PSEUDO,				ORG,	"ORG"	},
	{ DATA_8 + 2,				0xf6,	"ORI"	},
	{ PORT + 2,				0xd3,	"OUT"	},
	{ PSEUDO,				PAGE,	"PAGE"	},
	{ NONE + 1,				0xe9,	"PCHL"	},
	{ POP_REG + 1,				0xc1,	"POP"	},
	{ PSEUDO,				PRINT,	"PRINT" },
	{ POP_REG + 1,				0xc5,	"PUSH"	},
	{ NONE + 1,				0x17,	"RAL"	},
	{ NONE + 1,				0x1f,	"RAR"	},
	{ NONE + 1,				0xd8,	"RC"	},
	{ NONE + 1,				0xc9,	"RET"	},
	{ NONE + 1,				0x20,	"RIM"	},
	{ NONE + 1,				0x07,	"RLC"	},
	{ NONE + 1,				0xf8,	"RM"	},
	{ NONE + 1,				0xd0,	"RNC"	},
	{ NONE + 1,				0xc0,	"RNZ"	},
	{ NONE + 1,				0xf0,	"RP"	},
	{ NONE + 1,				0xe8,	"RPE"	},
	{ NONE + 1,				0xe0,	"RPO"	},
	{ NONE + 1,				0x0f,	"RRC"	},
	{ RST_NUM + 1,				0xc7,	"RST"	},
	{ NONE + 1,				0xc8,	"RZ"	},
	{ SRC_REG + 1,				0x98,	"SBB"	},
	{ DATA_8 + 2,				0xde,	"SBI"	},
	{ PSEUDO,				SET,	"SET"	},
	{ DATA_16 + 3,				0x22,	"SHLD"	},
	{ NONE + 1,				0x30,	"SIM"	},
	{ NONE + 1,				0xf9,	"SPHL"	},
	{ DATA_16 + 3,				0x32,	"STA"	},
	{ LDAX_REG + 1,				0x02,	"STAX"	},
	{ NONE + 1,				0x37,	"STC"	},
	{ SRC_REG + 1,				0x90,	"SUB"	},
	{ DATA_8 + 2,				0xd6,	"SUI"	},
	{ PSEUDO,				TITLE,	"TITLE"	},
	{ NONE + 1,				0xeb,	"XCHG"	},
	{ SRC_REG + 1,				0xa8,	"XRA"	},
	{ DATA_8 + 2,				0xee,	"XRI"	},
	{ NONE + 1,				0xe3,	"XTHL"	}
    };

    return bccsearch(opctbl,opctbl + (sizeof(opctbl) / sizeof(OPCODE)),nam);
}

/*  Operator table search routine.  This routine pats down the		*/
/*  operator table for a given operator and returns either a pointer	*/
/*  to it or NULL if the opcode doesn't exist.				*/

OPCODE *find_operator(char *nam)

{
    /* OPCODE *bsearch(); */

    static OPCODE oprtbl[] = {
	{ BCDEHLMA + REG,				A,	"A"	},
	{ BINARY + LOG1  + OPR,				AND,	"AND"	},
	{ BCDEHLMA + BDHPSW + BDHSP + BD + REG,		B,	"B"	},
	{ BCDEHLMA + REG,				C,	"C"	},
	{ BCDEHLMA + BDHPSW + BDHSP + BD + REG,		D,	"D"	},
	{ BCDEHLMA + REG,				E,	"E"	},
	{ BINARY + RELAT + OPR,				'=',	"EQ"	},
	{ BINARY + RELAT + OPR,				GE,	"GE"	},
	{ BINARY + RELAT + OPR,				'>',	"GT"	},
	{ BCDEHLMA + BDHPSW + BDHSP + REG,		H,	"H"	},
	{ UNARY  + UOP3  + OPR,				HIGH,	"HIGH"	},
	{ BCDEHLMA + REG,				L,	"L"	},
	{ BINARY + RELAT + OPR,				LE,	"LE"	},
	{ UNARY  + UOP3  + OPR,				LOW,	"LOW"	},
	{ BINARY + RELAT + OPR,				'<',	"LT"	},
	{ BCDEHLMA + REG,				M,	"M"	},
	{ BINARY + MULT  + OPR,				MOD,	"MOD"	},
	{ BINARY + RELAT + OPR,				NE,	"NE"	},
	{ UNARY  + UOP2  + OPR,				NOT,	"NOT"	},
	{ BINARY + LOG2  + OPR,				OR,	"OR"	},
	{ BDHPSW + REG,					PSW,	"PSW"	},
	{ BINARY + MULT  + OPR,				SHL,	"SHL"	},
	{ BINARY + MULT  + OPR,				SHR,	"SHR"	},
	{ BDHSP + REG,					SP,	"SP"	},
	{ BINARY + LOG2  + OPR,				XOR,	"XOR"	}
    };

    return bccsearch(oprtbl,oprtbl + (sizeof(oprtbl) / sizeof(OPCODE)),nam);
}

static OPCODE *bccsearch(OPCODE *lo, OPCODE *hi, char *nam)

{
    SCRATCH int i;
    SCRATCH OPCODE *chk;

    for (;;) {
	chk = lo + (hi - lo) / 2;
	if (!(i = ustrcmp(chk -> oname,nam))) return chk;
	if (chk == lo) return NULL;
	if (i < 0) lo = chk;
	else hi = chk;
    }
}

static int ustrcmp(char *s, char *t)

{
    SCRATCH int i;

    while (!(i = toupper(*s++) - toupper(*t)) && *t++);
    return i;
}

/*  Buffer storage for line listing routine.  This allows the listing	*/
/*  output routines to do all operations without the main routine	*/
/*  having to fool with it.						*/

static FILE *list = NULL;

/*  Listing file open routine.  If a listing file is already open, a	*/
/*  warning occurs.  If the listing file doesn't open correctly, a	*/
/*  fatal error occurs.  If no listing file is open, all calls to	*/
/*  lputs() and lclose() have no effect.				*/

void lopen(char *nam)

{

    if (list) warning(TWOLST);
    else if (!(list = fopen(nam,"w"))) fatal_error(LSTOPEN);
    return;
}

/*  Listing file line output routine.  This routine processes the	*/
/*  source line saved by popc() and the output of the line assembler in	*/
/*  buffer obj into a line of the listing.  If the disk fills up, a	*/
/*  fatal error occurs.							*/

void lputs(void)
{
    SCRATCH int i, j;
    SCRATCH unsigned *o;
    void fatal_error(char *);

    if (list) {
	i = bytes;  o = obj;
	do {
	    fprintf(list,"%c  ",errcode);
	    if (listhex) {
		fprintf(list,"%04x  ",address);
		for (j = 4; j; --j) {
		    if (i) { --i;  ++address;  fprintf(list," %02x",*o++); }
		    else fprintf(list,"   ");
		}
	    }
	    else fprintf(list,"%18s","");
	    fprintf(list,"   %s",lline);  strcpy(lline,"\n");
	    check_page();
	    if (ferror(list)) fatal_error(DSKFULL);
	} while (listhex && i);
    }
    return;
}

/*  Listing file close routine.  The symbol table is appended to the	*/
/*  listing in alphabetic order by symbol name, and the listing file is	*/
/*  closed.  If the disk fills up, a fatal error occurs.		*/

static int col = 0;

void lerror(void)
{
	 if (errors && list) fprintf(list, "%d Error(s)\n",errors); //hrj
}

void lclose(void)
{

    if (list) {
	if (sroot) {
	    list_sym(sroot);
	    if (col) fprintf(list,"\n");
	}
	fprintf(list,"\f");
	if (ferror(list) || fclose(list) == EOF) fatal_error(DSKFULL);
    }
    return;
}

static void list_sym(SYMBOL *sp)

{

    if (sp) {
	list_sym(sp -> left);
	fprintf(list,"%04x  %-10s",sp -> valu,sp -> sname);

	if ((col = (col + 1) % SYMCOLS)) fprintf(list,"    ");
	else {
	    fprintf(list,"\n");
	    if (sp -> right) check_page();
	}
	list_sym(sp -> right);
    }
    return;
}

static void check_page(void)
{
    if (pagelen && !--listleft) eject = TRUE;
    if (eject) {
	eject = FALSE;  listleft = pagelen;  fprintf(list,"\f");
	if (title[0]) { listleft -= 2;  fprintf(list,"%s\n\n",title); }
    }
    return;
}

/*  Buffer storage for hex output file.  This allows the hex file	*/
/*  output routines to do all of the required buffering and record	*/
/*  forming without the	main routine having to fool with it.		*/

static FILE *hex = NULL;
static unsigned cnt = 0;
static unsigned addr = 0;
static unsigned sum = 0;
static unsigned buf[HEXSIZE];

/*  Hex file open routine.  If a hex file is already open, a warning	*/
/*  occurs.  If the hex file doesn't open correctly, a fatal error	*/
/*  occurs.  If no hex file is open, all calls to hputc(), hseek(), and	*/
/*  hclose() have no effect.						*/

void hopen(char *nam)

{

    if (hex) warning(TWOHEX);
    else if (!(hex = fopen(nam,"w"))) fatal_error(HEXOPEN);
    return;
}

/*  Hex file write routine.  The data byte is appended to the current	*/
/*  record.  If the record fills up, it gets written to disk.  If the	*/
/*  disk fills up, a fatal error occurs.				*/

void hputc(unsigned c) // from hputc() HRJ

{

    if (hex) {
	buf[cnt++] = c;
	if (cnt == HEXSIZE) record(0);
    }
    return;
}

/*  Hex file address set routine.  The specified address becomes the	*/
/*  load address of the next record.  If a record is currently open,	*/
/*  it gets written to disk.  If the disk fills up, a fatal error	*/
/*  occurs.								*/

void hseek(unsigned a)

{

    if (hex) {
	if (cnt) record(0);
	addr = a;
    }
    return;
}

/*  Hex file close routine.  Any open record is written to disk, the	*/
/*  EOF record is added, and file is closed.  If the disk fills up, a	*/
/*  fatal error occurs.							*/

void hclose(void)
{

    if (hex) {
	   if (cnt) record(0);
	   record(1);
	   if (fclose(hex) == EOF) fatal_error(DSKFULL);
    }
    return;
}

static void record(unsigned typ)

{
    SCRATCH unsigned i;

	putc(':',hex);  putb(cnt);  putb(high(addr));
    putb(low(addr));  putb(typ);
    for (i = 0; i < cnt; ++i) putb(buf[i]);
    putb(low(0-sum));  putc('\n',hex); /* was (-sum) HRJ*/

    addr += cnt;  cnt = 0;

    if (ferror(hex)) fatal_error(DSKFULL);
    return;
}

static void putb(unsigned b)

{
    static char digit[] = "0123456789ABCDEF";

    putc(digit[b >> 4],hex);  putc(digit[b & 0x0f],hex);
    sum += b;  return;
}

/*  Error handler routine.  If the current error code is non-blank,	*/
/*  the error code is filled in and the	number of lines with errors	*/
/*  is adjusted.							*/

void error(char code)

{
    if (errcode == ' ') { errcode = code;  ++errors; }
    return;
}

/*  Fatal error handler routine.  A message gets printed on the stderr	*/
/*  device, and the program bombs.					*/

void fatal_error(char *msg)

{
    printf("Fatal Error -- %s\n",msg);
    exit(-1);
}

/*  Non-fatal error handler routine.  A message gets printed on the	*/
/*  stderr device, and the routine returns.				*/

void warning(char *msg)

{
    printf("Warning -- %s\n",msg);
    return;
}

