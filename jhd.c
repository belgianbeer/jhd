/*
 *  Jhd - Japanese Hexdecimal Dump
 *
 *    Written by Masato Minda <minmin@wide.ad.jp>
 *
 *    Created Sep. 11, 1986  Version 1.0
 *            Sep. 14, 1988  Version 2.0
 *    modified
 *      Jan. 20, 1989  Ver. 2.1
 *        little bug fix in octal-word.
 *      Feb. 23, 1989  Ver. 2.2
 *        correct offset, file is stdin.
 *      Nov. 10, 2026  Ver. 3.0
 *        ISO C (ANSI) style at last: prototypes instead of K&R definitions,
 *        static internal objects and functions, size_t/off_t instead of
 *        int/long, strtoll() instead of atol().
 *        The OS-9 (Motorola MC6809) and the LSI-C (MS-DOS) ports are gone:
 *        jhd is now built for POSIX environments only.
 *      Nov. 12, 2026  Ver. 3.1
 *        Fixed an old bug:  a dump line ending with the first half of a kanji
 *        had its newline held back until the second half could be printed at
 *        the head of the next line, so data that ended exactly on a line
 *        boundary -- a multiple of 16 bytes, or of 64 with -c -- lost its
 *        final line break.  dmpmain() now ends a pending half with "." and a
 *        newline.  Only the hexdecimal and charactor dumps, in the kanji
 *        terminal mode, can wait for a second half;  in EUC any byte of
 *        0xa1-0xfe, whether it leads or trails, and the kana shift 0x8e do.
 */
/*  not static, so that the id string stays in the image  */
char rcsid[] = "@(#)$Id: jhd.c,v 3.1 2026/11/12 12:56:54 minmin Exp $";

/*
 *  definition of the Kanji code
 *	EUC	1	EUC-JP		(default)
 *		0	Shift-JIS
 *  It can also be set from the command line: cc -DEUC=0 ...
 */
#ifndef  EUC
#define  EUC     1
#endif

/*
 *  NORETURN - tell the compiler that a function never comes back
 */
#if  defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define  NORETURN   _Noreturn
#elif  defined(__GNUC__)
#define  NORETURN   __attribute__((__noreturn__))
#else
#define  NORETURN
#endif

#include  <sys/types.h>		/*  off_t, size_t  */
#include  <ctype.h>
#include  <errno.h>
#include  <limits.h>		/*  LLONG_MAX  */
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <unistd.h>		/*  isatty(), fileno(), STDIN_FILENO  */

#define  BSIZE      16384		/*  input buffer size  */
#define  LWIDTH         64		/*  widest dump line, in char mode  */

#define  T_ASCII         0		/*  ascii terminal  */
#define  T_KANA          1		/*  kana terminal  */
#define  T_KANJI         2		/*  kanji terminal  */

#define  HEXDMP          0		/*  dump in hexdecimal  */
#define  CHRDMP          1		/*  dump in charactor  */
#define  OCTDMP          2		/*  dump in octal word  */
#define  OCTBYT          3		/*  dump in octal byte  */
#define  OCTREV          4		/*  dump in octal reverse  */

#define  KSHIFT      0x8e		/*  JAE kana shift in  */

static unsigned char	Buf[BSIZE];	/*  input buffer  */
static unsigned char	Tbuf[LWIDTH];	/*  copy of the last dumped line  */
static const unsigned char *Lbuf;	/*  last Dwidth bytes dumped  */

static const char	*Prgnam = "jhd";
static off_t		Offset;		/*  dump offset  */
static off_t		Addr;		/*  current address  */
static off_t		Dwidth = 16;	/*  dump width in byte  */
static int		Admode;		/*  address display mode  */
static FILE		*Fp;		/*  file pointer  */
static int		Ttype;		/*  terminal type  */
static int		Skip;		/*  skip next data (kanji2 printed)  */
static int		Putast;		/*  '*' display flag  */
static int		Mode = HEXDMP;	/*  dump mode  */
static int		Vflag;		/*  visual option  */
#if  EUC
static int		Kskip;		/*  skip next data (kana2 printed)  */
#endif

/*
 *  function prototypes
 */
