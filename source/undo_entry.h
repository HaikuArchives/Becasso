#ifndef _UNDO_ENTRY_H
#define _UNDO_ENTRY_H

#include "Layer.h"
#include "SBitmap.h"

typedef struct
{
int32		 type;
int32		 layer;
int32		 next;
SBitmap		*bitmap;
SBitmap		*sbitmap;
char		 fName[MAXLAYERNAME];
int			 fMode;
uchar		 fGlobalAlpha;
bool		 fHide;
} undo_entry;

#define UNDO_DRAW		  1
#define UNDO_SELECT		  2
#define UNDO_SWITCH		  3
#define UNDO_RESIZED	  4
#define UNDO_MERGE		  5
#define UNDO_LDEL		  6
#define UNDO_BOTH		  7

#endif