// 26 may 2015
#include "../ui.h"
#include "uipriv.h"

typedef struct pendingControlDestroy pendingControlDestroy;
struct pendingControlDestroy {
	uiControl *c;
	pendingControlDestroy *next;
};

static unsigned int userCallbackDepth;
static int flushingPendingControlDestroys;
static uintptr_t pendingControlDestroyFlushID;
static uintptr_t nextControlDestroyFlushID;
static pendingControlDestroy *pendingControlDestroys;
static pendingControlDestroy **pendingControlDestroysTail = &pendingControlDestroys;

#ifdef _UI_STATIC
static void (*controlDestroyScheduleFuncForTests)(uintptr_t);

void uiprivControlDestroySetScheduleFuncForTests(void (*f)(uintptr_t))
{
	controlDestroyScheduleFuncForTests = f;
}
#endif

static pendingControlDestroy **findPendingControlDestroy(uiControl *c)
{
	pendingControlDestroy **p;

	for (p = &pendingControlDestroys; *p != NULL; p = &((*p)->next))
		if ((*p)->c == c)
			return p;
	return NULL;
}

static void removePendingControlDestroy(uiControl *c)
{
	pendingControlDestroy **p;
	pendingControlDestroy *pending;

	p = findPendingControlDestroy(c);
	if (p == NULL)
		return;
	pending = *p;
	*p = pending->next;
	if (pending->next == NULL)
		pendingControlDestroysTail = p;
	uiprivFree(pending);
}

static void flushPendingControlDestroys(void)
{
	pendingControlDestroy *pending;
	uiControl *c;

	if (flushingPendingControlDestroys)
		return;
	flushingPendingControlDestroys = 1;
	while (pendingControlDestroys != NULL) {
		pending = pendingControlDestroys;
		pendingControlDestroys = pending->next;
		if (pendingControlDestroys == NULL)
			pendingControlDestroysTail = &pendingControlDestroys;
		c = pending->c;
		uiprivFree(pending);
		(*(c->Destroy))(c);
	}
	flushingPendingControlDestroys = 0;
}

static void schedulePendingControlDestroys(void)
{
	if (pendingControlDestroys == NULL || flushingPendingControlDestroys ||
		pendingControlDestroyFlushID != 0)
		return;
	nextControlDestroyFlushID++;
	if (nextControlDestroyFlushID == 0)
		nextControlDestroyFlushID++;
	pendingControlDestroyFlushID = nextControlDestroyFlushID;
#ifdef _UI_STATIC
	if (controlDestroyScheduleFuncForTests != NULL) {
		controlDestroyScheduleFuncForTests(pendingControlDestroyFlushID);
		return;
	}
#endif
	uiprivScheduleControlDestroyFlush(pendingControlDestroyFlushID);
}

void uiprivUserCallbackEnter(void)
{
	userCallbackDepth++;
}

void uiprivUserCallbackLeave(void)
{
	if (userCallbackDepth == 0)
		uiprivImplBug("attempt to leave a user callback while not in one");
	userCallbackDepth--;
	if (userCallbackDepth == 0)
		schedulePendingControlDestroys();
}

void uiprivControlDestroyFlush(uintptr_t id)
{
	if (id != pendingControlDestroyFlushID)
		return;
	pendingControlDestroyFlushID = 0;
	if (userCallbackDepth != 0)
		// The outermost Leave() will schedule another flush.
		return;
	flushPendingControlDestroys();
}

void uiprivControlDestroyFlushPending(void)
{
	if (userCallbackDepth != 0)
		uiprivImplBug("attempt to flush control destroys from a user callback");
	// Invalidate an already queued backend flush before flushing synchronously.
	pendingControlDestroyFlushID = 0;
	flushPendingControlDestroys();
}

void uiprivControlDestroyUninit(void)
{
	if (userCallbackDepth != 0)
		uiprivImplBug("uiUninit() reached with user callback depth %u", userCallbackDepth);
	uiprivControlDestroyFlushPending();
	if (pendingControlDestroys != NULL ||
		pendingControlDestroysTail != &pendingControlDestroys ||
		flushingPendingControlDestroys || pendingControlDestroyFlushID != 0)
		uiprivImplBug("uiUninit() reached with inconsistent deferred control destruction state");
	// Do not reset the ID counter: a backend flush queued before uiUninit()
	// must not match one scheduled after a later uiInit().
}

int uiprivControlDestroyPending(uiControl *c)
{
	if (pendingControlDestroys == NULL)
		return 0;
	for (; c != NULL; c = uiControlParent(c))
		if (findPendingControlDestroy(c) != NULL)
			return 1;
	return 0;
}

void uiControlDestroy(uiControl *c)
{
	if (c == NULL)
		uiprivUserBug("uiControlDestroy() cannot be called with NULL");
	if (userCallbackDepth != 0) {
		pendingControlDestroy *pending;

		if (uiprivControlDestroyPending(c))
			return;
		pending = uiprivNew(pendingControlDestroy);
		pending->c = c;
		*pendingControlDestroysTail = pending;
		pendingControlDestroysTail = &(pending->next);
		return;
	}
	(*(c->Destroy))(c);
}

uintptr_t uiControlHandle(uiControl *c)
{
	return (*(c->Handle))(c);
}

uiControl *uiControlParent(uiControl *c)
{
	return (*(c->Parent))(c);
}

void uiControlSetParent(uiControl *c, uiControl *parent)
{
	(*(c->SetParent))(c, parent);
}

int uiControlToplevel(uiControl *c)
{
	return (*(c->Toplevel))(c);
}

int uiControlVisible(uiControl *c)
{
	return (*(c->Visible))(c);
}

void uiControlShow(uiControl *c)
{
	(*(c->Show))(c);
}

void uiControlHide(uiControl *c)
{
	(*(c->Hide))(c);
}

int uiControlEnabled(uiControl *c)
{
	return (*(c->Enabled))(c);
}

void uiControlEnable(uiControl *c)
{
	(*(c->Enable))(c);
}

void uiControlDisable(uiControl *c)
{
	(*(c->Disable))(c);
}

#define uiprivControlSignature 0x7569436F

uiControl *uiAllocControl(size_t size, uint32_t OSsig, uint32_t typesig, const char *typenamestr)
{
	uiControl *c;

	c = (uiControl *) uiprivAlloc(size, typenamestr);
	c->Signature = uiprivControlSignature;
	c->OSSignature = OSsig;
	c->TypeSignature = typesig;
	return c;
}

void uiFreeControl(uiControl *c)
{
	if (uiControlParent(c) != NULL)
		uiprivUserBug("You cannot destroy a uiControl while it still has a parent. (control: %p)", c);
	// A parent destroy can synchronously destroy a child that was also queued.
	// Remove that stale queue entry before releasing the child's storage.
	removePendingControlDestroy(c);
	uiprivFree(c);
}

void uiControlVerifySetParent(uiControl *c, uiControl *parent)
{
	uiControl *curParent;

	if (uiControlToplevel(c))
		uiprivUserBug("You cannot give a toplevel uiControl a parent. (control: %p)", c);
	curParent = uiControlParent(c);
	if (parent != NULL && curParent != NULL)
		uiprivUserBug("You cannot give a uiControl a parent while it already has one. (control: %p; current parent: %p; new parent: %p)", c, curParent, parent);
	if (parent == NULL && curParent == NULL)
		uiprivImplBug("attempt to double unparent uiControl %p", c);
}

int uiControlEnabledToUser(uiControl *c)
{
	while (c != NULL) {
		if (!uiControlEnabled(c))
			return 0;
		c = uiControlParent(c);
	}
	return 1;
}
