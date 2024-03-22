#include "AttribFreehand.h"
#include "Colors.h"
#include <string.h>
#include "Settings.h"

static property_info prop_list[] = {{"PenSize", SET, DIRECT, "float: 0 .. 50"}, 0};

AttribFreehand::AttribFreehand() : AttribView(BRect(0, 0, 148, 40), lstring(27, "Freehand"))
{
	SetViewColor(LightGrey);
	pSlid = new Slider(
		BRect(8, 8, 140, 26), 60, lstring(310, "Pen Size"), 1, 50, 1, new BMessage('AFpc')
	);
	AddChild(pSlid);
	fPenSize = 1;
	fCurrentProperty = 0;
}

AttribFreehand::~AttribFreehand() {}

status_t
AttribFreehand::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Freehand");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribFreehand::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
)
{
	if (!strcasecmp(property, "PenSize")) {
		fCurrentProperty = PROP_PENSIZE;
		return this;
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribFreehand::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case B_GET_PROPERTY: {
		switch (fCurrentProperty) {
		}
		fCurrentProperty = 0;
		inherited::MessageReceived(msg);
		break;
	}
	case B_SET_PROPERTY: {
		switch (fCurrentProperty) {
		case PROP_PENSIZE: // float, 0 .. 50
		{
			float value;
			int32 ivalue;
			bool floatvalid = false;
			if (msg->FindInt32("data", &ivalue) == B_OK) // OK, we'll take int32's too.
			{
				value = ivalue;
				floatvalid = true;
			}
			if (floatvalid || msg->FindFloat("data", &value) == B_OK) {
				if (value >= 0 && value <= 50) {
					fPenSize = value;
					pSlid->SetValue(value);
					if (msg->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						msg->SendReply(&error);
					}
				}
			}
		}
			fCurrentProperty = 0;
			break;
		}
		inherited::MessageReceived(msg);
		break;
	}
	case 'AFpc':
		fPenSize = msg->FindFloat("value");
		break;
	default:
		inherited::MessageReceived(msg);
		break;
	}
}