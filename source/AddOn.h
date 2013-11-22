#ifndef ADDON_H
#define ADDON_H

#include "Layer.h"
#include "Selection.h"
#include "BecassoAddOn.h"
#include "AddOnWindow.h"
#include "Modes.h"
#include <Bitmap.h>

class AddOn
{
public:
			 AddOn (BEntry entry);
			~AddOn ();
status_t	 Init (uint32 index);
status_t	 Open (BWindow *client, const char *name);
status_t	 MakeConfig (BView **view, BRect rect);
status_t	 Close (bool client_quits = false);
status_t	 Process (Layer *inLayer, Selection *inSelection,
					  Layer **outLayer, Selection **outSelection, int32 mode = M_DRAW,
					  BRect *frame = NULL, bool final = true, 
					  BPoint point = BPoint (-1, -1), uint32 buttons = 0);
BBitmap		*Bitmap (char *title);
const int	 Type () { return (fInfo.type); };
const char	*Name () { return (fInfo.name); };
const uint8	 DoesPreview () { return (fInfo.does_preview); };
void		 ColorChanged ();
void		 ModeChanged ();

void		 SetTargetOfControlsRecurse (BView *target, BView *view);

private:

status_t	 (*addon_init)(uint32 index, becasso_addon_info *info);
status_t	 (*addon_exit)(void);
status_t	 (*addon_close)(void);
status_t	 (*addon_make_config)(BView **view, BRect rect);
status_t	 (*process)(Layer *inLayer, Selection *inSelection, 
						Layer **outLayer, Selection **outSelection, int32 mode,
						BRect *frame, bool final, BPoint point, uint32 buttons);
status_t	 (*addon_open)(void);		// capture add-ons only
BBitmap	*	 (*bitmap)(char *title);	// capture add-ons only
void		 (*addon_color_changed)(void);	// optional
void		 (*addon_mode_changed)(void);	// optional

image_id	 fAddOnID;
becasso_addon_info fInfo;
};

#endif 