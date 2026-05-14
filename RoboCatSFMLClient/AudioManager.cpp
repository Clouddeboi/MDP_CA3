#include "RoboCatClientPCH.hpp"

std::unique_ptr<AudioManager> AudioManager::sInstance;

void AudioManager::StaticInit()
{
	sInstance.reset(new AudioManager());
}

AudioManager::AudioManager() :
	mNextPoolIndex(0)
{
	//Cache one-shot SFX
	CacheEatSound("../Assets/Audio/eat1.ogg");
	CacheEatSound("../Assets/Audio/eat2.ogg");
	CacheEatSound("../Assets/Audio/eat3.ogg");
	CacheEatSound("../Assets/Audio/eat4.ogg");
	CacheEatSound("../Assets/Audio/eat5.ogg");
	CacheEatSound("../Assets/Audio/eat6.ogg");

	CacheSound("death", "../Assets/Audio/death.ogg");

	//Dash buffer loaded separately so we can loop it on demand
	mDashBuffer.loadFromFile("../Assets/Audio/dash.ogg");
	mDashSound.setBuffer(mDashBuffer);
	mDashSound.setLoop(true);
	mDashSound.setVolume(50.f);

	//Stream background music from file, does not load entirely into memory
	if (mBackgroundMusic.openFromFile("../Assets/Audio/music.ogg"))
	{
		mBackgroundMusic.setLoop(true);
		mBackgroundMusic.setVolume(15.f);
	}
}

void AudioManager::StartMusic()
{
	if (mBackgroundMusic.getStatus() != sf::Music::Playing)
	{
		mBackgroundMusic.play();
	}
}

void AudioManager::PlayEatSFX()
{
	if (mEatSoundCount == 0)
		return;

	//Pick a random eat sound from those that loaded successfully
	int index = rand() % mEatSoundCount;

	sf::Sound& slot = mSoundPool[mNextPoolIndex];
	mNextPoolIndex = (mNextPoolIndex + 1) % kSoundPoolSize;

	slot.setBuffer(mEatBuffers[index]);
	slot.setVolume(70.f);
	slot.play();
}


void AudioManager::PlaySFX(const string& inName)
{
	auto it = mSoundBuffers.find(inName);
	if (it == mSoundBuffers.end())
		return;

	// Grab the next slot in the pool, overwriting any finished sound
	sf::Sound& slot = mSoundPool[mNextPoolIndex];
	mNextPoolIndex = (mNextPoolIndex + 1) % kSoundPoolSize;

	slot.setBuffer(it->second);
	slot.setVolume(80.f);
	slot.play();
}

void AudioManager::StartDashSound()
{
	if (mDashSound.getStatus() != sf::Sound::Playing)
	{
		mDashSound.play();
	}
}

void AudioManager::StopDashSound()
{
	if (mDashSound.getStatus() == sf::Sound::Playing)
	{
		mDashSound.stop();
	}
}

bool AudioManager::CacheEatSound(const char* inFileName)
{
	if (mEatSoundCount >= kMaxEatSounds)
		return false;

	if (!mEatBuffers[mEatSoundCount].loadFromFile(inFileName))
		return false;//If file missing, slot stays unused, count stays the same

	++mEatSoundCount;
	return true;
}

bool AudioManager::CacheSound(const string& inName, const char* inFileName)
{
	sf::SoundBuffer buffer;
	if (!buffer.loadFromFile(inFileName))
		return false;

	mSoundBuffers[inName] = std::move(buffer);
	return true;
}