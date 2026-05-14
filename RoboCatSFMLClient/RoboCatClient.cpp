#include "RoboCatClientPCH.hpp"

RoboCatClient::RoboCatClient() :
	mTimeLocationBecameOutOfSync(0.f),
	mTimeVelocityBecameOutOfSync(0.f),
	mLastKnownSize(1.0f)
{
	mSpriteComponent.reset(new PlayerSpriteComponent(this));
	mSpriteComponent->SetTexture(TextureManager::sInstance->GetTexture("PlayerBlob"));
}

void RoboCatClient::HandleDying()
{
	if (GetPlayerId() == NetworkManagerClient::sInstance->GetPlayerId())
	{
		HUD::sInstance->StartRespawnCountdown(3.f);

		//Play death sound when the local player is eaten
		AudioManager::sInstance->PlaySFX("death");

		//Stop dash sound in case it was playing when we died
		AudioManager::sInstance->StopDashSound();
	}

	RoboCat::HandleDying();
}


void RoboCatClient::Update()
{
	//is this the cat owned by us?
	if (GetPlayerId() == NetworkManagerClient::sInstance->GetPlayerId())
	{
		const Move* pendingMove = InputManager::sInstance->GetAndClearPendingMove();
		//in theory, only do this if we want to sample input this frame / if there's a new move ( since we have to keep in sync with server )
		if (pendingMove) //is it time to sample a new move...
		{
			float deltaTime = pendingMove->GetDeltaTime();

			//apply that input

			ProcessInput(deltaTime, pendingMove->GetInputState());

			//and simulate!

			SimulateMovement(deltaTime);

			//LOG( "Client Move Time: %3.4f deltaTime: %3.4f left rot at %3.4f", latestMove.GetTimestamp(), deltaTime, GetRotation() );
		}
	}
	else
	{
		SimulateMovement(Timing::sInstance.GetDeltaTime());

		if (RoboMath::Is2DVectorEqual(GetVelocity(), Vector3::Zero))
		{
			//we're in sync if our velocity is 0
			mTimeLocationBecameOutOfSync = 0.f;
		}
	}
}

void RoboCatClient::Read(InputMemoryBitStream& inInputStream)
{
	bool stateBit;

	uint32_t readState = 0;

	inInputStream.Read(stateBit);
	if (stateBit)
	{
		uint32_t playerId;
		inInputStream.Read(playerId);
		SetPlayerId(playerId);
		readState |= ECRS_PlayerId;
	}

	Vector3 oldLocation = GetLocation();
	Vector3 oldVelocity = GetVelocity();

	Vector3 replicatedLocation;
	Vector3 replicatedVelocity;

	inInputStream.Read(stateBit);
	if (stateBit)
	{
		inInputStream.Read(replicatedVelocity.mX);
		inInputStream.Read(replicatedVelocity.mY);
		SetVelocity(replicatedVelocity);

		inInputStream.Read(replicatedLocation.mX);
		inInputStream.Read(replicatedLocation.mY);
		SetLocation(replicatedLocation);

		readState |= ECRS_Pose;
	}

	//Read horizontal dir
	auto ReadDir = [&](float& outDir)
		{
			bool isNonZero;
			inInputStream.Read(isNonZero);
			if (isNonZero)
			{
				bool isPositive;
				inInputStream.Read(isPositive);
				outDir = isPositive ? 1.f : -1.f;
			}
			else
			{
				outDir = 0.f;
			}
		};
	ReadDir(mHorizontalDir);
	ReadDir(mVerticalDir);

	inInputStream.Read(stateBit);
	if (stateBit)
	{
		Vector3 color;
		inInputStream.Read(color);
		SetColor(color);
		readState |= ECRS_Color;
	}

	inInputStream.Read(stateBit);
	if (stateBit)
	{
		float replicatedSize;
		inInputStream.Read(replicatedSize);

		if (GetPlayerId() == NetworkManagerClient::sInstance->GetPlayerId())
		{
			if (replicatedSize > mLastKnownSize + 0.01f)
			{
				AudioManager::sInstance->PlaySFX("eat");
			}
		}

		mLastKnownSize = replicatedSize;
		SetSize(replicatedSize);
		readState |= ECRS_Size;
	}

	if (GetPlayerId() == NetworkManagerClient::sInstance->GetPlayerId())
	{
		DoClientSidePredictionAfterReplicationForLocalCat(readState);

		if ((readState & ECRS_PlayerId) == 0)
		{
			InterpolateClientSidePrediction(oldLocation, oldVelocity, false);
		}
	}
	else
	{
		DoClientSidePredictionAfterReplicationForRemoteCat(readState);

		if ((readState & ECRS_PlayerId) == 0)
		{
			InterpolateClientSidePrediction(oldLocation, oldVelocity, true);
		}
	}
}

