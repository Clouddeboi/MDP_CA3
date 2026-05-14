#pragma once
class PlayerSpriteComponent : public SpriteComponent
{
public:
	PlayerSpriteComponent(GameObject* inGameObject);
	virtual sf::Sprite& GetSprite() override;
	virtual void DrawNameLabel() override;

private:
	float mAnimTime;
};
typedef shared_ptr<PlayerSpriteComponent >	PlayerSpriteComponentPtr;

