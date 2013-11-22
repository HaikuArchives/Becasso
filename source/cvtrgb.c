#include <stdio.h>
#define max(a,b) ((a)>(b)?(a):(b))

// Format of rgb.txt:
// r g b<tab(s)>Name\n
// 0 <= r,g,b <= 255

int main (void)
{
	char rgbin[753][60];
	unsigned char rgbout[753][60];
	int i, j, maxlen;
	FILE *in, *out;

	in = fopen ("rgb.txt", "r");

	i = 0;
	maxlen = 0;
	while (!feof (in))
	{
		char *p;
		fgets (rgbin[i], 59, in);

		sscanf (rgbin[i], "%d %d %d", 
			&rgbout[i][0], &rgbout[i][1], &rgbout[i][2]);

		p = rgbin[i];
		while (*p++ != '\t')
			;
		while (*++p == '\t')
			;
		strcpy (&rgbout[i][3], p); // -> Name
		rgbout[i][strlen (&rgbout[i][3]) + 2] = 0;
		maxlen = max (strlen (&rgbout[i][3]), maxlen);
		i++;
	}
	printf ("Max length = %d\n", maxlen);
	fclose (in);
	
	out = fopen ("colors.dat", "wb");
	
	printf ("%d items\n", --i);
	fprintf (out, "%c%c%c", maxlen, (char) (i % 256), (char) (i/256));
	for (j = 0; j < i; j++)
	{
		rgbout[j][maxlen + 3] = 0;
		printf ("%3d %3d %3d %s\n", 
		    (int) rgbout[j][0], (int) rgbout[j][1], (int) rgbout[j][2],
			&rgbout[j][3]);
		fprintf (out, "%c%c%c%s%c", 
		    (int) rgbout[j][0], (int) rgbout[j][1], (int) rgbout[j][2],
			&rgbout[j][3], 0);
	}
	fclose (out);
	return 0;
}
