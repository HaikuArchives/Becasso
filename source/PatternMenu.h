#ifndef PATTERNMENU_H
#define PATTERNMENU_H

#include <Rect.h>
#include <Menu.h>
#include <Point.h>
#include <View.h>
#include "PatternItem.h"
#include "sfx.h"
#include "DragWindow.h"

#define MAX_PATTERNS 20

class PatternMenu : public BMenu
{
friend class PatternMenuButton;
public:
					 PatternMenu (BView *_view, int h, int v, float s);
virtual				~PatternMenu ();
virtual void		 AddItem (PatternItem *item, BRect frame);
virtual void		 MouseMoved (BPoint point, uint32 transit, const BMessage *msg);
PatternItem			*FindMarked ();
PatternItem			*ItemAt (int i);
virtual void		 Mark (int _index);
void				 setParent (PatternMenuButton *pmb);
PatternMenuButton	*getParent () { return parent; };
void				 TearDone (BRect place, bool newwin);
void				 InvalidateWindow ();

protected:
virtual BPoint ScreenLocation ();

private:
typedef BMenu inherited;
DragWindow			*fWindow;
PatternMenuButton	*parent;
PatternItem			*items[MAX_PATTERNS];
int 				 index;
BView				*view;
float				 hs, vs;
};

#endif 