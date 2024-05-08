#if defined(EASTER_EGG_SFX)
#include "sfx.h"
#include <MediaDefs.h>
#include <stdio.h>

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

EffectsPlayer::EffectsPlayer(SoundEffect8_11* fx)
{
	media_raw_audio_format format;
	format.format = media_raw_audio_format::B_AUDIO_FLOAT;
	format.frame_rate = 11000;
	format.channel_count = 2;
#if defined(__INTEL__)
	format.byte_order = 2;
#else
	format.byte_order = 1;
#endif
	format.buffer_size = 10802;
	cookie = new cookie_record;
	cookie->pos = 0;
	cookie->size = fx->size();
	cookie->buf = fx->data();
	cookie->parent = this;

	myPlayer = new BSoundPlayer(&format, "Easter Egg", BufferProc, NULL, cookie);

	//	printf ("bufsize: %ld\n", format.buffer_size);

	//	myPlayer = new BSoundPlayer (&format, "Easter Egg");
	myPlayer->Start();
	myPlayer->SetVolume(0.5);
	myPlayer->SetHasData(false);
}

EffectsPlayer::~EffectsPlayer()
{
	//	printf ("~EffectsPlayer\n");
	if (myPlayer)
		myPlayer->Stop(false);
	//	printf ("Stopped BSoundPlayer...\n");
	delete myPlayer;
	delete cookie;
	//	printf ("Deleted BSoundPlayer...\n");
}

void
EffectsPlayer::StartEffect()
{
	//	printf ("Starting sound effect...\n");
	/*	media_raw_audio_format format = effect->sound()->Format();
		printf ("format = %ld\n", format.format);
		if (format.format != media_raw_audio_format::B_AUDIO_FLOAT)
			printf ("HEY!!\n");
		myPlayer->StartPlaying (effect->sound());
	*/
	// myPlayer->Start();
	myPlayer->SetHasData(true);
}

void
BufferProc(void* theCookie, void* buffer, size_t size, const media_raw_audio_format& format)
{
	//	printf ("BufferProc (%p, %ld)\n", buffer, size);
	size_t i, j;
	float* buf = (float*)buffer;
	uint32 channel_count = format.channel_count;
	size_t num_frames = size / (4 * channel_count);
	cookie_record* cookie = (cookie_record*)theCookie;

	if (format.format != media_raw_audio_format::B_AUDIO_FLOAT) {
		printf("Hey!!\n");
		return;
	}

	int32 pos = cookie->pos;
	num_frames = MIN(num_frames, (cookie->size - pos) / channel_count);
	//	printf ("%p, %ld, %ld\n", cookie->buf, cookie->size, float_size);
	if (num_frames) {
		for (i = 0; i < num_frames * channel_count; i += channel_count) {
			for (j = 0; j < channel_count; j++) {
				//	    		printf ("%f\n", buf[i + j]);
				buf[i + j] = cookie->buf[pos + i + j];
			}
		}
		//		for (i = num_frames - 1; i < size/4/channel_count; i += channel_count)
		//		{
		//			for (j = 0; j < channel_count; j++)
		//				buf[i + j] = 0.0;
		//		}
		cookie->pos += channel_count * num_frames;
	} else {
		//		printf ("Done.\n");
		cookie->parent->player()->SetHasData(false);
		// cookie->parent->player()->Stop();
		cookie->pos = 0;
	}
}

//===========

SoundEffect8_11::SoundEffect8_11(void* data, size_t size)
{
	myData = new float[2 * size];
	unsigned char* cd = (unsigned char*)data;
	for (int i = 0; i < size; i++) {
		// pan from left to right
		// (some day that is - it seems to be broken using (1 - pan)...)
		float val = (float(cd[i]) - 127.0) / 300.0;
		float pan = float(i) / size;
		myData[2 * i] = val * (pan);  //?
		myData[2 * i + 1] = val * (pan);
	}
	fSize = size;  // in frames, not bytes!!
}

SoundEffect8_11::~SoundEffect8_11()
{
	//	printf ("~SE\n");
	delete myData;
}

#endif
