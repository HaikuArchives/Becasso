// © 1999 Sum Software

#ifndef DIAL_H
#define DIAL_H

#include <View.h>
#include <Message.h>
#include <TextControl.h>

#define DIAL_90 90
#define DIAL_360 360

class IMPEXP Dial : public BView
{
  public:
	Dial(BRect frame, const char* name, int type, BMessage* msg);
	~Dial();
	virtual void MouseDown(BPoint point);
	virtual void Draw(BRect update);
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* msg);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void MakeFocus(bool focused = true);
	float Value();
	void SetValue(float _v);

  private:
	typedef BView inherited;
	void NotifyTarget();
	BMessage* fMsg;
	char fName[64];
	bigtime_t dcspeed;
	int click;
	BTextControl* tc;
	float fValue;
	int fType;
};

#endif