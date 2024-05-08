#include "AttribText.h"
#include <Box.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <string.h>
#include "Colors.h"
#include "Settings.h"
#include "Slider.h"

static property_info prop_list[] = {{"Family", SET, DIRECT, "string: family_name"},
	{"Style", SET, DIRECT, "string: style_name"}, {"PtSize|Size", SET, DIRECT, "float: 4 .. 144"},
	{"Shear", SET, DIRECT, "float: 45 .. 135"}, {"Rotation", SET, DIRECT, "float: 0 .. 360"},
	{"AntiAliasing", SET, DIRECT, "bool: true, false"}, {"Text", SET, DIRECT, "string: text"}, 0};

AttribText::AttribText() : AttribView(BRect(0, 0, 224, 208), lstring(25, "Text"))
{
	SetViewColor(LightGrey);
	fFont = BFont(be_plain_font);
	numFamilies = count_font_families();
	families = new font_family[numFamilies];
	font_family family;
	font_style style;
	uint32 flags;
	for (int32 i = 0; i < numFamilies; i++) {
		if (get_font_family(i, &family, &flags) == B_OK) {
			strcpy(families[i], family);
		}
	}
	get_font_family(2, &family, &flags);
	numStyles = count_font_styles(family);
	styles = new font_style[numStyles];
	for (int32 j = 0; j < numStyles; j++) {
		if (get_font_style(family, j, &style, &flags) == B_OK) {
			strcpy(styles[j], style);
		}
	}
	fFamilyPU = new BPopUpMenu("");
	for (int32 i = 0; i < numFamilies; i++)
		fFamilyPU->AddItem(new BMenuItem(families[i], new BMessage('font')));
	fFamilyPU->ItemAt(2)->SetMarked(true);
	fFamilyPU->SetTargetForItems(this);
	fStylePU = new BPopUpMenu("");
	for (int32 j = 0; j < numStyles; j++)
		fStylePU->AddItem(new BMenuItem(styles[j], new BMessage('fchg')));
	fStylePU->ItemAt(0)->SetMarked(true);
	fStylePU->SetTargetForItems(this);
	BMenuField* dFamily =
		new BMenuField(BRect(4, 2, 220, 20), "dFamily", lstring(360, "Family: "), fFamilyPU);
	dFamily->SetDivider(50);
	BMenuField* dStyle =
		new BMenuField(BRect(4, 24, 220, 40), "dStyle", lstring(361, "Style: "), fStylePU);
	dStyle->SetDivider(50);
	AddChild(dFamily);
	AddChild(dStyle);
	szSlid = new Slider(
		BRect(7, 50, 220, 66), 66, lstring(362, "Pt Size:"), 4, 144, 1, new BMessage('fcSz'));
	shSlid = new Slider(
		BRect(7, 70, 220, 86), 66, lstring(363, "Shear:"), 45, 135, 1, new BMessage('fcSh'));
	rtSlid = new Slider(
		BRect(7, 90, 220, 106), 66, lstring(364, "Rotation:"), 0, 360, 1, new BMessage('fcRt'));
	szSlid->SetValue(12);
	shSlid->SetValue(90);
	AddChild(szSlid);
	AddChild(shSlid);
	AddChild(rtSlid);
	aaCheck = new BCheckBox(BRect(7, 109, 216, 127), "anti-aliasing", lstring(365, "Anti Aliasing"),
		new BMessage('fcAA'));
	aaCheck->SetValue(1);
	AddChild(aaCheck);
	fFont.SetFamilyAndStyle(families[2], styles[0]);
	fFont.SetSize(12);
	fFont.SetShear(90);
	fFont.SetSpacing(B_BITMAP_SPACING);
	fFont.SetRotation(0);
	//	demo = new BStringView (BRect (8, 132, 176, 182), "demo", DEMO_TEXT);
	//	demo->SetFont (&fFont);
	//	demo->SetText (DEMO_TEXT);
	//	demo->SetViewColor (White);
	//	demo->SetLowColor (White);
	fText = new BTextView(BRect(8, 132, 216 - B_V_SCROLL_BAR_WIDTH, 202 - B_H_SCROLL_BAR_HEIGHT),
		"text", BRect(0, 0, 1024, 1), &fFont, 0, B_FOLLOW_NONE, B_WILL_DRAW);
	fText->SetWordWrap(false);
	fText->MakeResizable(false);
	fText->SetStylable(true);
	fText->SetText(DEMO_TEXT);
	BScrollView* demoBox = new BScrollView("demoBox", fText, B_FOLLOW_NONE, 0, true, true);
	AddChild(demoBox);
	fCurrentProperty = 0;
}

