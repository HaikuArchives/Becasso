#ifndef ATTRIBERASER_H
#define ATTRIBERASER_H

#include "AttribView.h"
#include <Message.h>
#include <PictureButton.h>
#include "Slider.h"

#define ERASER_RECT 1
#define ERASER_ELLIPSE 2

#define PROP_TYPE 2
#define PROP_XSIZE 1
#define PROP_YSIZE 3

class AttribEraser : public AttribView {
  public:
	AttribEraser();
	virtual ~AttribEraser();
	virtual void MessageReceived(BMessage* msg);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
	);
	virtual status_t GetSupportedSuites(BMessage* message);

	float getXSize() { return fXSize; };

	float getYSize() { return fYSize; };

	int getType() { return fType; };

  private:
	typedef AttribView inherited;
	float fXSize;
	float fYSize;
	int fType;
	BPictureButton *eT1, *eT2;
	Slider *xSlid, *ySlid;
	int fCurrentProperty;
};

#endif