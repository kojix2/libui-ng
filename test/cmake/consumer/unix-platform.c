#include <gtk/gtk.h>
#include "ui.h"
#include "ui_unix.h"

int main(void)
{
	return sizeof(uiUnixControl) == 0;
}
