#include "quakedef.h"
#include "wad.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void mockfs_set (const char *ext, const byte *data, size_t size);

static void load (const byte *data, size_t size)
{
	mockfs_set ("wad", data, size);
	W_LoadWadFile ();
}

static void release (void)
{
	Mem_Free (wad_base);
	wad_base = NULL;
	wad_lumps = NULL;
	wad_numlumps = 0;
}

#ifdef MAYHEM_KAT
#include "kat.h"
#else
int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
	int			i;
	lumpinfo_t *info;
	byte	   *lump;

	/* the engine reads the 12-byte header unchecked; gfx.wad is game data, not user input */
	if (size < sizeof (wadinfo_t))
		return -1;

	load (data, size);
	for (i = 0; i < wad_numlumps; i++)
	{
		lump = (byte *)W_GetLumpName (wad_lumps[i].name, &info);
		/* only read lumps that lie inside the file; the engine's own clamp of
		 * out-of-range lumps is what the fuzzer is testing, not this loop */
		if (lump && info->size > 0 && info->filepos >= 0 && (long long)info->size <= (long long)size - info->filepos)
		{
			volatile byte first = lump[0];
			volatile byte last = lump[info->size - 1];
			(void)first;
			(void)last;
		}
	}
	(void)W_GetLumpName ("conchars", &info);
	(void)W_GetLumpName ("PALETTE", &info);
	release ();
	return 0;
}
#endif

#ifdef MAYHEM_KAT
int main (void)
{
	byte		wad[12 + 4 + 12 + 2 * sizeof (lumpinfo_t)];
	lumpinfo_t *l;
	lumpinfo_t *info = NULL;
	byte	   *p;
	int			ofs = 12 + 4 + 12;

	memset (wad, 0, sizeof (wad));
	memcpy (wad, "WAD2", 4);
	*(int *)(wad + 4) = 2;
	*(int *)(wad + 8) = ofs;
	memcpy (wad + 12, "ABCD", 4);
	*(int *)(wad + 16) = 2; /* qpic width */
	*(int *)(wad + 20) = 1; /* qpic height */
	memcpy (wad + 24, "wxyz", 4);

	l = (lumpinfo_t *)(wad + ofs);
	l[0].filepos = 12;
	l[0].disksize = l[0].size = 4;
	l[0].type = TYP_LUMPY;
	strcpy (l[0].name, "Palette");
	l[1].filepos = 16;
	l[1].disksize = l[1].size = 12;
	l[1].type = TYP_QPIC;
	strcpy (l[1].name, "CONBACK");

	load (wad, sizeof (wad));
	KAT (wad_numlumps == 2);
	p = (byte *)W_GetLumpName ("palette", &info);
	KAT (p != NULL && info != NULL && info->size == 4 && memcmp (p, "ABCD", 4) == 0);
	p = (byte *)W_GetLumpName ("PALETTE", &info);
	KAT (p != NULL && memcmp (p, "ABCD", 4) == 0);
	p = (byte *)W_GetLumpName ("conback", &info);
	KAT (p != NULL && info->type == TYP_QPIC && info->size == 12);
	KAT (p != NULL && ((qpic_t *)p)->width == 2 && ((qpic_t *)p)->height == 1);
	KAT (W_GetLumpName ("missing", &info) == NULL);
	release ();

	memcpy (wad, "WAD3", 4);
	load (wad, sizeof (wad));
	KAT (wad_numlumps == 0);
	KAT (W_GetLumpName ("palette", &info) == NULL);
	release ();

	memcpy (wad, "WAD2", 4);
	*(int *)(wad + 4) = 1000;
	load (wad, sizeof (wad));
	KAT (wad_numlumps == 0);
	release ();

	KAT_REPORT ("wad");
	return kat_failed != 0;
}
#endif
