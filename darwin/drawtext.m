// 7 march 2018
#import "uipriv_darwin.h"
#import "draw.h"
#import "attrstr.h"

// problem: for a CTFrame made from an empty string, the CTLine array will be empty, and we will crash when doing anything requiring a CTLine
// solution: for those cases, maintain a separate framesetter just for computing those things
// in the usual case, the separate copy will just be identical to the regular one, with extra references to everything within
@interface uiprivTextFrame : NSObject {
	CFAttributedStringRef attrstr;
	NSArray *backgroundParams;
	CTFramesetterRef framesetter;
	CGSize size;
	CGPathRef path;
	CTFrameRef frame;
}
- (id)initWithLayoutParams:(uiDrawTextLayoutParams *)p;
- (void)draw:(uiDrawContext *)c textLayout:(uiDrawTextLayout *)tl at:(double)x y:(double)y;
- (void)returnWidth:(double *)width height:(double *)height;
- (CFArrayRef)lines;
@end

@implementation uiprivDrawTextBackgroundParams

- (id)initWithStart:(size_t)s end:(size_t)e r:(double)red g:(double)green b:(double)blue a:(double)alpha
{
	self = [super init];
	if (self) {
		self->start = s;
		self->end = e;
		self->r = red;
		self->g = green;
		self->b = blue;
		self->a = alpha;
	}
	return self;
}

- (void)draw:(CGContextRef)c layout:(uiDrawTextLayout *)layout at:(double)x y:(double)y utf8Mapping:(const size_t *)u16tou8
{
	// TODO
}

@end

@implementation uiprivTextFrame

- (id)initWithLayoutParams:(uiDrawTextLayoutParams *)p
{
	CFRange range;
	CGFloat cgwidth;
	CGRect rect;

	self = [super init];
	if (self) {
		self->attrstr = uiprivAttributedStringToCFAttributedString(p, &(self->backgroundParams));
		if (self->attrstr == NULL) {
			[self release];
			return nil;
		}
		// TODO kCTParagraphStyleSpecifierMaximumLineSpacing, kCTParagraphStyleSpecifierMinimumLineSpacing, kCTParagraphStyleSpecifierLineSpacingAdjustment for line spacing
		self->framesetter = CTFramesetterCreateWithAttributedString(self->attrstr);
		if (self->framesetter == NULL) {
			[self release];
			return nil;
		}

		range.location = 0;
		range.length = CFAttributedStringGetLength(self->attrstr);

		cgwidth = (CGFloat) (p->Width);
		if (cgwidth < 0)
			cgwidth = CGFLOAT_MAX;
		self->size = CTFramesetterSuggestFrameSizeWithConstraints(self->framesetter,
			range,
			// TODO kCTFramePathWidthAttributeName?
			NULL,
			CGSizeMake(cgwidth, CGFLOAT_MAX),
			NULL);

		rect.origin = CGPointZero;
		CGFloat frameWidth = (p->Width < 0) ? self->size.width : cgwidth;
		rect.size = CGSizeMake(frameWidth, self->size.height);
		self->path = CGPathCreateWithRect(rect, NULL);
		if (self->path == NULL) {
			[self release];
			return nil;
		}
		self->frame = CTFramesetterCreateFrame(self->framesetter,
			range,
			self->path,
			// TODO kCTFramePathWidthAttributeName?
			NULL);
		if (self->frame == NULL) {
			[self release];
			return nil;
		}
	}
	return self;
}

- (void)dealloc
{
	if (self->frame != NULL)
		CFRelease(self->frame);
	if (self->path != NULL)
		CFRelease(self->path);
	if (self->framesetter != NULL)
		CFRelease(self->framesetter);
	[self->backgroundParams release];
	if (self->attrstr != NULL)
		CFRelease(self->attrstr);
	[super dealloc];
}

