#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#import <AppKit/AppKit.h>
#include "../../ui.h"

@interface NSWindow (DropTestDeclarations)
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender;
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender;
- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender;
@end

// Exercise real AppKit pasteboards and libui destination methods. This does
// not simulate pointer routing or claim to cover cross-process providers.
@interface DropOffer : NSObject {
@public
	NSPasteboard *pasteboard;
	NSWindow *window;
	NSPoint point;
	NSDragOperation operation;
}
@end
@implementation DropOffer
- (NSPasteboard *)draggingPasteboard { return pasteboard; }
- (NSWindow *)draggingDestinationWindow { return window; }
- (NSPoint)draggingLocation { return point; }
- (NSDragOperation)draggingSourceOperationMask { return operation; }
- (NSInteger)draggingSequenceNumber { return 17; }
@end

typedef struct testContext {
	uiWindow *window;
	int drops, overs, leaves, releases;
	int mode;
	unsigned int offered;
	uiDropFormat format;
	double x, y;
	char *text;
	char **files;
} testContext;

@interface DropProvider : NSObject<NSPasteboardItemDataProvider> {
@public
	testContext *context;
}
@end
@implementation DropProvider
- (void)pasteboard:(NSPasteboard *)pasteboard item:(NSPasteboardItem *)item
	provideDataForType:(NSPasteboardType)type
{
	if (context->mode == 1) {
		assert_int_equal(uiWindowSetDropHandler(context->window, 0, NULL, NULL), uiDropStatusOK);
		assert_int_equal(context->releases, 1);
	} else {
		uiControlDestroy(uiControl(context->window));
		// The complete native receive operation must defer destruction, even
		// though this is a pasteboard provider rather than a libui callback.
		assert_int_equal(context->releases, 0);
	}
	[item setString:@"late provider text" forType:type];
}
@end

static void released(void *data) { ((testContext *) data)->releases++; }
static void left(void *data) { ((testContext *) data)->leaves++; }
static uiDropResult dropped(const uiDropInfo *info, const uiDropData *payload, void *data)
{
	testContext *c = data;
	c->drops++;
	c->offered = uiDropInfoFormats(info);
	c->format = uiDropDataFormat(payload);
	uiDropInfoPosition(info, &c->x, &c->y);
	if (c->format == uiDropFormatText)
		c->text = uiDropDataCopyText(payload);
	else
		c->files = uiDropDataCopyFiles(payload);
	if (c->mode == 1) {
		assert_int_equal(uiWindowSetDropHandler(c->window, 0, NULL, NULL), uiDropStatusOK);
		assert_int_equal(c->releases, 0);
	} else if (c->mode == 2)
		uiControlDestroy(uiControl(c->window));
	return uiDropAccepted;
}
static uiDropAction over(const uiDropInfo *info, void *data)
{
	testContext *c = data;
	c->overs++;
	if (c->mode == 3) {
		uiControlDestroy(uiControl(c->window));
		return uiDropActionCopy;
	}
	return uiDropActionNone;
}
static uiDropHandler handler(void)
{
	uiDropHandler h = {sizeof h, dropped, NULL, left, released};
	return h;
}

static DropOffer *offer(uiWindow *w, NSArray *objects)
{
	DropOffer *o = [DropOffer new];
	o->window = (NSWindow *) uiControlHandle(uiControl(w));
	o->pasteboard = [[NSPasteboard pasteboardWithUniqueName] retain];
	assert_true([o->pasteboard writeObjects:objects]);
	o->point = NSMakePoint(23, 41);
	o->operation = NSDragOperationCopy;
	return o;
}
static void freeOffer(DropOffer *o)
{
	[o->pasteboard releaseGlobally];
	[o->pasteboard release];
	[o release];
}
static int setup(void **state)
{
	uiInitOptions options = {0};
	testContext *c = calloc(1, sizeof *c);
	assert_null(uiInit(&options));
	c->window = uiNewWindow("Drop test", 320, 240, 0);
	*state = c;
	return 0;
}
static int teardown(void **state)
{
	testContext *c = *state;
	if (c->text != NULL) uiFreeText(c->text);
	if (c->files != NULL) uiFreeTextArray(c->files);
	if (c->mode != 2 && c->mode != 3)
		uiControlDestroy(uiControl(c->window));
	uiUninit(); // Also flushes deferred self-destruction.
	assert_int_equal(c->releases, 1);
	free(c);
	return 0;
}

