// 25 may 2016
#include "uipriv_unix.h"
#include "attrstr.h"

int uiprivGraphemesTakesUTF16(void)
{
	return 0;
}

uiprivGraphemes *uiprivNewGraphemes(void *s, size_t len)
{
	uiprivGraphemes *g;
	char *text = (char *) s;
	size_t lenchars;
	PangoLogAttr *logattrs;
	size_t i;
	size_t gpos;
	char *p, *next;

	g = uiprivNew(uiprivGraphemes);

	lenchars = g_utf8_strlen(text, -1);
	logattrs = (PangoLogAttr *) uiprivAlloc((lenchars + 1) * sizeof (PangoLogAttr), "PangoLogAttr[] (graphemes)");
	pango_get_log_attrs(text, len,
		-1, NULL,
		logattrs, lenchars + 1);

	// first figure out how many graphemes there are
	g->len = 0;
	for (i = 0; i < lenchars; i++)
		if (logattrs[i].is_cursor_position != 0)
			g->len++;

	g->pointsToGraphemes = (size_t *) uiprivAlloc((len + 1) * sizeof (size_t), "size_t[] (graphemes)");
	g->graphemesToPoints = (size_t *) uiprivAlloc((g->len + 1) * sizeof (size_t), "size_t[] (graphemes)");

	// compute both index conversion arrays in a single pass
	gpos = 0;
	p = text;
	for (i = 0; i < lenchars; i++) {
		next = g_utf8_next_char(p);
		if (logattrs[i].is_cursor_position != 0)
			g->graphemesToPoints[gpos++] = p - text;
		for (; p < next; p++)
			g->pointsToGraphemes[p - text] = gpos - 1;
	}
	// and set the entries for the end of the string
	g->graphemesToPoints[gpos] = len;
	g->pointsToGraphemes[len] = gpos;

	uiprivFree(logattrs);
	return g;
}
