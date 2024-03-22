// © 1999-2001 Sum Software

#ifndef SELECTION_H
#define SELECTION_H

#include <Bitmap.h>
#include <Rect.h>
#include "Build.h"
#include "AddOnSupport.h"

class IMPEXP Selection : public BBitmap {
  public:
	Selection(BRect bounds);
	Selection(const Selection& selection);
	virtual ~Selection();

	int getMode() { return fMode; };

	void setMode(int mode) { fMode = mode; };

	BRect Bounds() { return fRect; };

	void ClearTo(grey_pixel p);

  private:
	typedef BBitmap inherited;
	BRect fRect;
	int fMode;
};

#endif