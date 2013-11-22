/*
 *  ImageManipAddon.h
 *  Release 1.0.0 (Nov 18th 1999)
 *
 *  This header file defines the interface that should be 
 *  implemented and exported by an image manipulation add-on.
 *  You will only need to include this header when building 
 *  an actual add-on, not when just using the library.
 *
 *  Written by Edmund Vermeulen <edmundv@xs4all.nl>
 *  Public domain
 */


#ifndef _IMAGE_MANIP_ADDON_H
#define _IMAGE_MANIP_ADDON_H


#include <SupportDefs.h>


class BMessage;
class BView;
class BitmapAccessor;


/* These variables and functions should be exported by an image manipulation add-on */

extern "C"
{
_EXPORT	extern	const char addonName[];  /* required, C string, ex "Gamma Correct" */
_EXPORT	extern	const char addonInfo[];  /* required, descriptive C string, ex "Makes images brighter. Written by Slartibardfast." */
_EXPORT	extern	const char addonCategory[]; /* optional, C string, ex "Color" */
_EXPORT	extern	int32 addonVersion;  /* required, integer, ex 100 */

	/* At least one of the following two functions is required.
	   If called with checkOnly set to true, only check if you support
	   the supplied source bitmap and io extension. Return B_NOT_ALLOWED
	   if you don't. */

_EXPORT	extern	status_t Manipulate(
				BitmapAccessor *sourceBitmap,
				BMessage *ioExtension,        /* can be NULL */
				bool checkOnly);

_EXPORT	extern	status_t Convert(
				BitmapAccessor *sourceBitmap,
				BitmapAccessor *destBitmap,   /* call CreateBitmap() on this */
				BMessage *ioExtension,        /* can be NULL */
				bool checkOnly);

	/* The view will get resized to what the parent thinks is
	   reasonable. However, it will still receive MouseDowns etc.
	   Your view should change settings in the add-on immediately,
	   taking care not to change parameters for an image manipulation that
	   is currently running. Typically, you'll have a global struct for
	   settings that is atomically copied into the manipulation function
	   as a local when image manipulation starts.
	   Store your settings wherever you feel like it. */

_EXPORT	extern	status_t MakeConfig(  /* optional */
				BMessage *ioExtension,        /* can be NULL */
				BView **outView);

	/* Add the current settings to the ioExtension message, which may be
	   passed to the Manipulate() or Convert() function at some later time
	   when the user wants to use whatever settings you're using right now.
	   Add the capabilities (name and type code as int32) of all io
	   extensions that you understand to the ioCapability message. This
	   can be used by an application to check if an add-on supports a
	   certain extension. */

_EXPORT	extern	status_t GetConfigMessage(  /* optional */
				BMessage *ioExtension,
				BMessage *ioCapability);
}


#endif /* _IMAGE_MANIP_ADDON_H	*/
