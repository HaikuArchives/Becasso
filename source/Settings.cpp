#include "Settings.h"
#include <Path.h>
#include <Directory.h>
#include <SymLink.h>
#include <FindDirectory.h>
#include <RecentItems.h>
#include <MenuItem.h>
#include <Message.h>
#include <stdio.h>
#include <ctype.h>
#include <Application.h>

BLocker g_settings_lock("Becasso Settings Lock");
becasso_settings g_settings;
static char* lStrings[MAX_STRINGS];

BPoint
get_window_origin(uint32 index)
{
	BPoint origin;
	g_settings_lock.Lock();
	origin = g_settings.origin[index];
	g_settings_lock.Unlock();
	return origin;
}

void
set_window_origin(uint32 index, BPoint origin)
{
	g_settings_lock.Lock();
	g_settings.origin[index] = origin;
	g_settings.settings_touched = true;
	g_settings_lock.Unlock();
}

typedef struct
{
	entry_ref ref;
	time_t mtime;
} refandtime;

#ifdef BE_RECENT_MENU

BMenuItem*
make_recent_menu()
{
	g_settings_lock.Lock();
	int32 max_entries = g_settings.recents;
	g_settings_lock.Unlock();
	const char* typelist[2] = {"image", "application/postscript"};
	BMenuItem* recent = new BMenuItem(
		BRecentFilesList::NewFileListMenu(
			lstring(12, "Open…"), NULL, NULL, be_app, max_entries, false, typelist, 2, NULL
		),
		new BMessage('open')
	);
	recent->SetShortcut('O', B_COMMAND_KEY);
	return recent;
}

#else

void
make_recent_menu(BMenu* menu)
{
	for (int32 i = menu->CountItems(); i > 0; i--)
		delete menu->RemoveItem(int32(0));

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("Becasso/Recent");
		BDirectory dir(path.Path());
		refandtime entryList[256];
		int32 num_entries = 0;
		g_settings_lock.Lock();
		int32 max_entries = g_settings.recents;
		g_settings_lock.Unlock();
		BEntry entry;
		while (dir.GetNextEntry(&entry, false) == B_OK) {
			entry_ref ref;
			entry.GetRef(&ref);
			entryList[num_entries].ref = ref;

			BNode node(&entry);
			time_t mtime;
			node.GetModificationTime(&mtime); // modification time _of the link_
			entryList[num_entries++].mtime = mtime;
		}

		if (!num_entries) {
			menu->AddItem(new BMenuItem("<no entries>", NULL));
		} else // we have a list of entry_refs now.
		{
			// printf ("%ld entries found\n", num_entries);
			// Since the #items in the list will be small, do a stupid sort
			for (int i = 0; i < num_entries; i++) {
				int index = i;
				time_t mtime = entryList[i].mtime;
				time_t mtimei = mtime;
				int j;
				for (j = i; j < num_entries; j++) {
					if (difftime(entryList[j].mtime, mtimei) > 0) {
						index = j;
						mtimei = entryList[j].mtime;
					}
				}
				// j has the index of the youngest entry
				refandtime tmp = entryList[i];
				entryList[i] = entryList[index];
				entryList[index] = tmp;
			}
			if (num_entries > max_entries) {
				for (int i = max_entries; i < num_entries; i++) {
					BEntry entry(&entryList[i].ref);
					entry.Remove();
				}
			}
			for (int i = min_c(num_entries, max_entries) - 1; i >= 0; i--) {
				char name[B_FILE_NAME_LENGTH];
				entry_ref ref = entryList[i].ref;
				BEntry entry(&ref);
				entry.GetName(name);
				BMessage* msg = new BMessage('opnR');
				msg->AddRef("ref", &ref);
				//				printf ("%ld %s %s\n", entryList[i].mtime,
				// ctime(&entryList[i].mtime), name);
				BMenuItem* item = new BMenuItem(name, msg);
				item->SetTarget(be_app);
				menu->AddItem(item);
			}
		}
	}
}

