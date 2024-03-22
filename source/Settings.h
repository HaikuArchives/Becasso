#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <Point.h>
#include <Locker.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Entry.h>
#include <stdio.h>

static const BPoint InvalidPoint = BPoint(0, 0);

#define NUM_WINDOWS 9
#define MAX_STRINGS 9999
#define MAX_UNDO 17

#define SELECTION_IN_OUT 0
#define SELECTION_STATIC 1

enum {
	numMainWindow = 0,
	numAttribWindow = 1,
	numModeWindow = 2,
	numModeTO = 3,
	numToolTO = 4,
	numFGColorTO = 5,
	numBGColorTO = 6,
	numPatternTO = 7,
	numTOTDWindow = 8
};

#define BE_RECENT_MENU 1

BPoint
get_window_origin(uint32 win);
void
set_window_origin(uint32 win, BPoint origin);
#if BE_RECENT_MENU
BMenuItem*
make_recent_menu();
#else
void
make_recent_menu(BMenu* menu);
#endif
void
add_to_recent(entry_ref ref);
int32
max_undo();
status_t
init_strings(const char* file);
const char*
lstring(const int32 index, const char* default_string);

struct becasso_settings {
	BPoint origin[NUM_WINDOWS];
	char language[64];
	int32 recents;
	int32 max_undo;
	int32 selection_type;
	int32 preview_size;
	int32 totd;
	bool settings_touched;
};

class PrefsLoader {
  public:
	PrefsLoader();
	~PrefsLoader();
	void Save();
};

#endif
