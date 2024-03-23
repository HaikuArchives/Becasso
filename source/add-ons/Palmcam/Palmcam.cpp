// © 1999-2001 Sum Software

#define BUILDING_ADDON
#include "BecassoAddOn.h"
#include <Button.h>
#include <Window.h>
#include <Application.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <ListView.h>
#include <ListItem.h>
#include <Directory.h>
#include <Path.h>
#include <Entry.h>
#include <string.h>
#include <BitmapStream.h>
#include <TranslatorRoster.h>
#include <DataIO.h>
#include <StatusBar.h>
#include <Messenger.h>
#include "BeDSC.h"

BeDSC* camera = NULL;
int cursel = -1;
int nsel = 0;
int my_addon_index = 0;

class CaptureWindow : public BWindow
{
  public:
	CaptureWindow(BRect rect)
		: BWindow(rect, "PalmCam", B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK)
	{
		fBPS = B_9600_BPS;
		fPort[0] = 0;
		abort = false;
	}

	virtual ~CaptureWindow() {}

	virtual void MessageReceived(BMessage* msg);

	virtual bool QuitRequested()
	{
		Hide();
		if (camera)
			camera->Close();
		return false;
	}

	const char* portName() { return fPort; }

	data_rate portBPS() { return fBPS; };

	BButton* fGrab;
	BButton* fErase;
	BButton* fAbort;
	BListView* fList;
	BStatusBar* fStatusBar;

	bool abort;

  private:
	typedef BWindow inherited;
	data_rate fBPS;
	char fPort[64];
};

status_t
reopen_camera(const char* port, data_rate bps);
status_t
update_list();

void
CaptureWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case 'bps1':
		fBPS = B_9600_BPS;
		reopen_camera(fPort, fBPS);
		break;
	case 'bps2':
		fBPS = B_19200_BPS;
		reopen_camera(fPort, fBPS);
		break;
	case 'bps3':
		fBPS = B_38400_BPS;
		reopen_camera(fPort, fBPS);
		break;
	case 'bps4':
		fBPS = B_57600_BPS;
		reopen_camera(fPort, fBPS);
		break;
	case 'PCpt':
		const char* name;
		msg->FindString("port", &name);
		strcpy(fPort, name);
		reopen_camera(fPort, fBPS);
		break;
	case 'PCab':
		abort = true;
		break;
	case 'PCsc': // Selection changed
	{
		// BStringItem *item;
		bool someselected = false;
		int32 selected, i = 0;
		int32 lastselected = 0;
		nsel = 0;
		cursel = 0;
		while ((selected = fList->CurrentSelection(i++)) >= 0) {
			someselected = true;
			lastselected = selected;
			nsel++;
		}
		if (someselected) {
			if (camera->Preview(lastselected + 1) != B_OK)
				printf("Error previewing (%ld)\n", lastselected + 1);
		}
		fGrab->SetEnabled(someselected);
		fErase->SetEnabled(someselected);
		break;
	}
	case 'PCer': // erase items from camera
	{
		//		printf ("Erase selection...\n");
		bool someselected = false;
		int32 selected, i = 0;
		nsel = 0;
		cursel = 0;
		while ((selected = fList->CurrentSelection(i++)) >= 0) {
			someselected = true;
		}
		// We have to delete top-down because otherwise indices change during the process.
		if (someselected) {
			float numitems = --i;
			fStatusBar->Reset();
			i--;
			//			printf ("Items to be erased: %d.  First: %ld\n", int (numitems), i);
			while ((selected = fList->CurrentSelection(i)) >= 0) {
				//				camera->Preview (selected + 1);
				fStatusBar->Update(100.0 / numitems);
				//				printf ("Deleting %ld\n", selected + 1);
				if (camera->Delete(selected + 1) != B_OK)
					printf("Error deleting (%ld) from camera\n", selected + 1);
				fList->RemoveItem(selected);
				i--;
			}
		}
		fGrab->SetEnabled(false);
		fErase->SetEnabled(false);
		fStatusBar->Reset();
		break;
	}
	case CAPTURE_READY: {
		// We get here when there was a multiple selection.
		// Horrible Hack alert!!
		//
		BMessenger app(be_app);
		snooze(1000000); // Let's hope the image has been opened by now.
		app.SendMessage(msg);
		break;
	}
	default:
		inherited::MessageReceived(msg);
		break;
	}
}

CaptureWindow* window;

