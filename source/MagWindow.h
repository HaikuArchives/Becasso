#ifndef _MAGWINDOW_H
#define _MAGWINDOW_H

#include <Menu.h>
#include <MenuBar.h>
#include <Message.h>
#include <ScrollBar.h>
#include <Window.h>
#include "CanvasView.h"
#include "MagView.h"

class MagWindow : public BWindow {
	friend class CanvasView;

public:
	MagWindow(BRect frame, const char* name, CanvasView* _MyView);
	virtual ~MagWindow();
	virtual void MessageReceived(BMessage* msg);
	virtual bool QuitRequested();
	virtual void MenusBeginning();

	float menubarHeight();

private:
	typedef BWindow inherited;
	MagView* magView;
	BScrollBar *h, *v;
	BMenuBar* menubar;
	BMenu* editMenu;
	BWindow* myWindow;
	float menubarheight;
};

#endif
