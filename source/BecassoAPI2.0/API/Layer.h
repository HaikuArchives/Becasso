// © 1999 Sum Software

#ifndef LAYER_H
#define LAYER_H

#include <Bitmap.h>
#include <Rect.h>
#include "Build.h"
#include "AddOnSupport.h"

#define MAXLAYERNAME 128

class IMPEXP Layer : public BBitmap
{
  public:
	Layer(BRect bounds, const char* name);
	Layer(const Layer& layer);
	~Layer();

	int getMode() { return fMode; };

	void setMode(const int mode) { fMode = mode; };

	char* getName() { return fName; };

	void setName(const char* name);

	uchar getGlobalAlpha() { return fGlobalAlpha; };

	void setGlobalAlpha(const uchar a) { fGlobalAlpha = a; };

	BBitmap* getAlphaMap() { return fAlphaMap; };

	void setAlphaMap(BBitmap* a) { fAlphaMap = a; };

	BRect Bounds() { return fRect; };

	void Hide(bool h) { fHide = h; };

	bool IsHidden() { return fHide; };

	void ClearTo(bgra_pixel p);

  private:
	typedef BBitmap inherited;
	BBitmap* fAlphaMap;
	uchar fGlobalAlpha;
	BRect fRect;
	int fMode;
	bool fHide;
	char fName[MAXLAYERNAME];
};

#endif