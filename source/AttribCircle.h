#ifndef ATTRIBCIRCLE_H
#define ATTRIBCIRCLE_H

#include "AttribView.h"
#include <Message.h>
#include <PictureButton.h>
#include "Slider.h"

#define CIRCLE_OUTLINE 1
#define CIRCLE_FILL 2
#define CIRCLE_OUTFILL 3

#define FIXES_CENTER 1
#define FIXES_PERIMETER 2

#define PROP_PENSIZE 1
#define PROP_TYPE 2
#define PROP_FIXPOINT 3

class AttribCircle : public AttribView
{
  public:
	AttribCircle();
	virtual ~AttribCircle();
	virtual void MessageReceived(BMessage* msg);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
	);
	virtual status_t GetSupportedSuites(BMessage* message);

	float getPenSize() { return fPenSize; };

	int getType() { return fType; };

	int getFirst() { return fFirst; };

  private:
	typedef AttribView inherited;
	float fPenSize;
	int fType;
	int fFirst;
	BPictureButton *pT1, *pT2, *pT3, *pF1, *pF2;
	Slider* lSlid;
	int fCurrentProperty;
};

#endif