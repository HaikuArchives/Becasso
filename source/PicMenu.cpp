#include "PicMenu.h"
#include "PicItem.h"
#include "PicMenuView.h"
#include "BitmapView.h"
#include "BecassoAddOn.h"	// for some defines
#include <Roster.h>
#include <Application.h>
#include <Resources.h>
#include <Bitmap.h>
#include <Screen.h>
#include <stdio.h>
#include "Settings.h"

PicMenu::PicMenu (const char *name, BView *_view, int h, int v, float s)
: BMenu (name, h*s, v*s)
{
	marked = 0;
	numitems = 0;
	parent = NULL;
	fWindow = NULL;
	view = _view;
	hs = h*s;
	vs = v*s;
	hnum = h;
	strcpy (kind, name);
}

PicMenu::~PicMenu ()
{
}

void PicMenu::setParent (PicMenuButton *pmb)
{
	parent = pmb;
	
	// Now is also a good time to put the tear-off window on screen
	// (if the app last quit that way)
	if (!fWindow)
	{
		BPoint origin;
		BRect place = Bounds();
		if (!strcmp (Name(), "modePU"))
			origin = get_window_origin (numModeTO);
		else if (!strcmp (Name(), "toolPU"))
			origin = get_window_origin (numToolTO);
		if (origin != InvalidPoint)
		{
			place.OffsetTo (origin);
			TearDone (place, true);
		}
	}
}

//void PicMenu::AddItem (PicItem *item)
//{
//	if (index < MAX_ITEMS - 1)
//		items[index++] = item;
//	else
//		items[index] = item;
//	inherited::AddItem (item);
//}
//
void PicMenu::AddItem (PicItem *item, const BRect rect)
{
	if (numitems < MAX_ITEMS - 1)
		items[numitems++] = item;
	else
		items[numitems] = item;
	inherited::AddItem (item, rect);
}

void PicMenu::Mark (int _index)
{
	items[marked]->SetMarked (false);
	marked = _index;
	items[marked]->SetMarked (true);
}

BPoint PicMenu::ScreenLocation ()
{
	BScreen screen;
	BPoint currentpoint = BPoint (0, 0);
	view->ConvertToScreen (&currentpoint);
	currentpoint.y = min_c (currentpoint.y, screen.Frame().bottom - vs - 4);
	currentpoint.x = min_c (currentpoint.x, screen.Frame().right - hs - 4);
	return (currentpoint);
}

#define WINVERSION

#if defined (WINVERSION)

class picTearInfo 
{
public:
	picTearInfo (BRect r, PicMenu *p, BView *s) : dragRect (r), parent (p), someView (s) {};
	BRect dragRect;
	PicMenu *parent;
	BView *someView;
};

int32 pic_tear_drag (void *data);

int32 pic_tear_drag (void *data)
{
	picTearInfo *tearInfo = (picTearInfo *) data;
	uint32 buttons = 1;
	BPoint point;
	PicMenu *menu = tearInfo->parent;
	BRect place = tearInfo->dragRect;
	BView *view = tearInfo->someView;
	// It might seem a good idea to use `menu' as the view, but
	// this gave errors: `Method requires owner but doesn't have one'.
	delete tearInfo;	// The caller doesn't do this (race condition otherwise)

//	printf ("Entering loop\n");
	while (buttons)
	{
		view->Window()->Lock();
		view->GetMouse (&point, &buttons);
		view->Window()->Unlock();
		snooze (50000);
	}
//	printf ("Exited loop\n");
	view->Window()->Lock();
	point = view->ConvertToScreen (point);
	view->Window()->Unlock();

	// printf ("Released at (%.0f, %.0f)\n", point.x, point.y);
	place.OffsetTo (point);
	menu->getParent()->lock->Lock();
	menu->TearDone (place, !menu->getParent()->MenuWinOnScreen);
	menu->getParent()->lock->Unlock();

	return B_NO_ERROR;
}

#endif

