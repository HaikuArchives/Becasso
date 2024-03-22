#include <Message.h>
#include <Menu.h>
#include <MenuItem.h>
#include <View.h>
#include <Button.h>
#include <RadioButton.h>
#include <Box.h>
#include "TabView.h"
#include "ColorWindow.h"
#include "Colors.h"
#include "hsv.h"
#include <stdio.h>
#include <stdlib.h>
#include "Settings.h"

ColorWindow::ColorWindow(BRect frame, const char* name, ColorMenuButton* but)
	: BWindow(frame, name, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	button = but;
	ob = 0;
	BRect menubarFrame, bgFrame, bg2Frame, squareFrame;
	BRect rFrame, gFrame, bFrame, hFrame, sFrame, vFrame, tFrame;
	BRect setFrame, curFrame, matchFrame, distFrame;
	menubarFrame.Set(0, 0, 0, 0);
	menubar = new BMenuBar(menubarFrame, "ColorMenuBar");
	BMenu* fileMenu = new BMenu(lstring(10, "File"));
	fileMenu->AddItem(new BMenuItem(lstring(190, "Save Palette…"), new BMessage('Csav')));
	fileMenu->AddItem(new BMenuItem(lstring(191, "Load Palette…"), new BMessage('Clod')));
	BMenu* generateMenu = new BMenu(lstring(192, "Generate Palette"));
	generateMenu->AddItem(new BMenuItem(lstring(193, "Default"), new BMessage('PGdf')));
	generateMenu->AddItem(new BMenuItem(lstring(194, "Grey"), new BMessage('PGG')));
	generateMenu->AddItem(new BMenuItem(lstring(77, "Red"), new BMessage('PGr')));
	generateMenu->AddItem(new BMenuItem(lstring(78, "Green"), new BMessage('PGg')));
	generateMenu->AddItem(new BMenuItem(lstring(79, "Blue"), new BMessage('PGb')));
	generateMenu->AddItem(new BMenuItem(lstring(80, "Cyan"), new BMessage('PGgb')));
	generateMenu->AddItem(new BMenuItem(lstring(81, "Magenta"), new BMessage('PGrb')));
	generateMenu->AddItem(new BMenuItem(lstring(82, "Yellow"), new BMessage('PGrg')));
	generateMenu->AddItem(new BMenuItem(lstring(195, "Spectrum"), new BMessage('PGsp')));
	generateMenu->AddItem(new BMenuItem(lstring(196, "Heat Scale"), new BMessage('PGht')));
	fileMenu->AddItem(generateMenu, (int32)NULL);

	menubar->AddItem(fileMenu);
	AddChild(menubar);
	menubar->ResizeToPreferred();
	menubarFrame = menubar->Frame();
	bg2Frame.Set(
		0, menubarFrame.Height() + CE_HEIGHT - 145, CE_WIDTH, menubarFrame.Height() + CE_HEIGHT
	);
	BView* bg2 = new BView(bg2Frame, "CE bg2", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	bg2->SetViewColor(LightGrey);
	AddChild(bg2);
	bgFrame.Set(0, menubarFrame.Height() + 1, CE_WIDTH, menubarFrame.Height() + CE_HEIGHT - 146);
	TabView* bgTab = new TabView(bgFrame, "Color Editor bg");
	AddChild(bgTab);
	bgFrame.OffsetTo(B_ORIGIN);
	ResizeTo(CE_WIDTH, CE_HEIGHT + menubarFrame.Height());
	bgFrame.InsetBy(2, 2);
	bgFrame.top += TAB_HEIGHT;
	BView* rgbV = new BView(bgFrame, "RGB View", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	rgbV->SetViewColor(LightGrey);
	bgTab->AddView(rgbV, lstring(197, "RGB Model"));
	BView* hsvV = new BView(bgFrame, "HSV View", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	hsvV->SetViewColor(LightGrey);
	bgTab->AddView(hsvV, lstring(198, "HSV Model"));

	squareFrame.Set(8, 8, 311, 263);
	c = button->color();
	rgbSquare = new RGBSquare(squareFrame, 0, this);
	rgbV->AddChild(rgbSquare);
	rFrame.Set(16, 270, 70, 288);
	gFrame.Set(16, 292, 70, 310);
	bFrame.Set(16, 314, 70, 332);
	BRadioButton* rRB = new BRadioButton(rFrame, "rRB", lstring(77, "Red"), new BMessage('BrbR'));
	BRadioButton* gRB = new BRadioButton(gFrame, "gRB", lstring(78, "Green"), new BMessage('BrbG'));
	BRadioButton* bRB = new BRadioButton(bFrame, "bRB", lstring(79, "Blue"), new BMessage('BrbB'));
	rRB->SetValue(1);
	rgbV->AddChild(rRB);
	rgbV->AddChild(gRB);
	rgbV->AddChild(bRB);
	char rS[16];
	char gS[16];
	char bS[16];
	sprintf(rS, "%i", c.red);
	sprintf(gS, "%i", c.green);
	sprintf(bS, "%i", c.blue);
	rFrame.Set(72, 268, 150, 286);
	gFrame.Set(72, 290, 150, 308);
	bFrame.Set(72, 312, 150, 330);
	rTC = new BTextControl(rFrame, "rTC", "(0–255)", rS, new BMessage('TrbR'));
	gTC = new BTextControl(gFrame, "gTC", "(0–255)", gS, new BMessage('TrbG'));
	bTC = new BTextControl(bFrame, "bTC", "(0–255)", bS, new BMessage('TrbB'));
	rgbV->AddChild(rTC);
	rgbV->AddChild(gTC);
	rgbV->AddChild(bTC);

	hsvSquare = new HSVSquare(squareFrame, this);
	hsvV->AddChild(hsvSquare);
	hsv_color h = rgb2hsv(c);
	char hS[16];
	char sS[16];
	char vS[16];
	sprintf(hS, "%.0f", h.hue);
	sprintf(sS, "%1.3f", h.saturation);
	sprintf(vS, "%1.3f", h.value);
	hFrame.Set(16, 268, 160, 286);
	sFrame.Set(16, 290, 160, 308);
	vFrame.Set(16, 312, 160, 330);
	hTC = new BTextControl(hFrame, "hTC", lstring(199, "Hue (0–360)"), hS, new BMessage('TrbH'));
	sTC =
		new BTextControl(sFrame, "sTC", lstring(200, "Saturation (0–1)"), sS, new BMessage('TrbS'));
	vTC = new BTextControl(vFrame, "vTC", lstring(201, "Value (0–1)"), vS, new BMessage('TrbV'));
	hTC->SetDivider(90);
	sTC->SetDivider(90);
	vTC->SetDivider(90);
	hsvV->AddChild(hTC);
	hsvV->AddChild(sTC);
	hsvV->AddChild(vTC);

	tFrame.Set(8, 3, 316, 41);
	BBox* slidBox2 = new BBox(tFrame, "slidBox2");
	slidBox2->SetLabel(lstring(202, "Alpha Channel"));
	bg2->AddChild(slidBox2);
	tFrame.Set(8, 13, 296, 29);
	aSlid = new Slider(
		tFrame, 84, lstring(203, "Opacity"), 0, 255, 1, new BMessage('CXac'), B_HORIZONTAL, 0
	);
	aSlid->SetValue(c.alpha);
	slidBox2->AddChild(aSlid);

	tFrame.Set(8, 42, 316, 80);
	BBox* slidBox = new BBox(tFrame, "slidBox");
	slidBox->SetLabel(lstring(204, "Color Picker"));
	bg2->AddChild(slidBox);
	tFrame.Set(8, 13, 296, 29);
	Slider* tSlid = new Slider(
		tFrame, 84, lstring(205, "RGB Tolerance"), 0, 255, 1, new BMessage('CXtc'), B_HORIZONTAL, 0,
		"%3.1f"
	);
	tSlid->SetValue(button->getTolerance());
	slidBox->AddChild(tSlid);

	setFrame.Set(bg2Frame.Width() - 92, 84, bg2Frame.Width() - 8, 109);
	BButton* setCurrent =
		new BButton(setFrame, "CE Setbutton", lstring(206, "Current"), new BMessage('CXSc'));
	bg2->AddChild(setCurrent);
	setCurrent->MakeDefault(true);

	setFrame.Set(bg2Frame.Width() - 92, 115, bg2Frame.Width() - 8, 140);
	BButton* setMatch =
		new BButton(setFrame, "CE Matchbutton", lstring(207, "Best Match"), new BMessage('CXSm'));
	bg2->AddChild(setMatch);

	//	cancFrame = setFrame;
	//	cancFrame.OffsetBy (-64, 0);
	//	BButton *cancel = new BButton (cancFrame, "CE Cancelbutton", "Cancel", new BMessage
	//('CXcn')); 	bg2->AddChild (cancel);

	curFrame.Set(8, 84, 39, 109);
	cur = new ColorView(curFrame, "Current", c);
	bg2->AddChild(cur);

	matchFrame.Set(8, 112, 39, 137);
	match = new ColorView(matchFrame, "Match", c);
	bg2->AddChild(match);

	distFrame.Set(42, 84, 200, 100);
	BStringView* selText = new BStringView(distFrame, "SelText", lstring(208, "Current Selection"));
	bg2->AddChild(selText);

	distFrame.Set(42, 108, 200, 124);
	BStringView* matchText =
		new BStringView(distFrame, "MatchText", lstring(209, "Closest Match in Palette"));
	bg2->AddChild(matchText);

	distFrame.Set(42, 124, 200, 138);
	dist = new BStringView(distFrame, "DistText", lstring(210, "(RGB distance XXX)"));
	bg2->AddChild(dist);

	SetColor(c);
	savePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false);
	openPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false);
}

ColorWindow::~ColorWindow()
{
	delete savePanel;
	delete openPanel;
}

void
ColorWindow::Quit()
{
	button->editorSaysBye();
	inherited::Quit();
}

void
ColorWindow::ScreenChanged(BRect frame, color_space cs)
{
	rgbSquare->ScreenChanged(frame, cs);
	hsvSquare->ScreenChanged(frame, cs);
	cur->ScreenChanged(frame, cs);
	match->ScreenChanged(frame, cs);
	inherited::ScreenChanged(frame, cs);
}

void
ColorWindow::SetColor(rgb_color _c)
{
	char s[128];
	c = _c;
	cur->SetColor(c);
	rgb_color d = button->getClosest(c);
	float distance = diff(c, d);
	match->SetColor(d);
	sprintf(s, lstring(211, "(RGB distance: %3.1f)"), distance);
	dist->SetText(s);
}

void
ColorWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
	case B_PASTE: {
		rgb_color* dropped;
		long dummy;
		if (msg->FindData("RGBColor", B_RGB_COLOR_TYPE, (const void**)&dropped, &dummy) == B_OK) {
			BMessage* cdrop = new BMessage('SetC');
			cdrop->AddInt32("color", dropped->red);
			cdrop->AddInt32("color", dropped->green);
			cdrop->AddInt32("color", dropped->blue);
			PostMessage(cdrop);
			delete cdrop;
			aSlid->SetValue(dropped->alpha);
			button->setAlpha(dropped->alpha);
		}
		//		char *htmlString;
		//		if (msg->FindData ("text/plain", B_MIME_TYPE, (void **) &htmlString, &dummy) ==
		//B_OK)
		//		{
		//			printf ("HTML: <%s>\n", htmlString);
		//		}
		//		else
		//			printf ("HTML string not found...\n");
		//		break;
	}
	case 'SetC': {
		char s[16];
		c.red = msg->FindInt32("color", (int32)0);
		c.green = msg->FindInt32("color", (int32)1);
		c.blue = msg->FindInt32("color", (int32)2);
		int32 alpha;
		if (msg->FindInt32("alpha", &alpha) == B_OK) {
			c.alpha = alpha;
		}
		SetColor(c);
		hsv_color h = rgb2hsv(c);
		rgbSquare->SetColor(c);
		hsvSquare->SetColor(h);
		sprintf(s, "%i", c.red);
		rTC->SetText(s);
		sprintf(s, "%i", c.green);
		gTC->SetText(s);
		sprintf(s, "%i", c.blue);
		bTC->SetText(s);
		sprintf(s, "%3.0f", h.hue);
		hTC->SetText(s);
		sprintf(s, "%1.3f", h.saturation);
		sTC->SetText(s);
		sprintf(s, "%1.3f", h.value);
		vTC->SetText(s);
		aSlid->SetValue(c.alpha);
		button->setAlpha(c.alpha);
		break;
	}
	case 'BrbR':
		rgbSquare->SetNotColor(0);
		break;
	case 'BrbG':
		rgbSquare->SetNotColor(1);
		break;
	case 'BrbB':
		rgbSquare->SetNotColor(2);
		break;
	case 'TrbR': {
		char s[16];
		int r = atoi(rTC->Text());
		r = clipchar(r);
		sprintf(s, "%i", r);
		rTC->SetText(s);
		c.red = r;
		rgbSquare->SetColor(c);
		SetColor(c);
		break;
	}
	case 'TrbG': {
		char s[16];
		int g = atoi(gTC->Text());
		g = clipchar(g);
		sprintf(s, "%i", g);
		gTC->SetText(s);
		c.green = g;
		rgbSquare->SetColor(c);
		SetColor(c);
		break;
	}
	case 'TrbB': {
		char s[16];
		int b = atoi(bTC->Text());
		b = clipchar(b);
		sprintf(s, "%i", b);
		bTC->SetText(s);
		c.blue = b;
		rgbSquare->SetColor(c);
		SetColor(c);
		break;
	}
	case 'CSQc': {
		char s[16];
		c.red = msg->FindInt32("color", (int32)0);
		c.green = msg->FindInt32("color", (int32)1);
		c.blue = msg->FindInt32("color", (int32)2);
		sprintf(s, "%i", c.red);
		rTC->SetText(s);
		sprintf(s, "%i", c.green);
		gTC->SetText(s);
		sprintf(s, "%i", c.blue);
		bTC->SetText(s);
		SetColor(c);
		break;
	}
	case 'CSQh': {
		char s[16];
		hsv_color h;
		h.hue = msg->FindFloat("color", (int32)0);
		h.saturation = msg->FindFloat("color", (int32)1);
		h.value = msg->FindFloat("color", (int32)2);
		sprintf(s, "%3.0f", h.hue);
		hTC->SetText(s);
		sprintf(s, "%1.3f", h.saturation);
		sTC->SetText(s);
		sprintf(s, "%1.3f", h.value);
		vTC->SetText(s);
		c = hsv2rgb(h);
		SetColor(c);
		break;
	}
	case 'HSVc': // MouseDown on column
	case 'RGBc':
		ob = 1;
		break;
	case 'HSVs': // MouseDown on square
	case 'RGBs':
		ob = 2;
		break;
	case 'HSVu': // Mouse Up
	case 'RGBu':
		ob = 0;
		break;
	case 'HSVm': {
		if (ob) {
			BPoint point;
			point.x = msg->FindFloat("x");
			point.y = msg->FindFloat("y");
			hsvSquare->mouseDown(point, ob);
		}
		break;
	}
	case 'RGBm': {
		if (ob) {
			BPoint point;
			point.x = msg->FindFloat("x");
			point.y = msg->FindFloat("y");
			rgbSquare->mouseDown(point, ob);
		}
		break;
	}
	case 'CXSc':
		button->set(c);
		break;
	case 'CXSm':
		button->set(c, true);
		break;
	case 'CXtc': // This is not nice.  Change this to take effect after a button press.
		button->setTolerance(msg->FindFloat("value"));
		break;
	case 'CXac':
		// printf ("Alpha set!\n");
		button->setAlpha(msg->FindFloat("value"));
		break;
	case 'Csav':
		savePanel->Show();
		break;
	case 'Clod':
		openPanel->Show();
		break;
	case B_SAVE_REQUESTED: {
		entry_ref lref;
		char name[B_FILE_NAME_LENGTH];
		msg->FindRef("directory", &lref);
		BEntry entry = BEntry(&lref);
		if (msg->FindString("name")) {
			// The Save Panel returns the name separately
			BDirectory dir = BDirectory(&lref);
			strcpy(name, msg->FindString("name"));
			entry.SetTo(&dir, name, false);
		}
		button->Save(entry);
		break;
	}
	case B_REFS_RECEIVED:
	case B_SIMPLE_DATA: {
		entry_ref lref;
		msg->FindRef("refs", &lref);
		BEntry entry = BEntry(&lref);
		button->Load(entry);
		break;
	}
	case 'PGdf':
	case 'PGG':
	case 'PGr':
	case 'PGg':
	case 'PGb':
	case 'PGrg':
	case 'PGrb':
	case 'PGgb':
	case 'PGht':
	case 'PGsp':
		button->Generate(msg->what);
		break;
	default:
		inherited::MessageReceived(msg);
		break;
	}
}
