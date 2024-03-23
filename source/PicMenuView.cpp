#include "PicMenuView.h"
#include "PicMenu.h"
#include <string.h>

PicMenuView::PicMenuView(BRect frame, int _hnum, const char* name, PicMenuButton* pmb)
	: BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	fPMB = pmb;
	pmb->lock->Lock();
	pmb->MenuWinOnScreen = true;
	pmb->lock->Unlock();
	SetViewColor(LightGrey);
	index = -1;
	get_click_speed(&dcspeed);
	click = 1;
	hnum = _hnum;
}

PicMenuView::~PicMenuView()
{
	//	printf ("~MenuView\n");
	fPMB->lock->Lock();
	fPMB->MenuWinOnScreen = false;
	fPMB->lock->Unlock();
}

void
PicMenuView::Draw(BRect updateRect)
{
	updateRect = updateRect;
	if (fPMB) {
		float bw = Bounds().Width() / hnum;
		BRect thisbutton;
		PicMenu* pm = fPMB->getMenu();
		int numitems = pm->CountItems();
		// printf ("numitems = %i, index = %i\n", numitems, index);
		for (int i = 0; i < numitems; i++) {
			thisbutton.Set(
				(i % hnum) * bw, (i / hnum) * bw, (i % hnum + 1) * bw, (i / hnum + 1) * bw
			);
			SetLowColor(LightGrey);
			SetHighColor(DarkGrey);
			FillRect(thisbutton, index == i ? B_SOLID_HIGH : B_SOLID_LOW);
			PicItem* pmi = pm->ItemAt(i);
			BPicture* p = pmi->getPicture();
			DrawPicture(p, thisbutton.LeftTop());
			if (pmi->IsMarked()) {
				SetHighColor(Black);
				StrokeRect(pmi->Frame());
			}
			Sync();
		}
	} else {
	}
}

void
PicMenuView::MouseMoved(BPoint point, uint32 transit, const BMessage* msg)
{
	msg = msg;
	int previndex = index;
	float bw = Bounds().Width() / hnum + 1;
	PicMenu* menu = fPMB->getMenu();
	if (transit == B_EXITED_VIEW) {
		index = -1;
		BMessage* hlp = new BMessage('chlp');
		hlp->AddString("View", "");
		fPMB->Window()->PostMessage(hlp);
		delete hlp;
	} else {
		index = int(point.y / bw) * hnum + int(point.x / bw);
		char helpstring[2 * MAX_HLPNAME];
		strcpy(helpstring, menu->ItemAt(index)->helptext());
		strcat(helpstring, " ");
		strcat(helpstring, fPMB->Name());
		BMessage* hlp = new BMessage('chlp');
		hlp->AddString("View", helpstring);
		fPMB->Window()->PostMessage(hlp);
		delete hlp;
	}
	if (previndex != index) {
		if (previndex != -1)
			Invalidate(RectForIndex(previndex));
		if (index != -1)
			Invalidate(RectForIndex(index));
	}
}

BRect
PicMenuView::RectForIndex(int i)
{
	float bw = Bounds().Width() / hnum;
	return BRect(
		(i % hnum) * bw, (i / hnum) * bw, (i % hnum + 1) * bw - 1, (i / hnum + 1) * bw - 1
	);
}

void
PicMenuView::MouseDown(BPoint point)
{
	Window()->Lock();
	uint32 buttons = Window()->CurrentMessage()->FindInt32("buttons");
	//	uint32 clicks = Window()->CurrentMessage()->FindInt32 ("clicks");
	if (click != 2 && buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY)) {
		float bw = Bounds().Width() / hnum + 1;
		index = int(point.y / bw) * hnum + int(point.x / bw);
		fPMB->set(index);

		BPoint bp, pbp;
		uint32 bt;
		GetMouse(&pbp, &bt, true);
		bigtime_t start = system_time();
		while (system_time() - start < dcspeed) {
			snooze(20000);
			GetMouse(&bp, &bt, true);
			if (!bt && click != 2) {
				click = 0;
			}
			if (bt && !click) {
				click = 2;
			}
			if (bp != pbp)
				break;
		}
		if (click != 2) {
			click = 1;
		}
	}
	if (click == 2 || buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY) {
		float bw = Bounds().Width() / hnum + 1;
		index = int(point.y / bw) * hnum + int(point.x / bw);
		fPMB->set(index);
		click = 1;
		PicMenu* menu = fPMB->getMenu();
		AttribWindow* aWindow = menu->FindMarked()->getMyWindow();
		aWindow->Lock();
		if (aWindow->IsHidden()) {
			aWindow->Show();
		} else {
			aWindow->Activate();
		}
		aWindow->Unlock();
	}
	Window()->Unlock();
}