static void textAndWindowCoordinates(void **state)
{
	testContext *c = *state;
	uiDropHandler h = handler();
	DropOffer *o = offer(c->window, @[@"hello 日本語"]);
	assert_int_equal(uiWindowSetDropHandler(c->window, uiDropFormatText, &h, c), uiDropStatusOK);
	memset(&h, 0, sizeof h); // Native registration must retain its own copy.
	assert_int_equal([o->window draggingEntered:(id<NSDraggingInfo>) o], NSDragOperationCopy);
	assert_true([o->window prepareForDragOperation:(id<NSDraggingInfo>) o]);
	assert_true([o->window performDragOperation:(id<NSDraggingInfo>) o]);
	assert_int_equal(c->drops, 1);
	assert_int_equal(c->leaves, 1);
	assert_string_equal(c->text, "hello 日本語");
	NSView *cv = [o->window contentView];
	NSPoint p = [cv convertPoint:o->point fromView:nil];
	assert_true(c->x == p.x);
	assert_true(c->y == ([cv isFlipped] ? p.y : NSMaxY([cv bounds]) - p.y));
	assert_false([o->window performDragOperation:(id<NSDraggingInfo>) o]);
	assert_int_equal(uiWindowSetDropHandler(c->window, 0, NULL, NULL), uiDropStatusOK);
	assert_int_equal(uiWindowSetDropHandler(c->window, 0, NULL, NULL), uiDropStatusOK);
	freeOffer(o);
}

static void filesPreferredAndCopied(void **state)
{
	testContext *c = *state;
	uiDropHandler h = handler();
	DropOffer *o = offer(c->window, @[[NSURL fileURLWithPath:@"/tmp/日本語.txt"],
		[NSURL fileURLWithPath:@"/tmp/two"], @"hello"]);
	assert_int_equal(uiWindowSetDropHandler(c->window, uiDropFormatFiles | uiDropFormatText, &h, c), uiDropStatusOK);
	assert_int_equal([o->window draggingEntered:(id<NSDraggingInfo>) o], NSDragOperationCopy);
	assert_true([o->window performDragOperation:(id<NSDraggingInfo>) o]);
	assert_int_equal(c->format, uiDropFormatFiles);
	assert_string_equal(c->files[0], "/tmp/日本語.txt");
	assert_string_equal(c->files[1], "/tmp/two");
	assert_null(c->files[2]);
	freeOffer(o);
}

static void mixedURLListRejected(void **state)
{
	testContext *c = *state;
	uiDropHandler h = handler();
	DropOffer *o = offer(c->window, @[[NSURL fileURLWithPath:@"/tmp/one"],
		[NSURL URLWithString:@"https://example.invalid/file"]]);
	assert_int_equal(uiWindowSetDropHandler(c->window, uiDropFormatFiles | uiDropFormatText, &h, c), uiDropStatusOK);
	assert_int_equal([o->window draggingEntered:(id<NSDraggingInfo>) o], NSDragOperationCopy);
	assert_false([o->window performDragOperation:(id<NSDraggingInfo>) o]);
	assert_int_equal(c->drops, 0);
	freeOffer(o);
}

static void remoteFileURLRejected(void **state)
{
	testContext *c = *state;
	uiDropHandler h = handler();
	DropOffer *o = offer(c->window, @[[NSURL fileURLWithPath:@"/tmp/local"],
		[NSURL URLWithString:@"file://remote.invalid/share/file"]]);
	assert_int_equal(uiWindowSetDropHandler(c->window, uiDropFormatFiles, &h, c), uiDropStatusOK);
	assert_int_equal([o->window draggingEntered:(id<NSDraggingInfo>) o], NSDragOperationCopy);
	assert_false([o->window performDragOperation:(id<NSDraggingInfo>) o]);
	assert_int_equal(c->drops, 0);
	freeOffer(o);
}

static void sourceAndApplicationRejection(void **state)
{
	testContext *c = *state;
	uiDropHandler h = handler();
	DropOffer *o = offer(c->window, @[@"hello"]);
	h.Over = over;
	assert_int_equal(uiWindowSetDropHandler(c->window, uiDropFormatText, &h, c), uiDropStatusOK);
	o->operation = NSDragOperationMove;
	assert_int_equal([o->window draggingEntered:(id<NSDraggingInfo>) o], NSDragOperationNone);
	assert_int_equal(c->overs, 0);
	o->operation = NSDragOperationCopy;
	assert_int_equal([o->window draggingUpdated:(id<NSDraggingInfo>) o], NSDragOperationNone);
	assert_int_equal(c->overs, 1);
	assert_false([o->window performDragOperation:(id<NSDraggingInfo>) o]);
	assert_int_equal(c->drops, 0);
	freeOffer(o);
}

static void selfRemovalCanAccept(void **state)
{
	testContext *c = *state;
	uiDropHandler h = handler();
	DropOffer *o = offer(c->window, @[@"hello"]);
	c->mode = 1;
	assert_int_equal(uiWindowSetDropHandler(c->window, uiDropFormatText, &h, c), uiDropStatusOK);
	assert_int_equal([o->window draggingEntered:(id<NSDraggingInfo>) o], NSDragOperationCopy);
	assert_true([o->window performDragOperation:(id<NSDraggingInfo>) o]);
	assert_int_equal(c->releases, 1);
	freeOffer(o);
}

