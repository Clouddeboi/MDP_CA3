#include "RoboCatClientPCH.hpp"
#include <iostream>
#include <fstream>

namespace
{
	//Reads the config file and fills outDestination and outName
	//Falls back to command line args if the file is missing or incomplete
	void LoadConnectionConfig(string& outDestination, string& outName)
	{
		std::ifstream configFile("../Assets/config.txt");
		if (configFile.is_open())
		{
			std::getline(configFile, outDestination);
			std::getline(configFile, outName);

			if (!outDestination.empty() && outDestination.back() == '\r')
				outDestination.pop_back();
			if (!outName.empty() && outName.back() == '\r')
				outName.pop_back();
		}

		//Fall back to command line args if file was missing or lines were empty
		if (outDestination.empty())
			outDestination = StringUtils::GetCommandLineArg(1);
		if (outName.empty())
			outName = StringUtils::GetCommandLineArg(2);
	}
}

bool Client::StaticInit()
{
	// Create the Client pointer first because it initializes SDL
	Client* client = new Client();
	InputManager::StaticInit();

	WindowManager::StaticInit();
	FontManager::StaticInit();
	TextureManager::StaticInit();
	RenderManager::StaticInit();
	AudioManager::StaticInit();

	HUD::StaticInit();

	s_instance.reset(client);

	AudioManager::sInstance->StartMusic();

	return true;
}

Client::Client()
{
	GameObjectRegistry::sInstance->RegisterCreationFunction('RCAT', RoboCatClient::StaticCreate);
	GameObjectRegistry::sInstance->RegisterCreationFunction('MOUS', MouseClient::StaticCreate);

	string destination = StringUtils::GetCommandLineArg(1);
	string name = StringUtils::GetCommandLineArg(2);

	SocketAddressPtr serverAddress = SocketAddressFactory::CreateIPv4FromString(destination);

	NetworkManagerClient::StaticInit(*serverAddress, name);

	//NetworkManagerClient::sInstance->SetSimulatedLatency(0.0f);
}



void Client::DoFrame()
{
	InputManager::sInstance->Update();

	Engine::DoFrame();

	NetworkManagerClient::sInstance->ProcessIncomingPackets();

	//Find the local player's cat and update the camera to follow it
	uint32_t localPlayerId = NetworkManagerClient::sInstance->GetPlayerId();
	const auto& gameObjects = World::sInstance->GetGameObjects();
	for (const auto& go : gameObjects)
	{
		RoboCat* cat = go->GetAsCat();
		if (cat && cat->GetPlayerId() == localPlayerId)
		{
			RenderManager::sInstance->UpdateCamera(cat->GetLocation(), cat->GetSize());

			//Drive dash audio from the local player's current dash state
			if (cat->IsDashing())
				AudioManager::sInstance->StartDashSound();
			else
				AudioManager::sInstance->StopDashSound();

			break;
		}
	}


	RenderManager::sInstance->Render();

	NetworkManagerClient::sInstance->SendOutgoingPackets();
}

void Client::HandleEvent(sf::Event& p_event)
{
	switch (p_event.type)
	{
	case sf::Event::KeyPressed:
		InputManager::sInstance->HandleInput(EIA_Pressed, p_event.key.code);
		break;
	case sf::Event::KeyReleased:
		InputManager::sInstance->HandleInput(EIA_Released, p_event.key.code);
		break;
	default:
		break;
	}
}

bool Client::PollEvent(sf::Event& p_event)
{
	return WindowManager::sInstance->pollEvent(p_event);
}


