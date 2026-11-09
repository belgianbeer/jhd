/*
 *  Jhd - Japanese Hexdecimal Dump
 *
 *    Written by Masato Minda
 *               minmin@strauss.uec.junet
 *
 *    Created Sep. 11, 1986  Version 1.0
 *            Sep. 14, 1988  Version 2.0
 *    modified
 *      Jan. 20, 1989  Ver. 2.1
 *        little bug fix in octal-word.
 *      Feb. 23, 1989  Ver. 2.2
 *        correct offset, file is stdin.
 */
static char rcsid[] =
	"@(#)$Id: jhd.c,v 2.4 1999/01/27 03:30:30 minmin Exp $";
/*
 *  definition of System type
 */
#define  UNIX    1        /*  in UNIX  */
#define  OS9     0        /*  in OS9 68000 */
#define  LSI     0        /*  in LSI-C  */

#define  EUC     1        /*  if set, kanji code is EUC (Shift-JIS default)  */

#include  <stdio.h>
#include  <ctype.h>
#include  <unistd.h>

#define  BSIZE      16384       /*  input buffer size */

#define  T_ASCII        0       /*  ascii terminal  */
#define  T_KANA         1       /*  kana terminal  */
#define  T_KANJI        2       /*  kanji terminal  */

#define  HEXDMP         0       /*  dump in hexdecimal  */
#define  CHRDMP         1       /*  dump in charactor  */
#define  OCTDMP         2       /*  dump in octal word  */
#define  OCTBYT         3       /*  dump in octal byte  */
#define  OCTREV         4       /*  dump in octal reverse  */

#define  KSHIFT      0x8e       /*  JAE kana shift in  */

#if  OS9
#	define  EXSTAT   0
#	define  isatty(n)  (_gs_size(n) == -1)
#else
#	define  EXSTAT   1
#endif

#if   LSI
	unsigned char    Ibuf[BSIZE];
#endif

unsigned char    Buf[BSIZE];             /*  input buffer  */
unsigned char    Tbuf[64];
unsigned char   *Lbuf = NULL;            /*  last 16 byte  */

char   *Prgnam = "jhd";
long    Offset;                 /*  dump offset  */
long    Addr;                   /*  current address  */
int     Bsize = BSIZE;
int     Admode = 0;             /*  address display mode  */
FILE   *Fp;                     /*  file pointer  */
int     Ttype;                  /*  terminal type  */
int     Skip = 0;               /*  skip next data  (kanji2 printed)  */
int     Dwidth = 16;            /*  dump width in byte  */
int     Putast;                 /*  '*' display flag  */
int     Mode = HEXDMP;          /*  dump mode  */
int     Vflag = 0;              /*  visual option  */

#if  LSI
	extern unsigned char    _osmajor;    /*  MS-DOS version number  */
#endif

#if  EUC

int     Kskip = 0;

#ifdef  iskanji
#	undef  iskanji
#endif
#ifdef  iskanji2
#	undef  iskanji2
#endif
#ifdef iskana
#	undef  iskana
#endif
#ifdef iskana2
#	undef  iskana2
#endif

int
iskanji(c)
    register int    c;
{
	c &= 0xff;
	return (c >= 0xa1 && c <= 0xfe);
}

#define  iskana(c)	(c == KSHIFT)

int
iskana2(c)
    register int     c;
{
	c &= 0xff;
	return (c >= 0xa1 && c <= 0xdf);
}

#else  /*  EUC  */

/*
 *   for kanji kana mode
 */
#ifndef  iskanji
int    iskanji(c)
register int    c;
{
	c &= 0xff;
	return (c >= 0x81 && c <= 0x9f || c >= 0xe0 && c <= 0xfc);
}
#endif

#ifndef  iskanji2
int    iskanji2(c)
    register int    c;
{
	c &= 0xff;
	return (c >= 0x40 && c <= 0x7e || c >= 0x80 && c <= 0xfc);
}
#endif

#ifndef  iskana
int    iskana(c)
register int    c;
{
	c &= 0xff;
	return (c >= 0xa1 && c <= 0xdf);
}
#endif

#endif  /*  EUC  */

/*
 *  set_ttype - check terminal type
 */
