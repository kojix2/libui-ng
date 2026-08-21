#include "unit.h"
#include "../../common/uipriv.h"

typedef struct testControl testControl;
struct testControl {
	uiControl c;
	uiControl *parent;
	testControl *child;
	testControl *destroyFromCallback;
	int *destroyCount;
	int hideCount;
};

static int scheduleCount;
static uintptr_t scheduledID;

static void countDestroyed(uiControl *c, void *data)
{
	int *count = data;

	(*count)++;
}

static void destroyAgainWhenDestroyed(uiControl *c, void *data)
{
	int *count = data;

	(*count)++;
	uiControlDestroy(c);
}

static void captureSchedule(uintptr_t id)
{
	scheduleCount++;
	scheduledID = id;
}

static void testControlDestroy(uiControl *c)
{
	testControl *tc = (testControl *) c;

	if (tc->child != NULL) {
		tc->child->parent = NULL;
		uiControlDestroy(uiControl(tc->child));
	}
	if (tc->destroyFromCallback != NULL) {
		uiprivUserCallbackEnter(NULL);
		uiControlDestroy(uiControl(tc->destroyFromCallback));
		uiprivUserCallbackLeave();
	}
	(*tc->destroyCount)++;
	uiFreeControl(c);
}

static uintptr_t testControlHandle(uiControl *c)
{
	return 0;
}

static uiControl *testControlParent(uiControl *c)
{
	return ((testControl *) c)->parent;
}

static void testControlSetParent(uiControl *c, uiControl *parent)
{
	((testControl *) c)->parent = parent;
}

static int testControlFalse(uiControl *c)
{
	return 0;
}

static void testControlVoid(uiControl *c)
{
}

static void testControlHide(uiControl *c)
{
	((testControl *) c)->hideCount++;
}

static testControl *newTestControl(int *destroyCount)
{
	testControl *tc;

	tc = (testControl *) uiAllocControl(sizeof (testControl), 0, 0, "testControl");
	tc->c.Destroy = testControlDestroy;
	tc->c.Handle = testControlHandle;
	tc->c.Parent = testControlParent;
	tc->c.SetParent = testControlSetParent;
	tc->c.Toplevel = testControlFalse;
	tc->c.Visible = testControlFalse;
	tc->c.Show = testControlVoid;
	tc->c.Hide = testControlHide;
	tc->c.Enabled = testControlFalse;
	tc->c.Enable = testControlVoid;
	tc->c.Disable = testControlVoid;
	tc->destroyCount = destroyCount;
	return tc;
}

static void destroyIsDeferred(void **state)
{
	int destroyCount = 0;
	testControl *tc = newTestControl(&destroyCount);

	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(tc));
	assert_true(uiprivControlDestroyPending(uiControl(tc)));
	assert_int_equal(destroyCount, 0);
	uiprivUserCallbackLeave();
	assert_int_equal(destroyCount, 0);
	uiprivControlDestroyFlushPending();
	assert_int_equal(destroyCount, 1);
}

static void nestedCallbacksDeferUntilOutermostLeave(void **state)
{
	int destroyCount = 0;
	testControl *tc = newTestControl(&destroyCount);

	uiprivUserCallbackEnter(NULL);
	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(tc));
	uiprivUserCallbackLeave();
	assert_int_equal(destroyCount, 0);
	uiprivUserCallbackLeave();
	assert_int_equal(destroyCount, 0);
	uiprivControlDestroyFlushPending();
	assert_int_equal(destroyCount, 1);
}

static void duplicateDestroyIsCoalesced(void **state)
{
	int destroyCount = 0;
	testControl *tc = newTestControl(&destroyCount);

	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(tc));
	uiControlDestroy(uiControl(tc));
	uiprivUserCallbackLeave();
	assert_int_equal(destroyCount, 0);
	uiprivControlDestroyFlushPending();
	assert_int_equal(destroyCount, 1);
}

