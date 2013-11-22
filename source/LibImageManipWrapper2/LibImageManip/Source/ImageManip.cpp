/*
 *  ImageManip.cpp
 *  Release 1.1.0 (Jan 26th 2000)
 *
 *  Image manipulation library
 *
 *  Written by Edmund Vermeulen <edmundv@xs4all.nl>
 *  Public domain
 *
 *  Based on the original source code of the datatypes library by Jon Watte
 */


#define _BUILDING_imagemanip


#include <OS.h>
#include <image.h>
#include <String.h>
#include <File.h>
#include <Directory.h>
#include <Volume.h>
#include <string.h>
#include <OS.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <alloca.h>
#include <stdio.h>
#include "BitmapAccessor.h"
#include "ImageManip.h"


static int debug = 0;

// Multiple search paths are separated by colons (':')
static const char *defaultAddonPath = "/boot/home/config/add-ons/ImageManip";

// Default string for 'no category'
static const char *noCategory = "";

// File path for storing add-on settings
static const char *settingsPath = "/boot/home/config/settings/ImageManip Settings";


// Struct for keeping add-on info together
struct AddonInfo
{
	AddonInfo *next;  // linked list

	char name[NAME_MAX];
	char path[PATH_MAX];
	time_t mtime;
	image_id image;

	bool canManipulate;
	bool canConvert;

	BString addonName;
	BString addonInfo;
	BString addonCategory;
	int32 addonVersion;

	status_t (*Manipulate)(
		BitmapAccessor *sourceBitmap,
		BMessage *ioExtension,
		bool checkOnly);

	status_t (*Convert)(
		BitmapAccessor *sourceBitmap,
		BitmapAccessor *destBitmap,
		BMessage *ioExtension,
		bool checkOnly);

	status_t (*MakeConfig)(
		BMessage *ioExtension,
		BView **configView);

	status_t (*GetConfigMessage)(
		BMessage *ioExtension,
		BMessage *ioCapability);
};


class StAcquire
{
private:
	sem_id fSem;

public:
	StAcquire(sem_id sem)
	{
		acquire_sem(sem);
		fSem = sem;
	}
	~StAcquire()
	{
		release_sem(fSem);
	}
};


// Global data
static AddonInfo *firstInfo = NULL;
static sem_id sSem = 0;
static BMessage settings;


/* Get the current version of the library and the minimum
   version with which it is still compatible */
const char *
Image_Version(
	int32 *curVersion,         /* will receive the current version */
	int32 *minVersion)         /* will receive the minimum version */
{
	static char vString[80];
	static char vDate[] = __DATE__;
	if (!vString[0])
	{
		sprintf(vString, "Image manipulation library, Release %d.%d.%d (%s)",
			IMAGE_LIB_CUR_VERSION / 100,
			(IMAGE_LIB_CUR_VERSION / 10) % 10,
			IMAGE_LIB_CUR_VERSION % 10,
			vDate);
	}
	
	if (curVersion)
		*curVersion = IMAGE_LIB_CUR_VERSION;
	if (minVersion)
		*minVersion = IMAGE_LIB_MIN_VERSION;

	return vString;
}