void
set_ttype ()
{
#if  UNIX || OS9
	char   *getenv ();
	char   *p;

	if ((p = getenv ("TTYPE")) == NULL) {
		Ttype = T_ASCII;
	} else if (!strcmp(p, "mskanji") || !strcmp(p, "jiskanji")) {
		Ttype = T_KANJI;
#if  EUC == 0
	} else if (!strcmp(p, "kana")) {
		Ttype = T_KANA;
#endif
	} else {
		Ttype = T_ASCII;
	}
#endif  /*  UNIX || OS9  */
#if  LSI
	Ttype = T_KANJI;
#endif
}

/*
 *  Kindex returns a pointer to the first occurrence of character
 *  c in string s, or zero if c does not occur in  the string.
 *  checks at shift-JIS code.
 */
char   *kindex(s, c)
register char   *s;
register int     c;
{
	while (*s) {
		if (*s == c) {
			return s;
		}
		if (iskanji(*s)) {
			++s;
		}
		++s;
	}
	return NULL;
}

void putspc (n)
register int     n;
{
	while (n--) {
		putchar (' ');
	}
}

void putoct (n, w)
register long   n;
int    w;
{
	register int     c;

	if (w > 1) {
		putoct (n >> 3, w - 1);
	}
	c = n & 0x7;
	putchar (c + '0');
}

void
puthex (n, w)
register long   n;
int    w;
{
	register int     c;

	if (w > 1) {
		puthex (n >> 4, w - 1);
	}
	c = n & 0xf;
	c += c <= 9 ? '0': '7';
	putchar (c);
}

/*
 *  chksame - check n byte same data
 */
int    chksame(new, old, n)
register unsigned char   *new;
register unsigned char   *old;
register int     n;
{
	if (old == NULL) {
		return 0;
	}
	while (n--) {
		if (*new++ != *old++) {
			return 0;
		}
	}
	return 1;
}

#if  EUC

/*
 *  dmpkanji - dump in kanji with JAE-Kanji
 */
void
dmpkanji (base, size)
register unsigned char   *base;
register int     size;
{
	register int    c;

	putchar (' ');
	while (size--) {
		c = *base++;
		if (Skip || Kskip) {
			Kskip = Skip = 0;
			putchar (' ');
		} else if (size == 0 && iskanji(c)) {
			Skip = c;
		} else if (size == 0 && iskana(c)) {
			Kskip = c;
		} else if (iskanji(c) && iskanji(*base)) {
			putchar (c);
			putchar (*base);
			--size;
			++base;
		} else if (iskana(c) && iskana2(c)) {
			putchar (c);
			putchar (*base);
			--size;
			++base;
		} else if (isprint(c)) {
			putchar (c);
		} else {
			putchar ('.');
		}
	}
}

void
dmpkana (base, size)
unsigned char   *base;
int     size;
{
	/*  not called in EUC  */
}

#else  /*  EUC  */

/*
 *  dmpkanji - dump in kanji with Shift-JIS
 */
void
dmpkanji (base, size)
register unsigned char   *base;
register int     size;
{
	register int    c;

	putchar (' ');
	while (size--) {
		c = *base++;
		if (Skip) {
			Skip = 0;
			putchar (' ');
		} else if (size == 0 && iskanji(c)) {
			Skip = c;
		} else if (iskanji(c) && iskanji2(*base)) {
			putchar (c);
			putchar (*base);
			--size;
			++base;
		} else if (iskana(c) || isprint(c)) {
			putchar (c);
		} else {
			putchar ('.');
		}
	}
}

/*
 *  dmpkana - dump in hankaku katakana
 */
void
dmpkana (base, size)
register unsigned char   *base;
register int     size;
{
	register int     c;

	putchar (' ');
	while (size--) {
		c = *base++;
		if (isprint(c) || iskana(c)) {
			putchar (c);
		} else {
			putchar ('.');
		}
	}
}

#endif  /*  EUC  */

/*
 *  dmpascii - dump in ascii charactor
 */
void
dmpascii (base, size)
register unsigned char   *base;
register int     size;
{
	register int     c;

	putchar (' ');
	while (size--) {
		c = *base++;
		if (isprint(c)) {
			putchar (c);
		} else {
			putchar ('.');
		}
	}
}

/*
 *  octdmp0 - dump octal in word
 */
