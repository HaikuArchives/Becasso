#ifndef ATTRIBTEXT_H
#define ATTRIBTEXT_H

#include "AttribView.h"
#include <Message.h>
#include <Font.h>
#include <PopUpMenu.h>
#include <CheckBox.h>
#include <TextView.h>
#include "Slider.h"

#define DEMO_TEXT "Becasso"

#define PROP_FAMILY 0
#define PROP_STYLE 1
#define PROP_SIZE 2
#define PROP_SHEAR 3
#define PROP_ROTATION 4
#define PROP_ANTIALIAS 5
#define PROP_TEXT 6

class AttribText : public AttribView {
  public:
	AttribText();
	virtual ~AttribText();
	virtual void MessageReceived(BMessage* msg);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
	);
	virtual status_t GetSupportedSuites(BMessage* message);
	virtual void AttachedToWindow();

	BFont getFont() { return fFont; };

	const char* getText() { return fText->Text(); };

  private:
	typedef AttribView inherited;
	BFont fFont;
	font_family* families;
	font_style* styles;
	BPopUpMenu* fFamilyPU;
	BPopUpMenu* fStylePU;
	BTextView* fText;
	BCheckBox* aaCheck;
	int32 numStyles;
	int32 numFamilies;
	Slider *szSlid, *shSlid, *rtSlid;
	int fCurrentProperty;
};

#endif