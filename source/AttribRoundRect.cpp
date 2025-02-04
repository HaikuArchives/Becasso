#include "AttribRoundRect.h"
#include <Box.h>
#include <RadioButton.h>
#include <Rect.h>
#include <View.h>
#include <Window.h>
#include <string.h>
#include "Colors.h"
#include "Settings.h"
#include "TabView.h"

static property_info prop_list[] = {
	{ "PenSize", SET, DIRECT, "float: 0 .. 50" },
	{ "ShapeType|Type", SET, DIRECT, "string: FilledOutline, Filled, Outline" },
	{ "RelativeWidth|RelX", SET, DIRECT, "float: 0 .. 0.5" },
	{ "RelativeHeight|RelY", SET, DIRECT, "float: 0 .. 0.5" },
	{ "AbsoluteWidth|AbsX", SET, DIRECT, "float: 0 .. 200" },
	{ "AbsoluteHeight|AbsY", SET, DIRECT, "float: 0 .. 200" },
	{ "Corners", SET, DIRECT, "string: Relative, Absolute" },
	0,
};

AttribRoundRect::AttribRoundRect()
	: AttribView(BRect(0, 0, 148, 190), lstring(32, "Ovals"))
{
	SetViewColor(LightGrey);
	lSlid = new Slider(
		BRect(8, 8, 140, 26), 60, lstring(310, "Pen Size"), 1, 50, 1, new BMessage('ALpc'));
	AddChild(lSlid);
	fType = RRECT_OUTFILL;
	fPenSize = 1;
	fRadType = RRECT_RELATIVE;
	fRadXabs = 8;
	fRadYabs = 8;
	fRadXrel = 0.1;
	fRadYrel = 0.1;

	BBox* type = new BBox(BRect(18, 32, 130, 82), "type");
	type->SetLabel(lstring(311, "Type"));
	AddChild(type);

	BRect shape = BRect(3, 5, 27, 25);

	BWindow* picWindow = new BWindow(
		BRect(0, 0, 100, 100), "Temp Pic Window", B_BORDERED_WINDOW, uint32(NULL), uint32(NULL));
	BView* bg = new BView(BRect(0, 0, 100, 100), "Temp Pic View", uint32(NULL), uint32(NULL));
	picWindow->AddChild(bg);

	BPicture* p10;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(LightGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetLowColor(White);
	bg->FillRoundRect(shape, 8, 8, B_SOLID_LOW);
	bg->StrokeRoundRect(shape, 8, 8);
	p10 = bg->EndPicture();

	BPicture* p20;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(LightGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->FillRoundRect(shape, 8, 8, B_SOLID_LOW);
	p20 = bg->EndPicture();

	BPicture* p30;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(LightGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->StrokeRoundRect(shape, 8, 8);
	p30 = bg->EndPicture();

	BPicture* p11;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->FillRoundRect(shape, 8, 8, B_SOLID_LOW);
	bg->StrokeRoundRect(shape, 8, 8);
	p11 = bg->EndPicture();

	BPicture* p21;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->FillRoundRect(shape, 8, 8, B_SOLID_LOW);
	p21 = bg->EndPicture();

	BPicture* p31;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->StrokeRoundRect(shape, 8, 8);
	p31 = bg->EndPicture();

	SetViewColor(LightGrey);
	delete picWindow;

	pT1 = new BPictureButton(
		BRect(4, 15, 34, 45), "APVt1", p10, p11, new BMessage('pvT1'), B_TWO_STATE_BUTTON);
	pT2 = new BPictureButton(
		BRect(40, 15, 70, 45), "APVt2", p20, p21, new BMessage('pvT2'), B_TWO_STATE_BUTTON);
	pT3 = new BPictureButton(
		BRect(76, 15, 106, 45), "APVt3", p30, p31, new BMessage('pvT3'), B_TWO_STATE_BUTTON);
	type->AddChild(pT1);
	type->AddChild(pT2);
	type->AddChild(pT3);
	pT1->SetValue(B_CONTROL_ON);

	rel = new BRadioButton(
		BRect(8, 86, 144, 104), "rel", lstring(350, "Relative Corners"), new BMessage('RRsr'));
	abs = new BRadioButton(
		BRect(8, 107, 144, 125), "abs", lstring(351, "Absolute Corners"), new BMessage('RRsa'));
	rel->SetValue(B_CONTROL_ON);
	AddChild(rel);
	AddChild(abs);

	TabView* bgTab = new TabView(BRect(0, 128, 148, 191), "AttribRoundRect Tab");
	AddChild(bgTab);
	BView* relTab = new BView(BRect(2, TAB_HEIGHT + 4, 126, TAB_HEIGHT + 44), "Rel View",
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	BView* absTab = new BView(BRect(2, TAB_HEIGHT + 4, 126, TAB_HEIGHT + 44), "Abs View",
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	relTab->SetViewColor(LightGrey);
	absTab->SetViewColor(LightGrey);
	bgTab->AddView(relTab, lstring(352, "Relative"));
	bgTab->AddView(absTab, lstring(353, "Absolute"));
	relx = new Slider(BRect(4, 2, 142, 18), 10, "x", 0, 0.5, 0.01, new BMessage('RRRx'),
		B_HORIZONTAL, 0, "%1.2f");
	rely = new Slider(BRect(4, 22, 142, 38), 10, "y", 0, 0.5, 0.01, new BMessage('RRRy'),
		B_HORIZONTAL, 0, "%1.2f");
	absx = new Slider(BRect(4, 2, 142, 18), 10, "x", 0, 200, 1, new BMessage('RRAx'));
	absy = new Slider(BRect(4, 22, 142, 38), 10, "y", 0, 200, 1, new BMessage('RRAy'));
	relx->SetValue(fRadXrel);
	rely->SetValue(fRadYrel);
	absx->SetValue(fRadXabs);
	absy->SetValue(fRadYabs);
	relTab->AddChild(relx);
	relTab->AddChild(rely);
	absTab->AddChild(absx);
	absTab->AddChild(absy);

	fCurrentProperty = 0;
}

AttribRoundRect::~AttribRoundRect() {}

status_t
AttribRoundRect::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Ovals");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribRoundRect::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property)
{
	if (!strcasecmp(property, "PenSize")) {
		fCurrentProperty = PROP_PENSIZE;
		return this;
	}
	if (!strcasecmp(property, "ShapeType") || !strcasecmp(property, "Type")) {
		fCurrentProperty = PROP_TYPE;
		return this;
	}
	if (!strcasecmp(property, "RelativeWidth") || !strcasecmp(property, "RelX")) {
		fCurrentProperty = PROP_RELX;
		return this;
	}
	if (!strcasecmp(property, "RelativeHeight") || !strcasecmp(property, "RelY")) {
		fCurrentProperty = PROP_RELY;
		return this;
	}
	if (!strcasecmp(property, "AbsoluteWidth") || !strcasecmp(property, "AbsX")) {
		fCurrentProperty = PROP_ABSX;
		return this;
	}
	if (!strcasecmp(property, "AbsoluteHeight") || !strcasecmp(property, "AbsY")) {
		fCurrentProperty = PROP_ABSY;
		return this;
	}
	if (!strcasecmp(property, "Corners")) {
		fCurrentProperty = PROP_CORNERS;
		return this;
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribRoundRect::MessageReceived(BMessage* msg)
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
				case PROP_RELX:	 // float, 0 .. 0.5
				{
					float value;
					if (msg->FindFloat("data", &value) == B_OK) {
						if (value >= 0 && value <= 0.5) {
							fRadXrel = value;
							relx->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_RELY:	 // float, 0 .. 0.5
				{
					float value;
					if (msg->FindFloat("data", &value) == B_OK) {
						if (value >= 0 && value <= 0.5) {
							fRadYrel = value;
							rely->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_ABSX:	 // float, 0 .. 200
				{
					int32 value;
					if (msg->FindInt32("data", &value) == B_OK)	 // OK, we'll take int32's too.
					{
						if (value >= 0 && value <= 200) {
							fRadXabs = value;
							absx->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_ABSY:	 // float, 0 .. 200
				{
					int32 value;
					if (msg->FindInt32("data", &value) == B_OK)	 // OK, we'll take int32's too.
					{
						if (value >= 0 && value <= 200) {
							fRadYabs = value;
							absy->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_TYPE:	 // FilledOutline, Filled, Outline
				{
					const char* name;
					int value = 0;
					if (msg->FindString("data", &name) == B_OK) {
						if (!strcasecmp(name, "FilledOutline")) {
							pT1->SetValue(B_CONTROL_ON);
							pT2->SetValue(B_CONTROL_OFF);
							pT3->SetValue(B_CONTROL_OFF);
							value = RRECT_OUTFILL;
						} else if (!strcasecmp(name, "Filled")) {
							pT1->SetValue(B_CONTROL_OFF);
							pT2->SetValue(B_CONTROL_ON);
							pT3->SetValue(B_CONTROL_OFF);
							value = RRECT_FILL;
						} else if (!strcasecmp(name, "Outline")) {
							pT1->SetValue(B_CONTROL_OFF);
							pT2->SetValue(B_CONTROL_OFF);
							pT3->SetValue(B_CONTROL_ON);
							value = RRECT_OUTLINE;
						}
						if (value) {
							fType = value;
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						} else {
							// Error report...
						}
					}
					break;
				}
				case PROP_CORNERS:	// Relative, Absolute
				{
					const char* name;
					int value = 0;
					if (msg->FindString("data", &name) == B_OK) {
						if (!strcasecmp(name, "Relative")) {
							rel->SetValue(B_CONTROL_ON);
							value = RRECT_RELATIVE;
						} else if (!strcasecmp(name, "Absolute")) {
							abs->SetValue(B_CONTROL_ON);
							value = RRECT_ABSOLUTE;
						}
						if (value) {
							fType = value;
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
		case 'ALpc':
			fPenSize = msg->FindFloat("value");
			break;
		case 'pvT1':
			fType = RRECT_OUTFILL;
			pT1->SetValue(B_CONTROL_ON);
			pT2->SetValue(B_CONTROL_OFF);
			pT3->SetValue(B_CONTROL_OFF);
			break;
		case 'pvT2':
			fType = RRECT_FILL;
			pT1->SetValue(B_CONTROL_OFF);
			pT2->SetValue(B_CONTROL_ON);
			pT3->SetValue(B_CONTROL_OFF);
			break;
		case 'pvT3':
			fType = RRECT_OUTLINE;
			pT1->SetValue(B_CONTROL_OFF);
			pT2->SetValue(B_CONTROL_OFF);
			pT3->SetValue(B_CONTROL_ON);
			break;
		case 'RRRx':
			fRadXrel = msg->FindFloat("value");
			break;
		case 'RRRy':
			fRadYrel = msg->FindFloat("value");
			break;
		case 'RRAx':
			fRadXabs = msg->FindFloat("value");
			break;
		case 'RRAy':
			fRadYabs = msg->FindFloat("value");
			break;
		case 'RRsr':
			fRadType = RRECT_RELATIVE;
			break;
		case 'RRsa':
			fRadType = RRECT_ABSOLUTE;
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}
