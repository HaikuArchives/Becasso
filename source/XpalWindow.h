#ifndef _XPALWINDOW_H
#define _XPALWINDOW_H

#include <CheckBox.h>
#include <Messenger.h>
#include <Window.h>
#include "Slider.h"

class XpalWindow : public BWindow {
public:
	XpalWindow(BRect rect, const char* name, BMessenger* target);
	virtual ~XpalWindow();
	virtual void MessageReceived(BMessage* message);
	// int32		 Go ();

private:
	typedef BWindow inherited;
	int fNum;
	BMessenger* fTarget;
	BCheckBox* fClobberCB;
	int32 fStatus;
	bool fClobber;
};

#endif
