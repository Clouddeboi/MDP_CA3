#include "RoboCatPCH.hpp"

const float WORLD_HEIGHT = 5000.f;
const float WORLD_WIDTH = 5000.f;

RoboCat::RoboCat() :
	GameObject(),
	mMaxLinearSpeed(300.f),
	mVelocity(Vector3::Zero),
	mWallRestitution(0.1f),
	mCatRestitution(0.1f),
	mHorizontalDir(0.f),
	mVerticalDir(0.f),
	mPlayerId(0),
	mSize(1.0f),
	mIsDashing(false)
{
	SetCollisionRadius(60.f);
}

void RoboCat::ProcessInput(float inDeltaTime, const InputState& inInputState)
{
	//No rotation, direct 8-directional movement
	mHorizontalDir = inInputState.GetDesiredHorizontalDelta();
	mVerticalDir = inInputState.GetDesiredVerticalDelta();

	mIsDashing = inInputState.IsDashing() && (mSize > kMinSize);

	(void)inDeltaTime;
}

void RoboCat::AdjustVelocityByInput(float inDeltaTime)
{
	//Dash multiplies top speed, only allowed when above minimum size
	const float kDashMultiplier = 2.2f;
	float effectiveSpeed = mMaxLinearSpeed;
	if (mIsDashing && mSize > kMinSize)
	{
		effectiveSpeed *= kDashMultiplier;
	}

	//Target velocity from input axes, uses effectiveSpeed, not mMaxLinearSpeed
	float targetVX = mHorizontalDir * effectiveSpeed;
	float targetVY = -mVerticalDir * effectiveSpeed;

	//Acceleration when input is held, deceleration when released
	float accel = 1800.f;
	float decel = 1200.f;

	float lerpX = (targetVX != 0.f) ? accel : decel;
	float lerpY = (targetVY != 0.f) ? accel : decel;

	float alpha = inDeltaTime;

	//Normalize lerp against effectiveSpeed so acceleration feel stays consistent
	mVelocity.mX += (targetVX - mVelocity.mX) * (lerpX * alpha / effectiveSpeed);
	mVelocity.mY += (targetVY - mVelocity.mY) * (lerpY * alpha / effectiveSpeed);
}

void RoboCat::SimulateMovement(float inDeltaTime)
{
	//simulate us...
	AdjustVelocityByInput(inDeltaTime);
	
	SetLocation(GetLocation() + mVelocity * inDeltaTime);

	ProcessCollisions();
}

void RoboCat::Update()
{

}

void RoboCat::ProcessCollisions()
{
	//right now just bounce off the sides..
	ProcessCollisionsWithScreenWalls();

	float sourceRadius = GetCollisionRadius();
	Vector3 sourceLocation = GetLocation();

	//now let's iterate through the world and see what we hit...
	//note: since there's a small number of objects in our game, this is fine.
	//but in a real game, brute-force checking collisions against every other object is not efficient.
	//it would be preferable to use a quad tree or some other structure to minimize the
	//number of collisions that need to be tested.
	for (auto goIt = World::sInstance->GetGameObjects().begin(), end = World::sInstance->GetGameObjects().end(); goIt != end; ++goIt)
	{
		GameObject* target = goIt->get();
		if (target != this && !target->DoesWantToDie())
		{
			//simple collision test for spheres- are the radii summed less than the distance?
			Vector3 targetLocation = target->GetLocation();
			float targetRadius = target->GetCollisionRadius();

			Vector3 delta = targetLocation - sourceLocation;
			float distSq = delta.LengthSq2D();
			float collisionDist = (sourceRadius + targetRadius);
			if (distSq < (collisionDist * collisionDist))
			{
				//first, tell the other guy there was a collision with a cat, so it can do something...

				if (target->HandleCollisionWithCat(this))
				{
					//okay, you hit something!
					//so, project your location far enough that you're not colliding
					Vector3 dirToTarget = delta;
					dirToTarget.Normalize2D();
					Vector3 acceptableDeltaFromSourceToTarget = dirToTarget * collisionDist;
					//important note- we only move this cat. the other cat can take care of moving itself
					SetLocation(targetLocation - acceptableDeltaFromSourceToTarget);


					Vector3 relVel = mVelocity;

					//if other object is a cat, it might have velocity, so there might be relative velocity...
					RoboCat* targetCat = target->GetAsCat();
					if (targetCat)
					{
						relVel -= targetCat->mVelocity;
					}

					//got vel with dir between objects to figure out if they're moving towards each other
					//and if so, the magnitude of the impulse ( since they're both just balls )
					float relVelDotDir = Dot2D(relVel, dirToTarget);

					if (relVelDotDir > 0.f)
					{
						Vector3 impulse = relVelDotDir * dirToTarget;

						if (targetCat)
						{
							mVelocity -= impulse;
							mVelocity *= mCatRestitution;
						}
						else
						{
							mVelocity -= impulse * 2.f;
							mVelocity *= mWallRestitution;
						}

					}
				}
			}
		}
	}

}

