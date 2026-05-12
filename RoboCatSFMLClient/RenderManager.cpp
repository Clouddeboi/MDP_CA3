#include "RoboCatClientPCH.hpp"

std::unique_ptr< RenderManager >	RenderManager::sInstance;

RenderManager::RenderManager()
{
	//Game view
	mGameView.reset(sf::FloatRect(0.f, 0.f, 1280.f, 720.f));

	//HUD view
	mHUDView.reset(sf::FloatRect(0.f, 0.f, 1280.f, 720.f));

	WindowManager::sInstance->setView(mGameView);
}


void RenderManager::StaticInit()
{
	sInstance.reset(new RenderManager());
}

void RenderManager::UpdateCamera(const Vector3& playerPos, float playerSize)
{
	//Center camera on the player
	mGameView.setCenter(playerPos.mX, playerPos.mY);

	//Zoom out as the player grows
	//At size 1.0  -> viewport is 1280x720  (normal)
	//At size 3.0  -> viewport is 2560x1440 (2x zoom out)
	//Clamped so the view never shrinks below the base size
	float zoomFactor = 1.0f + (playerSize - 1.0f) * 0.5f;
	if (zoomFactor < 1.0f) zoomFactor = 1.0f;
	if (zoomFactor > 6.0f) zoomFactor = 6.0f;//cap at 6x zoom out

	mGameView.setSize(1280.f * zoomFactor, 720.f * zoomFactor);

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


//this part that renders the world is really a camera-
//in a more detailed engine, we'd have a list of cameras, and then render manager would
//render the cameras in order
void RenderManager::RenderComponents()
{
	//Get the logical viewport so we can pass this to the SpriteComponents when it's draw time
	for (SpriteComponent* c : mComponents)
	{	
		WindowManager::sInstance->draw(c->GetSprite());	
	}
}

void RenderManager::Render()
{
	WindowManager::sInstance->clear(sf::Color(50, 168, 82, 255));

	//Draw world objects using the game camera
	RenderManager::sInstance->RenderComponents();

	//Switch to fixed HUD view so scoreboard text stays on screen
	WindowManager::sInstance->setView(mHUDView);
	HUD::sInstance->Render();

	//Switch back to game view for next frame
	WindowManager::sInstance->setView(mGameView);

	WindowManager::sInstance->display();

}
