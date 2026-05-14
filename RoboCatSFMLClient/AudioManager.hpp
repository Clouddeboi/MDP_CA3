#pragma once

class AudioManager
{
public:
	static void StaticInit();
	static std::unique_ptr<AudioManager> sInstance;

	//Background music, streams from file, loops automatically
	void StartMusic();

	//One-shot SFX
	void PlaySFX(const string& inName);

	//Plays a random eat sound from the eat pool
	void PlayEatSFX();

	//Dash sound, looping while held, stopped on release
	void StartDashSound();
	void StopDashSound();

private:
	AudioManager();

	bool CacheSound(const string& inName, const char* inFileName);
	bool CacheEatSound(const char* inFileName);

	//Background music, sf::Music streams directly from disk
	sf::Music mBackgroundMusic;

	//Dedicated dash sound that loops while held
	sf::SoundBuffer mDashBuffer;
	sf::Sound       mDashSound;

	//One-shot SFX pool, buffers kept alive, sounds recycled
	unordered_map<string, sf::SoundBuffer> mSoundBuffers;

	//Pool of eat sound buffers, one is chosen at random each time
	static const int kMaxEatSounds = 6;
	sf::SoundBuffer  mEatBuffers[kMaxEatSounds];
	int              mEatSoundCount;

	//Small pool so multiple sounds can overlap without cutting each other off
	static const int kSoundPoolSize = 8;
	sf::Sound        mSoundPool[kSoundPoolSize];
	int              mNextPoolIndex;
};