// Load add-on image for real
static AddonInfo *
LoadAddonImage(image_addon_id imageAddon)
{
	AddonInfo *info = (AddonInfo *) imageAddon;  // dumb cast
	
	StAcquire lock(sSem);

	// Already loaded?
	if (info->image > 0)
		return info;

	image_id image = load_add_on(info->path);
	if (debug) printf("load_add_on(%s) = %ld\n", info->path, image);
	if (image <= 0)
		return NULL;

	// Find all symbols in the add-on
	const char* string = 0;
	if (get_image_symbol(image, "addonName", B_SYMBOL_TYPE_DATA, (void **) &string) != B_OK)
		return NULL;  // required
	info->addonName = string;

	if (get_image_symbol(image, "addonInfo", B_SYMBOL_TYPE_DATA, (void **) &string) != B_OK)
		return NULL;  // required
	info->addonInfo = string;

	if (get_image_symbol(image, "addonCategory", B_SYMBOL_TYPE_DATA, (void **) &string) != B_OK)
		info->addonCategory = noCategory;  // not found
	else
		info->addonCategory = string;

	long *vptr;
	if	(get_image_symbol(image, "addonVersion", B_SYMBOL_TYPE_DATA, (void **) &vptr) != B_OK)
		return NULL;  // required
	info->addonVersion = *vptr;

	info->canManipulate =
		get_image_symbol(image, "Manipulate", B_SYMBOL_TYPE_TEXT, (void **) &info->Manipulate) == B_OK;
	if (!info->canManipulate)
		info->Manipulate = NULL;

	info->canConvert =
		get_image_symbol(image, "Convert", B_SYMBOL_TYPE_TEXT, (void **) &info->Convert) == B_OK;
	if (!info->canConvert)
		info->Convert = NULL;

	if (!info->canManipulate && !info->canConvert)
		return NULL;  // at least one required

	if (get_image_symbol(image, "MakeConfig", B_SYMBOL_TYPE_TEXT, (void **) &info->MakeConfig) != B_OK)
		info->MakeConfig = NULL;  // not found

	if (get_image_symbol(image, "GetConfigMessage", B_SYMBOL_TYPE_TEXT, (void **) &info->GetConfigMessage) != B_OK)
		info->GetConfigMessage = NULL;  // not found

	info->image = image;
	return info;
}


static status_t
LoadAddon(const char *path, time_t mtime)
{
	const char *name = strrchr(path, '/');
	if (name)
		++name;
	else
		name = path;

	// Create a temporary add-on info struct on the stack
 	AddonInfo *info = new AddonInfo;
	strcpy(info->name, name);
	strcpy(info->path, path);
	info->mtime = mtime;
	info->image = 0;

	// Check if add-on is the same as last time
	BMessage addon_settings;
	if (settings.FindMessage(info->name, &addon_settings) == B_OK &&
	    addon_settings.FindInt32("mtime") == mtime)
	{
		// Use the stored info; load on demand
		info->addonName = addon_settings.FindString("addonName");
		info->addonInfo = addon_settings.FindString("addonInfo");
		info->addonCategory = addon_settings.FindString("addonCategory");
		info->addonVersion = addon_settings.FindInt32("addonVersion");
		info->canManipulate = addon_settings.FindBool("canManipulate");
		info->canConvert = addon_settings.FindBool("canConvert");
	}
	else
	{
		// Add-on is new or modified; load now
		if (!LoadAddonImage((image_addon_id) info))
		{
			delete info;
			return B_ERROR;
		}
	}

	// Add to global list
 	info->next = firstInfo;
 	firstInfo = info;

	return B_NO_ERROR;
}


static void
LoadDir(const char *path, int32& loadErr, int32& nLoaded)
{
	loadErr = B_OK;

	DIR *dir = opendir(path);
	if (debug) printf("LoadDir(%s) opendir() = %p\n", path, dir);

	if (!dir)
	{
		loadErr = B_FILE_NOT_FOUND;
		return;
	}

	struct dirent *dent;
	struct stat stbuf;
	char cwd[PATH_MAX] = "";
	while ((dent = readdir(dir)) != NULL)
	{
		strcpy(cwd, path);
		strcat(cwd, "/");
		strcat(cwd, dent->d_name);
		status_t err = stat(cwd, &stbuf);
		if (debug) printf("stat(%s) = %08lx\n", cwd, err);
		if (!err &&
		    S_ISREG(stbuf.st_mode) &&
		    strcmp(dent->d_name, ".") && strcmp(dent->d_name, ".."))
		{
			err = LoadAddon(cwd, stbuf.st_mtime);
			if (err != B_OK)
				loadErr = err;
			else
				++nLoaded;
			if (debug) printf("LoadAddon(%s) = %ld  (%ld/%08lx)\n", cwd, err, nLoaded, loadErr);
		}
	}

	closedir(dir);
}


