// 27 may 2016
#include "uipriv_darwin.h"

// see http://stackoverflow.com/questions/37979445/how-do-i-properly-set-up-a-scrolling-nstableview-using-auto-layout-what-ive-tr for why we don't use auto layout

NSScrollView *uiprivMkScrollView(uiprivScrollViewCreateParams *p)
{
	NSScrollView *sv;
	NSBorderType border;

	sv = [[NSScrollView alloc] initWithFrame:NSZeroRect];
	if (p->BackgroundColor != nil)
		[sv setBackgroundColor:p->BackgroundColor];
	[sv setDrawsBackground:p->DrawsBackground];
	border = NSNoBorder;
	if (p->Bordered)
		border = NSBezelBorder;
	// document view seems to set the cursor properly
	[sv setBorderType:border];
	[sv setAutohidesScrollers:YES];
	[sv setHasHorizontalRuler:NO];
	[sv setHasVerticalRuler:NO];
	[sv setRulersVisible:NO];
	[sv setScrollerKnobStyle:NSScrollerKnobStyleDefault];
	// the scroller style is documented as being set by default for us
	// LONGTERM verify line and page for programmatically created NSTableView
	[sv setScrollsDynamically:YES];
	[sv setFindBarPosition:NSScrollViewFindBarPositionAboveContent];
	[sv setUsesPredominantAxisScrolling:NO];
	[sv setHorizontalScrollElasticity:NSScrollElasticityAutomatic];
	[sv setVerticalScrollElasticity:NSScrollElasticityAutomatic];
	[sv setAllowsMagnification:NO];

	[sv setDocumentView:p->DocumentView];
	[sv setHasHorizontalScroller:p->HScroll];
	[sv setHasVerticalScroller:p->VScroll];

	return sv;
}
