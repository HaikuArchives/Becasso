// © 1998-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include <Window.h>
#include <Button.h>
#include <Application.h>
#include <string.h>

class CaptureWindow : public BWindow
{
public:
			 CaptureWindow (BRect rect) : BWindow (rect, "Capture Test", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK) {};
virtual		~CaptureWindow () {};
virtual bool QuitRequested () { Hide(); return false; };
};

CaptureWindow	*window;

status_t addon_init (uint32 index, becasso_addon_info *info)
{
	strcpy (info->name, "CaptureTest");
	strcpy (info->author, "Sander Stoks");
	strcpy (info->copyright, "© 1998-2001 ∑ Sum Software");
	strcpy (info->description, "Fake Capture add-on");
	info->type				= BECASSO_CAPTURE;
	info->index				= index;
	info->version			= 1;
	info->release			= 2;
	info->becasso_version	= 2;
	info->becasso_release	= 0;
	info->does_preview		= 0;
	info->flags				= 0;
	window = new CaptureWindow (BRect (100, 180, 100 + 188, 180 + 72));
	BView *bg = new BView (BRect (0, 0, 188, 72), "bg", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	bg->SetViewColor (LightGrey);
	window->AddChild (bg);
	BMessage *msg = new BMessage (CAPTURE_READY);
	msg->AddInt32 ("index", index);
	BButton *grab = new BButton (BRect (128, 40, 180, 64), "grab", "Grab", msg);
	grab->SetTarget (be_app);
	bg->AddChild (grab);
	window->Run();
	return (0);
}

status_t addon_exit (void)
{
	return B_OK;
}

status_t addon_open ()
{
	window->Lock();
	if (window->IsHidden())
		window->Show ();
	else
		window->Activate ();
	window->Unlock();
	return (0);
}

BBitmap *bitmap (char *title)
{
	strcpy (title, "Grabbed");
	BBitmap *b = new BBitmap (BRect (0, 0, 127, 127), B_RGB_32_BIT, true);
	BView *v = new BView (BRect (0, 0, 127, 127), "bg", 0, 0);
	b->Lock();
	b->AddChild (v);
	rgb_color fg, bg;
	fg.red = 255; fg.green = 255; fg.blue = 255; fg.alpha = 255;
	bg.red = 0;   bg.green = 0;   bg.blue = 0;   bg.alpha = 127;
	v->SetHighColor (fg);
	v->SetLowColor (bg);
	v->FillRect (BRect (0, 0, 127, 127), B_MIXED_COLORS);
	v->Sync();
	b->RemoveChild (v);
	b->Unlock();
	delete v;
	return (b);
}
