#include "XpalWindow.h"
#include "Colors.h"
#include <Button.h>
#include <CheckBox.h>
#include <Message.h>
#include <stdio.h>
#include "Settings.h"

#define XPW_BSIZE 74

XpalWindow::XpalWindow (BRect rect, const char *name, BMessenger *target)
: BWindow (rect, name, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE), fTarget (target)
{
	BView *bg = new BView (Bounds(), "bg", B_FOLLOW_ALL, B_WILL_DRAW);
	bg->SetViewColor (LightGrey);
	AddChild (bg);

	float bw = Bounds().Width();
	Slider *nSlid = new Slider (BRect (8, 8, 8 + (bw - 16), 24), 50, lstring (184, "# Colors"), 1, 256, 1, new BMessage ('numC'));
	bg->AddChild (nSlid);
	fNum = 256;
	nSlid->SetValue (256);
	fClobberCB = new BCheckBox (BRect (8, 30, 8 + (bw - 16), 46), "clobber", lstring (185, "Clobber Remaining Palette"), new BMessage ('clob'));
	fClobber = false;
	fClobberCB->SetValue (false);
	fClobberCB->SetEnabled (false);
	bg->AddChild (fClobberCB);
	BButton *xFG = new BButton (BRect (bw - 2*XPW_BSIZE - 16, 50, bw - XPW_BSIZE - 16, 74), "fg", lstring (70, "Foreground"), new BMessage ('xpfg'));
	BButton *xBG = new BButton (BRect (bw - XPW_BSIZE - 8, 50, bw - 8, 74), "bg", lstring (71, "Background"), new BMessage ('xpbg'));
	bg->AddChild (xFG);
	bg->AddChild (xBG);
	fStatus = 0;
}

XpalWindow::~XpalWindow ()
{
	delete fTarget;
}

#if 0
int32 XpalWindow::Go ()
{
	BWindow *w = dynamic_cast<BWindow *> (BLooper::LooperForThread (find_thread (NULL)));
//	bigtime_t timeout = w ? 200000 : B_INFINITE_TIMEOUT;
	bool ishidden = false;
	Show();
	while (!ishidden)
	{
		Lock();
		ishidden = IsHidden();
		Unlock();
		snooze (50000);
//		while ((((err = acquire_sem_etc (wait_sem, 1, B_TIMEOUT, timeout)) == B_TIMED_OUT) || (err == B_INTERRUPTED)) && !ishidden)
//		{
			if (w)
				w->UpdateIfNeeded();
//		}
	}
	return (fStatus);
}
#endif

void XpalWindow::MessageReceived (BMessage *message)
{
	switch (message->what)
	{
	case 'numC':
		fNum = int (message->FindFloat ("value"));
		if (fNum < 256)
			fClobberCB->SetEnabled (true);
		else
			fClobberCB->SetEnabled (false);
		break;
	case 'clob':
		fClobber = fClobberCB->Value();
		if (fClobber)
			printf ("Clobber now true\n");
		else
			printf ("Clobber now false\n");
		break;
	case 'xpfg':
	{
		BMessage *m = new BMessage ('xclF');
		m->AddBool ("clobber", fClobber);
		m->AddInt32 ("num_cols", fNum);
		// m->PrintToStream();
		fTarget->SendMessage (m);
		delete m;
		break;
	}
	case 'xpbg':
	{
		BMessage *m = new BMessage ('xclB');
		m->AddBool ("clobber", fClobber);
		m->AddInt32 ("num_cols", fNum);
		fTarget->SendMessage (m);
		delete m;
		break;
	}
	default:
		inherited::MessageReceived (message);
		break;
	}
}