/* Initialize the library before usage */
status_t
Image_Init(const char *path)      /* NULL for the default */
{
	if (getenv("IMAGEMANIP_DEBUG") != NULL)
		debug = 1;
	if (sSem <= 0)
		sSem = create_sem(1, "ImageManip");
	if (sSem <= 0)
		return sSem;

	// Unflatten the settings from disk
	BFile settings_file(settingsPath, B_READ_WRITE);
	settings.Unflatten(&settings_file);

	status_t loadErr = 0;
	int32 nLoaded = 0;

	if (path == NULL)
		path = getenv("IMAGEMANIP");
	if (path == NULL)
		path = defaultAddonPath;

	// Parse path syntax; load folders and files
	char pathbuf[PATH_MAX];
	const char *ptr = path;
	const char *end = ptr;
	struct stat stbuf;
	while (*ptr != 0)
	{
		// Find segments specified by colons
		end = strchr(ptr, ':');
		if (end == NULL)
			end = ptr + strlen(ptr);
		if (end-ptr > PATH_MAX - 1)
		{
			loadErr = B_BAD_VALUE;
			if (debug) printf("too long path!\n");
		}
		else
		{
			// Copy this segment of the path into a path, and load it
			memcpy(pathbuf, ptr, end - ptr);
			pathbuf[end - ptr] = 0;
			if (debug) printf("load path: %s\n", pathbuf);
			if (!stat(pathbuf, &stbuf))
			{
				// Files are loaded as add-ons
				if (S_ISREG(stbuf.st_mode))
				{
					status_t err = LoadAddon(pathbuf, stbuf.st_mtime);
					if (err != B_OK)
						loadErr = err;
					else
						++nLoaded;
				}
				else
				{
					// Directories are scanned
					LoadDir(pathbuf, loadErr, nLoaded);
				}
			} else if (debug) printf("cannot stat()!\n");
		}
		ptr = end + 1;
		if (*end == 0)
			break;
	}

	// If anything loaded, it's not too bad
	if (nLoaded)
		loadErr = B_OK;

	return loadErr;
}


