#ifndef BECASSO_H
#define BECASSO_H

#include <Application.h>
#include <Message.h>
#include <Entry.h>
#include "AboutWindow.h"

#define MAX_CANVAS_NAME 1024

#define CURSOR_HAND 0
#define CURSOR_CROSS 1
#define CURSOR_CCROSS 2
#define CURSOR_PICKER 3
#define CURSOR_MOVER 4
#define CURSOR_ROTATOR 5
#define CURSOR_SELECT 6
#define CURSOR_OPEN_HAND 7
#define CURSOR_GRAB 8

class Becasso : public BApplication {
  public:
	Becasso();
	virtual ~Becasso();

	virtual void MessageReceived(BMessage* message);
	virtual void AboutRequested();
	virtual bool QuitRequested();
	virtual void ReadyToRun();
	virtual void RefsReceived(BMessage* message);
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void Pulse();
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
	);
	virtual status_t GetSupportedSuites(BMessage* message);
	void setHand();
	void setCross();
	void setBusy();
	void setReady();
	void setCCross();
	void setSelect();
	void setGrab(bool b, bool down = false);
	void setPicker(bool b);
	void setMover();
	void setRotator();
	void FixCursor();
	void LoadAddOns();
	void PrintSetup();

  private:
	typedef BApplication inherited;
	AboutWindow* about;
	BMessage* launchMessage;
	BMessenger* fCurrentScriptee;
	entry_ref fRef;
	int fBusy;
	int fBusySave;
	int fCurrentCursor;
	int fCurrentCursorSave;
	int fCurrentProperty;
	bool launching;
	bool refvalid;
	BList canvases;
	int32 canvas_index;
};

class canvas {
  public:
	int32 index;
	char name[MAX_CANVAS_NAME];
	BLooper* its_looper;
};

const int32 NumTools = 14;
const int32 NumModes = 2;

#endif