#include "ColorMenu.h"
#include "ColorItem.h"
#include "Colors.h"
#include "ColorMenuView.h"
#include "BecassoAddOn.h" // for some defines
#include <InterfaceDefs.h>
#include <Screen.h>
#include <Application.h>
#include <Resources.h>
#include <Bitmap.h>
#include <stdio.h>
#include "Settings.h"

ColorMenu::ColorMenu(const char* name, BView* _view, int h, int v, float s)
	: BMenu(name, h * s, v * s)
{
	index = 0;
	for (int i = 0; i < v; i++)
		for (int j = 0; j < h; j++) {
			BRect cframe = BRect(j * s, i * s, (j + 1) * s, (i + 1) * s);
			AddItem(new ColorItem(system_colors()->color_list[index]), cframe);
		}
	Mark(0);
	view = _view;
	hs = h * s;
	vs = v * s;
	parent = NULL;
	fWindow = NULL;
}

ColorMenu::~ColorMenu() {}

void
ColorMenu::setParent(ColorMenuButton* pmb)
{
	parent = pmb;

	// Now is also a good time to put the tear-off window on screen
	// (if the app last quit that way)
	if (!fWindow) {
		BPoint origin;
		BRect place = Bounds();
		if (!strcmp(Name(), "fg"))
			origin = get_window_origin(numFGColorTO);
		else if (!strcmp(Name(), "bg"))
			origin = get_window_origin(numBGColorTO);
		if (origin != InvalidPoint) {
			place.OffsetTo(origin);
			TearDone(place, true);
		}
	}
}

void
ColorMenu::AddItem(ColorItem* item, BRect frame)
{
	if (index < MAX_COLORS - 1)
		items[index++] = item;
	else
		items[index] = item;
	inherited::AddItem(item, frame);
}

ColorItem*
ColorMenu::FindMarked()
{
	return (items[index]);
}

class colorTearInfo {
  public:
	colorTearInfo(BRect r, ColorMenu* p, BView* s) : dragRect(r), parent(p), someView(s){};
	BRect dragRect;
	ColorMenu* parent;
	BView* someView;
};

int32
color_tear_drag(void* data);

int32
color_tear_drag(void* data)
{
	colorTearInfo* tearInfo = (colorTearInfo*)data;
	uint32 buttons = 1;
	BPoint point;
	ColorMenu* menu = tearInfo->parent;
	BRect place = tearInfo->dragRect;
	BView* view = tearInfo->someView;
	// It might seem a good idea to use `menu' as the view, but
	// this gave errors: `Method requires owner but doesn't have one'.
	delete tearInfo; // The caller doesn't do this (race condition otherwise)

	// printf ("Entering loop; view = %p\n", view);
	while (buttons) {
		view->Window()->Lock();
		view->GetMouse(&point, &buttons);
		view->Window()->Unlock();
		snooze(50000);
	}
	// printf ("Exited loop\n");
	view->Window()->Lock();
	point = view->ConvertToScreen(point);
	view->Window()->Unlock();

	// printf ("Released at (%.0f, %.0f)\n", point.x, point.y);
	place.OffsetTo(point);

	menu->getParent()->lock->Lock();
	menu->TearDone(place, !menu->getParent()->MenuWinOnScreen);
	menu->getParent()->lock->Unlock();

	return B_NO_ERROR;
}

void
ColorMenu::MouseMoved(BPoint point, uint32 transit, const BMessage* msg)
{
	msg = msg;
	uint32 buttons;
	if (!Parent())
		return;
	//	printf ("("); fflush (stdout);
	GetMouse(&point, &buttons);
	//	printf (")"); fflush (stdout);
	if (transit == B_EXITED_VIEW && buttons) // Do the tear off thing!
	{
#if defined(EASTER_EGG_SFX)
		extern bool EasterEgg;
		if (modifiers() & B_SHIFT_KEY && EasterEgg) {
			extern EffectsPlayer* easterEgg;
			easterEgg->StartEffect();
		}
#endif
		BMessage* tearmsg = new BMessage('tear');
		BBitmap* dragmap = new BBitmap(Bounds(), B_RGBA32, true);
		dragmap->Lock();
		BView* dragview = new BView(Bounds(), "temp dragmap view", B_FOLLOW_ALL, B_WILL_DRAW);
		dragmap->AddChild(dragview);
		// dragview->SetLowColor (LightGrey);
		// dragview->FillRect (Bounds(), B_SOLID_LOW);
		for (int i = 0; i < MAX_COLORS; i++) {
			dragview->SetHighColor(items[i]->getColor());
			dragview->FillRect(items[i]->Frame());
			dragview->Sync();
		}
		dragview->SetHighColor(DarkGrey);
		dragview->StrokeRect(Bounds());
		dragmap->RemoveChild(dragview);
		bgra_pixel* bits = (bgra_pixel*)dragmap->Bits();
		for (bgra_pixel p = 0; p < dragmap->BitsLength() / 4; p++) {
			bgra_pixel pixel = *bits;
			*bits++ = (pixel & COLOR_MASK) | (127 << ALPHA_BPOS);
		}
		dragmap->Unlock();
		delete dragview;
		DragMessage(tearmsg, dragmap, B_OP_ALPHA, B_ORIGIN);
		delete tearmsg;
		BRect place = Bounds();

		// Send a fake Esc keydown to the popup
		char kbuf[2];
		BMessage kmsg(B_KEY_DOWN);
		kmsg.AddInt64("when", system_time());
		kmsg.AddInt32("modifiers", 0);
		kmsg.AddInt32("key", B_ESCAPE);
		kmsg.AddInt8("byte", B_ESCAPE);
		kbuf[0] = B_ESCAPE;
		kbuf[1] = '\0';
		kmsg.AddString("bytes", kbuf);
		Window()->PostMessage(&kmsg, this);
		// This makes the original popup go away.
		// We can't use Hide() since that crashes.

		colorTearInfo* tearInfo = new colorTearInfo(place, this, parent);
		resume_thread(spawn_thread(color_tear_drag, "Menu Tear Thread", B_NORMAL_PRIORITY, tearInfo)
		);
	}
}

void
ColorMenu::TearDone(BRect place, bool newwin)
{
	// printf ("CM::TearDone\n");
	if (newwin) {
		fWindow = new DragWindow(Name(), place, parent->Name());
		BRect mvRect = Bounds();
		// mvRect.OffsetBy (0, PW_TABSIZE + 1);
		ColorMenuView* mv = new ColorMenuView(mvRect, "MenuView", parent);
		fWindow->AddChild(mv);
		fWindow->Show();
	}
	else {
		fWindow->MoveTo(place.LeftTop());
		fWindow->Activate();
	}
}

void
ColorMenu::Mark(int _index)
{
	if (_index >= MAX_COLORS)
		return;

	items[index]->SetMarked(false);
	index = _index;
	items[index]->SetMarked(true);
}

BPoint
ColorMenu::ScreenLocation()
{
	BScreen screen;
	BPoint currentpoint = BPoint(0, 0);
	view->Window()->Lock();
	view->ConvertToScreen(&currentpoint);
	view->Window()->Unlock();
	currentpoint.y = min_c(currentpoint.y, screen.Frame().bottom - vs - 4);
	currentpoint.x = min_c(currentpoint.x, screen.Frame().right - hs - 4);
	return (currentpoint);
}

void
ColorMenu::InvalidateWindow()
{
	if (fWindow) {
		fWindow->Lock();
		fWindow->ChildAt(0)->Invalidate();
		fWindow->UpdateIfNeeded();
		fWindow->Unlock();
	}
}