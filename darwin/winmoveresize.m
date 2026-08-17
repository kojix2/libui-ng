// 1 november 2016
#import "uipriv_darwin.h"

// TODO option while resizing resizes both opposing sides at once (thanks swillits in irc.freenode.net/#macdev for showing this to me); figure out how far back that behavior goes when we do implement it

// because we are changing the window frame each time the mouse moves, the successive -[NSEvent locationInWindow]s cannot be meaningfully used together
// make sure they are all following some sort of standard to avoid this problem; the screen is the most obvious possibility since it requires only one conversion (the only one that a NSWindow provides)
static NSPoint makeIndependent(NSPoint p, NSWindow *w)
{
	NSRect r;

	r.origin = p;
	// mikeash in irc.freenode.net/#macdev confirms both that any size will do and that we can safely ignore the resultant size
	r.size = NSZeroSize;
	return [w convertRectToScreen:r].origin;
}

void uiprivDoManualMove(NSWindow *w, NSEvent *initialEvent)
{
	[w performWindowDragWithEvent:initialEvent];
}

// see http://stackoverflow.com/a/40352996/3408572
static void minMaxAutoLayoutSizes(NSWindow *w, NSSize *min, NSSize *max)
{
	NSLayoutConstraint *cw, *ch;
	NSView *contentView;
	NSRect contentRect, prevFrame;

	prevFrame = [w frame];

	// minimum: encourage the window to be as small as possible
	contentView = [w contentView];
	cw = uiprivMkConstraint(contentView, NSLayoutAttributeWidth,
		NSLayoutRelationEqual,
		nil, NSLayoutAttributeNotAnAttribute,
		0, 0,
		@"window minimum width finding constraint");
	[cw setPriority:NSLayoutPriorityDragThatCanResizeWindow];
	[contentView addConstraint:cw];
	ch = uiprivMkConstraint(contentView, NSLayoutAttributeHeight,
		NSLayoutRelationEqual,
		nil, NSLayoutAttributeNotAnAttribute,
		0, 0,
		@"window minimum height finding constraint");
	[ch setPriority:NSLayoutPriorityDragThatCanResizeWindow];
	[contentView addConstraint:ch];
	*min = [contentView fittingSize];
	[contentView removeConstraint:cw];
	[contentView removeConstraint:ch];
	contentRect = NSMakeRect(0, 0, min->width, min->height);
	*min = [w frameRectForContentRect:contentRect].size;

	// maximum: encourage the window to be as large as possible
	contentView = [w contentView];
	cw = uiprivMkConstraint(contentView, NSLayoutAttributeWidth,
		NSLayoutRelationEqual,
		nil, NSLayoutAttributeNotAnAttribute,
		0, CGFLOAT_MAX,
		@"window maximum width finding constraint");
	[cw setPriority:NSLayoutPriorityDragThatCanResizeWindow];
	[contentView addConstraint:cw];
	ch = uiprivMkConstraint(contentView, NSLayoutAttributeHeight,
		NSLayoutRelationEqual,
		nil, NSLayoutAttributeNotAnAttribute,
		0, CGFLOAT_MAX,
		@"window maximum height finding constraint");
	[ch setPriority:NSLayoutPriorityDragThatCanResizeWindow];
	[contentView addConstraint:ch];
	*max = [contentView fittingSize];
	[contentView removeConstraint:cw];
	[contentView removeConstraint:ch];
	contentRect = NSMakeRect(0, 0, max->width, max->height);
	*max = [w frameRectForContentRect:contentRect].size;

	[w setFrame:prevFrame display:YES];		// TODO really YES?
}

static void handleResizeLeft(NSRect *frame, NSPoint old, NSPoint new)
{
	frame->origin.x += new.x - old.x;
	frame->size.width -= new.x - old.x;
}

static void handleResizeTop(NSRect *frame, NSPoint old, NSPoint new)
{
	frame->size.height += new.y - old.y;
}

static void handleResizeRight(NSRect *frame, NSPoint old, NSPoint new)
{
	frame->size.width += new.x - old.x;
}

static void handleResizeBottom(NSRect *frame, NSPoint old, NSPoint new)
{
	frame->origin.y += new.y - old.y;
	frame->size.height -= new.y - old.y;
}

struct onResizeDragParams {
	NSWindow *w;
	// using the previous point causes weird issues like the mouse seeming to fall behind the window edge... so do this instead
	NSRect initialFrame;
	NSPoint initialPoint;
	uiWindowResizeEdge edge;
	NSSize min;
	NSSize max;
};