#endif

void
add_to_recent(entry_ref ref)
{
	return;
	// We're using Be's version now

	BPath dirpath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &dirpath) == B_OK) {
		dirpath.Append("Becasso/Recent");
		BDirectory dir(dirpath.Path());

		BEntry entry(&ref);
		BPath path(&entry);
		// printf ("Adding %s to recent menu\n", path.Path());
		BSymLink link;
		dir.CreateSymLink(path.Leaf(), path.Path(), &link);
	}
}

int32
max_undo()
{
	g_settings_lock.Lock();
	int32 m = g_settings.max_undo + 1;
	g_settings_lock.Unlock();
	return m;
}

PrefsLoader::PrefsLoader()
{
	// printf ("PrefsLoader...\n");
	g_settings_lock.Lock();

	// defaults:
	strcpy(g_settings.language, "US-English");
	g_settings.recents = 10;
	g_settings.origin[0] = BPoint(100, 100);
	g_settings.max_undo = 8;
	g_settings.selection_type = SELECTION_IN_OUT;
	g_settings.preview_size = 64;
	g_settings.totd = 1;
	for (int index = 1; index < NUM_WINDOWS; index++)
		g_settings.origin[index] = InvalidPoint;

	g_settings.settings_touched = false;

	extern bool NoSettings;

	if (!NoSettings) {
		BPath path;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)) {
			path.SetTo("/tmp");
		}
		path.Append("Becasso/Settings");
		FILE* fp = fopen(path.Path(), "r");
		// printf ("fopen (%s)\n", path.Path());
		if (fp) {
			char line[1024];
			char name[32];
			char* ptr;
			while (true) {
				line[0] = 0;
				fgets(line, 1024, fp);
				if (!line[0]) {
					break;
				}
				ptr = line;
				while (isspace(*ptr))
					ptr++;
				if (*ptr == '#' || !*ptr) // Comment line
					continue;
				if (sscanf(ptr, "%31[a-zA-Z_0-9] =", name) != 1) {
					fprintf(stderr, "Strange Becasso settings line: %s\n", name);
				} else {
					if (!strcmp(name, "window_origin")) {
						int32 index;
						while (*ptr != '=')
							ptr++;
						ptr++;
						if (sscanf(ptr, "%ld:", &index) == 1) {
							while (*ptr != ':')
								ptr++;
							ptr++;
							if (sscanf(
									ptr, "%f,%f", &g_settings.origin[index].x,
									&g_settings.origin[index].y
								) != 2)
								fprintf(
									stderr, "Illegal window origin in Becasso settings: %s\n", ptr
								);
						} else
							fprintf(stderr, "Illegal window index in Becasso settings: %s\n", ptr);
					} else if (!strcmp(name, "language")) {
						while (*ptr != '=')
							ptr++;
						ptr++;
						int32 i = 0;
						while (*ptr != '\n')
							g_settings.language[i++] = *ptr++;
						g_settings.language[i] = 0;
					} else if (!strcmp(name, "recent_entries")) {
						while (*ptr != '=')
							ptr++;
						ptr++;
						if (sscanf(ptr, "%ld", &g_settings.recents) != 1)
							fprintf(
								stderr, "Illegal # of Recent items in Becasso settings: %s\n", ptr
							);
					} else if (!strcmp(name, "max_undo")) {
						while (*ptr != '=')
							ptr++;
						ptr++;
						if (sscanf(ptr, "%ld", &g_settings.max_undo) != 1)
							fprintf(stderr, "Illegal max_undo # in Becasso settings: %s\n", ptr);
					} else if (!strcmp(name, "preview_size")) {
						while (*ptr != '=')
							ptr++;
						ptr++;
						if (sscanf(ptr, "%ld", &g_settings.preview_size) != 1)
							fprintf(stderr, "Illegal preview_size in Becasso settings: %s\n", ptr);
					} else if (!strcmp(name, "totd")) {
						while (*ptr != '=')
							ptr++;
						ptr++;
						if (sscanf(ptr, "%ld", &g_settings.totd) != 1)
							fprintf(stderr, "Illegal totd number in Becasso settings: %s\n", ptr);
					} else if (!strcmp(name, "selection_render")) {
						while (*ptr != '=')
							ptr++;
						ptr++;
						if (sscanf(ptr, "%ld", &g_settings.selection_type) != 1)
							fprintf(
								stderr, "Illegal selection_render in Becasso settings: %s\n", ptr
							);
					} else {
						fprintf(stderr, "Unknown Becasso setting: %s\n", name);
					}
				}
			}
			fclose(fp);
		}
	}
	g_settings_lock.Unlock();
}

