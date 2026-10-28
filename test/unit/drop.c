#include "drop_test.h"

typedef struct context {
	uiprivDropEndpoint *e;
	int drops, overs, leaves, releases;
	int mode;
} context;
static void released(void *data) { ((context *) data)->releases++; }
static void left(void *data) { ((context *) data)->leaves++; }
static uiDropResult dropped(const uiDropInfo *info, const uiDropData *payload, void *data)
{
	context *c = data;
	c->drops++;
	assert_string_equal(payload->Text, "hello");
	if (c->mode == 1) {
		assert_int_equal(uiprivDropSetHandler(c->e, 0, NULL, NULL), uiDropStatusOK);
		assert_int_equal(c->releases, 0);
	} else if (c->mode == 2) {
		uiControlDestroy(c->e->owner);
		assert_true(uiprivControlDestroyPending(c->e->owner));
		assert_int_equal(c->releases, 0);
	} else if (c->mode == 3)
		uiControlDestroy(uiControlParent(c->e->owner));
	return uiDropAccepted;
}
static uiDropHandler handler(void)
{
	uiDropHandler h = {sizeof h, dropped, NULL, left, released};
	return h;
}
static uiDropAction over(const uiDropInfo *info, void *data)
{
	context *c = data;
	uiDropHandler h = handler();
	c->overs++;
	if (c->mode == 1) {
		assert_int_equal(uiprivDropSetHandler(c->e, uiDropFormatText, &h, c), uiDropStatusOK);
		assert_int_equal(c->releases, 0);
	} else if (c->mode == 2)
		uiControlDestroy(c->e->owner);
	else if (c->mode == 3)
		uiControlDestroy(uiControlParent(c->e->owner));
	return uiDropActionCopy;
}
static uiDropStatus nativeFailure(uiprivDropEndpoint *e, int active)
{
	return uiDropStatusNativeError;
}

static void registrationFailureIsAtomic(void **state)
{
	uiprivDropEndpoint *e = newEndpoint();
	context c = {0}, failed = {0};
	uiDropHandler h = handler();
	uint64_t generation;
	c.e = e;
	e->enable = nativeFailure;
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, &failed), uiDropStatusNativeError);
	assert_int_equal(failed.releases, 0);
	e->enable = NULL;
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, &c), uiDropStatusOK);
	generation = e->generation;
	e->enable = nativeFailure;
	assert_int_equal(uiprivDropSetHandler(e, 0, NULL, NULL), uiDropStatusNativeError);
	assert_true(uiprivDropActive(e, generation));
	assert_int_equal(c.releases, 0);
	e->enable = NULL;
	destroyEndpoint(e);
	assert_int_equal(c.releases, 1);
}

static void copiedPartialHandler(void **state)
{
	uiprivDropEndpoint *e = newEndpoint();
	context c = {0};
	uiDropHandler h = handler();
	uiDropInfo info;
	c.e = e;
	// The partial optional pointer must not be read.
	h.Size = offsetof(uiDropHandler, Leave) + sizeof h.Leave - 1;
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatFiles | uiDropFormatText, &h, &c), uiDropStatusOK);
	memset(&h, 0, sizeof h);
	info = uiprivDropMakeInfo(e, uiDropFormatFiles | uiDropFormatText, 12.5, 300.25);
	assert_int_equal(uiprivDropOver(e, &info, 0), uiDropActionNone);
	assert_int_equal(uiprivDropOver(e, &info, 1), uiDropActionCopy);
	uiprivDropLeave(e, e->generation);
	assert_int_equal(c.leaves, 0);
	destroyEndpoint(e);
	assert_int_equal(c.releases, 0);
}

static void staleTransfersAndReentrantReplacement(void **state)
{
	uiprivDropEndpoint *e = newEndpoint();
	context a = {0}, b = {0};
	uiDropHandler h = handler();
	uiprivDropData payload = {0};
	uiDropInfo info;
	uint64_t generation;
	a.e = b.e = e;
	assert_true(uiprivDropSetText(&payload, "hello", 5));
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, &a), uiDropStatusOK);
	generation = e->generation;
	info = uiprivDropMakeInfo(e, uiDropFormatText, 0, 0);
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, &b), uiDropStatusOK);
	assert_int_equal(a.releases, 1);
	assert_int_equal(uiprivDropDeliver(e, generation, &info, &payload.data), uiDropRejected);
	assert_int_equal(b.drops, 0);
	h.Over = over;
	assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, &a), uiDropStatusOK);
	a.releases = 0;
	a.mode = 1;
	assert_int_equal(uiprivDropOver(e, &info, 1), uiDropActionNone);
	assert_int_equal(a.releases, 1); // old registration released after Over returned
	a.mode = 1;
	a.releases = 0;
	assert_int_equal(uiprivDropDeliver(e, e->generation, &info, &payload.data), uiDropAccepted);
	assert_int_equal(a.releases, 1); // current callback may remove itself and accept
	assert_int_equal(uiprivDropSetHandler(e, 0, NULL, NULL), uiDropStatusOK);
	uiprivDropDataClear(&payload);
	destroyEndpoint(e);
}

