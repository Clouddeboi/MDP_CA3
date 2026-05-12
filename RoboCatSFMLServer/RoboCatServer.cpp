#include "RoboCatServerPCH.hpp"

RoboCatServer::RoboCatServer() :
	mCatControlType(ESCT_Human)
{}

void RoboCatServer::HandleDying()
{
	NetworkManagerServer::sInstance->UnregisterGameObject(this);
}

void RoboCatServer::Update()
{
	RoboCat::Update();

	Vector3 oldLocation = GetLocation();
	Vector3 oldVelocity = GetVelocity();

	//are you controlled by a player?
	//if so, is there a move we haven't processed yet?
	if (mCatControlType == ESCT_Human)
	{
		ClientProxyPtr client = NetworkManagerServer::sInstance->GetClientProxy(GetPlayerId());
		if (client)
		{
			MoveList& moveList = client->GetUnprocessedMoveList();
			for (const Move& unprocessedMove : moveList)
			{
				const InputState& currentState = unprocessedMove.GetInputState();

				float deltaTime = unprocessedMove.GetDeltaTime();

				ProcessInput(deltaTime, currentState);
				SimulateMovement(deltaTime);

				//LOG( "Server Move Time: %3.4f deltaTime: %3.4f left rot at %3.4f", unprocessedMove.GetTimestamp(), deltaTime, GetRotation() );

			}

			moveList.Clear();
		}
	}
	else
	{
		//do some AI stuff
		SimulateMovement(Timing::sInstance.GetDeltaTime());
	}

	CheckForEating();

	if (!RoboMath::Is2DVectorEqual(oldLocation, GetLocation()) ||
		!RoboMath::Is2DVectorEqual(oldVelocity, GetVelocity()))
	{
		NetworkManagerServer::sInstance->SetStateDirty(GetNetworkId(), ECRS_Pose);
	}
}

void RoboCatServer::GrowBy(float inAmount)
{
	float newSize = GetSize() + inAmount;
	SetSize(newSize);
	NetworkManagerServer::sInstance->SetStateDirty(GetNetworkId(), ECRS_Size);
	//Collision radius changed so position may need correcting — mark pose dirty too
	NetworkManagerServer::sInstance->SetStateDirty(GetNetworkId(), ECRS_Pose);
}

void RoboCatServer::CheckForEating()
{
	//Don't eat if we're already dying
	if (DoesWantToDie())
		return;

	float myRadius = GetCollisionRadius();
	Vector3 myLocation = GetLocation();
	float mySize = GetSize();

	for (auto goIt = World::sInstance->GetGameObjects().begin(),
		end = World::sInstance->GetGameObjects().end(); goIt != end; ++goIt)
	{
		GameObject* target = goIt->get();

		//Only eat other cats that are still alive
		if (target == this || target->DoesWantToDie())
			continue;

		RoboCat* targetCat = target->GetAsCat();
		if (!targetCat)
			continue;

		float targetSize = targetCat->GetSize();

		//Must be at least 10% bigger to eat
		if (mySize <= targetSize * 1.1f)
			continue;

		//Sphere overlap check
		Vector3 delta = targetCat->GetLocation() - myLocation;
		float distSq = delta.LengthSq2D();
		float eatDist = myRadius;//fully overlap target's center
		if (distSq >= (eatDist * eatDist))
			continue;

		//Grow by a portion of the eaten cat's size
		GrowBy(targetSize * 0.5f);

		//Kill the victim
		targetCat->SetDoesWantToDie(true);

		//Notify the victim's client so respawn timer starts
		int victimPlayerId = targetCat->GetPlayerId();
		if (victimPlayerId > 0)
		{
			ClientProxyPtr victimClient = NetworkManagerServer::sInstance->GetClientProxy(victimPlayerId);
			if (victimClient)
			{
				victimClient->HandleCatDied();
			}
		}

		//Award a score point to the eater
		ScoreBoardManager::sInstance->IncScore(GetPlayerId(), 3);

		break;
	}
}