status_t
reopen_camera(const char* port, data_rate bps)
{
	extern int DebugLevel;

	if (!camera) {
		if (DebugLevel)
			fprintf(stderr, "No camera\n");
		return B_ERROR;
	}

	if (DebugLevel)
		fprintf(stderr, "Trying to close the camera\n");
	//	delete camera;
	//	camera = new DSC();

	if (camera->Close() != B_OK) {
		// Could be that the camera simply wasn't opened.  No problem.
	}

	if (DebugLevel) {
		fprintf(stderr, "Trying to open camera at %s... ", port);
		fflush(stderr);
	}

	if (camera->Open(port, bps) != B_OK) {
		fprintf(stderr, "Error opening camera\n");
		return B_ERROR;
	}
	if (DebugLevel)
		fprintf(stderr, "Opened.\n");

	window->Lock();
	window->fStatusBar->Reset();
	for (int i = 0; i < DSCPAUSE * 8; i++) {
		window->fStatusBar->Update(100.0 / (8 * DSCPAUSE));
		window->UpdateIfNeeded();
		snooze(125000); // Wait for the camera to redraw its screen
	}
	window->fStatusBar->Reset();
	window->Unlock();
	return (update_list());
}

status_t
update_list()
{
	window->fList->MakeEmpty();
	nsel = 0;
	cursel = 0;

	if (!camera)
		return B_ERROR;

	int n, nimg;
	dsc_quality_t qbuf[DSCMAXIMAGE];
	if ((nimg = camera->GetIndex(qbuf)) == -1) {
		return B_OK;
	}

	extern int DebugLevel;
	if (DebugLevel)
		fprintf(stderr, "%d images\n", nimg);

	for (n = 0; n < nimg; n++) {
		char item[128];
		sprintf(item, "dsc%04u.jpg (%7ld) - ", n + 1, camera->RequestImage(n + 1));
		switch (qbuf[n]) {
		case normal:
			strcat(item, "normal");
			break;
		case fine:
			strcat(item, "fine");
			break;
		case superfine:
			strcat(item, "superfine");
			break;
		default:
			strcat(item, "[???]");
			break;
		}
		window->fList->AddItem(new BStringItem(item));
	}
	return B_OK;
}

