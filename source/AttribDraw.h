#ifndef ATTRIBDRAW_H
#define ATTRIBDRAW_H

#include "AttribView.h"
#include <PopUpMenu.h>
#include <Message.h>

#define MAX_LAYERS 32

class AttribDraw : public AttribView
{
public:
			 AttribDraw ();
virtual		~AttribDraw ();
virtual void MessageReceived (BMessage *msg);
virtual BHandler *ResolveSpecifier (BMessage *message, int32 index, BMessage *specifier, int32 command, const char *property);
virtual status_t GetSupportedSuites (BMessage *message);
drawing_mode getDrawingMode () { return drawmode [fModePU->IndexOf (fModePU->FindMarked())]; };

private:
typedef AttribView inherited;
BPopUpMenu *fModePU;
BPopUpMenu *fTabletPU;
drawing_mode drawmode[9];
};

#endif 