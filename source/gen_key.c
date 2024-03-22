/* Generate keyfile for Becasso 2.0 */
/* (c) 2000 Sum Software */
/* CONFIDENTIAL */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define KEYFILE_MASK                                                                               \
	0x87, 0xF6, 0xA1, 0x7C, 0xD6, 0x3A, 0x87, 0xE6, 0x0E, 0x84, 0xF6, 0xCA, 0x81, 0xBF, 0x76,      \
		0x12, 0xE8, 0x3D, 0xF7, 0x6A, 0x29, 0xC3, 0x84, 0xB5, 0x6B, 0xC3, 0x4A, 0x8F, 0x76, 0xD9,  \
		0x8A, 0x76, 0xA2, 0x3B, 0x98, 0xD7, 0xE6, 0xF5, 0x9C, 0x87, 0x6C, 0x49, 0x8C, 0x7D, 0x65,  \
		0xD2, 0xAF, 0x17, 0x8A, 0xD6, 0x49, 0xF8, 0x7A, 0x1B, 0x23, 0x9C, 0x7C, 0xC8, 0xA6, 0x19,  \
		0xEE, 0x28, 0x7F, 0x68, 0x48, 0x7D, 0xD6, 0x5A, 0xB3, 0x8C, 0xF7, 0x52, 0xFE, 0x32, 0xD9,  \
		0x84, 0xA7, 0xA5, 0xC9, 0x87, 0x2A, 0x8F, 0xF7, 0x6D, 0x48, 0x7D, 0xE6, 0x25, 0xED, 0x54,  \
		0x9B, 0xC3, 0x6A, 0xA5, 0x98, 0x64, 0xAC, 0x10, 0xD9, 0x8F, 0xE7, 0x30, 0xA9, 0x54, 0xE4,  \
		0x5C, 0xD4, 0x35, 0xA4, 0x35, 0xFF, 0x26, 0x4F, 0x87, 0xEA, 0x65, 0x3B, 0xDE, 0x48, 0x76,  \
		0x1E, 0xB8, 0x72, 0xC5, 0xD8, 0x5E, 0x8A, 0x24

#define KEYFILE "Keyfile"

#define REG_NONE 0
#define REG_CD 1
#define REG_ESD 2
#define REG_HAS_14 14

int
main(int argc, char* argv[])
{
	char keyString[256];
	char nameString[128];
	struct tm* tms;
	time_t tm;
	int i, vlen;
	unsigned long sum;
	FILE* kf;
	char wbuffer[256];
	char* schar;
	char Version[] = "2.0";
	char xAlphaMask[256] = {KEYFILE_MASK};
	char filler[] = "Well done, you have cracked the keyfile code!  Please don't pass it on, I "
					"need to eat too - Sander";

	if (argc != 2) {
		printf("Generate Keyfile for Becasso 1.5 registration\n");
		printf("Usage: gen_key <name>\n");
		printf("(Quote <name>; it will probably contain spaces)\n");
		return 2;
	}

	strncpy(nameString, argv[1], 127);

	for (i = 0; i <= strlen(nameString); i++)
		keyString[2 * i] = nameString[i];
	for (i = strlen(nameString) + 1; i < 128; i++)
		keyString[2 * i] = rand() & 0xFF;
	tm = time(NULL);
	tms = localtime(&tm);

	vlen = strlen(Version);
	keyString[1] = vlen;
	keyString[3] = tms->tm_year;
	keyString[5] = tms->tm_mon;
	keyString[7] = tms->tm_mday;
	keyString[9] = 31;
	keyString[11] = 42;
	keyString[13] = REG_ESD;
	for (i = 0; i <= vlen; i++)
		keyString[2 * i + 15] = Version[i];
	for (i = 0; i < strlen(filler); i++)
		keyString[2 * i + 2 * vlen + 15] = filler[i];
	sum = 0;
	for (i = 0; i < 252; i++)
		sum += keyString[i];
	schar = (char*)(&sum);
	keyString[252] = schar[0];
	keyString[253] = schar[1];
	keyString[254] = schar[2];
	keyString[255] = schar[3];
	for (i = 0; i < 255; i++)
		wbuffer[i] = keyString[i] ^ xAlphaMask[i];

#ifdef WRITE_TO_FILE
	kf = fopen(KEYFILE, "wb");
#else
	kf = stdout;
#endif
	if (kf)
		fwrite(wbuffer, 1, 255, kf);
	else {
		fprintf(stderr, "Couldn't open %s for writing\n", KEYFILE);
		return 1;
	}
#ifdef WRITE_TO_FILE
	fclose(kf);
#endif

	return 0;
}