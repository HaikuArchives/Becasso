#ifndef LAYERNAMEWINDOW_H
#define LAYERNAMEWINDOW_H

#include <Window.h>
#include <TextControl.h>

class LayerNameWindow : public BWindow
{
  public:
	LayerNameWindow(const char* name);
	virtual ~LayerNameWindow();

	virtual void MessageReceived(BMessage* message);
	virtual int32 Go();

	const char* name() { return fName->Text(); };

  private:
	typedef BWindow inherited;
	sem_id wait_sem;
	BTextControl* fName;
	int32 fStatus;
};

#endif