
/* beta version for BeOS */

/*
 * dc1000 version 0.5 (2000-07-02)
 * Copyright (C) 2000 Fredrik Roubert <roubert@df.lth.se>
 * DC1580 code Copyright (C) 1999 Galen Brooks <galen@nine.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <memory.h>
#include <errno.h>

#if __BEOS__
#include <kernel/OS.h>
#include <posix/signal.h>
#else
#if !linux
#include <time.h>
#endif
#endif

#include "dsc.h"


#define READTIMEOUT 4


#ifdef DEBUG

#ifndef __BASE_FILE__
#define __BASE_FILE__ "dsc.c"
#endif

#define PLEASEREPORT "PLEASE REPORT THIS TO roubert@df.lth.se AT ONCE!"

#define returnerror(ERROR, FUNCTION)                                                               \
	{                                                                                              \
		errorline = __LINE__;                                                                      \
		dsc->lasterror.lerror = ERROR;                                                             \
		dsc->lasterror.lerrno = errno;                                                             \
		goto FUNCTION##_ERROR;                                                                     \
	}

#define DPRINTERR(LINE, ERROR, FUNCTION)                                                           \
	fprintf(                                                                                       \
		stderr, __BASE_FILE__ ": " #FUNCTION "() return from line %u, code == %u\n", LINE, ERROR   \
	)

#else

#define returnerror(ERROR, FUNCTION)                                                               \
	{                                                                                              \
		dsc->lasterror.lerror = ERROR;                                                             \
		dsc->lasterror.lerrno = errno;                                                             \
		goto FUNCTION##_ERROR;                                                                     \
	}

#define DPRINTERR(LINE, ERROR, FUNCTION)

#endif


#define DSC2_SEND_DATA 0x00
#define DSC2_SET_BAUD 0x04
#define DSC2_GET_INDEX 0x07
#define DSC2_OK 0x08
#define DSC2_CONNECT 0x10
#define DSC2_DELETE 0x11
#define DSC2_PREVIEW 0x14
#define DSC2_SET_RES 0x15
#define DSC2_THUMB 0x16
#define DSC2_SELECT 0x1a
#define DSC2_GET_DATA 0x1e
#define DSC2_RESET 0x1f


static const u_int8_t

	s_prefix[] = /* generic command prefix */
	{'M', 'K', 'E', ' ', 'D', 'S', 'C', ' ', 'P', 'C', ' ', ' '},

	r_prefix[] = /* generic response prefix */
	{'M', 'K', 'E', ' ', 'P', 'C', ' ', ' ', 'D', 'S', 'C', ' '},

	r_ok_1[] = /* response ok dsc1 */
	{0x00, 0x00, 0x00, 0x01, 0x01, 0x00},

	r_ok_2[] = /* response ok dsc2 */
	{0x08, 0x00, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


#if !__GNU_LIBRARY__
void
cfmakeraw(struct termios* termios_p)
{
	termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	termios_p->c_oflag &= ~OPOST;
	termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	termios_p->c_cflag &= ~(CSIZE | PARENB);
	termios_p->c_cflag |= CS8;
}
#endif

#if __BEOS__

/*
 * The BeOS code for dsc_read() is based on the work of Peter Goodeve in his
 * port of photopc to the BeOS.
 *
 */

struct reader_s {
	thread_id target;
	long timeout;
	int fd;
	char* buf;
	size_t count;
	ssize_t r;
};

int32
dsc_timer_thread(void* ctrlp)
{
	struct reader_s* ctrl;

	ctrl = (struct reader_s*)ctrlp;
	snooze((bigtime_t)ctrl->timeout);
	send_signal(ctrl->target, SIGINT);
	suspend_thread(find_thread(NULL));
	return 0;
}

int32
dsc_reader_thread(void* ctrlp)
{
	struct reader_s* ctrl;

	ctrl = (struct reader_s*)ctrlp;

	while (ctrl->r < ctrl->count)
		ctrl->r += read(ctrl->fd, ctrl->buf + ctrl->r, ctrl->count - ctrl->r);

	return 0;
}

#endif


ssize_t
dsc_read(dsc_t* dsc, void* buf, size_t count)
{
#ifdef DEBUGREAD
	int i;
#endif
	size_t n;

#if __BEOS__

	status_t tmp;
	struct reader_s ctrl;
	thread_id timer_id;

	if (count > 0) {
		ctrl.fd = dsc->fd;
		ctrl.buf = buf;
		ctrl.count = count;
		ctrl.r = 0;
		ctrl.timeout = (READTIMEOUT) * 1000000L;

		ctrl.target = spawn_thread(dsc_reader_thread, "read fd", B_NORMAL_PRIORITY, &ctrl);

		timer_id = spawn_thread(dsc_timer_thread, "timeout", B_NORMAL_PRIORITY, &ctrl);

		resume_thread(timer_id);
		resume_thread(ctrl.target);
		wait_for_thread(ctrl.target, &tmp);
		kill_thread(timer_id);
		n = ctrl.r;
	}
	else
		n = 0;

#else

	fd_set rfds;
	struct timeval tv;
	int s;
	ssize_t r;
#if !linux
	time_t t;
#endif

	FD_ZERO(&rfds);
	FD_SET(dsc->fd, &rfds);

	tv.tv_sec = READTIMEOUT;
	tv.tv_usec = 0;

#if !linux
	t = time(NULL);
#endif
	n = 0;
	while (n < count && timerisset(&tv)) {
		s = select(dsc->fd + 1, &rfds, NULL, NULL, &tv);

		if (s == 1) {
			if ((r = read(dsc->fd, (u_int8_t*)buf + n, count - n)) == -1) {
				DPRINTERR(__LINE__ + 1, errno, dsc_read);
				return -1;
			};
			n += r;
#if !linux
			/* assume tv is left unchanged after call to select() */
			if ((tv.tv_sec = READTIMEOUT + t - time(NULL)) < 0)
				tv.tv_sec = 0;
#endif
		}
		else if (s == -1) {
			DPRINTERR(__LINE__ + 1, errno, dsc_read);
			return -1;
		}
	}
#endif

#ifdef DEBUGREAD
	fprintf(stderr, "dsc_read() == %u [", (unsigned int)n);
	for (i = 0; i < n; i++)
		fprintf(
			stderr, *((char*)buf + i) >= 32 && *((char*)buf + i) < 127 ? "%c" : "\\x%02x",
			*((char*)buf + i)
		);
	fprintf(stderr, "]\n");
#endif

	if (n < count) {
#ifdef DEBUG
		fprintf(
			stderr,
			__BASE_FILE__ ": dsc_read() timeout (%u/%u) "
						  "return from line %u, code == %u\n",
			(unsigned int)n, (unsigned int)count, __LINE__ + 4, EDSCRTMOUT
		);
#endif
		dsc->lasterror.lerror = EDSCRTMOUT; /* read time out */
		return -1;
	}

	return (ssize_t)n;
}

int
dsc2_send_cmd(dsc_t* dsc, int cmd, int data, int sequence)
{
	u_int8_t databuf[16];
	int n;

	memset(databuf, 0, 16);

	databuf[0] = 0x08;
	databuf[1] = sequence;
	databuf[2] = 0xff - sequence;
	databuf[3] = cmd;
	databuf[4] = data;

	databuf[14] = cmd + data - 1; /* checksum? */

	n = write(dsc->fd, databuf, 16);
	tcdrain(dsc->fd);
	return n;
}

dsc_t*
dsc_open(const char* pathname, speed_t speed, dsc_error* error)
{
#ifdef DEBUG
	int errorline;
#endif

	dsc_t* dsc;
	int n;
	u_int8_t s_bps;
	ssize_t s;

	static const u_int8_t s_init1[] = {0x00, 0x00, 0x00, 0x01, 0x04},
						  s_init2[] = {0x00, 0x00, 0x00, 0x00, 0x02},
						  r_init2[] = {0x00, 0x00, 0x00, 0x04, 0x03, 'D', 'S', 'C'},
						  s_init3[] = {0x00, 0x00, 0x00, 0x01, 0x10, 0x00};

	switch (speed) {
	case B9600:
		s_bps = 0x02;
		break;

	case B19200:
		s_bps = 0x0d;
		break;

	case B38400:
		s_bps = 0x01;
		break;

	case B57600:
		s_bps = 0x03;
		break;

	default:
		DPRINTERR(__LINE__ + 2, EDSCBPSRNG, dsc_open);
		error->lerror = EDSCBPSRNG; /* bps out of range */
		return NULL;
	}

	if ((dsc = (dsc_t*)malloc(sizeof(dsc_t))) == NULL) {
		DPRINTERR(__LINE__ + 2, errno, dsc_open);
		error->lerror = EDSCSERRNO;
		error->lerrno = errno;
		return NULL;
	}

	if ((dsc->buf = (u_int8_t*)malloc(256)) == NULL) {
		DPRINTERR(__LINE__ + 2, errno, dsc_open);
		error->lerror = EDSCSERRNO;
		error->lerrno = errno;
		free(dsc);
		return NULL;
	}

	if ((dsc->fd = open(pathname, O_RDWR | O_NOCTTY | O_NONBLOCK)) == -1) {
		DPRINTERR(__LINE__ + 1, errno, dsc_open);
		error->lerror = EDSCSERRNO;
		error->lerrno = errno;
		free(dsc->buf);
		free(dsc);
		return NULL;
	}

#if __BEOS__

	/* remove O_NONBLOCK */
	fcntl(dsc->fd, F_SETFL, 0);

#endif

	if (tcgetattr(dsc->fd, &dsc->term) == -1)
		returnerror(EDSCSERRNO, dsc_open);

	cfmakeraw(&dsc->term);

	if (cfsetospeed(&dsc->term, B9600) == -1)
		returnerror(EDSCSERRNO, dsc_open);

#if __BEOS__

	/* this is needed, but I don't know why */
	dsc->term.c_cflag |= CLOCAL;

#endif

	if (tcsetattr(dsc->fd, TCSANOW, &dsc->term) == -1)
		returnerror(EDSCSERRNO, dsc_open);

	for (n = 0; n < 3; n++) {
		write(dsc->fd, s_prefix, 12);
		write(dsc->fd, s_init1, 5);
		write(dsc->fd, &s_bps, 1);
		tcdrain(dsc->fd);
		if ((s = dsc_read(dsc, dsc->buf, 18)) == 18) {
			if (memcmp(dsc->buf, r_prefix, 12) == 0 && memcmp(dsc->buf + 12, r_ok_1, 6) == 0)
				break; /* got handshake from camera */
			else
				s = 0;
		}
	}
	if (s != 18)
		returnerror(EDSCNOANSW, dsc_open);
	/* no answer from camera */

	if (speed != B9600) {
		if (cfsetospeed(&dsc->term, speed) == -1)
			returnerror(EDSCSERRNO, dsc_open);

		if (tcsetattr(dsc->fd, TCSANOW, &dsc->term) == -1)
			returnerror(EDSCSERRNO, dsc_open);
	}

	write(dsc->fd, s_prefix, 12);
	write(dsc->fd, s_init2, 5);
	tcdrain(dsc->fd);
	if (dsc_read(dsc, dsc->buf, 12 + 9) != 12 + 9 || memcmp(dsc->buf, r_prefix, 12) != 0 ||
		memcmp(dsc->buf + 12, r_init2, 8) != 0)
		returnerror(EDSCNOANSW, dsc_open);
	/* no answer from camera */

	switch (dsc->buf[20]) {
	case '1':
		dsc->type = dsc1;
		write(dsc->fd, s_prefix, 12);
		write(dsc->fd, s_init3, 6);
		tcdrain(dsc->fd);
		if (dsc_read(dsc, dsc->buf, 12 + 6) != 12 + 6 || memcmp(dsc->buf, r_prefix, 12) != 0 ||
			memcmp(dsc->buf + 12, r_ok_1, 6) != 0)
			returnerror(EDSCNOANSW, dsc_open);
		/* no answer from camera */
		break;

	case '2':
		dsc->type = dsc2;
		dsc2_send_cmd(dsc, DSC2_CONNECT, 0, 0);
		if (dsc_read(dsc, dsc->buf, 16) != 16 || memcmp(dsc->buf, r_ok_2, 16) != 0)
			returnerror(EDSCNOANSW, dsc_open);
		/* no answer from camera */
		break;

	default:
		returnerror(EDSCBADPCL, dsc_open);
		/* bad protocol */
	}

	return dsc;

dsc_open_ERROR:

	DPRINTERR(
		errorline,
		dsc->lasterror.lerror == EDSCSERRNO ? dsc->lasterror.lerrno : dsc->lasterror.lerror,
		dsc_open
	);

	error->lerror = dsc->lasterror.lerror;
	error->lerrno = dsc->lasterror.lerrno;
	close(dsc->fd);
	free(dsc->buf);
	free(dsc);
	return NULL;
}

int
dsc_close(dsc_t* dsc)
{
#ifdef DEBUG
	int errorline;
#endif

	static const u_int8_t s_reset[] = {0x00, 0x00, 0x00, 0x00, 0x1f};

	switch (dsc->type) {
	case dsc1:
		write(dsc->fd, s_prefix, 12);
		write(dsc->fd, s_reset, 5);
		tcdrain(dsc->fd);
		if (dsc_read(dsc, dsc->buf, 12 + 6) != 12 + 6 || memcmp(dsc->buf, r_prefix, 12) != 0 ||
			memcmp(dsc->buf + 12, r_ok_1, 6) != 0)
			returnerror(EDSCNRESET, dsc_close);
		/* could not reset camera */
		break;

	case dsc2:
		dsc2_send_cmd(dsc, DSC2_RESET, 0, 0);
		if (dsc_read(dsc, dsc->buf, 16) != 16 || memcmp(dsc->buf, r_ok_2, 16) != 0)
			returnerror(EDSCNRESET, dsc_close);
		/* could not reset camera */
		break;

	default:
		returnerror(EDSCBADPCL, dsc_close);
		/* bad protocol */
	}

	printf("Closing fd\n");
	close(dsc->fd);
	printf("Freeing buf\n");
	free(dsc->buf);
	printf("Freeing dsc\n");
	free(dsc);
	printf("Return\n");
	return 0;

dsc_close_ERROR:

	DPRINTERR(errorline, dsc->lasterror.lerror, dsc_open);
	close(dsc->fd);
	free(dsc->buf);
	free(dsc);
	return -1;
}

int
dsc_getindex(dsc_t* dsc, dsc_quality_t* buf)
{
#ifdef DEBUG
	int errorline;
#endif

	unsigned int nimg = 0, n;

	static const u_int8_t s_index[] = {0x00, 0x00, 0x00, 0x00, 0x07},
						  r_index_1[] = {0x00, 0x00, 0x00}, r_index_2[] = {0x08, 0x00, 0xff, 0x08};

	switch (dsc->type) {
	case dsc1:
		write(dsc->fd, s_prefix, 12);
		write(dsc->fd, s_index, 5);
		tcdrain(dsc->fd);
		if (dsc_read(dsc, dsc->buf, 17) != 17 || memcmp(dsc->buf, r_prefix, 12) != 0 ||
			memcmp(dsc->buf + 12, r_index_1, 3) != 0)
			returnerror(EDSCNOANSW, dsc_getindex);
		/* no answer from camera */

		if ((nimg = (int)(dsc->buf[15] >> 1)) == 0)
			return 0;

#ifdef DEBUG
		if (nimg > DSCMAXIMAGE)
			fprintf(
				stderr,
				__BASE_FILE__ ": dsc_getindex() "
							  "camera states %u images in memory "
							  "(only %u possible)\n\n" PLEASEREPORT "\n\n",
				nimg, DSCMAXIMAGE
			);
#endif

		if (dsc_read(dsc, dsc->buf, nimg * 2) != nimg * 2)
			returnerror(EDSCNOANSW, dsc_getindex);
		/* no answer from camera */

		for (n = 0; n < nimg; n++)
			buf[n] = (dsc_quality_t)dsc->buf[1 + n * 2];

		break;

	case dsc2:
		dsc2_send_cmd(dsc, DSC2_GET_INDEX, 0, 0);
		if (dsc_read(dsc, dsc->buf, 16) != 16 || memcmp(dsc->buf, r_index_2, 4) != 0)
			returnerror(EDSCNOANSW, dsc_getindex);
		/* no answer from camera */

		nimg = dsc->buf[4];
		memset(buf, unavailable, nimg);
		/* don't know how to get stats on images
		 * without downloading */

		break;

	default:
		returnerror(EDSCBADPCL, dsc_getindex);
		/* bad protocol */
	}

	return nimg;

dsc_getindex_ERROR:

	DPRINTERR(errorline, dsc->lasterror.lerror, dsc_getindex);
	return -1;
}

int
dsc_preview(dsc_t* dsc, int index)
{
#ifdef DEBUG
	int errorline;
#endif

	u_int8_t s_index = index;

	static const u_int8_t s_request[] = {0x00, 0x00, 0x00, 0x01, 0x14};

	if (index < 1 || index > DSCMAXIMAGE)
		returnerror(EDSCBADNUM, dsc_preview);
	/* bad image number */

	switch (dsc->type) {
	case dsc1:
		write(dsc->fd, s_prefix, 12);
		write(dsc->fd, s_request, 5);
		write(dsc->fd, &s_index, 1);
		tcdrain(dsc->fd);
		if (dsc_read(dsc, dsc->buf, 12 + 6) != 12 + 6 || memcmp(dsc->buf, r_prefix, 12) != 0 ||
			memcmp(dsc->buf + 12, r_ok_1, 6) != 0)
			returnerror(EDSCNOANSW, dsc_preview);
		/* no answer from camera */
		break;

	case dsc2:
		dsc2_send_cmd(dsc, DSC2_PREVIEW, index, 0);
		if (dsc_read(dsc, dsc->buf, 16) != 16)
			/* memcmp(dsc->buf, r_index_2, 4) != 0 */
			returnerror(EDSCNOANSW, dsc_preview);
		/* no answer from camera */
		break;

	default:
		returnerror(EDSCBADPCL, dsc_preview);
		/* bad protocol */
	}

	return 0;

dsc_preview_ERROR:

	DPRINTERR(errorline, dsc->lasterror.lerror, dsc_preview);
	return -1;
}

int
dsc_delete(dsc_t* dsc, int index)
{
#ifdef DEBUG
	int errorline;
#endif

	u_int8_t s_index = index;
	unsigned int s;

	static const u_int8_t s_request[] = {0x00, 0x00, 0x00, 0x01, 0x11};

	if (index < 1 || index > DSCMAXIMAGE)
		returnerror(EDSCBADNUM, dsc_delete);
	/* bad image number */

	switch (dsc->type) {
	case dsc1:
		write(dsc->fd, s_prefix, 12);
		write(dsc->fd, s_request, 5);
		write(dsc->fd, &s_index, 1);
		tcdrain(dsc->fd);

		/* redraw screen, delete image, redraw again */
		for (s = 3 * DSCPAUSE; s; s = sleep(s))
			;

		if (dsc_read(dsc, dsc->buf, 12 + 6) != 12 + 6 || memcmp(dsc->buf, r_prefix, 12) != 0 ||
			memcmp(dsc->buf + 12, r_ok_1, 6) != 0)
			returnerror(EDSCNOANSW, dsc_delete);
		/* no answer from camera */
		break;

	case dsc2:
		dsc2_send_cmd(dsc, DSC2_DELETE, index, 0);

		/* redraw screen, delete image, redraw again */
		for (s = 3 * DSCPAUSE; s; s = sleep(s))
			;

		if (dsc_read(dsc, dsc->buf, 16) != 16 || memcmp(dsc->buf, r_ok_2, 16) != 0)
			returnerror(EDSCNOANSW, dsc_delete);
		/* no answer from camera */
		break;

	default:
		returnerror(EDSCBADPCL, dsc_delete);
		/* bad protocol */
	}

	return 0;

dsc_delete_ERROR:

	DPRINTERR(errorline, dsc->lasterror.lerror, dsc_delete);
	return -1;
}

ssize_t
dsc_requestimage(dsc_t* dsc, int index)
{
#ifdef DEBUG
	int errorline;
#endif

	u_int8_t s_index = index;
	u_int32_t size = 0;
	int command = 0x1a;

	static const u_int8_t s_request[] = {0x00, 0x00, 0x00, 0x01, 0x1a},
						  r_request_1[] = {0x00, 0x00, 0x00, 0x04, 0x1d, 0x00},
						  r_request_2[] = {0x08, 0x00, 0xff, 0x1d};

	switch (dsc->type) {
	case dsc1:
		if (index < 1 || index > DSCMAXIMAGE)
			returnerror(EDSCBADNUM, dsc_requestimage);
		/* bad image number */

		write(dsc->fd, s_prefix, 12);
		write(dsc->fd, s_request, 5);
		write(dsc->fd, &s_index, 1);
		tcdrain(dsc->fd);
		if (dsc_read(dsc, dsc->buf, 12 + 6) != 12 + 6 || memcmp(dsc->buf, r_prefix, 12) != 0 ||
			memcmp(dsc->buf + 12, r_request_1, 6) != 0)
			returnerror(EDSCNOANSW, dsc_requestimage);
		/* no answer from camera */

		if (dsc_read(dsc, dsc->buf, 3) != 3)
			returnerror(EDSCNOANSW, dsc_requestimage);
		/* no answer from camera */

		size =
			((u_int32_t)dsc->buf[0] << 16) | ((u_int32_t)dsc->buf[1] << 8) | (u_int32_t)dsc->buf[2];
		break;

	case dsc2:
		command = DSC2_SELECT;
		if (index < 0) {
			index = -index;
			command = DSC2_THUMB;
		}

		if (index < 1 || index > DSCMAXIMAGE)
			returnerror(EDSCBADNUM, dsc_requestimage);
		/* bad image number */

		dsc2_send_cmd(dsc, command, index, 0);
		if (dsc_read(dsc, dsc->buf, 16) != 16 || memcmp(dsc->buf, r_request_2, 4) != 0)
			returnerror(EDSCBADNUM, dsc_requestimage);
		/* no answer from camera */

		size = ((u_int32_t)dsc->buf[6] << 16) | ((u_int32_t)dsc->buf[5] << 8);
		break;

	default:
		returnerror(EDSCBADPCL, dsc_requestimage);
		/* bad protocol */
	}

	return (ssize_t)size;

dsc_requestimage_ERROR:

	DPRINTERR(errorline, dsc->lasterror.lerror, dsc_requestimage);
	return -1;
}

ssize_t
dsc_readimageblock(dsc_t* dsc, int block, void* buf)
{
#ifdef DEBUG
	int errorline;
#endif

	u_int8_t s_block = block;
	u_int32_t size;

	static const u_int8_t getblock[] = {0x00, 0x00, 0x00, 0x02, 0x1e, 0x00};

	switch (dsc->type) {
	case dsc1:
		write(dsc->fd, s_prefix, 12);
		write(dsc->fd, getblock, 6);
		write(dsc->fd, &s_block, 1);
		tcdrain(dsc->fd);
		if (dsc_read(dsc, dsc->buf, 12 + 5) != 12 + 5 || memcmp(dsc->buf, r_prefix, 12) != 0 ||
			dsc->buf[12] != 0 || dsc->buf[13] != 0 || dsc->buf[16] != 0)
			returnerror(EDSCNOANSW, dsc_readimageblock);
		/* no answer from camera */

		size = ((u_int32_t)dsc->buf[14] << 8) | (u_int32_t)dsc->buf[15];

#ifdef DEBUG
		if (size > 1024)
			fprintf(
				stderr,
				__BASE_FILE__ ": dsc_readimageblock() "
							  "camera sends image block bigger "
							  "than 1k (%u)\n\n" PLEASEREPORT "\n\n",
				size
			);
#endif

		if (dsc_read(dsc, buf, size) != size)
			returnerror(EDSCNOANSW, dsc_readimageblock);
		/* no answer from camera */
		break;

	case dsc2:
		dsc2_send_cmd(dsc, DSC2_GET_DATA, block, block);
		if (dsc_read(dsc, dsc->buf, 4) != 4 || dsc->buf[0] != 1 || dsc->buf[1] != block ||
			dsc->buf[3] != 5)
			returnerror(EDSCNOANSW, dsc_readimageblock);
		/* no answer from camera */

		size = 1024; /* always returns 1024 bytes */

		if (dsc_read(dsc, buf, size) != size)
			returnerror(EDSCNOANSW, dsc_readimageblock);
		/* no answer from camera */

		/* checksum */
		if (dsc_read(dsc, dsc->buf + 4, 2) != 2)
			returnerror(EDSCNOANSW, dsc_readimageblock);
		/* no answer from camera */
		break;

	default:
		returnerror(EDSCBADPCL, dsc_readimageblock);
		/* bad protocol */
	}

	return (ssize_t)size;

dsc_readimageblock_ERROR:

	DPRINTERR(errorline, dsc->lasterror.lerror, dsc_readimageblock);
	return -1;
}

const char*
dsc_strerror(const dsc_error* lasterror)
{
	static const char* const errorlist[] = {
		"Unknown error code!\n"
#ifdef DEBUG
		PLEASEREPORT
#endif
		,
		"BPS out of range",
		"No answer from camera",
		"Read time out",
		"Could not reset camera",
		"Bad image number",
		"Bad protocol"
	};

	return lasterror->lerror == EDSCSERRNO							 ? strerror(lasterror->lerrno)
		   : lasterror->lerror < 1 || lasterror->lerror > EDSCMAXERR ? errorlist[0]
																	 : errorlist[lasterror->lerror];
}
