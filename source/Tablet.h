#ifndef TABLET_H
#define TABLET_H

#include <device/SerialPort.h>
#include "Wacom.h"
#include <string.h>
#include <Point.h>
#include <Rect.h>

#define UD_TABLET	1
#define ET_TABLET	2	// Graphire

class Tablet
{
public:
				 Tablet (const char *portname);
virtual			~Tablet ();
virtual bool	 Init ();
virtual status_t Update ();
virtual void	 SetScale (float x, float y) { fScaleX = x; fScaleY = y; };
virtual void	 SetRect (const BRect &rect) { fRect = rect; };
virtual BRect	 GetRect () { return fRect; };
virtual void	 SetThreshold (float t) { fTreshold = t; };
virtual BPoint	 Point ();
virtual const char *Model () { return (fModel); };
virtual uint32	 Buttons () { return (fPos.buttons); };
virtual bool	 Proximity () { return (fPos.proximity); };
virtual float	 Pressure () { return (fPos.pressure/fMaxPressure); };
virtual bool	 IsValid () { return (fValid); };
virtual float	 Threshold () { return (fTreshold); };
virtual BPoint	 Tilt () { return (BPoint (fPos.xtiltsign ? fPos.xtiltdata : -fPos.xtiltdata,
											fPos.ytiltsign ? fPos.ytiltdata : -fPos.ytiltdata)); };

private:
float			 fMaxX, fMaxY;
float			 fScaleX, fScaleY;
float			 fTreshold;
tablet_info 	 fInfo;
tablet_position	 fPos;
BRect			 fRect;
BSerialPort		*fPort;
char			 fPortName[128];
char			 fBuffer[256];
char			 fModel[32];
bool			 fValid;
float			 fMaxPressure;
int32			 fTabletType;
};

#endif
