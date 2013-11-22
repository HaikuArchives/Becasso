#include "PatternMenu.h"
#include "PatternMenuButton.h"
#include "PatternItem.h"
#include "PatternMenuView.h"
#include "ColorMenuButton.h"
#include "BecassoAddOn.h"	// for some defines
#include <InterfaceDefs.h>
#include <Screen.h>
#include <Application.h>
#include <Roster.h>
#include <Resources.h>
#include <Bitmap.h>
#include "Settings.h"

PatternMenu::PatternMenu (BView *_view, int h, int v, float s)
: BMenu ("PatternMenu", h*s, v*s)
{
	pattern patterns[MAX_PATTERNS];
	uchar data[MAX_PATTERNS][8] = 
	  {	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	  	{0xFF, 0xDD, 0xFF, 0x77, 0xFF, 0xDD, 0xFF, 0x77},
		{0xFF, 0xAA, 0xFF, 0xAA, 0xFF, 0xAA, 0xFF, 0xAA},
		{0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD},
//		{0x9B, 0xB9, 0x7D, 0xD6, 0xED, 0xBB, 0x6E, 0xB7},
		{0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55},
//		{0x55, 0x55, 0xAA, 0xAA, 0x55, 0x55, 0xAA, 0xAA},
//		{0x94, 0x29, 0x82, 0x29, 0x92, 0x44, 0x91, 0x48},
		{0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC},
		{0x00, 0x55, 0x00, 0x55, 0x00, 0x55, 0x00, 0x55},
		{0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22},
		{0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00, 0x88},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},

		{0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80},
		{0x83, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0xC1},
		{0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88},
		{0xBB, 0x77, 0xEE, 0xDD, 0xBB, 0x77, 0xEE, 0xDD},
		{0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01},
		{0xC1, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x83},
		{0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11},
		{0xDD, 0xEE, 0x77, 0xBB, 0xDD, 0xEE, 0x77, 0xBB},
		{0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00},
		{0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88} };

	for (int i = 0; i < h*v; i++)
		for (int j = 0; j < 8; j++)
			patterns[i].data[j] = data[i][j];

	index = 0;
	for (int i = 0; i < v; i++)
		for (int j = 0; j < h; j++)
		{
			BRect cframe = BRect (j*s, i*s, (j + 1)*s, (i + 1)*s);
			AddItem (new PatternItem (patterns[index]), cframe);
		}
	Mark (0);
	view = _view;
	hs = h*s;
	vs = v*s;
	parent = NULL;
	fWindow = NULL;
//	app_info info;
//	be_app->GetAppInfo (&info);
}

PatternMenu::~PatternMenu ()
{
}

void PatternMenu::setParent (PatternMenuButton *pmb)
{
	parent = pmb;
	
	// Now is also a good time to put the tear-off window on screen
	// (if the app last quit that way)
	if (!fWindow)
	{
		BPoint origin;
		BRect place = Bounds();
		origin = get_window_origin (numPatternTO);
		if (origin != InvalidPoint)
		{
			place.OffsetTo (origin);
			TearDone (place, true);
		}
	}
}

void PatternMenu::AddItem (PatternItem *item, BRect frame)
{
	if (index < MAX_PATTERNS - 1)
		items[index++] = item;
	else
		items[index] = item;
	inherited::AddItem (item, frame);
}

PatternItem *PatternMenu::ItemAt (int i)
{
	return (items[i]);
}

PatternItem *PatternMenu::FindMarked ()
{
	return (items[index]);
}

class patternTearInfo 
{
public:
	patternTearInfo (BRect r, PatternMenu *p, BView *s) : dragRect (r), parent (p), someView (s) {};
	BRect dragRect;
	PatternMenu *parent;
	BView *someView;
};

int32 pattern_tear_drag (void *data);

int32 pattern_tear_drag (void *data)
{
	patternTearInfo *tearInfo = (patternTearInfo *) data;
	uint32 buttons = 1;
	BPoint point;
	PatternMenu *menu = tearInfo->parent;
	BRect place = tearInfo->dragRect;
	BView *view = tearInfo->someView;
	// It might seem a good idea to use `menu' as the view, but
	// this gave errors: `Method requires owner but doesn't have one'.
	delete tearInfo;	// The caller doesn't do this (race condition otherwise)

	// printf ("Entering loop; view = %p\n", view);
	while (buttons)
	{
		view->Window()->Lock();
		view->GetMouse (&point, &buttons);
		view->Window()->Unlock();
		snooze (50000);
	}
	// printf ("Exited loop\n");
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

void PatternMenu::MouseMoved (BPoint point, uint32 transit, const BMessage * /* msg */)
{
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
		//dragview->SetLowColor (LightGrey);
		//dragview->FillRect (Bounds(), B_SOLID_LOW);
		extern ColorMenuButton *locolor, *hicolor;
		dragview->SetLowColor (locolor->color());
		for (int i = 0; i < MAX_PATTERNS; i++)
		{
			BRect frame = items[i]->Frame();
			dragview->SetHighColor (hicolor->color());
			dragview->FillRect (frame, items[i]->getPattern());
			dragview->SetHighColor (Black);
			dragview->StrokeRect (frame);
			dragview->Sync();
		}
		dragview->SetHighColor (DarkGrey);
		dragview->StrokeRect (Bounds());
		dragmap->RemoveChild (dragview);
		dragmap->Unlock();
		bgra_pixel *bits = (bgra_pixel *) dragmap->Bits();
		for (bgra_pixel p = 0; p < dragmap->BitsLength()/4; p++)
		{
			bgra_pixel pixel = *bits;
			*bits++ = (pixel & COLOR_MASK) | (127 << ALPHA_BPOS);
		}
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
		
		patternTearInfo *tearInfo = new patternTearInfo (place, this, parent);
		resume_thread (spawn_thread (pattern_tear_drag, "Menu Tear Thread", B_NORMAL_PRIORITY, tearInfo));
	}
	//	inherited::MouseMoved (point, transit, msg);
}

void PatternMenu::TearDone (BRect place, bool newwin)
{
	if (newwin)
	{
		fWindow = new DragWindow ("pat", place, parent->Name());
		BRect mvRect = Bounds();
		// mvRect.OffsetBy (0, PW_TABSIZE + 1);
		PatternMenuView *mv = new PatternMenuView (mvRect, "MenuView", parent);
		fWindow->AddChild (mv);
		fWindow->Show();
	}
	else
	{
		fWindow->MoveTo (place.LeftTop());
		fWindow->Activate();
	}
}

void PatternMenu::Mark (int _index)
{
	items[index]->SetMarked (false);
	index = _index;
	items[index]->SetMarked (true);
}

BPoint PatternMenu::ScreenLocation ()
{
	BScreen screen;
	BPoint currentpoint = BPoint (0, 0);
	view->ConvertToScreen (&currentpoint);
	currentpoint.y = min_c (currentpoint.y, screen.Frame().bottom - vs - 4);
	currentpoint.x = min_c (currentpoint.x, screen.Frame().right - hs - 4);
	return (currentpoint);
}

void PatternMenu::InvalidateWindow ()
{
	if (fWindow)
	{
		fWindow->Lock();
		fWindow->ChildAt(0)->Invalidate();
		fWindow->UpdateIfNeeded();
		fWindow->Unlock();
	}
}
