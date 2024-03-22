
/* beta version for BeOS */
/* dc1000 0.5 (2000-07-02) Copyright (C) Fredrik Roubert <roubert@df.lth.se> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#if __BEOS__ && _X86_ && !__GNU_LIBRARY__
#define __GNU_LIBRARY__ 1
#endif

#if __GNU_LIBRARY__
#include <getopt.h>
#endif

#ifndef NOSIG
#include <signal.h>
#endif

#if __BEOS__
#include <kernel/fs_attr.h>
#include <support/TypeConstants.h>
#endif

#include "dsc.h"


#define FILENAME "dsc%04u.jpg"
#define THUMBFILENAME "dsc%04u-thumbnail.jpg"

static const char envport[] = "DSCPORT", envbaud[] = "DSCBAUD";

const speed_t defaultbaud = B9600;

#if __BEOS__
static const char attr[] = "BEOS:TYPE", mime[] = "image/jpeg";
#endif

#if defined(DEBUG) && !defined(__BASE_FILE__)
#define __BASE_FILE__ "dc1000.c"
#endif

#if __GNU_LIBRARY__
#define TRYHELP "Try `%s --help' for more information.\n"
#else
#define TRYHELP "Try `%s -h' for more information.\n"
#endif


int
list(dsc_t*);
int
listlong(dsc_t*);
int
get(dsc_t*, int*, const char*, int, int, int);
int
getall(dsc_t*, int, int, int);
int
thumbnail(dsc_t*, int*, const char*, int, int);
int
thumbnailall(dsc_t*, int, int);
int
show(dsc_t*, int*, int);
int delete(dsc_t*, int*, int);
int
parse(int*, const char*);
int
getimage(dsc_t*, int, const char*, int, int, int);

#ifndef NOSIG
void
sigint_h(int);
#endif

const char* logname;
static int sigint = 0;


#ifdef NOSIG
#define sleep_s sleep
#else

/* sleep() wakes up when a signal arrives */
void
sleep_s(unsigned int seconds)
{
	for (; seconds; seconds = sleep(seconds))
		;
}

#endif


