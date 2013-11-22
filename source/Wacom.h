/* +++++++++++
	FILE:	wacom_driver.h
	REVS:	$Revision: 1.19 $
	NAME:	William Adams
	DATE:	Mon Jun 16 11:27:53 PST 1997
	Copyright (c) 1995-97 by Be Incorporated.  All Rights Reserved.
+++++ */
// Modified for use in Becasso, © 1998 Sum Software
// Modified for Graphire Tablets [NDA] 23/11/1999 Sander Stoks

#ifndef _WACOM_DRIVER_H
#define _WACOM_DRIVER_H

const uint8 kSyncBit			= 7;
const uint8 kProximityBit 		= 6;
const uint8 kPointerBit			= 5;
const uint8 kButtonBit			= 3;
const uint8 kPressureSignBit	= 6;
const uint8 kP0Bit				= 2;
const uint8 kAp1Bit				= 2;

const uint8	kSyncBitMask = 0x1 << kSyncBit;
const uint8 kProximityBitMask = 0x1 << kProximityBit;
const uint8 kPointerBitMask = 0x1 << kPointerBit;
const uint8 kButtonBitMask = 0x1 << kButtonBit;
const uint8 kPressureSignBitMask = 0x1 << kPressureSignBit;
const uint8 kP0BitMask = 0x1 << kP0Bit;
const uint8 kAp1BitMask = 0x1 << kAp1Bit;

/*
 control codes
*/

/*
enum {
	TABLET_GETINFO = B_DEVICE_OP_CODES_END,
	TABLET_SETINFO,
	TABLET_GETPOSITION,
	TABLET_SAVESETTINGS,
	TABLET_READSETTINGS
};
*/

/*--------------------------------------------------------------------------
 General info struct used to pass control info in and 
 out of driver.
*/

enum ECommandSet {
	WT_WacomIV = 3,
	WT_WacomIIS = 2,
	WT_MM1201 = 1,
	WT_BitPad = 0
};

enum EBaudRate {
	WT_19200 = 3,
	WT_9600 = 2,
	WT_4800 = 1,
	WT_2400 = 0	
};

enum EParity {
	WT_EVEN = 3,
	WT_ODD = 2,
	WT_NONE = 0
};

enum EDataBits {
	WT_8bit = 1,
	WT_7bit = 0
};

enum EStopBits {
	WT_2STOPBITS	= 1,
	WT_1STOPBIT		= 0
};

typedef struct
{
	int8	commandset;
	int8	baudrate;
	int8	parity;
	int8	datalength;
	int8	stopbits;
	
	int8	ctsdsr;
	int8	transfermode;
	int8	outputformat;
	int8	coordinatesystem;
	int8	transferrate;
	
	int8	resolution;
	int8	origin;
	int8	outofrange;
	int8	terminator;
	
	int8	pressure;
	int8	readingheight;
	int8	multimode;
	int8	mmcommandset;
	int8	mm961orient;
	int8	bitpadIIcursor;
	int8	remotemode;
} tablet_info;


typedef struct 
{
	int32	x;
	int32	y;
	uint32	buttons;
	int32	pressuredata;
	int32	pressure;
	int32	xtiltdata;
	int32	ytiltdata;
	int32	status;
	bool	pressuresign;
	bool	proximity;
	bool	stylus;
	bool	buttonflag;
	bool	xtiltsign;
	bool	ytiltsign;
} tablet_position;

int GetLine(BSerialPort *port, char *buff, const long buffLen);
status_t convertpositiontostruct(const char *data, tablet_position &, uint32 tablet_type);
status_t convertinfotostruct(const uint32 *data, tablet_info &);

#endif