PrefsLoader::~PrefsLoader()
{
	//	printf ("~PrefsLoader\n");

	extern bool NoSettings, WriteSettings;
	if (!NoSettings || WriteSettings) {
		BPath path;
		// Make sure ~/config/settings/Becasso exists
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
			BDirectory dir(path.Path());
			dir.CreateDirectory("Becasso", &dir); // this won't clobber anyway
		}
		Save();
	}
}

void
PrefsLoader::Save()
{
	// 	printf ("Save\n");
	g_settings_lock.Lock();
	if (g_settings.settings_touched) {
		BPath path;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)) {
			path.SetTo("/tmp");
		}
		path.Append("Becasso/Settings");
		FILE* fp = fopen(path.Path(), "w");
		if (fp) {
			fprintf(fp, "# Becasso settings - Sum Software (http://www.sumware.demon.nl)\n");
			fprintf(fp, "language=%s\n", g_settings.language);
			fprintf(fp, "recent_entries=%ld\n", g_settings.recents);
			fprintf(fp, "max_undo=%ld\n", g_settings.max_undo);
			fprintf(fp, "preview_size=%ld\n", g_settings.preview_size);
			fprintf(fp, "selection_render=%ld\n", g_settings.selection_type);
			fprintf(fp, "totd=%ld\n", g_settings.totd);
			for (int index = 0; index < NUM_WINDOWS; index++) {
				BPoint origin = g_settings.origin[index];
				if (origin != InvalidPoint)
					fprintf(fp, "window_origin = %d:%g,%g\n", index, origin.x, origin.y);
			}
			fclose(fp);
		} else
			fprintf(stderr, "Couldn't save Settings\n");
	}
	g_settings.settings_touched = false;
	g_settings_lock.Unlock();
}

status_t
init_strings(const char* file)
{
	FILE* fp = fopen(file, "rb");
	if (!fp)
		return B_ERROR;

	char buffer[4096];

	do {
		int index;
		int32 i = 0;
		char c = fgetc(fp);
		if (c == '#') {
			fgets(buffer, 4095, fp); // comment line
			// printf ("%s", buffer);
			continue;
		} else
			ungetc(c, fp);

		if (fscanf(fp, "%d:", &index) > 0) {
			do {
				c = fgetc(fp);
				if (c == '\n' || feof(fp))
					c = 0;
				else if (c == '\\') {
					c = fgetc(fp);
					switch (c) {
					case 'n':
						c = '\n';
						break;
					case '\\':
						break;
					default:
						fprintf(stderr, "\\%c unknown in strings\n", c);
					}
				}
				buffer[i++] = c;
			} while (c);
		}
		lStrings[index] = new char[strlen(buffer) + 1];
		strcpy(lStrings[index], buffer);
		// printf ("%d: %s.\n", index, lStrings[index]);
	} while (!feof(fp));

	fclose(fp);
	return B_OK;
}

const char*
lstring(const int32 index, const char* default_string)
{
	if (lStrings[index]) {
		// printf ("%ld -> %s\n", index, lStrings[index]);
		return lStrings[index];
	} else {
		// printf ("Warning about %ld\n", index);
		return default_string;
	}
}
