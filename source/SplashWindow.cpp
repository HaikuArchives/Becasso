#include "SplashWindow.h"
#include "AboutView.h"
#include "Becasso.h"

#include <stdio.h>

SplashWindow::SplashWindow(BRect rect, BBitmap* becasso, BBitmap* sum)
	: BWindow(rect, "Splash Screen", B_BORDERED_WINDOW, (int32)NULL)
{
	abV = new SAboutView(Bounds(), becasso, sum, true);
	AddChild(abV);
}

SplashWindow::~SplashWindow() {}

void
SplashWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case 'Iaos':
		{
			const char* s;
			msg->FindString("InitString", &s);
			// 		printf ("s = %s\n", s);
			abV->SetInitString(s);
			break;
		}
		default:
			inherited::MessageReceived(msg);
			break;
	}
}