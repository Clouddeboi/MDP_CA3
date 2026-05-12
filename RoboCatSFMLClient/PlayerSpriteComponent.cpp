#include "RoboCatClientPCH.hpp"

PlayerSpriteComponent::PlayerSpriteComponent(GameObject* inGameObject) :
	SpriteComponent(inGameObject)
{}

sf::Sprite& PlayerSpriteComponent::GetSprite()
{
	//Update the sprite based on the game object stuff.
	auto pos = mGameObject->GetLocation();
	auto rot = mGameObject->GetRotation();
	float scale = mGameObject->GetScale();

	m_sprite.setPosition(pos.mX, pos.mY);
	m_sprite.setRotation(rot);
	m_sprite.setScale(scale, scale);

	RoboCat* player = dynamic_cast<RoboCat*>(mGameObject);
	Vector3 playerColor = player->GetColor();
	m_sprite.setColor(sf::Color(playerColor.mX, playerColor.mY, playerColor.mZ, 255));

	return m_sprite;
}

void PlayerSpriteComponent::DrawNameLabel()
{
	RoboCat* player = dynamic_cast<RoboCat*>(mGameObject);
	if (!player)
		return;

	//Look up the player name from the scoreboard
	uint32_t playerId = player->GetPlayerId();
	ScoreBoardManager::Entry* entry = ScoreBoardManager::sInstance->GetEntry(playerId);
	if (!entry)
		return;

	const string& name = entry->GetPlayerName();
	float scale = mGameObject->GetScale();

	//Pick a font size that fits comfortably inside
	unsigned int charSize = static_cast<unsigned int>(18.f * scale);
	if (charSize < 6)  charSize = 6;
	if (charSize > 120) charSize = 120;

	sf::Font* font = FontManager::sInstance->GetFont("carlito").get();
	if (!font)
		return;

	sf::Text label;
	label.setString(name);
	label.setFont(*font);
	label.setCharacterSize(charSize);
	label.setFillColor(sf::Color(255, 255, 255, 230));
	label.setOutlineColor(sf::Color(0, 0, 0, 220));
	label.setOutlineThickness(std::max(1.f, charSize * 0.08f));

	//Measure and shrink if text wider than sprite diameter
	float spriteDiameter = 120.f * scale;
	sf::FloatRect bounds = label.getLocalBounds();

	if (bounds.width > spriteDiameter * 0.85f)
	{
		//Scale char size down proportionally so text fits
		float ratio = (spriteDiameter * 0.85f) / bounds.width;
		charSize = static_cast<unsigned int>(charSize * ratio);
		if (charSize < 4) charSize = 4;
		label.setCharacterSize(charSize);
		bounds = label.getLocalBounds();
	}

	//Center the text on the player's world position
	label.setOrigin(bounds.left + bounds.width / 2.f,
		bounds.top + bounds.height / 2.f);

	auto pos = mGameObject->GetLocation();
	label.setPosition(pos.mX, pos.mY);

	WindowManager::sInstance->draw(label);
}