AttribText::~AttribText()
{
	delete[] families;
	delete[] styles;
}

void
AttribText::AttachedToWindow()
{
	inherited::AttachedToWindow();
}

status_t
AttribText::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/x-sum-becasso-Text");
	BPropertyInfo info(prop_list);
	message->AddFlat("messages", &info);
	return AttribView::GetSupportedSuites(message);
}

BHandler*
AttribText::ResolveSpecifier(
	BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property)
{
	if (!strcasecmp(property, "Family")) {
		fCurrentProperty = PROP_FAMILY;
		return this;
	}
	if (!strcasecmp(property, "Style")) {
		fCurrentProperty = PROP_STYLE;
		return this;
	}
	if (!strcasecmp(property, "PtSize") || !strcasecmp(property, "Size")) {
		fCurrentProperty = PROP_SIZE;
		return this;
	}
	if (!strcasecmp(property, "Shear")) {
		fCurrentProperty = PROP_SHEAR;
		return this;
	}
	if (!strcasecmp(property, "Rotation")) {
		fCurrentProperty = PROP_ROTATION;
		return this;
	}
	if (!strcasecmp(property, "AntiAliasing")) {
		fCurrentProperty = PROP_ANTIALIAS;
		return this;
	}
	if (!strcasecmp(property, "Text")) {
		fCurrentProperty = PROP_TEXT;
		return this;
	}
	return inherited::ResolveSpecifier(message, index, specifier, command, property);
}

