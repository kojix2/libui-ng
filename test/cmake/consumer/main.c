#include <stdio.h>
#include "ui.h"

int libuiDummyExtensionVersionIsKnown(void);

int main(void)
{
	static const char utf8Text[] = "libui UTF-8: ✓";
	(void) utf8Text;
	printf("%s\n", uiVersion());
	return libuiDummyExtensionVersionIsKnown() ? 0 : 1;
}
