
/* beta version for BeOS */
/* dc1000 0.5 (2000-07-02) Copyright (C) Fredrik Roubert <roubert@df.lth.se> */

#ifndef BEDSC_H
#define BEDSC_H

#include "dsc.h"

#include <termios.h>
#include <unistd.h>
#include <device/SerialPort.h>

class BeDSC
{

  private:
	dsc_t* dsc;
	dsc_error open_error;


  public:
	BeDSC()
	{
		dsc = 0;
		open_error.lerror = 0;
	}

	inline status_t Open(const char* pathname, data_rate speed)
	{
		speed_t termios_speed;

		switch (speed) {
		case B_9600_BPS:
			termios_speed = B9600;
			break;

		case B_19200_BPS:
			termios_speed = B19200;
			break;

		case B_38400_BPS:
			termios_speed = B38400;
			break;

		case B_57600_BPS:
			termios_speed = B57600;
			break;

		default:
			open_error.lerror = EDSCBPSRNG;
			/* bps out of range */
			return B_ERROR;
		}

		dsc = dsc_open(pathname, termios_speed, &open_error);

		return dsc == NULL ? B_ERROR : B_OK;
	}

	inline status_t Close(void)
	{
		if (dsc)
			return dsc_close(dsc) == -1 ? B_ERROR : B_OK;
		else
			return B_OK;
	}

	inline int GetIndex(dsc_quality_t* buf) { return dsc_getindex(dsc, buf); }

	inline status_t Preview(int index) { return dsc_preview(dsc, index) == -1 ? B_ERROR : B_OK; }

	inline status_t Delete(int index) { return dsc_delete(dsc, index) == -1 ? B_ERROR : B_OK; }

	inline ssize_t RequestImage(int index) { return dsc_requestimage(dsc, index); }

	inline ssize_t ReadImageBlock(int block, void* buf)
	{
		return dsc_readimageblock(dsc, block, buf);
	}

	inline const dsc_error* Error(void)
	{
		return open_error.lerror == 0 ? &dsc->lasterror : &open_error;
	}

	inline const char* ErrorString(void) { return dsc_strerror(Error()); }

	inline static const char* ErrorString(dsc_error* lasterror) { return dsc_strerror(lasterror); }
};

#endif
