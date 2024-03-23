#ifndef ATTRIBSPRAYCAN_H
#define ATTRIBSPRAYCAN_H

#include "AttribView.h"
#include <CheckBox.h>
#include <Message.h>
#include "Slider.h"

#define frand() ((double)rand() / ((double)RAND_MAX + 1))

#define PROP_SIGMA 0
#define PROP_RATIO 1
#define PROP_RATE 2
#define PROP_FADE 3

class AttribSpraycan : public AttribView
{
  public:
	AttribSpraycan();
	virtual ~AttribSpraycan();
	virtual void MessageReceived(BMessage* msg);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
	);
	virtual status_t GetSupportedSuites(BMessage* message);

	float getSigma() { return fSigma; };

	bool getLighten() { return fLighten; };

	float getColorRatio() { return fColorRatio; };

	float getFlowRate() { return fFlowRate; };

  private:
	typedef AttribView inherited;
	float fSigma;
	bool fLighten;
	float fColorRatio;
	float fFlowRate;
	BCheckBox* sL;
	Slider *cSlid, *sSlid, *fSlid;
	int fCurrentProperty;
};

#endif