static void selfDestructionCanAccept(void **state)
{
	testContext *c = *state;
	uiDropHandler h = handler();
	DropOffer *o = offer(c->window, @[@"hello"]);
	c->mode = 2;
	assert_int_equal(uiWindowSetDropHandler(c->window, uiDropFormatText, &h, c), uiDropStatusOK);
	assert_int_equal([o->window draggingEntered:(id<NSDraggingInfo>) o], NSDragOperationCopy);
	assert_true([o->window performDragOperation:(id<NSDraggingInfo>) o]);
	assert_int_equal(c->drops, 1);
	freeOffer(o);
}

static void destructionDuringOverRejects(void **state)
{
	testContext *c = *state;
	uiDropHandler h = handler();
	DropOffer *o = offer(c->window, @[@"hello"]);
	c->mode = 3;
	h.Over = over;
	assert_int_equal(uiWindowSetDropHandler(c->window, uiDropFormatText, &h, c), uiDropStatusOK);
	assert_int_equal([o->window draggingEntered:(id<NSDraggingInfo>) o], NSDragOperationNone);
	assert_false([o->window performDragOperation:(id<NSDraggingInfo>) o]);
	freeOffer(o);
}

static void scrollingAreaCoordinates(void **state)
{
	testContext *c = *state;
	uiDropHandler h = handler();
	uiAreaHandler ah = {0};
	uiArea *area = uiNewScrollingArea(&ah, 900, 800);
	uiWindowSetChild(c->window, uiControl(area));
	NSScrollView *sv = (NSScrollView *) uiControlHandle(uiControl(area));
	NSView *view = [sv documentView];
	DropOffer *o = offer(c->window, @[@"hello"]);
	[sv setFrame:NSMakeRect(18, 22, 200, 150)];
	[view scrollPoint:NSMakePoint(100, 200)];
	o->point = [view convertPoint:NSMakePoint(125, 235) toView:nil];
	assert_int_equal(uiAreaSetDropHandler(area, uiDropFormatText, &h, c), uiDropStatusOK);
	assert_int_equal([view draggingEntered:(id<NSDraggingInfo>) o], NSDragOperationCopy);
	assert_true([view performDragOperation:(id<NSDraggingInfo>) o]);
	assert_true(c->x == 125 && c->y == 235);
	freeOffer(o);
}

static void reentrantProvider(testContext *c, int mode)
{
	uiDropHandler h = handler();
	DropProvider *provider = [DropProvider new];
	NSPasteboardItem *item = [NSPasteboardItem new];
	provider->context = c;
	c->mode = mode;
	[item setDataProvider:provider forTypes:@[NSPasteboardTypeString]];
	DropOffer *o = offer(c->window, @[item]);
	assert_int_equal(uiWindowSetDropHandler(c->window, uiDropFormatText, &h, c), uiDropStatusOK);
	assert_int_equal([o->window draggingEntered:(id<NSDraggingInfo>) o], NSDragOperationCopy);
	assert_false([o->window performDragOperation:(id<NSDraggingInfo>) o]);
	assert_int_equal(c->drops, 0);
	freeOffer(o);
	[item release];
	[provider release];
}
static void providerRemovalDiscardsPayload(void **state)
{
	reentrantProvider(*state, 1);
}
static void providerDestructionDiscardsPayload(void **state)
{
	reentrantProvider(*state, 2);
}

int main(void)
{
	@autoreleasepool {
		const struct CMUnitTest tests[] = {
			cmocka_unit_test_setup_teardown(textAndWindowCoordinates, setup, teardown),
			cmocka_unit_test_setup_teardown(filesPreferredAndCopied, setup, teardown),
			cmocka_unit_test_setup_teardown(mixedURLListRejected, setup, teardown),
			cmocka_unit_test_setup_teardown(remoteFileURLRejected, setup, teardown),
			cmocka_unit_test_setup_teardown(sourceAndApplicationRejection, setup, teardown),
			cmocka_unit_test_setup_teardown(selfRemovalCanAccept, setup, teardown),
			cmocka_unit_test_setup_teardown(selfDestructionCanAccept, setup, teardown),
			cmocka_unit_test_setup_teardown(destructionDuringOverRejects, setup, teardown),
			cmocka_unit_test_setup_teardown(scrollingAreaCoordinates, setup, teardown),
			cmocka_unit_test_setup_teardown(providerRemovalDiscardsPayload, setup, teardown),
			cmocka_unit_test_setup_teardown(providerDestructionDiscardsPayload, setup, teardown),
		};
		return cmocka_run_group_tests_name("drop Darwin", tests, NULL, NULL);
	}
}
