#ifndef ATTRIBFILL_H
#define ATTRIBFILL_H

#include "AttribView.h"
#include <Message.h>
#include <RadioButton.h>
#include "Slider.h"

#define FILLTOL_TOL 0
#define FILLTOL_RGB 1

#define PROP_TOLTYPE 0
#define PROP_VISUAL 1
#define PROP_DRED 2
#define PROP_DGREEN 3
#define PROP_DBLUE 4

class AttribFill : public AttribView
{
  public:
	AttribFill();
	virtual ~AttribFill();
	virtual void MessageReceived(BMessage* msg);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
	);
	virtual status_t GetSupportedSuites(BMessage* message);

	int getTolMode() { return fTolMode; };

	float getTolerance() { return fTolerance; };

	rgb_color getToleranceRGB() { return fToleranceRGB; };

  private:
	typedef AttribView inherited;
	int fTolMode;
	float fTolerance;
	rgb_color fToleranceRGB;
	BRadioButton *tol, *rgb;
	Slider *sT, *sR, *sG, *sB;
	int fCurrentProperty;
};

#endif
