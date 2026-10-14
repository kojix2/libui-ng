// 26 june 2015
#import "uipriv_darwin.h"

// LONGTERM restructure this whole file
// LONGTERM explicitly document this works as we want
// LONGTERM note that font and color buttons also do this

#define windowWindow(w) ((w) ? (NSWindow *) uiControlHandle(uiControl(w)) : nil)

// source of code modal logic: http://stackoverflow.com/questions/604768/wait-for-nsalert-beginsheetmodalforwindow

// note: whether extensions are actually shown depends on a user setting in Finder; we can't control it here
static void setupSavePanel(NSSavePanel *s)
{
	[s setCanCreateDirectories:YES];
	[s setExtensionHidden:NO];
	[s setCanSelectHiddenExtension:NO];
}

static NSInteger runPanel(NSWindow *parent, NSSavePanel *s)
{
	NSInteger result;

	if (parent == nil) {
		result = [s runModal];
	} else {
		[s beginSheetModalForWindow:parent completionHandler:^(NSInteger result) {
			[uiprivNSApp() stopModalWithCode:result];
		}];
		result = [uiprivNSApp() runModalForWindow:s];
	}
	return result;
}

static char *runSavePanel(NSWindow *parent, NSSavePanel *s)
{
	if (runPanel(parent, s) != NSFileHandlingPanelOKButton)
		return NULL;
	return uiDarwinNSStringToText([[s URL] path]);
}

char *uiOpenFile(uiWindow *parent)
{
	NSOpenPanel *o;

	o = [NSOpenPanel openPanel];
	[o setCanChooseFiles:YES];
	[o setCanChooseDirectories:NO];
	[o setResolvesAliases:NO];
	[o setAllowsMultipleSelection:NO];
	setupSavePanel(o);
	// panel is autoreleased
	return runSavePanel(windowWindow(parent), o);
}

char *uiOpenFolder(uiWindow *parent)
{
	NSOpenPanel *o;

	o = [NSOpenPanel openPanel];
	[o setCanChooseFiles:NO];
	[o setCanChooseDirectories:YES];
	[o setResolvesAliases:NO];
	[o setAllowsMultipleSelection:NO];
	setupSavePanel(o);
	// panel is autoreleased
	return runSavePanel(windowWindow(parent), o);
}

static char **openMultiple(uiWindow *parent, BOOL folders)
{
	NSOpenPanel *o;
	NSArray *urls;
	NSUInteger i, count;
	char **paths;
	NSURL *url;
	NSString *path;

	o = [NSOpenPanel openPanel];
	[o setCanChooseFiles:!folders];
	[o setCanChooseDirectories:folders];
	[o setResolvesAliases:NO];
	[o setAllowsMultipleSelection:YES];
	setupSavePanel(o);
	if (runPanel(windowWindow(parent), o) != NSFileHandlingPanelOKButton)
		return NULL;
	urls = [o URLs];
	count = [urls count];
	if (count == 0)
		return NULL;
	paths = uiprivNewTextArray((size_t) count);
	if (paths == NULL)
		return NULL;
	for (i = 0; i < count; i++) {
		url = [urls objectAtIndex:i];
		path = [url path];
		if (![url isFileURL] || path == nil) {
			uiFreeTextArray(paths);
			return NULL;
		}
		paths[i] = uiDarwinNSStringToText(path);
	}
	return paths;
}

char **uiOpenFiles(uiWindow *parent)
{
	return openMultiple(parent, NO);
}

char **uiOpenFolders(uiWindow *parent)
{
	return openMultiple(parent, YES);
}

char *uiSaveFile(uiWindow *parent)
{
	NSSavePanel *s;

	s = [NSSavePanel savePanel];
	setupSavePanel(s);
	// panel is autoreleased
	return runSavePanel(windowWindow(parent), s);
}

@interface libuiCodeModalAlertPanel : NSObject {
	NSAlert *panel;
	NSWindow *parent;
}
- (id)initWithPanel:(NSAlert *)p parent:(NSWindow *)w;
- (NSInteger)run;
@end

@implementation libuiCodeModalAlertPanel

- (id)initWithPanel:(NSAlert *)p parent:(NSWindow *)w
{
	self = [super init];
	if (self) {
		self->panel = p;
		self->parent = w;
	}
	return self;
}

- (NSInteger)run
{
	if (self->parent == nil)
		return [self->panel runModal];

	[self->panel beginSheetModalForWindow:self->parent completionHandler:^(NSModalResponse result) {
		[uiprivNSApp() stopModalWithCode:result];
	}];
	return [uiprivNSApp() runModalForWindow:[self->panel window]];
}

@end

static void msgbox(NSWindow *parent, const char *title, const char *description, NSAlertStyle style)
{
	NSAlert *a;
	libuiCodeModalAlertPanel *cm;

	a = [NSAlert new];
	[a setAlertStyle:style];
	[a setShowsHelp:NO];
	[a setShowsSuppressionButton:NO];
	[a setMessageText:uiprivToNSString(title)];
	[a setInformativeText:uiprivToNSString(description)];
	[a addButtonWithTitle:@"OK"];
	cm = [[libuiCodeModalAlertPanel alloc] initWithPanel:a parent:parent];
	[cm run];
	[cm release];
	[a release];
}

void uiMsgBox(uiWindow *parent, const char *title, const char *description)
{
	msgbox(windowWindow(parent), title, description, NSAlertStyleInformational);
}

void uiMsgBoxError(uiWindow *parent, const char *title, const char *description)
{
	msgbox(windowWindow(parent), title, description, NSAlertStyleCritical);
}
