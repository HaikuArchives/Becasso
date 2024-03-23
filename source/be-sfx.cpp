
/*	Be sound effects system by Jon Watte <besfx@mindcontrol.org>	*/
/*	Copyright © 1997 Jon Watte	*/

#include <string.h>
#include <alloca.h>
#include <ByteOrder.h>

#include "be-sfx.h"

#if !defined(DEBUG)
/*	#define DEBUG 1		/*	debugging output	*/
#define DEBUG 0 /*	no debugging (faster)	*/
#endif

SoundEffect::SoundEffect(bool swap_data, short* pointer, int numFrames, int nLoops)
{
	fPointer = pointer;
	fFrames = numFrames;
	fLoops = nLoops;
	if (swap_data) {
		while (numFrames-- > 0) {
			*pointer = B_SWAP_INT16(*pointer);
			pointer++;
		}
	}
}

SoundEffect::~SoundEffect()
{
	if (fPointer != NULL) /*	superclasses may do different things	*/
		free(fPointer);
}

short*
SoundEffect::GetBuffer(int& numFrames, int& numLoops)
{
	numFrames = fFrames;
	numLoops = fLoops;
	return fPointer;
}

short*
SoundEffect8_11::Convert8_11(char* buffer, int size, int loop, bool toggle)
{
	int rsize = size * 8; /*	11->44, 8->16	*/
	short* ret = (short*)malloc(rsize);
	short prev;
	short* out = ret;

	/*	if you are playing sounds in a loop that should not glitch,			*/
	/*	you should use this code to make sure the last and first samples butt	*/
	if (loop != 0)
		if (toggle)
			prev = buffer[size - 1] - 128;
		else
			prev = buffer[size - 1];
	else
		prev = 0;
	for (int ix = 0; ix < size; ix++) {
		short now;
		if (toggle)
			now = buffer[ix] - 128;
		else
			now = buffer[ix];
		out[0] = (prev * 3 + now) * 64;
		out[1] = (prev * 2 + now * 2) * 64;
		out[2] = (prev + now * 3) * 64;
		prev = now;
		out[3] = prev * 256;
		out += 4;
	}
	return ret;
}

SoundEffect8_11::SoundEffect8_11(char* buffer, int size, int loop, bool toggle)
	: SoundEffect(false, Convert8_11(buffer, size, loop, toggle), size * 4, loop) /* 11->44 */
{
	if (buffer != NULL)
		free(buffer
		); /*	since we eat an already allocated malloc() block (say, from FindResource())	*/
}

EffectsPlayer::EffectsPlayer(int bgVolume, int nChannels)
	: BSubscriber("be-sfx by h+"), fLock("sfx locker")
{
	/*	set up the area used for remembering what's being played	*/
	fPlayList = new PlayingEffect[nChannels];
	fPlaySize = nChannels;
	nPlaying = 0;
	fVolume = 15;
	fBGVolume = bgVolume;
	/*	make ourselves part of the audio stream	*/
	fOutput = new BDACStream();
	fInit = Subscribe(fOutput);
	if (!fInit)
		fInit = fOutput->SetSamplingRate(44100.0);
	if (!fInit)
		fInit = fOutput->SetStreamBuffers(1024, 3); /*	minimize latency	*/
	if (!fInit)
		fInit = EnterStream(0, false, this, PlayHook, NULL, true);
}

EffectsPlayer::~EffectsPlayer()
{
	if (!fInit)
		ExitStream(true);
	Unsubscribe();
	delete[] fPlayList;
	delete fOutput;
}

