#ifndef DRAGWINDOW_H
#define DRAGWINDOW_H

#include <Window.h>
#include <stdio.h>
#include <string.h>
#include "Settings.h"

class DragWindow : public BWindow {
public:
	DragWindow(const char* kind, BRect frame, const char* title)
		: BWindow(frame, title, B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			  B_WILL_ACCEPT_FIRST_CLICK | B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
	// Do not add B_AVOID_FOCUS since you then can't close it using the closer widget anymore
	{
		strcpy(fKind, kind);
	}

	virtual ~DragWindow() {}

	virtual bool QuitRequested()
	{
		SavePos(false);
		return true;
	}

	void SavePos(bool v = true)
	{
		BPoint origin = Frame().LeftTop();
		if (!strcmp(fKind, "modePU"))
			set_window_origin(numModeTO, v ? origin : InvalidPoint);
		else if (!strcmp(fKind, "toolPU"))
			set_window_origin(numToolTO, v ? origin : InvalidPoint);
		else if (!strcmp(fKind, "fg"))
			set_window_origin(numFGColorTO, v ? origin : InvalidPoint);
		else if (!strcmp(fKind, "bg"))
			set_window_origin(numBGColorTO, v ? origin : InvalidPoint);
		else if (!strcmp(fKind, "pat"))
			set_window_origin(numPatternTO, v ? origin : InvalidPoint);
	}


private:
	char fKind[64];
};

#endif
