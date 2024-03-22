// © 1998 Sum Software

#include "BecassoAddOn.h"

// #include "ScannerBe.h"

class CaptureWindow : public BWindow {
  public:
	CaptureWindow(BRect rect)
		: BWindow(
			  rect, "Capture Test", B_TITLED_WINDOW,
			  B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK
		  ){};
	virtual ~CaptureWindow(){};

	virtual bool QuitRequested()
	{
		Hide();
		return false;
	};
};

CaptureWindow* window;

class ClickWatcher : public BLooper {
  public:
	virtual void MessageReceived(BMessage* msg);
};

const uint32 kTriggerScan = 'trig';
static bool gBeScanOpen = false;
// static scan_id			gScanID = 0;
static ClickWatcher* gClickWatch = NULL;

int
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "Jim");
	strcpy(info->author, "Jim Moy");
	strcpy(info->copyright, "© 1997 Jim Moy");
	strcpy(info->description, "Fake Capture add-on");
	info->type = BECASSO_CAPTURE;
	info->index = index;
	info->version = 1;
	info->release = 0;
	info->becasso_version = 1;
	info->becasso_release = 0;
	info->does_preview = true;
	//	window = new CaptureWindow (BRect (100, 180, 100 + 188, 180 + 72));
	//	BView *bg = new BView (BRect (0, 0, 188, 72), "bg", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	//	bg->SetViewColor (LightGrey);
	//	window->AddChild (bg);
	//	BMessage *msg = new BMessage (CAPTURE_READY);
	//	msg->AddInt32 ("index", index);
	//	BButton *grab = new BButton (BRect (128, 40, 180, 64), "grab", "Grab", msg);
	//	grab->SetTarget (be_app);
	//	bg->AddChild (grab);
	//	window->Run();
	gClickWatch = new ClickWatcher;
	gClickWatch->Run();
	return (0);
}

int
addon_exit(void)
{
	return (0);
}

int
addon_open(BWindow*, const char*)
{
	//	window->Lock();
	//	if (window->IsHidden())
	//		window->Show ();
	//	else
	//		window->Activate ();
	//	window->Unlock();
	printf("Posting kTriggerScan\n");
	gClickWatch->PostMessage(kTriggerScan);
	return (0);
}

int
addon_close()
{
	//	window->Close();
	return (0);
}

BBitmap*
bitmap(char* title)
{
	printf("Here!\n");
	strcpy(title, "Grabbed");
	BBitmap* b = new BBitmap(BRect(0, 0, 127, 127), B_RGB_32_BIT, true);
	BView* v = new BView(BRect(0, 0, 127, 127), "bg", NULL, NULL);
	b->Lock();
	b->AddChild(v);
	v->FillRect(BRect(0, 0, 127, 127), B_MIXED_COLORS);
	v->Sync();
	b->RemoveChild(v);
	b->Unlock();
	delete v;
	return b;
}

void
ClickWatcher::MessageReceived(BMessage* msg)
{
	if (msg->what != kTriggerScan) {
		BLooper::MessageReceived(msg);
		return;
	}

	printf("before scan_start()\n");
	// fflush( stdout );

	status_t status = B_OK;
	//	status = scan_start( gScanID );
	if (status == B_OK)
		be_app->PostMessage(CAPTURE_READY);

	printf("after scan_start() (0x%X)\n", status);
	// fflush( stdout );
}