void
octdmp0 (base, size)
register unsigned char   *base;
register int     size;
{
	while (size > 0) {
		putchar (' ');
		putoct ((long)((unsigned)(base[0] << 8) + (unsigned)base[1]), 6);
		base += 2;
		size -= 2;
	}
}

/*
 *  octdmp1 - dump octal in byte
 */
void
octdmp1 (base, size, adr)
register unsigned char   *base;
register int     size;
register int     adr;
{
	while (size--) {
		putchar (' ');
		putoct ((long)((unsigned)*base++), 3);
		if (++adr == 8) {
			putchar (' ');
		}
	}
}

/*
 *  octdmp2 - dump octal in reverse word
 */
void
octdmp2 (base, size)
register unsigned char   *base;
register int     size;
{
	while (size > 0) {
		putchar (' ');
		putoct ((long)((unsigned)(base[1] << 8) + (unsigned)base[0]), 6);
		base += 2;
		size -= 2;
	}
}

/*
 * hdmp - dump hexdecimal
 */
void
hexdmp (base, size, adr)
register unsigned char   *base;
register int     size;
register int     adr;
{
	while (size--) {
		putchar (' ');
		puthex ((long)((unsigned)*base++), 2);
		if (++adr == 8) {
			putchar (' ');
		}
	}
}

void
dmpmode (buf, size, adv)
register unsigned char   *buf;
register int     size;
register int     adv;
{
	Putast = 0;
#if  EUC
	if (Kskip) {
		if (iskana2(*buf)) {
			putchar (Kskip);
			putchar (*buf);
			putchar ('\n');
		} else {
			putchar ('.');
			putchar ('\n');
			Kskip = 0;
		}
	}
	if (Skip) {
		if (iskanji(*buf)) {
			putchar (Skip);
			putchar (*buf);
			putchar ('\n');
		} else {
			putchar ('.');
			putchar ('\n');
			Skip = 0;
		}
	}
#else  /*  EUC  */
	if (Skip) {
		if (iskanji2(*buf)) {
			putchar (Skip);
			putchar (*buf);
			putchar ('\n');
		} else {
			putchar ('.');
			putchar ('\n');
			Skip = 0;
		}
	}
#endif  /*  EUC  */
	if (Admode) {
		putoct (Addr, 11);
	} else {
		puthex (Addr, 8);
	}
	putchar (' ');
	buf += adv;
	switch (Mode) {

	case 0:  /*  hex dump with ascii */
		putspc (adv * 3 + (adv >= 8? 1: 0));
		hexdmp (buf, size, adv);
		putspc ((Dwidth - (size + adv)) * 3 + ((size + adv) < 8? 2: 1) + adv);
		dmpascii (buf, size);
		break;

	case 1:  /*  hex dump with kana */
		putspc (adv * 3 + (adv >= 8? 1: 0));
		hexdmp (buf, size, adv);
		putspc ((Dwidth - (size + adv)) * 3 + ((size + adv) < 8? 2: 1) + adv);
		dmpkana (buf, size);
		break;
	
	case 2:  /*  hex dump with kanji  */
		putspc (adv * 3 + (adv >= 8? 1: 0));
		hexdmp (buf, size, adv);
		putspc ((Dwidth - (size + adv)) * 3 + ((size + adv) < 8? 2: 1) + adv);
		dmpkanji (buf, size);
		break;

	case 3:  /*  char dump in ascii  */
		putspc (adv);
		dmpascii (buf, size);
		break;

	case 4:  /*  char dump in kana  */
		putspc (adv);
		dmpkana (buf, size);
		break;

	case 5:  /*  char dump in kanji  */
		putspc (adv);
		dmpkanji (buf, size);
		break;

	case 6:  /*  octal dump in word  */
	case 7:
	case 8:
		putspc ((adv / 2) * 7);
		octdmp0 (buf, size);
		break;

	case 9:  /*  octal dump in byte  */
	case 10:
	case 11:
		putspc (adv * 4 + (adv >= 8? 1: 0));
		octdmp1 (buf, size, adv);
		break;

	case 12: /*  octal dump in reverse word  */
	case 13:
	case 14:
		putspc (adv * 7);
		octdmp2 (buf, size);
		break;
	}
}

