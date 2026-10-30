#include "quakedef.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef MAYHEM_KAT
#include "kat.h"
#else
int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
	char		*text, *copy, *tmp, *mline;
	const char	*p;
	stringview_t line;
	size_t		 n, i, nsub;
	char		 out[64], out2[256];
	char	   **subs;
	int			 iv;
	float		 fv;

	if (size == 0)
		return -1;

	text = (char *)malloc (size + 1);
	memcpy (text, data, size);
	text[size] = '\0';

	for (p = text, i = 0; p && i < 100000; i++)
		p = COM_Parse (p);
	for (p = text, i = 0; p && i < 100000; i++)
		p = COM_ParseEx (p, CPE_NOTRUNC);
	for (p = text, i = 0; p && i < 100000; i++)
		p = COM_ParseEx (p, CPE_ALLOWTRUNC);

	p = text;
	for (i = 0; i < 100000 && COM_ParseLine (&p, &line); i++)
		;

	copy = (char *)malloc (size + 1);
	memcpy (copy, text, size + 1);
	tmp = copy;
	for (i = 0; i < 100000 && COM_ParseMutableLine (&tmp, &mline); i++)
		;
	free (copy);

	p = COM_ParseIntNewline (text, &iv);
	p = COM_ParseFloatNewline (text, &fv);
	p = COM_ParseStringNewline (text);
	(void)p;

	Cmd_TokenizeString (text);
	for (i = 0; i < (size_t)Cmd_Argc (); i++)
		(void)strlen (Cmd_Argv ((int)i));
	(void)Cmd_Args ();

	n = UTF8_FromQuake (NULL, 0, text);
	if (n)
	{
		char *u = (char *)malloc (n);
		UTF8_FromQuake (u, n, text);
		free (u);
	}

	copy = (char *)malloc (size + 1);
	memcpy (copy, text, size + 1);
	(void)q_strtrim (copy);
	free (copy);

	copy = (char *)malloc (size + 1);
	memcpy (copy, text, size + 1);
	subs = q_strsplit (copy, " \t\n,;/", &nsub);
	if (subs)
		for (i = 0; i < nsub; i++)
			(void)strlen (subs[i]);
	Mem_Free (subs);
	free (copy);

	COM_FileBase (text, out, sizeof (out));
	COM_StripExtension (text, out, sizeof (out));
	COM_AddExtension (out, ".bsp", sizeof (out));
	(void)COM_FileGetExtension (text);
	(void)COM_SkipPath (text);
	(void)COM_HashString (text);

	copy = (char *)malloc (size + 1);
	memcpy (copy, text, size + 1);
	COM_WordWrap (out2, copy, sizeof (out2), 40);
	free (copy);
	(void)mline;

	free (text);
	return 0;
}
#endif

#ifdef MAYHEM_KAT
int main (void)
{
	const char *p;
	int			iv = 0;
	float		fv = 0;
	char		buf[64];
	char		s1[] = "  ab \t";
	char		s2[] = "a,b;;c";
	size_t		nsub = 0, n;
	char	  **subs;

	p = COM_Parse ("  hello world");
	KAT (p && strcmp (com_token, "hello") == 0);
	p = COM_Parse (p);
	KAT (strcmp (com_token, "world") == 0);
	KAT (p != NULL && *p == '\0');
	p = COM_Parse (p);
	KAT (p == NULL && com_token[0] == '\0');

	p = COM_Parse ("\"a b\" c");
	KAT (strcmp (com_token, "a b") == 0);
	p = COM_Parse (p);
	KAT (strcmp (com_token, "c") == 0);

	p = COM_Parse ("// comment\nfoo");
	KAT (strcmp (com_token, "foo") == 0);

	p = COM_Parse ("{ x");
	KAT (strcmp (com_token, "{") == 0);

	Cmd_TokenizeString ("map e1m1 \"x y\"\nignored");
	KAT (Cmd_Argc () == 3);
	KAT (strcmp (Cmd_Argv (0), "map") == 0);
	KAT (strcmp (Cmd_Argv (1), "e1m1") == 0);
	KAT (strcmp (Cmd_Argv (2), "x y") == 0);
	KAT (strcmp (Cmd_Args (), "e1m1 \"x y\"\nignored") == 0 || strncmp (Cmd_Args (), "e1m1 \"x y\"", 10) == 0);
	Cmd_TokenizeString ("");
	KAT (Cmd_Argc () == 0);

	p = COM_ParseIntNewline ("42\nrest", &iv);
	KAT (iv == 42);
	KAT (p && strcmp (p, "rest") == 0);
	p = COM_ParseFloatNewline ("2.5\nrest", &fv);
	KAT (fv == 2.5f);
	KAT (p && strcmp (p, "rest") == 0);

	KAT (strcmp (q_strtrim (s1), "ab") == 0);
	subs = q_strsplit (s2, ",;", &nsub);
	KAT (subs != NULL && nsub == 3);
	if (subs && nsub == 3)
		KAT (strcmp (subs[0], "a") == 0 && strcmp (subs[1], "b") == 0 && strcmp (subs[2], "c") == 0);
	Mem_Free (subs);

	COM_FileBase ("maps/e1m1.bsp", buf, sizeof (buf));
	KAT (strcmp (buf, "e1m1") == 0);
	COM_StripExtension ("maps/e1m1.bsp", buf, sizeof (buf));
	KAT (strcmp (buf, "maps/e1m1") == 0);
	KAT (strcmp (COM_FileGetExtension ("a/b.tga"), "tga") == 0);
	KAT (strcmp (COM_SkipPath ("a/b/c.lmp"), "c.lmp") == 0);

	n = UTF8_FromQuake (buf, sizeof (buf), "Hi");
	KAT (n == 3 && strcmp (buf, "Hi") == 0);
	KAT (UTF8_CodePointLength (0x41) == 1);
	KAT (UTF8_CodePointLength (0xe9) == 2);
	KAT (UTF8_CodePointLength (0x1f600) == 4);
	KAT (UTF8_WriteCodePoint (buf, sizeof (buf), 0x20ac) == 3 && memcmp (buf, "\xe2\x82\xac", 3) == 0);

	KAT_REPORT ("text");
	return kat_failed != 0;
}
#endif
