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
	CacheSound("eat", "../Assets/Audio/eat.ogg");
	CacheSound("death", "../Assets/Audio/death.ogg");

	//Dash buffer loaded separately so we can loop it on demand
	mDashBuffer.loadFromFile("../Assets/Audio/dash.wav");
	mDashSound.setBuffer(mDashBuffer);
	mDashSound.setLoop(true);
	mDashSound.setVolume(60.f);

	//Stream background music from file, does not load entirely into memory
	if (mBackgroundMusic.openFromFile("../Assets/Audio/music.ogg"))
	{
		mBackgroundMusic.setLoop(true);
		mBackgroundMusic.setVolume(35.f);
	}
}

void AudioManager::StartMusic()
{
	if (mBackgroundMusic.getStatus() != sf::Music::Playing)
	{
		mBackgroundMusic.play();
	}
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

bool AudioManager::CacheSound(const string& inName, const char* inFileName)
{
	sf::SoundBuffer buffer;
	if (!buffer.loadFromFile(inFileName))
		return false;

	mSoundBuffers[inName] = std::move(buffer);
	return true;
}