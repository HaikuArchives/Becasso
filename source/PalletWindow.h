#ifndef _PALLET_WINDOW
#define _PALLET_WINDOW

#ifndef _WINDOW_H
#define _WINDOW_H
#include <Window.h>
#endif

#ifndef _DRAGVIEW_H
#define _DRAGVIEW_H
#include "DragView.h"
#endif

#define PW_TABSIZE 16

class PalletWindow : public BWindow {
public:
	PalletWindow(BRect Frame, const char* name = "PalletWindow", int Orientation = 0);
	virtual ~PalletWindow();
	DragView* dv;
	/*
	virtual	void			Quit();
	virtual void			ScreenChanged(BRect screen_size, color_space depth);
	virtual	void			WindowActivated(bool state);
	virtual	void			Show();
	virtual	void			Hide();
	*/
};

#endif