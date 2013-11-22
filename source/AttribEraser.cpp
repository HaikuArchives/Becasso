#include "AttribEraser.h"
#include "Slider.h"
#include "Colors.h"
#include <Box.h>
#include <Window.h>
#include <string.h>
#include <stdio.h>
#include "Settings.h"

static property_info prop_list[] = {
	{"XSize|Width|HSize", SET, DIRECT, "float: 1 .. 50"},
	{"YSize|Height|VSize", SET, DIRECT, "float: 1 .. 50"},
	{"Shape|Type", SET, DIRECT, "string: Ellipse, Rectangle"},
	0
};

AttribEraser::AttribEraser ()
: AttribView (BRect (0, 0, 126, 102), lstring (23, "Eraser"))
{
	SetViewColor (LightGrey);
	BBox *type = new BBox (BRect (8, 8, 119, 58), "type");
	type->SetLabel (lstring (333, "Shape"));
	AddChild (type);
	xSlid = new Slider (BRect (8, 62, 119, 78), 45, lstring (334, "H Size"), 1, 50, 1, new BMessage ('AEhs'));
	ySlid = new Slider (BRect (8, 82, 119, 98), 45, lstring (335, "V Size"), 1, 50, 1, new BMessage ('AEvs'));
	xSlid->SetValue (16);
	ySlid->SetValue (16);
	fXSize = 16;
	fYSize = 16;
	AddChild (xSlid);
	AddChild (ySlid);

	BRect shape = BRect (3, 7, 27, 23);

	BWindow *picWindow = new BWindow (BRect (0, 0, 100, 100), "Temp Pic Window", B_BORDERED_WINDOW, uint32 (NULL), uint32 (NULL));
	BView *bg = new BView (BRect (0, 0, 100, 100), "Temp Pic View", uint32 (NULL), uint32 (NULL));
	picWindow->AddChild (bg);
	
	BPicture *p10;
	bg->BeginPicture (new BPicture);
	bg->SetLowColor (LightGrey);
	bg->FillRect (BRect (0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor (Black);
	bg->SetLowColor (White);
	bg->FillRect (shape, B_SOLID_LOW);
	bg->StrokeRect (shape);
	p10 = bg->EndPicture();
	
	BPicture *p20;
	bg->BeginPicture (new BPicture);
	bg->SetLowColor (LightGrey);
	bg->FillRect (BRect (0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor (Black);
	bg->SetLowColor (White);
	bg->FillEllipse (shape, B_SOLID_LOW);
	bg->StrokeEllipse (shape);
	p20 = bg->EndPicture();

	BPicture *p11;
	bg->BeginPicture (new BPicture);
	bg->SetLowColor (DarkGrey);
	bg->FillRect (BRect (0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor (Black);
	bg->SetLowColor (White);
	bg->FillRect (shape, B_SOLID_LOW);
	bg->StrokeRect (shape);
	p11 = bg->EndPicture();
	
	BPicture *p21;
	bg->BeginPicture (new BPicture);
	bg->SetLowColor (DarkGrey);
	bg->FillRect (BRect (0, 0, 32, 32), B_SOLID_LOW);
	bg->SetHighColor (Black);
	bg->SetLowColor (White);
	bg->FillEllipse (shape, B_SOLID_LOW);
	bg->StrokeEllipse (shape);
	p21 = bg->EndPicture();
	
	delete picWindow;
	
	eT1 = new BPictureButton (BRect (22, 15, 52, 45), "eT1", p10, p11, new BMessage ('peT1'));
	eT2 = new BPictureButton (BRect (58, 15, 88, 45), "eT2", p20, p21, new BMessage ('peT2'));
	type->AddChild (eT1);
	type->AddChild (eT2);
	eT1->SetValue (B_CONTROL_ON);
	fType = ERASER_RECT;
	fCurrentProperty = 0;
}


AttribEraser::~AttribEraser ()
{
//	printf ("~AttribEraser\n");
}

status_t AttribEraser::GetSupportedSuites (BMessage *message)
{
	message->AddString ("suites", "suite/x-sum-becasso-Eraser");
	BPropertyInfo info (prop_list);
	message->AddFlat ("messages", &info);
	return AttribView::GetSupportedSuites (message);
}

BHandler *AttribEraser::ResolveSpecifier (BMessage *message, int32 index, BMessage *specifier, int32 command, const char *property)
{
	if (!strcasecmp (property, "XSize") || !strcasecmp (property, "Width") || !strcasecmp (property, "HSize"))
	{
		fCurrentProperty = PROP_XSIZE;
		return this;
	}
	if (!strcasecmp (property, "YSize") || !strcasecmp (property, "Height") || !strcasecmp (property, "VSize"))
	{
		fCurrentProperty = PROP_YSIZE;
		return this;
	}
	if (!strcasecmp (property, "Shape") || !strcasecmp (property, "Type"))
	{
		fCurrentProperty = PROP_TYPE;
		return this;
	}
	return inherited::ResolveSpecifier (message, index, specifier, command, property);
}

void AttribEraser::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
	case B_GET_PROPERTY:
	{
		switch (fCurrentProperty)
		{
		}
		fCurrentProperty = 0;
		inherited::MessageReceived (msg);
		break;
	}
	case B_SET_PROPERTY:
	{
		switch (fCurrentProperty)
		{
		case PROP_XSIZE:	// float, 1 .. 50
		{
			float value;
			int32 ivalue;
			bool floatvalid = false;
			if (msg->FindInt32 ("data", &ivalue) == B_OK)	// OK, we'll take int32's too.
			{
				value = ivalue;
				floatvalid = true;
			}
			if (floatvalid || msg->FindFloat ("data", &value) == B_OK)
			{
				if (value >= 1 && value <= 50)
				{
					xSlid->SetValue (value);
					fXSize = value;
					if (msg->IsSourceWaiting())
					{
						BMessage error (B_REPLY);
						error.AddInt32 ("error", B_NO_ERROR);
						msg->SendReply (&error);
					}
				}
			}
			break;
		}
		case PROP_YSIZE:	// float, 1 .. 50
		{
			float value;
			int32 ivalue;
			bool floatvalid = false;
			if (msg->FindInt32 ("data", &ivalue) == B_OK)	// OK, we'll take int32's too.
			{
				value = ivalue;
				floatvalid = true;
			}
			if (floatvalid || msg->FindFloat ("data", &value) == B_OK)
			{
				if (value >= 1 && value <= 50)
				{
					fYSize = value;
					ySlid->SetValue (value);
					if (msg->IsSourceWaiting())
					{
						BMessage error (B_REPLY);
						error.AddInt32 ("error", B_NO_ERROR);
						msg->SendReply (&error);
					}
				}
			}
			break;
		}
		case PROP_TYPE:	// Ellipse, Rectangle
		{
			const char *name;
			int value = 0;
			if (msg->FindString ("data", &name) == B_OK)
			{
				if (!strcasecmp (name, "Ellipse"))
				{
					eT1->SetValue (B_CONTROL_OFF);
					eT2->SetValue (B_CONTROL_ON);
					value = ERASER_ELLIPSE;
				}
				else if (!strcasecmp (name, "Rectangle"))
				{
					eT1->SetValue (B_CONTROL_ON);
					eT2->SetValue (B_CONTROL_OFF);
					value = ERASER_RECT;
				}
				if (value)
				{
					fType = value;
					if (msg->IsSourceWaiting())
					{
						BMessage error (B_REPLY);
						error.AddInt32 ("error", B_NO_ERROR);
						msg->SendReply (&error);
					}
				}
				else
				{
					// Error report...
				}
			}
		}
		fCurrentProperty = 0;
		break;
		}
		inherited::MessageReceived (msg);
		break;
	}
	case 'AEhs':
		fXSize = msg->FindFloat ("value");
		break;
	case 'AEvs':
		fYSize = msg->FindFloat ("value");
		break;
	case 'peT1':
		eT1->SetValue (B_CONTROL_ON);
		eT2->SetValue (B_CONTROL_OFF);
		fType = ERASER_RECT;
		break;
	case 'peT2':
		eT1->SetValue (B_CONTROL_OFF);
		eT2->SetValue (B_CONTROL_ON);
		fType = ERASER_ELLIPSE;
		break;
	default:
		inherited::MessageReceived (msg);
		break;
	}
}