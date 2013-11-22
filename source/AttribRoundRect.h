#ifndef ATTRIBROUNDRECT_H
#define ATTRIBROUNDRECT_H

#include "AttribView.h"
#include <Message.h>
#include <PictureButton.h>
#include <RadioButton.h>
#include "Slider.h"

#define RRECT_OUTLINE 1
#define RRECT_FILL    2
#define RRECT_OUTFILL 3

#define RRECT_RELATIVE 1
#define RRECT_ABSOLUTE 2

#define PROP_PENSIZE	1
#define PROP_TYPE		2
#define PROP_RELX		3
#define PROP_RELY		4
#define PROP_ABSX		5
#define PROP_ABSY		6
#define PROP_CORNERS	7

class AttribRoundRect : public AttribView
{
public:
			 AttribRoundRect ();
virtual		~AttribRoundRect ();
virtual void MessageReceived (BMessage *msg);
virtual BHandler *ResolveSpecifier (BMessage *message, int32 index, BMessage *specifier, int32 command, const char *property);
virtual status_t GetSupportedSuites (BMessage *message);
float		 getPenSize () { return fPenSize; };
int			 getType () { return fType; };
int			 getRadType () { return fRadType; };
float		 getRadXabs () { return fRadXabs; };
float		 getRadYabs () { return fRadYabs; };
float		 getRadXrel () { return fRadXrel; };
float		 getRadYrel () { return fRadYrel; };

private:
typedef AttribView inherited;
float		 fPenSize;
float		 fRadXabs;
float		 fRadYabs;
float		 fRadXrel;
float		 fRadYrel;
int			 fRadType;
int			 fType;
BPictureButton	*pT1, *pT2, *pT3;
Slider		*lSlid, *relx, *rely, *absx, *absy;
BRadioButton	*rel, *abs;
int			 fCurrentProperty;
};

#endif 