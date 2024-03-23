#include "AttribSpraycan.h"
#include "Colors.h"
#include <string.h>
#include "Settings.h"

static property_info prop_list[] = {
	{"Sigma", SET, DIRECT, "float: 1 .. 25"},
	{"Ratio|ColorRatio", SET, DIRECT, "float: 0 .. 1"},
	{"Rate|FlowRate", SET, DIRECT, "float: 1 .. 20"},
	{"Fade", SET, DIRECT, "bool: true, false"},
	0
};

AttribSpraycan::AttribSpraycan() : AttribView(BRect(0, 0, 164, 90), lstring(26, "Spraycan"))
{
	SetViewColor(LightGrey);
	sSlid =
		new Slider(BRect(4, 4, 160, 22), 60, lstring(355, "Sigma"), 1, 25, 1, new BMessage('ASsg'));
	cSlid = new Slider(
		BRect(4, 26, 160, 44), 60, lstring(356, "Color Ratio"), 0, 1, 0.01, new BMessage('AScr'),
		B_HORIZONTAL, 0, "%.2f"
	);
	fSlid = new Slider(
		BRect(4, 48, 160, 64), 60, lstring(357, "Flow Rate"), 1, 20, 1, new BMessage('ASfr')
	);
	AddChild(sSlid);
	AddChild(cSlid);
	AddChild(fSlid);
	fSigma = 5;
	sSlid->SetValue(5);
	cSlid->SetValue(0);
	fSlid->SetValue(10);
	sL = new BCheckBox(
		BRect(4, 68, 162, 86), "sL", lstring(358, "Fade with Distance"), new BMessage('ASfd')
	);
	sL->SetValue(0);
	fLighten = false;
	AddChild(sL);
	fColorRatio = 0;
	fFlowRate = 5;
	fCurrentProperty = 0;
}

AttribSpraycan::~AttribSpraycan() {}

status_t
AttribSpraycan::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Spray Can");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribSpraycan::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
)
{
	if (!strcasecmp(property, "Sigma")) {
		fCurrentProperty = PROP_SIGMA;
		return this;
	}
	if (!strcasecmp(property, "Ratio") || !strcasecmp(property, "ColorRatio")) {
		fCurrentProperty = PROP_RATIO;
		return this;
	}
	if (!strcasecmp(property, "Rate") || !strcasecmp(property, "FlowRate")) {
		fCurrentProperty = PROP_RATE;
		return this;
	}
	if (!strcasecmp(property, "Fade")) {
		fCurrentProperty = PROP_FADE;
		return this;
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribSpraycan::MessageReceived(BMessage* msg)
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
		case PROP_SIGMA: // float, 1 .. 25
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
				if (value >= 1 && value <= 25) {
					fSigma = value;
					sSlid->SetValue(value);
					if (msg->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						msg->SendReply(&error);
					}
				}
			}
			break;
		}
		case PROP_RATE: // float, 1 .. 20
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
				if (value >= 1 && value <= 20) {
					fFlowRate = value;
					fSlid->SetValue(value);
					if (msg->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						msg->SendReply(&error);
					}
				}
			}
			break;
		}
		case PROP_RATIO: // float, 0 .. 1
		{
			float value;
			if (msg->FindFloat("data", &value) == B_OK) {
				if (value >= 0 && value <= 1) {
					fColorRatio = value;
					cSlid->SetValue(value);
					if (msg->IsSourceWaiting()) {
						BMessage error(B_REPLY);
						error.AddInt32("error", B_NO_ERROR);
						msg->SendReply(&error);
					}
				}
			}
			break;
		}
		case PROP_FADE: // boolean
		{
			bool value;
			if (msg->FindBool("data", &value) == B_OK) {
				fLighten = value;
				sL->SetValue(value);
			}
		}
			fCurrentProperty = 0;
			break;
		}
		inherited::MessageReceived(msg);
		break;
	}
	case 'ASsg':
		fSigma = msg->FindFloat("value");
		break;
	case 'AScr':
		fColorRatio = msg->FindFloat("value");
		break;
	case 'ASfr':
		fFlowRate = msg->FindFloat("value");
		break;
	case 'ASfd':
		fLighten = sL->Value();
		break;
	default:
		inherited::MessageReceived(msg);
		break;
	}
}