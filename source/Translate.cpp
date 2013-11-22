// © 1998 Sum Software

#include "BecassoAddOn.h"
#include "AddOnWindow.h"
#include "AddOnSupport.h"
#include "Slider.h"

#define MAXAMOUNT 100

class TranslateWindow : public AddOnWindow
{
public:
			 TranslateWindow (BRect rect, becasso_addon_info *info) : AddOnWindow (rect, info)
			 {
			 	fX = 0;
			 	fY = 0;
			 };
virtual		~TranslateWindow () {};
virtual void MessageReceived (BMessage *msg);
float		 xAmount () { return (fX); };
float		 yAmount () { return (fY); };

private:
typedef AddOnWindow inherited;
float		 fX;
float		 fY;
};

void TranslateWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
	case 'trlX':
		fX = msg->FindFloat ("value");
		break;
	case 'trlY':
		fY = msg->FindFloat ("value");
		break;
	default:
		inherited::MessageReceived (msg);
		return;
	}
	aPreview();
}

TranslateWindow	*window;

int addon_init (uint32 index, becasso_addon_info *info)
{
	strcpy (info->name, "Translate");
	strcpy (info->author, "Sander Stoks");
	strcpy (info->copyright, "© 1998 ∑ Sum Software");
	strcpy (info->description, "Translate a selection of the canvas");
	info->type				= BECASSO_TRANSFORMER;
	info->index				= index;
	info->version			= 0;
	info->release			= 1;
	info->becasso_version	= 1;
	info->becasso_release	= 0;
	info->does_preview		= PREVIEW_FULLSCALE;
	window = new TranslateWindow (BRect (100, 180, 100 + 188, 180 + 116), info);
	Slider *xSlid = new Slider (BRect (8, 8, 180, 24), 12, "x", -MAXAMOUNT, MAXAMOUNT, 1, new BMessage ('trlX'));
	Slider *ySlid = new Slider (BRect (8, 28, 180, 44), 12, "y", -MAXAMOUNT, MAXAMOUNT, 1, new BMessage ('trlY'));
	window->Background()->AddChild (xSlid);
	window->Background()->AddChild (ySlid);
	xSlid->SetValue (0);
	ySlid->SetValue (0);
	window->Run();
	return (0);
}

int addon_exit (void)
{
	return (0);
}

int addon_open (BWindow *client, const char *name)
{
	char title[B_FILE_NAME_LENGTH];
	sprintf (title, "Translate %s", name);
	window->Lock();
	window->SetTitle (title);
	if (window->IsHidden())
		window->aShow (client);
	else
		window->aActivate (client);
	window->Unlock();
	return (0);
}

int addon_close ()
{
	window->aClose();
	return (0);
}

int process (Layer *inLayer, Selection *inSelection, 
			 Layer **outLayer, Selection **outSelection, int32 mode,
			 BRect *frame, bool /* final */, BPoint /* point */, uint32 /* buttons */)
{
	int error = ADDON_OK;
//	BRect bounds = inLayer->Bounds();
	
//	if (*outLayer == NULL && mode== M_DRAW)
//		*outLayer = new Layer (*inLayer);

//	if (mode == M_SELECT)
//	{
//		if (inSelection)
//			*outSelection = new Selection (*inSelection);
//		else	// No Selection to translate!
//			return (error);
//	}
	
	BRect oldrect = *frame;
	BRect newrect = oldrect;
	oldrect.OffsetBy (-window->xAmount(), -window->yAmount());
	
//	if (*outLayer)
//		(*outLayer)->Lock();
//	if (*outSelection)
//		(*outSelection)->Lock();

	switch (mode)
	{
	case M_DRAW:
	{
		BView *view = new BView (inLayer->Bounds(), "tmpView", 0, 0);
		inLayer->AddChild (view);
		view->DrawBitmapAsync (inLayer, oldrect, newrect);
		view->Sync();
		inLayer->RemoveChild (view);
		delete view;
		break;
	}
	case M_SELECT:
	{
		break;
	}
	default:
		fprintf (stderr, "Translate: Unknown mode\n");
		error = 1;
	}

//	if (*outSelection)
//		(*outSelection)->Unlock();
//	if (*outLayer)
//		(*outLayer)->Unlock();

	*frame = newrect;
	return (error);
}

