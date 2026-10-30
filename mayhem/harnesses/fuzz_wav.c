#include "quakedef.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MAYHEM_KAT
#include "kat.h"

static void u32 (byte *p, unsigned v)
{
	p[0] = v & 255;
	p[1] = v >> 8 & 255;
	p[2] = v >> 16 & 255;
	p[3] = v >> 24 & 255;
}

static size_t make_wav (byte *w, int format, int channels, int rate, int bits, int datalen)
{
	memcpy (w, "RIFF", 4);
	u32 (w + 4, 36 + datalen);
	memcpy (w + 8, "WAVE", 4);
	memcpy (w + 12, "fmt ", 4);
	u32 (w + 16, 16);
	w[20] = format;
	w[21] = 0;
	w[22] = channels;
	w[23] = 0;
	u32 (w + 24, rate);
	u32 (w + 28, rate * channels * bits / 8);
	w[32] = channels * bits / 8;
	w[33] = 0;
	w[34] = bits;
	w[35] = 0;
	memcpy (w + 36, "data", 4);
	u32 (w + 40, datalen);
	memset (w + 44, 0x55, datalen);
	return 44 + datalen;
}

int main (void)
{
	byte	  w[256];
	wavinfo_t info;
	size_t	  n;

	n = make_wav (w, 1, 1, 11025, 8, 4);
	info = GetWavinfo ("a.wav", w, (int)n);
	KAT (info.rate == 11025 && info.width == 1 && info.channels == 1);
	KAT (info.samples == 4 && info.dataofs == 44 && info.loopstart == -1);

	n = make_wav (w, 1, 2, 44100, 16, 8);
	info = GetWavinfo ("b.wav", w, (int)n);
	KAT (info.rate == 44100 && info.width == 2 && info.channels == 2);
	KAT (info.samples == 4 && info.dataofs == 44);

	n = make_wav (w, 3, 1, 11025, 8, 4);
	info = GetWavinfo ("c.wav", w, (int)n);
	KAT (info.rate == 0 && info.samples == 0);

	n = make_wav (w, 1, 1, 11025, 12, 4);
	info = GetWavinfo ("d.wav", w, (int)n);
	KAT (info.width == 0 && info.samples == 0);

	n = make_wav (w, 1, 1, 11025, 8, 4);
	memcpy (w + 36, "junk", 4);
	info = GetWavinfo ("e.wav", w, (int)n);
	KAT (info.samples == 0 && info.dataofs == 0);

	memcpy (w, "RIFX", 4);
	info = GetWavinfo ("f.wav", w, (int)n);
	KAT (info.rate == 0);

	info = GetWavinfo ("g.wav", NULL, 0);
	KAT (info.rate == 0 && info.samples == 0);

	KAT_REPORT ("wav");
	return kat_failed != 0;
}
#else
int main (int argc, char **argv)
{
	FILE	 *f;
	long	  len;
	byte	 *buf;
	wavinfo_t info;

	if (argc < 2)
		return 2;
	f = fopen (argv[1], "rb");
	if (!f)
		return 2;
	fseek (f, 0, SEEK_END);
	len = ftell (f);
	fseek (f, 0, SEEK_SET);
	if (len <= 0 || len > (1 << 24))
	{
		fclose (f);
		return 2;
	}
	buf = (byte *)malloc ((size_t)len);
	if (fread (buf, 1, (size_t)len, f) != (size_t)len)
	{
		fclose (f);
		free (buf);
		return 2;
	}
	fclose (f);

	info = GetWavinfo ("fuzz.wav", buf, (int)len);
	free (buf);
	return info.samples > 0 ? 0 : 1;
}
#endif
