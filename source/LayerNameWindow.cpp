#include "LayerNameWindow.h"
#include "Colors.h"
#include <Button.h>
#include "Settings.h"

LayerNameWindow::LayerNameWindow (const char *_name)
: BWindow (BRect (100, 100, 260, 170), "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	wait_sem = create_sem (0, "wait_sem");
	BRect bgFrame, cancelFrame, okFrame, textFrame;
	bgFrame.Set (0, 0, 160, 70);
	BView *bg = new BView (bgFrame, "LNW bg", B_FOLLOW_ALL, B_WILL_DRAW);
	bg->SetViewColor (LightGrey);
	AddChild (bg);
	
	textFrame.Set (8, 8, 152, 26);
	fName = new BTextControl (textFrame, "fName", lstring (176, "Name: "), _name, new BMessage ('LNWc'));
	fName->SetAlignment (B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fName->SetDivider (40);
	bg->AddChild (fName);
	
	cancelFrame.Set (38, 36, 90, 60);
	okFrame.Set (98, 36, 150, 60);
	BButton *cancel = new BButton (cancelFrame, "LNW cancel", lstring (131, "Cancel"), new BMessage ('LNcn'));
	BButton *ok = new BButton (okFrame, "LNW open", lstring (136, "OK"), new BMessage ('LNok')); 
	ok->MakeDefault (true);
	bg->AddChild (cancel);
	bg->AddChild (ok);
	
	fStatus = 0;
}

LayerNameWindow::~LayerNameWindow ()
{
	delete_sem (wait_sem);
}

int32 LayerNameWindow::Go ()
{
//	status_t err;
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

void LayerNameWindow::MessageReceived (BMessage *message)
{
	switch (message->what)
	{
	case 'LNcn':
		fStatus = 0;
		Hide();
		break;
	case 'LNok':
		fStatus = 1;
		Hide();
		break;
	default:
		inherited::MessageReceived(message);
		break;
	}
}