void RoboCatClient::DoClientSidePredictionAfterReplicationForLocalCat(uint32_t inReadState)
{
	if ((inReadState & ECRS_Pose) != 0)
	{
		//simulate pose only if we received new pose- might have just gotten thrustDir
		//in which case we don't need to replay moves because we haven't warped backwards

		//all processed moves have been removed, so all that are left are unprocessed moves
		//so we must apply them...
		const MoveList& moveList = InputManager::sInstance->GetMoveList();

		for (const Move& move : moveList)
		{
			float deltaTime = move.GetDeltaTime();
			ProcessInput(deltaTime, move.GetInputState());

			SimulateMovement(deltaTime);
		}
	}
}


void RoboCatClient::InterpolateClientSidePrediction(const Vector3& inOldLocation, const Vector3& inOldVelocity, bool inIsForRemoteCat)
{
	float roundTripTime = NetworkManagerClient::sInstance->GetRoundTripTime();

	if (!RoboMath::Is2DVectorEqual(inOldLocation, GetLocation()))
	{
		float time = Timing::sInstance.GetFrameStartTime();
		if (mTimeLocationBecameOutOfSync == 0.f)
		{
			mTimeLocationBecameOutOfSync = time;
		}

		float durationOutOfSync = time - mTimeLocationBecameOutOfSync;
		if (durationOutOfSync < roundTripTime)
		{
			SetLocation(Lerp(inOldLocation, GetLocation(), inIsForRemoteCat ? (durationOutOfSync / roundTripTime) : 0.1f));
		}
	}
	else
	{
		mTimeLocationBecameOutOfSync = 0.f;
	}

	if (!RoboMath::Is2DVectorEqual(inOldVelocity, GetVelocity()))
	{
		float time = Timing::sInstance.GetFrameStartTime();
		if (mTimeVelocityBecameOutOfSync == 0.f)
		{
			mTimeVelocityBecameOutOfSync = time;
		}

		float durationOutOfSync = time - mTimeVelocityBecameOutOfSync;
		if (durationOutOfSync < roundTripTime)
		{
			SetVelocity(Lerp(inOldVelocity, GetVelocity(), inIsForRemoteCat ? (durationOutOfSync / roundTripTime) : 0.1f));
		}
	}
	else
	{
		mTimeVelocityBecameOutOfSync = 0.f;
	}
}


//so what do we want to do here? need to do some kind of interpolation...

void RoboCatClient::DoClientSidePredictionAfterReplicationForRemoteCat(uint32_t inReadState)
{
	if ((inReadState & ECRS_Pose) != 0)
	{

		//simulate movement for an additional RTT
		float rtt = NetworkManagerClient::sInstance->GetRoundTripTime();
		//LOG( "Other cat came in, simulating for an extra %f", rtt );

		//let's break into framerate sized chunks though so that we don't run through walls and do crazy things...
		float deltaTime = 1.f / 30.f;

		while (true)
		{
			if (rtt < deltaTime)
			{
				SimulateMovement(rtt);
				break;
			}
			else
			{
				SimulateMovement(deltaTime);
				rtt -= deltaTime;
			}
		}
	}
}

