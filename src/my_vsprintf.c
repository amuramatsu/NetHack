/*
 * integer vsnprintf, et. al. based on BDS C v1.6 library
 */
/*
	STDLIB2.C -- for BDS C v1.6 --  1/86
	Copyright (c) 1982, 1986 by BD Software, Inc.

	The files STDLIB1.C, STDLIB2.C and STDLIB3.C contain the source
	listings for all functions present in the DEFF.CRL library object
 	file. (Note: DEFF2.CRL contains the .CSM-coded portions of the
	library.)

	STDLIB2.C contains mainly formatted text I/O functions:

	printf 	fprintf	sprintf	lprintf	_spr
	scanf	fscanf	sscanf	_scn
	getline	puts
	putdec

*/

#ifdef USE_MY_VSPRINTF

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

typedef enum { OK, ERROR } _result;
static int _spr(const char *format, va_list va, _result (*putcf)(char));

static char *bufptr;
static char *endbufptr;
static _result
_writer(char c)
{
    if (endbufptr && bufptr >= endbufptr)
	return ERROR;
    *bufptr++ = c;
    return OK;
}

int
__vsnprintf(char *buffer, int n, const char *format, va_list va)
{
    if (n == 0) return 0;
    bufptr = buffer;
    endbufptr = buffer + n;
    _spr(format, va, &_writer);
    _writer('\0');
    buffer[n-1] = '\0';
    return bufptr - buffer;
}

int
__vsprintf(char *buffer, const char *format, va_list va)
{
    bufptr = buffer;
    endbufptr = NULL;
    _spr(format, va, &_writer);
    _writer('\0');
    return bufptr - buffer;
}

int
__snprintf(char *buffer, int n, const char *format, ...)
{
    int s;
    va_list va;
    va_start(va, format);
    s = __vsnprintf(buffer, n, format, va);
    va_end(va);
    return s;
}

int
__sprintf(char *buffer, const char *format, ...)
{
    int s;
    va_list va;
    va_start(va, format);
    s = __vsprintf(buffer, format, va);
    va_end(va);
    return s;
}

/*
  Internal routine used by "_spr" to perform ascii-
  to-decimal conversion and update an associated pointer:
*/

static int
_gv2(const char **sptr)
{
    int n;
    n = 0;
    while (isdigit(**sptr))
	n = 10*n + *(*sptr)++ - '0';
    return n;
}

static char
_uspr(char **string, unsigned n, unsigned base, int upperflag)
{
    int length;
    if (n < base) {
	*(*string)++ = (n < 10) ? n + '0'
	    : (upperflag ? (n + 'A' - 10) : (n + 'a' - 10));
	return 1;
    }
    length = _uspr(string, n / base, base, upperflag);
    _uspr(string, n % base, base, upperflag);
    return length + 1;
}

static int
_bc(char c, char b)
{
    if (isalpha(c = toupper(c)))
	c -= 55;
    else if (isdigit(c))
	c -= 0x30;
    else
	return ERROR;

    if (c > b-1)
	return ERROR;
    else
	return c;
}

static int
_spr(const char *format, va_list va, _result (*putcf)(char))
{
    char c, prefill, *wptr;
    long value;
    int ljflag, upperflag;
    int base = 10;
    char wbuf[128];	/* 128 is enough for all but %s */
    int length, *args, width, precision;
    
    while ((c = *format++)) {
	if (c == '%') {
	    wptr = wbuf;
	    precision = INT_MAX;
	    width = ljflag = 0;
	    upperflag = 0;
	    prefill = ' ';

	    if (*format == '-') {
		format++;
		ljflag=1;
	    }
	    if (*format == '0') {
		format++;
		prefill = '0';
	    }
	    if (*format == '*') {
		format++;
		width = va_arg(va, int);
	    }
	    else if (isdigit(*format))
		width = _gv2(&format);
	    if (*format == '.') {
		format++;
		if (*format == '*') {
		    format++;
		    precision = va_arg(va, int);
		}
		else {   
		    precision = _gv2(&format);
		}
	    }
	    else if (*format == 'l')
		format++;	/* no longs here */
	    
	    switch (c = *format++) {
	    case 'd':
		{
		    int value = va_arg(va, int);
		    if (value < 0) {
			*wptr++ = '-';
			value = -value;
			width--;
		    }
		    width -= _uspr(&wptr, value, base, 0);
		}
		goto pad;
	    case 'u':
		base = 10; goto val;
	    case 'b':
		base = 2; goto val;
	    case 'x':
		base = 16; goto val;
	    case 'X':
		base = 16; upperflag = 1; goto val;
	    case 'o':
		base = 8;
	    val:
		width -= _uspr(&wptr, va_arg(va, unsigned int), base, upperflag);
		goto pad;
		
	    case 'c':
		*wptr++ = va_arg(va, int) & 0xff;
		width--;
		goto pad;
		
	    pad:
		*wptr = '\0';
		length = strlen(wptr = wbuf);
	    pad7: /* don't modify the string at wptr */
		if (!ljflag) {
		    while (width-- > 0)
			if (putcf(prefill) == ERROR)
			    return ERROR;
		}
		while (length--) {
		    if (putcf(*wptr++) == ERROR) 
			return ERROR;
		}
		if (ljflag) {
		    while (width-- > 0)
			if (putcf(' ') == ERROR)
			    return ERROR;
		}
		break;
		
	    case 's':
		wptr = va_arg(va, char *);
		length = strlen(wptr);
		if (precision < length)
		    length = precision;
		width -= length;
		goto pad7;
		
	    case '\0':
		return OK;
			   
	    default:
		if (putcf(c) == ERROR)
		    return ERROR;
	    }
	}
	else if (putcf(c) == ERROR)
	    return ERROR;
    }
    return OK;
}

#ifdef DEBUG
#include <stdio.h>

void
test(const char *name, const char *format, ...)
{
    char buf1[1024], buf2[1024];
    va_list va1, va2;
    va_start(va1, format);
    va_copy(va2, va1);
    vsprintf(buf1, format, va1);
    __vsprintf(buf2, format, va2);
    va_end(va1);
    va_end(va2);
    if (strcmp(buf1, buf2) != 0)
	printf("ERR!:: %s:%s:%s\n", name, buf1, buf2);
}

int
main()
{
    test("test1", "testdata, %d, %10d", 10, 200);
    test("test2", "testdata, %s, %10.10s", "dfadf,", "xxxxxxxxxxxxx");
    test("test3", "testdata, %x, %10X", 10, 200);
    test("test4", "testdata, %X, %10x", 10, 200);
    test("test5", "testdata, %d, %10d", 10, 200);
    test("test6", "testdata, %d, %10d", 10, 200);
    test("test7", "testdata, %d, %10d", 10, 200);
    test("test8", "testdata, %d, %10d", 10, 200);
    test("test9", "testdata, %d, %10d", 10, 200);
    return 0;
}

#endif
#endif /* USE_MY_VSPRINTF */
