#include "RoboCatServerPCH.hpp"

const float Server::kRoundOverDuration = 5.f;

namespace
{
	const float kWorldCX = 2500.f;
	const float kWorldCY = 2500.f;
	const float kWorldRadius = 2300.f;

	//Returns a random point guaranteed to be inside the world circle,
	//padded by inMargin so spawns never appear right on the boundary.
	Vector3 GetRandomPointInCircle(float inMargin)
	{
		float spawnRadius = kWorldRadius - inMargin;

		//Uniform distribution inside a circle via polar coordinates:
		//sqrt(random) gives a uniform radial distribution (not biased toward centre)
		float r = spawnRadius * std::sqrt(RoboMath::GetRandomFloat());
		float angle = RoboMath::GetRandomFloat() * 2.f * RoboMath::PI;

		return Vector3(
			kWorldCX + r * std::cos(angle),
			kWorldCY + r * std::sin(angle),
			0.f);
	}

	// Returns true if inPosition is clear of all living cats,
	// given that the spawning player will have inSpawnRadius collision radius.
	bool IsSpawnPositionClear(const Vector3& inPosition, float inSpawnRadius)
	{
		for (const auto& go : World::sInstance->GetGameObjects())
		{
			if (go->DoesWantToDie())
				continue;

			RoboCat* cat = go->GetAsCat();
			if (!cat)
				continue;

			Vector3 delta = go->GetLocation() - inPosition;
			float   distSq = delta.LengthSq2D();
			float   minSafeDist = inSpawnRadius + cat->GetCollisionRadius();

			// Add a small extra gap so the spawned player is visibly separate
			const float kSafetyPadding = 60.f;
			minSafeDist += kSafetyPadding;

			if (distSq < minSafeDist * minSafeDist)
				return false;
		}
		return true;
	}

	// Tries up to kMaxAttempts random positions and returns the first clear one.
	// Falls back to the last candidate if no clear position is found
	// (can happen in very crowded servers).
	Vector3 FindClearSpawnPoint(float inMargin, float inSpawnRadius)
	{
		const int kMaxAttempts = 20;
		Vector3 candidate;

		for (int i = 0; i < kMaxAttempts; ++i)
		{
			candidate = GetRandomPointInCircle(inMargin);
			if (IsSpawnPositionClear(candidate, inSpawnRadius))
				return candidate;
		}

		// No clear spot found — return the last candidate anyway
		return candidate;
	}
}


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
	string portString;

	//Try to read port from config file first
	std::ifstream configFile("../Assets/server_config.txt");
	if (configFile.is_open())
	{
		std::getline(configFile, portString);

		//Trim carriage return for Windows line endings
		if (!portString.empty() && portString.back() == '\r')
			portString.pop_back();
	}

	//Fall back to command line arg if file missing or empty
	if (portString.empty())
		portString = StringUtils::GetCommandLineArg(1);

	if (portString.empty())
	{
		//Last resort default port
		portString = "50000";
	}

	uint16_t port = static_cast<uint16_t>(stoi(portString));
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
		for (int i = 0; i < inCount; ++i)
		{
			GameObjectPtr go = GameObjectRegistry::sInstance->CreateGameObject('MOUS');
			//150 unit margin keeps pickups away from the border ring
			go->SetLocation(GetRandomPointInCircle(150.f));
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

	if (mRoundOver)
	{
		mRoundOverTimer -= Timing::sInstance.GetDeltaTime();
		if (mRoundOverTimer <= 0.f)
		{
			mRoundOver = false;
			ResetRound();
		}

		//Still send heartbeats so clients stay connected
		float time = Timing::sInstance.GetFrameStartTime();
		if (time > mHeartbeatTimer + kHeartbeatInterval)
		{
			mHeartbeatTimer = time;
			NetworkManagerServer::sInstance->UpdateAllClients();
		}
		NetworkManagerServer::sInstance->SendOutgoingPackets();
		return;
	}

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

void Server::HandleRoundWon(int inWinnerPlayerId)
{
	if (mRoundOver) return;

	mRoundOver = true;
	mRoundOverTimer = kRoundOverDuration;

	//Look up winner name from scoreboard
	string winnerName = "Unknown";
	ScoreBoardManager::Entry* entry = ScoreBoardManager::sInstance->GetEntry(inWinnerPlayerId);
	if (entry) winnerName = entry->GetPlayerName();

	BroadcastRoundOver(winnerName);
}

void Server::BroadcastRoundOver(const string& inWinnerName)
{
	//Build a kRoundOverCC packet containing the winner name and countdown duration
	OutputMemoryBitStream packet;
	packet.Write(NetworkManager::kRoundOverCC);
	packet.Write(inWinnerName);
	packet.Write(kRoundOverDuration);

	//Send directly to every connected client
	const auto& clientMap = NetworkManagerServer::sInstance->GetAddressToClientMap();
	for (const auto& pair : clientMap)
		NetworkManagerServer::sInstance->SendPacket(packet, pair.first);
}

void Server::ResetRound()
{
	// Reset every living cat to minimum size and respawn at a new clear location
	const auto& gameObjects = World::sInstance->GetGameObjects();
	for (int i = 0, c = static_cast<int>(gameObjects.size()); i < c; ++i)
	{
		RoboCat* cat = gameObjects[i]->GetAsCat();
		if (!cat || cat->DoesWantToDie()) continue;

		cat->SetSize(RoboCat::kMinSize);
		cat->SetVelocity(Vector3::Zero);
		cat->SetLocation(FindClearSpawnPoint(250.f, 60.f));

		int netId = cat->GetNetworkId();
		NetworkManagerServer::sInstance->SetStateDirty(netId, RoboCat::ECRS_Size);
		NetworkManagerServer::sInstance->SetStateDirty(netId, RoboCat::ECRS_Pose);

		//Update scoreboard to reflect reset size
		if (cat->GetPlayerId() > 0)
			ScoreBoardManager::sInstance->UpdateSize(cat->GetPlayerId(), RoboCat::kMinSize);
	}
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
	RoboCatPtr cat = std::static_pointer_cast<RoboCat>(
		GameObjectRegistry::sInstance->CreateGameObject('RCAT'));

	cat->SetColor(ScoreBoardManager::sInstance->GetEntry(inPlayerId)->GetColor());
	cat->SetPlayerId(inPlayerId);
	cat->SetSize(1.0f);

	//Collision radius at size 1.0 is 60.f
	const float kSpawnCollisionRadius = 60.f;
	const float kBorderMargin = 250.f;

	//Find a spawn point that does not overlap any existing living cat
	Vector3 spawnPos = FindClearSpawnPoint(kBorderMargin, kSpawnCollisionRadius);
	cat->SetLocation(spawnPos);

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
	for (int i = 0, c = static_cast<int>(gameObjects.size()); i < c; ++i)
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