static void onResizeDrag(struct onResizeDragParams *p, NSEvent *e)
{
	NSPoint new;
	NSRect frame;
	CGFloat fixedRight, fixedTop;

	new = makeIndependent([e locationInWindow], p->w);
	frame = p->initialFrame;

	// horizontal
	switch (p->edge) {
	case uiWindowResizeEdgeLeft:
	case uiWindowResizeEdgeTopLeft:
	case uiWindowResizeEdgeBottomLeft:
		handleResizeLeft(&frame, p->initialPoint, new);
		break;
	case uiWindowResizeEdgeRight:
	case uiWindowResizeEdgeTopRight:
	case uiWindowResizeEdgeBottomRight:
		handleResizeRight(&frame, p->initialPoint, new);
		break;
	}
	// vertical
	switch (p->edge) {
	case uiWindowResizeEdgeTop:
	case uiWindowResizeEdgeTopLeft:
	case uiWindowResizeEdgeTopRight:
		handleResizeTop(&frame, p->initialPoint, new);
		break;
	case uiWindowResizeEdgeBottom:
	case uiWindowResizeEdgeBottomLeft:
	case uiWindowResizeEdgeBottomRight:
		handleResizeBottom(&frame, p->initialPoint, new);
		break;
	}

	// Constrain the frame while keeping the edge opposite the dragged edge fixed.
	fixedRight = NSMaxX(p->initialFrame);
	fixedTop = NSMaxY(p->initialFrame);
	if (frame.size.width < p->min.width)
		frame.size.width = p->min.width;
	if (frame.size.height < p->min.height)
		frame.size.height = p->min.height;
	if (frame.size.width > p->max.width)
		frame.size.width = p->max.width;
	if (frame.size.height > p->max.height)
		frame.size.height = p->max.height;
	switch (p->edge) {
	case uiWindowResizeEdgeLeft:
	case uiWindowResizeEdgeTopLeft:
	case uiWindowResizeEdgeBottomLeft:
		frame.origin.x = fixedRight - frame.size.width;
		break;
	}
	switch (p->edge) {
	case uiWindowResizeEdgeBottom:
	case uiWindowResizeEdgeBottomLeft:
	case uiWindowResizeEdgeBottomRight:
		frame.origin.y = fixedTop - frame.size.height;
		break;
	}

	[p->w setFrame:frame display:YES];			// and do reflect the new frame immediately
}

static void validateResizeMouseDownType(NSEventType type)
{
	switch (type) {
	case NSEventTypeLeftMouseDown:
	case NSEventTypeRightMouseDown:
	case NSEventTypeOtherMouseDown:
		return;
	default:
		uiprivImplBug("invalid initial event type in uiprivDoManualResize(): %lu",
			(unsigned long) type);
	}
}

static BOOL isMouseDraggedType(NSEventType type)
{
	return type == NSEventTypeLeftMouseDragged ||
		type == NSEventTypeRightMouseDragged ||
		type == NSEventTypeOtherMouseDragged;
}

static BOOL isMouseUpType(NSEventType type)
{
	return type == NSEventTypeLeftMouseUp ||
		type == NSEventTypeRightMouseUp ||
		type == NSEventTypeOtherMouseUp;
}

// TODO do our events get fired with this? *should* they?
void uiprivDoManualResize(NSWindow *w, NSEvent *initialEvent, uiWindowResizeEdge edge)
{
	__block struct onResizeDragParams rdp;
	uiprivNextEventArgs nea;
	BOOL (^handleEvent)(NSEvent *e);
	__block BOOL done;
	NSInteger buttonNumber;

	validateResizeMouseDownType([initialEvent type]);
	buttonNumber = [initialEvent buttonNumber];

	rdp.w = w;
	rdp.initialFrame = [rdp.w frame];
	rdp.initialPoint = makeIndependent([initialEvent locationInWindow], rdp.w);
	rdp.edge = edge;
	// TODO what happens if these change during the loop?
	minMaxAutoLayoutSizes(rdp.w, &(rdp.min), &(rdp.max));

	// Follow the physical button even if AppKit changes the event type family,
	// and consume other buttons as complete sequences during the modal resize.
	nea.mask = NSEventMaskLeftMouseDown | NSEventMaskLeftMouseDragged | NSEventMaskLeftMouseUp |
		NSEventMaskRightMouseDown | NSEventMaskRightMouseDragged | NSEventMaskRightMouseUp |
		NSEventMaskOtherMouseDown | NSEventMaskOtherMouseDragged | NSEventMaskOtherMouseUp;
	nea.duration = [NSDate distantFuture];
	nea.mode = NSEventTrackingRunLoopMode;		// nextEventMatchingMask: docs suggest using this for manual mouse tracking
	nea.dequeue = YES;
	handleEvent = ^(NSEvent *e) {
		if ([e buttonNumber] != buttonNumber)
			return YES;	// ignore other buttons during the modal resize
		if (isMouseUpType([e type])) {
			done = YES;
			return YES;	// do not send
		}
		if (isMouseDraggedType([e type]))
			onResizeDrag(&rdp, e);
		return YES;		// do not send
	};
	done = NO;
	while (uiprivMainStep(&nea, handleEvent))
		if (done)
			break;
}
