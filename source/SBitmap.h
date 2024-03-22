// SBitmap: BBitmap replacement which doesn't clear on ctor

#ifndef _SBITMAP_H
#define _SBITMAP_H

#include <Bitmap.h>

class SBitmap {
  public:
	SBitmap(BBitmap* src);
	SBitmap(const BRect bounds, const color_space cs);
	~SBitmap();

	int32 BytesPerRow() const;
	int32 BytesPerPixel() const;
	int32 BitsLength() const;
	void* Bits();
	BRect Bounds() const;
	color_space ColorSpace() const;

  private:
	BRect fBounds;
	int32 fBPP;
	color_space fCS;
	int32 fBPR;

	void* fBits;
};

#endif