#include "AttribCircle.h"
#include <Box.h>
#include <Rect.h>
#include <Window.h>
#include <string.h>
#include "Colors.h"
#include "Settings.h"

static property_info prop_list[] = {{"PenSize", SET, DIRECT, "float: 0 .. 50"},
	{"ShapeType|Type", SET, DIRECT, "string: FilledOutline, Filled, Outline"},
	{"FixPoint|FirstClick", SET, DIRECT, "string: Center, Perimeter"}, 0};

AttribCircle::AttribCircle() : AttribView(BRect(0, 0, 148, 148), lstring(33, "Circles"))
{
	SetViewColor(LightGrey);
	lSlid = new Slider(
		BRect(8, 8, 140, 26), 60, lstring(310, "Pen Size"), 1, 50, 1, new BMessage('ALpc'));
	AddChild(lSlid);
	fType = CIRCLE_OUTFILL;
	fFirst = FIXES_CENTER;
	fPenSize = 1;
	BBox* type = new BBox(BRect(18, 32, 130, 82), "type");
	type->SetLabel(lstring(311, "Type"));
	AddChild(type);
	BBox* first = new BBox(BRect(18, 90, 130, 140), "first");
	first->SetLabel(lstring(312, "First Click"));
	AddChild(first);

	BRect shape = BRect(4, 4, 26, 26);
	BWindow* picWindow = new BWindow(
		BRect(0, 0, 100, 100), "Temp Pic Window", B_BORDERED_WINDOW, uint32(NULL), uint32(NULL));
	BView* bg = new BView(BRect(0, 0, 100, 100), "Temp Pic View", uint32(NULL), uint32(NULL));
	picWindow->AddChild(bg);

	BPicture* p10;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(LightGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetLowColor(White);
	bg->FillEllipse(shape, B_SOLID_LOW);
	bg->StrokeEllipse(shape);
	p10 = bg->EndPicture();

	BPicture* p20;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(LightGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->FillEllipse(shape, B_SOLID_LOW);
	p20 = bg->EndPicture();

	BPicture* p30;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(LightGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->StrokeEllipse(shape);
	p30 = bg->EndPicture();

	BPicture* p11;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->FillEllipse(shape, B_SOLID_LOW);
	bg->StrokeEllipse(shape);
	p11 = bg->EndPicture();

	BPicture* p21;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->FillEllipse(shape, B_SOLID_LOW);
	p21 = bg->EndPicture();

	BPicture* p31;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->StrokeEllipse(shape);
	p31 = bg->EndPicture();

	BPicture* p40;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(LightGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->StrokeEllipse(shape);
	bg->SetHighColor(Red);
	bg->StrokeLine(BPoint(12, 15), BPoint(18, 15));
	bg->StrokeLine(BPoint(15, 12), BPoint(15, 18));
	p40 = bg->EndPicture();

	BPicture* p41;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->StrokeEllipse(shape);
	bg->SetHighColor(Red);
	bg->StrokeLine(BPoint(12, 15), BPoint(18, 15));
	bg->StrokeLine(BPoint(15, 12), BPoint(15, 18));
	p41 = bg->EndPicture();

	BPicture* p50;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(LightGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->StrokeEllipse(shape);
	bg->SetHighColor(Red);
	bg->StrokeLine(BPoint(5, 7), BPoint(11, 7));
	bg->StrokeLine(BPoint(8, 4), BPoint(8, 10));
	p50 = bg->EndPicture();

	BPicture* p51;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->StrokeEllipse(shape);
	bg->SetHighColor(Red);
	bg->StrokeLine(BPoint(5, 7), BPoint(11, 7));
	bg->StrokeLine(BPoint(8, 4), BPoint(8, 10));
	p51 = bg->EndPicture();

	delete picWindow;

	pT1 = new BPictureButton(
		BRect(4, 15, 34, 45), "APVt1", p10, p11, new BMessage('pvT1'), B_TWO_STATE_BUTTON);
	pT2 = new BPictureButton(
		BRect(40, 15, 70, 45), "APVt2", p20, p21, new BMessage('pvT2'), B_TWO_STATE_BUTTON);
	pT3 = new BPictureButton(
		BRect(76, 15, 106, 45), "APVt3", p30, p31, new BMessage('pvT3'), B_TWO_STATE_BUTTON);
	pF1 = new BPictureButton(
		BRect(22, 15, 52, 45), "ACVt1", p40, p41, new BMessage('pvF1'), B_TWO_STATE_BUTTON);
	pF2 = new BPictureButton(
		BRect(58, 15, 88, 45), "ACTt2", p50, p51, new BMessage('pvF2'), B_TWO_STATE_BUTTON);
	type->AddChild(pT1);
	type->AddChild(pT2);
	type->AddChild(pT3);
	first->AddChild(pF1);
	first->AddChild(pF2);
	pT1->SetValue(B_CONTROL_ON);
	pF1->SetValue(B_CONTROL_ON);

	fCurrentProperty = 0;
}

AttribCircle::~AttribCircle() {}

status_t
AttribCircle::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Circle");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribCircle::ResolveSpecifier(
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
	if (!strcasecmp(property, "FixPoint") || !strcasecmp(property, "FirstClick")) {
		fCurrentProperty = PROP_FIXPOINT;
		return this;
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribCircle::MessageReceived(BMessage* msg)
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
				case PROP_TYPE:	 // FilledOutline, Filled, Outline
				{
					const char* name;
					int value = 0;
					if (msg->FindString("data", &name) == B_OK) {
						if (!strcasecmp(name, "FilledOutline")) {
							pT1->SetValue(B_CONTROL_ON);
							pT2->SetValue(B_CONTROL_OFF);
							pT3->SetValue(B_CONTROL_OFF);
							value = CIRCLE_OUTFILL;
						} else if (!strcasecmp(name, "Filled")) {
							pT1->SetValue(B_CONTROL_OFF);
							pT2->SetValue(B_CONTROL_ON);
							pT3->SetValue(B_CONTROL_OFF);
							value = CIRCLE_FILL;
						} else if (!strcasecmp(name, "Outline")) {
							pT1->SetValue(B_CONTROL_OFF);
							pT2->SetValue(B_CONTROL_OFF);
							pT3->SetValue(B_CONTROL_ON);
							value = CIRCLE_OUTLINE;
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
				case PROP_FIXPOINT:	 // Center, Perimeter
				{
					const char* name;
					int value = 0;
					if (msg->FindString("data", &name) == B_OK) {
						if (!strcasecmp(name, "Center")) {
							pF1->SetValue(B_CONTROL_ON);
							pF2->SetValue(B_CONTROL_OFF);
							value = FIXES_CENTER;
						} else if (!strcasecmp(name, "Perimeter")) {
							pF1->SetValue(B_CONTROL_OFF);
							pF2->SetValue(B_CONTROL_ON);
							value = FIXES_PERIMETER;
						}
						if (value) {
							fFirst = value;
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
			fType = CIRCLE_OUTFILL;
			pT1->SetValue(B_CONTROL_ON);
			pT2->SetValue(B_CONTROL_OFF);
			pT3->SetValue(B_CONTROL_OFF);
			break;
		case 'pvT2':
			fType = CIRCLE_FILL;
			pT1->SetValue(B_CONTROL_OFF);
			pT2->SetValue(B_CONTROL_ON);
			pT3->SetValue(B_CONTROL_OFF);
			break;
		case 'pvT3':
			fType = CIRCLE_OUTLINE;
			pT1->SetValue(B_CONTROL_OFF);
			pT2->SetValue(B_CONTROL_OFF);
			pT3->SetValue(B_CONTROL_ON);
			break;
		case 'pvF1':
			fFirst = FIXES_CENTER;
			pF1->SetValue(B_CONTROL_ON);
			pF2->SetValue(B_CONTROL_OFF);
			break;
		case 'pvF2':
			fFirst = FIXES_PERIMETER;
			pF1->SetValue(B_CONTROL_OFF);
			pF2->SetValue(B_CONTROL_ON);
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}