static void parentDestroyCancelsQueuedChild(void **state)
{
	int parentDestroyCount = 0;
	int childDestroyCount = 0;
	testControl *parent = newTestControl(&parentDestroyCount);
	testControl *child = newTestControl(&childDestroyCount);

	parent->child = child;
	child->parent = uiControl(parent);
	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(parent));
	uiControlDestroy(uiControl(child));
	uiprivUserCallbackLeave();
	assert_int_equal(parentDestroyCount, 0);
	assert_int_equal(childDestroyCount, 0);
	uiprivControlDestroyFlushPending();
	assert_int_equal(parentDestroyCount, 1);
	assert_int_equal(childDestroyCount, 1);
}

static void pendingAncestorMakesChildPending(void **state)
{
	int parentDestroyCount = 0;
	int childDestroyCount = 0;
	testControl *parent = newTestControl(&parentDestroyCount);
	testControl *child = newTestControl(&childDestroyCount);

	parent->child = child;
	child->parent = uiControl(parent);
	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(parent));
	assert_true(uiprivControlDestroyPending(uiControl(parent)));
	assert_true(uiprivControlDestroyPending(uiControl(child)));
	uiprivUserCallbackLeave();
	uiprivControlDestroyFlushPending();
	assert_int_equal(parentDestroyCount, 1);
	assert_int_equal(childDestroyCount, 1);
}

static void callbackDuringFlushAppendsDestroy(void **state)
{
	int firstDestroyCount = 0;
	int secondDestroyCount = 0;
	testControl *first = newTestControl(&firstDestroyCount);
	testControl *second = newTestControl(&secondDestroyCount);

	first->destroyFromCallback = second;
	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(first));
	uiprivUserCallbackLeave();
	assert_int_equal(firstDestroyCount, 0);
	assert_int_equal(secondDestroyCount, 0);
	uiprivControlDestroyFlushPending();
	assert_int_equal(firstDestroyCount, 1);
	assert_int_equal(secondDestroyCount, 1);
}

static void scheduledFlushChecksIDAndWaitsForOutermostLeave(void **state)
{
	int destroyCount = 0;
	testControl *tc = newTestControl(&destroyCount);
	uintptr_t firstID;

	scheduleCount = 0;
	scheduledID = 0;
	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(tc));
	uiprivUserCallbackLeave();
	assert_int_equal(scheduleCount, 1);
	assert_true(scheduledID != 0);
	firstID = scheduledID;

	// A stale backend task must not consume the pending destroy.
	uiprivControlDestroyFlush(firstID + 1);
	assert_int_equal(destroyCount, 0);
	assert_int_equal(scheduleCount, 1);

	// A matching task inside a nested loop must neither destroy nor busy-spin.
	uiprivUserCallbackEnter(NULL);
	uiprivControlDestroyFlush(firstID);
	assert_int_equal(destroyCount, 0);
	assert_int_equal(scheduleCount, 1);
	uiprivUserCallbackLeave();
	assert_int_equal(scheduleCount, 2);
	assert_true(scheduledID != firstID);

	uiprivControlDestroyFlush(scheduledID);
	assert_int_equal(destroyCount, 1);
}

static void destroyedHandlerRunsAtFree(void **state)
{
	int destroyCount = 0;
	int destroyedHandlerCount = 0;
	testControl *tc = newTestControl(&destroyCount);

	uiControlOnDestroyed(uiControl(tc), countDestroyed, &destroyedHandlerCount);
	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(tc));
	assert_int_equal(destroyedHandlerCount, 0);
	uiprivUserCallbackLeave();
	assert_int_equal(destroyedHandlerCount, 0);
	uiprivControlDestroyFlushPending();
	assert_int_equal(destroyedHandlerCount, 1);
}

static void parentDestroyNotifiesForChild(void **state)
{
	int parentDestroyCount = 0;
	int childDestroyCount = 0;
	int childDestroyedHandlerCount = 0;
	testControl *parent = newTestControl(&parentDestroyCount);
	testControl *child = newTestControl(&childDestroyCount);

	parent->child = child;
	child->parent = uiControl(parent);
	uiControlOnDestroyed(uiControl(child), countDestroyed,
		&childDestroyedHandlerCount);
	uiControlDestroy(uiControl(parent));
	assert_int_equal(parentDestroyCount, 1);
	assert_int_equal(childDestroyCount, 1);
	assert_int_equal(childDestroyedHandlerCount, 1);
}

