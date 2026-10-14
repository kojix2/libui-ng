#include <stdio.h>
#include <stdlib.h>
#include "ui.h"

static void (*const tooltipSetter)(uiControl *, const char *) = uiControlSetTooltip;

int libuiDummyExtensionVersionIsKnown(void);

int main(void)
{
	static const char utf8Text[] = "libui UTF-8: ✓";
	(void) utf8Text;
	printf("%s\n", uiVersion());
	uiFreeTextArray(NULL);
	if (getenv("LIBUI_TEST_DIALOG") != NULL) {
		char **paths = uiOpenFiles(NULL);
		uiFreeTextArray(paths);
		paths = uiOpenFolders(NULL);
		uiFreeTextArray(paths);
	}
	return tooltipSetter != NULL && libuiDummyExtensionVersionIsKnown() ? 0 : 1;
}
