#ifndef ATTRIBFREEHAND_H
#define ATTRIBFREEHAND_H

#include "AttribView.h"
#include <Message.h>
#include "Slider.h"

#define PROP_PENSIZE	1

class AttribFreehand : public AttribView
{
public:
			 AttribFreehand ();
virtual		~AttribFreehand ();
virtual void MessageReceived (BMessage *msg);
virtual BHandler *ResolveSpecifier (BMessage *message, int32 index, BMessage *specifier, int32 command, const char *property);
virtual status_t GetSupportedSuites (BMessage *message);
float		 getPenSize () { return fPenSize; };

private:
typedef AttribView inherited;
float		 fPenSize;
Slider		*pSlid;
int			 fCurrentProperty;
};

#endif 