#ifndef ABOUTVIEW_H
#define ABOUTVIEW_H

#include <Bitmap.h>
#include <Button.h>
#include <View.h>

class SAboutView : public BView {
public:
	SAboutView(BRect rect, BBitmap* becasso, BBitmap* sum, bool startup = false);
	virtual ~SAboutView();
	virtual void Draw(BRect updateRect);
	virtual void SetInitString(const char* s);

private:
	typedef BView inherited;
	BBitmap* fBecasso;
	BBitmap* fSum;
	bool fStartup;
	BButton *url, *doc;
	char fAddOnString[256];
};

#endif