static int		iskanji(int c);
#if  EUC
static int		iskana2(int c);
#else
static int		iskanji2(int c);
static int		iskana(int c);
#endif
static void		set_ttype(void);
static const char	*kindex(const char *s, int c);
static void		putspc(int n);
static void		putnum(unsigned long long value, int shift, int width);
static void		putoct(unsigned long long value, int width);
static void		puthex(unsigned long long value, int width);
static void		putaddr(void);
static int		chksame(const unsigned char *cur,
			    const unsigned char *old, int n);
static void		dmpkanji(const unsigned char *base, int size);
static void		dmpkana(const unsigned char *base, int size);
static void		dmpascii(const unsigned char *base, int size);
static void		dmptext(const unsigned char *base, int size);
static void		octdmp0(const unsigned char *base, int size);
static void		octdmp1(const unsigned char *base, int size, int adr);
static void		octdmp2(const unsigned char *base, int size);
static void		hexdmp(const unsigned char *base, int size, int adr);
static void		putpartial(const unsigned char *buf);
static void		dmpnl(int how);
static void		dmpmode(const unsigned char *buf, int size, int adv);
static void		dmpall(const unsigned char *buf, int size);
static void		setlast(const unsigned char *ptr);
static void		dmpmain(void);
static const char	*setname(const char *path);
static void		setoption(int opt);
static NORETURN void	usage(void);
static NORETURN void	toobig(void);
static NORETURN void	no_open(const char *name);
static void		skipread(FILE *fp);
static void		skipfile(FILE *fp);
static void		chkofst(const char *p);
static off_t		linestart(off_t addr);
static int		parseargs(int argc, char **argv);

/*
 *  Kanji code recognition.
 *  The byte reaching these is always read from an unsigned char, so the
 *  masking is just a guarantee for odd arguments.
 */
#if  EUC

#ifdef	iskanji
#undef	iskanji
#endif
#ifdef	iskanji2
#undef	iskanji2
#endif
#ifdef	iskana2
#undef	iskana2
#endif
#ifdef	iskana
#undef	iskana
#endif

/*
 *  iskanji - lead or trail byte of a EUC kanji
 */
static int
iskanji(int c)
{
	c &= 0xff;
	return (c >= 0xa1 && c <= 0xfe);
}

/*
 *  iskana2 - trail byte of a half width katakana
 */
static int
iskana2(int c)
{
	c &= 0xff;
	return (c >= 0xa1 && c <= 0xdf);
}

#define	iskana(c)	((c) == KSHIFT)		/*  lead byte  */

#else  /*  EUC  */

/*
 *   for kanji kana mode
 */
#ifdef	iskanji
#undef	iskanji
#endif
#ifdef	iskanji2
#undef	iskanji2
#endif
#ifdef	iskana
#undef	iskana
#endif

/*
 *  iskanji - lead byte of a Shift-JIS kanji
 */
static int
iskanji(int c)
{
	c &= 0xff;
	return (c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc);
}

/*
 *  iskanji2 - trail byte of a Shift-JIS kanji
 */
static int
iskanji2(int c)
{
	c &= 0xff;
	return (c >= 0x40 && c <= 0x7e) || (c >= 0x80 && c <= 0xfc);
}

/*
 *  iskana - byte of a half width katakana
 */
static int
iskana(int c)
{
	c &= 0xff;
	return (c >= 0xa1 && c <= 0xdf);
}

#endif  /*  EUC  */

/*
 *  set_ttype - check terminal type
 */
static void
set_ttype(void)
{
	const char	*p;

	p = getenv("TTYPE");
	if (p == NULL) {
		Ttype = T_ASCII;
	} else if (strcmp(p, "mskanji") == 0 || strcmp(p, "jiskanji") == 0) {
		Ttype = T_KANJI;
#if  EUC == 0
	} else if (strcmp(p, "kana") == 0) {
		Ttype = T_KANA;
#endif
	} else {
		Ttype = T_ASCII;
	}
}

/*
 *  kindex returns a pointer to the first occurrence of character
 *  c in string s, or zero if c does not occur in the string.
 *  Kanji code is skipped, so that a c inside a kanji is not found.
 */
