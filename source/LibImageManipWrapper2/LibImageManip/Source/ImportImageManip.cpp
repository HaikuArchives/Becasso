/*
 *  ImportImageManip.cpp
 *  Release 1.0.0 (Nov 18th 1999)
 *
 *  Import the functions from the image manipulation library as an add-on.
 *  This allows applications to run even if libimagemanip.so isn't installed.
 *  Of course you won't get the image manipulation functionality in that case.
 *
 *  Written by Edmund Vermeulen <edmundv@xs4all.nl>
 *  Public domain
 */


// Weak linkage with libimagemanip.so
#define _IMPEXP_IMAGEMANIP


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <image.h>
#include "ImageManip.h"
#include "BBitmapAccessor.h"


#define LIBIMAGEMANIP_NAME "libimagemanip.so"


static image_id image = -1;
static const char *no_lib = "libimagemanip.so not found";


// Pointers to the real functions in the library
static const char *(*_Image_Version)(
	int32 *curVersion,
	int32 *minVersion);
static status_t (*_Image_Init)(
	const char *loadPath);
static status_t	(*_Image_Shutdown)();
static status_t	(*_Image_GetManipulators)(
	BitmapAccessor *sourceBitmap,
	BMessage *ioExtension,
	image_addon_id **outList,
	int32 *outCount);
static status_t	(*_Image_GetConverters)(
	BitmapAccessor *sourceBitmap,
	BMessage *ioExtension,
	image_addon_id **outList,
	int32 *outCount);
static status_t (*_Image_GetAddonInfo)(
	image_addon_id imageAddon,
	const char **addonName,
	const char **addonInfo,
	const char **addonCategory,
	int32 *addonVersion);
static status_t (*_Image_Manipulate)(
	image_addon_id imageManipulator,
	BitmapAccessor *sourceBitmap,
	BMessage *ioExtension,
	bool checkOnly);
static status_t (*_Image_Convert)(
	image_addon_id imageConverter,
	BitmapAccessor *sourceBitmap,
	BitmapAccessor *destBitmap,
	BMessage *ioExtension,
	bool checkOnly);
static status_t (*_Image_MakeConfigurationView)(
	image_addon_id imageAddon,
	BMessage *ioExtension,
	BView **configView);
static status_t (*_Image_GetConfigurationMessage)(
	image_addon_id imageAddon,
	BMessage *ioExtension,
	BMessage *ioCapability);
static BBitmapAccessor *(*_Image_CreateBBitmapAccessor)(
	BBitmap *bitmap,
	const BRect *section);


const char *
Image_Version(
	int32 *curVersion,
	int32 *minVersion)
{
	if (Image_Init(0) != B_OK)
		return no_lib;

	return _Image_Version(curVersion, minVersion);
}


static int
getappdir(
	char * dir,
	size_t size)
{
	image_info info;
	thread_info tinfo;
	status_t err;
	if (size < 1)
		return B_BAD_VALUE;
	if ((err = get_thread_info(find_thread(NULL), &tinfo)) != B_OK)
		return err;
	for (int32 ix=0; !get_next_image_info(tinfo.team, &ix, &info); )
	{
		if (info.type == B_APP_IMAGE)
		{
			strncpy(dir, info.name, size);
			dir[size-1] = 0;
			char * ptr = strrchr(dir, '/');
			if (ptr) ptr[1] = 0;
			return B_OK;
		}
	}
	return B_ERROR;
}


