// © 1997-2001 Sum Software

#ifndef SLIDER_H
#define SLIDER_H

#include <View.h>
#include <Rect.h>
#include <Bitmap.h>
#include <Message.h>
#include <TextControl.h>
#include "Build.h"

class IMPEXP Slider : public BView
{
public:
			 Slider (BRect frame, float sep, const char *name, float _min, float _max, float step, BMessage *_msg, orientation _posture = B_HORIZONTAL, int f = 0, const char *_fmt = "%.0f");
virtual		~Slider ();
//void		 ValueChanged (long newValue);
virtual void MouseDown (BPoint point);
virtual void Draw (BRect update);
virtual void AttachedToWindow ();
virtual void MessageReceived (BMessage *msg);
virtual void KeyDown (const char *bytes, int32 numBytes);
virtual void MakeFocus (bool focused = true);
virtual void SetTarget (BHandler *target);
float		 Value ();
void		 SetValue (float _v);

private:
typedef BView inherited;
void		 NotifyTarget ();
float		 min;
float		 max;
float		 sep;
float		 step;
BMessage	*msg;
char		 fmt[16];
char		 name[64];
orientation	 pos;
float		 value;
BRect		 bounds;
BRect		 knob;
BBitmap		*offslid;
BView		*offview;
BPoint		 knobpos;
float		 knobsize;
int			 f;
float		 width;
float		 height;
bigtime_t	 dcspeed;
int			 click;
BTextControl *tc;
BHandler	 *target;
};

#endif 