#include <time.h>
#include <Window.h>
#include <Roster.h>
#include "Becasso.h"
#include "AboutView.h"
#include "SplashWindow.h"
#include "debug.h"
#include "mmx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <Resources.h>
#include <Screen.h>
#include <Bitmap.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Alert.h>
#include "Settings.h"
#include "RegWindow.h"

//#define TIME_LIMITED 1

#define REGISTERED		0
#define EASTER_EGG		false

#define KEYFILE_MASK 0x87, 0xF6, 0xA1, 0x7C, 0xD6, 0x3A, 0x87, 0xE6, 0x0E, 0x84, 0xF6, 0xCA, 0x81, 0xBF, 0x76, 0x12, \
 0xE8, 0x3D, 0xF7, 0x6A, 0x29, 0xC3, 0x84, 0xB5, 0x6B, 0xC3, 0x4A, 0x8F, 0x76, 0xD9, 0x8A, 0x76, 0xA2, 0x3B, 0x98, 0xD7, \
 0xE6, 0xF5, 0x9C, 0x87, 0x6C, 0x49, 0x8C, 0x7D, 0x65, 0xD2, 0xAF, 0x17, 0x8A, 0xD6, 0x49, 0xF8, 0x7A, 0x1B, 0x23, 0x9C, \
 0x7C, 0xC8, 0xA6, 0x19, 0xEE, 0x28, 0x7F, 0x68, 0x48, 0x7D, 0xD6, 0x5A, 0xB3, 0x8C, 0xF7, 0x52, 0xFE, 0x32, 0xD9, 0x84, \
 0xA7, 0xA5, 0xC9, 0x87, 0x2A, 0x8F, 0xF7, 0x6D, 0x48, 0x7D, 0xE6, 0x25, 0xED, 0x54, 0x9B, 0xC3, 0x6A, 0xA5, 0x98, 0x64, \
 0xAC, 0x10, 0xD9, 0x8F, 0xE7, 0x30, 0xA9, 0x54, 0xE4, 0x5C, 0xD4, 0x35, 0xA4, 0x35, 0xFF, 0x26, 0x4F, 0x87, 0xEA, 0x65, \
 0x3B, 0xDE, 0x48, 0x76, 0x1E, 0xB8, 0x72, 0xC5, 0xD8, 0x5E, 0x8A, 0x24

Becasso *mainapp;
SplashWindow *splash;
bool VerbQuit, VerbAddOns, NoAddOns, EasterEgg, UseMMX, ShowColors, ExportCstruct;
bool PatronizeMIME, BuiltInTablet, NoSettings, WriteSettings;
int gGlobalAlpha;		// this is actually "Registered", but for people with debuggers...
char gAlphaBuffer[256];	// this actually holds the key file
char gAlphaMask[128];	// Registered to

int SilentOperation;

PrefsLoader g_prefsloader;

extern "C" int sqr (int a);


