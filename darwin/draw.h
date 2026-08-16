// 6 january 2017

// Private drawing state shared by draw.m and drawtext.m.
struct uiDrawContext {
	CGContextRef c;
	CGFloat height;				// used for text coordinate conversion in drawtext.m
};
