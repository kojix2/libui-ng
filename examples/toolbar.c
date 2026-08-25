#include <stdio.h>
#include <stdint.h>
#include <ui.h>
#include "toolbar/data.h"

static uiToolbar *toolbar;
static uiImage *icons[4];

static int base64Value(char c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	if (c >= '0' && c <= '9')
		return c - '0' + 52;
	if (c == '+')
		return 62;
	if (c == '/')
		return 63;
	return -1;
}

static void decodePixels(const char *const encoded[], uint8_t *pixels, size_t len)
{
	unsigned value = 0;
	int bits = -8;
	size_t out = 0;

	for (; *encoded != NULL && out < len; encoded++) {
		const char *p;

		for (p = *encoded; *p != '\0' && *p != '=' && out < len; p++) {
			int digit = base64Value(*p);

			if (digit < 0)
				continue;
			value = (value << 6) | (unsigned) digit;
			bits += 6;
			if (bits >= 0) {
				pixels[out++] = (uint8_t) (value >> bits);
				bits -= 8;
			}
		}
	}
}

static uiImage *newIcon(const char *const pixels16[],
	const char *const pixels32[])
{
	uint8_t pixels[32 * 32 * 4];
	uiImage *image;

	image = uiNewImage(16, 16);
	decodePixels(pixels16, pixels, 16 * 16 * 4);
	uiImageAppend(image, pixels, 16, 16, 16 * 4);
	decodePixels(pixels32, pixels, 32 * 32 * 4);
	uiImageAppend(image, pixels, 32, 32, 32 * 4);
	return image;
}

static void onAction(uiToolbarItem *item, void *data)
{
	puts((const char *) data);
}

static void onToggle(uiToolbarItem *item, void *data)
{
	printf("Pinned: %s\n", uiToolbarItemChecked(item) ? "yes" : "no");
}

static int onClosing(uiWindow *w, void *data)
{
	uiWindowSetToolbar(w, NULL);
	uiFreeToolbar(toolbar);
	toolbar = NULL;
	uiFreeImage(icons[3]);
	uiFreeImage(icons[2]);
	uiFreeImage(icons[1]);
	uiFreeImage(icons[0]);
	uiQuit();
	return 1;
}

int main(void)
{
	uiInitOptions options = {0};
	uiWindow *window;
	uiToolbarItem *item;
	const char *err;

	err = uiInit(&options);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	window = uiNewWindow("Toolbar", 640, 400, 0);
	uiWindowOnClosing(window, onClosing, NULL);
	uiWindowSetChild(window, uiControl(uiNewLabel("Toolbar example")));

	icons[0] = newIcon(iconNew16, iconNew32);
	icons[1] = newIcon(iconOpen16, iconOpen32);
	icons[2] = newIcon(iconPin16, iconPin32);
	icons[3] = newIcon(iconHelp16, iconHelp32);
	toolbar = uiNewToolbar();
	item = uiToolbarAppendButton(toolbar, "New", icons[0]);
	uiToolbarItemSetTooltip(item, "New document");
	uiToolbarItemOnClicked(item, onAction, "New clicked");
	item = uiToolbarAppendButton(toolbar, "Open", icons[1]);
	uiToolbarItemSetTooltip(item, "Open document");
	uiToolbarItemOnClicked(item, onAction, "Open clicked");
	uiToolbarAppendSeparator(toolbar);
	item = uiToolbarAppendToggleButton(toolbar, "Pinned", icons[2]);
	uiToolbarItemSetTooltip(item, "Pin document");
	uiToolbarItemOnClicked(item, onToggle, NULL);
	item = uiToolbarAppendButton(toolbar, "Help", icons[3]);
	uiToolbarItemSetTooltip(item, "Show help");
	uiToolbarItemOnClicked(item, onAction, "Help clicked");
	uiWindowSetToolbar(window, toolbar);

	uiControlShow(uiControl(window));
	uiMain();
	uiUninit();
	return 0;
}
