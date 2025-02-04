#include "AttribLines.h"
#include <Handler.h>
#include <string.h>
#include "Colors.h"
#include "Settings.h"

static property_info prop_list[] = {
	{ "PenSize", SET, DIRECT, "float: 0 .. 50" },
	{ "Corners|FillCorners", SET, DIRECT, "bool: true, false" },
	0,
};

AttribLines::AttribLines()
	: AttribView(BRect(0, 0, 148, 54), lstring(28, "Lines"))
{
	SetViewColor(LightGrey);
	lSlid = new Slider(
		BRect(8, 8, 140, 26), 60, lstring(310, "Pen Size"), 1, 50, 1, new BMessage('ALpc'));
	AddChild(lSlid);
	fPenSize = 1;
	lFC = new BCheckBox(
		BRect(8, 30, 120, 48), "lFC", lstring(348, "Fill Corners"), new BMessage('ALfc'));
	lFC->SetValue(1);
	fFillCorners = true;
	AddChild(lFC);
	fCurrentProperty = 0;
}

AttribLines::~AttribLines() {}

status_t
AttribLines::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Lines");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribLines::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property)
{
	if (!strcasecmp(property, "PenSize")) {
		fCurrentProperty = PROP_PENSIZE;
		return this;
	}
	if (!strcasecmp(property, "Corners") || !strcasecmp(property, "FillCorners")) {
		fCurrentProperty = PROP_FILLCORNERS;
		return this;
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribLines::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_GET_PROPERTY:
		{
			switch (fCurrentProperty) {
			}
			fCurrentProperty = 0;
			inherited::MessageReceived(msg);
			break;
		}
		case B_SET_PROPERTY:
		{
			switch (fCurrentProperty) {
				case PROP_PENSIZE:	// float, 0 .. 50
				{
					float value;
					int32 ivalue;
					bool floatvalid = false;
					if (msg->FindInt32("data", &ivalue) == B_OK)  // OK, we'll take int32's too.
					{
						value = ivalue;
						floatvalid = true;
					}
					if (floatvalid || msg->FindFloat("data", &value) == B_OK) {
						if (value >= 0 && value <= 50) {
							fPenSize = value;
							lSlid->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_FILLCORNERS:	// boolean
				{
					bool value;
					if (msg->FindBool("data", &value) == B_OK) {
						fFillCorners = value;
						lFC->SetValue(value);
					}
				}
					fCurrentProperty = 0;
					break;
			}
			inherited::MessageReceived(msg);
			break;
		}
		case 'ALpc':
			fPenSize = msg->FindFloat("value");
			break;
		case 'ALfc':
			fFillCorners = lFC->Value();
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}
