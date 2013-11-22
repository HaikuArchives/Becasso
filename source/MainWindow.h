#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Window.h>
#include <Rect.h>
#include <Message.h>
#include <Picture.h>
#include "PicMenu.h"

#define BUTTON_SIZE 32
#define NUM_BUTTONS 5
#define BUT_ARRAY 16

class MainWindow : public BWindow
{
public:
			 MainWindow (const BRect frame, const char *name);
virtual		~MainWindow ();
virtual bool QuitRequested ();
virtual void RefsReceived (BMessage *message);
virtual void MenusBeginning ();
virtual BHandler *ResolveSpecifier (BMessage *message, int32 index, BMessage *specifier, int32 command, const char *property);
virtual void MessageReceived (BMessage *message);

private:
typedef BWindow inherited;
BMenuBar	*menubar;
BPicture	*buttons[BUT_ARRAY];
};

#endif 