void generate_alphabuffer (char *alpha);
// This is for existing Becasso users upon first launch, to generate
// the keyfile.  Silly check: simply look for the SoftwareValet db file
// (it need only exist, so a user can fake it by simply touching it.  Oh well.)
void generate_alphabuffer (char *alpha)
{
	BPath p;
	char reg = REG_NONE;
#if defined (__POWERPC__)
	reg = REG_PPC;
#else
	char path[B_FILE_NAME_LENGTH];
	find_directory (B_USER_SETTINGS_DIRECTORY, &p);
	strcpy (path, p.Path());
	strcat (path, "/packages/Becasso 1.41.4.db");
	FILE *db = fopen (path, "rb");
	if (db)		// db file exists, so generate the keyfile here.
		reg = REG_HAS_14;
	fclose (db);
#endif
#if defined (__HAIKU__)
	reg = REG_HAIKU;
#endif
	// Other possibility: Preinstalled Becasso
	if (reg == REG_NONE)
	{
		char path[B_FILE_NAME_LENGTH];
		find_directory (B_USER_SETTINGS_DIRECTORY, &p);
		strcpy (path, p.Path());
		strcat (path, "/Becasso/Preinstalled");
		FILE *db = fopen (path, "rb");
		if (db)		// file exists, so generate the keyfile here.
			reg = REG_PREINST;
		fclose (db);
	}
	// Other possibility: This is a CD installation
	if (reg == REG_NONE)
	{
		bool done = false;
		while (!done)
		{
			FILE *cd = fopen ("/BECASSO/README", "rb");
			if (cd)
			{
				reg = REG_CD;
				done = true;
			}
			fclose (cd);
			if (!done)
			{
				int button = (new BAlert ("Reg", lstring (423, "For registration: please mount the Becasso 2.0 CD-ROM"), lstring (131, "Cancel"), lstring (424, "Done")))->Go();
				if (button == 0)
					done = true;
			}
		}
	}
	if (reg == REG_NONE)
		return;
	
	RegWindow *rw = new RegWindow (BRect (100, 100, 440, 240), lstring (420, "Register Becasso 2.0"), reg);
	int val = rw->Go();
	rw->Lock();
	rw->Quit();
	
	if (val == 0)
		return;
	
	if (reg == REG_PREINST)
	{
		system ("rm -rf /boot/home/config/settings/Becasso/Preinstalled");
	}
	
	extern char gAlphaBuffer[256];
	extern char gAlphaMask[128];
	
	int i;
	for (i = 0; i <= strlen (gAlphaMask); i++)
		gAlphaBuffer[2*i] = gAlphaMask[i];
	for (i = strlen(gAlphaMask) + 1; i < 128; i++)
		gAlphaBuffer[2*i] = rand() & 0xFF;
	time_t tm = time (NULL);
	struct tm *tms = localtime (&tm);
	extern const char *Version;
	int vlen = strlen(Version);
	gAlphaBuffer[1] = vlen;
	gAlphaBuffer[3] = tms->tm_year;
	gAlphaBuffer[5] = tms->tm_mon;
	gAlphaBuffer[7] = tms->tm_mday;
	gAlphaBuffer[9] = 31;
	gAlphaBuffer[11] = 42;
	gAlphaBuffer[13] = reg;
	for (i = 0; i <= vlen; i++)
		gAlphaBuffer[2*i + 15] = Version[i];	
	char filler[] = "Well done, you have cracked the keyfile code!  Please don't pass it on, I need to eat too - Sander";
	for (i = 0; i < strlen(filler); i++)
		gAlphaBuffer[2*i + 2*vlen + 15] = filler[i];
	uint32 sum = 0;
	for (i = 0; i < 252; i++)
		sum += gAlphaBuffer[i];
	char *schar = (char *)(&sum);
	gAlphaBuffer[252] = schar[0];
	gAlphaBuffer[253] = schar[1];
	gAlphaBuffer[254] = schar[2];
	gAlphaBuffer[255] = schar[3];
	char xAlphaMask[256] = { KEYFILE_MASK };
	char wbuffer[256];
	for (i = 0; i < 255; i++)
		wbuffer[i] = gAlphaBuffer[i] ^ xAlphaMask[i];
	
	FILE *kf = fopen (alpha, "wb");
	fwrite (wbuffer, 1, 255, kf);
	fclose (kf);
	extern int gGlobalAlpha;
	gGlobalAlpha = 1;
}


