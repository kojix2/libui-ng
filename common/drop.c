#include <stdlib.h>
#include "../ui.h"
#include "uipriv.h"
#include "drop.h"

struct uiprivDropRegistration {
	uiDropHandler handler;
	void *data;
	unsigned int formats;
	unsigned int refs;
};

static void releaseRegistration(uiprivDropRegistration *r)
{
	if (r == NULL || --r->refs != 0)
		return;
	if (r->handler.ReleaseUserData != NULL) {
		uiprivUserCallbackEnter(NULL);
		r->handler.ReleaseUserData(r->data);
		uiprivUserCallbackLeave();
	}
	uiprivFree(r);
}

uiDropStatus uiprivDropValidate(unsigned int formats, const uiDropHandler *h, void *data)
{
	if (h == NULL)
		return formats == 0 && data == NULL ? uiDropStatusOK : uiDropStatusInvalidArgument;
	if (formats == 0 || (formats & ~(uiDropFormatText | uiDropFormatFiles)) != 0 ||
		h->Size < offsetof(uiDropHandler, Drop) + sizeof h->Drop || h->Drop == NULL)
		return uiDropStatusInvalidArgument;
	return uiDropStatusOK;
}

uiprivDropEndpoint *uiprivDropNew(uiControl *owner)
{
	uiprivDropEndpoint *e = uiprivNew(uiprivDropEndpoint);
	e->owner = owner;
	e->refs = 1;
	e->generation = 1;
	return e;
}

void uiprivDropRetain(uiprivDropEndpoint *e) { e->refs++; }

void uiprivDropRelease(uiprivDropEndpoint *e)
{
	if (--e->refs != 0)
		return;
	if (e->dispose != NULL)
		e->dispose(e);
	uiprivFree(e);
}

uiDropStatus uiprivDropSetHandler(uiprivDropEndpoint *e, unsigned int formats,
	const uiDropHandler *h, void *data)
{
	uiprivDropRegistration *r = NULL, *old;
	uiDropStatus status = uiprivDropValidate(formats, h, data);
	if (status != uiDropStatusOK)
		return status;
	if (e->owner == NULL || uiprivControlDestroyPending(e->owner))
		return uiDropStatusUnavailable;
	if (h != NULL) {
		r = uiprivNew(uiprivDropRegistration);
		r->handler.Size = sizeof r->handler;
		r->handler.Drop = h->Drop;
#define COPY_FIELD(f) if (h->Size >= offsetof(uiDropHandler, f) + sizeof h->f) r->handler.f = h->f
		COPY_FIELD(Over);
		COPY_FIELD(Leave);
		COPY_FIELD(ReleaseUserData);
#undef COPY_FIELD
		r->formats = formats;
		r->data = data;
		r->refs = 1;
	}
	uiprivDropRetain(e);
	uiprivUserCallbackEnter(NULL);
	old = e->current;
	if ((old == NULL) != (r == NULL) && e->enable != NULL) {
		status = e->enable(e, r != NULL);
		if (status != uiDropStatusOK) {
			if (r != NULL)
				uiprivFree(r); // Failed registrations never own userdata.
			goto done;
		}
	}
	e->generation++;
	e->current = r;
	if (e->cancel != NULL)
		e->cancel(e);
	releaseRegistration(old);
done:
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return status;
}

void uiprivDropDestroy(uiprivDropEndpoint *e)
{
	uiprivDropRegistration *old;
	if (e == NULL)
		return;
	uiprivUserCallbackEnter(NULL);
	old = e->current;
	e->current = NULL;
	e->owner = NULL;
	e->generation++;
	if (e->enable != NULL)
		e->enable(e, 0);
	if (e->cancel != NULL)
		e->cancel(e);
	releaseRegistration(old);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
}

int uiprivDropActive(uiprivDropEndpoint *e, uint64_t generation)
{
	return e != NULL && e->owner != NULL && e->current != NULL &&
		e->generation == generation && !uiprivControlDestroyPending(e->owner) &&
		uiControlEnabledToUser(e->owner);
}

uiDropInfo uiprivDropMakeInfo(uiprivDropEndpoint *e, unsigned int formats, double x, double y)
{
	uiDropInfo info;
	unsigned int enabled = e->current == NULL ? 0 : formats & e->current->formats;
	info.Formats = formats & (uiDropFormatFiles | uiDropFormatText);
	info.Format = (enabled & uiDropFormatFiles) ? uiDropFormatFiles :
		(enabled & uiDropFormatText) ? uiDropFormatText : uiDropFormatNone;
	info.X = x;
	info.Y = y;
	return info;
}

uiDropAction uiprivDropOver(uiprivDropEndpoint *e, const uiDropInfo *info, int copy)
{
	uiprivDropRegistration *r;
	uiDropAction action = uiDropActionNone;
	uint64_t generation = e->generation;
	if (!copy || info->Format == uiDropFormatNone || !uiprivDropActive(e, generation))
		return action;
	uiprivDropRetain(e);
	r = e->current;
	r->refs++;
	uiprivUserCallbackEnter(e->owner);
	action = r->handler.Over == NULL ? uiDropActionCopy : r->handler.Over(info, r->data);
	if (!uiprivDropActive(e, generation) || action != uiDropActionCopy)
		action = uiDropActionNone;
	releaseRegistration(r);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return action;
}

