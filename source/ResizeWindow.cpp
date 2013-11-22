#include <TextControl.h>
#include <Box.h>
#include <Rect.h>
#include <View.h>
#include <Button.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <StringView.h>
#include "ResizeWindow.h"
#include "Colors.h"
#include <stdio.h>
#include "Settings.h"

ResizeWindow::ResizeWindow (CanvasWindow *target, const char *title, int32 _h, int32 _w)
: BWindow (BRect (100, 100, 300, 248), title, B_TITLED_WINDOW, B_NOT_RESIZABLE)
{
	char hS[16], wS[16], rD[16];
	fTarget = target;
	fRez = 72;
	fH = _w;
	fV = _h;
	fHUnit = UNIT_PIXELS;
	fVUnit = UNIT_PIXELS;
	BRect bgFrame, cancelFrame, openFrame, textFrame, heightFrame, widthFrame, rezFrame;
	bgFrame = Bounds();
	BView *bg = new BView (bgFrame, "SW bg", B_FOLLOW_ALL, B_WILL_DRAW);
	bg->SetViewColor (LightGrey);
	AddChild (bg);

	textFrame.Set (8, 8, 192, 102);
	BBox *text = new BBox (textFrame, "SW box");
	text->SetLabel (lstring (180, "Canvas Size"));
	bg->AddChild (text);
	
#if 0
	textFrame.Set (8, 80, 192, 120);
	BBox *rez = new BBox (textFrame, "SW r box");
	rez->SetLabel (lstring (186, "Resolution"));
	bg->AddChild (rez);
#endif

	sprintf (hS, "%li", _h);
	sprintf (wS, "%li", _w);
	sprintf (rD, "%li", fRez);
	widthFrame.Set (8, 15, 120, 33);
	heightFrame.Set (8, 39, 120, 57);
	rezFrame.Set (8, 63, 120, 81);
	newWidth = new BTextControl (widthFrame, "NewWidth", lstring (181, "Width"), wS, new BMessage ('Swdt'));
	newHeight = new BTextControl (heightFrame, "NewHeight", lstring (182, "Height"), hS, new BMessage ('Shgt'));
	rDPI = new BTextControl (rezFrame, "rez", lstring (186, "Resolution"), rD, new BMessage ('Srez'));
	newHeight->SetAlignment (B_ALIGN_RIGHT, B_ALIGN_LEFT);
	newWidth->SetAlignment (B_ALIGN_RIGHT, B_ALIGN_LEFT);
	rDPI->SetAlignment (B_ALIGN_RIGHT, B_ALIGN_LEFT);
	BStringView *dpiV = new BStringView (BRect (rezFrame.right + 4, rezFrame.top, rezFrame.right + 40, rezFrame.bottom),
		"dpiV", "dpi");
	BPopUpMenu *hPU = new BPopUpMenu ("");
	hPU->AddItem (new BMenuItem (lstring (187, "pixels"), new BMessage ('h_px')));
	hPU->AddItem (new BMenuItem (lstring (188, "in"), new BMessage ('h_in')));
	hPU->AddItem (new BMenuItem (lstring (189, "cm"), new BMessage ('h_cm')));
	hPU->ItemAt(0)->SetMarked (true);
	BMenuField *hMF = new BMenuField (BRect (124, 14, 180, 30), "hUnit", NULL, hPU);
	BPopUpMenu *vPU = new BPopUpMenu ("");
	vPU->AddItem (new BMenuItem (lstring (187, "pixels"), new BMessage ('v_px')));
	vPU->AddItem (new BMenuItem (lstring (188, "in"), new BMessage ('v_in')));
	vPU->AddItem (new BMenuItem (lstring (189, "cm"), new BMessage ('v_cm')));
	vPU->ItemAt(0)->SetMarked (true);
	BMenuField *vMF = new BMenuField (BRect (124, 38, 180, 54), "vUnit", NULL, vPU);
		text->AddChild (newWidth);
	text->AddChild (hMF);
	text->AddChild (newHeight);
	text->AddChild (vMF);
	text->AddChild (rDPI);
	text->AddChild (dpiV);
	
	cancelFrame.Set (82, 112, 134, 136);
	openFrame.Set (140, 112, 192, 136);
	BButton *cancel = new BButton (cancelFrame, "RSW cancel", lstring (131, "Cancel"), new BMessage ('RScn'));
	BButton *open = new BButton (openFrame, "RSW ok", lstring (136, "OK"), new BMessage ('RSok')); 
	open->MakeDefault (true);
	bg->AddChild (cancel);
	bg->AddChild (open);
	
	fStatus = 0;
}

