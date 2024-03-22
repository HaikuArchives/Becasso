#include "ColorMenuView.h"
#include "ColorMenuButton.h"
#include "ColorMenu.h"
#include "ColorWindow.h"

ColorMenuView::ColorMenuView(BRect frame, const char* name, ColorMenuButton* cmb)
	: BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	fCMB = cmb;
	cmb->lock->Lock();
	cmb->MenuWinOnScreen = true;
	cmb->lock->Unlock();
	SetViewColor(LightGrey);
	index = -1;
	get_click_speed(&dcspeed);
	click = 1;
}

ColorMenuView::~ColorMenuView()
{
	fCMB->lock->Lock();
	fCMB->MenuWinOnScreen = false;
	fCMB->lock->Unlock();
}

void
ColorMenuView::Draw(BRect /*updateRect*/)
{
	if (fCMB) {
		ColorMenu* pm = fCMB->getMenu();
		int numitems = pm->CountItems();
		// printf ("numitems = %i, index = %i\n", numitems, index);
		for (int i = 0; i < numitems; i++) {
			ColorItem* pmi = pm->ItemAt(i);
			SetHighColor(pmi->getColor());
			BRect frame = pmi->Frame();
			frame.right -= 1;
			frame.bottom -= 1;
			FillRect(frame);
			if (pmi->IsMarked() || index == i) {
				SetHighColor(White);
				StrokeRect(frame);
				frame.InsetBy(1, 1);
				SetHighColor(Black);
				StrokeRect(frame);
			}
			Sync();
		}
	}
	else {
	}
}

void
ColorMenuView::MouseDown(BPoint point)
{
	Window()->Lock();
	uint32 buttons = Window()->CurrentMessage()->FindInt32("buttons");
	//	uint32 clicks = Window()->CurrentMessage()->FindInt32 ("clicks");
	if (click != 2 && buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY)) {
		int previndex = fCMB->selected();
		index = int(point.y / C_SIZE) * C_H_NUM + int(point.x / C_SIZE);
		fCMB->set(index, true);
		Invalidate(RectForIndex(previndex));

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
		index = int(point.y / C_SIZE) * C_H_NUM + int(point.x / C_SIZE);
		fCMB->set(index);
		//		ColorMenu *menu = fCMB->getMenu();
		click = 1;
		if (fCMB->IsEditorShowing()) {
			rgb_color c = fCMB->ColorForIndex(index);
			BMessage* msg = new BMessage('SetC');
			msg->AddInt32("color", c.red);
			msg->AddInt32("color", c.green);
			msg->AddInt32("color", c.blue);
			fCMB->Editor()->PostMessage(msg);
			delete msg;
		}
		else {
			fCMB->ShowEditor();
			Invalidate();
		}
	}
	Window()->Unlock();
}

BRect
ColorMenuView::RectForIndex(int ind)
{
	BRect rect = BRect(0, 0, C_SIZE - 1, C_SIZE - 1);
	rect.OffsetBy(int(ind % C_H_NUM) * C_SIZE, int(ind / C_H_NUM) * C_SIZE);
	return (rect);
}

void
ColorMenuView::MouseMoved(BPoint point, uint32 transit, const BMessage* /* msg */)
{
	int previndex = index;
	if (transit == B_EXITED_VIEW) {
		index = -1;
	}
	else {
		index = int(point.y / C_SIZE) * C_H_NUM + int(point.x / C_SIZE);
	}
	if (previndex != index) {
		if (previndex != -1)
			Invalidate(RectForIndex(previndex));
		if (index != -1)
			Invalidate(RectForIndex(index));
	}
}