status_t
Image_Init(const char *loadPath)
{
	// Already initialised?
	if (image >= 0)
		return B_OK;

	// Load the library as an add-on
	image = load_add_on(LIBIMAGEMANIP_NAME);
	if (image < 0)
	{
		char * env = getenv("LIBRARY_PATH");
		char * end, * temp, * str;
		if (!env)
			return image;
		env = strdup(env);
		temp = env;
		while (1)
		{
			end = strchr(temp, ':');
			if (end)
				*end = 0;
			if (!strncmp(temp, "%A/", 3))
			{
				str = (char *)malloc(1024);
				if (!str)
				{
					free(env);
					return B_NO_MEMORY;
				}
				if ((errno = getappdir(str, 1023)) != B_OK)
				{
					free(env);
					free(str);
					return errno;
				}
				strcat(str, temp+2);	/*	include slash we know is there	*/
			}
			else
			{
				str = (char *)malloc(strlen(temp)+40);
				if (!str)
				{
					free(env);
					return B_NO_MEMORY;
				}
				strcpy(str, temp);
			}
			strcat(str, "/");
			strcat(str, LIBIMAGEMANIP_NAME);
			image = load_add_on(str);
			free(str);
			if (image > 0)
				break;
			if (!end)
				break;
			temp = end+1;
		}
		free(env);
		if (image < 0)
			return image;
	}

	// Import the functions in the library
	if (get_image_symbol(image, "Image_Version",                 B_SYMBOL_TYPE_TEXT, (void **) &_Image_Version)                 != B_OK ||
	    get_image_symbol(image, "Image_Init",                    B_SYMBOL_TYPE_TEXT, (void **) &_Image_Init)                    != B_OK ||
	    get_image_symbol(image, "Image_Shutdown",                B_SYMBOL_TYPE_TEXT, (void **) &_Image_Shutdown)                != B_OK ||
	    get_image_symbol(image, "Image_GetManipulators",         B_SYMBOL_TYPE_TEXT, (void **) &_Image_GetManipulators)         != B_OK ||
	    get_image_symbol(image, "Image_GetConverters",           B_SYMBOL_TYPE_TEXT, (void **) &_Image_GetConverters)           != B_OK ||
	    get_image_symbol(image, "Image_GetAddonInfo",            B_SYMBOL_TYPE_TEXT, (void **) &_Image_GetAddonInfo)            != B_OK ||
	    get_image_symbol(image, "Image_Manipulate",              B_SYMBOL_TYPE_TEXT, (void **) &_Image_Manipulate)              != B_OK ||
	    get_image_symbol(image, "Image_Convert",                 B_SYMBOL_TYPE_TEXT, (void **) &_Image_Convert)                 != B_OK ||
	    get_image_symbol(image, "Image_MakeConfigurationView",   B_SYMBOL_TYPE_TEXT, (void **) &_Image_MakeConfigurationView)   != B_OK ||
	    get_image_symbol(image, "Image_GetConfigurationMessage", B_SYMBOL_TYPE_TEXT, (void **) &_Image_GetConfigurationMessage) != B_OK ||
	    get_image_symbol(image, "Image_CreateBBitmapAccessor",   B_SYMBOL_TYPE_TEXT, (void **) &_Image_CreateBBitmapAccessor)   != B_OK ||
	    _Image_Init(loadPath) != B_OK)
	{
		unload_add_on(image);
		image = -1;
		return B_ERROR;
	}

	return B_OK;
}


status_t
Image_Shutdown()
{
	if (image < 0)
		return B_NO_INIT;

	status_t rc = _Image_Shutdown();
	unload_add_on(image);
	image = -1;
	return rc;
}


status_t
Image_GetManipulators(
	BitmapAccessor *sourceBitmap,
	BMessage *ioExtension,
	image_addon_id **outList,
	int32 *outCount)
{
	if (image < 0)
		return B_NO_INIT;

	return (*_Image_GetManipulators)(sourceBitmap, ioExtension, outList, outCount);
}

status_t
Image_GetConverters(
	BitmapAccessor *sourceBitmap,
	BMessage *ioExtension,
	image_addon_id **outList,
	int32 *outCount)
{
	if (image < 0)
		return B_NO_INIT;

	return (*_Image_GetConverters)(sourceBitmap, ioExtension, outList, outCount);
}

status_t
Image_GetAddonInfo(
	image_addon_id imageAddon,
	const char **addonName, 
	const char **addonInfo,
	const char **addonCategory,
	int32 *addonVersion)
{
	if (image < 0)
		return B_NO_INIT;

	return (*_Image_GetAddonInfo)(imageAddon, addonName, addonInfo, addonCategory, addonVersion);
}

status_t
Image_Manipulate(
	image_addon_id imageManipulator,
	BitmapAccessor *sourceBitmap,
	BMessage *ioExtension,
	bool checkOnly)
{
	if (image < 0)
		return B_NO_INIT;

	return (*_Image_Manipulate)(imageManipulator, sourceBitmap, ioExtension, checkOnly);
}


status_t
Image_Convert(
	image_addon_id imageConverter,
	BitmapAccessor *sourceBitmap,
	BitmapAccessor *destBitmap,
	BMessage *ioExtension,
	bool checkOnly)
{
	if (image < 0)
		return B_NO_INIT;

	return (*_Image_Convert)(imageConverter, sourceBitmap, destBitmap, ioExtension, checkOnly);
}


status_t
Image_MakeConfigurationView(
	image_addon_id imageAddon,
	BMessage *ioExtension,
	BView **configView)
{
	if (image < 0)
		return B_NO_INIT;

	return (*_Image_MakeConfigurationView)(imageAddon, ioExtension, configView);
}


status_t
Image_GetConfigurationMessage(
	image_addon_id imageAddon,
	BMessage *ioExtension,
	BMessage *ioCapability)
{
	if (image < 0)
		return B_NO_INIT;

	return (*_Image_GetConfigurationMessage)(imageAddon, ioExtension, ioCapability);
}


BBitmapAccessor *
Image_CreateBBitmapAccessor(
	BBitmap *bitmap,
	const BRect *section)
{
	if (image < 0)
		return NULL;

	return (*_Image_CreateBBitmapAccessor)(bitmap, section);
}
