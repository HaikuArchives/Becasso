#include "debug.h"
#include <stdio.h>

#pragma export on
int DebugLevel;
#pragma export off

void verbose (int level, const char *fmt, ...)
{
	if (DebugLevel >= level)
	{
		va_list ap;
		const char *p, *svalue;
		int ivalue;
		/* float dvalue; */
		
		va_start (ap, fmt);
		for (p = fmt; *p; p++)
		{
			if (*p != '%')
			{
				putc (*p, stderr);
				continue;
			}
			switch (*++p)
			{
				case 'd':
					ivalue = va_arg (ap, int);
					fprintf (stderr, "%d", ivalue);
					break;
				
				/*case 'f':
					dvalue = va_arg (ap, float);
					fprintf (stderr, "%d", dvalue);
					break; */
					
				case 's':
					for (svalue = va_arg (ap, char *); *svalue; svalue++)
						putc (*svalue, stderr);
					break;
				
				default:
					putc (*p, stderr);
					break;
			}
			va_end (ap);
		}
	}
}