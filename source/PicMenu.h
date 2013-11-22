#ifndef PICMENU_H
#define PICMENU_H

#include <Rect.h>
#include <Picture.h>
#include <Menu.h>
#include <Point.h>
#include "PicItem.h"
#include "sfx.h"
#include "DragWindow.h"

#define MAX_ITEMS 64
#define MENU_WIN_TITLE "     "

class PicMenuButton;

class PicMenu : public BMenu
{
friend class PicMenuButton;
public:
				 PicMenu (const char *name, BView *_view, int h, int v, float s);
virtual			~PicMenu ();
//virtual void	 AddItem (PicItem *item);
virtual void	 AddItem (PicItem *item, const BRect rect);
virtual void	 MouseMoved (BPoint point, uint32 transit, const BMessage *msg);
virtual void	 MessageReceived (BMessage *msg);
PicItem			*FindMarked ();
PicItem			*ItemAt (int ind) { return items[ind]; };
virtual void	 Mark (int _index);
long			 Selected ();
void			 setParent (PicMenuButton *pmb);
PicMenuButton	*getParent () { return parent; };
void			 TearDone (BRect place, bool newwin);
void			 InvalidateWindow ();

protected:
virtual BPoint ScreenLocation ();

private:
typedef BMenu inherited;
DragWindow		*fWindow;
PicItem			*items[MAX_ITEMS];
int32			 marked;
int32			 numitems;
PicMenuButton	*parent;
SoundEffect8_11	*fxTear;
BView			*view;
float			 hs, vs;
int				 hnum;
char			 kind[64];
};

#endif 