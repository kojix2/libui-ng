// 9 april 2015
#include "uipriv_unix.h"

char *uiUnixStrdupText(const char *t)
{
	return g_strdup(t);
}

void uiFreeText(char *t)
{
	g_free(t);
}

int uiprivStricmp(const char *a, const char *b)
{
	char *afold, *bfold;
	int result;

	afold = g_utf8_casefold(a, -1);
	bfold = g_utf8_casefold(b, -1);
	result = g_strcmp0(afold, bfold);
	g_free(bfold);
	g_free(afold);
	return result;
}