effect_id
EffectsPlayer::StartEffect(SoundEffect* effect, int volume, int pan)
{
	/*	test for init conditions	*/
	if (fInit)
		return B_ERROR;
	if (volume < 1)
		return -1;
	fLock.Lock(); /*	keep locked for atomic update	*/
	if (nPlaying >= fPlaySize) {
		int found = 0;
		int toplay = fPlayList[0].frames - fPlayList[0].offset;
		/*	find someone to kick out	*/
		/*	we kick out the sound with the least amount left to play	*/
		for (int ix = 1; ix < fPlaySize; ix++) {
			int nplay = fPlayList[ix].frames - fPlayList[ix].offset;
			if (nplay < toplay) {
				found = ix;
				toplay = nplay;
			}
		}
		effect_id evicted = fPlayList[found].id;
		memcpy(
			&fPlayList[found], &fPlayList[found + 1],
			sizeof(PlayingEffect) * (fPlaySize - found - 1)
		);
		nPlaying = fPlaySize - 1;
		SoundCompleted(evicted, kSoundEvicted);
	}
	/*	finally, we know where we live - set up struct to remember us	*/
	fPlayList[nPlaying].data =
		effect->GetBuffer(fPlayList[nPlaying].frames, fPlayList[nPlaying].loop);
	fPlayList[nPlaying].offset = 0;
	fPlayList[nPlaying].lvol = volume * ((pan > 0) ? (127.0 - pan) / 127.0 : 1.0);
	fPlayList[nPlaying].rvol = volume * ((pan < 0) ? (127.0 + pan) / 127.0 : 1.0);
	effect_id id = ++fNextEffect;
	fPlayList[nPlaying].id = id;
	nPlaying++;
	fLock.Unlock();
	return id;
}

bool
EffectsPlayer::IsPlaying(effect_id effect)
{
	/*	try to find given sound instance	*/
	bool ret = false;
	fLock.Lock();
	for (int ix = 0; ix < nPlaying; ix++)
		if (fPlayList[ix].id == effect)
			ret = true;
	fLock.Unlock();
	return ret;
}

void
EffectsPlayer::StopEffect(effect_id effect)
{
	/*	try to find given sound instance	*/
	fLock.Lock();
	for (int ix = 0; ix < nPlaying; ix++)
		if (fPlayList[ix].id == effect) {
			/*	kick it out	*/
			memcpy(&fPlayList[ix], &fPlayList[ix + 1], sizeof(PlayingEffect) * (nPlaying - ix - 1));
			nPlaying--;
			SoundCompleted(effect, kSoundStopped);
			break;
		}
	fLock.Unlock();
}

void
EffectsPlayer::SetEffectVolPan(/*	-1 means no change	*/
							   effect_id effect, int vol, int pan
)
{
	if (effect < 0)
		return;
	if ((vol < 0) && (pan < 0))
		return;
	/*	try to find given sound instance	*/
	fLock.Lock();
	for (int ix = 0; ix < nPlaying; ix++)
		if (fPlayList[ix].id == effect) {
			/*	we can recover volume and pan from left and right volume	*/
			/*	because of the linear relation between them. The largest	*/
			/*	is always whatever the volume is.							*/
			if (vol < 0)
				vol =
					((fPlayList[ix].rvol > fPlayList[ix].lvol) ? fPlayList[ix].rvol
															   : fPlayList[ix].lvol);
			if (pan < 0)
				pan =
					((fPlayList[ix].rvol >= fPlayList[ix].lvol)
						 ? (127 - 127 * fPlayList[ix].lvol / fPlayList[ix].rvol)
						 : (127 * fPlayList[ix].rvol / fPlayList[ix].lvol - 127));
			fPlayList[nPlaying].lvol = vol * ((pan > 0) ? (127.0 - pan) / 127.0 : 1.0);
			fPlayList[nPlaying].rvol = vol * ((pan < 0) ? (127.0 + pan) / 127.0 : 1.0);
			break;
		}
	fLock.Unlock();
}

void
EffectsPlayer::SoundCompleted(effect_id /* effect */, int /* reason */)
{
	/*	do nothing	*/
}

void
EffectsPlayer::SetVolume(int volume) /*	0->15	*/
{
	fVolume = volume;
	if (fVolume > 15)
		fVolume = 15;
	if (fVolume < 0)
		fVolume = 0;
}

int
EffectsPlayer::GetVolume()
{
	return fVolume;
}

void
EffectsPlayer::SetBackgroundVolume(int bgVolume)
{
	fBGVolume = bgVolume;
	if (fBGVolume > 15)
		fBGVolume = 15;
	if (fBGVolume < 0)
		fBGVolume = 0;
}

int
EffectsPlayer::GetBackgroundVolume()
{
	return fBGVolume;
}

