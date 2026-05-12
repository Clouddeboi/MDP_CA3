#include "RoboCatClientPCH.hpp"


SpriteComponent::SpriteComponent(GameObject* inGameObject) :
	mGameObject(inGameObject)
{
	RenderManager::sInstance->AddComponent(this);
}

SpriteComponent::~SpriteComponent()
{
	RenderManager::sInstance->RemoveComponent(this);
}

void SpriteComponent::SetTexture(TexturePtr inTexture)
{
	auto tSize = inTexture->getSize();
	m_sprite.setTexture(*inTexture);
	m_sprite.setOrigin(tSize.x / 2, tSize.y / 2);
}

sf::Sprite& SpriteComponent::GetSprite()
{
	auto pos = mGameObject->GetLocation();
	auto rot = mGameObject->GetRotation();
	float scale = mGameObject->GetScale();

	m_sprite.setPosition(pos.mX, pos.mY);
	m_sprite.setRotation(rot);
	m_sprite.setScale(scale, scale);

	return m_sprite;
}

