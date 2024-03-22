#include <TextControl.h>
#include <Box.h>
#include <Rect.h>
#include <View.h>
#include <Button.h>
#include <PopUpMenu.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <StringView.h>
#include "SizeWindow.h"
#include "Colors.h"
#include <stdio.h>
#include "Settings.h"

SizeWindow::SizeWindow(int32 _h, int32 _w, int32 _c)
	: BWindow(
		  BRect(100, 100, 300, 272), "", B_MODAL_WINDOW, B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS
	  )
{
	char hS[16], wS[16], rD[16];
	fRez = 72;
	fH = _w;
	fV = _h;
	fColor = _c;
	fHUnit = UNIT_PIXELS;
	fVUnit = UNIT_PIXELS;

	BBox* text = new BBox("SW box");
	text->SetLabel(lstring(180, "Canvas Size"));

	sprintf(hS, "%li", _h);
	sprintf(wS, "%li", _w);
	sprintf(rD, "%li", fRez);

	newWidth = new BTextControl("NewWidth", lstring(181, "Width"), wS, new BMessage('Swdt'));
	newHeight = new BTextControl("NewHeight", lstring(182, "Height"), hS, new BMessage('Shgt'));
	rDPI = new BTextControl("rez", lstring(186, "Resolution"), rD, new BMessage('Srez'));

	newHeight->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	newWidth->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	rDPI->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	BStringView* dpiV = new BStringView("dpiV", "dpi");
	BPopUpMenu* hPU = new BPopUpMenu("");
	hPU->AddItem(new BMenuItem(lstring(187, "pixels"), new BMessage('h_px')));
	hPU->AddItem(new BMenuItem(lstring(188, "in"), new BMessage('h_in')));
	hPU->AddItem(new BMenuItem(lstring(189, "cm"), new BMessage('h_cm')));
	hPU->ItemAt(0)->SetMarked(true);
	BMenuField* hMF = new BMenuField("hUnit", NULL, hPU);
	BPopUpMenu* vPU = new BPopUpMenu("");
	vPU->AddItem(new BMenuItem(lstring(187, "pixels"), new BMessage('v_px')));
	vPU->AddItem(new BMenuItem(lstring(188, "in"), new BMessage('v_in')));
	vPU->AddItem(new BMenuItem(lstring(189, "cm"), new BMessage('v_cm')));
	vPU->ItemAt(0)->SetMarked(true);
	BMenuField* vMF = new BMenuField("vUnit", NULL, vPU);

	BView* view = new BView("SW bg", B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BLayoutBuilder::Grid<>(view, 2.0, 2.0) // FIX
		.SetInsets(
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING
		)
		.Add(newWidth->CreateLabelLayoutItem(), 0, 0)
		.Add(newWidth->CreateTextViewLayoutItem(), 1, 0)
		.Add(hMF, 2, 0)
		.Add(newHeight->CreateLabelLayoutItem(), 0, 1)
		.Add(newHeight->CreateTextViewLayoutItem(), 1, 1)
		.Add(vMF, 2, 1)
		.Add(rDPI->CreateLabelLayoutItem(), 0, 2)
		.Add(rDPI->CreateTextViewLayoutItem(), 1, 2)
		.Add(dpiV, 2, 2);
	text->AddChild(view);

	BButton* createButton = new BButton("SW open", lstring(183, "Create"), new BMessage('Sopn'));
	BButton* cancelButton = new BButton("SW cancel", lstring(131, "Cancel"), new BMessage('Scnc'));
	createButton->MakeDefault(true);

	BPopUpMenu* cPU = new BPopUpMenu("");
	cPU->AddItem(new BMenuItem(lstring(407, "Current Foreground"), new BMessage('cFG ')));
	cPU->AddItem(new BMenuItem(lstring(408, "Current Background"), new BMessage('cBG ')));
	cPU->AddItem(new BMenuItem(lstring(409, "Transparent"), new BMessage('cTR ')));
	cPU->ItemAt(fColor)->SetMarked(true);
	BMenuField* cMF = new BMenuField("color", lstring(406, "Color: "), cPU);


	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING
		)
		.Add(text)
		.Add(cMF)
		.AddGroup(B_HORIZONTAL)
		.AddGlue()
		.Add(cancelButton)
		.Add(createButton);

	fStatus = 0;
}

SizeWindow::~SizeWindow() {}

int32
SizeWindow::Go()
{
	// not too happy with this implementation...
	bool ishidden = false;
	Show();
	while (!ishidden) {
		Lock();
		ishidden = IsHidden();
		Unlock();
		snooze(50000);
	}
	return (fStatus);
}

void
SizeWindow::recalc()
{
	char h[16], v[16];
	switch (fHUnit) {
	case UNIT_PIXELS:
		sprintf(h, "%li", fH);
		newWidth->SetText(h);
		break;
	case UNIT_INCH:
		sprintf(h, "%.2f", (float)fH / fRez + .005);
		newWidth->SetText(h);
		break;
	case UNIT_CM:
		sprintf(h, "%.2f", (float)fH / fRez * 2.54 + .005);
		newWidth->SetText(h);
		break;
	default:
		fprintf(stderr, "SizeWindow: Unknown Unit (H)\n");
	}
	switch (fVUnit) {
	case UNIT_PIXELS:
		sprintf(v, "%li", fV);
		newHeight->SetText(v);
		break;
	case UNIT_INCH:
		sprintf(v, "%.2f", (float)fV / fRez + .005);
		newHeight->SetText(v);
		break;
	case UNIT_CM:
		sprintf(v, "%.2f", (float)fV / fRez * 2.54 + .005);
		newHeight->SetText(v);
		break;
	default:
		fprintf(stderr, "SizeWindow: Unknown Unit (V)\n");
	}
}

void
SizeWindow::readvalues()
{
	switch (fHUnit) {
	case UNIT_PIXELS:
		fH = atoi(newWidth->Text());
		break;
	case UNIT_INCH:
		fH = int32(fRez * atof(newWidth->Text()));
		break;
	case UNIT_CM:
		fH = int32(fRez / 2.54 * atof(newWidth->Text()));
		break;
	}

	switch (fVUnit) {
	case UNIT_PIXELS:
		fV = atoi(newHeight->Text());
		break;
	case UNIT_INCH:
		fV = int32(fRez * atof(newHeight->Text()));
		break;
	case UNIT_CM:
		fV = int32(fRez / 2.54 * atof(newHeight->Text()));
		break;
	}
}

void
SizeWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case 'Scnc':
		fStatus = 0;
		Hide();
		break;
	case 'Sopn':
		fStatus = 1;
		readvalues();
		Hide();
		break;
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
		fRez = atoi(rDPI->Text());
		readvalues();
		break;
	case 'cFG ':
		fColor = COLOR_FG;
		break;
	case 'cBG ':
		fColor = COLOR_BG;
		break;
	case 'cTR ':
		fColor = COLOR_TR;
		break;
	default:
		inherited::MessageReceived(message);
		break;
	}
}
