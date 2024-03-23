#ifndef ATTRIBLINES_H
#define ATTRIBLINES_H

#include "AttribView.h"
#include <Message.h>
#include <CheckBox.h>
#include "Slider.h"

#define PROP_PENSIZE 1
#define PROP_FILLCORNERS 2

class AttribLines : public AttribView
{
  public:
	AttribLines();
	virtual ~AttribLines();
	virtual void MessageReceived(BMessage* msg);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
	);
	virtual status_t GetSupportedSuites(BMessage* message);

	float getPenSize() { return fPenSize; };

	bool fillCorners() { return fFillCorners; };

  private:
	typedef AttribView inherited;
	BCheckBox* lFC;
	float fPenSize;
	bool fFillCorners;
	Slider* lSlid;
	int fCurrentProperty;
};

#endif