void
dmpall (buf, size)
register unsigned char   *buf;
register int     size;
{
	register int     ss;

	ss = Offset - Addr;
	if (size < Dwidth) {
		dmpmode (buf, size - ss, ss);
#if  EUC
		if (Kskip || Skip) {
			putchar ('.');
			Skip = Kskip = 0;
		}
#else
		if (Skip) {
			putchar ('.');
			Skip = 0;
		}
#endif
		putchar ('\n');
		Addr += size;
		return;
	}
	if (Addr != Offset) {
		dmpmode (buf, Dwidth - ss, ss);
#if  EUC 
		if (!Skip && !Kskip) {
			putchar ('\n');
		}
#else
		if (!Skip) {
			putchar ('\n');
		}
#endif
		Addr += Dwidth;
		buf += Dwidth;
		size -= Dwidth;
	}
	while (size >= Dwidth) {
		if (!Vflag && chksame(buf, Lbuf, Dwidth)) {
			if (!Putast) {
#if  EUC
				if (Skip || Kskip) {
					putchar ('.');
					putchar ('\n');
					Skip = Kskip = 0;
				}
#else
				if (Skip) {
					putchar ('.');
					putchar ('\n');
					Skip = 0;
				}
#endif
				putchar ('*');
				putchar ('\n');
				Putast = 1;
			}
		} else {
			dmpmode (buf, Dwidth, 0);
#if  EUC
			if (!Skip && !Kskip) {
				putchar ('\n');
			}
#else
			if (!Skip) {
				putchar ('\n');
			}
#endif
		}
		Lbuf = buf;
		Addr += Dwidth;
		buf += Dwidth;
		size -= Dwidth;
	}
	if (size != 0) {
		dmpmode (buf, size, 0);
#if  EUC
		if (Skip || Kskip) {
			putchar ('.');
			Skip = Kskip = 0;
		}
#else
		if (Skip) {
			putchar ('.');
			Skip = 0;
		}
#endif
		Addr += size;
		putchar ('\n');
	}
	Offset = Addr;
}

void
setlast (ptr)
register unsigned char   *ptr;
{
	register int    n;
	register unsigned char  *bp;

	n = Dwidth;
	Lbuf = bp = Tbuf;
	while (n--) {
		*bp++ = *ptr++;
	}
}

/*
 *  dump file
 */
void
dmpmain ()
{
	register int     size;

	Mode = Mode * 3 + Ttype;
	Addr = (Offset & ~(Dwidth - 1));
	while ((size = fread(Buf, 1, BSIZE, Fp)) != 0) {
		dmpall (Buf, size);
		setlast (Buf + size - Dwidth);
	}
	if (Putast) {
		--Addr;
		if (Admode) {
			putoct (Addr, 11);
		} else {
			puthex (Addr, 8);
		}
		putchar ('\n');
	}
}

/*
 *  setname return a tail name of path
 */
char   *setname(path)
register char   *path;
{
	register char   *pt;

	while ((pt = kindex(path, '/')) != NULL) {
		path = pt + 1;
	}
#if  LSI
	while ((pt = kindex(path, ':')) != NULL) {
		path = pt + 1;
	}
	while ((pt = kindex(path, '\\')) != NULL) {
		path = pt + 1;
	}
	if (kindex(path, '.') != NULL) {
		*kindex(path, '.') = '\0';
	}
	pt = path;
	while (*pt) {
		/* --- program name to lower case --- */
		if (isupper(*pt)) {
			*pt += ' ';
		}
		++pt;
	}
#endif
	return path;
}

void
usage ()
{
	fputs ("Usage: ", stderr);
	fputs (Prgnam, stderr);
#if  EUC
	fputs (" [ -abcjorvO ] [ <file> ] [ +[x|o]<offset>[k] ]\n", stderr);
#else
	fputs (" [ -abcjkorvO ] [ <file> ] [ +[x|o]<offset>[k] ]\n", stderr);
#endif
	exit (EXSTAT);
}

void
toobig ()
{
	fputs (Prgnam, stderr);
	fputs (": Offset too large!\n", stderr);
	exit (EXSTAT);
}

void
no_open (name)
char   *name;
{
	fputs (Prgnam, stderr);
	fputs (": ", stderr);
	fputs (name, stderr);
	fputs (": can't open.\n", stderr);
	exit (EXSTAT);
}