static void destroyDuringOverAndDrop(void **state)
{
	for (int scenario = 0; scenario < 4; scenario++) {
		int drop = scenario % 2;
		uiprivDropEndpoint *e = newEndpoint();
		context c = {0};
		uiDropHandler h = handler();
		uiDropInfo info;
		uiprivDropData payload = {0};
		c.e = e;
		c.mode = scenario < 2 ? 2 : 3;
		if (c.mode == 3) {
			uiprivDropEndpoint *parent = newEndpoint();
			((testControl *) parent->owner)->child = (testControl *) e->owner;
			((testControl *) e->owner)->parent = parent->owner;
		}
		h.Over = over;
		assert_int_equal(uiprivDropSetHandler(e, uiDropFormatText, &h, &c), uiDropStatusOK);
		info = uiprivDropMakeInfo(e, uiDropFormatText, 0, 0);
		if (drop) {
			assert_true(uiprivDropSetText(&payload, "hello", 5));
			assert_int_equal(uiprivDropDeliver(e, e->generation, &info, &payload.data), uiDropAccepted);
			uiprivDropDataClear(&payload);
		} else
			assert_int_equal(uiprivDropOver(e, &info, 1), uiDropActionNone);
		assert_int_equal(c.releases, 0);
		uiprivControlDestroyFlushPending();
		assert_int_equal(c.releases, 1);
		assert_int_equal(allocations, 0);
	}
}

static void payloadValidationAndIndependentCopies(void **state)
{
	uiprivDropData p = {0};
	char *copy;
	char **files;
	const char *bad[] = {"\xC0\x80", "\xED\xA0\x80", "\xF4\x90\x80\x80", "\xE2\x82", "\x80"};
	for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++)
		assert_false(uiprivDropSetText(&p, bad[i], strlen(bad[i])));
	assert_false(uiprivDropSetText(&p, "a\0b", 3));
	assert_false(uiprivDropSetText(&p, "x", SIZE_MAX));
	assert_true(uiprivDropSetText(&p, "", 0));
	assert_non_null(p.data.Text);
	copy = uiDropDataCopyText(&p.data);
	uiprivDropDataClear(&p);
	assert_string_equal(copy, "");
	uiFreeText(copy);
	assert_true(uiprivDropSetText(&p, "\xEF\xBF\xBD\xF0\x9F\x98\x80", 7));
	uiprivDropDataClear(&p);
	assert_false(uiprivDropAddFile(&p, "", 0));
	assert_false(uiprivDropAddFile(&p, "a\0b", 3));
	assert_true(uiprivDropAddFile(&p, "/tmp/\xFF", 6)); // Unix paths are bytes.
	assert_true(uiprivDropAddFile(&p, "/tmp/two", 8));
	files = uiDropDataCopyFiles(&p.data);
	uiprivDropDataClear(&p);
	assert_string_equal(files[0], "/tmp/\xFF");
	assert_string_equal(files[1], "/tmp/two");
	assert_null(files[2]);
	uiFreeTextArray(files);
	for (size_t i = 0; i < uiprivDropMaxFiles; i++)
		assert_true(uiprivDropAddFile(&p, "/x", 2));
	assert_null(p.data.Files[p.data.FileCount]);
	assert_false(uiprivDropAddFile(&p, "/x", 2));
	uiprivDropDataClear(&p);
	p.bytes = uiprivDropMaxBytes - 1;
	assert_false(uiprivDropAddFile(&p, "/x", 2));
	assert_int_equal(allocations, 0);
}

static uiControl *destroyDuringCancel;
static void cancelAndDestroyAnotherControl(uiprivDropEndpoint *e)
{
	uiControlDestroy(destroyDuringCancel);
	assert_true(uiprivControlDestroyPending(destroyDuringCancel));
}

static void nativeTeardownDefersReentrantDestruction(void **state)
{
	uiprivDropEndpoint *e = newEndpoint();
	uiprivDropEndpoint *other = newEndpoint();
	destroyDuringCancel = other->owner;
	e->cancel = cancelAndDestroyAnotherControl;
	uiControlDestroy(e->owner);
	assert_true(uiprivControlDestroyPending(destroyDuringCancel));
	uiprivControlDestroyFlushPending();
	destroyDuringCancel = NULL;
	assert_int_equal(allocations, 0);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(registrationFailureIsAtomic),
		cmocka_unit_test(copiedPartialHandler),
		cmocka_unit_test(staleTransfersAndReentrantReplacement),
		cmocka_unit_test(destroyDuringOverAndDrop),
		cmocka_unit_test(payloadValidationAndIndependentCopies),
		cmocka_unit_test(nativeTeardownDefersReentrantDestruction),
	};
	return cmocka_run_group_tests_name("drop common", tests, NULL, NULL);
}
