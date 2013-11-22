#include "PicMenuButton.h"
#include <MenuItem.h>
#include <Point.h>
#include <Message.h>
#include <string.h>
#include "PicItem.h"
#include "MainWindow.h"		// Hack alert!
#include "AttribView.h"
#include "AttribWindow.h"
#include "Colors.h"
#include "Settings.h"

#define OPEN_RAD 4

PicMenuButton::PicMenuButton (BRect frame, const char *name, BPicture *p)
: BPictureButton (frame, name, p, p, NULL)
{
	SetViewColor (B_TRANSPARENT_32_BIT);
	index = 0;
	strcpy (_name, name);
	get_click_speed (&dcspeed);
	click = 1;
	lock = new BLocker();
	MenuWinOnScreen = false;
	menu = 0;
//	printf ("PicMenuButton::ctor done\n");
}

PicMenuButton::~PicMenuButton ()
{
	delete menu;
	delete lock;
}

void PicMenuButton::AddItem (PicMenu *_m)
{
	menu = _m;
	menu->setParent (this);
}

void PicMenuButton::Draw (BRect update)
{
//	printf ("PicMenuButton::Draw\n");
	update = Bounds();
	SetLowColor (LightGrey);
	FillRect (update, B_SOLID_LOW);
	inherited::Draw (update);
	SetHighColor (Grey31);
	StrokeLine (update.LeftBottom(), 
		BPoint (update.left, update.top));
	StrokeLine (BPoint (update.right - 1, update.top));
	SetHighColor (DarkGrey);
	StrokeLine (BPoint (update.right - 1, update.bottom));
	SetHighColor (Grey23);
	StrokeLine (update.LeftBottom());
}

void PicMenuButton::MouseDown (BPoint point)
{
	Window()->Lock();
	uint32 buttons = Window()->CurrentMessage()->FindInt32 ("buttons");
//	uint32 clicks = Window()->CurrentMessage()->FindInt32 ("clicks");
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
			BMenuItem *mselected;
			point = BPoint (0, -index*(BUTTON_SIZE + 3));	// Hack!
			ConvertToScreen (&point);
			BRect openRect = BRect (point.x - OPEN_RAD, point.y - OPEN_RAD,
				point.x + OPEN_RAD, point.y + OPEN_RAD);
			menu->Show();
			if ((mselected = menu->Track (true, &openRect)) != NULL)
			{
				index = menu->IndexOf (mselected);
				set (index);
				// menu->Mark (index);
				if (MenuWinOnScreen)
					menu->InvalidateWindow();

//				mselected->SetMarked (true);
//				SetEnabledOn (menu->FindMarked()->getPicture());
//				SetEnabledOff (menu->FindMarked()->getPicture());
//				index = menu->IndexOf (mselected);
//				AttribWindow *aWindow = menu->FindMarked()->getMyWindow();
//				aWindow->Lock();
//				aWindow->RaiseView (index);
//				aWindow->Unlock();
//				if (MenuWinOnScreen)
//				{
//					menu->InvalidateWindow();
//				}
			}
			menu->Hide();
			Invalidate();
			click = 1;
		}
	}
	if (click == 2 || buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY)
	{
		click = 1;
		AttribWindow *aWindow = menu->FindMarked()->getMyWindow();
		aWindow->Lock();
		if (aWindow->IsHidden())
		{
			aWindow->Show();
		}
		else
		{
			aWindow->Activate();
		}
		aWindow->Unlock();
	}
	const char *ht = menu->FindMarked()->helptext();
	char *helpstring = new char [strlen (ht) + strlen (_name) + 2];
	strcpy (helpstring, menu->FindMarked()->helptext());
	strcat (helpstring, " ");
	strcat (helpstring, _name);
	BMessage hlp ('chlp');
	hlp.AddString ("View", helpstring);
	Window()->PostMessage (&hlp);
	delete [] helpstring;
	Window()->Unlock();
}

void PicMenuButton::MouseMoved (BPoint point, uint32 transit, const BMessage *msg)
{
	point = point;
	msg = msg;
	if (transit == B_ENTERED_VIEW)
	{
		char helpstring[2*MAX_HLPNAME];
		strcpy (helpstring, menu->FindMarked()->helptext());
		strcat (helpstring, " ");
		strcat (helpstring, _name);
		BMessage *hlp = new BMessage ('chlp');
		hlp->AddString ("View", helpstring);
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

void PicMenuButton::MessageReceived (BMessage *msg)
{
//	msg->PrintToStream();
	inherited::MessageReceived (msg);
}

int32 PicMenuButton::selected ()
{
	return (index);
}

void PicMenuButton::set (int32 ind)
{
	index = ind;
	menu->Mark (index);
	BPicture *pic = menu->FindMarked()->getPicture();
	SetEnabledOff (pic);
	SetEnabledOn (pic);
	AttribWindow *aWindow = menu->FindMarked()->getMyWindow();
	aWindow->Lock();
	aWindow->RaiseView (index);
	aWindow->Unlock();
	Window()->Lock();
	Invalidate();
	Window()->Unlock();
	if (MenuWinOnScreen)
		menu->InvalidateWindow();
}