status_t
addon_init(uint32 index, becasso_addon_info* info)
{
	strcpy(info->name, "PalmCam Capture");
	strcpy(info->author, "Sander Stoks");
	strcpy(info->copyright, "© 2000-2001 ∑ Sum Software, Fredrik Roubert");
	strcpy(info->description, "Panasonic PalmCam digital camera Capture add-on");
	info->type = BECASSO_CAPTURE;
	info->index = index;
	info->version = 0;
	info->release = 5;
	info->becasso_version = 2;
	info->becasso_release = 0;
	info->does_preview = 0;
	info->flags = 0;
	window = new CaptureWindow(BRect(100, 180, 100 + 300, 180 + 178));
	BView* bg = new BView(BRect(0, 0, 300, 178), "bg", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	bg->SetViewColor(LightGrey);
	window->AddChild(bg);

	BPopUpMenu* portPU = new BPopUpMenu("Port");
	// Get the available serial ports
	BDirectory dir("/dev/ports");
	BEntry entry;
	bool foundone = false;
	while (dir.GetNextEntry(&entry, true) == B_OK) {
		foundone = true;
		char name[B_FILE_NAME_LENGTH];
		entry.GetName(name);
		BMessage* msg = new BMessage('PCpt');
		msg->AddString("port", name);
		portPU->AddItem(new BMenuItem(name, msg));
	}
	if (!foundone) {
		portPU->AddItem(new BMenuItem("<no ports>", NULL));
	}
	BMenuField* portMF = new BMenuField(BRect(8, 4, 120, 24), "dPort", "Port: ", portPU);
	portMF->SetDivider(40);
	bg->AddChild(portMF);

	BPopUpMenu* speedPU = new BPopUpMenu("");
	speedPU->AddItem(new BMenuItem(" 9600", new BMessage('bps1')));
	speedPU->AddItem(new BMenuItem("19200", new BMessage('bps2')));
	speedPU->AddItem(new BMenuItem("38400", new BMessage('bps3')));
	speedPU->AddItem(new BMenuItem("57600", new BMessage('bps4')));
	speedPU->ItemAt(0)->SetMarked(true);
	BMenuField* speedMF = new BMenuField(BRect(8, 28, 120, 48), "dSpeed", "Speed: ", speedPU);
	speedMF->SetDivider(40);
	bg->AddChild(speedMF);

	my_addon_index = index;
	BMessage* msg = new BMessage(CAPTURE_READY);
	msg->AddInt32("index", index);
	window->fGrab = new BButton(BRect(8, 60, 60, 80), "grab", "Grab", msg);
	window->fGrab->SetTarget(be_app);
	window->fGrab->SetEnabled(false);
	bg->AddChild(window->fGrab);

	window->fErase = new BButton(BRect(8, 88, 60, 108), "erase", "Erase", new BMessage('PCer'));
	window->fErase->SetEnabled(false);
	bg->AddChild(window->fErase);

	window->fAbort = new BButton(BRect(8, 116, 60, 136), "abort", "Abort", new BMessage('PCab'));
	window->fAbort->SetEnabled(false);
	bg->AddChild(window->fAbort);

	window->fList = new BListView(
		BRect(110, 8, 294 - B_V_SCROLL_BAR_WIDTH, 170), "image list", B_MULTIPLE_SELECTION_LIST,
		B_FOLLOW_ALL_SIDES
	);
	window->fList->SetSelectionMessage(new BMessage('PCsc'));

	BScrollView* scroller =
		new BScrollView("scroller", window->fList, B_FOLLOW_ALL_SIDES, 0, false, true);
	bg->AddChild(scroller);

	window->fStatusBar = new BStatusBar(BRect(8, 148, 102, 171), "status");
	window->fStatusBar->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	window->fStatusBar->SetBarHeight(10.0);
	bg->AddChild(window->fStatusBar);

	camera = new BeDSC();
	window->Run();
	return B_OK;
}

status_t
addon_exit(void)
{
	delete camera;
	return B_OK;
}

status_t
addon_open(void)
{
	extern int DebugLevel;
	window->Lock();
	if (window->IsHidden())
		window->Show();
	else
		window->Activate();
	window->Unlock();
	if (!window->portName()[0]) {
		if (DebugLevel)
			fprintf(stderr, "No port defined for the camera yet\n");
	} else if (camera->Open(window->portName(), window->portBPS()) != B_OK) {
		if (DebugLevel)
			fprintf(stderr, "Couldn't open camera at %s\n", window->portName());
	} else {
		update_list();
		window->Lock();
		window->fList->Invalidate();
		window->UpdateIfNeeded();
		window->Unlock();
	}
	return B_OK;
}

BBitmap*
bitmap(char* title)
{
	int32 selected;

	if (!nsel || cursel == -1) {
		fprintf(stderr, "PalmCam error - somehow nothing is selected.\n");

		// Just to make sure, diable the buttons.
		window->fGrab->SetEnabled(false);
		window->fErase->SetEnabled(false);

		strcpy(title, "Error");
		return (NULL);
	}

	selected = window->fList->CurrentSelection(cursel++) + 1; // The Palmcam starts at #1

	// Retreive the image from the camera
	sprintf(title, "dsc%04u.jpg", int(selected));
	ssize_t imgsize = camera->RequestImage(selected);
	if (imgsize == -1) {
		fprintf(stderr, "PalmCam error - file size of image %ld reported to be -1\n", selected);
		strcpy(title, "Error");
		return (NULL);
	}
	extern int DebugLevel;
	if (DebugLevel)
		fprintf(stderr, "Image size of %ld is %ld bytes\n", selected, imgsize);

	ssize_t upimgsize = (imgsize / 2048 + 1) * 2048; // Round up for some extra space
	uint8* imgbuf = new uint8[upimgsize];

	window->Lock();
	window->fAbort->SetEnabled(true);
	window->fStatusBar->Reset();
	window->Unlock();
	ssize_t s, bf;
	int i;
	for (i = 0, s = 0; s < imgsize;) {
		window->Lock();
		window->fStatusBar->Update(102400.0 / imgsize, title);
		window->Unlock();
		if (window->abort) {
			fprintf(stderr, "Transfer aborted\n");
			strcpy(title, "Aborted");
			cursel--;
			delete[] imgbuf;
			window->Lock();
			window->fAbort->SetEnabled(false);
			window->fStatusBar->Reset();
			window->abort = false;
			window->Unlock();
			return NULL;
		}
		if ((bf = camera->ReadImageBlock(i, imgbuf + s)) == -1) // Error reading block
		{
			if (camera->RequestImage(selected) == -1) // Something's seriously wrong.  Bail out.
			{
				fprintf(stderr, "PalmCam error during file transfer\n");
				strcpy(title, "Error");
				delete[] imgbuf;
				window->Lock();
				window->fStatusBar->Reset();
				window->Unlock();
				return (NULL);
			}
			// else simply retry
		} else {
			i++;
			s += bf;
		}
	}
	window->Lock();
	window->fStatusBar->Reset();
	window->Unlock();

	// OK, we now have a JPEG image in imgbuf.  Use the Translation Kit to make that into a bitmap.
	BBitmapStream outStream;
	BMemoryIO inStream(imgbuf, upimgsize);
	BBitmap* b = NULL;
	if (BTranslatorRoster::Default()->Translate(
			&inStream, NULL, NULL, &outStream, B_TRANSLATOR_BITMAP
		) < B_OK) {
		// Something went wrong translating the JPEG
		fprintf(stderr, "PalmCam error during JPEG translation\n");
		strcpy(title, "Error");
		return NULL;
	}
	outStream.DetachBitmap(&b);

	delete[] imgbuf;

	if (cursel < nsel) // There are some more images to get...
	{
		BMessage* msg = new BMessage(CAPTURE_READY);
		msg->AddInt32("index", my_addon_index);
		BMessenger me(window);
		window->Lock();
		me.SendMessage(msg);
		window->Unlock();
	} else {
		window->Lock();
		window->fAbort->SetEnabled(false);
		window->Unlock();
	}

	return b;
}
