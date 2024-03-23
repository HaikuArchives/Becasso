#ifndef PREFSWINDOW_H
#define PREFSWINDOW_H

#include <Window.h>
#include <NumberFormat.h>
#include <stdlib.h>
#include "Settings.h"

class BCheckBox;
class BTextControl;
class BPopUpMenu;
class Slider;

class PrefsWindow : public BWindow
{
  public:
	PrefsWindow();
	virtual ~PrefsWindow();

	virtual void MessageReceived(BMessage* message);

	void refresh();

  private:
	typedef BWindow inherited;
	becasso_settings fLocalSettings;
	becasso_settings fBackup;

	BTextControl* fNumEntriesTC;
	BNumberFormat fNumberFormat;
	BPopUpMenu* fLangPU;
	BPopUpMenu* fPrevSizePU;
	Slider* fUndoSlider;
	BCheckBox* fSelectionCB;
};

#endif