static const char *
kindex(const char *s, int c)
{
	while (*s != '\0') {
		if ((unsigned char)*s == (unsigned char)c) {
			return s;
		}
		if (iskanji((unsigned char)*s)) {
			++s;
		}
		++s;
	}
	return NULL;
}

/*
 *  putspc - print n blanks
 */
static void
putspc(int n)
{
	printf("%*s", n, "");
}

/*
 *  putnum - print the low `width' digits of `value', `shift' bits each
 */
static void
putnum(unsigned long long value, int shift, int width)
{
	static const char	digits[] = "0123456789ABCDEF";
	const unsigned long long mask = (1ULL << shift) - 1;
	char			buf[64];
	int			i;

	if (width < 0 || width >= (int)sizeof(buf)) {
		width = (int)sizeof(buf) - 1;
	}
	i = width;
	buf[i] = '\0';
	while (i > 0) {
		buf[--i] = digits[(size_t)(value & mask)];
		value >>= shift;
	}
	fputs(buf, stdout);
}

static void
putoct(unsigned long long value, int width)
{
	putnum(value, 3, width);
}

static void
puthex(unsigned long long value, int width)
{
	putnum(value, 4, width);
}

/*
 *  putaddr - print the current address, octal or hexdecimal
 */
static void
putaddr(void)
{
	if (Admode) {
		putoct((unsigned long long)Addr, 11);
	} else {
		puthex((unsigned long long)Addr, 8);
	}
}

/*
 *  chksame - check n byte same data
 */
static int
chksame(const unsigned char *cur, const unsigned char *old, int n)
{
	if (old == NULL) {
		return 0;
	}
	while (n--) {
		if (*cur++ != *old++) {
			return 0;
		}
	}
	return 1;
}

#if  EUC

/*
 *  dmpkanji - dump in kanji with JAE-Kanji
 */
static void
dmpkanji(const unsigned char *base, int size)
{
	int	c;

	putchar(' ');
	while (size--) {
		c = *base++;
		if (Skip || Kskip) {
			Kskip = Skip = 0;
			putchar(' ');
		} else if (size == 0 && iskanji(c)) {
			Skip = c;
		} else if (size == 0 && iskana(c)) {
			Kskip = c;
		} else if (iskanji(c) && iskanji(*base)) {
			putchar(c);
			putchar(*base);
			--size;
			++base;
		} else if (iskana(c) && iskana2(*base)) {
			/*  a KSHIFT (SS2) and a byte of the half width kana  */
			putchar(c);
			putchar(*base);
			--size;
			++base;
		} else if (isprint(c)) {
			putchar(c);
		} else {
			putchar('.');
		}
	}
}

/*
 *  dmpkana - no half width kana mode in EUC
 */
/* ARGSUSED */
static void
dmpkana(const unsigned char *base, int size)
{
	/*  not called in EUC  */
	(void)base;
	(void)size;
}

#else  /*  EUC  */

/*
 *  dmpkanji - dump in kanji with Shift-JIS
 */
static void
dmpkanji(const unsigned char *base, int size)
{
	int	c;

	putchar(' ');
	while (size--) {
		c = *base++;
		if (Skip) {
			Skip = 0;
			putchar(' ');
		} else if (size == 0 && iskanji(c)) {
			Skip = c;
		} else if (iskanji(c) && iskanji2(*base)) {
			putchar(c);
			putchar(*base);
			--size;
			++base;
		} else if (iskana(c) || isprint(c)) {
			putchar(c);
		} else {
			putchar('.');
		}
	}
}

/*
 *  dmpkana - dump in hankaku katakana
 */
static void
dmpkana(const unsigned char *base, int size)
{
	int	c;

	putchar(' ');
	while (size--) {
		c = *base++;
		if (isprint(c) || iskana(c)) {
			putchar(c);
		} else {
			putchar('.');
		}
	}
}

#endif  /*  EUC  */

/*
 *  dmpascii - dump in ascii charactor
 */
static void
dmpascii(const unsigned char *base, int size)
{
	int	c;

	putchar(' ');
	while (size--) {
		c = *base++;
		if (isprint(c)) {
			putchar(c);
		} else {
			putchar('.');
		}
	}
}

/*
 *  dmptext - dump the character column of the terminal in use
 */