void
AttribText::MessageReceived(BMessage* msg)
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
				case PROP_FAMILY:
				{
					const char* name;
					if (msg->FindString("data", &name) == B_OK) {
						BMenuItem* item = fFamilyPU->FindItem(name);
						if (!item)	// Courtesy to the user: Do a partial match.
						{
							for (int32 i = fFamilyPU->CountItems() - 1; i >= 0; i--) {
								BMenuItem* thisone = fFamilyPU->ItemAt(i);
								if (!strncasecmp(thisone->Label(), name, strlen(name))) {
									item = thisone;
								}
							}
						}
						if (item) {
							item->SetMarked(true);
							BMessage font = BMessage('font');
							MessageReceived(&font);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_STYLE:
				{
					const char* name;
					if (msg->FindString("data", &name) == B_OK) {
						BMenuItem* item = fStylePU->FindItem(name);
						if (!item)	// Courtesy to the user: Do a partial match.
						{
							for (int32 i = fStylePU->CountItems() - 1; i >= 0; i--) {
								BMenuItem* thisone = fStylePU->ItemAt(i);
								if (!strncasecmp(thisone->Label(), name, strlen(name))) {
									item = thisone;
								}
							}
						}
						if (item) {
							item->SetMarked(true);
							BMessage font = BMessage('fchg');
							MessageReceived(&font);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_SIZE:	 // float, 4 .. 144
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
						if (value >= 4 && value <= 144) {
							fFont.SetSize(value);
							fText->SetStylable(false);
							fText->SetFontAndColor(&fFont);
							fText->SetStylable(true);
							szSlid->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_SHEAR:  // float, 45 .. 135
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
						if (value >= 45 && value <= 135) {
							fFont.SetShear(value);
							fText->SetStylable(false);
							fText->SetFontAndColor(&fFont);
							fText->SetStylable(true);
							shSlid->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_ROTATION:	 // float, 0 .. 360
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
						if (value >= 0 && value <= 360) {
							fFont.SetRotation(value);
							fText->SetStylable(false);
							fText->SetFontAndColor(&fFont);
							fText->SetStylable(true);
							rtSlid->SetValue(value);
							if (msg->IsSourceWaiting()) {
								BMessage error(B_REPLY);
								error.AddInt32("error", B_NO_ERROR);
								msg->SendReply(&error);
							}
						}
					}
					break;
				}
				case PROP_ANTIALIAS:  // boolean
				{
					bool value;
					if (msg->FindBool("data", &value) == B_OK) {
						fFont.SetFlags(value ? ~B_DISABLE_ANTIALIASING : B_DISABLE_ANTIALIASING);
						fText->SetStylable(false);
						fText->SetFontAndColor(&fFont);
						fText->SetStylable(true);
						aaCheck->SetValue(value);
						if (msg->IsSourceWaiting()) {
							BMessage error(B_REPLY);
							error.AddInt32("error", B_NO_ERROR);
							msg->SendReply(&error);
						}
					}
					break;
				}
				case PROP_TEXT:	 // string
				{
					const char* value;
					if (msg->FindString("data", &value) == B_OK) {
						fText->SetText(value);
						if (msg->IsSourceWaiting()) {
							BMessage error(B_REPLY);
							error.AddInt32("error", B_NO_ERROR);
							msg->SendReply(&error);
						}
					}
				}
					fCurrentProperty = 0;
					break;
			}
			inherited::MessageReceived(msg);
			break;
		}
		case 'font':
		{
			font_family family;
			strcpy(family, families[fFamilyPU->IndexOf(fFamilyPU->FindMarked())]);
			for (int32 j = numStyles; j > 0; j--)
				fStylePU->RemoveItem(j - 1);

			delete[] styles;
			numStyles = count_font_styles(family);
			styles = new font_style[numStyles];
			uint32 flags;
			for (int32 j = 0; j < numStyles; j++) {
				font_style style;
				if (get_font_style(family, j, &style, &flags) == B_OK) {
					strcpy(styles[j], style);
					fStylePU->AddItem(new BMenuItem(style, new BMessage('fchg')));
				}
			}
			fStylePU->ItemAt(0)->SetMarked(true);
			fFont.SetFamilyAndStyle(family, styles[0]);
			fText->SetStylable(false);
			fText->SetFontAndColor(&fFont);
			fText->SetStylable(true);
			break;
		}
		case 'fchg':
		{
			int f = fFamilyPU->IndexOf(fFamilyPU->FindMarked());
			int s = fStylePU->IndexOf(fStylePU->FindMarked());
			// printf ("families[%d] = %s, styles[%d] = %s\n", f, families[f], s, styles[s]);
			fFont.SetFamilyAndStyle(families[f], styles[s]);
			fText->SetStylable(false);
			fText->SetFontAndColor(&fFont);
			fText->SetStylable(true);
			break;
		}
		case 'fcSz':
			fFont.SetSize(msg->FindFloat("value"));
			fText->SetFontAndColor(&fFont);
			fText->SetStylable(false);
			fText->SetFontAndColor(&fFont);
			fText->SetStylable(true);
			break;
		case 'fcSh':
			fFont.SetShear(msg->FindFloat("value"));
			fText->SetFontAndColor(&fFont);
			fText->SetStylable(false);
			fText->SetFontAndColor(&fFont);
			fText->SetStylable(true);
			break;
		case 'fcRt':
			fFont.SetRotation(msg->FindFloat("value"));
			fText->SetFontAndColor(&fFont);
			fText->SetStylable(false);
			fText->SetFontAndColor(&fFont);
			fText->SetStylable(true);
			break;
		case 'fcAA':
			fFont.SetFlags(aaCheck->Value() ? ~B_DISABLE_ANTIALIASING : B_DISABLE_ANTIALIASING);
			fText->SetFontAndColor(&fFont);
			fText->SetStylable(false);
			fText->SetFontAndColor(&fFont);
			fText->SetStylable(true);
			break;
		default:
			// msg->PrintToStream();
			inherited::MessageReceived(msg);
			break;
	}
}