#ifndef HSV_H
#define HSV_H

#include "AddOnSupport.h"	// For hsv_color

rgb_color hsv2rgb (hsv_color c);
hsv_color rgb2hsv (rgb_color c);
float diff (rgb_color a, rgb_color b);
uchar clipchar (float x);
uchar clipchar (int x);
float clipone (float x);
float clipdegr (float x);

#endif 