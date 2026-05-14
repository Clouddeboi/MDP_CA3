class Server : public Engine
{
public:

	static bool StaticInit();

	virtual void DoFrame() override;

	virtual int Run();

	void HandleNewClient(ClientProxyPtr inClientProxy);
	void HandleLostClient(ClientProxyPtr inClientProxy);

	RoboCatPtr	GetCatForPlayer(int inPlayerId);
	void	SpawnCatForPlayer(int inPlayerId);

	void		HandleRoundWon(int inWinnerPlayerId);

private:
	Server();

	bool	InitNetworkManager();
	void	SetupWorld();
	void	RespawnPickupsIfNeeded();
	void	BroadcastRoundOver(const string& inWinnerName);
	void	ResetRound();

	float	mPickupRespawnTimer;
	float	mHeartbeatTimer;

	bool	mRoundOver;
	float	mRoundOverTimer;
	static const float kRoundOverDuration;

};

