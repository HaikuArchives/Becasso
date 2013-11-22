#ifndef ATTRIBCLONE_H
#define ATTRIBCLONE_H

#include "AttribView.h"
#include "BitmapView.h"
#include <Message.h>
#include "Brush.h"
#include "Slider.h"

#define PROP_SPACING	 1
#define PROP_STRENGTH	 2
#define PROP_ANGLE		 3
#define PROP_X			 4
#define PROP_Y			 5
#define PROP_HARDNESS	 6

class AttribClone : public AttribView
{
public:
			 AttribClone ();
virtual		~AttribClone ();
virtual void MessageReceived (BMessage *msg);
virtual BHandler *ResolveSpecifier (BMessage *message, int32 index, BMessage *specifier, int32 command, const char *property);
virtual status_t GetSupportedSuites (BMessage *message);
float		 getSpacing () { return fSpacing; };
int			 getStrength () { return fStrength; };
Brush		*getBrush () { return fBrush; };

private:
typedef AttribView inherited;
void		 ConstructBrush ();
void		 mkGaussianBrush (Brush *b, float sigmaxsq, float sigmaysq, float angle, int cval, float hardness);
void		 mkDiagonalBrush (Brush *b, int dir);

float		 fSpacing;
int			 fStrength;
float		 fAngle;
int			 fX;
int			 fY;
float		 fHardness;
Brush		*fBrush;
BitmapView	*bv;
int			 fCurrentProperty;

Slider		*pSlid, *xSlid, *ySlid, *aSlid, *sSlid, *hSlid;
};

#endif 