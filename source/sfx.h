#ifndef _SFX_H
#define _SFX_H

#include <SoundPlayer.h>
#include <Sound.h>

void
BufferProc(void* theCookie, void* buffer, size_t size, const media_raw_audio_format& format);

class SoundEffect8_11
{
  public:
	SoundEffect8_11(void* data, size_t size);
	~SoundEffect8_11();

	BSound* sound() { return mySound; };

	float* data() { return myData; };

	int size() { return fSize; };

  private:
	BSound* mySound;
	float* myData;
	int fSize;
};

class EffectsPlayer;

typedef struct cookie_record
{
	int32 pos;
	int32 size;
	float* buf;
	EffectsPlayer* parent;
} cookie_record;

class EffectsPlayer
{
  public:
	EffectsPlayer(SoundEffect8_11* effect);
	virtual ~EffectsPlayer();
	void StartEffect();

	BSoundPlayer* player() { return myPlayer; };

  private:
	BSoundPlayer* myPlayer;
	BSound* myEffect;
	cookie_record* cookie;
};

#endif
