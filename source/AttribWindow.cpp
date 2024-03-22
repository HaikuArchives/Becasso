#include "AttribWindow.h"
#include <string.h>
#include <stdio.h>
#include "Becasso.h"
#include "Colors.h"
#include "Settings.h"

AttribWindow::AttribWindow(BRect frame, const char* title)
	: BWindow(
		  frame, title, B_TITLED_WINDOW,
		  B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK
	  )
{
	current = -1;
	numviews = 0;
	strcpy(orig_title, title);
}

AttribWindow::~AttribWindow()
{
	if (current != -1) {
		//		printf ("Removing view %d from Window %s... ", current, Name()); fflush (stdout);
		view_n_icon c = views[current];
		RemoveChild(c.view);
		if (c.icon)
			RemoveChild(c.icon);
		//		printf ("Removed.\n");
	}
	for (int i = 0; i < numviews; i++) {
		//		printf ("deleting view %d: %s... ", i, views[i]->Name()); fflush (stdout);
		//		snooze (20000);
		view_n_icon c = views[i];
		delete c.view;
		delete c.icon;
		//		printf ("Deleted.\n");
	}
}

void
AttribWindow::Quit()
{
	BPoint origin = Frame().LeftTop();
	if (!strcmp(orig_title, "Mode"))
		set_window_origin(numModeWindow, IsHidden() ? InvalidPoint : origin);
	else if (!strcmp(orig_title, "Attribs"))
		set_window_origin(numAttribWindow, IsHidden() ? InvalidPoint : origin);
	inherited::Quit();
}

int
AttribWindow::AddView(AttribView* view, BBitmap* icon)
{
	view_n_icon entry;
	entry.view = view;
	entry.icon = 0;
	if (icon) {
		view->MoveBy(40, 0);
		BitmapView* iview = new BitmapView(
			BRect(0, 0, 39, view->Bounds().Height()), "icon", icon, B_OP_OVER, false
		);
		iview->SetViewColor(DarkGrey);
		iview->SetPosition(BPoint(4, 8));
		entry.icon = iview;
	}
	views[numviews] = entry;
	return (numviews++);
}

void
AttribWindow::RaiseView(int index)
{
	Lock();
	if (current != -1) {
		RemoveChild(views[current].view);
		if (views[current].icon)
			RemoveChild(views[current].icon);
	}
	view_n_icon c = views[index];
	AddChild(c.view);
	if (c.icon)
		AddChild(c.icon);
	current = index;
	ResizeTo(c.view->Bounds().Width() + (c.icon ? 40 : 0), views[index].view->Bounds().Height());
	SetTitle(c.view->Name());
	Unlock();
}

bool
AttribWindow::QuitRequested()
{
	Hide();
	return false;
}

void
AttribWindow::Show()
{
	inherited::Show();
}

void
AttribWindow::Hide()
{
	inherited::Hide();
}

BHandler*
AttribWindow::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
)
{
	//	printf ("\nAttribWindow::ResolveSpecifier():  message:\n");
	//	message->PrintToStream();
	//	printf ("specifier:\n");
	//	specifier->PrintToStream();

	const char* name;
	if (!strcasecmp(property, "Tool") || !strcasecmp(property, "Mode")) {
		if (specifier->FindString("name", &name) != B_OK) {
			int32 index;
			if (specifier->FindInt32("index", &index) == B_OK) {
				if (!strcasecmp(property, "Tool")) {
					//				extern const int32 NumTools;
					extern const char* ToolSpecifiers[];
					if (index >= 0 && index <= NumTools) {
						name = ToolSpecifiers[index];
					}
					else
						return inherited::ResolveSpecifier(
							message, index, specifier, command, property
						);
				}
				else if (!strcasecmp(property, "Mode")) {
					//				extern const int32 NumModes;
					extern const char* ModeSpecifiers[];
					if (index >= 0 && index <= NumModes) {
						name = ModeSpecifiers[index];
					}
					else
						return inherited::ResolveSpecifier(
							message, index, specifier, command, property
						);
				}
			}
			else // Name nor index?!
				return inherited::ResolveSpecifier(message, index, specifier, command, property);
		}
	}
	else // This must be a "normal" Be scripting call
		return inherited::ResolveSpecifier(message, index, specifier, command, property);

	message->PopSpecifier();

	int32 i;
	//	printf ("name = %s\n", name);
	for (i = 0; i < numviews; i++) {
		//		printf ("views[%ld].view->Name() = %s\n", i, views[i].view->Name());
		if (!strcasecmp(views[i].view->Name(), name)) {
			RaiseView(i);
			return views[i].view;
		}
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	default:
		if (!msg->HasBool("passed"))
			views[current].view->MessageReceived(msg);
		// Commenting this out fixes the TAB key bug... I'm expecting some
		// unwanted side effects but at least this doesn't crash anymore...
		// UPDATE: The unwanted side effects have been spotted: Popup menus
		// didn't work anymore.

		else
			inherited::MessageReceived(msg);
		break;
	}
}
