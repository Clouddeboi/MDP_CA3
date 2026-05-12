class RoboCat : public GameObject
{
public:
	CLASS_IDENTIFICATION('RCAT', GameObject)

	enum ECatReplicationState
	{
		ECRS_Pose = 1 << 0,
		ECRS_Color = 1 << 1,
		ECRS_PlayerId = 1 << 2,
		ECRS_Size = 1 << 3,

		ECRS_AllState = ECRS_Pose | ECRS_Color | ECRS_PlayerId | ECRS_Size
	};


	static	GameObject* StaticCreate() { return new RoboCat(); }

	virtual uint32_t GetAllStateMask()	const override { return ECRS_AllState; }

	virtual	RoboCat* GetAsCat() override { return this; }

	virtual bool HandleCollisionWithCat(RoboCat* inCat) override { (void)inCat; return false; }

	virtual void Update() override;

	void ProcessInput(float inDeltaTime, const InputState& inInputState);
	void SimulateMovement(float inDeltaTime);

	void ProcessCollisions();
	void ProcessCollisionsWithScreenWalls();

	void		SetPlayerId(uint32_t inPlayerId) { mPlayerId = inPlayerId; }
	uint32_t	GetPlayerId()						const { return mPlayerId; }

	void			SetVelocity(const Vector3& inVelocity) { mVelocity = inVelocity; }
	const Vector3& GetVelocity()						const { return mVelocity; }

	void	SetSize(float inSize);
	float	GetSize() const { return mSize; }

	virtual uint32_t	Write(OutputMemoryBitStream& inOutputStream, uint32_t inDirtyState) const override;

protected:
	RoboCat();

private:


	void	AdjustVelocityByInput(float inDeltaTime);

	Vector3				mVelocity;


	float				mMaxLinearSpeed;

	//bounce fraction when hitting various things
	float				mWallRestitution;
	float				mCatRestitution;


	uint32_t			mPlayerId;

	float		mSize;

protected:

	///move down here for padding reasons...

	float				mLastMoveTimestamp;

	float				mHorizontalDir;
	float				mVerticalDir;
};

typedef shared_ptr< RoboCat >	RoboCatPtr;

