#ifndef SVIEW_H
#define SVIEW_H

#include <View.h>
#include "Position.h"
#include "Tablet.h"
#include "Wacom.h"

class SView : public BView {
  public:
	SView(BRect frame, const char* name, uint32 resizeMask, uint32 flags);
	virtual ~SView();
	virtual void StrokeRect(BRect r, pattern p = B_SOLID_HIGH);
	virtual void FillRect(BRect r, pattern p = B_SOLID_HIGH);
	virtual void InvertRect(BRect r);
	virtual void StrokeRoundRect(BRect r, float xRadius, float yRadius, pattern p = B_SOLID_HIGH);
	virtual void FillRoundRect(BRect r, float xRadius, float yRadius, pattern p = B_SOLID_HIGH);
	virtual void StrokeEllipse(BRect r, pattern p = B_SOLID_HIGH);
	virtual void
	StrokeEllipse(BPoint center, float xRadius, float yRadius, pattern p = B_SOLID_HIGH);
	virtual void FillEllipse(BRect r, pattern p = B_SOLID_HIGH);
	virtual void FillEllipse(BPoint center, float xRadius, float yRadius, pattern p = B_SOLID_HIGH);

	virtual void SStrokeRect(BRect r, pattern p = B_SOLID_HIGH);
	virtual void SFillRect(BRect r, pattern p = B_SOLID_HIGH);
	virtual void SInvertRect(BRect r);
	virtual void SStrokeRoundRect(BRect r, float xRadius, float yRadius, pattern p = B_SOLID_HIGH);
	virtual void SFillRoundRect(BRect r, float xRadius, float yRadius, pattern p = B_SOLID_HIGH);
	virtual void SStrokeEllipse(BRect r, pattern p = B_SOLID_HIGH);
	virtual void
	SStrokeEllipse(BPoint center, float xRadius, float yRadius, pattern p = B_SOLID_HIGH);
	virtual void SFillEllipse(BRect r, pattern p = B_SOLID_HIGH);
	virtual void
	SFillEllipse(BPoint center, float xRadius, float yRadius, pattern p = B_SOLID_HIGH);
	virtual void GetMouse(BPoint* pos, uint32* buttons, bool checkqueue = true);
	virtual void GetPosition(Position* position, bool setcursor = false);

	inline BRect makePositive(BRect r);

	void setScale(float s) { fScale = s; };

	void setPressure(uint8 pressure) { fPressure = pressure; };

	uint8 getPressure() { return fPressure; };

	uint32 getSnoozeTime() { return fSnoozeTime; };

	void setSnoozeTime(uint32 time) { fSnoozeTime = time; };

  private:
	typedef BView inherited;
	float fScale;
	uint8 fPressure;
	uint32 fSnoozeTime;
	int fDevice;
};

#define DEV_NONE 0
#define DEV_MOUSE 1
#define DEV_TABLET 2

#endif