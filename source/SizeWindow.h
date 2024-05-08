#ifndef SIZEWINDOW_H
#define SIZEWINDOW_H

#include <NumberFormat.h>
#include <TextControl.h>
#include <Window.h>
#include <stdlib.h>

// Returns 1 if the `Open' button was pressed,
//         0 if the 'Cancel' button was pressed.

#define UNIT_PIXELS 0
#define UNIT_INCH 1
#define UNIT_CM 2

#define COLOR_FG 0
#define COLOR_BG 1
#define COLOR_TR 2

class SizeWindow : public BWindow {
public:
	SizeWindow(int32 h, int32 w, int32 c);
	virtual ~SizeWindow();

	virtual void MessageReceived(BMessage* message);
	virtual int32 Go();

	int32 w() { return fH; };

	int32 h() { return fV; };

	int32 color() { return fColor; };

private:
	void recalc();
	void readvalues();

	typedef BWindow inherited;
	BNumberFormat fNumberFormat;
	BTextControl* newWidth;
	BTextControl* newHeight;
	BTextControl* rDPI;

	int32 fStatus;
	int32 fHUnit;
	int32 fVUnit;
	int32 fH;
	int32 fV;
	int32 fColor;
	int32 fRez;
};

#endif