static void
dmptext(const unsigned char *base, int size)
{
	switch (Ttype) {
	case T_ASCII:
		dmpascii(base, size);
		break;
	case T_KANA:
		dmpkana(base, size);
		break;
	default:
		dmpkanji(base, size);
		break;
	}
}

/*
 *  octdmp0 - dump octal in word
 */
static void
octdmp0(const unsigned char *base, int size)
{
	while (size > 0) {
		putchar(' ');
		putoct(((unsigned long long)base[0] << 8) + base[1], 6);
		base += 2;
		size -= 2;
	}
}

/*
 *  octdmp1 - dump octal in byte
 */
static void
octdmp1(const unsigned char *base, int size, int adr)
{
	while (size--) {
		putchar(' ');
		putoct(*base++, 3);
		if (++adr == 8) {
			putchar(' ');
		}
	}
}

/*
 *  octdmp2 - dump octal in reverse word
 */
static void
octdmp2(const unsigned char *base, int size)
{
	while (size > 0) {
		putchar(' ');
		putoct(((unsigned long long)base[1] << 8) + base[0], 6);
		base += 2;
		size -= 2;
	}
}

/*
 *  hexdmp - dump hexdecimal
 */
/*
 *  hexdmp - dump hexdecimal
 */
static void
hexdmp(const unsigned char *base, int size, int adr)
{
	while (size--) {
		putchar(' ');
		puthex(*base++, 2);
		if (++adr == 8) {
			putchar(' ');
		}
	}
}

/*
 *  dmpmode - print one dumped line
 *
 *  buf	dump starts `adv' bytes before the data to be displayed, because
 *	the leading half of a kanji may sit on the previous line.
 *  size	number of bytes to display.
 */
static void
dmpmode(const unsigned char *buf, int size, int adv)
{
	int	n;

	Putast = 0;
	putpartial(buf);
	putaddr();
	putchar(' ');
	buf += adv;
	n = size + adv;			/*  bytes of this line in the file  */
	switch (Mode) {
	case HEXDMP:			/*  hex dump with char column  */
		putspc(adv * 3 + (adv >= 8 ? 1 : 0));
		hexdmp(buf, size, adv);
		putspc((int)(Dwidth - n) * 3 + (n < 8 ? 2 : 1) + adv);
		dmptext(buf, size);
		break;

	case CHRDMP:			/*  char dump only  */
		putspc(adv);
		dmptext(buf, size);
		break;

	case OCTDMP:			/*  octal dump in word  */
		putspc((adv / 2) * 7);
		octdmp0(buf, size);
		break;

	case OCTBYT:			/*  octal dump in byte  */
		putspc(adv * 4 + (adv >= 8 ? 1 : 0));
		octdmp1(buf, size, adv);
		break;

	case OCTREV:			/*  octal dump in reverse word  */
		putspc(adv * 7);
		octdmp2(buf, size);
		break;
	}
}

/*
 *  end of line, with the two flavours the kanji code may ask for
 */
#define	LINEEND		0		/*  close the line, dot if pending  */
#define	LINECONT	1		/*  half kanji crosses to next line  */
#define	LINESKIP	2		/*  repeated data, nothing displayed  */

/*
 *  dmpnl - terminate a dumped line
 */
static void
dmpnl(int how)
{
#if  EUC
	int	pend = (Skip || Kskip);
#else
	int	pend = (Skip != 0);
#endif

	switch (how) {
	case LINEEND:
		if (pend) {
			putchar('.');
		}
		putchar('\n');
		break;
	case LINECONT:
		if (!pend) {
			putchar('\n');
		}
		return;
	default:			/*  LINESKIP  */
		if (pend) {
			putchar('.');
			putchar('\n');
		}
		break;
	}
	if (pend) {
#if  EUC
		Skip = Kskip = 0;
#else
		Skip = 0;
#endif
	}
}

/*
 *  putpartial - display a kanji or kana whose second byte sits on the
 *  line that is dumped now
 */
