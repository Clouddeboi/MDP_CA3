#include <iostream>
#include "RoboCatServerPCH.hpp"

bool Server::StaticInit()
{
	s_instance.reset(new Server());
	return true;
}

Server::Server() :
	mPickupRespawnTimer(0.f)
{
	GameObjectRegistry::sInstance->RegisterCreationFunction('RCAT', RoboCatServer::StaticCreate);
	GameObjectRegistry::sInstance->RegisterCreationFunction('MOUS', MouseServer::StaticCreate);

	InitNetworkManager();

	float latency = 0.0f;
	string latencyString = StringUtils::GetCommandLineArg(2);
	if (!latencyString.empty())
	{
		latency = stof(latencyString);
	}
	NetworkManagerServer::sInstance->SetSimulatedLatency(latency);
}

int Server::Run()
{
	SetupWorld();
	return Engine::Run();
}

bool Server::InitNetworkManager()
{
	string portString = StringUtils::GetCommandLineArg(1);
	uint16_t port = stoi(portString);
	return NetworkManagerServer::StaticInit(port);
}

namespace
{
	const int   kMaxPickups = 30;
	const float kPickupRespawnDelay = 2.f;

	void SpawnMice(int inCount)
	{
		Vector3 mouseMin(100.f, 100.f, 0.f);
		Vector3 mouseMax(4900.f, 4900.f, 0.f);

		for (int i = 0; i < inCount; ++i)
		{
			GameObjectPtr go = GameObjectRegistry::sInstance->CreateGameObject('MOUS');
			go->SetLocation(RoboMath::GetRandomVector(mouseMin, mouseMax));
		}
	}

	int CountPickups()
	{
		int count = 0;
		for (const auto& go : World::sInstance->GetGameObjects())
		{
			if (go->GetClassId() == 'MOUS')
			{
				++count;
			}
		}
		return count;
	}
}

void Server::SetupWorld()
{
	SpawnMice(kMaxPickups);
}

void Server::DoFrame()
{
	NetworkManagerServer::sInstance->ProcessIncomingPackets();
	NetworkManagerServer::sInstance->CheckForDisconnects();
	NetworkManagerServer::sInstance->RespawnCats();

	Engine::DoFrame();

	RespawnPickupsIfNeeded();

	NetworkManagerServer::sInstance->SendOutgoingPackets();
}

void Server::RespawnPickupsIfNeeded()
{
	float time = Timing::sInstance.GetFrameStartTime();

	if (time > mPickupRespawnTimer + kPickupRespawnDelay)
	{
		mPickupRespawnTimer = time;

		int current = CountPickups();
		if (current < kMaxPickups)
		{
			SpawnMice(kMaxPickups - current);
		}
	}
}

void Server::HandleNewClient(ClientProxyPtr inClientProxy)
{
	int playerId = inClientProxy->GetPlayerId();
	ScoreBoardManager::sInstance->AddEntry(playerId, inClientProxy->GetName());
	SpawnCatForPlayer(playerId);
}

void Server::SpawnCatForPlayer(int inPlayerId)
{
	RoboCatPtr cat = std::static_pointer_cast<RoboCat>(GameObjectRegistry::sInstance->CreateGameObject('RCAT'));
	cat->SetColor(ScoreBoardManager::sInstance->GetEntry(inPlayerId)->GetColor());
	cat->SetPlayerId(inPlayerId);
	cat->SetSize(1.0f);

	//Random spawn anywhere in the world (for now just spawn so we can see them)
	Vector3 spawnMin(200.f, 200.f, 0.f);
	Vector3 spawnMax(200.f, 200.f, 0.f);
	cat->SetLocation(RoboMath::GetRandomVector(spawnMin, spawnMax));
}

void Server::HandleLostClient(ClientProxyPtr inClientProxy)
{
	int playerId = inClientProxy->GetPlayerId();
	ScoreBoardManager::sInstance->RemoveEntry(playerId);
	RoboCatPtr cat = GetCatForPlayer(playerId);
	if (cat)
	{
		cat->SetDoesWantToDie(true);
	}
}

RoboCatPtr Server::GetCatForPlayer(int inPlayerId)
{
	const auto& gameObjects = World::sInstance->GetGameObjects();
	for (int i = 0, c = gameObjects.size(); i < c; ++i)
	{
		GameObjectPtr go = gameObjects[i];
		RoboCat* cat = go->GetAsCat();
		if (cat && cat->GetPlayerId() == inPlayerId)
		{
			return std::static_pointer_cast<RoboCat>(go);
		}
	}
	return nullptr;
}