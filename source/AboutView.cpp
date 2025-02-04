#include "AboutView.h"
#include <Application.h>
#include <stdio.h>
#include <string.h>
#include "Colors.h"
#include "Settings.h"

SAboutView::SAboutView(BRect rect, BBitmap* becasso, BBitmap* sum, bool startup)
	: BView(rect, "SAboutView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	fBecasso = becasso;
	fSum = sum;
	SetViewColor(Grey28);
	fStartup = startup;
	fAddOnString[0] = 0;
	url = NULL;
	doc = NULL;
	if (!startup) {
		url = new BButton(
			BRect(126, 178, 196, 196), "url", lstring(1, "URL"), new BMessage('aURL'));
		doc = new BButton(
			BRect(204, 178, 274, 196), "doc", lstring(2, "Manual"), new BMessage('aDOC'));
		url->SetTarget(be_app);
		doc->SetTarget(be_app);
		AddChild(url);
		AddChild(doc);
	}
}

SAboutView::~SAboutView()
{
	delete fBecasso;
	delete fSum;
}

void
SAboutView::Draw(BRect updateRect)
{
	if (updateRect != BRect(85, 170, 250, 195)) {
		if (fBecasso)
			DrawBitmap(fBecasso, BPoint(25, 10));
		if (fSum)
			DrawBitmap(fSum, BPoint(10, 115));
		SetLowColor(Grey28);
		SetHighColor(Grey8);
		char verstring[80];
		extern const char* Version;
		extern int gGlobalAlpha;
		extern char gAlphaMask[128];
		SetFontSize(13);
		sprintf(verstring, lstring(3, "Version %s, built %s"), Version, __DATE__);
		//	sprintf (verstring, "MacWorld Demo Version");
		DrawString(verstring, BPoint(85, 124));
		SetFontSize(11);
		DrawString("© 1997-2001 ∑ Sum Software", BPoint(85, 138));
		if (!gGlobalAlpha) {
			SetHighColor(Red);
			SetFontSize(13);
			DrawString(lstring(4, "Unregistered Version"), BPoint(85, 155));
		} else {
			SetHighColor(Grey8);
#if defined(__HAIKU__)
			DrawString("Released under the MIT license", BPoint(85, 152));
#else
			DrawString(lstring(7, "Registered to"), BPoint(85, 152));
			SetHighColor(Black);
			SetFontSize(12);
			DrawString(gAlphaMask, BPoint(85, 168));
#endif
		}
		//	DrawString ("Add-On developers beta version", BPoint (85, 158));
		//	DrawString ("This is a ", BPoint (85, 158));
		//	SetHighColor (Red);
		//	DrawString ("time limited");
		//	SetHighColor (Grey8);
		//	DrawString (" demo that will");
		//	DrawString ("stop working on January 1st, 1998", BPoint (85, 172));
	}
	if (fStartup) {
		SetLowColor(Grey28);
		SetHighColor(Grey8);
		SetFontSize(12);
		DrawString(lstring(5, "Initializing Add-Ons:"), BPoint(85, 182));
		SetFontSize(11);
		DrawString(fAddOnString, BPoint(85, 194));
	}
}

void
SAboutView::SetInitString(const char* s)
{
	strcpy(fAddOnString, s);
	//	printf ("Set to %s\n", s);
	Invalidate(BRect(85, 170, 250, 195));
}
