/* Known-answer-test helpers for the MAYHEM_KAT builds of each harness. */
#ifndef MAYHEM_KAT_H
#define MAYHEM_KAT_H

#include <stdio.h>

static int kat_passed, kat_failed;

#define KAT(cond)                                                          \
	do                                                                     \
	{                                                                      \
		if (cond)                                                          \
			kat_passed++;                                                  \
		else                                                               \
		{                                                                  \
			kat_failed++;                                                  \
			printf ("KAT FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
		}                                                                  \
	} while (0)

#define KAT_REPORT(name) printf ("KAT %s passed=%d failed=%d\n", name, kat_passed, kat_failed)

#endif
