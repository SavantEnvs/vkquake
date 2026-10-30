#include "quakedef.h"
#include "image.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void mockfs_set (const char *ext, const byte *data, size_t size);

static const char *const exts[] = {"png", "tga", "jpg"};

static byte *load (const char *ext, const byte *data, size_t size, int *w, int *h)
{
	enum srcformat fmt;
	mockfs_set (ext, data, size);
	return Image_LoadImage ("textures/fuzz", w, h, &fmt, 0);
}

#ifdef MAYHEM_KAT
#include "kat.h"
#include "kat_image_data.h"
#else
int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
	int	  w = 0, h = 0;
	byte *rgba;

	if (size < 2)
		return -1;

	rgba = load (exts[data[0] % 3], data + 1, size - 1, &w, &h);
	if (!rgba)
		return -1;
	if (w > 0 && h > 0)
		(void)rgba[(size_t)w * (size_t)h * 4 - 1];
	Mem_Free (rgba);
	return 0;
}
#endif

#ifdef MAYHEM_KAT
int main (void)
{
	int	  w = 0, h = 0;
	byte *p;

	p = load ("png", kat_png, sizeof (kat_png), &w, &h);
	KAT (p != NULL && w == 2 && h == 1);
	if (p)
	{
		KAT (p[0] == 10 && p[1] == 20 && p[2] == 30 && p[3] == 255);
		KAT (p[4] == 200 && p[5] == 100 && p[6] == 50 && p[7] == 255);
		Mem_Free (p);
	}

	w = h = 0;
	p = load ("tga", kat_tga, sizeof (kat_tga), &w, &h);
	KAT (p != NULL && w == 2 && h == 1);
	if (p)
	{
		KAT (p[0] == 10 && p[1] == 20 && p[2] == 30 && p[3] == 255);
		KAT (p[4] == 200 && p[5] == 100 && p[6] == 50 && p[7] == 255);
		Mem_Free (p);
	}

	w = h = 0;
	p = load ("jpg", kat_jpg, sizeof (kat_jpg), &w, &h);
	KAT (p != NULL && w == 8 && h == 8);
	if (p)
	{
		KAT (p[3] == 255);
		KAT (p[0] >= 124 && p[0] <= 132 && p[1] >= 124 && p[1] <= 132 && p[2] >= 124 && p[2] <= 132);
		Mem_Free (p);
	}

	KAT (load ("png", (const byte *)"not an image at all", 19, &w, &h) == NULL);
	KAT (load ("png", kat_png, 20, &w, &h) == NULL);
	KAT (load ("jpg", kat_jpg, 40, &w, &h) == NULL);
	KAT (load ("bmp", kat_png, sizeof (kat_png), &w, &h) == NULL);

	KAT_REPORT ("image");
	return kat_failed != 0;
}
#endif
