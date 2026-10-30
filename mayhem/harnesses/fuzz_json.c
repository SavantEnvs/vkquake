#include "quakedef.h"
#include "json.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void walk (const jsonentry_t *e, int depth, size_t *count)
{
	for (; e; e = e->next)
	{
		(*count)++;
		if (e->type == JSON_STRING && e->string)
			*count += strlen (e->string);
		if (e->firstchild && depth < 256)
			walk (e->firstchild, depth + 1, count);
	}
}

#ifdef MAYHEM_KAT
#include "kat.h"
#else
int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
	char   *text;
	json_t *json;
	size_t	count = 0;

	if (size == 0)
		return -1;

	text = (char *)malloc (size + 1);
	memcpy (text, data, size);
	text[size] = '\0';

	json = JSON_Parse (text);
	free (text);
	if (!json)
		return -1;

	walk (json->root, 0, &count);
	(void)JSON_FindString (json->root, "name");
	(void)JSON_FindNumber (json->root, "version");
	(void)JSON_FindBoolean (json->root, "enabled");
	(void)JSON_Find (json->root, "items", JSON_ARRAY);
	JSON_Free (json);
	return 0;
}
#endif

#ifdef MAYHEM_KAT
static json_t *parse (const char *s)
{
	return JSON_Parse (s);
}

int main (void)
{
	json_t *j = parse ("{\"a\":\"b\",\"n\":12.5,\"t\":true,\"f\":false,\"z\":null,\"arr\":[1,2,3]}");
	KAT (j != NULL);
	if (j)
	{
		const char	   *a = JSON_FindString (j->root, "a");
		const double   *n = JSON_FindNumber (j->root, "n");
		const qboolean *t = JSON_FindBoolean (j->root, "t");
		const qboolean *f = JSON_FindBoolean (j->root, "f");
		const jsonentry_t *arr = JSON_Find (j->root, "arr", JSON_ARRAY);
		const jsonentry_t *z = JSON_Find (j->root, "z", JSON_NULL);
		KAT (j->root->type == JSON_OBJECT);
		KAT (a && strcmp (a, "b") == 0);
		KAT (n && *n == 12.5);
		KAT (t && *t == true);
		KAT (f && *f == false);
		KAT (z != NULL);
		KAT (JSON_FindString (j->root, "missing") == NULL);
		KAT (JSON_FindNumber (j->root, "a") == NULL);
		KAT (arr && arr->firstchild && arr->firstchild->type == JSON_NUMBER && arr->firstchild->number == 1.0);
		KAT (arr && arr->lastchild && arr->lastchild->number == 3.0);
		JSON_Free (j);
	}

	j = parse ("{\"s\":\"x\\u00e9\\n\\t\\\"q\\\"\"}");
	KAT (j != NULL);
	if (j)
	{
		const char *s = JSON_FindString (j->root, "s");
		KAT (s && strcmp (s, "x\xc3\xa9\n\t\"q\"") == 0);
		JSON_Free (j);
	}

	j = parse ("{\"s\":\"\\ud83d\\ude00\"}");
	KAT (j != NULL);
	if (j)
	{
		const char *s = JSON_FindString (j->root, "s");
		KAT (s && strcmp (s, "\xf0\x9f\x98\x80") == 0);
		JSON_Free (j);
	}

	j = parse ("{\"s\":\"\\ud800\"}");
	KAT (j != NULL);
	if (j)
	{
		const char *s = JSON_FindString (j->root, "s");
		KAT (s && strcmp (s, "\xef\xbf\xbd") == 0);
		JSON_Free (j);
	}

	j = parse ("\xef\xbb\xbf{\"k\":1}");
	KAT (j != NULL);
	if (j)
	{
		const double *k = JSON_FindNumber (j->root, "k");
		KAT (k && *k == 1.0);
		JSON_Free (j);
	}

	KAT (parse ("{\"a\":") == NULL);
	KAT (parse ("{\"a\":1") == NULL);
	KAT (parse ("\xff\xfe{\"k\":1}") == NULL);
	KAT (parse ("") == NULL);

	KAT_REPORT ("json");
	return kat_failed != 0;
}
#endif
