#include "AttribRect.h"
#include <Box.h>
#include <Rect.h>
#include <Window.h>
#include <string.h>
#include "Colors.h"
#include "Settings.h"

static property_info prop_list[] = {{"PenSize", SET, DIRECT, "float: 0 .. 50"},
	{"ShapeType|Type", SET, DIRECT, "string: FilledOutline, Filled, Outline"}, 0};

AttribRect::AttribRect() : AttribView(BRect(0, 0, 148, 90), lstring(31, "Rectangles"))
{
	SetViewColor(LightGrey);
	lSlid = new Slider(
		BRect(8, 8, 140, 26), 60, lstring(310, "Pen Size"), 1, 50, 1, new BMessage('ALpc'));
	AddChild(lSlid);
	fType = RECT_OUTFILL;
	fPenSize = 1;
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
	bg->FillRect(shape, B_SOLID_LOW);
	bg->StrokeRect(shape);
	p10 = bg->EndPicture();

	BPicture* p20;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(LightGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->FillRect(shape, B_SOLID_LOW);
	p20 = bg->EndPicture();

	BPicture* p30;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(LightGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->StrokeRect(shape);
	p30 = bg->EndPicture();

	BPicture* p11;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->FillRect(shape, B_SOLID_LOW);
	bg->StrokeRect(shape);
	p11 = bg->EndPicture();

	BPicture* p21;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->FillRect(shape, B_SOLID_LOW);
	p21 = bg->EndPicture();

	BPicture* p31;
	bg->BeginPicture(new BPicture);
	bg->SetLowColor(DarkGrey);
	bg->FillRect(BRect(0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor(Black);
	bg->SetLowColor(White);
	bg->StrokeRect(shape);
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
	fCurrentProperty = 0;
}

AttribRect::~AttribRect() {}

status_t
AttribRect::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Rectangles");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribRect::ResolveSpecifier(
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
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribRect::MessageReceived(BMessage* msg)
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
							value = RECT_OUTFILL;
						} else if (!strcasecmp(name, "Filled")) {
							pT1->SetValue(B_CONTROL_OFF);
							pT2->SetValue(B_CONTROL_ON);
							pT3->SetValue(B_CONTROL_OFF);
							value = RECT_FILL;
						} else if (!strcasecmp(name, "Outline")) {
							pT1->SetValue(B_CONTROL_OFF);
							pT2->SetValue(B_CONTROL_OFF);
							pT3->SetValue(B_CONTROL_ON);
							value = RECT_OUTLINE;
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
			fType = RECT_OUTFILL;
			pT1->SetValue(B_CONTROL_ON);
			pT2->SetValue(B_CONTROL_OFF);
			pT3->SetValue(B_CONTROL_OFF);
			break;
		case 'pvT2':
			fType = RECT_FILL;
			pT1->SetValue(B_CONTROL_OFF);
			pT2->SetValue(B_CONTROL_ON);
			pT3->SetValue(B_CONTROL_OFF);
			break;
		case 'pvT3':
			fType = RECT_OUTLINE;
			pT1->SetValue(B_CONTROL_OFF);
			pT2->SetValue(B_CONTROL_OFF);
			pT3->SetValue(B_CONTROL_ON);
			break;
		default:
			inherited::MessageReceived(msg);
			break;
	}
}