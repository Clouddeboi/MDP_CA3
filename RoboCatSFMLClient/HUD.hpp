//I take care of rendering things!

class HUD
{
public:

	static void StaticInit();
	static std::unique_ptr< HUD >	sInstance;

	void Render();

	void StartRespawnCountdown(float inDuration);

private:

	HUD();

	void	RenderBandWidth();
	void	RenderRoundTripTime();
	void	RenderScoreBoard();
	void	RenderText(const string& inStr, const Vector3& origin, const Vector3& inColor);
	void	RenderDeathScreen();

	Vector3										mBandwidthOrigin;
	Vector3										mRoundTripTimeOrigin;
	Vector3										mScoreBoardOrigin;
	Vector3										mScoreOffset;
	int											mHealth;

	float	mRespawnTimeRemaining;
	float	mRespawnDuration;
};



