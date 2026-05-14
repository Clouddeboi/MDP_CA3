#include "RoboCatClientPCH.hpp"

std::unique_ptr< RenderManager >	RenderManager::sInstance;

namespace
{
	const float kMaxManualZoom = 4.0f;

	// How quickly the camera zoom catches up to the target (higher = snappier)
	const float kZoomLerpSpeed = 8.0f;

	const float kWorldCX = 2500.f;
	const float kWorldCY = 2500.f;
	const float kWorldR = 2300.f;
}

RenderManager::RenderManager():
	mTargetZoomOffset(0.f),
	mCurrentZoomOffset(0.f)
{
	//Game view
	mGameView.reset(sf::FloatRect(0.f, 0.f, 1280.f, 720.f));

	//HUD view
	mHUDView.reset(sf::FloatRect(0.f, 0.f, 1280.f, 720.f));

	//Load tile texture
	if (mBackgroundTexture.loadFromFile("../Assets/tiles.png"))
	{
		mBackgroundTexture.setRepeated(true);

		//Set texture rect to cover the entire world (5000x5000)
		//SFML will repeat the tile texture across this rect
		mBackgroundSprite.setTexture(mBackgroundTexture);
		mBackgroundSprite.setTextureRect(sf::IntRect(0, 0, 5000, 5000));
		mBackgroundSprite.setPosition(0.f, 0.f);
	}

	mBorderCircle.setRadius(kWorldR);
	mBorderCircle.setOrigin(kWorldR, kWorldR);
	mBorderCircle.setPosition(kWorldCX, kWorldCY);
	mBorderCircle.setFillColor(sf::Color::Transparent);
	mBorderCircle.setOutlineColor(sf::Color(255, 80, 80, 220));
	mBorderCircle.setOutlineThickness(18.f);
	mBorderCircle.setPointCount(128);   // high point count = smooth circle


	WindowManager::sInstance->setView(mGameView);
}

void RenderManager::AdjustTargetZoom(float inDelta)
{
	mTargetZoomOffset += inDelta;

	//Q can never zoom in past the size-based floor
	if (mTargetZoomOffset < 0.f)            mTargetZoomOffset = 0.f;
	if (mTargetZoomOffset > kMaxManualZoom)  mTargetZoomOffset = kMaxManualZoom;
}

void RenderManager::StaticInit()
{
	sInstance.reset(new RenderManager());
}

void RenderManager::UpdateCamera(const Vector3& playerPos, float playerSize)
{
	float dt = Timing::sInstance.GetDeltaTime();
	mCurrentZoomOffset += (mTargetZoomOffset - mCurrentZoomOffset) * kZoomLerpSpeed * dt;

	mGameView.setCenter(playerPos.mX, playerPos.mY);

	float zoomFactor = 1.0f + (playerSize - 1.0f) * 0.5f;
	if (zoomFactor < 1.0f) zoomFactor = 1.0f;
	if (zoomFactor > 6.0f) zoomFactor = 6.0f;

	float totalZoom = zoomFactor + mCurrentZoomOffset;
	if (totalZoom > 6.0f + kMaxManualZoom) totalZoom = 6.0f + kMaxManualZoom;

	mGameView.setSize(1280.f * totalZoom, 720.f * totalZoom);
	WindowManager::sInstance->setView(mGameView);
}

void RenderManager::AddComponent(SpriteComponent* inComponent)
{
	mComponents.emplace_back(inComponent);
}

void RenderManager::RemoveComponent(SpriteComponent* inComponent)
{
	int index = GetComponentIndex(inComponent);

	if (index != -1)
	{
		int lastIndex = mComponents.size() - 1;
		if (index != lastIndex)
		{
			mComponents[index] = mComponents[lastIndex];
		}
		mComponents.pop_back();
	}
}

int RenderManager::GetComponentIndex(SpriteComponent* inComponent) const
{
	for (int i = 0, c = mComponents.size(); i < c; ++i)
	{
		if (mComponents[i] == inComponent)
		{
			return i;
		}
	}

	return -1;
}

void RenderManager::RenderBackground()
{
	WindowManager::sInstance->draw(mBackgroundSprite);

	WindowManager::sInstance->draw(mBorderCircle);
}

//this part that renders the world is really a camera-
//in a more detailed engine, we'd have a list of cameras, and then render manager would
//render the cameras in order
void RenderManager::RenderComponents()
{
	//Sort smallest scale first so larger players render on top
	std::sort(mComponents.begin(), mComponents.end(),
		[](const SpriteComponent* a, const SpriteComponent* b)
		{
			return a->GetGameObjectScale() < b->GetGameObjectScale();
		});

	for (SpriteComponent* c : mComponents)
	{
		WindowManager::sInstance->draw(c->GetSprite());
		c->DrawNameLabel();
	}
}

void RenderManager::Render()
{
	// Clear with a neutral colour — only visible outside the world boundary
	WindowManager::sInstance->clear(sf::Color(30, 30, 30, 255));

	//Draw tiled world background
	RenderBackground();

	//Draw all game objects on top
	RenderComponents();

	//Switch to fixed HUD view
	WindowManager::sInstance->setView(mHUDView);
	HUD::sInstance->Render();

	//Restore game view for next frame
	WindowManager::sInstance->setView(mGameView);

	WindowManager::sInstance->display();
}