void PicMenu::MouseMoved (BPoint point, uint32 transit, const BMessage *msg)
{
	msg = msg;
	uint32 buttons;
	if (!Parent())
		return;
	GetMouse (&point, &buttons);
	if (transit == B_EXITED_VIEW && buttons)	// Do the tear off thing!
	{
#if defined (EASTER_EGG_SFX)
		extern bool EasterEgg;
		if (modifiers() & B_SHIFT_KEY && EasterEgg)
		{
			extern EffectsPlayer *easterEgg;
			easterEgg->StartEffect();
		}
#endif
		BMessage *tearmsg = new BMessage ('tear');
		BBitmap *dragmap = new BBitmap (Bounds(), B_RGBA32, true);
		dragmap->Lock();
		BView *dragview = new BView (Bounds(), "temp dragmap view", B_FOLLOW_ALL, B_WILL_DRAW);
		dragmap->AddChild (dragview);
		dragview->SetLowColor (LightGrey);
		dragview->FillRect (Bounds(), B_SOLID_LOW);
		for (int i = 0; i < numitems; i++)
		{
			BPoint point;
			if (hnum < 2)
			{
				point.x = 1;
				point.y = i*(Bounds().Width() + 1) + 1;
			}
			else
			{
				// only works for 2 wide...
				point.x = i & 1 ? Bounds().Width()/hnum + 1 : 1;
				point.y = (i/2)*(Bounds().Width()/hnum) + 1;
			}	
			dragview->DrawPicture (items[i]->getPicture(), point);
			dragview->Sync();
		}
		dragview->SetHighColor (DarkGrey);
		dragview->StrokeRect (Bounds());
		dragmap->RemoveChild (dragview);
		bgra_pixel *bits = (bgra_pixel *) dragmap->Bits();
		for (bgra_pixel p = 0; p < dragmap->BitsLength()/4; p++)
		{
			bgra_pixel pixel = *bits;
			*bits++ = (pixel & COLOR_MASK) | (127 << ALPHA_BPOS);
		}
		dragmap->Unlock();
		delete dragview;
		DragMessage (tearmsg, dragmap, B_OP_ALPHA, B_ORIGIN);
		delete tearmsg;
		BRect place = Bounds();
		
		// Send a fake Esc keydown to the popup
		char kbuf[2];
		BMessage kmsg (B_KEY_DOWN);
		kmsg.AddInt64 ("when", system_time());
		kmsg.AddInt32 ("modifiers", 0);
		kmsg.AddInt32 ("key", B_ESCAPE);
		kmsg.AddInt8 ("byte", B_ESCAPE);
		kbuf[0] = B_ESCAPE;
		kbuf[1] = '\0';
		kmsg.AddString ("bytes", kbuf);
		Window()->PostMessage (&kmsg, this);
		// This makes the original popup go away.
		// We can't use Hide() since that crashes.
		
		picTearInfo *tearInfo = new picTearInfo (place, this, parent);
		resume_thread (spawn_thread (pic_tear_drag, "Menu Tear Thread", B_NORMAL_PRIORITY, tearInfo));
	}
}

void PicMenu::TearDone (BRect place, bool newwin)
{
	if (newwin)
	{
		fWindow = new DragWindow (Name(), place, MENU_WIN_TITLE);
		BRect mvRect = Bounds();
		// mvRect.OffsetBy (0, PW_TABSIZE + 1);
		PicMenuView *mv = new PicMenuView (mvRect, hnum, "MenuView", parent);
		fWindow->AddChild (mv);
		fWindow->Show();
	}
	else
	{
		fWindow->MoveTo (place.LeftTop());
		fWindow->Activate();
	}
}

void PicMenu::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
	case 'tear':
	{
		break;
	}
	default:
		// msg->PrintToStream();
		inherited::MessageReceived (msg);
	}
}

PicItem *PicMenu::FindMarked ()
{
	return items[marked]; //IndexOf (inherited::FindMarked())];
}

void PicMenu::InvalidateWindow ()
{
	if (fWindow)
	{
		fWindow->Lock();
		fWindow->ChildAt(0)->Invalidate();
		fWindow->UpdateIfNeeded();
		fWindow->Unlock();
	}
}