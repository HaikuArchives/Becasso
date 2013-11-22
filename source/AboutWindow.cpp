#include "AboutWindow.h"
#include "Becasso.h"
#include "Settings.h"

AboutWindow::AboutWindow (BRect rect)
: BWindow (rect, lstring (0, "About Becasso"), B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
}

AboutWindow::~AboutWindow ()
{
}

void AboutWindow::MessageReceived (BMessage *msg)
{
//	printf ("Huh?!\n");
	switch (msg->what)
	{

	default:
		inherited::MessageReceived (msg);
		break;
	}
}