ResizeWindow::~ResizeWindow ()
{
}

void ResizeWindow::recalc ()
{
	char h[16], v[16];
	switch (fHUnit)
	{
	case UNIT_PIXELS:
		sprintf (h, "%li", fH);
		newWidth->SetText (h);
		break;
	case UNIT_INCH:
		sprintf (h, "%.2f", (float) fH/fRez + .005);
		newWidth->SetText (h);
		break;
	case UNIT_CM:
		sprintf (h, "%.2f", (float) fH/fRez*2.54 + .005);
		newWidth->SetText (h);
		break;
	default:
		fprintf (stderr, "SizeWindow: Unknown Unit (H)\n");
	}
	switch (fVUnit)
	{
	case UNIT_PIXELS:
		sprintf (v, "%li", fV);
		newHeight->SetText (v);
		break;
	case UNIT_INCH:
		sprintf (v, "%.2f", (float) fV/fRez + .005);
		newHeight->SetText (v);
		break;
	case UNIT_CM:
		sprintf (v, "%.2f", (float) fV/fRez*2.54 + .005);
		newHeight->SetText (v);
		break;
	default:
		fprintf (stderr, "SizeWindow: Unknown Unit (V)\n");
	}
}

void ResizeWindow::readvalues ()
{
	switch (fHUnit)
	{
	case UNIT_PIXELS:
		fH = atoi (newWidth->Text());
		break;
	case UNIT_INCH:
		fH = int32 (fRez * atof (newWidth->Text()));
		break;
	case UNIT_CM:
		fH = int32 (fRez / 2.54 * atof (newWidth->Text()));
		break;
	}

	switch (fVUnit)
	{
	case UNIT_PIXELS:
		fV = atoi (newHeight->Text());
		break;
	case UNIT_INCH:
		fV = int32 (fRez * atof (newHeight->Text()));
		break;
	case UNIT_CM:
		fV = int32 (fRez / 2.54 * atof (newHeight->Text()));
		break;
	}
}

void ResizeWindow::MessageReceived (BMessage *message)
{
	switch (message->what)
	{
	case 'RScn':
		Quit();
		break;
	case 'RSok':
	{
		readvalues();
		BMessage msg ('rszt');
		msg.AddInt32 ("width", fH);
		msg.AddInt32 ("height", fV);
		fTarget->PostMessage (&msg);
		Quit();
		break;
	}
	case 'h_px':
		fHUnit = UNIT_PIXELS;
		recalc();
		break;
	case 'h_in':
		fHUnit = UNIT_INCH;
		recalc();
		break;
	case 'h_cm':
		fHUnit = UNIT_CM;
		recalc();
		break;
	case 'v_px':
		fVUnit = UNIT_PIXELS;
		recalc();
		break;
	case 'v_in':
		fVUnit = UNIT_INCH;
		recalc();
		break;
	case 'v_cm':
		fVUnit = UNIT_CM;
		recalc();
		break;
	case 'Swdt':
	case 'Shgt':
		readvalues();
		break;
	case 'Srez':
		fRez = atoi (rDPI->Text());
		readvalues();
		break;
	default:
		inherited::MessageReceived(message);
		break;
	}
}
