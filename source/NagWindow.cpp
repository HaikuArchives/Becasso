#include "NagWindow.h"
#include <Application.h>
#include <Bitmap.h>
#include <File.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <StringView.h>
#include <View.h>
#include <unistd.h>
#include "BitmapView.h"
#include "Colors.h"
#include "Settings.h"

#define NAG_TIME 15

NagWindow::NagWindow(BRect frame)
	: BWindow(frame, "", B_MODAL_WINDOW, B_NOT_RESIZABLE)
{
	BRect bgFrame, iFrame;
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

	char wait[256];
	sprintf(wait, lstring(432, "Please wait %d seconds…"), NAG_TIME);

	bg->AddChild(new BStringView(BRect(4, 4, 400, 20), "", lstring(4, "Unregistered Version")));
	bg->AddChild(new BStringView(BRect(4, 22, 400, 38), "", wait));
	bg->AddChild(new BStringView(BRect(4, 40, 400, 56), "",
		lstring(433, "(This delay will go away when you register Becasso.)")));
	bg->AddChild(new BStringView(BRect(4, 58, 400, 74), "", lstring(434, "")));
}

NagWindow::~NagWindow() {}

void
NagWindow::Go()
{
	Show();
	sleep(NAG_TIME);
	Lock();
	Quit();
}