static void destroyedHandlerCanBeReplacedAndRemoved(void **state)
{
	int destroyCount = 0;
	int firstCount = 0;
	int secondCount = 0;
	testControl *tc = newTestControl(&destroyCount);

	uiControlOnDestroyed(uiControl(tc), countDestroyed, &firstCount);
	uiControlOnDestroyed(uiControl(tc), countDestroyed, &secondCount);
	uiControlOnDestroyed(uiControl(tc), NULL, NULL);
	uiControlDestroy(uiControl(tc));
	assert_int_equal(firstCount, 0);
	assert_int_equal(secondCount, 0);
}

static void destroyedHandlerCannotQueueSelfAgain(void **state)
{
	int destroyCount = 0;
	int destroyedHandlerCount = 0;
	testControl *tc = newTestControl(&destroyCount);

	uiControlOnDestroyed(uiControl(tc), destroyAgainWhenDestroyed,
		&destroyedHandlerCount);
	uiControlDestroy(uiControl(tc));
	assert_int_equal(destroyCount, 1);
	assert_int_equal(destroyedHandlerCount, 1);
	uiprivControlDestroyFlushPending();
	assert_int_equal(destroyCount, 1);
}

static void pendingControlIsHiddenAndSuppressesCallbacks(void **state)
{
	int destroyCount = 0;
	testControl *tc = newTestControl(&destroyCount);

	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(tc));
	assert_int_equal(tc->hideCount, 1);
	assert_false(uiprivUserCallbackEnter(uiControl(tc)));
	uiprivUserCallbackLeave();
	uiprivControlDestroyFlushPending();
	assert_int_equal(destroyCount, 1);
}

static void pendingAncestorSuppressesChildCallbacks(void **state)
{
	int parentDestroyCount = 0;
	int childDestroyCount = 0;
	testControl *parent = newTestControl(&parentDestroyCount);
	testControl *child = newTestControl(&childDestroyCount);

	parent->child = child;
	child->parent = uiControl(parent);
	uiprivUserCallbackEnter(NULL);
	uiControlDestroy(uiControl(parent));
	assert_false(uiprivUserCallbackEnter(uiControl(child)));
	uiprivUserCallbackLeave();
	uiprivControlDestroyFlushPending();
	assert_int_equal(parentDestroyCount, 1);
	assert_int_equal(childDestroyCount, 1);
}
static int controlSetup(void **state)
{
	uiInitOptions o = {0};

	assert_null(uiInit(&o));
	uiprivControlDestroySetScheduleFuncForTests(captureSchedule);
	scheduleCount = 0;
	scheduledID = 0;
	return 0;
}

static int controlTeardown(void **state)
{
	uiprivControlDestroyFlushPending();
	uiprivControlDestroySetScheduleFuncForTests(NULL);
	uiUninit();
	return 0;
}

int controlRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(destroyIsDeferred),
		cmocka_unit_test(nestedCallbacksDeferUntilOutermostLeave),
		cmocka_unit_test(duplicateDestroyIsCoalesced),
		cmocka_unit_test(parentDestroyCancelsQueuedChild),
		cmocka_unit_test(pendingAncestorMakesChildPending),
		cmocka_unit_test(callbackDuringFlushAppendsDestroy),
		cmocka_unit_test(scheduledFlushChecksIDAndWaitsForOutermostLeave),
		cmocka_unit_test(destroyedHandlerRunsAtFree),
		cmocka_unit_test(parentDestroyNotifiesForChild),
		cmocka_unit_test(destroyedHandlerCanBeReplacedAndRemoved),
		cmocka_unit_test(destroyedHandlerCannotQueueSelfAgain),
		cmocka_unit_test(pendingControlIsHiddenAndSuppressesCallbacks),
		cmocka_unit_test(pendingAncestorSuppressesChildCallbacks),
	};

	return cmocka_run_group_tests_name("uiControl deferred destruction", tests,
		controlSetup, controlTeardown);
}