void
EffectsPlayer::Mix(short* buffer, int numFrames)
{
	if (fVolume < 1)
		return;	  /*	don't affect when sound is off	*/
	fLock.Lock(); /*	we keep it locked during the entire mix, since we are list heavy	*/
				  /*	A significant amount of cycles are expended to traverse the list when many	*/
	/*	items are being played. However, this function is only called about 100 times a	*/
	/*	second, so it's not that bad.	*/
#if DEBUG
	int lclipped = 0;
	int rclipped = 0;
#endif

	/*	keep a list of effects that need SoundCompleted notifications.	*/
	/*	we defer notification until the current sample is done to make	*/
	/*	for seamless sample chaining - if we notified within the sample	*/
	/*	loop for the current sample, the first sample of any new sample	*/
	/*	added in SoundCompleted() would get added to the currently			*/
	/*	playing last sample of the completed sound. Not good.				*/

	effect_id* list = (effect_id*)alloca(sizeof(effect_id) * fPlaySize);
	int nNotify = 0;

	for (int ix = 0; ix < numFrames; ix++) {
		int curl = 0;
		int curr = 0;
		for (int iz = 0; iz < nPlaying; iz++) {
			/*	calculate left and right mix depending on panning	*/
			int offset = fPlayList[iz].offset;
			int sample = fPlayList[iz].data[offset];
			curl += sample * fPlayList[iz].lvol;
			curr += sample * fPlayList[iz].rvol;
			offset++;
			if (offset >= fPlayList[iz].frames) {
				if (fPlayList[iz].loop > 0) {
					/*	loop a predetermined number of times	*/
					fPlayList[iz].loop--;
					fPlayList[iz].offset = 0;
					list[nNotify++] = fPlayList[iz].id;
				} else if (fPlayList[iz].loop < 0) {
					/*	loop forever (until manually stopped)	*/
					fPlayList[iz].offset = 0;
					list[nNotify++] = fPlayList[iz].id;
				} else {
					/*	kick it out	*/
					list[nNotify++] = -fPlayList[iz].id; /*	to know what kind of notify		*/
					memcpy(
						&fPlayList[iz], &fPlayList[iz + 1],
						sizeof(PlayingEffect) * (nPlaying - iz - 1)
					);
					nPlaying--;
					iz--; /*	will get incremented in for() clause	*/
				}
			} else
				fPlayList[iz].offset = offset;
		}
		int left;
		int right;
		curl >>= (22 - fVolume); /*	7 for *127 and up to 15 for volume	*/
		curr >>= (22 - fVolume);
		if (fBGVolume > 0) {
			/*	let the background in 	*/
			left = buffer[ix * 2];
			right = buffer[ix * 2 + 1];
			if (fBGVolume < 15) {
				left = left >> (15 - fBGVolume);
				right = right >> (15 - fBGVolume);
			}
			left += curl;
			right += curr;
		} else {
			left = curl;
			right = curr;
		}
		/*	Check for clipping. Some clipping might be OK in a game, but excessive amounts	*/
		/*	mean that you are allowing the background and/or foreground volumes to go too		*/
		/*	high. Typically, you'll not want the volumes to go above 14 when you have both	*/
		/*	background and foreground on; or 13 if you're playing a lot of sounds				*/
		/*	simultaneously.																		*/
		if (left < -32768) {
#if DEBUG
			lclipped++;
#endif
			left = -32768;
		}
		if (left > 32767) {
#if DEBUG
			lclipped++;
#endif
			left = 32767;
		}
		buffer[ix * 2] = left; /*	write to buffer	*/
		if (right < -32768) {
#if DEBUG
			rclipped++;
#endif
			right = -32768;
		}
		if (right > 32767) {
#if DEBUG
			rclipped++;
#endif
			right = 32767;
		}
		buffer[ix * 2 + 1] = right; /*	write to buffer	*/

		if (nNotify > 0) {
			for (int ix = 0; ix < nNotify; ix++) {
				if (list[ix] > 0)
					SoundCompleted(list[ix], kSoundLooping);
				else
					SoundCompleted(-list[ix], kSoundDone);
			}
			nNotify = 0;
		}
	}
#if DEBUG
	if (rclipped + lclipped > 0) {
		printf("Clipping: %d left %d right\n", lclipped, rclipped);
	}
#endif
	fLock.Unlock();
}

bool
EffectsPlayer::PlayHook(void* userData, char* buffer, size_t count, void* /* header */)
{
	/*	stereo, 16 bit stereo samples -> numFrames	*/
	((EffectsPlayer*)userData)->Mix((short*)buffer, count / 4);
	return true;
}
