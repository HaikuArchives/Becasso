/*	be-sfx.h	*/

/*	Be sound effects system by Jon Watte <be-sfx@mindcontrol.org>	*/
/*	Copyright 1997-8 Jon Watte	*/

#if !defined(BE_SFX_H)
#define BE_SFX_H

/* #pragma once */

/*	Usage is simple:	*/

/*	1.	Create one EffectsPlayer object in your app. It could be a global variable.	*/

/*	2.	For each individual sound effect you may want to play, create one SoundEffect	*/
/*		object (or subclass) and initialize it to hold the data of the sound effect.	*/
/*		If the data you read is of a different byte order than the host byte order,	*/
/*		pass "true" for swap_data, else pass "false". 8-bit data needs no swapping.	*/

/*	3.	When the time comes to play the effect, call StartEffect() with the			*/
/*		SoundEffect object on the EffectsPlayer instance you created at startup.		*/

/*	4.	It is safe to call StartEffect() with a sound that is already playing; doing	*/
/*		so will start another copy of that same sound.									*/

/*	5.	If there is crowding (more sounds are being started than the limit specified	*/
/*		when you created the EffectsPlayer) the sound with the least time left to		*/
/*		play will be stopped, and the new sound started (even if the new sound is		*/
/*		shorter than what the evicted sound had left to play) This is generally a		*/
/*		good enough priority scheme.													*/

/*	6.	Don't delete a SoundEffect until you know that the sound embodied by that		*/
/*		object is no longer playing.													*/

/*	7.	Delete the EffectsPlayer before you quit your application, or the audio		*/
/*		server may get upset. It is so touchy at times.								*/

/*	8.	Call SetVolume(0) to temporarily turn off sound. When no sound is playing,	*/
/*		or the global volume is 0, (almost) no cycles are wasted by this class.		*/

/*	9.	Avoid creating SoundEffect instances at runtime, since doing so could be		*/
/*		slow. It's OK to create and delete them, say, when changing levels in a game.	*/

/*	10.	Create a new subclass of SoundEffect per audio format you need to be able to	*/
/*		read into your program. The buffer SoundEffect provides should be 44 kHz,		*/
/*		16 bit signed mono audio.														*/

/*	11.	BackgroundVolume and Volume (for effects) go between 0 (off) and 15 (full)	*/

/*	12.	volume goes between 0 and 127 for started effects. pan goes between -127		*/
/*		(left) and 127 (right) when passed to StartEffect().							*/

/*	13.	For sounds you want to loop a fixed number of times, pass that number into	*/
/*		the SoundEffect constructor. Use kDontLoop for sounds that play only once, 	*/
/*		and use kLoopForever for sounds that should keep playing forever.				*/
/*		Setting "loop" to 1 means it loops back one time, playing the sound twice.	*/

/*	14.	When sounds are removed from the active play list, and when they are looped	*/
/*		back to the beginning, the SoundCompleted() callback is called. You can use	*/
/*		this callback in an override to do seamless chaining of sound effects if you	*/
/*		want to.																		*/

/*	15.	A cool future improvement would be to allow starting sound effects at various	*/
/*		times in the future; just have a queue of sound effects waiting to start.		*/

#include <AudioStream.h>
#include <Locker.h>
#include <Subscriber.h>
#include <stdlib.h>

enum {
	kLoopForever = -1,
	kDontLoop = 0
};

class SoundEffect {
public:
	/* set swap_data to true if data is in different byte order than host */
	SoundEffect(bool swap_data, short* pointer, int numFrames, int nLoops = kDontLoop);
	~SoundEffect();

	short* GetBuffer(int& numFrames, int& numLoops);

protected:
	short* fPointer;
	int fFrames;
	int fLoops;
};

class SoundEffect8_11 : public SoundEffect {
protected:
	/*	set toggle to true if sound is in unsigned char format		*/
	/*	this only does linear expansion - mulaw requires another subclass	*/
	/*	No swapping necessary for 8-bit samples...	*/
	static short* Convert8_11(char* buffer, int size, int loop = kDontLoop, bool toggle = false);

public:
	SoundEffect8_11(char* buffer, int size, int loops = kDontLoop, bool toggle = false);
};

typedef int effect_id;

enum {			   /*	reasons for SoundCompleted		*/
	kSoundDone,	   /*	sound played the last sample		*/
	kSoundLooping, /*	sound will loop to first sample	*/
	kSoundEvicted, /*	sound was evicted by newer sound	*/
	kSoundStopped  /*	sound was manually stopped			*/
};

class EffectsPlayer : public BSubscriber {
public:
	EffectsPlayer(int backgroundVolume = 0, int nChannels = 8);
	~EffectsPlayer();

	status_t InitCheck() { return fInit; }

	effect_id StartEffect(/*	don't delete the effect until the player is gone	*/
		SoundEffect* effect, int volume = 127, int pan = 0);
	bool IsPlaying(effect_id effect);
	void StopEffect(effect_id effect);
	virtual void SoundCompleted(effect_id effect, int reason);

	void SetEffectVolPan(/*	-1 means no change	*/
		effect_id effect, int vol = -1, int pan = -1);

	void SetVolume(int volume); /*	0->15	*/
	int GetVolume();
	void SetBackgroundVolume(int bgVolume);
	int GetBackgroundVolume();
	void Mix(short* buffer, int numFrames);

	static bool PlayHook(void* userData, char* buffer, size_t count, void* /* header */);

protected:
	status_t fInit;
	BDACStream* fOutput;
	int fVolume;
	int fBGVolume;
	effect_id fNextEffect;

	struct PlayingEffect {
		short* data;
		int frames;
		int offset;
		int lvol;
		int rvol;
		int loop;
		effect_id id;
	};

	PlayingEffect* fPlayList;
	int nPlaying;
	int fPlaySize;
	BLocker fLock;
};

#endif /* BE_SFX_H	*/
