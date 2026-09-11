#include <string.h>
#include "ui.h"

int libuiDummyExtensionVersionIsKnown(void)
{
	return strcmp(uiVersion(), "") != 0;
}
