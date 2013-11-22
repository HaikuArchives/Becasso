#include <Alert.h>
#include "AddOnWindow.h"
#include "CanvasWindow.h"
#include "Colors.h"
#include <string.h>
#include "Settings.h"
#include "AddOn.h"

AddOnWindow::AddOnWindow (BRect frame)
: BWindow (frame, "Unopened", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK)
{
	BRect mybounds = Bounds();
	bg = new BView (mybounds, "addonbg", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	bg->SetViewColor (LightGrey);
	AddChild (bg);
	fStatusBar = new BStatusBar (BRect (mybounds.left + 8, mybounds.bottom - 68, mybounds.right - 8, mybounds.bottom - 60), "AddonStatusBar");
	fStatusBar->SetResizingMode (B_FOLLOW_BOTTOM);
	bg->AddChild (fStatusBar);
	fStatusBar->SetBarHeight (8);
	fStop = new BButton (BRect (mybounds.right - 180, mybounds.bottom - 32, mybounds.right - 128, mybounds.bottom - 8), "AddonStop", lstring (370, "Stop"), new BMessage ('ao_s'), B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	fStop->SetEnabled (false);
	bg->AddChild (fStop);
	fInfo = new BButton (BRect (mybounds.right - 120, mybounds.bottom - 32, mybounds.right - 68, mybounds.bottom - 8), "AddonInfo", lstring (371, "Info"), new BMessage ('ao_i'), B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	bg->AddChild (fInfo);
	fApply = new BButton (BRect (mybounds.right - 60, mybounds.bottom - 32, mybounds.right - 8, mybounds.bottom - 8), "AddonApply", lstring (373, "Apply"), 0, B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	bg->AddChild (fApply);
	fApply->MakeDefault (true);
	fClient = 0;
}

void AddOnWindow::SetAddOn (becasso_addon_info *info)
{
	SetName (info->name);
	BMessage *msg = NULL;
	switch (info->type)
	{
	case BECASSO_FILTER:
		msg = new BMessage (ADDON_FILTER);
		break;
	case BECASSO_TRANSFORMER:
		msg = new BMessage (ADDON_TRANSFORMER);
		break;
	case BECASSO_GENERATOR:
		msg = new BMessage (ADDON_GENERATOR);
		break;
	default:
		fprintf (stderr, "AddOnWindow: Unknown Add-On Type!\n");
		throw (0);
	}
	msg->AddInt32 ("index", info->index);
	fApply->SetMessage (msg);
	memcpy (&fInfoStruct, info, sizeof (becasso_addon_info));
	stop = false;
}

AddOnWindow::~AddOnWindow ()
{
}

bool AddOnWindow::QuitRequested ()
{
//	printf ("AddOnWindow::QuitRequested ()\n");
	SetClient (NULL);	// This sends a message to the previous client;
						// that'll notify the AddOn.
	return false;
}

void AddOnWindow::Start ()
{
	ResetStatusBar();
	fStop->SetEnabled (true);
}

void AddOnWindow::Stopped ()
{
	ResetStatusBar();
	fStop->SetEnabled (false);
	stop = false;
}

void AddOnWindow::UpdateStatusBar (float delta, const char *text, const char *trailingText)
{
	fStatusBar->Update (delta, text, trailingText);
	UpdateIfNeeded();
}

void AddOnWindow::ResetStatusBar (const char *label, const char *trailingLabel)
{
	fStatusBar->Reset (label, trailingLabel);
	UpdateIfNeeded();
}

void AddOnWindow::aPreview ()
{
	BMessage *msg = new BMessage (ADDON_PREVIEW);
	msg->AddInt32 ("type", fInfoStruct.type);
	msg->AddInt32 ("index", fInfoStruct.index);	
	fClient->Lock();
	fClient->PostMessage (msg);
	fClient->Unlock();
	delete msg;
}

void AddOnWindow::SetClient (BWindow *client)
{
//	printf ("AddOnWindow::SetClient\n");
	CanvasWindow *old_win = dynamic_cast <CanvasWindow *> (fClient);

	if (old_win && old_win != client && !(old_win->IsQuitting()))
	{
//		printf ("Sending Bye message...\n");
		BMessenger tmpmessenger (fClient);
		BMessage *bye = new BMessage ('adcl');
		BMessage reply;
		bye->AddInt32 ("index", fInfoStruct.index);
		tmpmessenger.SendMessage (bye, &reply, 100000, 100000);
		delete bye;
	}
	fClient = client;
	
	CanvasWindow *new_win = dynamic_cast <CanvasWindow *> (fClient);
//	printf ("client = %p, old_win = %p, new_win = %p\n", client, old_win, new_win);
	
	if (new_win)
	{
		char name[256];
		sprintf (name, "%s %s", fInfoStruct.name, new_win->CanvasName());
		SetTitle (name);
	}
	fApply->SetTarget (client);
}

void AddOnWindow::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
	case 'ao_i':
	{
		char infostring[1024];
		sprintf (infostring, "%s ", fInfoStruct.name);
		switch (fInfoStruct.type)
		{
		case BECASSO_FILTER:
			strcat (infostring, "Filter ");
			break;
		case BECASSO_TRANSFORMER:
			strcat (infostring, "Transformer ");
			break;
		case BECASSO_GENERATOR:
			strcat (infostring, "Generator ");
			break;
		default:
			fprintf (stderr, "AddOnWindow: Unknown Add-On Type!\n");
		}
		char version[16];
		sprintf (version, "v%i.%i\n", fInfoStruct.version, fInfoStruct.release);
		strcat (infostring, version);
		strcat (infostring, fInfoStruct.author);
		strcat (infostring, "\n");
		strcat (infostring, fInfoStruct.copyright);
		strcat (infostring, "\n");
		strcat (infostring, fInfoStruct.description);
		BAlert *infoBox = new BAlert ("", infostring, "OK");
		infoBox->Go();
		break;
	}
	case 'ao_s':
		stop = true;
		break;
	case 'cack':	// Closing is acknowledged by the client.
		printf ("cack\n");
		break;
	case ADDON_RESIZED:
	{
		// Note: see AddOn::Open()
		BView *config = FindView ("config view");
		if (config)
		{
			BRect bounds = config->Bounds();
			if (bounds.right < 188)
				bounds.right = 188;
			bounds.bottom += 64;
			ResizeTo (bounds.Width(), bounds.Height());
		}
		else
			fprintf (stderr, "AddOnWindow: ADDON_RESIZED but couldn't find config view\n");
		break;
	}
	default:
		inherited::MessageReceived (msg);
		break;
	}
}
