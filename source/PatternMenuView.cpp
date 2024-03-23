#include "PatternMenuView.h"
#include "PatternMenuButton.h"
#include "PatternMenu.h"
#include "ColorMenuButton.h"

// #include "PatternWindow.h"

PatternMenuView::PatternMenuView(BRect frame, const char* name, PatternMenuButton* cmb)
	: BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	fPMB = cmb;
	cmb->lock->Lock();
	cmb->MenuWinOnScreen = true;
	cmb->lock->Unlock();
	index = -1;
	get_click_speed(&dcspeed);
	click = 1;
}

PatternMenuView::~PatternMenuView()
{
	fPMB->lock->Lock();
	fPMB->MenuWinOnScreen = false;
	fPMB->lock->Unlock();
}

void
PatternMenuView::Draw(BRect /* updateRect */)
{
	if (fPMB) {
		PatternMenu* pm = fPMB->getMenu();
		int numitems = pm->CountItems();
		// printf ("numitems = %i, index = %i\n", numitems, index);
		extern ColorMenuButton *locolor, *hicolor;
		SetLowColor(locolor->color());
		for (int i = 0; i < numitems; i++) {
			PatternItem* pmi = pm->ItemAt(i);
			BRect frame = pmi->Frame();
			SetHighColor(hicolor->color());
			frame.left++;
			frame.top++;
			FillRect(frame, pmi->getPattern());
			frame.left--;
			frame.top--;
			SetHighColor(Black);
			StrokeRect(frame);
			if (pmi->IsMarked() || index == i) {
				frame.right--;
				frame.bottom--;
				SetHighColor(White);
				StrokeRect(frame);
				frame.InsetBy(1, 1);
				SetHighColor(Black);
				StrokeRect(frame);
			}
		}
		Sync();
	} else {
	}
}

void
PatternMenuView::MouseDown(BPoint point)
{
	Window()->Lock();
	uint32 buttons = Window()->CurrentMessage()->FindInt32("buttons");
	//	uint32 clicks = Window()->CurrentMessage()->FindInt32 ("clicks");
	if (click != 2 && buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY)) {
		index = int(point.y / P_SIZE) * P_H_NUM + int(point.x / P_SIZE);
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
		index = int(point.y / P_SIZE) * P_H_NUM + int(point.x / P_SIZE);
		fPMB->set(index);
		//		PatternMenu *menu = fPMB->getMenu();
		click = 1;
		if (fPMB->IsEditorShowing()) {
			//			rgb_color c = fPMB->ColorForIndex (index);
			//			BMessage *msg = new BMessage ('SetC');
			//			msg->AddInt32 ("color", c.red);
			//			msg->AddInt32 ("color", c.green);
			//			msg->AddInt32 ("color", c.blue);
			//			fPMB->Editor()->PostMessage (msg);
			//			delete msg;
		} else {
			fPMB->ShowEditor();
			Invalidate();
		}
	}
	Window()->Unlock();
}

BRect
PatternMenuView::RectForIndex(int ind)
{
	BRect rect = BRect(0, 0, P_SIZE - 1, P_SIZE - 1);
	rect.OffsetBy(int(ind % P_H_NUM) * P_SIZE, int(ind / P_H_NUM) * P_SIZE);
	return (rect);
}

void
PatternMenuView::MouseMoved(BPoint point, uint32 transit, const BMessage* /* msg */)
{
	int previndex = index;
	if (transit == B_EXITED_VIEW) {
		index = -1;
	} else {
		index = int(point.y / P_SIZE) * P_H_NUM + int(point.x / P_SIZE);
	}
	if (previndex != index) {
		if (previndex != -1)
			Invalidate(RectForIndex(previndex));
		if (index != -1)
			Invalidate(RectForIndex(index));
	}
}