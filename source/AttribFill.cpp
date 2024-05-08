#include "AttribFill.h"
#include <Box.h>
#include <RadioButton.h>
#include <StringView.h>
#include <View.h>
#include <string.h>
#include "Colors.h"
#include "Settings.h"
#include "TabView.h"

static property_info prop_list[] = {
	{"ToleranceType|Type", SET, DIRECT, "string: Visual, Absolute|RGB"},
	{"Tolerance|Visual", SET, DIRECT, "float: 0 .. 255"}, {"Red", SET, DIRECT, "int: 0 .. 255"},
	{"Green", SET, DIRECT, "int: 0 .. 255"}, {"Blue", SET, DIRECT, "int: 0 .. 255"}, 0};

AttribFill::AttribFill() : AttribView(BRect(0, 0, 146, 146), lstring(24, "Fill"))
{
	SetViewColor(LightGrey);
	BBox* tolSets = new BBox(BRect(4, 4, 142, 142), "tol");
	tolSets->SetLabel(lstring(338, "Tolerance"));
	AddChild(tolSets);
	tol = new BRadioButton(
		BRect(8, 13, 124, 30), "tol", lstring(339, "Visual Distance"), new BMessage('AFtV'));
	rgb = new BRadioButton(
		BRect(8, 30, 124, 46), "rgb", lstring(340, "Absolute RGB"), new BMessage('AFtS'));
	tol->SetValue(B_CONTROL_ON);
	tolSets->AddChild(tol);
	tolSets->AddChild(rgb);
	TabView* bgTab = new TabView(BRect(4, 50, 132, 134), "AttribFill Tab");
	tolSets->AddChild(bgTab);
	BView* tolTab = new BView(BRect(2, TAB_HEIGHT + 4, 126, TAB_HEIGHT + 63), "tol View",
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	BView* rgbTab = new BView(BRect(2, TAB_HEIGHT + 4, 126, TAB_HEIGHT + 63), "rgb View",
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	tolTab->SetViewColor(LightGrey);
	rgbTab->SetViewColor(LightGrey);
	bgTab->AddView(tolTab, lstring(341, "Visual"));
	bgTab->AddView(rgbTab, lstring(342, "RGB"));
	BStringView* explD =
		new BStringView(BRect(2, 8, 122, 20), "explD", lstring(343, "Visual Factors:"));
	BStringView* facD =
		new BStringView(BRect(2, 20, 122, 32), "facD", lstring(344, "R: 0.213 G: 0.715 B: 0.072"));
	explD->SetFontSize(10);
	facD->SetFontSize(10);
	sT = new Slider(BRect(4, 42, 122, 58), 10, "D", 0, 255, 1, new BMessage('AFcT'));
	sR = new Slider(BRect(4, 2, 122, 18), 10, "R", 0, 255, 1, new BMessage('AFcR'));
	sG = new Slider(BRect(4, 22, 122, 38), 10, "G", 0, 255, 1, new BMessage('AFcG'));
	sB = new Slider(BRect(4, 42, 122, 58), 10, "B", 0, 255, 1, new BMessage('AFcB'));
	tolTab->AddChild(sT);
	tolTab->AddChild(explD);
	tolTab->AddChild(facD);
	rgbTab->AddChild(sR);
	rgbTab->AddChild(sG);
	rgbTab->AddChild(sB);
	fTolMode = FILLTOL_TOL;
	fTolerance = 0;
	fToleranceRGB.red = 0;
	fToleranceRGB.green = 0;
	fToleranceRGB.blue = 0;
	fCurrentProperty = 0;
}

AttribFill::~AttribFill() {}

status_t
AttribFill::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Fill");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribFill::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property)
{
	if (!strcasecmp(property, "ToleranceType") || !strcasecmp(property, "Type")) {
		fCurrentProperty = PROP_TOLTYPE;
		return this;
	}
	if (!strcasecmp(property, "Tolerance") || !strcasecmp(property, "Visual")) {
		fCurrentProperty = PROP_VISUAL;
		return this;
	}
	if (!strcasecmp(property, "Red")) {
		fCurrentProperty = PROP_DRED;
		return this;
	}
	if (!strcasecmp(property, "Green")) {
		fCurrentProperty = PROP_DGREEN;
		return this;
	}
	if (!strcasecmp(property, "Blue")) {
		fCurrentProperty = PROP_DBLUE;
		return this;
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribFill::MessageReceived(BMessage* msg)
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
				case PROP_VISUAL:  // float, 0 .. 255
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
						if (value >= 0 && value <= 255) {
							fTolerance = value;
							sT->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_DRED:	 // int, 0 .. 255
				{
					int32 value;
					if (msg->FindInt32("data", &value) == B_OK) {
						if (value >= 0 && value <= 255) {
							fToleranceRGB.red = value;
							sR->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_DGREEN:  // int, 0 .. 255
				{
					int32 value;
					if (msg->FindInt32("data", &value) == B_OK) {
						if (value >= 0 && value <= 255) {
							fToleranceRGB.green = value;
							sG->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_DBLUE:  // int, 0 .. 255
				{
					int32 value;
					if (msg->FindInt32("data", &value) == B_OK) {
						if (value >= 0 && value <= 255) {
							fToleranceRGB.blue = value;
							sB->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_TOLTYPE:	// Visual, RGB|Absolute
				{
					const char* name;
					int value = 0;
					if (msg->FindString("data", &name) == B_OK) {
						if (!strcasecmp(name, "Visual")) {
							tol->SetValue(B_CONTROL_ON);
							value = FILLTOL_TOL;
						} else if (!strcasecmp(name, "RGB") || !strcasecmp(name, "Absolute")) {
							rgb->SetValue(B_CONTROL_ON);
							value = FILLTOL_RGB;
						}
						if (value) {
							fTolMode = value;
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						} else {
							// Error report...
						}
					}
				}
					fCurrentProperty = 0;
					break;
			}
			inherited::MessageReceived(msg);
			break;
		}
		case 'AFtV':
			fTolMode = FILLTOL_TOL;
			break;
		case 'AFtS':
			fTolMode = FILLTOL_RGB;
			break;
		case 'AFcT':
			fTolerance = msg->FindFloat("value");
			break;
		case 'AFcR':
			fToleranceRGB.red = uint8(msg->FindFloat("value"));
			break;
		case 'AFcG':
			fToleranceRGB.green = uint8(msg->FindFloat("value"));
			break;
		case 'AFcB':
			fToleranceRGB.blue = uint8(msg->FindFloat("value"));
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}