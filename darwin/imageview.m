// uiImageView — macOS (AppKit) implementation (MVP, copy-owned, MRR, no-QuartzCore)
#import "uipriv_darwin.h"
// 依存: AppKit, CoreGraphics, Foundation

@interface uiprivImageViewHost : NSView
@property(nonatomic, retain) NSImageView *iv;
@end

@implementation uiprivImageViewHost
- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        _iv = [[NSImageView alloc] init];
        _iv.imageFrameStyle = NSImageFrameNone;
        _iv.imageAlignment  = NSImageAlignCenter;
        _iv.imageScaling    = NSImageScaleProportionallyUpOrDown; // default Fit
        _iv.translatesAutoresizingMaskIntoConstraints = NO;
        [self addSubview:_iv];
        // subview 追加で NSView が retain するため、こちらの retain を解放
        [_iv release];

        [NSLayoutConstraint activateConstraints:@[
            [_iv.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
            [_iv.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
            [_iv.topAnchor constraintEqualToAnchor:self.topAnchor],
            [_iv.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
        ]];
    }
    return self;
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
    [v->host release];
    uiFreeControl(uiControl(v));
}

static NSImageScaling scalingFor(uiImageViewContentMode m)
{
    switch (m) {
    case uiImageViewContentCenter: return NSImageScaleNone;
    case uiImageViewContentFit:    return NSImageScaleProportionallyUpOrDown;
    }
    return NSImageScaleProportionallyUpOrDown;
}

static void updateDisplayMode(uiImageView *v)
{
    v->iv.hidden = NO;
    [v->iv setImageScaling:scalingFor(v->mode)];
    // AppKit のレイアウトに任せる（CALayer なし）
}

uiImageView *uiNewImageView(void)
{
    uiImageView *v;
    uiDarwinNewControl(uiImageView, v);

    v->host = [[uiprivImageViewHost alloc] initWithFrame:NSZeroRect];
    v->iv   = v->host.iv;
    v->mode = uiImageViewContentFit; // 既定: Fit
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
        return;
    }
    // Copy-owned: uiImage から NSImage を取得し、明示コピーで寿命を独立
    NSImage *nsimg = uiprivImageNSImage((uiImage *)image);
    NSImage *owned = [nsimg copy];
    [v->iv setImage:owned];
    updateDisplayMode(v);
    [owned release];
}
