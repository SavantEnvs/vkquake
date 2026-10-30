/* In-memory stand-in for the engine's file layer, plus the few utility
 * symbols the loaders link against. The real layer needs SDL3 and a game
 * directory; the loaders under test only see bytes. */
#include "quakedef.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const byte *mock_data;
static size_t	   mock_size;
static long long   mock_pos;
static int		   mock_eof;
static const char *mock_ext;

THREAD_LOCAL qfileofs_t com_filesize;
char					com_basedir[MAX_OSPATH];

static short LittleShortNoop (short l)
{
	return l;
}
static int LittleLongNoop (int l)
{
	return l;
}
static float LittleFloatNoop (float l)
{
	return l;
}
short (*LittleShort) (short l) = LittleShortNoop;
int (*LittleLong) (int l) = LittleLongNoop;
float (*LittleFloat) (float l) = LittleFloatNoop;

void mockfs_set (const char *ext, const byte *data, size_t size)
{
	mock_ext = ext;
	mock_data = data;
	mock_size = size;
	mock_pos = 0;
	mock_eof = 0;
	com_filesize = (qfileofs_t)size;
}

qboolean COM_FileExists (const char *filename, unsigned int *path_id)
{
	const char *dot = strrchr (filename, '.');
	if (!mock_ext || !dot || strcmp (dot + 1, mock_ext) != 0)
		return false;
	if (path_id)
		*path_id = 1;
	return true;
}

qfilesize_t COM_OpenFile (const char *filename, int *handle, unsigned int *path_id)
{
	(void)filename;
	if (path_id)
		*path_id = 1;
	*handle = 3;
	mock_pos = 0;
	mock_eof = 0;
	com_filesize = (qfileofs_t)mock_size;
	return (qfilesize_t)mock_size;
}

void COM_CloseFile (int h)
{
	(void)h;
}

byte *COM_LoadFile (const char *path, unsigned int *path_id)
{
	byte *buf;
	(void)path;
	if (path_id)
		*path_id = 1;
	buf = (byte *)Mem_Alloc (mock_size + 1);
	if (!buf)
		return NULL;
	memcpy (buf, mock_data, mock_size);
	com_filesize = (qfileofs_t)mock_size;
	return buf;
}

int Sys_FileRead (int handle, void *dest, int count)
{
	long long avail = (long long)mock_size - mock_pos;
	(void)handle;
	if (count < 0)
		return 0;
	if (avail < 0)
		avail = 0;
	if (count > avail)
	{
		count = (int)avail;
		mock_eof = 1;
	}
	memcpy (dest, mock_data + mock_pos, (size_t)count);
	mock_pos += count;
	return count;
}

qfileofs_t Sys_FilePos (int handle)
{
	(void)handle;
	return mock_pos;
}

int Sys_FileSeek (int handle, qfileofs_t position)
{
	(void)handle;
	if (position < 0)
		return -1;
	mock_pos = position;
	mock_eof = 0;
	return 0;
}

int Sys_fgetc (int handle)
{
	(void)handle;
	if (mock_pos >= (long long)mock_size)
	{
		mock_eof = 1;
		return EOF;
	}
	return mock_data[mock_pos++];
}

bool Sys_feof (int handle)
{
	(void)handle;
	return mock_eof != 0;
}

int q_vsnprintf (char *str, size_t size, const char *format, va_list args)
{
	int ret = vsnprintf (str, size, format, args);
	if (ret < 0 || (size_t)ret >= size)
		str[size - 1] = '\0';
	return ret;
}

int q_snprintf (char *str, size_t size, const char *format, ...)
{
	va_list args;
	int		ret;
	va_start (args, format);
	ret = q_vsnprintf (str, size, format, args);
	va_end (args);
	return ret;
}

int q_strcasecmp (const char *s1, const char *s2)
{
	return strcasecmp (s1, s2);
}

#define MOCK_ALLOC_LIMIT (64u << 20)

void *Mem_Alloc (const size_t size)
{
	return size > MOCK_ALLOC_LIMIT ? NULL : calloc (1, size);
}

void *Mem_AllocNonZero (const size_t size)
{
	return size > MOCK_ALLOC_LIMIT ? NULL : malloc (size);
}

void *Mem_Realloc (void *ptr, const size_t size)
{
	return size > MOCK_ALLOC_LIMIT ? NULL : realloc (ptr, size);
}

void Mem_Free (const void *ptr)
{
	free ((void *)ptr);
}
