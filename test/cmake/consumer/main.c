#include <stdio.h>
#include "ui.h"

static void (*const tooltipSetter)(uiControl *, const char *) = uiControlSetTooltip;

int libuiDummyExtensionVersionIsKnown(void);

int main(void)
{
	static const char utf8Text[] = "libui UTF-8: ✓";
	(void) utf8Text;
	printf("%s\n", uiVersion());
	return tooltipSetter != NULL && libuiDummyExtensionVersionIsKnown() ? 0 : 1;
}
