#include <StringView.h>
#include "AttribSelect.h"
#include "Slider.h"
#include "Colors.h"
#include "Settings.h"

static property_info prop_list[] = {0};

AttribSelect::AttribSelect() : AttribView(BRect(0, 0, 128, 40), lstring(21, "Select"))
{
	SetViewColor(LightGrey);
	BStringView* sv =
		new BStringView(BRect(6, 6, 120, 26), "no controls", lstring(306, "No Parameters"));
	AddChild(sv);
	//	Slider *pSlid = new Slider (BRect (8, 8, 120, 26), 46, "Pen Size", 0, 50, 1, new BMessage
	//('AFpc')); 	AddChild (pSlid);
}

AttribSelect::~AttribSelect() {}

status_t
AttribSelect::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Select");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribSelect::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
)
{
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribSelect::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	default:
		inherited::MessageReceived(msg);
		break;
	}
}