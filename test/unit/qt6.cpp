#include <cassert>
#include <ui_qt.h>

static int queued;

static void runQueued(void *)
{
	queued++;
	uiQuit();
}

int main()
{
	uiInitOptions options{};
	assert(uiInit(&options) == nullptr);

	uiButton *button = uiNewButton("Qt handle");
	auto *widget = reinterpret_cast<QWidget *>(uiControlHandle(uiControl(button)));
	assert(widget != nullptr);
	assert(widget == uiQtControlWidget(uiQtControl(button)));

	uiQueueMain(runQueued, nullptr);
	uiMain();
	assert(queued == 1);

	uiControlDestroy(uiControl(button));
	uiUninit();
	return 0;
}
