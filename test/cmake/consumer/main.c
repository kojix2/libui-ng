#include <stdio.h>
#include "ui.h"

int libuiDummyExtensionVersionIsKnown(void);

int main(void)
{
	printf("%s\n", uiVersion());
	return libuiDummyExtensionVersionIsKnown() ? 0 : 1;
}
