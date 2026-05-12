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
	const int   kMaxPickups = 80;
	const int   kPickupsPerSpawn = 5;
	const float kPickupRespawnDelay = 1.f;

	const float kHeartbeatInterval = 0.1f;

	void SpawnMice(int inCount)
	{
		Vector3 pickupMin(200.f, 200.f, 0.f);
		Vector3 pickupMax(4800.f, 4800.f, 0.f);

		for (int i = 0; i < inCount; ++i)
		{
			GameObjectPtr go = GameObjectRegistry::sInstance->CreateGameObject('MOUS');
			go->SetLocation(RoboMath::GetRandomVector(pickupMin, pickupMax));
		}
	}

	int CountPickups()
	{
		int count = 0;
		for (const auto& go : World::sInstance->GetGameObjects())
		{
			if (go->GetClassId() == 'MOUS')
				++count;
		}
		return count;
	}
}

void Server::SetupWorld()
{
	SpawnMice(kPickupsPerSpawn);
}

void Server::DoFrame()
{
	NetworkManagerServer::sInstance->ProcessIncomingPackets();
	NetworkManagerServer::sInstance->CheckForDisconnects();
	NetworkManagerServer::sInstance->RespawnCats();

	Engine::DoFrame();

	RespawnPickupsIfNeeded();

	float time = Timing::sInstance.GetFrameStartTime();
	if (time > mHeartbeatTimer + kHeartbeatInterval)
	{
		mHeartbeatTimer = time;
		NetworkManagerServer::sInstance->UpdateAllClients();
	}

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
			//Spawn a small batch per tick instead of all at once
			int needed = kMaxPickups - current;
			int toSpawn = (needed < kPickupsPerSpawn) ? needed : kPickupsPerSpawn;
			SpawnMice(toSpawn);
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
	Vector3 spawnMax(4700.f, 4700.f, 0.f);
	cat->SetLocation(RoboMath::GetRandomVector(spawnMin, spawnMax));

	NetworkManagerServer::sInstance->SetStateDirty(cat->GetNetworkId(), RoboCat::ECRS_AllState);
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