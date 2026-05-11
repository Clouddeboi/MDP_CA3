#include "RoboCatPCH.hpp"

std::unique_ptr<Engine> Engine::s_instance;

Engine::Engine() : m_should_keep_running(true)
{
	SocketUtil::StaticInit();

	srand(static_cast<uint32_t>(time(nullptr)));

	GameObjectRegistry::StaticInit();

	World::StaticInit();

	ScoreBoardManager::StaticInit();
}

Engine::~Engine()
{
	SocketUtil::CleanUp();
}

int Engine::Run()
{
	return DoRunLoop();
}

void Engine::HandleEvent(sf::Event& p_event)
{
	// Default implementation does nothing, up to derived classes to handle them, if they so choose
	(void)p_event;
}

bool Engine::PollEvent(sf::Event& p_event)
{
	return false;
}

int Engine::DoRunLoop()
{
	//Main message loop
	bool quit = false;

	sf::Event current_event;

	while (!quit && m_should_keep_running)
	{
		while (PollEvent(current_event))
		{
			if (current_event.type == sf::Event::Closed)
			{
				quit = true;
			}
			else
			{
				HandleEvent(current_event);
			}
		}

		Timing::sInstance.Update();
		DoFrame();
	}
	return current_event.type;
}

void Engine::DoFrame()
{
	World::sInstance->Update();
}