void setup_alphabuffer ();
// You guessed it: This actually does registration checking.
void setup_alphabuffer ()
{
	char buffer[256];
	BPath p;
	char path[B_FILE_NAME_LENGTH];
	find_directory (B_USER_SETTINGS_DIRECTORY, &p);
	strcpy (path, p.Path());
	strcat (path, "/Becasso/Keyfile");
	FILE *kf = fopen (path, "rb");
	extern int gGlobalAlpha;
	if (!kf)
	{
		fclose (kf);
		// Hmm, there is no keyfile... Perhaps a SV db file then?
		generate_alphabuffer (path);
	}
	else
	{
		if (fread (buffer, 255, 1, kf) != 1)
		{
			(new BAlert ("", lstring (8, "Your Keyfile is damaged"), lstring (136, "OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			fclose (kf);
			return;
		}
		fclose (kf);
		// Here, demangle the keyfile into the registration info.
		// first, xor using the mask.
		char xAlphaMask[256] = { KEYFILE_MASK };
		int i;
		for (i = 0; i < 255; i++)
			gAlphaBuffer[i] = buffer[i] ^ xAlphaMask[i];
	
		// then, check the checksum
		uint32 sum = 0;
		for (i = 0; i < 252; i++)
			sum += gAlphaBuffer[i];
		char *schar = (char *)(&sum);
#if defined (__POWERPC__)
		if (gAlphaBuffer[252] != schar[3]
		 || gAlphaBuffer[253] != schar[2]
		 || gAlphaBuffer[254] != schar[1]
		 || gAlphaBuffer[255] != schar[0])
#else
		if (gAlphaBuffer[252] != schar[0]
		 || gAlphaBuffer[253] != schar[1]
		 || gAlphaBuffer[254] != schar[2]
		 || gAlphaBuffer[255] != schar[3])
#endif
		{
			fprintf (stderr, "keyfile checksum failure\n");//: %x, %x %x %x %x\n", sum, 
//				gAlphaBuffer[252], gAlphaBuffer[253], gAlphaBuffer[254], gAlphaBuffer[255]);
		 	return;	// checksum failure
		}
		
		// check the magic numbers
		if (gAlphaBuffer[9] != 31
		 || gAlphaBuffer[11] != 42)
		{
			fprintf (stderr, "keyfile magic number failure\n");
			return;
		}
		
		// OK, I believe this is a genuine keyfile.
		// Extract the name from it
		extern char gAlphaMask[128];
		for (i = 0; i < 128; i++)
			gAlphaMask[i] = gAlphaBuffer[2*i];
		gGlobalAlpha = 1;
		
		// Note that the keyfile contains more:  The registered version, kind of registration,
		// and the date of registration.  Who knows whether this will be interesting at one point.
	}
}

int main (int argc, char **argv)
{
	extern bool VerbQuit, VerbAddOns, NoAddOns, EasterEgg, UseMMX, ShowColors, BuiltInTablet, ExportCstruct;
	extern int DebugLevel, SilentOperation;
	
	VerbQuit		= false;
	VerbAddOns		= false;
	NoAddOns		= false;
	ShowColors		= false;
	PatronizeMIME	= true;
	BuiltInTablet	= false;
	ExportCstruct	= false;
	NoSettings		= false;
	WriteSettings	= false;
	EasterEgg		= EASTER_EGG;
	gGlobalAlpha	= REGISTERED;
	DebugLevel		= 0;
	SilentOperation	= 0;

	UseMMX		= true;

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'v':
			{
				VerbAddOns = true;
				break;
			}
			case 'q':
			{
				VerbQuit = true;
				break;
			}
			case 'x':
			{
				NoAddOns = true;
				break;
			}
			case 'e':
			{
				EasterEgg = !EASTER_EGG;
				break;
			}
			case 'D':
			{
				DebugLevel = atoi (&(argv[i][2]));
				fprintf (stderr, "Debug level set to %d\n", DebugLevel);
				break;
			}
			case 'd':
			{
				NoSettings = true;
				break;
			}
			case 'w':
			{
				WriteSettings = true;
				break;
			}
			case 'S':
			{
				SilentOperation = atoi (&(argv[i][2]));;
				break;
			}
			case 'M':
			{
				UseMMX = false;
				break;
			}
			case 'C':
			{
				ExportCstruct = true;
				break;
			}
			case 'c':
			{
				ShowColors = true;
				break;
			}
			case 't':
			{
				BuiltInTablet = true;
				break;
			}
			case 'h':
			{
				fprintf (stderr, "Recognized command line options:\n");
				fprintf (stderr, "  -v    Verbose add-on loading (useful for debugging your own)\n");
				fprintf (stderr, "  -q    Verbose quitting\n");
				fprintf (stderr, "  -x    Don't load add-ons (faster launch time)\n");
				fprintf (stderr, "  -d    Use default settings (don't load settings)\n");
				fprintf (stderr, "  -w    Force saving of settings (use with -d)\n");
				fprintf (stderr, "  -Dn   Debug verbosity level\n");
				fprintf (stderr, "  -Sn   Silence level (scripting operation)\n");
				fprintf (stderr, "  -M    Don't use MMX\n");
				fprintf (stderr, "  -c    Show recognized RGB color names at startup\n");
				fprintf (stderr, "  -C    Enable export as C struct\n");
				fprintf (stderr, "  -t    Switch on builtin Wacom tablet support\n");
				fprintf (stderr, "  -h    Display this message\n");
				break;
			}
			default:
				fprintf (stderr, "Unrecognized command line option: '%c'\n(Becasso -h displays command line usage help)\n", argv[i][1]);
			}
		}
	}
	#if defined (TIME_LIMITED)
		struct tm *mytime;
		time_t mytimet = time (NULL);
	#endif
	
#if defined (__INTEL__)
	if (UseMMX)
		UseMMX	= mmx_available();
#else
	UseMMX		= false;
#endif

	if (UseMMX)
		verbose (1, "MMX detected\n");
	else
		verbose (1, "No MMX\n");
	
	if (BuiltInTablet)
		verbose (1, "Builtin Tablet support enabled\n");
		
	mainapp = new Becasso();
	setup_alphabuffer();

	if (DebugLevel)
	{
		extern const char *Version;
		fprintf (stderr, "Becasso version %s %s, built %s\n",
			Version, (gGlobalAlpha ? "(Registered)" : "(Unregistered)"), __DATE__);
	}

	size_t bsize, ssize;
	void *becassodata, *sumdata;
	app_info info;
	mainapp->GetAppInfo (&info);
	BFile file (&info.ref, O_RDONLY);
	BResources res (&file);
	if (!(becassodata = res.FindResource ('blog', 128, &bsize)))
		fprintf (stderr, "Becasso logo resource not found\n");
	if (!(sumdata = res.FindResource ('slog', 129, &ssize)))
		fprintf (stderr, "Sum logo resource not found\n");
	BScreen screen;
	BRect becassorect = BRect (0, 0, 231, 93);
	BRect sumrect = BRect (0, 0, 63, 83);
	BBitmap *becasso = new BBitmap (becassorect, (screen.ColorSpace() == B_COLOR_8_BIT) ? B_COLOR_8_BIT : B_RGB32);
	// This strange color space thing is because we can't store bitmaps in 16 bit depth, so we use
	// a 32bit version for this screen mode (which doesn't look too bad).
	BBitmap *sum = new BBitmap (sumrect, B_RGB32);
	becasso->SetBits (becassodata, bsize, 0, B_RGB32);
	sum->SetBits (sumdata, ssize, 0, B_RGB32);
	BRect center = BRect (0, 0, 280, 210);
	center.OffsetTo (screen.Frame().Width()/2 - 140, screen.Frame().Height()/2 - 105);
	splash = new SplashWindow (center, becasso, sum);
	splash->Minimize (SilentOperation >= 3);
	splash->Show();
	mainapp->LoadAddOns();
	#if defined (TIME_LIMITED)
		mytime = localtime (&mytimet);
//		if (mytime->tm_year == 97 && mytime->tm_mon < 11)
		if (mytime->tm_year == 97)
			mainapp->Run();
		else
		{
			BAlert *alert = new BAlert ("", "This demo version of Becasso has expired.\nOn http://www.sumware.demon.nl you can find info on obtaining a newer version.\nThanks for your interest in Becasso!", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->Go();
		}
	#else
		mainapp->Run();
	#endif
	delete mainapp;
//	delete BTranslatorRoster::Default();
	return 0;
}
