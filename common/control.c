// 26 may 2015
#include "../ui.h"
#include "uipriv.h"

typedef enum pendingDestroyType {
	pendingDestroyControl,
	pendingDestroyResource,
} pendingDestroyType;

typedef struct pendingDestroy pendingDestroy;
struct pendingDestroy {
	pendingDestroyType type;
	void *p;
	void (*free)(void *);
	pendingDestroy *next;
};

typedef struct controlDestroyedHandler controlDestroyedHandler;
struct controlDestroyedHandler {
	uiControl *c;
	void (*f)(uiControl *, void *);
	void *data;
	controlDestroyedHandler *next;
};

typedef struct freeingControl freeingControl;
struct freeingControl {
	uiControl *c;
	freeingControl *next;
};

static unsigned int userCallbackDepth;
static int flushingPendingControlDestroys;
static uintptr_t pendingControlDestroyFlushID;
static uintptr_t nextControlDestroyFlushID;
static pendingDestroy *pendingDestroys;
static pendingDestroy **pendingDestroysTail = &pendingDestroys;
static controlDestroyedHandler *controlDestroyedHandlers;
static freeingControl *freeingControls;

#ifdef _UI_STATIC
static void (*controlDestroyScheduleFuncForTests)(uintptr_t);

void uiprivControlDestroySetScheduleFuncForTests(void (*f)(uintptr_t))
{
	controlDestroyScheduleFuncForTests = f;
}
#endif

static pendingDestroy **findPendingControlDestroy(uiControl *c)
{
	pendingDestroy **p;

	for (p = &pendingDestroys; *p != NULL; p = &((*p)->next))
		if ((*p)->type == pendingDestroyControl && (*p)->p == c)
			return p;
	return NULL;
}

static void removePendingControlDestroy(uiControl *c)
{
	pendingDestroy **p;
	pendingDestroy *pending;

	p = findPendingControlDestroy(c);
	if (p == NULL)
		return;
	pending = *p;
	*p = pending->next;
	if (pending->next == NULL)
		pendingDestroysTail = p;
	uiprivFree(pending);
}

static controlDestroyedHandler **findControlDestroyedHandler(uiControl *c)
{
	controlDestroyedHandler **p;

	for (p = &controlDestroyedHandlers; *p != NULL; p = &((*p)->next))
		if ((*p)->c == c)
			return p;
	return NULL;
}

static controlDestroyedHandler *removeControlDestroyedHandler(uiControl *c)
{
	controlDestroyedHandler **p;
	controlDestroyedHandler *handler;

	p = findControlDestroyedHandler(c);
	if (p == NULL)
		return NULL;
	handler = *p;
	*p = handler->next;
	return handler;
}

static int controlIsBeingFreed(uiControl *c)
{
	freeingControl *freeing;

	for (freeing = freeingControls; freeing != NULL; freeing = freeing->next)
		if (freeing->c == c)
			return 1;
	return 0;
}

static void flushPendingControlDestroys(void)
{
	pendingDestroy *pending;

	if (flushingPendingControlDestroys)
		return;
	flushingPendingControlDestroys = 1;
	while (pendingDestroys != NULL) {
		pending = pendingDestroys;
		pendingDestroys = pending->next;
		if (pendingDestroys == NULL)
			pendingDestroysTail = &pendingDestroys;
		if (pending->type == pendingDestroyControl) {
			uiControl *c = pending->p;

			uiprivFree(pending);
			(*(c->Destroy))(c);
			continue;
		}
		void (*free)(void *) = pending->free;
		void *p = pending->p;

		uiprivFree(pending);
		(*free)(p);
	}
	flushingPendingControlDestroys = 0;
}

static void schedulePendingControlDestroys(void)
{
	if (pendingDestroys == NULL || flushingPendingControlDestroys ||
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

int uiprivUserCallbackEnter(uiControl *c)
{
	if (c != NULL && uiprivControlDestroyPending(c))
		return 0;
	userCallbackDepth++;
	return 1;
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
	if (pendingDestroys != NULL ||
		pendingDestroysTail != &pendingDestroys ||
		flushingPendingControlDestroys || pendingControlDestroyFlushID != 0)
		uiprivImplBug("uiUninit() reached with inconsistent deferred control destruction state");
	// Do not reset the ID counter: a backend flush queued before uiUninit()
	// must not match one scheduled after a later uiInit().
}

int uiprivControlDestroyPending(uiControl *c)
{
	for (; c != NULL; c = uiControlParent(c))
		if (findPendingControlDestroy(c) != NULL || controlIsBeingFreed(c))
			return 1;
	return 0;
}

int uiprivUserCallbackDeferFree(void *p, void (*free)(void *))
{
	pendingDestroy *pending;

	if (userCallbackDepth == 0)
		return 0;
	for (pending = pendingDestroys; pending != NULL; pending = pending->next)
		if (pending->type == pendingDestroyResource &&
			pending->p == p && pending->free == free)
			return 1;
	pending = uiprivNew(pendingDestroy);
	pending->type = pendingDestroyResource;
	pending->p = p;
	pending->free = free;
	*pendingDestroysTail = pending;
	pendingDestroysTail = &(pending->next);
	return 1;
}

void uiControlOnDestroyed(uiControl *c, void (*f)(uiControl *, void *), void *data)
{
	controlDestroyedHandler **p;
	controlDestroyedHandler *handler;

	if (c == NULL)
		uiprivUserBug("uiControlOnDestroyed() cannot be called with NULL");
	p = findControlDestroyedHandler(c);
	if (p != NULL) {
		handler = *p;
		if (f == NULL) {
			*p = handler->next;
			uiprivFree(handler);
			return;
		}
		handler->f = f;
		handler->data = data;
		return;
	}
	if (f == NULL)
		return;
	handler = uiprivNew(controlDestroyedHandler);
	handler->c = c;
	handler->f = f;
	handler->data = data;
	handler->next = controlDestroyedHandlers;
	controlDestroyedHandlers = handler;
}

void uiControlDestroy(uiControl *c)
{
	if (c == NULL)
		uiprivUserBug("uiControlDestroy() cannot be called with NULL");
	if (userCallbackDepth != 0) {
		pendingDestroy *pending;

		if (uiprivControlDestroyPending(c))
			return;
		pending = uiprivNew(pendingDestroy);
		pending->type = pendingDestroyControl;
		pending->p = c;
		pending->free = NULL;
		*pendingDestroysTail = pending;
		pendingDestroysTail = &(pending->next);
		(*(c->Hide))(c);
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
	controlDestroyedHandler *handler;
	freeingControl freeing;
	void (*f)(uiControl *, void *);
	void *data;

	if (uiControlParent(c) != NULL)
		uiprivUserBug("You cannot destroy a uiControl while it still has a parent. (control: %p)", c);
	// A parent destroy can synchronously destroy a child that was also queued.
	// Remove that stale queue entry before releasing the child's storage.
	removePendingControlDestroy(c);
	handler = removeControlDestroyedHandler(c);
	if (handler != NULL) {
		f = handler->f;
		data = handler->data;
		uiprivFree(handler);
		freeing.c = c;
		freeing.next = freeingControls;
		freeingControls = &freeing;
		uiprivUserCallbackEnter(NULL);
		(*f)(c, data);
		uiprivUserCallbackLeave();
		freeingControls = freeing.next;
		// A destroyed handler may try to register another handler for the
		// control that is about to be freed. Such a handler can never run;
		// remove it so it does not retain a dangling control pointer.
		handler = removeControlDestroyedHandler(c);
		if (handler != NULL)
			uiprivFree(handler);
	}
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
