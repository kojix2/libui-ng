// uiImageView — macOS (AppKit) implementation (MVP, copy-owned, MRR)
#import "uipriv_darwin.h"

@interface uiprivImageViewHost : NSView
@property(nonatomic, retain) NSImageView *iv;
@property(nonatomic, retain) CALayer *imageLayer;
@end

@implementation uiprivImageViewHost
- (instancetype)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	if (self) {
		// Create NSImageView for Center and Fit modes
		_iv = [[NSImageView alloc] init];
		_iv.imageFrameStyle = NSImageFrameNone;
		_iv.imageAlignment = NSImageAlignCenter;
		_iv.imageScaling = NSImageScaleProportionallyUpOrDown; // default Fit
		_iv.translatesAutoresizingMaskIntoConstraints = NO;
		[self addSubview:_iv];
		// NSView retains the subview, so release our reference to balance alloc
		[_iv release];

		// Create CALayer for Fill mode (aspect-fill with crop)
		_imageLayer = [CALayer layer];
		_imageLayer.contentsGravity = kCAGravityResizeAspectFill;
		_imageLayer.masksToBounds = YES;
		_imageLayer.hidden = YES; // Initially hidden, shown only for Fill mode
		[self setWantsLayer:YES];
		[self.layer addSublayer:_imageLayer];

		[NSLayoutConstraint activateConstraints:@[
			[_iv.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
			[_iv.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
			[_iv.topAnchor constraintEqualToAnchor:self.topAnchor],
			[_iv.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
		]];
	}
	return self;
}

- (void)layout
{
	[super layout];
	// Update CALayer frame to match view bounds
	_imageLayer.frame = self.bounds;
}

// Update contentsScale when backing properties change (e.g., monitor move)
- (void)viewDidChangeBackingProperties
{
	[super viewDidChangeBackingProperties];
	CGFloat scale = self.window ? self.window.backingScaleFactor : 1.0;
	self.layer.contentsScale = scale;
	self.imageLayer.contentsScale = scale;
}
@end

#define uiImageViewSignature 0x49566965

struct uiImageView {
	uiDarwinControl c;
	uiprivImageViewHost *host;
	NSImageView *iv;
	uiImageViewContentMode mode;
};

uiDarwinControlAllDefaultsExceptDestroy(uiImageView, host)

static void uiImageViewDestroy(uiControl *c)
{
	uiImageView *v = uiImageView(c);
	// Manual memory management (MRR) - release the host view
	[v->host release];
	uiFreeControl(uiControl(v));
}

static NSImageScaling scalingFor(uiImageViewContentMode m)
{
	switch (m) {
	case uiImageViewContentCenter: return NSImageScaleNone;
	case uiImageViewContentFit:    return NSImageScaleProportionallyUpOrDown;
	case uiImageViewContentFill:   return NSImageScaleProportionallyUpOrDown; // Not used for Fill mode
	}
	return NSImageScaleProportionallyUpOrDown;
}

static void updateDisplayMode(uiImageView *v)
{
	if (v->mode == uiImageViewContentFill) {
		// Use CALayer for Fill mode (aspect-fill with crop)
		v->host.iv.hidden = YES;
		v->host.imageLayer.hidden = NO;
		
		NSImage *image = v->host.iv.image;
		if (image != nil) {
			// Convert to CGImage (selects optimal representation)
			CGImageRef cgimg = [image CGImageForProposedRect:NULL context:nil hints:nil];
			v->host.imageLayer.contents = (__bridge id)cgimg;
			
			// HiDPI support: apply current backing scale factor
			CGFloat scale = 1.0;
			if (v->host.window != nil) {
				scale = v->host.window.backingScaleFactor;
			} else if (NSScreen.mainScreen != nil) {
				scale = NSScreen.mainScreen.backingScaleFactor;
			}
			v->host.imageLayer.contentsScale = scale;
		} else {
			v->host.imageLayer.contents = nil;
		}
	} else {
		// Release held CGImage when exiting Fill mode to prevent unnecessary retention
		v->host.imageLayer.contents = nil;
		v->host.iv.hidden = NO;
		v->host.imageLayer.hidden = YES;
		[v->host.iv setImageScaling:scalingFor(v->mode)];
	}
}

uiImageView *uiNewImageView(void)
{
	uiImageView *v;

	uiDarwinNewControl(uiImageView, v);

	v->host = [[uiprivImageViewHost alloc] initWithFrame:NSZeroRect];
	v->iv = v->host.iv;
	v->mode = uiImageViewContentFit;
	[v->iv setImageScaling:scalingFor(v->mode)];
	return v;
}

void uiImageViewSetContentMode(uiImageView *v, uiImageViewContentMode mode)
{
	v->mode = mode;
	updateDisplayMode(v);
	[v->host setNeedsLayout:YES];
}

void uiImageViewSetImage(uiImageView *v, const uiImage *image)
{
	if (image == NULL) {
		[v->iv setImage:nil];
		v->host.imageLayer.contents = nil;
		return;
	}
	// Copy-owned: take our own strong reference to an NSImage equivalent
	NSImage *nsimg = uiprivImageNSImage((uiImage *)image);
	// Make an explicit copy so lifetime is decoupled from uiImage (representations may share until written; acceptable for MVP)
	NSImage *owned = [nsimg copy];
	[v->iv setImage:owned];
	
	// Update display mode to ensure correct rendering
	updateDisplayMode(v);
	
	[owned release];
}