/* Shutdown the library after usage */
status_t
Image_Shutdown()
{
	if (sSem <= 0)
		return B_NO_INIT;	

	acquire_sem(sSem);

	// Store all add-on settings in a BMessage
	settings.MakeEmpty();
	settings.AddString("libVersion", Image_Version(NULL, NULL));

	// Work through the linked list with add-on info
	AddonInfo *info = firstInfo;
	while (info != NULL)
	{
		AddonInfo *cur = info;
		info = info->next;

		// Each add-on's settings go into a separate BMessage
		BMessage addon_settings;
		addon_settings.AddInt32("mtime", cur->mtime);
		addon_settings.AddString("addonName", cur->addonName.String());
		addon_settings.AddString("addonInfo", cur->addonInfo.String());
		addon_settings.AddString("addonCategory", cur->addonCategory.String());
		addon_settings.AddInt32("addonVersion", cur->addonVersion);
		addon_settings.AddBool("canManipulate", cur->canManipulate);
		addon_settings.AddBool("canConvert", cur->canConvert);

		// Store the settings under the add-on's name
		settings.AddMessage(cur->name, &addon_settings);

		// Remove the add-on
		unload_add_on(cur->image);
		delete cur;
	}
	firstInfo = NULL;  // list is now empty

	// Flatten the settings to disk
	BFile settings_file(settingsPath, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	settings.Flatten(&settings_file);

	// Delete the semaphore
	delete_sem(sSem);
	sSem = 0;

	return B_NO_ERROR;
}


// Private function to get all image manipulators
static status_t
GetAllManipulators(image_addon_id **outList, int32 *outCount)
{
	// Count the manipulator add-ons
	AddonInfo *info;
	for (info = firstInfo; info != NULL; info = info->next)
	{
		if (info->canManipulate)
			++(*outCount);
	}

	// Create an array of them
	*outList = new image_addon_id[*outCount];
	*outCount = 0;
	for (info = firstInfo; info != NULL; info = info->next)
	{
		if (info->canManipulate)
		{
			(*outList)[*outCount] = (image_addon_id) info;
			++(*outCount);
		}
	}

	return B_NO_ERROR;
}


/* Get all image manipulators that support a given source bitmap */
status_t
Image_GetManipulators(
	BitmapAccessor *sourceBitmap,  /* NULL to get all manipulators */
	BMessage *ioExtension,         /* can be NULL */
	image_addon_id **outList,      /* call delete[] on it when done */
	int32 *outCount)               /* will receive number in list */
{
	*outList = NULL;
	*outCount = 0;

	if (sSem <= 0)
		return B_NO_INIT;

	if (sourceBitmap == NULL)
		return GetAllManipulators(outList, outCount);

	if (!sourceBitmap->IsValid())
		return B_BAD_TYPE;

	int32 physCount = 10;
	*outList = new image_addon_id[physCount];
	*outCount = 0;

	AddonInfo *info = firstInfo;
	while (info)
	{
		image_addon_id imageAddon = (image_addon_id) info;
	
		// Ask the add-on if it supports it
		if (info->canManipulate &&
			LoadAddonImage(imageAddon) &&
			info->Manipulate &&
		    info->Manipulate(sourceBitmap, ioExtension, true) == B_OK)
		{
			if (*outCount == physCount)
			{
				// Resize the array dynamically
				physCount *= 2;
				image_addon_id *newList = new image_addon_id[physCount];
				for (int i = 0; i < *outCount; ++i)
					newList[i] = (*outList)[i];
				delete[] *outList;
				*outList = newList;
			}

			(*outList)[*outCount] = imageAddon;
			++(*outCount);
		}

		info = info->next;
	}

	if (*outCount == 0)
	{
		delete[] *outList;
		*outList = NULL;
	}

	return B_NO_ERROR;
}


// Private function to get all image converters
static status_t
GetAllConverters(image_addon_id **outList, int32 *outCount)
{
	// Count the converter add-ons
	AddonInfo *info;
	for (info = firstInfo; info != NULL; info = info->next)
	{
		if (info->canConvert)
			++(*outCount);
	}

	// Create an array of them
	*outList = new image_addon_id[*outCount];
	*outCount = 0;
	for (info = firstInfo; info != NULL; info = info->next)
	{
		if (info->canConvert)
		{
			(*outList)[*outCount] = (image_addon_id) info;
			++(*outCount);
		}
	}

	return B_NO_ERROR;
}


/* Get all image converters that support a given source bitmap */
status_t
Image_GetConverters(
	BitmapAccessor *sourceBitmap,  /* NULL to get all converters */
	BMessage *ioExtension,         /* can be NULL */
	image_addon_id **outList,      /* call delete[] on it when done */
	int32 *outCount)               /* will receive number in list */
{
	*outList = NULL;
	*outCount = 0;

	if (sSem <= 0)
		return B_NO_INIT;

	if (sourceBitmap == NULL)
		return GetAllConverters(outList, outCount);

	if (!sourceBitmap->IsValid())
		return B_BAD_TYPE;

	int32 physCount = 10;
	*outList = new image_addon_id[physCount];
	*outCount = 0;

	AddonInfo *info = firstInfo;
	while (info)
	{
		image_addon_id imageAddon = (image_addon_id) info;

		// Check with the add-on if it supports it
		if (info->canConvert &&
			LoadAddonImage(imageAddon) &&
			info->Convert &&
		    info->Convert(sourceBitmap, NULL, ioExtension, true) == B_OK)
		{
			if (*outCount == physCount)
			{
				// Resize the array dynamically
				physCount *= 2;
				image_addon_id *newList = new image_addon_id[physCount];
				for (int i = 0; i < *outCount; ++i)
					newList[i] = (*outList)[i];
				delete[] *outList;
				*outList = newList;
			}

			(*outList)[*outCount] = imageAddon;
			++(*outCount);
		}

		info = info->next;
	}

	if (*outCount == 0)
	{
		delete[] *outList;
		*outList = NULL;
	}

	return B_NO_ERROR;
}


/* Get info on an image manipulator or image convertor add-on */
status_t
Image_GetAddonInfo(
	image_addon_id imageAddon,
	const char **addonName,        /* will receive pointer to the name */
	const char **addonInfo,        /* will receive pointer to the info */
	const char **addonCategory,    /* will receive pointer to the category */
	int32 *addonVersion)           /* will receive the version */
{
	if (!imageAddon)
		return B_BAD_VALUE;

	if (sSem <= 0)
		return B_NO_INIT;
	AddonInfo *info = (AddonInfo *) imageAddon;

	*addonName = info->addonName.String();
	*addonInfo = info->addonInfo.String();
	*addonCategory = info->addonCategory.String();
	*addonVersion = info->addonVersion;

	return B_NO_ERROR;
}


/* Let an add-on manipulate a bitmap */
status_t
Image_Manipulate(
	image_addon_id imageManipulator,
	BitmapAccessor *sourceBitmap,
	BMessage *ioExtension,         /* can be NULL */
	bool checkOnly)
{
	if (!imageManipulator || !sourceBitmap)
		return B_BAD_VALUE;

	if (!sourceBitmap->IsValid())
		return B_BAD_TYPE;

	if (sSem <= 0)
		return B_NO_INIT;

	AddonInfo *info = LoadAddonImage(imageManipulator);
	if (!info)
		return B_ERROR;

	if (!info->Manipulate)
		return B_BAD_TYPE;

	return info->Manipulate(sourceBitmap, ioExtension, checkOnly);
}


/* Let an add-on convert a bitmap to another bitmap */
status_t
Image_Convert(
	image_addon_id imageConverter,
	BitmapAccessor *sourceBitmap,
	BitmapAccessor *destBitmap,    /* will be called CreateBitmap() on */
	BMessage *ioExtension,         /* can be NULL */
	bool checkOnly)
{
	if (!imageConverter || !sourceBitmap || !destBitmap)
		return B_BAD_VALUE;

	if (!sourceBitmap->IsValid())
		return B_BAD_TYPE;

	if (sSem <= 0)
		return B_NO_INIT;

	AddonInfo *info = LoadAddonImage(imageConverter);
	if (!info)
		return B_ERROR;

	if (!info->Convert)
		return B_BAD_TYPE;

	return info->Convert(
		sourceBitmap, destBitmap, ioExtension, checkOnly);
}


/* Let an add-on make a BView that allows the user to configure it */
status_t
Image_MakeConfigurationView(
	image_addon_id imageAddon,
	BMessage *ioExtension,         /* can be NULL */
	BView **configView)            /* will receive pointer to the new BView */
{
	*configView = NULL;

	if (!imageAddon)
		return B_BAD_VALUE;

	if (sSem <= 0)
		return B_NO_INIT;

	AddonInfo *info = LoadAddonImage(imageAddon);
	if (!info)
		return B_ERROR;

	if (!info->MakeConfig)
		return B_ERROR;

	return info->MakeConfig(ioExtension, configView);
}


/* Get configuration and capabilities from an add-on */
status_t
Image_GetConfigurationMessage(
	image_addon_id imageAddon,
	BMessage *ioExtension,         /* message to add config info to */
	BMessage *ioCapability)        /* message to add capability info to */
{
	if (!imageAddon || !ioExtension || !ioCapability)
		return B_BAD_VALUE;

	if (sSem <= 0)
		return B_NO_INIT;

	AddonInfo *info = LoadAddonImage(imageAddon);
	if (!info)
		return B_ERROR;

	if (!info->GetConfigMessage)
		return B_ERROR;

	return info->GetConfigMessage(ioExtension, ioCapability);
}