void RoboCat::ProcessCollisionsWithScreenWalls()
{
	Vector3 location = GetLocation();
	float x = location.mX;
	float y = location.mY;

	float vx = mVelocity.mX;
	float vy = mVelocity.mY;

	float radius = GetCollisionRadius();

	if ((y + radius) >= WORLD_HEIGHT && vy > 0)
	{
		mVelocity.mY = 0.f;
		location.mY = WORLD_HEIGHT - radius;
		SetLocation(location);
	}
	else if (y - radius <= 0 && vy < 0)
	{
		mVelocity.mY = 0.f;
		location.mY = radius;
		SetLocation(location);
	}

	if ((x + radius) >= WORLD_WIDTH && vx > 0)
	{
		mVelocity.mX = 0.f;
		location.mX = WORLD_WIDTH - radius;
		SetLocation(location);
	}
	else if (x - radius <= 0 && vx < 0)
	{
		mVelocity.mX = 0.f;
		location.mX = radius;
		SetLocation(location);
	}
}

uint32_t RoboCat::Write(OutputMemoryBitStream& inOutputStream, uint32_t inDirtyState) const
{
	uint32_t writtenState = 0;

	if (inDirtyState & ECRS_PlayerId)
	{
		inOutputStream.Write((bool)true);
		inOutputStream.Write(GetPlayerId());
		writtenState |= ECRS_PlayerId;
	}
	else
	{
		inOutputStream.Write((bool)false);
	}

	if (inDirtyState & ECRS_Pose)
	{
		inOutputStream.Write((bool)true);

		Vector3 velocity = mVelocity;
		inOutputStream.Write(velocity.mX);
		inOutputStream.Write(velocity.mY);

		Vector3 location = GetLocation();
		inOutputStream.Write(location.mX);
		inOutputStream.Write(location.mY);

		writtenState |= ECRS_Pose;
	}
	else
	{
		inOutputStream.Write((bool)false);
	}

	//Always write movement dirs for remote cat interpolation (2 bits each)
	auto WriteDir = [&](float dir)
		{
			bool isNonZero = (dir != 0.f);
			inOutputStream.Write(isNonZero);
			if (isNonZero) inOutputStream.Write(dir > 0.f);
		};
	WriteDir(mHorizontalDir);
	WriteDir(mVerticalDir);

	if (inDirtyState & ECRS_Color)
	{
		inOutputStream.Write((bool)true);
		inOutputStream.Write(GetColor());
		writtenState |= ECRS_Color;
	}
	else
	{
		inOutputStream.Write((bool)false);
	}

	if (inDirtyState & ECRS_Size)
	{
		inOutputStream.Write((bool)true);
		inOutputStream.Write(mSize);
		writtenState |= ECRS_Size;
	}
	else
	{
		inOutputStream.Write((bool)false);
	}

	return writtenState;
}

void RoboCat::SetSize(float inSize)
{
	mSize = inSize;
	SetCollisionRadius(60.f * mSize);   //base radius 60 scaled by size
	SetScale(mSize);                    //SpriteComponent reads GetScale() every frame
}