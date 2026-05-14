//I take care of rendering things!

class RenderManager
{
public:

	static void StaticInit();
	static std::unique_ptr< RenderManager >	sInstance;

	void Render();
	void RenderComponents();
	void UpdateCamera(const Vector3& playerPos, float playerSize);

	void AdjustTargetZoom(float inDelta);

	//vert inefficient method of tracking scene graph...
	void AddComponent(SpriteComponent* inComponent);
	void RemoveComponent(SpriteComponent* inComponent);
	int	 GetComponentIndex(SpriteComponent* inComponent) const;

private:

	RenderManager();
	void RenderBackground();

	//this can't be only place that holds on to component- it has to live inside a GameObject in the world
	vector< SpriteComponent* >		mComponents;

	sf::View	mGameView;    //follows the player, moves and zooms
	sf::View	mHUDView;     //fixed to screen, never moves

	sf::Texture     mBackgroundTexture;
	sf::Sprite      mBackgroundSprite;

	sf::CircleShape	mDarknessOverlay;
	sf::CircleShape	mBorderCircle; 

	float	mTargetZoomOffset;
	float	mCurrentZoomOffset;
};


