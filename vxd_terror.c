/**************************************************************************

Copyright (c) 2026 Jaroslav Hensl <emulator@emulace.cz>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*****************************************************************************/
#include <stdarg.h>
#include "winhack.h"
#include "vmm.h"
#include "vxd.h"
#include "vxd_lib.h"
#include "vxd_terror.h"

#define TERROR_MAX 256
#define TERROR_MAX_NUM 12

static char fmtbuffer[TERROR_MAX];
static char hex_lower[] = "0123456789abcdef";
static char hex_upper[] = "0123456789ABCDEF";

static void terrorf_d(char **outp, long d)
{
	char buf[TERROR_MAX_NUM];
	int n = d;
	char *ptr = buf;
	char *out = *outp;
	
	if(d == 0)
	{
		*out = '0';
		out++;
		*outp = out;
		return;
	}
	
	while(n != 0)
	{
		int k = (n % 10);
		if(k < 0) k = -k;
		*ptr = k + '0';
		n /= 10;
		ptr++;
	}
	
	if(d < 0)
	{
		*ptr = '-';
		ptr++;
	}
	
	while(ptr != buf)
	{
		ptr--;
		*out = *ptr;
		out++;
	}
	*outp = out;
}

static void terrorf_u(char **outp, unsigned long d)
{
	char buf[TERROR_MAX_NUM];
	int n = d;
	char *ptr = buf;
	char *out = *outp;
	
	if(d == 0)
	{
		*out = '0';
		out++;
		*outp = out;
		return;
	}
	
	while(n != 0)
	{
		*ptr = (n % 10) + '0';
		n /= 10;
		ptr++;
	}
	
	while(ptr != buf)
	{
		ptr--;
		*out = *ptr;
		out++;
	}
	*outp = out;
}

static void terrorf_x(char **outp, unsigned long x, int upper)
{
	char buf[TERROR_MAX_NUM];
	int n = x;
	char *ptr = buf;
	char *out = *outp;
	const char *hex = hex_lower;

	if(x == 0)
	{
		*out = '0';
		out++;
		*outp = out;
		return;
	}

	if(upper)
	{
		hex = hex_upper;
	}

	while(n != 0)
	{
		unsigned long k = n % 16;
		*ptr = hex[k];
		n /= 16;
		ptr++;
	}

	while(ptr != buf)
	{
		ptr--;
		*out = *ptr;
		out++;
	}
	*outp = out;
}

static void terrorf_s(char **outp, const char *s)
{
	char *out = *outp;
	while(*s != '\0')
	{
		*out = *s;
		s++;
		out++;
	}
	*outp = out;
}

static volatile void terror_out(char c)
{
	_asm mov ax, 0x200
	_asm mov dl, [c]
	_asm push dword ptr 0x21
	VMMCall(Exec_VxD_Int)
}

static volatile char terror_in()
{
	volatile char c = 0;
	
	_asm mov ax, 0x100
	_asm push dword ptr 0x21
	VMMCall(Exec_VxD_Int)
	_asm mov [c], al
	
	return c;
}

/**
 * Very simple support only:
 * d, i,
 * u
 * x, X
 * s
 **/
void terrorf(const char *fmt, ...)
{
	const char *ptr;
	char *out = fmtbuffer;
	va_list va;
	int in_command = 0;
	
	va_start(va, fmt);
	for(ptr = fmt;*ptr != '\0'; ptr++)
	{
		if(in_command == 0)
		{
			switch(*ptr)
			{
				case '%':
					in_command = 1;
					break;
				default:
					*out = *ptr;
					out++;
					break;
			}
		}
		else
		{
			switch(*ptr)
			{
				case '%':
					*out = '%';
					in_command = 0;
					break;
				case 'i':
				case 'd':
				{
					long d = va_arg(va, long);
					terrorf_d(&out, d);
					in_command = 0;
					break;
				}
				case 'u':
				{
					unsigned long u = va_arg(va, unsigned long);
					terrorf_u(&out, u);
					in_command = 0;
					break;
				}
				case 's':
				{
					const char *s = va_arg(va, const char *);
					terrorf_s(&out, s);
					in_command = 0;
					break;
				}
				case 'x':
				case 'X':
				{
					unsigned long u = va_arg(va, unsigned long);
					terrorf_x(&out, u, *ptr == 'X');
					in_command = 0;
					break;
				}				
				default:
					/* eat all potencial formating... */
					break;
			}
		}
	}
	*out = '\0';
	
	va_end(va);
	terror(fmtbuffer);
}

void terror(const char *str)
{
	const char *ptr = str;
	while(*ptr != '\0')
	{
		/* auto include return */
		if(*ptr == '\n')
		{
			terror_out('\r');
		}
		
		terror_out(*ptr);
		ptr++;
	}
}

void tpause()
{
	terror("Press Enter to continue...\n");
	for(;;)
	{
		char c = terror_in();
		if(c == '\r' || c == '\n')
		{
			break;
		}
	}
}

