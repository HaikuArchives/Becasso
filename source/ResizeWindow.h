#ifndef RESIZEWINDOW_H
#define RESIZEWINDOW_H

#include <Window.h>
#include <TextControl.h>
#include <stdlib.h>
#include "CanvasWindow.h"

#define UNIT_PIXELS	0
#define UNIT_INCH	1
#define UNIT_CM		2

class ResizeWindow : public BWindow
{
public:
			 ResizeWindow (CanvasWindow *target, const char *title, int32 h, int32 w);
virtual		~ResizeWindow ();

virtual void MessageReceived (BMessage *message);
	
int32		 w() { return fH; };
int32		 h() { return fV; };

private:

void		 recalc ();
void		 readvalues ();

typedef BWindow inherited;
BTextControl *newWidth;
BTextControl *newHeight;
BTextControl *rDPI;
CanvasWindow *fTarget;
int32		 fRez;
int32		 fStatus;
int32		 fHUnit;
int32		 fVUnit;
int32		 fH;
int32		 fV;
};

#endif 