void uiprivDropLeave(uiprivDropEndpoint *e, uint64_t generation)
{
	uiprivDropRegistration *r;
	if (!uiprivDropActive(e, generation) || e->current->handler.Leave == NULL)
		return;
	uiprivDropRetain(e);
	r = e->current;
	r->refs++;
	uiprivUserCallbackEnter(e->owner);
	r->handler.Leave(r->data);
	releaseRegistration(r);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
}

uiDropResult uiprivDropDeliver(uiprivDropEndpoint *e, uint64_t generation,
	const uiDropInfo *info, const uiDropData *payload)
{
	uiprivDropRegistration *r;
	uiDropResult result;
	if (!uiprivDropActive(e, generation) || info->Format == uiDropFormatNone ||
		payload->Format != info->Format || !(e->current->formats & payload->Format))
		return uiDropRejected;
	uiprivDropRetain(e);
	r = e->current;
	r->refs++;
	uiprivUserCallbackEnter(e->owner);
	result = r->handler.Drop(info, payload, r->data);
	// Drop may destroy its owner after consuming the payload and still accept.
	releaseRegistration(r);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return result == uiDropAccepted ? uiDropAccepted : uiDropRejected;
}

static char *copyBytes(const char *s, size_t n)
{
	char *out = (char *) uiprivAlloc(n + 1, "drop string");
	memcpy(out, s, n);
	out[n] = 0;
	return out;
}

int uiprivDropSetText(uiprivDropData *p, const char *s, size_t n)
{
	size_t i = 0;
	if (s == NULL || n >= uiprivDropMaxBytes || memchr(s, 0, n) != NULL ||
		p->data.Format != uiDropFormatNone)
		return 0;
	while (i < n) {
		uint32_t rune;
		const char *next = uiprivUTF8DecodeRune(s + i, n - i, &rune);
		// DecodeRune consumes one byte for malformed sequences; U+FFFD itself
		// is a valid three-byte sequence and must remain accepted.
		if ((unsigned char) s[i] >= 0x80 && next == s + i + 1)
			return 0;
		i = (size_t) (next - s);
	}
	p->data.Text = copyBytes(s, n);
	p->data.TextLength = n;
	p->bytes = n + 1;
	p->data.Format = uiDropFormatText;
	return 1;
}

int uiprivDropAddFile(uiprivDropData *p, const char *s, size_t n)
{
	if (s == NULL || n == 0 || n >= uiprivDropMaxBytes ||
		p->bytes > uiprivDropMaxBytes - n - 1 || p->data.FileCount >= uiprivDropMaxFiles ||
		memchr(s, 0, n) != NULL ||
		(p->data.Format != uiDropFormatNone && p->data.Format != uiDropFormatFiles))
		return 0;
	// Grow geometrically rather than reallocating for every path.
	if ((p->data.FileCount & (p->data.FileCount - 1)) == 0)
		p->files = (const char **) uiprivRealloc(p->files,
			((p->data.FileCount == 0 ? 1 : p->data.FileCount * 2) + 1) * sizeof *p->files, "drop files");
	p->files[p->data.FileCount++] = copyBytes(s, n);
	p->files[p->data.FileCount] = NULL;
	p->data.Files = p->files;
	p->bytes += n + 1;
	p->data.Format = uiDropFormatFiles;
	return 1;
}

void uiprivDropDataClear(uiprivDropData *p)
{
	size_t i;
	if (p->data.Text != NULL)
		uiprivFree((void *) p->data.Text);
	for (i = 0; i < p->data.FileCount; i++)
		uiprivFree((void *) p->files[i]);
	if (p->files != NULL)
		uiprivFree(p->files);
	memset(p, 0, sizeof *p);
}

static char *copyPublicText(const char *s, size_t n)
{
	char *out;
#ifdef _WIN32
	out = (char *) uiprivAlloc(n + 1, "drop text copy");
#else
	// uiFreeText() uses free()/g_free() on Darwin/Unix, unlike Windows.
	out = (char *) malloc(n + 1);
	if (out == NULL) abort();
#endif
	memcpy(out, s, n + 1);
	return out;
}

char *uiDropDataCopyText(const uiDropData *p)
{
	return p->Format == uiDropFormatText ? copyPublicText(p->Text, p->TextLength) : NULL;
}

char **uiDropDataCopyFiles(const uiDropData *p)
{
	char **out;
	size_t i;
	if (p->Format != uiDropFormatFiles)
		return NULL;
	out = uiprivNewTextArray(p->FileCount);
	if (out != NULL)
		for (i = 0; i < p->FileCount; i++)
			out[i] = copyPublicText(p->Files[i], strlen(p->Files[i]));
	return out;
}
