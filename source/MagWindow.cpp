#include "MagWindow.h"
#include "MagView.h"
#include "Settings.h"
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>

MagWindow::MagWindow (BRect frame, const char *name, CanvasView *_myView)
: BWindow (frame, name, B_DOCUMENT_WINDOW, 0, 0)
{
	BRect viewFrame, hFrame, vFrame, menubarFrame;
	menubarFrame.Set (0, 0, 0, 0);
	menubar = new BMenuBar (menubarFrame, "Magnify Menubar");
	BPopUpMenu *zoomMenu = new BPopUpMenu ("");
	zoomMenu->AddItem (new BMenuItem ("1:2", new BMessage ('zm2')));
	zoomMenu->AddItem (new BMenuItem ("1:4", new BMessage ('zm4')));
	zoomMenu->AddItem (new BMenuItem ("1:8", new BMessage ('zm8')));
	zoomMenu->AddItem (new BMenuItem ("1:16", new BMessage ('zm16')));
	zoomMenu->FindItem('zm8')->SetMarked (true);
	menubar->AddItem (zoomMenu);
	BPopUpMenu *gridMenu = new BPopUpMenu ("");
	gridMenu->AddItem (new BMenuItem (lstring (410, "Grid off"), new BMessage ('grd0')));
	gridMenu->AddItem (new BMenuItem (lstring (411, "Grid B"), new BMessage ('grdB')));
	gridMenu->AddItem (new BMenuItem (lstring (412, "Grid W"), new BMessage ('grdW')));
	gridMenu->FindItem('grdB')->SetMarked (true);
	menubar->AddItem (gridMenu);
	editMenu = new BMenu (lstring (60, ""));
	editMenu->AddItem (new BMenuItem (lstring (61, "Undo"), new BMessage (B_UNDO), 'Z'));
	editMenu->AddItem (new BMenuItem (lstring (62, "Redo"), new BMessage ('redo'), 'Z', B_SHIFT_KEY));
	menubar->AddItem (editMenu);
	AddChild (menubar);
	menubar->ResizeToPreferred();
	menubarFrame = menubar->Frame();
	menubarheight = menubarFrame.Height();
	viewFrame.Set (0, menubarheight + 1,
		frame.Width() - B_V_SCROLL_BAR_WIDTH,
		frame.Height() - B_H_SCROLL_BAR_HEIGHT);
	magView = new MagView (viewFrame, "Magnify View", _myView);
	hFrame.Set (0, frame.Height() - B_H_SCROLL_BAR_HEIGHT + 1,
		frame.Width() - B_V_SCROLL_BAR_WIDTH + 1,
		frame.Height() + 1);
	h = new BScrollBar (hFrame, NULL, magView, 0, 0, B_HORIZONTAL);
	vFrame.Set (frame.Width() - B_V_SCROLL_BAR_WIDTH + 1,
		menubarFrame.Height(), frame.Width() + 1,
		frame.Height() - B_H_SCROLL_BAR_HEIGHT + 1);
	v = new BScrollBar (vFrame, NULL, magView, 0, 0, B_VERTICAL);
	AddChild (h);
	AddChild (v);
	magView->setScrollBars (h, v);
	AddChild (magView);
	myWindow = _myView->Window();
	SetPulseRate (50000);
}

MagWindow::~MagWindow ()
{
	RemoveChild (h);
	delete h;
	RemoveChild (v);
	delete v;
	RemoveChild (menubar);
	delete menubar;
	RemoveChild (magView);
	delete magView;
}

void MagWindow::MenusBeginning ()
{
	if (magView)
	{
		editMenu->FindItem (B_UNDO)->SetEnabled (magView->CanUndo());
		editMenu->FindItem ('redo')->SetEnabled (magView->CanRedo());		
	}
}

void MagWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
	case 'zm2':
		magView->setzoom (2);
		break;
	case 'zm4':
		magView->setzoom (4);
		break;
	case 'zm8':
		magView->setzoom (8);
		break;
	case 'zm16':
		magView->setzoom (16);
		break;
	case 'grd0':
		magView->setgrid (0);
		break;
	case 'grdB':
		magView->setgrid (1);
		break;
	case 'grdW':
		magView->setgrid (2);
		break;
	case 'draw':
		magView->Invalidate ();
		break;
	case 'magO':
		Show ();
		break;
	case B_UNDO:
		magView->Undo();
		break;
	case 'redo':
		magView->Redo();
		break;
	default:
		inherited::MessageReceived (msg);
		break;
	}
}

bool MagWindow::QuitRequested ()
{
	myWindow->PostMessage ('magQ');
	return false;
}

float MagWindow::menubarHeight ()
{
	return menubarheight;
}