static void
putpartial(const unsigned char *buf)
{
#if  EUC
	if (Kskip) {
		if (iskana2(*buf)) {
			putchar(Kskip);
			putchar(*buf);
		} else {
			putchar('.');
			Kskip = 0;
		}
		putchar('\n');
	}
#endif
	if (Skip) {
#if  EUC
		if (iskanji(*buf)) {
#else
		if (iskanji2(*buf)) {
#endif
			putchar(Skip);
			putchar(*buf);
		} else {
			putchar('.');
			Skip = 0;
		}
		putchar('\n');
	}
}

/*
 *  dmpall - dump a block that was read in
 */
static void
dmpall(const unsigned char *buf, int size)
{
	int	first = (int)(Offset - Addr);	/*  bytes to skip, 1st line  */
	int	width = (int)Dwidth;

	if (size < width) {
		if (size < first) {
			first = size;	/*  the offset sits past the end  */
		}
		dmpmode(buf, size - first, first);
		dmpnl(LINEEND);
		Addr += size;
		return;
	}
	if (Addr != Offset) {
		dmpmode(buf, width - first, first);
		dmpnl(LINECONT);
		Addr += width;
		buf += width;
		size -= width;
	}
	while (size >= width) {
		if (!Vflag && chksame(buf, Lbuf, width)) {
			if (!Putast) {
				dmpnl(LINESKIP);
				putchar('*');
				putchar('\n');
				Putast = 1;
			}
		} else {
			dmpmode(buf, width, 0);
			dmpnl(LINECONT);
		}
		Lbuf = buf;
		Addr += width;
		buf += width;
		size -= width;
	}
	if (size != 0) {
		dmpmode(buf, size, 0);
		Addr += size;
		dmpnl(LINEEND);
	}
	Offset = Addr;
}

static void
setlast(const unsigned char *ptr)
{
	int	n;
	unsigned char	*bp;

	n = (int)Dwidth;
	Lbuf = bp = Tbuf;
	while (n--) {
		*bp++ = *ptr++;
	}
}

/*
 *  dump file
 */
static void
dmpmain(void)
{
	size_t	size;

	Addr = linestart(Offset);
	while ((size = fread(Buf, 1, sizeof(Buf), Fp)) != 0) {
		dmpall(Buf, (int)size);
		if (size >= (size_t)Dwidth) {
			setlast(Buf + size - (size_t)Dwidth);
		}
	}
	if (Putast) {
		--Addr;
		putaddr();
		putchar('\n');
	}
#if  EUC
	if (Skip || Kskip) {
#else
	if (Skip) {
#endif
		putchar('.');		/*  1st half kanji at last data */
		putchar('\n');
	}
}

/*
 *  setname - return the tail name of a path
 */
static const char *
setname(const char *path)
{
	const char	*pt;

	while ((pt = kindex(path, '/')) != NULL) {
		path = pt + 1;
	}
	return path;
}

/*
 *  setoption - apply one option letter of the command line
 */
static void
setoption(int opt)
{
	switch (opt) {
	case 'b':
		Mode = OCTBYT;
		Dwidth = 16;
		break;

	case 'c':
		Mode = CHRDMP;
		Dwidth = LWIDTH;
		break;

	case 'o':
		Mode = OCTDMP;
		Dwidth = 16;
		break;

	case 'r':
		Mode = OCTREV;
		Dwidth = 16;
		break;

	case 'a':
		Ttype = T_ASCII;
		break;

#if  EUC == 0
	case 'k':
		Ttype = T_KANA;
		break;
#endif

	case 'j':
		Ttype = T_KANJI;
		break;

	case 'v':
		Vflag = 1;
		break;

	case 'O':
		Admode = 1;
		break;

	default:
		usage();
	}
}

/*
 *  parseargs - pick the options, the offset and the file out of argv.
 *
 *  Options are every argument up to the first one that does not begin
 *  with '-' (a lone "-" means the standard input), and they may be
 *  clustered, as in "-vjO".  The offset is "+[x|o]<offset>[k]", and it
 *  may be given either before or after the file name.
 *
 *  Returns the index of the file to dump, or 0 when the standard input
 *  has to be used.  Usage is displayed when there is nothing to dump and
 *  the standard input is a terminal.
 */
static int
parseargs(int argc, char **argv)
{
	int	i;
	int	file;

	for (i = 1; i < argc; ++i) {
		const char	*p;

		if (argv[i][0] != '-' || argv[i][1] == '\0') {
			break;
		}
		for (p = argv[i] + 1; *p != '\0'; ++p) {
			setoption(*p);
		}
	}
	file = i;
	if (i < argc && argv[i][0] == '+') {
		chkofst(argv[i] + 1);
		file = i + 1;
	} else if (i + 1 < argc && argv[i + 1][0] == '+') {
		chkofst(argv[i + 1] + 1);
	}
	if (file >= argc) {
		if (isatty(STDIN_FILENO)) {
			usage();
		}
		return 0;
	}
	return argv[file][0] == '-' ? 0 : file;
}

/*
 *  usage - tell how to run this program
 */
static NORETURN void
usage(void)
{
	fputs("Usage: ", stderr);
	fputs(Prgnam, stderr);
#if  EUC
	fputs(" [ -abcjorvO ] [ <file> ] [ +[x|o]<offset>[k] ]\n", stderr);
#else
	fputs(" [ -abcjkorvO ] [ <file> ] [ +[x|o]<offset>[k] ]\n", stderr);
#endif
	exit(EXIT_FAILURE);
}

/*
 *  toobig - the offset reaches beyond the data
 */
static NORETURN void
toobig(void)
{
	fputs(Prgnam, stderr);
	fputs(": Offset too large!\n", stderr);
	exit(EXIT_FAILURE);
}

static NORETURN void
no_open(const char *name)
{
	fputs(Prgnam, stderr);
	fputs(": ", stderr);
	fputs(name, stderr);
	fputs(": can't open.\n", stderr);
	exit(EXIT_FAILURE);
}

/*
 *  skipread - walk the stream up to the offset, reading it
 *
 *  Used for input that cannot be seeked (a terminal or a pipe); the data
 *  in front of the offset is read into the buffer and dropped.
 */
static void
skipread(FILE *fp)
{
	off_t	offset;

	if (Offset == 0) {
		return;
	}
	offset = linestart(Offset);
	while (offset > (off_t)sizeof(Buf)) {
		if (fread(Buf, 1, sizeof(Buf), fp) != sizeof(Buf)) {
			toobig();
		}
		offset -= (off_t)sizeof(Buf);
	}
	if (fread(Buf, 1, (size_t)offset, fp) != (size_t)offset) {
		toobig();
	}
}

/*
 *  skipfile - seek the file to the offset
 */
static void
skipfile(FILE *fp)
{
	if (fseeko(fp, linestart(Offset), SEEK_SET) != 0) {
		toobig();
	}
}

/*
 *  linestart - the address of the dump line that holds addr
 *
 *  Dwidth is a power of two, so this rounds down without a division.
 */
static off_t
linestart(off_t addr)
{
	return addr - (addr & (Dwidth - 1));
}

/*
 *  chkofst - decode "+..." into Offset
 *
 *  x/X selects hexdecimal, o/O or a leading 0 selects octal, decimal else;
 *  a trailing k multiplies by 1024.
 */
static void
chkofst(const char *p)
{
	char		*end;
	long long	value;
	int		basesize = 10;

	if (*p == 'x' || *p == 'X') {
		basesize = 16;
		++p;
	} else if (*p == 'o' || *p == 'O') {
		basesize = 8;
		++p;
	} else if (*p == '0') {
		basesize = 8;
	}
	errno = 0;
	value = strtoll(p, &end, basesize);
	if (errno == ERANGE || value < 0 || value > LLONG_MAX >> 10) {
		toobig();
	}
	Offset = (off_t)value;
	if (Offset && (*end == 'K' || *end == 'k')) {
		Offset <<= 10;
	}
}

int
main(int argc, char **argv)
{
	int	file;

	Prgnam = setname(argv[0]);
	set_ttype();
	file = parseargs(argc, argv);
	if (file == 0) {			/*  the standard input  */
		Fp = stdin;
		skipread(Fp);
		dmpmain();
		return 0;
	}
	Fp = fopen(argv[file], "r");
	if (Fp == NULL) {
		no_open(argv[file]);
	}
	if (file + 1 < argc && argv[file + 1][0] == '+') {
		chkofst(argv[file + 1] + 1);
	}
	if (isatty(fileno(Fp))) {
		skipread(Fp);
	} else {
		skipfile(Fp);
	}
	dmpmain();
	fclose(Fp);
	return 0;
}
