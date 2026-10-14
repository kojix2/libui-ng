#include "../ui.h"
#include "uipriv.h"

char **uiprivNewTextArray(size_t count)
{
	char **texts;
	size_t i;

	if (count > SIZE_MAX / sizeof(char *) - 1)
		return NULL;
	texts = (char **) uiprivAlloc((count + 1) * sizeof(char *), "char *[]");
	for (i = 0; i <= count; i++)
		texts[i] = NULL;
	return texts;
}

void uiFreeTextArray(char **texts)
{
	size_t i;

	if (texts == NULL)
		return;
	for (i = 0; texts[i] != NULL; i++)
		uiFreeText(texts[i]);
	uiprivFree(texts);
}
