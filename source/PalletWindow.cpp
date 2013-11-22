#include "PalletWindow.h"

#define inherited BWindow

PalletWindow::PalletWindow(BRect frame, const char *name, int Orientation)
	: BWindow(frame, name, B_BORDERED_WINDOW, B_NOT_RESIZABLE | B_WILL_ACCEPT_FIRST_CLICK)
{
	// Need a dragbar at the top
	BRect border;

	if (Orientation == 0)
		border.Set (0, 0, frame.Width() - 1, PW_TABSIZE);
	else
		border.Set (0, 0, PW_TABSIZE, frame.Height() - 1);
	
	dv =  new DragView (border, name, Orientation);

	// Need to be increase our height to accomodate the dragbar
	if (Orientation == 0)
		ResizeBy (0, PW_TABSIZE);
	else
		ResizeBy (PW_TABSIZE, 0);

	AddChild(dv);
}

PalletWindow::~PalletWindow()
{
}
