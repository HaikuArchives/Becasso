// This code is (c) 1998 Sum Software!
// Used with permission (of myself) for the Xmlf program (ONLY!).

#include "picture.h"
#include <stdlib.h>
#include <stdio.h>

#define TOL (1E-6)
#define OMTOL (1 - TOL)

long gcd (long p, long q)
{
    long r;
    while (r = p % q, r)
    {
        p = q;
        q = r;
    }
    return q;
}

bool scale_p (picture *src, picture *dest)
{
    double xscale, yscale;
    float *mtmp;
    int x, y, nb, nbx, nby, msx, msy, mdx, mdy;

    xscale = (double) dest->x/src->x;
    yscale = (double) dest->y/src->y;
    nbx = gcd (dest->x, src->x);
    nby = gcd (dest->y, src->y);
    mdx = dest->x/nbx;
    mdy = dest->y/nby;
    msx = src->x/nbx;
    msy = src->y/nby;

    printf ("src: [%d, %d]; dest: [%d, %d]; xscale = %f, yscale = %f\n",
            src->x, src->y, dest->x, dest->y, xscale, yscale);
    mtmp = (float *) malloc (dest->x * src->y * sizeof (float));
    // mtmp contains the horizontally scaled image
    printf ("xscale = %f\n", xscale);
    if (xscale < 1)        // Shrink horizontally
    {
        float *srcdata  = src->data;
        float *destdata = mtmp;
        printf ("Shrink horizontally.\n");
        printf ("nbx = %i, msx = %i, mdx = %i\n", nbx, msx, mdx);
        for (y = 0; y < src->y; y++)
        {
            float *srcline = srcdata + y*src->x;
            float *destline = destdata + y*dest->x;
            for (nb = 0; nb < nbx; nb++)
            {
                int s = 1;
                int d = 1;
                bool stillinblock = TRUE;
                float alpha, beta;
                float r = 0;
                while (stillinblock)
                {
                    while (s*mdx < d*msx)
                    {
                        r += *(srcline++)*mdx/msx;
                        s++;
                    }
                    if (s*mdx != d*msx)
                    {
                        beta  = (float) s*mdx/msx - d;
                        alpha = (float) mdx/msx - beta;
                        r += alpha* *(srcline);
                        *(destline++) = r;
                        d++;
                        r = beta* *(srcline++);
                        s++;
                    }
                    else
                    {
                        r += *(srcline++)*mdx/msx;
                        *(destline++) = r;
                        stillinblock = FALSE;
                    }
                }
            }
        }
    }
    else if (xscale > 1)    // Stretch horizontally
    {
        float *srcdata = src->data;
        float *destdata = mtmp;
        printf ("Stretch horizontally.\n");
        printf ("nbx = %i, msx = %i, mdx = %i\n", nbx, msx, mdx);
        for (y = 0; y < src->y; y++)
        {
            float *destline = destdata + y*dest->x;
            float *srcline = srcdata + y*src->x;
            for (nb = 0; nb < nbx; nb++)
            {
                int s = 1;
                int d = 1;
                bool stillinblock = TRUE;
                float alpha, beta;
                while (stillinblock)
                {
                    while (d*msx < s*mdx)
                    {
                        *(destline++) = *srcline;
                        d++;
                    }
                    if (d*msx != s*mdx)
                    {
                        alpha = (float) d*msx/mdx - s;
                        beta  = 1 - alpha;
                        *(destline++) = alpha*(*srcline) + beta*(*(srcline+1));
                        srcline++;
                        s++;
                        d++;
                    }
                    else
                    {
                        *(destline++) = *(srcline++);
                        stillinblock = FALSE;
                    }
                }
            }
        }
    }
    else    // No scaling in x-dimension -> copy to temp bitmap.
    {
        printf ("No scaling in x-dimension -> copy to temp bitmap.\n");
        memcpy (mtmp, src->data, src->x*src->y*sizeof (float));
    }
    printf ("yscale = %f\n", yscale);
    if (yscale < 1)        // Shrink vertically
    {
        float *srcdata = mtmp;
        float *destdata = dest->data;
        int line = dest->x;
        printf ("Shrink vertically\n");
        printf ("nby = %i, msy = %i, mdy = %i\n", nby, msy, mdy);
        for (x = 0; x < dest->x; x++)
        {
            float *destcol = destdata + x;
            float *srccol  = srcdata + x;
            for (nb = 0; nb< nby; nb++)
            {
                int s = 1;
                int d = 1;
                bool stillinblock = TRUE;
                float alpha, beta;
                float r = 0;
                while (stillinblock)
                {
                    while (s*mdy < d*msy)
                    {
                        r += *srccol * mdy/msy;
                        srccol += line;
                        s++;
                    }
                    if (s*mdy != d*msy)
                    {
                        beta  = (float) s*mdy/msy - d;
                        alpha = (float) mdy/msy - beta;
                        r += alpha* *srccol;
                        *destcol = r;
                        destcol += line;
                        d++;
                        r = beta* *srccol;
                        srccol += line;
                        s++;
                    }
                    else
                    {
                        r += *srccol * mdy/msy;
                        srccol += line;
                        *destcol = r;
                        destcol += line;
                        stillinblock = FALSE;
                    }
                }
            }
        }
    }
    else if (yscale > 1)    // Stretch vertically
    {
        float *srcdata  = mtmp;
        float *destdata = dest->data;
        int line = dest->x;
        printf ("Stretch vertically\n");
        printf ("nby = %i, msy = %i, mdy = %i\n", nby, msy, mdy);
        for (x = 0; x < dest->x; x++)
        {
            float *destcol = destdata + x;
            float *srccol  = srcdata + x;
            for (nb = 0; nb < nby; nb++)
            {
                int s = 1;
                int d = 1;
                bool stillinblock = FALSE;
                float alpha, beta;
                while (stillinblock)
                {
                    while (d*msy < s*mdy)
                    {
                        *destcol = *srccol;
                        destcol += line;
                        d++;
                    }
                    if (d*msy != s*mdy)
                    {
                        alpha = (float) d*msy/mdy - s;
                        beta  = 1 - alpha;
                        *destcol = alpha* *srccol + beta* *(srccol + line);
                        destcol += line;
                        d++;
                        srccol += line;
                        s++;
                    }
                    else
                    {
                        *destcol = *srccol;
                        destcol += line;
                        srccol  += line;
                        stillinblock = FALSE;
                    }
                }
            }
        }
    }
    else    // No scaling in y-dimension -> Just copy the bitmap to dest.
    {
        printf ("No scaling in y-dimension -> Just copy the bitmap to dest.\n");
        memcpy (dest->data, mtmp, dest->x*dest->y*sizeof (float));
    }
    printf ("Done.\n");
    free (mtmp); printf ("Freed.\n");
    return 0; 
}
