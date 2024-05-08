#ifndef ATTRIBELLIPSE_H
#define ATTRIBELLIPSE_H

#include <Message.h>
#include <PictureButton.h>
#include "AttribView.h"
#include "Slider.h"

#define ELLIPSE_OUTLINE 1
#define ELLIPSE_FILL 2
#define ELLIPSE_OUTFILL 3

#define PROP_PENSIZE 1
#define PROP_TYPE 2

class AttribEllipse : public AttribView {
public:
	AttribEllipse();
	virtual ~AttribEllipse();
	virtual void MessageReceived(BMessage* msg);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property);
	virtual status_t GetSupportedSuites(BMessage* message);

	float getPenSize() { return fPenSize; };

	int getType() { return fType; };

private:
	typedef AttribView inherited;
	float fPenSize;
	int fType;
	BPictureButton *pT1, *pT2, *pT3;
	Slider* lSlid;
	int fCurrentProperty;
};

#endif