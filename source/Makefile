# Makefile for Becasso

CC = gcc -c
AS = nasm -f elf 
SLD = gcc -lbe -Xlinker -x -nostart -Xlinker -soname=

ADDONDIR = add-ons/

CFLAGS = -Wall -Wno-multichar -Wno-ctor-dtor-privacy -Wbad-function-cast\
	-Wconversion  -Wextern-inline  -Wwrite-strings -Woverloaded-virtual -O3\
	-ffast-math
	-march=pentiumpro -mcpu=pentiumpro


addons: $(ADDONDIR)Blur $(ADDONDIR)CaptureTest $(ADDONDIR)Diffuse \
	$(ADDONDIR)Gaussian $(ADDONDIR)Gradient $(ADDONDIR)Mandelbrot \
	$(ADDONDIR)MotionBlur $(ADDONDIR)Noise $(ADDONDIR)OilPaint \
	$(ADDONDIR)Ripple $(ADDONDIR)Scale $(ADDONDIR)Tile $(ADDONDIR)Wave \
	$(ADDONDIR)Quantize $(ADDONDIR)Palmcam $(ADDONDIR)ColorCurves \
	$(ADDONDIR)Negate $(ADDONDIR)Solarize $(ADDONDIR)DuoTone \
	$(ADDONDIR)BumpMap $(ADDONDIR)WinGrab $(ADDONDIR)VideoGrabber \
	$(ADDONDIR)Brightness $(ADDONDIR)AutoContrast

Blur.o: Blur.cpp
	$(CC) $(CFLAGS) Blur.cpp
$(ADDONDIR)Blur: Blur.o
	$(SLD)Blur -o Blur Blur.o _APP_
	strip Blur
	mv Blur add-ons

CaptureTest.o: CaptureTest.cpp
	$(CC) $(CFLAGS) CaptureTest.cpp
$(ADDONDIR)CaptureTest: CaptureTest.o
	$(SLD)CaptureTest -o CaptureTest CaptureTest.o _APP_
	strip CaptureTest
	mv CaptureTest add-ons

Diffuse.o: Diffuse.cpp
	$(CC) $(CFLAGS) Diffuse.cpp
$(ADDONDIR)Diffuse: Diffuse.o
	$(SLD)Diffuse -o Diffuse Diffuse.o _APP_
	strip Diffuse
	mv Diffuse add-ons

Gaussian.o: Gaussian.cpp
	$(CC) $(CFLAGS) Gaussian.cpp
$(ADDONDIR)Gaussian: Gaussian.o
	$(SLD)Gaussian -o Gaussian Gaussian.o _APP_
	strip Gaussian
	mv Gaussian add-ons

Gradient.o: Gradient.cpp
	$(CC) $(CFLAGS) Gradient.cpp
$(ADDONDIR)Gradient: Gradient.o
	$(SLD)Gradient -o Gradient Gradient.o _APP_
	strip Gradient
	mv Gradient add-ons

mandelbrot.o: mandelbrot.cpp
	$(CC) $(CFLAGS) mandelbrot.cpp
$(ADDONDIR)Mandelbrot: mandelbrot.o
	$(SLD)Mandelbrot -o Mandelbrot mandelbrot.o _APP_
	strip Mandelbrot
	mv Mandelbrot add-ons

MotionBlur.o: MotionBlur.cpp
	$(CC) $(CFLAGS) MotionBlur.cpp
$(ADDONDIR)MotionBlur: MotionBlur.o
	$(SLD)MotionBlur -o MotionBlur MotionBlur.o _APP_
	strip MotionBlur
	mv MotionBlur add-ons

Noise.o: Noise.cpp
	$(CC) $(CFLAGS) Noise.cpp
$(ADDONDIR)Noise: Noise.o
	$(SLD)Noise -o Noise Noise.o _APP_
	strip Noise
	mv Noise add-ons

OilPaint.o: OilPaint.cpp
	$(CC) $(CFLAGS) OilPaint.cpp
$(ADDONDIR)OilPaint: OilPaint.o
	$(SLD)OilPaint -o OilPaint OilPaint.o _APP_
	strip OilPaint
	mv OilPaint add-ons

Ripple.o: Ripple.cpp
	$(CC) $(CFLAGS) Ripple.cpp
$(ADDONDIR)Ripple: Ripple.o
	$(SLD)Ripple -o Ripple Ripple.o _APP_
	strip Ripple
	mv Ripple add-ons

Scale.o: Scale.cpp
	$(CC) $(CFLAGS) Scale.cpp
$(ADDONDIR)Scale: Scale.o
	$(SLD)Scale -o Scale Scale.o _APP_
	strip Scale
	mv Scale add-ons

