// 6 january 2015
#import <Cocoa/Cocoa.h>
#import <dlfcn.h>		// see undocumented.m
#import "../ui.h"
#import "../ui_darwin.h"
#import "../common/uipriv.h"

// TODO should we rename the uiprivMk things or not
// TODO what about renaming class wrapper functions that return the underlying class (like uiprivNewLabel())

#if __has_feature(objc_arc)
#error Sorry, libui cannot be compiled with ARC.
#endif

#define uiprivToNSString(str) [NSString stringWithUTF8String:(str)]

// nstextfield.m
void uiprivNSTextFieldSetStyleLabel(NSTextField *t);
void uiprivNSTextFieldSetStyleEntry(NSTextField *t);
void uiprivNSTextFieldSetStyleSearchEntry(NSTextField *t);

// menu.m
@interface uiprivMenuItem : NSMenuItem {
@public
	uiMenuItem *item;
}
- (BOOL)validateMenuItem:(NSMenuItem *)menuItem;
@end
@interface uiprivMenuManager : NSObject {
	BOOL hasQuit;
	BOOL hasPreferences;
	BOOL hasAbout;
	BOOL finalized;
}
@property (unsafe_unretained) uiprivMenuItem *quitItem;
@property (unsafe_unretained) uiprivMenuItem *preferencesItem;
@property (unsafe_unretained) uiprivMenuItem *aboutItem;
- (NSMenu *)makeMenubar;
- (BOOL)finalized;
- (void)finalize;
@end
extern void uiprivFinalizeMenus(void);
extern void uiprivUninitMenus(void);

// main.m
@interface uiprivApplicationClass : NSApplication
@end
// this is needed because NSApp is of type id, confusing clang
#define uiprivNSApp() ((uiprivApplicationClass *) NSApp)
@interface uiprivAppDelegate : NSObject<NSApplicationDelegate>
@property (strong) uiprivMenuManager *menuManager;
@end
#define uiprivAppDelegate() ((uiprivAppDelegate *) [uiprivNSApp() delegate])
typedef struct uiprivNextEventArgs uiprivNextEventArgs;
struct uiprivNextEventArgs {
	NSEventMask mask;
	NSDate *duration;
	// LONGTERM no NSRunLoopMode?
	NSString *mode;
	BOOL dequeue;
};
extern int uiprivMainStep(uiprivNextEventArgs *nea, BOOL (^interceptEvent)(NSEvent *));

// util.m
extern void uiprivDisableAutocorrect(NSTextView *);

// entry.m
extern NSTextField *uiprivNewEditableTextField(void);

// window.m
@interface uiprivNSWindow : NSWindow<NSWindowDelegate> {
	uiWindow *window;
}
- (BOOL)windowShouldClose:(id)sender;
- (void)windowDidResize:(NSNotification *)note;
- (void)windowDidEnterFullScreen:(NSNotification *)note;
- (void)windowDidExitFullScreen:(NSNotification *)note;
- (void)windowDidBecomeKey:(NSNotification *)note;
- (void)windowDidResignKey:(NSNotification *)note;
- (uiWindow *)window;
- (void)uiprivDoMove:(NSEvent *)initialEvent;
- (void)uiprivDoResize:(NSEvent *)initialEvent on:(uiWindowResizeEdge)edge;
@end
extern uiWindow *uiprivWindowFromNSWindow(NSWindow *);

// alloc.m
extern void uiprivInitAlloc(void);
extern void uiprivUninitAlloc(void);

// autolayout.m
extern NSLayoutConstraint *uiprivMkConstraint(id view1, NSLayoutAttribute attr1, NSLayoutRelation relation, id view2, NSLayoutAttribute attr2, CGFloat multiplier, CGFloat c, NSString *desc);
extern void uiprivJiggleViewLayout(NSView *view);
typedef struct uiprivSingleChildConstraints uiprivSingleChildConstraints;
struct uiprivSingleChildConstraints {
	NSLayoutConstraint *leadingConstraint;
	NSLayoutConstraint *topConstraint;
	NSLayoutConstraint *trailingConstraintGreater;
	NSLayoutConstraint *trailingConstraintEqual;
	NSLayoutConstraint *bottomConstraintGreater;
	NSLayoutConstraint *bottomConstraintEqual;
};
extern void uiprivSingleChildConstraintsEstablish(uiprivSingleChildConstraints *c, NSView *contentView, NSView *childView, BOOL hugsTrailing, BOOL hugsBottom, int margined, NSString *desc);
extern void uiprivSingleChildConstraintsRemove(uiprivSingleChildConstraints *c, NSView *cv);
extern void uiprivSingleChildConstraintsSetMargined(uiprivSingleChildConstraints *c, int margined);

// area.m
extern int uiprivSendAreaEvents(NSEvent *);

// areaevents.m
extern BOOL uiprivFromKeycode(unsigned short keycode, uiAreaKeyEvent *ke);
extern BOOL uiprivKeycodeModifier(unsigned short keycode, uiModifiers *mod);

// draw.m
extern uiDrawContext *uiprivDrawNewContext(CGContextRef, CGFloat);
extern void uiprivDrawFreeContext(uiDrawContext *);

// fontbutton.m
extern BOOL uiprivFontButtonInhibitSendAction(SEL sel, id from, id to);
extern BOOL uiprivFontButtonOverrideTargetForAction(SEL sel, id from, id to, id *override);
extern void uiprivSetupFontPanel(void);

// colorbutton.m
extern BOOL uiprivColorButtonInhibitSendAction(SEL sel, id from, id to);

// scrollview.m
typedef struct uiprivScrollViewCreateParams uiprivScrollViewCreateParams;
struct uiprivScrollViewCreateParams {
	// TODO MAYBE fix these identifiers
	NSView *DocumentView;
	NSColor *BackgroundColor;
	BOOL DrawsBackground;
	BOOL Bordered;
	BOOL HScroll;
	BOOL VScroll;
};
extern NSScrollView *uiprivMkScrollView(uiprivScrollViewCreateParams *p);

// label.m
extern NSTextField *uiprivNewLabel(NSString *str);

// image.m
extern NSImage *uiprivImageNSImage(uiImage *);

// winmoveresize.m
extern void uiprivDoManualMove(NSWindow *w, NSEvent *initialEvent);
extern void uiprivDoManualResize(NSWindow *w, NSEvent *initialEvent, uiWindowResizeEdge edge);

// undocumented.m
extern CFStringRef uiprivUNDOC_kCTFontPreferredSubFamilyNameKey;
extern CFStringRef uiprivUNDOC_kCTFontPreferredFamilyNameKey;
extern void uiprivLoadUndocumented(void);

// event.m
extern BOOL uiprivSendKeyboardEditEvents(uiprivApplicationClass *app, NSEvent *e);
