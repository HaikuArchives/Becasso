#include "PatternMenuButton.h"
#include "ColorMenuButton.h"
#include "PatternMenu.h"
#include "Colors.h"
#include "debug.h"
#include <Application.h>

#define OPEN_RAD 4

PatternMenuButton::PatternMenuButton (BRect frame, const char *name)
: BView (frame, name, B_FOLLOW_LEFT, B_WILL_DRAW)
{
	SetViewColor (B_TRANSPARENT_32_BIT);
	index = 0;
	strcpy (_name, name);
	menu = new PatternMenu (this, P_H_NUM, P_V_NUM, P_SIZE);
	lock = new BLocker();
	MenuWinOnScreen = false;
	editorshowing = false;
	get_click_speed (&dcspeed);
	click = 1;
	menu->setParent (this);
}

PatternMenuButton::~PatternMenuButton ()
{
	delete menu;
	delete lock;
}

void PatternMenuButton::editorSaysBye ()
{
	editorshowing = false;
}

void PatternMenuButton::ShowEditor ()
{
	// editor = new PatternWindow (BRect (), _name, this);
	editorshowing = true;
	// editor->Show();
}

void PatternMenuButton::MouseDown (BPoint point)
{
//	BMenuItem *selected;
//	point = BPoint (0, 0);
//	ConvertToScreen (&point);
//	BRect openRect = BRect (point.x - OPEN_RAD, point.y - OPEN_RAD,
//		point.x + OPEN_RAD, point.y + OPEN_RAD);
//	menu->Show();
//	if ((selected = menu->Track (true, &openRect)) != NULL)
//	{
//		index = menu->IndexOf (selected);
//		menu->Mark (index);
//		menu->InvalidateWindow();
//	}
//	menu->Hide();
//	Invalidate();
	Window()->Lock();
	uint32 buttons = Window()->CurrentMessage()->FindInt32 ("buttons");
//	uint32 clicks = Window()->CurrentMessage()->FindInt32 ("clicks");
	BMenuItem *mselected;
	point = BPoint (0, 0);
	ConvertToScreen (&point);
	if (click != 2 && buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY))
	{
		BPoint bp, pbp;
		uint32 bt;
		GetMouse (&pbp, &bt, true);
		bigtime_t start = system_time();
		while (system_time() - start < dcspeed)
		{
			snooze (20000);
			GetMouse (&bp, &bt, true);
			if (!bt && click != 2)
			{
				click = 0;
			}
			if (bt && !click)
			{
				click = 2;
			}
			if (bp != pbp)
				break;
		}
		if (click != 2)
		{
			BRect openRect = BRect (point.x - OPEN_RAD, point.y - OPEN_RAD,
				point.x + OPEN_RAD, point.y + OPEN_RAD);
			menu->Show();
			if ((mselected = menu->Track (true, &openRect)) != NULL)
			{
				index = menu->IndexOf (mselected);
				menu->Mark (index);
				if (MenuWinOnScreen)
					menu->InvalidateWindow();
			}
			menu->Hide();
			if (editorshowing)
			{
//				rgb_color c = color();
//				BMessage *msg = new BMessage ('SetC');
//				msg->AddInt32 ("color", c.red);
//				msg->AddInt32 ("color", c.green);
//				msg->AddInt32 ("color", c.blue);
//				editor->PostMessage (msg);
//				delete msg;
			}
			click = 1;
		}
	}
	if (click == 2 || buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
	{
		click = 1;
		if (editorshowing)
		{
//			rgb_color c = color();
//			BMessage *msg = new BMessage ('SetC');
//			msg->AddInt32 ("color", c.red);
//			msg->AddInt32 ("color", c.green);
//			msg->AddInt32 ("color", c.blue);
//			editor->PostMessage (msg);
//			delete msg;
		}
		else
		{
			ShowEditor();
		}
	}
	Invalidate();
	Window()->Unlock();
}

void PatternMenuButton::MessageReceived (BMessage *msg)
{
	inherited::MessageReceived (msg);
}

void PatternMenuButton::MouseMoved (BPoint point, uint32 transit, const BMessage *msg)
{
	point = point;
	msg = msg;
	if (transit == B_ENTERED_VIEW)
	{
		BMessage *hlp = new BMessage ('chlp');
		hlp->AddString ("View", _name);
		Window()->PostMessage (hlp);
		delete hlp;
	}
	else if (transit == B_EXITED_VIEW)
	{
		BMessage *hlp = new BMessage ('chlp');
		hlp->AddString ("View", "");
		Window()->PostMessage (hlp);
		delete hlp;
	}
}

void PatternMenuButton::Draw (BRect update)
{
	extern ColorMenuButton *locolor, *hicolor;
	rgb_color Hi = hicolor->color();
	rgb_color Lo = locolor->color();
	update = Bounds();
	SetHighColor (Hi);
	SetLowColor (Lo);
	FillRect (update, pat());
	SetHighColor (Grey8);
	SetDrawingMode (B_OP_ADD);
	StrokeLine (update.LeftBottom(), update.LeftTop());
	StrokeLine (update.RightTop());
	SetDrawingMode (B_OP_SUBTRACT);
	StrokeLine (update.RightBottom());
	StrokeLine (update.LeftBottom());
}

void PatternMenuButton::set (int _index)
{
	if (_index >= P_SIZE)
		return;
	
	index = _index;
	menu->Mark (index);
	if (Window())
	{
		Window()->Lock();
		Invalidate();
		Window()->Unlock();
	}
	if (MenuWinOnScreen)
		menu->InvalidateWindow();
	if (editorshowing)
	{
//		rgb_color c = color();
//		BMessage *msg = new BMessage ('SetC');
//		msg->AddInt32 ("color", c.red);
//		msg->AddInt32 ("color", c.green);
//		msg->AddInt32 ("color", c.blue);
//		msg->AddInt32 ("alpha", c.alpha);
//		editor->PostMessage (msg);
//		delete msg;
	}
	be_app->PostMessage ('clrX');
}

void PatternMenuButton::set (pattern _p)
{
	verbose (1, "PMB::set\n");
	menu->FindMarked()->setPattern (_p);
	Window()->Lock();
	Invalidate();
	Window()->Unlock();
	if (MenuWinOnScreen)
		menu->InvalidateWindow();
	if (editorshowing)
	{
//		BMessage *msg = new BMessage ('SetC');
//		msg->AddInt32 ("color", c.red);
//		msg->AddInt32 ("color", c.green);
//		msg->AddInt32 ("color", c.blue);
//		msg->AddInt32 ("alpha", c.alpha);
//		editor->PostMessage (msg);
//		delete msg;
	}
	be_app->PostMessage ('clrX');
}

int32 PatternMenuButton::selected ()
{
	return (index);
}

pattern PatternMenuButton::pat ()
{
	return (menu->FindMarked()->getPattern());
}