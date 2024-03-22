#ifndef CANVASWINDOW_H
#define CANVASWINDOW_H

#include <Window.h>
#include <ScrollBar.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Menu.h>
#include <Rect.h>
#include <Point.h>
#include <Alert.h>
#include <Entry.h>
#include <Path.h>
#include <File.h>
#include <FilePanel.h>
#if defined(DATATYPES)
#include <Datatypes.h>
#else
#include <translation/TranslationUtils.h>
#define DATAInfo translator_info
#endif
#include <Invoker.h>
#include <Messenger.h>
#include <stdio.h>
#include "Colors.h"

const float MIN_WIDTH = 200;
const float MIN_HEIGHT = 80;
const int MAX_ALERTSIZE = 100;
const int MAX_FNAME = 1024;

class CanvasView;
class BGView;
class ProgressiveBitmapStream;
class OutputFormatWindow;
class MagWindow;
class LayerWindow;
class PosView;
class XpalWindow;

class CanvasWindow : public BWindow {
	friend class CanvasView;

  public:
	CanvasWindow(
		BRect frame, const char* name, BBitmap* map = NULL, BMessenger* target = NULL,
		bool AskForAlpha = true, rgb_color color = White
	);
	CanvasWindow(BRect frame, entry_ref ref, bool AskForAlpha = true, BMessenger* target = NULL);
	virtual ~CanvasWindow();
	virtual void Quit();
	virtual bool QuitRequested();
	virtual void MenusBeginning();
	virtual void MenusEnded();

	bool MenuOnScreen() { return menuIsOn; };

	virtual void MessageReceived(BMessage* message);
	virtual void FrameMoved(BPoint screenpoint);
	virtual void FrameResized(float width, float height);
	virtual void ScreenChanged(BRect frame, color_space mode);
	virtual BHandler* ResolveSpecifier(
		BMessage* message, int32 index, BMessage* specifier, int32 command, const char* property
	);
	virtual status_t GetSupportedSuites(BMessage* message);


	void save(entry_ref* ref, char* file, int fd);
	void setMenuItem(uint32 command, bool v);
	void setPaste(bool v);
	void setLayerMenu();

	bool isLayerOpen() { return layerOpen; }

	MagWindow* TheMagWindow() { return magWindow; }

	void CanvasResized(float width, float height);
	void CanvasScaled(float scale);

	float getScale() { return fScale; }

	char* FileName() { return fName; }

	char* CanvasName() { return fCanvasName; }

	void setName(char* n);

	bool IsQuitting() { return quitting; }

	entry_ref MyRef() { return myRef; }

  private:
	typedef BWindow inherited;
	void restOfCtor(
		BRect frame, BBitmap* map, FILE* fp = NULL, bool AskForAlpha = true, rgb_color color = White
	);
	BMessage* sendbitmapreply();

	char fName[B_FILE_NAME_LENGTH];
	char fCanvasName[B_FILE_NAME_LENGTH];
	float fScale;
	BMenuBar* menubar;
	BMenu* fileMenu;
	BMenu* editMenu;
	BMenu* windowMenu;
	BMenu* layerMenu;
	BMenu* layerNamesMenu;
	BScrollBar* h;
	BScrollBar* v;
	BGView* bg;
	CanvasView* canvas;
	PosView* posview;
	BFilePanel* savePanel;
	LayerWindow* layerWindow;
	MagWindow* magWindow;
	XpalWindow* extractWindow;
	ProgressiveBitmapStream* bitmapStream;
	OutputFormatWindow* outputFormatWindow;
	uint32 out_type;
	// DATAInfo	 out_info;
	translator_id out_translator;
	bool layerOpen;
	bool quitting;
	bool scripted;
	int fCurrentProperty;
	BInvoker* of_selected;
	entry_ref myRef;
	bool fromRef;
	BMessenger* myTarget; // InterfaceElements
	BMessenger* ieTarget; // ImageElements
	bool menuIsOn;
};

#endif