int
main(int argc, char* argv[])
{
	char op = '\0';
	dsc_t* dsc;
	int error = 0;
	dsc_error dsc_open_error;
#ifndef NOSIG
	struct sigaction sigact;
#endif

	const char *port = NULL, *output = NULL, *cbaud = NULL;
	speed_t baud = B0;
	int flag_V = 0, flag_q = 0, flag_c = 0, index[DSCMAXIMAGE + 1];

	logname = argv[0];
	index[0] = 0;

	for (;;) {
		int c;

		static const char optstring[] = "hvp:b:o:VqclLg:Gt:Ts:d:";

#if __GNU_LIBRARY__
		static const struct option longopts[] = {
			{"help", no_argument, NULL, 'h'},		  {"version", no_argument, NULL, 'v'},
			{"port", required_argument, NULL, 'p'},	  {"baud", required_argument, NULL, 'b'},
			{"output", required_argument, NULL, 'o'}, {"verbose", no_argument, NULL, 'V'},
			{"quiet", no_argument, NULL, 'q'},		  {"silent", no_argument, NULL, 'q'},
			{"continue", no_argument, NULL, 'c'},	  {"list", no_argument, NULL, 'l'},
			{"listlong", no_argument, NULL, 'L'},	  {"get", required_argument, NULL, 'g'},
			{"getall", no_argument, NULL, 'G'},		  {"thumbnail", required_argument, NULL, 't'},
			{"thumbnailall", no_argument, NULL, 'T'}, {"show", required_argument, NULL, 's'},
			{"delete", required_argument, NULL, 'd'}, {0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, optstring, longopts, NULL);
#else
		c = getopt(argc, argv, optstring);
#endif

		if (c == EOF)
			break;

		switch (c) {
		case 'h': /* help */
			printf(
				"\n"
				"Usage: %s [-p port] [-b rate] [-o file] [-Vqc] {-hvlLGT|-gtsd index}\n"
				"\n"
#if __GNU_LIBRARY__
				"  -p, --port=NAME        use camera connected to serial port NAME\n"
				"  -b, --baud=RATE        communicate at line speed RATE\n"
				"  -o, --output=NAME      write to file NAME, used in conjunction with -g\n"
				"  -V, --verbose          explain what is being done\n"
				"  -q, --quiet, --silent  don't print status during file transfer\n"
				"  -c, --continue         continue an aborted transfer\n"
				"\n"
				"  -h, --help             show this text\n"
				"  -v, --version          show program version\n"
				"  -l, --list             list the contents of the camera memory\n"
				"  -L, --listlong         like list, but also shows the size of each image\n"
				"  -g, --get=INDEX        transfer image(s) INDEX from the camera\n"
				"  -G, --getall           transfer all images from the camera\n"
				"  -t, --thumbnail=INDEX  transfer thumbnail(s) INDEX from the camera\n"
				"  -T, --thumbnailall     transfer all thubmnails from the camera\n"
				"  -s, --show=INDEX       show image(s) INDEX on the camera display\n"
				"  -d, --delete=INDEX     delete image(s) INDEX from the camera memory\n"
#else
				"  -p NAME   use camera connected to serial port NAME\n"
				"  -b RATE   communicate at line speed RATE\n"
				"  -o NAME   write to file NAME, used in conjunction with -g\n"
				"  -V        explain what is being done\n"
				"  -q        don't print status during file transfer\n"
				"  -c        continue an aborted transfer\n"
				"\n"
				"  -h        show this text\n"
				"  -v        show program version\n"
				"  -l        list the contents of the camera memory\n"
				"  -L        like list, but also shows the size of each image\n"
				"  -g INDEX  transfer image(s) INDEX from the camera\n"
				"  -G        transfer all images from the camera\n"
				"  -t INDEX  transfer thumbnail(s) INDEX from the camera\n"
				"  -T        transfer all thubmnails from the camera\n"
				"  -s INDEX  show image(s) INDEX on the camera display\n"
				"  -d INDEX  delete image(s) INDEX from the camera memory\n"
#endif
				"\n"
				"Report bugs to <roubert@df.lth.se>.\n"
				"\n",
				argv[0]
			);
			return 0;

		case 'v': /* version */
			printf("dc1000 0.5 (2000-06-02) BeOS beta\n"
				   "Copyright (C) 2000 Fredrik Roubert <roubert@df.lth.se>\n"
				   "dc1580 code copyright (C) 1999 Galen Brooks <galen@nine.com>\n"
				   "This program comes with NO WARRANTY, to the extent permitted by law.\n"
				   "You may freely distribute copies of the program as long as this\n"
				   "copyright notice is left intact.\n");
			return 0;

		case 'p': /* port */
			port = optarg;
			break;

		case 'b': /* baud */
			cbaud = optarg;
			break;

		case 'o': /* output */
			output = optarg;
			break;

		case 'V': /* verbose */
			flag_V = 1;
			break;

		case 'q': /* quiet */
			flag_q = 1;
			break;

		case 'c': /* continue */
			flag_c = 1;
			break;

		case 'g': /* get */
		case 't': /* thumbnail */
		case 's': /* show */
		case 'd': /* delete */
			if (parse(index, optarg) == -1)
				return -1;

		case 'l': /* list */
		case 'L': /* listlong */
		case 'G': /* getall */
		case 'T': /* thumbnailall */
			op = c;
			break;

		case '?':
			fprintf(stderr, TRYHELP, logname);
			return 1;

		default:
			fprintf(
				stderr,
				"%s: error parsing command line, "
#if __GNU_LIBRARY__
				"getopt_long()"
#else
				"getopt()"
#endif
				" returned 0x%02x\n",
				logname, c
			);
			return 1;
		}
	}

	if (optind < argc) {
		fprintf(stderr, "%s: unrecognized argument `%s'\n", logname, argv[optind]);
		return 1;
	}

	if (!op) {
		fprintf(stderr, "%s: nothing to do\n" TRYHELP, logname, logname);
		return 1;
	}

	if (port == NULL)
		port = getenv(envport);

	if (cbaud == NULL)
		cbaud = getenv(envbaud);

	if (port == NULL) {
		fprintf(stderr, "%s: no port specified\n" TRYHELP, logname, logname);
		return 1;
	}

	if (cbaud == NULL)
		baud = defaultbaud;
	else
		switch (atoi(cbaud)) {
		case 9600:
			baud = B9600;
			break;

		case 19200:
			baud = B19200;
			break;

		case 38400:
			baud = B38400;
			break;

		case 57600:
			baud = B57600;
			break;

		default:
			fprintf(
				stderr,
				"%s: unsupported baudrate `%u'\n"
				"Use one of 9600, 19200, 38400 "
				"or 57600 instead.\n",
				logname, atoi(cbaud)
			);
			return 1;
		}

	if (output != NULL && op != 'g')
		fprintf(stderr, "%s: Ignoring option -o when using -%c\n", logname, op);

	if (output != NULL && op == 'g' && index[0] == 1 && output[0] == '-' && output[1] == '\0') {
		flag_q = 1;

		if (flag_V) {
			flag_V = 0;
			fprintf(
				stderr,
				"%s: Ignoring option -V "
				"when output is stdout\n",
				logname
			);
		}

		if (flag_c) {
			flag_c = 0;
			fprintf(
				stderr,
				"%s: Ignoring option -c "
				"when output is stdout\n",
				logname
			);
		}
	}

	if (flag_c && op != 'g' && op != 'G') {
		flag_c = 0;
		fprintf(
			stderr,
			"%s: Ignoring option -c "
			"when operation is not -g or -G\n",
			logname
		);
	}

	if (flag_V) {
		printf("\nPort: %s\nBaud: ", port);
		switch (baud) {
		case B9600:
			printf("9600");
			break;

		case B19200:
			printf("19200");
			break;

		case B38400:
			printf("38400");
			break;

		case B57600:
			printf("57600");
			break;
		}

		if (index[0] > 0) {
			int i, f;

			printf(index[0] > 1 ? "\n\nImages: " : "\n\nImage: ");
			for (i = 1, f = 1; i <= DSCMAXIMAGE; i++)
				if (index[i]) {
					if (f)
						f = 0;
					else
						printf(", ");
					printf("%u", i);
				}
		}

		printf("\n\nInitializing connection...\n");
	}

#ifndef NOSIG
	sigact.sa_handler = sigint_h;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;

	if (sigaction(SIGINT, &sigact, NULL) == -1)
		fprintf(stderr, "%s: System call sigaction() failed\n", logname);
#endif

	if ((dsc = dsc_open(port, baud, &dsc_open_error)) == NULL) {
		fprintf(stderr, "%s: %s\n", logname, dsc_strerror(&dsc_open_error));
		return 1;
	}

	if (flag_V) {
		printf("Connected. Camera model is ");
		switch (dsc->type) {
		case unknown:
			printf("unknown");
			break;

		case dsc1:
			printf("DC1000");
			break;

		case dsc2:
			printf("DC1580");
			break;
		}
		printf(".\nWaiting for camera to redraw screen...\n");
	}

	sleep_s(DSCPAUSE);

	if (!sigint)
		switch (op) {
		case 'l': /* list */
			if (flag_V)
				printf("Requesting index "
					   "from camera...\n");
			error = list(dsc);
			break;

		case 'L': /* listlong */
			if (flag_V)
				printf("Requesting index "
					   "from camera...\n");
			error = listlong(dsc);
			break;

		case 'g': /* get */
			error = get(dsc, index, output, flag_V, flag_q, flag_c);
			break;

		case 'G': /* getall */
			error = getall(dsc, flag_V, flag_q, flag_c);
			break;

		case 't': /* thumbnail */
			error = thumbnail(dsc, index, output, flag_V, flag_q);
			break;

		case 'T': /* thumbnailall */
			error = thumbnailall(dsc, flag_V, flag_q);
			break;

		case 's': /* show */
			error = show(dsc, index, flag_V);
			break;

		case 'd': /* delete */
			error = delete (dsc, index, flag_V);
			break;
		}

	if (sigint)
		fprintf(stderr, "%s: Interrupt from keyboard\n", logname);
	else if (error == -1)
		fprintf(stderr, "%s: %s\n", logname, dsc_strerror(&dsc->lasterror));

	if (flag_V)
		printf("Closing connection to camera...\n");

	if (dsc_close(dsc) == -1) {
		fprintf(stderr, "%s: %s\n", logname, dsc_strerror(&dsc->lasterror));
		error = -1;
	}
	else if (flag_V)
		printf("Connection successfully closed.\n\n");

	return (error == -1 ? 1 : 0);
}

int
list(dsc_t* dsc)
{
	int n, nimg;
	dsc_quality_t buf[DSCMAXIMAGE];

	if ((nimg = dsc_getindex(dsc, buf)) == -1)
		return -1;

	printf("\nTotal %u images.\n", nimg);

	for (n = 0; n < nimg; n++) {
		printf("\nImage %02u", n + 1);
		switch (buf[n]) {
		case unavailable:
			break;

		case normal:
			printf(": Normal");
			break;

		case fine:
			printf(": Fine");
			break;

		case superfine:
			printf(": Super fine");
			break;
		}
	}
	printf("\n\n");

	return 0;
}

int
listlong(dsc_t* dsc)
{
	int n, nimg;
	dsc_quality_t buf[DSCMAXIMAGE];

	if ((nimg = dsc_getindex(dsc, buf)) == -1)
		return -1;

	for (n = 0; n < nimg && !sigint; n++) {
		printf(" %7i  " FILENAME, (int)dsc_requestimage(dsc, n + 1), n + 1);
		switch (buf[n]) {
		case unavailable:
			break;

		case normal:
			printf("  normal");
			break;

		case fine:
			printf("  fine");
			break;

		case superfine:
			printf("  super fine");
			break;
		}
		putchar('\n');
	}

	return (sigint ? -1 : 0);
}

int
get(dsc_t* dsc, int* index, const char* output, int flag_V, int flag_q, int flag_c)
{
	int n;

	if (output != NULL && index[0] > 1) {
		output = NULL;
		fprintf(stderr, "%s: Ignoring option -o for multiple images\n", logname);
	}

	for (n = 1; n <= DSCMAXIMAGE; n++)
		if (index[n] && getimage(dsc, n, output, flag_V, flag_q, flag_c) == -1)
			return -1;

	return 0;
}

int
getall(dsc_t* dsc, int flag_V, int flag_q, int flag_c)
{
	int n, nimg;
	dsc_quality_t buf[DSCMAXIMAGE];

	if ((nimg = dsc_getindex(dsc, buf)) == -1)
		return -1;

	for (n = 1; n <= nimg; n++)
		if (getimage(dsc, n, NULL, flag_V, flag_q, flag_c) == -1)
			return -1;

	return 0;
}

int
thumbnail(dsc_t* dsc, int* index, const char* output, int flag_V, int flag_q)
{
	int n;

	if (output != NULL && index[0] > 1) {
		output = NULL;
		fprintf(stderr, "%s: Ignoring option -o for multiple images\n", logname);
	}

	for (n = 1; n <= DSCMAXIMAGE; n++)
		if (index[n] && getimage(dsc, -n, output, flag_V, flag_q, 0) == -1)
			return -1;

	return 0;
}

int
thumbnailall(dsc_t* dsc, int flag_V, int flag_q)
{
	int n, nimg;
	dsc_quality_t buf[DSCMAXIMAGE];

	if ((nimg = dsc_getindex(dsc, buf)) == -1)
		return -1;

	for (n = 1; n <= nimg; n++)
		if (getimage(dsc, -n, NULL, flag_V, flag_q, 0) == -1)
			return -1;

	return 0;
}

int
show(dsc_t* dsc, int* index, int flag_V)
{
	int n;

	for (n = 1; n <= DSCMAXIMAGE && !sigint; n++)
		if (index[n]) {
			if (flag_V)
				printf(
					"Sending request to show image "
					"#%u...\n",
					n
				);
			if (dsc_preview(dsc, n) == -1)
				return -1;
			if (flag_V)
				printf("Waiting for camera to "
					   "redraw screen...\n");
			sleep_s(DSCPAUSE);
		}

	return (sigint ? -1 : 0);
}

int delete(dsc_t* dsc, int* index, int flag_V)
{
	int n, i;

	for (n = 1; n <= DSCMAXIMAGE && !sigint;)
		if (index[n]) {
			if (flag_V)
				printf(
					"Sending request to delete image "
					"#%u...\n",
					n
				);
			if (dsc_delete(dsc, n) == -1)
				return -1;

			for (i = n; i < DSCMAXIMAGE; i++)
				index[i] = index[i + 1];
			index[DSCMAXIMAGE] = 0;
		}
		else
			n++;

	return (sigint ? -1 : 0);
}

int
parse(int* index, const char* arg)
{
	int i, p, lp, l, n;
	char *a, ap;

	for (i = 1; i <= DSCMAXIMAGE; i++)
		index[i] = 0;

	if ((a = strdup(arg)) == NULL) {
#ifdef DEBUG
		fprintf(
			stderr,
			__BASE_FILE__ ": parse() "
						  "return from line %u, strdup() failed\n",
			__LINE__ + 3
		);
#endif
		return -1;
	}

	for (p = 0, lp = 0, l = -1;;)
		if (a[p] >= '0' && a[p] <= '9')
			p++;
		else if (a[p] == ',' || a[p] == '-' || a[p] == '\0') {
			ap = a[p];
			a[p] = '\0';
			n = atoi(a + lp);

			if (n < 1 || n > DSCMAXIMAGE) {
				fprintf(stderr, "%s: bad index `%i'\n", logname, n);
				break;
			}

			if (ap == '-')
				l = n;
			else if (l == -1)
				index[n] ^= 1;
			else {
				for (i = (l < n ? l : n); i <= (n > l ? n : l); i++)
					index[i] ^= 1;
				l = -1;
			}

			if (ap == '\0')
				break;

			lp = ++p;
		}
		else {
			fprintf(stderr, "%s: bad character `%c' in index\n", logname, a[p]);
			break;
		}

	free(a);

	for (i = 1, n = 0; i <= DSCMAXIMAGE; i++)
		n += index[i];

	index[0] = n; /* number of images */

	return (n < 1 ? -1 : 0);
}

int
getimage(dsc_t* dsc, int index, const char* name, int flag_V, int flag_q, int flag_c)
{
#ifdef DEBUG
	int errline;

#define returnerror()                                                                              \
	{                                                                                              \
		errline = __LINE__;                                                                        \
		goto getimage_ERROR;                                                                       \
	}
#define returnerrno()                                                                              \
	{                                                                                              \
		errline = __LINE__;                                                                        \
		dsc->lasterror.lerror = EDSCSERRNO;                                                        \
		dsc->lasterror.lerrno = errno;                                                             \
		goto getimage_ERROR;                                                                       \
	}

#else

#define returnerror()                                                                              \
	{                                                                                              \
		goto getimage_ERROR;                                                                       \
	}
#define returnerrno()                                                                              \
	{                                                                                              \
		dsc->lasterror.lerror = EDSCSERRNO;                                                        \
		dsc->lasterror.lerrno = errno;                                                             \
		goto getimage_ERROR;                                                                       \
	}

#endif

	int file = -1, i;
	unsigned int cblk = 0;
	char tmpname[PATH_MAX];
	u_int8_t buf[2048];
	ssize_t imagesize, s, b, csize = 0;
	struct stat statbuf;

	if (name == NULL) {
		if (index < 0)
			sprintf(tmpname, THUMBFILENAME, -index);
		else
			sprintf(tmpname, FILENAME, index);
		name = tmpname;
	}

	if (flag_c) {
		if (stat(name, &statbuf) == -1) {
			if (errno != ENOENT)
				returnerrno();

			flag_c = 0;
		}
		else {
			if (flag_V)
				printf("Found file \"%s\" %lu bytes.\n", name, (unsigned long)statbuf.st_size);

			if ((imagesize = dsc_requestimage(dsc, index)) == -1)
				returnerror();

			if (flag_V)
				printf("Requested image #%u is %u bytes.\n", index, (unsigned int)imagesize);

			if (statbuf.st_size == imagesize) {
				if (flag_V)
					printf("Image already "
						   "downloaded, skipping.\n");
				else if (!flag_q)
					printf(
						"%s (%u bytes) already "
						"downloaded.\n",
						name, (unsigned int)imagesize
					);

				return 0;
			}

			if (statbuf.st_size > imagesize) {
				if (flag_V)
					printf("File is larger than "
						   "image, aborting.\n");
				else if (!flag_q)
					printf(
						"%s (%lu bytes) is larger "
						"than image #%u (%u bytes), "
						"aborting.\n",
						name, (unsigned long)statbuf.st_size, index, (unsigned int)imagesize
					);

				return -1;
			}

			cblk = statbuf.st_size / 1024;
			csize = cblk * 1024;

			if ((file = open(name, O_WRONLY)) == -1)
				returnerrno();

			if (lseek(file, csize, SEEK_SET) != csize)
				returnerrno();

			if (flag_V) {
				printf("Remaining size is %u bytes. Reading", (unsigned int)(imagesize - csize));
				if (flag_q)
					printf("...\n");
			}
			else if (!flag_q) {
				printf("%s (%u bytes) reading", name, (unsigned int)imagesize);
				for (i = 0; i < cblk; i++)
					putchar(',');
			}
		}
	}

	if (!flag_c) {
		if (flag_V)
			printf("Creating file \"%s\"...\n", name);

		if (name[0] == '-' && name[1] == '\0')
			file = STDOUT_FILENO;
		else if ((file = creat(name, 00666)) == -1)
			returnerrno();

		if (flag_V)
			printf(
				"Sending request to download %s #%u...\n", index < 0 ? "thumbnail" : "image",
				index < 0 ? -index : index
			);

		if ((imagesize = dsc_requestimage(dsc, index)) == -1)
			returnerror();

		if (flag_V) {
			printf("Image size is %u bytes.\nReading", (unsigned int)imagesize);
			if (flag_q)
				printf("...\n");
		}
		else if (!flag_q)
			printf("%s (%u bytes) reading", name, (unsigned int)imagesize);
	}

	if (!flag_q)
		fflush(stdout);

	if (flag_c) {
		i = cblk;
		s = csize;
	}
	else {
		i = 0;
		s = 0;
	}

	while (s < imagesize) {
		if (sigint)
			returnerrno();

		if ((b = dsc_readimageblock(dsc, i, buf)) == -1) {
			if (sigint)
				returnerrno();
			if (dsc_requestimage(dsc, index) == -1)
				returnerror();
		}
		else {
			if (write(file, buf, b) != b)
				returnerrno();
			if (!flag_q) {
				putchar('.');
				fflush(stdout);
			}
			i++;
			s += b;
		}
	}

	if (!flag_q)
		putchar('\n');

#if __BEOS__

	if (file != STDOUT_FILENO) {
		if (flag_V)
			printf("Setting MIME type...\n");

		if (fs_write_attr(file, attr, B_STRING_TYPE, 0, mime, sizeof(mime)) != sizeof(mime)) {
#ifdef DEBUG
			fprintf(
				stderr,
				__BASE_FILE__ ": getimage() "
							  "fs_write_attr() failed, code == %u\n",
				errno
			);
#else
			if (flag_V)
				printf("Could not set MIME type.\n");
#endif
		}
	}

#endif

	if (file != STDOUT_FILENO && close(file) == -1)
		return -1;

	return 0;

getimage_ERROR:

#ifdef DEBUG
	fprintf(
		stderr,
		__BASE_FILE__ ": getimage() "
					  "return from line %u, code == %u\n",
		errline, dsc->lasterror.lerror == EDSCSERRNO ? dsc->lasterror.lerrno : dsc->lasterror.lerror
	);
#else
	if (!flag_q)
		putchar('\n');
#endif

	if (file != STDOUT_FILENO && file != -1 && close(file) == -1)
		perror(logname);

	return -1;

#undef returnerror
#undef returnerrno
}

#ifndef NOSIG

void
sigint_h(int s)
{
#ifdef DEBUG
	static const char msg[] = __BASE_FILE__ ": interrupt from keyboard\n";
	write(STDERR_FILENO, msg, sizeof(msg) - 1);
#else
#ifndef NOBELL
	static const char bell = '\7';
	write(STDERR_FILENO, &bell, 1);
#endif
#endif
	sigint = 1;
}

#endif
