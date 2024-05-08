// © 1999-2001 Sum Software

#ifndef ADDONWINDOW_H
#define ADDONWINDOW_H

#include <Button.h>
#include <Message.h>
#include <StatusBar.h>
#include <Window.h>
#include "BecassoAddOn.h"
#include "Build.h"

class AddOnWindow : public BWindow {
public:
	AddOnWindow(BRect frame);
	virtual ~AddOnWindow();
	void SetAddOn(becasso_addon_info* info);
	void SetClient(BWindow* client);

	void aPreview();

	virtual void MessageReceived(BMessage* msg);
	virtual bool QuitRequested();

	bool Stop() { return stop; };

	void Stopped();
	void Start();
	void UpdateStatusBar(float delta, const char* text = NULL, const char* trailingText = NULL);
	void ResetStatusBar(const char* label = NULL, const char* trailingLabel = NULL);

	BView* Background() { return bg; };

private:
	typedef BWindow inherited;
	bool stop;
	BWindow* fClient;
	BView* bg;
	BButton *fStop, *fInfo, *fApply;
	BStatusBar* fStatusBar;
	becasso_addon_info fInfoStruct;
};

#endif