void
skipread (fp)
FILE   *fp;
{
	register long   offset;

	if (Offset == 0L) {
		return;
	}
	offset = Offset & ~(Dwidth - 1);
	while (offset > (long)BSIZE) {
		if (fread(Buf, 1, BSIZE, fp) < BSIZE) {
			toobig ();
		}
		offset -= BSIZE;
	}
	if (fread(Buf, 1, (int)offset, fp) < (int)offset) {
		toobig ();
	}
}

void
skipfile (fp)
FILE   *fp;
{
	if (fseek(fp, Offset & ~(Dwidth - 1), 0) != 0) {
		toobig ();
		/* --- not reached --- */
	}
}

void
chkofst (p)
register char   *p;
{
	long    atol();
	register unsigned     c;

	if (*p == 'x' || *p == 'X') {
		Offset = 0;
		++p;
		c = (unsigned)*p++;
		while (isxdigit(c)) {
			if (isdigit(c)) {
				Offset = (Offset << 4) + (c - '0');
			} else {
				Offset = (Offset << 4) +
				       ((islower(c)? toupper(c): c) - 'A' + 10);
			}
			c = (unsigned)*p++;
		}
	} else if (*p == 'o' || *p == 'O' || *p == '0') {
		Offset = 0;
		++p;
		while (isdigit(*p)) {
			Offset = ((Offset << 3) & ~7L) + ((*p - '0') & 7);
			++p;
		}
	} else {
		Offset = atol(p);
		while (isdigit(*p)) {
			++p;
		}
	}
	if (Offset && (*p == 'K' || *p == 'k')) {
		Offset <<= 10;
	}
}

void
main (argc, argv)
int     argc;
char  **argv;
{
#if  UNIX || OS9
	Prgnam = setname(*argv);
#endif
#if  LSI
	if (_osmajor > 2) {
		/* --- MS-DOS version 3.xx or later --- */
		Prgnam = setname(*argv);
	}
#endif
	set_ttype();
	while (--argc && **++argv == '-' && *(*argv + 1) != '\0') {
		while (*++*argv) switch (**argv) {

		case 'b':
			Mode = OCTBYT;
			Dwidth = 16;
			continue;

		case 'c':
			Mode = CHRDMP;
			Dwidth = 64;
			continue;

		case 'o':
			Mode = OCTDMP;
			Dwidth = 16;
			continue;

		case 'r':
			Mode = OCTREV;
			Dwidth = 16;
			continue;

		case 'a':
			Ttype = T_ASCII;
			continue;
#if  EUC == 0
		case 'k':
			Ttype = T_KANA;
			continue;
#endif
		case 'j':
			Ttype = T_KANJI;
			continue;

		case 'v':
			Vflag = 1;
			continue;

		case 'O':
			Admode = 1;
			continue;

		default:
			usage ();
		}
	}
	++argc;
	--argv;
	if (argc > 1 && argv[1][0] == '+') {
		chkofst (argv[1] + 1);
		--argc;
		++argv;
	} else if (argc > 2 && argv[2][0] == '+') {
		chkofst (argv[2] + 1);
	}
	if (argc == 1 && isatty(fileno(stdin))) {
		usage ();
		/* --- not reached --- */
	}
	if (argc == 1 || (argc > 1 && **(argv + 1) == '-')) {
		Fp = stdin;
#if  LSI
		Fp->mode |= _BINARY;
		setvbuf (Fp, Ibuf, _IOFBF, BSIZE);
#endif
		skipread (stdin);
		dmpmain ();
		exit (0);
	}
	--argc;
#if  OS9
	if ((Fp = fopen(*++argv, "r")) == NULL && (Fp = fopen(*argv, "d")) == NULL)
#endif
#if  LSI 
	if ((Fp = fopen(*++argv, "rb")) == NULL)
#endif
#if  UNIX
	if ((Fp = fopen(*++argv, "r")) == NULL)
#endif
	{
		no_open(*argv);
		/* --- not reached --- */
	}
	if (argc > 1 && argv[1][0] == '+') {
		chkofst (argv[1] + 1);
	}
#if  LSI
	setvbuf (Fp, Ibuf, _IOFBF, BSIZE);
#endif
	if (isatty(fileno(Fp))) {
		skipread (Fp);
	} else {
		skipfile (Fp);
	}
	dmpmain ();
	fclose (Fp);
	exit (0);
}
