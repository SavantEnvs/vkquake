#include "quakedef.h"
#include "image.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void mockfs_set (const char *ext, const byte *data, size_t size);

static byte *load (const char *ext, const byte *data, size_t size, int *w, int *h, enum srcformat *fmt)
{
	mockfs_set (ext, data, size);
	return Image_LoadImage ("gfx/fuzz", w, h, fmt, 0);
}

/* The engine treats a malformed PCX header, or an LMP shorter than its own
 * 8-byte header, as fatal (Sys_Error). Only reject those; everything the
 * loaders accept is passed through untouched. */
static int pcx_header_ok (const byte *d, size_t n, int *w, int *h)
{
	unsigned xmin, ymin, xmax, ymax;
	if (n < 768)
		return 0;
	if (d[0] != 0x0A || d[1] != 5 || d[2] != 1 || d[3] != 8 || d[65] != 1)
		return 0;
	xmin = d[4] | d[5] << 8;
	ymin = d[6] | d[7] << 8;
	xmax = d[8] | d[9] << 8;
	ymax = d[10] | d[11] << 8;
	if (xmax < xmin || ymax < ymin)
		return 0;
	*w = (int)(xmax - xmin + 1);
	*h = (int)(ymax - ymin + 1);
	return *w <= 2048 && *h <= 2048;
}

#ifdef MAYHEM_KAT
#include "kat.h"
#else
int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
	enum srcformat fmt;
	int			   w = 0, h = 0, pw, ph;
	byte		  *img;

	if (size < 2)
		return -1;

	if (data[0] & 1)
	{
		if (size - 1 < 8)
			return -1;
		img = load ("lmp", data + 1, size - 1, &w, &h, &fmt);
		mockfs_set ("lmp", data + 1, size - 1);
		Image_GetLMPSize ("gfx/fuzz", &pw, &ph);
	}
	else
	{
		if (!pcx_header_ok (data + 1, size - 1, &pw, &ph))
			return -1;
		img = load ("pcx", data + 1, size - 1, &w, &h, &fmt);
	}
	if (!img)
		return -1;
	Mem_Free (img);
	return 0;
}
#endif

#ifdef MAYHEM_KAT
int main (void)
{
	byte		   pcx[128 + 6 + 768];
	byte		   lmp[8 + 4] = {2, 0, 0, 0, 2, 0, 0, 0, 7, 8, 9, 10};
	byte		  *p;
	int			   w = 0, h = 0, pw, ph, i;
	enum srcformat fmt = SRC_RGBA;

	memset (pcx, 0, sizeof (pcx));
	pcx[0] = 0x0A;
	pcx[1] = 5;
	pcx[2] = 1;
	pcx[3] = 8;
	pcx[8] = 1;	 /* xmax = 1 -> width 2 */
	pcx[10] = 1; /* ymax = 1 -> height 2 */
	pcx[65] = 1;
	pcx[66] = 2; /* bytes_per_line */
	pcx[128] = 1;
	pcx[129] = 2;
	pcx[130] = 0xC2; /* run of 2 */
	pcx[131] = 3;
	for (i = 0; i < 256; i++)
	{
		pcx[sizeof (pcx) - 768 + i * 3] = (byte)i;
		pcx[sizeof (pcx) - 768 + i * 3 + 1] = (byte)(i * 2);
		pcx[sizeof (pcx) - 768 + i * 3 + 2] = (byte)(255 - i);
	}
	KAT (pcx_header_ok (pcx, sizeof (pcx), &pw, &ph) && pw == 2 && ph == 2);
	p = load ("pcx", pcx, sizeof (pcx), &w, &h, &fmt);
	KAT (p != NULL && w == 2 && h == 2 && fmt == SRC_RGBA);
	if (p)
	{
		KAT (p[0] == 1 && p[1] == 2 && p[2] == 254 && p[3] == 255);
		KAT (p[4] == 2 && p[5] == 4 && p[6] == 253 && p[7] == 255);
		KAT (p[8] == 3 && p[9] == 6 && p[10] == 252 && p[11] == 255);
		KAT (p[12] == 3 && p[13] == 6 && p[14] == 252 && p[15] == 255);
		Mem_Free (p);
	}
	pcx[0] = 0;
	KAT (!pcx_header_ok (pcx, sizeof (pcx), &pw, &ph));

	p = load ("lmp", lmp, sizeof (lmp), &w, &h, &fmt);
	KAT (p != NULL && w == 2 && h == 2 && fmt == SRC_INDEXED);
	if (p)
	{
		KAT (p[0] == 7 && p[1] == 8 && p[2] == 9 && p[3] == 10);
		Mem_Free (p);
	}
	mockfs_set ("lmp", lmp, sizeof (lmp));
	Image_GetLMPSize ("gfx/fuzz", &pw, &ph);
	KAT (pw == 2 && ph == 2);
	KAT (load ("lmp", lmp, sizeof (lmp) - 1, &w, &h, &fmt) == NULL);

	KAT_REPORT ("pcx");
	return kat_failed != 0;
}
#endif
