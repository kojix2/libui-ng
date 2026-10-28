#import "uipriv_darwin.h"
#include "drop.h"

typedef struct darwinDrop darwinDrop;
struct darwinDrop {
	id destination; // Owned by the control; never accessed after detach.
	uint64_t generation;
	NSInteger sequence;
	BOOL hover;
	BOOL accepted;
};

static uiDropStatus enable(uiprivDropEndpoint *e, int active)
{
	darwinDrop *d = e->native;
	if (active)
		[d->destination registerForDraggedTypes:@[NSPasteboardTypeFileURL,
			NSPasteboardTypeString, NSFilenamesPboardType]];
	else
		[d->destination unregisterDraggedTypes];
	return uiDropStatusOK;
}

static void cancel(uiprivDropEndpoint *e)
{
	darwinDrop *d = e->native;
	d->hover = NO;
	d->accepted = NO;
}

static void dispose(uiprivDropEndpoint *e) { uiprivFree(e->native); }

uiprivDropEndpoint *uiprivDarwinDropNew(uiControl *c, id destination)
{
	uiprivDropEndpoint *e = uiprivDropNew(c);
	darwinDrop *d = uiprivNew(darwinDrop);
	d->destination = destination;
	e->native = d;
	e->enable = enable;
	e->cancel = cancel;
	e->dispose = dispose;
	return e;
}

static unsigned int formats(NSPasteboard *pb)
{
	unsigned int result = 0;
	NSArray *types = [pb types];
	if ([types containsObject:NSPasteboardTypeFileURL] || [types containsObject:NSFilenamesPboardType])
		result |= uiDropFormatFiles;
	if ([types containsObject:NSPasteboardTypeString])
		result |= uiDropFormatText;
	return result;
}

static uiDropInfo makeInfo(uiprivDropEndpoint *e, id<NSDraggingInfo> sender, NSView *view, BOOL window)
{
	NSPoint p = [view convertPoint:[sender draggingLocation] fromView:nil];
	if (window && ![view isFlipped])
		p.y = NSMaxY([view bounds]) - p.y;
	return uiprivDropMakeInfo(e, formats([sender draggingPasteboard]), p.x, p.y);
}

NSDragOperation uiprivDarwinDropOver(uiprivDropEndpoint *e, id<NSDraggingInfo> sender, NSView *view, BOOL window)
{
	NSDragOperation result = NSDragOperationNone;
	uint64_t generation;
	darwinDrop *d;
	uiDropInfo info;
	if (e == NULL)
		return result;
	uiprivDropRetain(e);
	uiprivUserCallbackEnter(NULL);
	generation = e->generation;
	d = e->native;
	info = makeInfo(e, sender, view, window);
	if (uiprivDropActive(e, generation)) {
		d->generation = generation;
		d->sequence = [sender draggingSequenceNumber];
		d->hover = YES;
		d->accepted = uiprivDropOver(e, &info,
			([sender draggingSourceOperationMask] & NSDragOperationCopy) != 0) == uiDropActionCopy;
		if (d->accepted && uiprivDropActive(e, generation))
			result = NSDragOperationCopy;
	}
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return result;
}

void uiprivDarwinDropLeave(uiprivDropEndpoint *e, id<NSDraggingInfo> sender)
{
	darwinDrop *d;
	uint64_t generation;
	if (e == NULL)
		return;
	uiprivDropRetain(e);
	uiprivUserCallbackEnter(NULL);
	d = e->native;
	generation = d->generation;
	if (d->hover && d->sequence == [sender draggingSequenceNumber]) {
		d->hover = NO;
		d->accepted = NO;
		uiprivDropLeave(e, generation);
	}
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
}

BOOL uiprivDarwinDropPrepare(uiprivDropEndpoint *e, id<NSDraggingInfo> sender)
{
	darwinDrop *d;
	BOOL accepted;
	uint64_t generation;
	if (e == NULL)
		return NO;
	uiprivDropRetain(e);
	uiprivUserCallbackEnter(NULL);
	d = e->native;
	generation = d->generation;
	accepted = d->accepted && d->sequence == [sender draggingSequenceNumber] &&
		([sender draggingSourceOperationMask] & NSDragOperationCopy) != 0 &&
		uiprivDropActive(e, generation);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return accepted;
}

static int readFiles(NSPasteboard *pb, uiprivDropData *payload)
{
	size_t fileItems = 0;
	NSArray *urls = [pb readObjectsForClasses:@[[NSURL class]]
		options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];
	if ([urls count] == 0 || [urls count] > uiprivDropMaxFiles)
		return 0;
	// File-only reading must not silently turn a mixed URL list into success.
	for (NSPasteboardItem *item in [pb pasteboardItems]) {
		NSString *value = [item stringForType:NSPasteboardTypeURL];
		if (value != nil && ![[NSURL URLWithString:value] isFileURL])
			return 0;
		if ([[item types] containsObject:NSPasteboardTypeFileURL]) {
			value = [item stringForType:NSPasteboardTypeFileURL];
			if (value == nil || ![[NSURL URLWithString:value] isFileURL])
				return 0;
			fileItems++;
		}
	}
	if (fileItems != 0 && fileItems != [urls count])
		return 0;
	for (NSURL *url in urls) {
		NSString *host = [url host];
		NSString *path = [url path];
		// A remote URL's path component is not a local filesystem path.
		if (![url isFileURL] || ([host length] != 0 && [host caseInsensitiveCompare:@"localhost"] != NSOrderedSame) ||
			!uiprivDropAddFile(payload, [path UTF8String], [path lengthOfBytesUsingEncoding:NSUTF8StringEncoding]))
			return 0;
	}
	return 1;
}

BOOL uiprivDarwinDropPerform(uiprivDropEndpoint *e, id<NSDraggingInfo> sender, NSView *view, BOOL window)
{
	uiprivDropData payload = {0};
	uiDropInfo info;
	uint64_t generation;
	BOOL accepted = NO;
	darwinDrop *d;
	if (e == NULL)
		return NO;
	uiprivDropRetain(e);
	uiprivUserCallbackEnter(NULL);
	d = e->native;
	generation = d->generation;
	if (!uiprivDarwinDropPrepare(e, sender))
		goto done;
	info = makeInfo(e, sender, view, window);
	d->accepted = NO;
	d->hover = NO;
	// End feedback even when AppKit does not send draggingExited after Drop.
	uiprivDropLeave(e, generation);
	if (!uiprivDropActive(e, generation))
		goto done;
	@try {
		NSPasteboard *pb = [sender draggingPasteboard];
		int valid = 0;
		if (info.Format == uiDropFormatFiles)
			valid = readFiles(pb, &payload);
		else if (info.Format == uiDropFormatText) {
			NSString *s = [pb stringForType:NSPasteboardTypeString];
			if (s != nil)
				valid = uiprivDropSetText(&payload, [s UTF8String],
					[s lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
		}
		if (valid)
			accepted = uiprivDropDeliver(e, generation, &info, &payload.data) == uiDropAccepted;
	} @catch (NSException *exception) {
		accepted = NO;
	}
done:
	uiprivDropDataClear(&payload);
	uiprivDropRelease(e);
	uiprivUserCallbackLeave();
	return accepted;
}
