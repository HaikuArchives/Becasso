// Registration Window

#include "RegWindow.h"
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <File.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>
#include <stdio.h>
#include <string.h>
#include "BitmapView.h"
#include "Colors.h"
#include "Settings.h"

RegWindow::RegWindow(const BRect frame, const char* title, char type)
	: BWindow(frame, title, B_TITLED_WINDOW, B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
{
	fType = type;
	fStatus = 0;
	BRect bgFrame, iFrame, cancelFrame, regFrame;
	bgFrame = Bounds();
	iFrame = Bounds();
	bgFrame.left = 40;
	iFrame.right = 39;
	BView* bg = new BView(bgFrame, "RW bg", B_FOLLOW_ALL, B_WILL_DRAW);
	bg->SetViewColor(LightGrey);
	AddChild(bg);

	app_info info;
	be_app->GetAppInfo(&info);
	BFile file(&info.ref, O_RDONLY);
	BResources res(&file);
	BBitmap* icon = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
	size_t size;
	const void* icondata = res.LoadResource('ICON', "BEOS:L:image/x-becasso", &size);
	icon->SetBits(icondata, 1024, 0, B_COLOR_8_BIT);
	BitmapView* iView = new BitmapView(iFrame, "icon", icon, B_OP_OVER, false);
	AddChild(iView);
	iView->SetPosition(BPoint(4, 8));
	iView->SetViewColor(DarkGrey);

	regFrame.Set(
		bgFrame.Width() - 80, bgFrame.bottom - 34, bgFrame.Width() - 8, bgFrame.bottom - 8);
	cancelFrame.Set(regFrame.left - 88, regFrame.top, regFrame.left - 8, regFrame.bottom);
	BButton* cancel
		= new BButton(cancelFrame, "RW cancel", lstring(131, "Cancel"), new BMessage('Rcnc'));
	BButton* reg = new BButton(regFrame, "RW reg", lstring(426, "Register"), new BMessage('Rreg'));
	reg->MakeDefault(true);
	bg->AddChild(cancel);
	bg->AddChild(reg);

	fName = new BTextControl(BRect(4, regFrame.top - 30, bgFrame.Width() - 4, regFrame.top - 8),
		"Name", lstring(425, "Name:"), "", new BMessage('Rnam'));
	fName->SetDivider(50);
	bg->AddChild(fName);

	if (type == REG_PPC) {
		bg->AddChild(new BStringView(BRect(4, 4, bgFrame.Width() - 4, 20), "rg1",
			lstring(436, "PowerPC hardware detected")));
		bg->AddChild(new BStringView(
			BRect(4, 20, bgFrame.Width() - 4, 36), "rg2", lstring(437, "This is a free version.")));
	} else if (type == REG_HAS_14) {
		bg->AddChild(new BStringView(
			BRect(4, 4, bgFrame.Width() - 4, 20), "rg1", lstring(421, "Becasso 1.4 detected")));
		bg->AddChild(new BStringView(
			BRect(4, 20, bgFrame.Width() - 4, 36), "rg2", lstring(422, "This is a free upgrade.")));
	} else if (type == REG_PREINST) {
		bg->AddChild(new BStringView(BRect(4, 4, bgFrame.Width() - 4, 20), "rg1",
			lstring(430, "Becasso 2.0 was pre-installed,")));
		bg->AddChild(new BStringView(BRect(4, 20, bgFrame.Width() - 4, 36), "rg2",
			lstring(431, "but hasn't been registered yet.")));
	} else	// must have the CD
	{
		bg->AddChild(new BStringView(BRect(4, 4, bgFrame.Width() - 4, 20), "rg3",
			lstring(427, "Thank you for purchasing Becasso!")));
	}
	bg->AddChild(new BStringView(BRect(4, 40, bgFrame.Width() - 4, 56), "rg4",
		lstring(428, "Please register by entering your name.")));
	bg->AddChild(new BStringView(BRect(4, 56, bgFrame.Width() - 4, 72), "rg5",
		lstring(429, "This will unlock the full version.")));
}

RegWindow::~RegWindow() {}

int32
RegWindow::Go()
{
	// not too happy with this implementation...
	bool ishidden = false;
	Show();
	while (!ishidden) {
		Lock();
		ishidden = IsHidden();
		Unlock();
		snooze(50000);
	}
	return (fStatus);
}

void
RegWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'Rcnc':
			fStatus = 0;
			Hide();
			break;
		case 'Rreg':
		{
			extern char gAlphaMask[128];
			strcpy(gAlphaMask, fName->Text());
			fStatus = 1;
			Hide();
			break;
		}
		case 'Rnam':
			break;
		default:
			BWindow::MessageReceived(msg);
	}
}