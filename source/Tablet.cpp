#include "Tablet.h"
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>

Tablet::Tablet (const char *port)
{
	openlog ("TabletDriver", LOG_THID, LOG_USER);
	strcpy (fPortName, port);
	fValid = false;
	fPort = new BSerialPort ();
	fTreshold = 0.1;
	fTabletType = 0;
	fMaxPressure = 256;
}

Tablet::~Tablet ()
{
	closelog();
	if (fValid)
		fPort->Close();
	delete fPort;
}

bool Tablet::Init ()
{
	fValid = false;
	///////////////////////
	// Something strange is going on with the serial ports on Intel...
	//
	// return (false);
	///////////////////////
	if (strlen (fPortName) < 1)
		return false;
	
	syslog (LOG_DEBUG, "Checking for Tablet at %s\n", fPortName);
	fPort->SetDataRate (B_9600_BPS);
	fPort->SetFlowControl (B_NOFLOW_CONTROL);
	fPort->SetBlocking (false);
	if (fPort->Open (fPortName) > 0)
	{
		fPort->SetTimeout (20000);
		int num;
		syslog (LOG_DEBUG, "Wrote %ld bytes\n", fPort->Write ("~#\r", 3));
		snooze (100000);
		num = GetLine (fPort, fModel, 32);
		if (num > 0)
		{
			syslog (LOG_DEBUG, "Tablet model: %s\n", fModel);
			if (!strcpy (fModel, "~#ET"))
				fTabletType = ET_TABLET;	// Graphire
			else
				fTabletType = UD_TABLET;	// ASSUMPTION!!!
			// Should check for UD here - what does a PenPartner say? (KT?)
			if (fTabletType == ET_TABLET)
			{
				fPort->Write ("ZF0\r", 4);	// Filter off.
				fMaxPressure = 512;
			}
			fPort->Write ("~C\r", 3);
			snooze (250000);
			num = GetLine (fPort, fBuffer, 255);
		//  printf ("Buffer: %s\n", fBuffer);
			fMaxX = atof (&fBuffer[2]);
			fMaxY = atof (&fBuffer[8]);
			syslog (LOG_DEBUG, "Max Coords: (%f, %f)\n", fMaxX, fMaxY);
			fRect = BRect (0, 0, fMaxX, fMaxY);
			fPos.x = fPos.y = 0;
			fValid = true;
		}
		else
			syslog (LOG_DEBUG, "No tablet at this port, or tablet switched off...\n");
	}
	else
		syslog (LOG_DEBUG, "No tablet at this port\n");
	return (fValid);
}


BPoint Tablet::Point ()
{
//	syslog (LOG_DEBUG, "Returning (%f, %f)", fScaleX*(fPos.x - fRect.left)/(fRect.Width() + 1), fScaleY*(fPos.y - fRect.top)/(fRect.Height() + 1));
	return (BPoint (fScaleX*(fPos.x - fRect.left)/(fRect.Width() + 1), fScaleY*(fPos.y - fRect.top)/(fRect.Height() + 1)));
}


status_t Tablet::Update ()
{
//	fPort->ClearOutput();
	fPort->ClearInput();
	fPort->Write ("@", 1);
	snooze (20000);
//	int32 num = GetLine (fPort, fBuffer, 255);

/*  NOTE: This was cause of a particularly nasty bug.
 *  GetLine scanned the line until a <cr> sign, ASCII 13.  However, in some
 *  coordinate data packages, that value (0xD) also appeared as part of the
 *  data structure, signalling GetLine() to end the input there!!
 *  Since 13 is also Xoff, I spent a lot of time looking in entirely the
 *  wrong direction as to what was going wrong...
*/ 
	/*int32 num = */fPort->Read (fBuffer, 7);
	/*printf ("Read %d bytes... ", num);*/ fflush (stdout);
	if (convertpositiontostruct (fBuffer, fPos, fTabletType) == B_ERROR)
	{
		return (B_ERROR);
		syslog (LOG_DEBUG, "Invalid Data.\n");
	}
	return B_OK;
}

status_t convertinfotostruct (const uint32 * /*data */, tablet_info & /*info*/)
{
	return (B_NO_ERROR);
}

status_t convertpositiontostruct (const char *data, tablet_position &info, uint32 tablet_type)
{	
	int mid;
	
	if (!(data[0] & kSyncBitMask))
	{
		// printf("convertpositiontostruct - early return, no sync bit\n");
		return B_ERROR;
	}
	
	info.proximity = data[0] & kProximityBitMask;
	info.stylus = data[0] & kPointerBitMask;
	info.buttonflag = data[0] & kButtonBitMask;

	info.x = (data[0] & 0x3)*16384;
	info.x += (data[1] & 0x7f)*128;
	info.x += (data[2] & 0x7f);
	
	info.buttons = (data[3] & 0x78) >> 3;
	
	info.y = (data[3] & 0x3)*16384;
	info.y += (data[4] & 0x7f)*128;
	info.y += (data[5] & 0x7f);
		
	info.pressuresign = (data[6] & kPressureSignBitMask) >> kPressureSignBit;
	info.pressuredata = (data[6] & 0x3f) << 1 | ((data[4] & kP0BitMask) >> kP0Bit);
	if (tablet_type == ET_TABLET)
	{
		info.pressuredata = (info.pressuredata << 1) & ((data[0] * kAp1BitMask) >> kAp1Bit);
		mid = 256;
	}
	else
		mid = 128;
	if (info.pressuresign)
		info.pressure = info.pressuredata;
	else
		info.pressure = mid + info.pressuredata;
	if (!info.proximity)
		info.pressure = 0;
	
	return B_NO_ERROR;
}

// Retrieve a line of data from the tablet.  It should be
// terminated with a <cr>
// ...but it's not, apparently.

int GetLine (BSerialPort *port, char *buff, const long buffLen)
{
	//return;
	
	bool done = false;
	char *ptr = buff;
	long totalread = 0;
	
#if 0
	
	totalread = port->Read (ptr, buffLen);
	
#else

	uint8 aChar;
	while (!done && (totalread < buffLen))
	{
		long numRead = port->Read (&aChar, 1);
		if (numRead > 0)
		{
			if (aChar == '\r' || !aChar)
			{
				*ptr = '\0';
				done = true;
				break;
			}
			else
				*ptr = aChar;
			ptr++;
			totalread++;
		}
		else	// Nothing within timeout
		{
			*ptr = 0;
			syslog (LOG_DEBUG, "<null>\n");
			done = true;
			break;
		}
	}

#endif

	port->ClearInput();
	return (totalread);
}