Tile.o: Tile.cpp
	$(CC) $(CFLAGS) Tile.cpp
$(ADDONDIR)Tile: Tile.o
	$(SLD)Tile -o Tile Tile.o _APP_
	strip Tile
	mv Tile add-ons

Wave.o: Wave.cpp
	$(CC) $(CFLAGS) Wave.cpp
$(ADDONDIR)Wave: Wave.o
	$(SLD)Wave -o Wave Wave.o _APP_
	strip Wave
	mv Wave add-ons

Quantize.o: Quantize.cpp
	$(CC) $(CFLAGS) Quantize.cpp
$(ADDONDIR)Quantize: Quantize.o
	$(SLD)Quantize -o Quantize Quantize.o _APP_
	strip Quantize
	mv Quantize add-ons

Palmcam.o: Palmcam.cpp
	$(CC) $(CFLAGS) Palmcam.cpp
dsc.o: dsc.c
	$(CC) $(CFLAGS) dsc.c
dc1000.o: dc1000.c
	$(CC) $(CFLAGS) dc1000.c
$(ADDONDIR)Palmcam: Palmcam.o dsc.o dc1000.o
	$(SLD)Palmcam -o Palmcam Palmcam.o dsc.o dc1000.o -ldevice -ltranslation _APP_
	strip Palmcam
	mv Palmcam add-ons

ColorCurves.o: ColorCurves.cpp
	$(CC) $(CFLAGS) ColorCurves.cpp
Colorcurves_x86.o: Colorcurves_x86.asm
	$(AS) Colorcurves_x86.asm
$(ADDONDIR)ColorCurves: ColorCurves.o Colorcurves_x86.o
	$(SLD)ColorCurves -o ColorCurves ColorCurves.o Colorcurves_x86.o _APP_
	strip ColorCurves
	mv ColorCurves add-ons

Negate.o: Negate.cpp
	$(CC) $(CFLAGS) Negate.cpp
$(ADDONDIR)Negate: Negate.o
	$(SLD)Negate -o Negate Negate.o _APP_
	strip Negate
	mv Negate add-ons

Solarize.o: Solarize.cpp
	$(CC) $(CFLAGS) Solarize.cpp
$(ADDONDIR)Solarize: Solarize.o
	$(SLD)Solarize -o Solarize Solarize.o _APP_
	strip Solarize
	mv Solarize add-ons

Sepia.o: Sepia.cpp
	$(CC) $(CFLAGS) Sepia.cpp
$(ADDONDIR)DuoTone: Sepia.o
	$(SLD)DuoTone -o DuoTone Sepia.o _APP_
	strip DuoTone
	mv DuoTone add-ons

MultiGradient.o: MultiGradient.cpp
	$(CC) $(CFLAGS) MultiGradient.cpp
$(ADDONDIR)MultiGradient: MultiGradient.o
	$(SLD)MultiGradient -o MultiGradient MultiGradient.o _APP_
	strip MultiGradient
	mv MultiGradient add-ons

BumpMap.o: BumpMap.cpp
	$(CC) $(CFLAGS) BumpMap.cpp
$(ADDONDIR)BumpMap: BumpMap.o
	$(SLD)BumpMap -o BumpMap BumpMap.o _APP_
	strip BumpMap
	mv BumpMap add-ons

WinGrab.o: WinGrab.cpp
	$(CC) $(CFLAGS) WinGrab.cpp
$(ADDONDIR)WinGrab: WinGrab.o
	$(SLD)WinGrab -o WinGrab WinGrab.o _APP_
	strip WinGrab
	mv WinGrab add-ons

VideoGrabber.o: VideoGrabber.cpp
	$(CC) $(CFLAGS) VideoGrabber.cpp
VideoConsumer.o: VideoConsumer.cpp
	$(CC) $(CFLAGS) VideoConsumer.cpp
$(ADDONDIR)VideoGrabber: VideoGrabber.o VideoConsumer.o
	$(SLD)VideoGrabber -lmedia -ltranslation -o VideoGrabber VideoGrabber.o VideoConsumer.o _APP_
	strip VideoGrabber
	mv VideoGrabber add-ons

Brightness.o: Brightness.cpp
	$(CC) $(CFLAGS) Brightness.cpp
$(ADDONDIR)Brightness: Brightness.o
	$(SLD)Brightness -o Brightness Brightness.o _APP_
	strip Brightness
	mv Brightness add-ons

AutoContrast.o: AutoContrast.cpp
	$(CC) $(CFLAGS) AutoContrast.cpp
$(ADDONDIR)AutoContrast: AutoContrast.o
	$(SLD)AutoContrast -o AutoContrast AutoContrast.o _APP_
	strip AutoContrast
	mv AutoContrast add-ons
