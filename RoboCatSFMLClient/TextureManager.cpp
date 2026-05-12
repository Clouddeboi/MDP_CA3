#include "RoboCatClientPCH.hpp"

std::unique_ptr< TextureManager >		TextureManager::sInstance;

void TextureManager::StaticInit()
{
	sInstance.reset(new TextureManager());
}

TextureManager::TextureManager()
{
	CacheTexture("PlayerBlob", "../Assets/PlayerBlob.png");
	CacheTexture("mouse", "../Assets/mouse.png");
	CacheTexture("yarn", "../Assets/yarn.png");
	CacheTexture("tiles", "../Assets/tiles.png");

	CacheTexture("GreenBlob", "../Assets/GreenBlob.png");
	CacheTexture("GreenBlob", "../Assets/GreenBlob.png");
	CacheTexture("RedBlob", "../Assets/RedBlob.png");
	CacheTexture("YellowBlob", "../Assets/YellowBlob.png");
}

TexturePtr	TextureManager::GetTexture(const string& inTextureName)
{
	auto it = mNameToTextureMap.find(inTextureName);
	if (it != mNameToTextureMap.end())
		return it->second;

	return nullptr;
}

bool TextureManager::CacheTexture(string inTextureName, const char* inFileName)
{
	TexturePtr newTexture(new sf::Texture());
	if (!newTexture->loadFromFile(inFileName))
	{
		return false;
	}

	mNameToTextureMap[inTextureName] = newTexture;
	return true;

}
