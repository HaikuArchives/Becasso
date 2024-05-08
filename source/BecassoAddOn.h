// Becasso 2.0 Add-On information
// © 1998-2001 Sum Software

#ifndef _BECASSOADDON_H
#define _BECASSOADDON_H

#include <stdio.h>
#include "Colors.h"
#include "Layer.h"
#include "Modes.h"
#include "Selection.h"

class BView;

#define ADDON_EXPORT

// Undefined Hue - some of the HSV functions from Becasso can return this.
#define HUE_UNDEF -1.0

// The type of add-on
#define BECASSO_FILTER 0
#define BECASSO_TRANSFORMER 1
#define BECASSO_GENERATOR 2
#define BECASSO_CAPTURE 3

// Defines for constructing a preview info bit mask
#define PREVIEW_FULLSCALE 1
#define PREVIEW_2x2 2
#define PREVIEW_MOUSE 4
#define LAYER_AND_SELECTION 8

// Defines for addon type
#define LAYER_ONLY 1
#define SELECTION_ONLY 2

// Return codes for the process() function
#define ADDON_OK 0
#define ADDON_ABORT 1
#define ADDON_UNKNOWN 2

// Messages used in the communication with Becasso
#define ADDON_CLOSES 'AOcl'
#define ADDON_PREVIEW 'AOpr'
#define ADDON_RESIZED 'AOrz'
#define ADDON_FILTER 'ao_f'
#define ADDON_TRANSFORMER 'ao_t'
#define ADDON_GENERATOR 'ao_g'
#define CAPTURE_READY 'CTrd'

// Useful functions to keep your add-on code as
// processor-agnostic as possible
#if defined(__POWERPC__)
#define COLOR_MASK 0xFFFFFF00
#define ALPHA_MASK 0x000000FF
#define RED_MASK 0x0000FF00
#define GREEN_MASK 0x00FF0000
#define BLUE_MASK 0xFF000000
#define CYAN_MASK 0xFF000000
#define MAGENTA_MASK 0x00FF0000
#define YELLOW_MASK 0x0000FF00
#define BLACK_MASK 0x000000FF
#define IRED_MASK 0xFFFF00FF
#define IGREEN_MASK 0xFF00FFFF
#define IBLUE_MASK 0x00FFFFFF
#define ICYAN_MASK 0x00FFFFFF
#define IMAGENTA_MASK 0xFF00FFFF
#define IYELLOW_MASK 0xFFFF00FF
#define IBLACK_MASK 0xFFFFFF00
#define ALPHA_BPOS 0
#define RED_BPOS 8
#define GREEN_BPOS 16
#define BLUE_BPOS 24
#define CYAN_BPOS 24
#define MAGENTA_BPOS 16
#define YELLOW_BPOS 8
#define BLACK_BPOS 0
#define BLUE(x) ((x) >> 24)
#define GREEN(x) (((x) >> 16) & 0xFF)
#define RED(x) (((x) >> 8) & 0xFF)
#define ALPHA(x) ((x) & 0xFF)
#define CYAN(x) ((x) & 0xFF)
#define MAGENTA(x) (((x) >> 8) & 0xFF)
#define YELLOW(x) (((x) >> 16) & 0xFF)
#define BLACK(x) ((x) >> 24)
#define PIXEL(r, g, b, a)                                                                     \
	(((int(b) << 24) & 0xFF000000) | ((int(g) << 16) & 0xFF0000) | ((int(r) << 8) & 0xFF00) | \
		(int(a) & 0xFF))
#else  // IA
#define COLOR_MASK 0x00FFFFFF
#define ALPHA_MASK 0xFF000000
#define RED_MASK 0x00FF0000
#define GREEN_MASK 0x0000FF00
#define BLUE_MASK 0x000000FF
#define CYAN_MASK 0x000000FF
#define MAGENTA_MASK 0x0000FF00
#define YELLOW_MASK 0x00FF0000
#define BLACK_MASK 0xFF000000
#define IRED_MASK 0xFF00FFFF
#define IGREEN_MASK 0xFFFF00FF
#define IBLUE_MASK 0xFFFFFF00
#define ICYAN_MASK 0xFFFFFF00
#define IMAGENTA_MASK 0xFFFF00FF
#define IYELLOW_MASK 0xFF00FFFF
#define IBLACK_MASK 0x00FFFFFF
#define ALPHA_BPOS 24
#define RED_BPOS 16
#define GREEN_BPOS 8
#define BLUE_BPOS 0
#define CYAN_BPOS 0
#define MAGENTA_BPOS 8
#define YELLOW_BPOS 16
#define BLACK_BPOS 24
#define ALPHA(x) ((x) >> 24)
#define RED(x) (((x) >> 16) & 0xFF)
#define GREEN(x) (((x) >> 8) & 0xFF)
#define BLUE(x) ((x) & 0xFF)
#define CYAN(x) ((x) & 0xFF)
#define MAGENTA(x) (((x) >> 8) & 0xFF)
#define YELLOW(x) (((x) >> 16) & 0xFF)
#define BLACK(x) ((x) >> 24)
#define PIXEL(r, g, b, a)                                                                     \
	(((int(a) << 24) & 0xFF000000) | ((int(r) << 16) & 0xFF0000) | ((int(g) << 8) & 0xFF00) | \
		(int(b) & 0xFF))
#endif

// Rectangle constants
const BRect EmptyRect = BRect(0, 0, 0, 0);

// The becasso_addon_info struct
typedef struct {
	char name[80];			// The name as it appears in the menu
	uint32 index;			// A unique index assigned at init time
	int type;				// Filter, Transformer, Generator, or Capture
	int version;			// Version of the add-on
	int release;			// Release of the add-on
	int becasso_version;	// Required Becasso version (other won't load)
	int becasso_release;	// Written for release (older will warn)
	char author[128];		// Author (company) of the add-on
	char copyright[128];	// Copyright notice
	char description[256];	// Explains what the add-on does
	uint8 does_preview;		// bitmask for various preview notifications
	uint32 flags;			// bitmask
} becasso_addon_info;

// Function prototypes
extern "C" ADDON_EXPORT status_t addon_init(uint32 index, becasso_addon_info* info);
extern "C" ADDON_EXPORT status_t addon_exit(void);
extern "C" ADDON_EXPORT status_t addon_close(void);
extern "C" ADDON_EXPORT status_t addon_make_config(BView** view, BRect rect);
extern "C" ADDON_EXPORT status_t process(Layer* inLayer, Selection* inSelection, Layer** outLayer,
	Selection** outSelection, int32 mode, BRect* frame, bool final, BPoint point, uint32 buttons);
// for Capture add-ons
extern "C" ADDON_EXPORT status_t addon_open(void);
extern "C" ADDON_EXPORT BBitmap* bitmap(char* title);
// optional hooks
extern "C" ADDON_EXPORT void addon_color_changed(void);
extern "C" ADDON_EXPORT void addon_mode_changed(void);

// Status updating calls
IMPEXP void addon_start(void);
IMPEXP bool addon_stop(void);
IMPEXP void addon_done(void);
IMPEXP void addon_update_statusbar(
	float delta, const char* text = NULL, const char* trailingText = NULL);
IMPEXP void addon_reset_statusbar(const char* label = NULL, const char* trailingText = NULL);
IMPEXP void addon_preview(void);
IMPEXP void addon_refresh_config(void);

#endif
