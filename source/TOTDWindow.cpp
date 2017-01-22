#include "TOTDWindow.h"
#include "Colors.h"
#include "BitmapView.h"
#include "Settings.h"
#include <string.h>
#include <View.h>
#include <CheckBox.h>
#include <Button.h>
#include <Bitmap.h>
#include <Resources.h>
#include <File.h>
#include <Roster.h>
#include <Application.h>
#include <Locker.h>
#include <TextView.h>

TOTDWindow::TOTDWindow (const BRect frame, const int num)
: BWindow (frame, lstring (500, "Tip of the day"), B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	fTotd = num;

	app_info info;
	be_app->GetAppInfo (&info);
	BFile file (&info.ref, O_RDONLY);
	BResources res (&file);
	BBitmap *icon = new BBitmap (BRect (0, 0, 31, 31), B_CMAP8);
	size_t size;
	const void *icondata = res.LoadResource ('ICON', "BEOS:L:TOTD", &size);
	icon->SetBits (icondata, 1024, 0, B_COLOR_8_BIT);
	BitmapView *iview = new BitmapView (BRect (0, 0, 39, Bounds().Height()), "icon", icon, B_OP_OVER, false);
	iview->SetViewColor (DarkGrey);
	iview->SetPosition (BPoint (4, 8));
	AddChild (iview);
	
	BRect rest = Bounds();
	rest.left = 40;
	
	BView *v = new BView (rest, "totd", B_FOLLOW_ALL, 0);
	v->SetViewColor (LightGrey);
	AddChild (v);
	rest.OffsetTo (B_ORIGIN);
	
	BButton *next = new BButton (BRect (rest.right - 80, rest.bottom - 32, rest.right - 4, rest.bottom - 4), "next", lstring (502, "Next tip"),new BMessage ('next'));
	v->AddChild (next);
	
	BCheckBox *show = new BCheckBox (BRect (8, rest.bottom - 24, rest.right - 84, rest.bottom - 8), "show", lstring (501, "Show tips at startup"), new BMessage ('show'));
	v->AddChild (show);
	
	fTextView = new BTextView (BRect (4, 4, rest.right - 4, rest.bottom - 36), "totd", BRect (0, 0, rest.right - 8, rest.bottom - 44), B_FOLLOW_ALL, B_WILL_DRAW);
	fTextView->SetViewColor (LightGrey);
	fTextView->MakeEditable (false);
	v->AddChild (fTextView);
	
	extern becasso_settings g_settings;
	extern BLocker g_settings_lock;
	g_settings_lock.Lock();
	fTotd = g_settings.totd;
	g_settings_lock.Unlock();
	show->SetValue (fTotd);

	const char *totdText = lstring (502 + fTotd, "@@@");
	if (!strcmp (totdText, "@@@"))
	{
		g_settings_lock.Lock();
		g_settings.totd = 1;
		g_settings_lock.Unlock();
		fTotd = 1;
		fTextView->SetText (lstring (503, ""));
	}
	else
		fTextView->SetText (totdText);
}

TOTDWindow::~TOTDWindow ()
{
	BPoint origin = Frame().LeftTop();
	set_window_origin (numTOTDWindow, origin);

	extern becasso_settings g_settings;
	extern BLocker g_settings_lock;
	g_settings_lock.Lock();
	if (g_settings.totd)
		g_settings.totd = ++fTotd;
	g_settings_lock.Unlock();
}

bool TOTDWindow::QuitRequested ()
{
	return true;
}

void TOTDWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case 'show':
		{
			extern BLocker g_settings_lock;
			extern becasso_settings g_settings;
			g_settings_lock.Lock();
			if (msg->FindInt32 ("be:value"))
				g_settings.totd = fTotd;
			else
				g_settings.totd = 0;
			g_settings_lock.Unlock();
			break;
		}
		case 'next':
		{
			fTotd++;
			const char *totdText = lstring (502 + fTotd, "@@@");
			if (!strcmp (totdText, "@@@"))
			{
				fTotd = 1;
				fTextView->SetText (lstring (503, ""));
			}
			else
				fTextView->SetText (totdText);
			break;
		}
		default:
			BWindow::MessageReceived (msg);
	}
}
