#include "area_test.h"
#import <AppKit/AppKit.h>

static NSView *attachArea(void **state)
{
	struct state *s = *state;
	uiWindowSetChild(s->w, s->c);
	NSView *view = (NSView *) uiControlHandle(s->c);
	[view setFrame:NSMakeRect(18, 22, 200, 150)];
	if ([view isKindOfClass:[NSScrollView class]]) {
		NSScrollView *sv = (NSScrollView *) view;
		[sv setHorizontalScrollElasticity:NSScrollElasticityNone];
		[sv setVerticalScrollElasticity:NSScrollElasticityNone];
		view = [sv documentView];
		[view scrollPoint:NSMakePoint(100, 200)];
	}
	handler.ScrollEvent = areaScroll;
	scrollCalls = 0;
	scrollHandled = 1;
	return view;
}

static NSEvent *wheelWithModifiers(int dx, int dy, BOOL precise, CGEventFlags flags)
{
	CGEventRef cg = CGEventCreateScrollWheelEvent(NULL,
		precise ? kCGScrollEventUnitPixel : kCGScrollEventUnitLine, 2, dy, dx);
	assert_non_null(cg);
	CGEventSetFlags(cg, flags);
	CGEventSetLocation(cg, CGPointMake(63, 81));
	NSEvent *event = [NSEvent eventWithCGEvent:cg];
	CFRelease(cg);
	assert_non_null(event);
	return event;
}

static NSEvent *wheel(int dx, int dy, BOOL precise)
{
	return wheelWithModifiers(dx, dy, precise, 0);
}

static void waitForScroll(NSScrollView *sv, CGFloat before)
{
	NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:2];
	CGFloat previous = before;
	int stable = 0;
	// Wait for the animation to settle so it cannot satisfy the next case.
	while ([deadline timeIntervalSinceNow] > 0) {
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
		CGFloat y = [[sv contentView] bounds].origin.y;
		stable = y > before && y == previous ? stable + 1 : 0;
		if (stable == 8)
			return;
		previous = y;
	}
	fail_msg("scroll did not settle past %g (at %g)", (double) before, (double) previous);
}

void areaDarwinScrollEvents(void **state)
{
	NSView *view = attachArea(state);
	NSEvent *event = wheel(2, -3, NO);
	NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
	[view scrollWheel:event];
	assert_int_equal(scrollCalls, 1);
	assert_true(lastScroll.DeltaX == -[event scrollingDeltaX]);
	assert_true(lastScroll.DeltaY == -[event scrollingDeltaY]);
	assert_true(lastScroll.DeltaX < 0 && lastScroll.DeltaY > 0);
	assert_true(lastScroll.X == point.x && lastScroll.Y == point.y);
	assert_int_equal(lastScroll.Modifiers, 0);
	event = wheel(-2, 3, YES);
	assert_true([event hasPreciseScrollingDeltas]);
	[view scrollWheel:event];
	assert_int_equal(scrollCalls, 2);
	assert_true(lastScroll.DeltaX == 2 && lastScroll.DeltaY == -3);
	[view scrollWheel:wheelWithModifiers(2, -3, NO,
		kCGEventFlagMaskShift | kCGEventFlagMaskControl |
		kCGEventFlagMaskAlternate | kCGEventFlagMaskCommand)];
	assert_int_equal(scrollCalls, 3);
	assert_int_equal(lastScroll.Modifiers,
		uiModifierShift | uiModifierCtrl | uiModifierAlt | uiModifierSuper);
	[view scrollWheel:wheel(0, 0, YES)];
	assert_int_equal(scrollCalls, 3);
	handler.ScrollEvent = NULL;
	[view scrollWheel:wheel(0, -1, NO)];
	assert_int_equal(scrollCalls, 3);
	handler.ScrollEvent = areaScroll;
	uiControlDisable(((struct state *) *state)->c);
	[view scrollWheel:event];
	assert_int_equal(scrollCalls, 3);
}

void scrollingAreaDarwinScrollEvents(void **state)
{
	NSView *view = attachArea(state);
	NSScrollView *sv = [view enclosingScrollView];
	NSEvent *event = wheel(0, -1, NO);
	NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
	NSPoint before = [[sv contentView] bounds].origin;
	assert_true(before.x == 100 && before.y == 200);
	[view scrollWheel:event];
	assert_int_equal(scrollCalls, 1);
	assert_true(lastScroll.X == point.x && lastScroll.Y == point.y);
	assert_true(NSEqualPoints(before, [[sv contentView] bounds].origin));
	// Returning zero must allow the enclosing native scroll view to scroll.
	scrollHandled = 0;
	[view scrollWheel:event];
	assert_int_equal(scrollCalls, 2);
	waitForScroll(sv, before.y);
	before = [[sv contentView] bounds].origin;
	handler.ScrollEvent = NULL;
	[view scrollWheel:wheel(0, -1, NO)];
	assert_int_equal(scrollCalls, 2);
	waitForScroll(sv, before.y);
	handler.ScrollEvent = areaScroll;
}

static int destroyOnScroll(uiAreaHandler *ah, uiArea *area, uiAreaScrollEvent *event)
{
	scrollCalls++;
	uiControlDestroy(uiControlParent(uiControl(area)));
	return 0;
}

void areaDarwinScrollDestroysWindow(void **state)
{
	struct state *s = *state;
	NSView *view = attachArea(state);
	NSScrollView *sv = [view enclosingScrollView];
	NSPoint before = [[sv contentView] bounds].origin;
	NSEvent *event = wheel(0, -1, NO);
	handler.ScrollEvent = destroyOnScroll;
	[view scrollWheel:event];
	assert_int_equal(scrollCalls, 1);
	assert_false(uiControlVisible(uiControl(s->w)));
	assert_true(NSEqualPoints(before, [[sv contentView] bounds].origin));
	// Further events before the deferred destroy must not enter user code.
	[view scrollWheel:event];
	assert_int_equal(scrollCalls, 1);
	handler.ScrollEvent = areaScroll;
}
