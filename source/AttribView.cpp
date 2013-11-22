#include "AttribView.h"
#include "AttribWindow.h"

AttribView::AttribView (BRect frame, const char *title)
: BView (frame, title, B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE)
{
}

AttribView::~AttribView ()
{
}

void AttribView::MessageReceived (BMessage *message)
{
	bool passed = true;
	message->AddBool ("passed", passed);
	inherited::MessageReceived (message);
}

BHandler *AttribView::ResolveSpecifier (BMessage *message, int32 index, BMessage *specifier, int32 command, const char *property)
{
	return inherited::ResolveSpecifier (message, index, specifier, command, property);
}