- (void)draw:(uiDrawContext *)c textLayout:(uiDrawTextLayout *)tl at:(double)x y:(double)y
{
	uiprivDrawTextBackgroundParams *dtb;
	CGAffineTransform textMatrix;

	CGContextSaveGState(c->c);
	// save the text matrix because it's not part of the graphics state
	textMatrix = CGContextGetTextMatrix(c->c);

	for (dtb in self->backgroundParams)
		/* TODO */;

	// Core Text doesn't draw onto a flipped view correctly; we have to pretend it was unflipped
	// see the iOS bits of the first example at https://developer.apple.com/library/mac/documentation/StringsTextFonts/Conceptual/CoreText_Programming/LayoutOperations/LayoutOperations.html#//apple_ref/doc/uid/TP40005533-CH12-SW1 (iOS is naturally flipped)
	// TODO how is this affected by a non-identity CTM?
	CGContextTranslateCTM(c->c, 0, c->height);
	CGContextScaleCTM(c->c, 1.0, -1.0);
	CGContextSetTextMatrix(c->c, CGAffineTransformIdentity);

	// wait, that's not enough; we need to offset y values to account for our new flipping
	// TODO explain this calculation
	y = c->height - self->size.height - y;

	// CTFrameDraw() draws in the path we specified when creating the frame
	// this means that in our usage, CTFrameDraw() will draw at (0,0)
	// so move the origin to be at (x,y) instead
	// TODO are the signs correct?
	CGContextTranslateCTM(c->c, x, y);

	CTFrameDraw(self->frame, c->c);

	CGContextSetTextMatrix(c->c, textMatrix);
	CGContextRestoreGState(c->c);
}

- (void)returnWidth:(double *)width height:(double *)height
{
	if (width != NULL)
		*width = self->size.width;
	if (height != NULL)
		*height = self->size.height;
}

- (CFArrayRef)lines
{
	return CTFrameGetLines(self->frame);
}

@end

struct uiDrawTextLayout {
	uiprivTextFrame *frame;
	uiprivTextFrame *forLines;

	// for converting CFAttributedString indices from/to byte offsets
	size_t *u8tou16;
	size_t nUTF8;
	size_t *u16tou8;
	size_t nUTF16;
};

uiDrawTextLayout *uiDrawNewTextLayout(uiDrawTextLayoutParams *p)
{
	uiDrawTextLayout *tl;

	tl = uiprivNew(uiDrawTextLayout);
	tl->frame = nil;
	tl->forLines = nil;
	tl->u8tou16 = NULL;
	tl->nUTF8 = 0;
	tl->u16tou8 = NULL;
	tl->nUTF16 = 0;
	tl->frame = [[uiprivTextFrame alloc] initWithLayoutParams:p];
	if (tl->frame == nil)
		goto fail;
	if (uiAttributedStringLen(p->String) != 0)
		tl->forLines = [tl->frame retain];
	else {
		uiAttributedString *space;
		uiDrawTextLayoutParams p2;

		space = uiNewAttributedString(" ");
		p2 = *p;
		p2.String = space;
		tl->forLines = [[uiprivTextFrame alloc] initWithLayoutParams:&p2];
		uiFreeAttributedString(space);
		if (tl->forLines == nil)
			goto fail;
	}

	// and finally copy the UTF-8/UTF-16 conversion tables
	tl->u8tou16 = uiprivAttributedStringCopyUTF8ToUTF16Table(p->String, &(tl->nUTF8));
	tl->u16tou8 = uiprivAttributedStringCopyUTF16ToUTF8Table(p->String, &(tl->nUTF16));
	return tl;

fail:
	uiDrawFreeTextLayout(tl);
	return NULL;
}

void uiDrawFreeTextLayout(uiDrawTextLayout *tl)
{
	if (tl == NULL)
		return;
	if (tl->u16tou8 != NULL)
		uiprivFree(tl->u16tou8);
	if (tl->u8tou16 != NULL)
		uiprivFree(tl->u8tou16);
	[tl->forLines release];
	[tl->frame release];
	uiprivFree(tl);
}

void uiDrawText(uiDrawContext *c, uiDrawTextLayout *tl, double x, double y)
{
	if (tl == NULL)
		return;
	[tl->frame draw:c textLayout:tl at:x y:y];
}

// TODO document that the width and height of a layout is not necessarily the sum of the widths and heights of its constituent lines
// TODO width doesn't include trailing whitespace...
// TODO figure out how paragraph spacing should play into this
// TODO standardize and document the behavior of this on an empty layout
void uiDrawTextLayoutExtents(uiDrawTextLayout *tl, double *width, double *height)
{
	if (tl == NULL) {
		*width = 0;
		*height = 0;
		return;
	}
	// TODO explain this, given the above
	[tl->frame returnWidth:width height:NULL];
	[tl->forLines returnWidth:NULL height:height];
}

void uiLoadControlFont(uiFontDescriptor *f)
{
	CTFontRef ctfont;
	CTFontDescriptorRef ctdesc;

	ctfont = (CTFontRef) [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSControlSizeRegular]];
	ctdesc = CTFontCopyFontDescriptor(ctfont);
	uiprivFontDescriptorFromCTFontDescriptor(ctdesc, f);
	CFRelease(ctdesc);
	f->Size = CTFontGetSize(ctfont);
}

void uiFreeFontDescriptor(uiFontDescriptor *desc)
{
	uiFreeText((char *) (desc->Family));
}
