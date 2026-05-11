#include "RoboCatClientPCH.hpp"

MouseClient::MouseClient()
{
	mSpriteComponent.reset(new SpriteComponent(this));
	mSpriteComponent->SetTexture(TextureManager::sInstance->GetTexture("mouse"));
}
