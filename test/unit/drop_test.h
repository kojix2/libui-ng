#ifndef LIBUI_DROP_TEST_SUPPORT_H
#define LIBUI_DROP_TEST_SUPPORT_H
#include "unit.h"
#ifndef LIBUI_DROP_TEST_NATIVE_HEADERS
#include "../../common/uipriv.h"
#endif
#include "../../common/drop.h"

// Run the real common registration and deferred-destruction code without a
// display server. Native adapters are exercised in separate platform tests.
static size_t allocations;
void *uiprivAlloc(size_t size, const char *type)
{
	void *p = calloc(1, size);
	assert_non_null(p);
	allocations++;
	return p;
}
void *uiprivRealloc(void *p, size_t size, const char *type)
{
	if (p == NULL) allocations++;
	p = realloc(p, size);
	assert_non_null(p);
	return p;
}
void uiprivFree(void *p)
{
	assert_non_null(p);
	assert_true(allocations > 0);
	allocations--;
	free(p);
}
void uiFreeText(char *s)
{
#ifdef _WIN32
	uiprivFree(s);
#else
	free(s);
#endif
}
void uiprivControlMinimumSizeRemove(uiControl *c) { (void) c; }
void uiprivScheduleControlDestroyFlush(uintptr_t id) { (void) id; }
void uiprivDoImplBug(const char *f, const char *l, const char *fn, const char *fmt, ...)
{
	fail_msg("libui implementation bug: %s", fmt);
	abort();
}
void uiprivDoUserBug(const char *f, const char *l, const char *fn, const char *fmt, ...)
{
	fail_msg("libui user bug: %s", fmt);
	abort();
}

typedef struct testControl {
	uiControl c;
	uiControl *parent;
	struct testControl *child;
	uiprivDropEndpoint *endpoint;
} testControl;

static inline uiControl *parent(uiControl *c) { return ((testControl *) c)->parent; }
static inline int enabled(uiControl *c) { return 1; }
static inline void hide(uiControl *c) { (void) c; }
static inline void destroy(uiControl *c)
{
	testControl *t = (testControl *) c;
	if (t->child != NULL) {
		t->child->parent = NULL;
		uiControlDestroy(uiControl(t->child));
	}
	uiprivDropDestroy(t->endpoint);
	uiFreeControl(c);
}
static inline uiprivDropEndpoint *newEndpoint(void)
{
	testControl *c = (testControl *) uiAllocControl(sizeof(testControl), 0, 0, "test control");
	c->c.Parent = parent;
	c->c.Enabled = enabled;
	c->c.Hide = hide;
	c->c.Destroy = destroy;
	c->endpoint = uiprivDropNew(uiControl(c));
	return c->endpoint;
}
static inline void destroyEndpoint(uiprivDropEndpoint *e)
{
	uiControlDestroy(e->owner);
	uiprivControlDestroyFlushPending();
	assert_int_equal(allocations, 0);
}

#endif
