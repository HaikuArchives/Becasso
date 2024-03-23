
/* beta version for BeOS */

/*
 * dc1000 version 0.5 (2000-07-02)
 * Copyright (C) 2000 Fredrik Roubert <roubert@df.lth.se>
 * DC1580 code Copyright (C) 1999 Galen Brooks <galen@nine.com>
 */

#ifndef DSC_H
#define DSC_H

#include <sys/types.h>
#include <termios.h>

/* What is the best way to do this? */
#if !_GNU_TYPES_H
#if sun
typedef uint8_t u_int8_t;
typedef uint32_t u_int32_t;
#else
#if __BEOS__
#include <inttypes.h>
#endif
#endif
#endif

typedef enum
{
	unknown = 0,
	dsc1 = 1,
	dsc2 = 2
} dsc_protocol_t;

typedef enum
{
	unavailable = -1,
	normal = 0,
	fine = 1,
	superfine = 2
} dsc_quality_t;

typedef struct
{
	int lerror, lerrno;
} dsc_error;

typedef struct
{
	int fd;
	struct termios term;
	dsc_protocol_t type;
	dsc_error lasterror;
	u_int8_t* buf;
} dsc_t;

#define EDSCSERRNO -1 /* see errno value */
#define EDSCUNKNWN 0  /* unknown error code */
#define EDSCBPSRNG 1  /* bps out of range */
#define EDSCNOANSW 2  /* no answer from camera */
#define EDSCRTMOUT 3  /* read time out */
#define EDSCNRESET 4  /* could not reset camera */
#define EDSCBADNUM 5  /* bad image number */
#define EDSCBADPCL 6  /* bad protocol */
#define EDSCMAXERR 6  /* highest used error code */

#ifndef DSCPAUSE
#define DSCPAUSE 4 /* seconds to wait for camera to redraw screen */
#endif

#define DSCMAXIMAGE 200 /* highest possible number of an image */


#ifdef __cplusplus
extern "C"
{
#endif


	ssize_t dsc_read(dsc_t*, void*, size_t);
	int dsc2_send_cmd(dsc_t*, int, int, int);

	dsc_t* dsc_open(const char*, speed_t, dsc_error*);
	int dsc_close(dsc_t*);
	int dsc_getindex(dsc_t*, dsc_quality_t*);
	int dsc_preview(dsc_t*, int);
	int dsc_delete(dsc_t*, int);
	ssize_t dsc_requestimage(dsc_t*, int);
	ssize_t dsc_readimageblock(dsc_t*, int, void*);

	const char* dsc_strerror(const dsc_error*);


#ifdef __cplusplus
}
#endif

#endif
