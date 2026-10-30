/* Engine-service stubs shared by every harness. The engine's console, host and
 * system layers pull in SDL3/Vulkan; the fuzzed parsers only need these few. */
#include "quakedef.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void Con_Printf (const char *fmt, ...)
{
	(void)fmt;
}
void Con_DPrintf (const char *fmt, ...)
{
	(void)fmt;
}
void Con_DPrintf2 (const char *fmt, ...)
{
	(void)fmt;
}
void Con_Warning (const char *fmt, ...)
{
	(void)fmt;
}
void Con_SafePrintf (const char *fmt, ...)
{
	(void)fmt;
}

void Sys_Error (const char *error, ...)
{
	va_list ap;
	va_start (ap, error);
	fputs ("Sys_Error: ", stderr);
	vfprintf (stderr, error, ap);
	fputc ('\n', stderr);
	va_end (ap);
	exit (1); // the engine's own behaviour: Sys_Error terminates the process
}

void Host_Error (const char *error, ...)
{
	va_list ap;
	va_start (ap, error);
	fputs ("Host_Error: ", stderr);
	vfprintf (stderr, error, ap);
	fputc ('\n', stderr);
	va_end (ap);
	abort ();
}
