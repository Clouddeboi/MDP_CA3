#include "RoboCatClientPCH.hpp"

std::unique_ptr< HUD >	HUD::sInstance;

namespace
{
	const float kClientRespawnDelay = 3.f;
}

HUD::HUD() :
	mScoreBoardOrigin(50.f, 60.f, 0.0f),
	mBandwidthOrigin(50.f, 10.f, 0.0f),
	mRoundTripTimeOrigin(580.f, 10.f, 0.0f),
	mScoreOffset(0.f, 32.f, 0.0f),
	mRespawnTimeRemaining(0.f),
	mRespawnDuration(0.f),
	mRoundOverTimeRemaining(0.f)
{
}


void HUD::StaticInit()
{
	sInstance.reset(new HUD());
}

void HUD::StartRespawnCountdown(float inDuration)
{
	mRespawnDuration = inDuration;
	mRespawnTimeRemaining = inDuration;
}

void HUD::StartRoundOverScreen(const string& inWinnerName, float inRemainingTime)
{
	mRoundOverWinnerName = inWinnerName;

	//Only accept the server's value if it's lower than what we currently have,
	//or if the screen isn't showing yet, prevents duplicate packets resetting the timer
	if (mRoundOverTimeRemaining <= 0.f || inRemainingTime < mRoundOverTimeRemaining)
		mRoundOverTimeRemaining = inRemainingTime;

	//Suppress the death screen for the duration of the round over overlay
	mRespawnTimeRemaining = 0.f;
}

void HUD::Render()
{
	//Decrement respawn timer
	if (mRespawnTimeRemaining > 0.f)
	{
		mRespawnTimeRemaining -= Timing::sInstance.GetDeltaTime();
		if (mRespawnTimeRemaining < 0.f)
			mRespawnTimeRemaining = 0.f;
	}

	//Decrement round-over timer locally every frame
	if (mRoundOverTimeRemaining > 0.f)
	{
		mRoundOverTimeRemaining -= Timing::sInstance.GetDeltaTime();
		if (mRoundOverTimeRemaining < 0.f)
			mRoundOverTimeRemaining = 0.f;
	}

	RenderScoreBoard();

	if (mRoundOverTimeRemaining > 0.f)
		RenderRoundOverScreen();
	else if (mRespawnTimeRemaining > 0.f)
		RenderDeathScreen();
}

void HUD::RenderRoundOverScreen()
{
	// Dark full-screen overlay
	sf::RectangleShape overlay;
	overlay.setSize(sf::Vector2f(1280.f, 720.f));
	overlay.setPosition(0.f, 0.f);
	overlay.setFillColor(sf::Color(0, 0, 0, 180));
	WindowManager::sInstance->draw(overlay);

	sf::Font* font = FontManager::sInstance->GetFont("carlito").get();
	if (!font) return;

	//Winner title — "(name) Wins!"
	{
		sf::Text title;
		title.setString(mRoundOverWinnerName + " Wins!");
		title.setFont(*font);
		title.setCharacterSize(60);
		title.setFillColor(sf::Color(255, 215, 0, 255));
		title.setOutlineColor(sf::Color(0, 0, 0, 220));
		title.setOutlineThickness(3.f);
		sf::FloatRect b = title.getLocalBounds();
		title.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
		title.setPosition(640.f, 280.f);
		WindowManager::sInstance->draw(title);
	}

	//Sub-title
	{
		sf::Text sub;
		sub.setString("Starting new round...");
		sub.setFont(*font);
		sub.setCharacterSize(32);
		sub.setFillColor(sf::Color(220, 220, 220, 230));
		sf::FloatRect b = sub.getLocalBounds();
		sub.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
		sub.setPosition(640.f, 360.f);
		WindowManager::sInstance->draw(sub);
	}

	//Countdown
	{
		int seconds = static_cast<int>(std::ceil(mRoundOverTimeRemaining));
		char buf[32];
		snprintf(buf, sizeof(buf), "%d", seconds);

		sf::Text countdown;
		countdown.setString(buf);
		countdown.setFont(*font);
		countdown.setCharacterSize(80);
		countdown.setFillColor(sf::Color(255, 255, 255, 230));
		countdown.setOutlineColor(sf::Color(0, 0, 0, 200));
		countdown.setOutlineThickness(4.f);
		sf::FloatRect b = countdown.getLocalBounds();
		countdown.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);
		countdown.setPosition(640.f, 450.f);
		WindowManager::sInstance->draw(countdown);
	}
}

void HUD::RenderDeathScreen()
{
	sf::RectangleShape overlay;
	overlay.setSize(sf::Vector2f(1280.f, 720.f));
	overlay.setPosition(0.f, 0.f);
	overlay.setFillColor(sf::Color(0, 0, 0, 160));
	WindowManager::sInstance->draw(overlay);

	{
		sf::Text title;
		title.setString("You were eaten!");
		title.setFont(*FontManager::sInstance->GetFont("carlito"));
		title.setCharacterSize(52);
		title.setFillColor(sf::Color(220, 60, 60, 255));
		sf::FloatRect bounds = title.getLocalBounds();
		title.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
		title.setPosition(640.f, 300.f);
		WindowManager::sInstance->draw(title);
	}

	{
		int seconds = static_cast<int>(std::ceil(mRespawnTimeRemaining));
		char buf[64];
		snprintf(buf, 64, "Respawning in %d...", seconds);

		sf::Text countdown;
		countdown.setString(buf);
		countdown.setFont(*FontManager::sInstance->GetFont("carlito"));
		countdown.setCharacterSize(34);
		countdown.setFillColor(sf::Color(255, 255, 255, 220));
		sf::FloatRect bounds = countdown.getLocalBounds();
		countdown.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
		countdown.setPosition(640.f, 380.f);
		WindowManager::sInstance->draw(countdown);
	}
}

void HUD::RenderBandWidth()
{
	string bandwidth = StringUtils::Sprintf("In %d  Bps, Out %d Bps",
		static_cast<int>(NetworkManagerClient::sInstance->GetBytesReceivedPerSecond().GetValue()),
		static_cast<int>(NetworkManagerClient::sInstance->GetBytesSentPerSecond().GetValue()));
	RenderText(bandwidth, mBandwidthOrigin, Colors::White);
}

void HUD::RenderRoundTripTime()
{
	float rttMS = NetworkManagerClient::sInstance->GetAvgRoundTripTime().GetValue() * 1000.f;

	string roundTripTime = StringUtils::Sprintf("RTT %d ms", (int)rttMS);
	RenderText(roundTripTime, mRoundTripTimeOrigin, Colors::White);
}

void HUD::RenderScoreBoard()
{
	//Copy entries and sort largest size first
	vector< ScoreBoardManager::Entry > sorted = ScoreBoardManager::sInstance->GetEntries();
	std::sort(sorted.begin(), sorted.end(),
		[](const ScoreBoardManager::Entry& a, const ScoreBoardManager::Entry& b)
		{
			return a.GetSize() > b.GetSize();
		});

	const float rowHeight = mScoreOffset.mY;
	const float panelPadX = 10.f;
	const float panelPadY = 6.f;
	const float panelWidth = 260.f;

	//Draw a semi-transparent background panel behind the whole scoreboard
	sf::RectangleShape panel;
	panel.setSize(sf::Vector2f(panelWidth, rowHeight * sorted.size() + panelPadY * 2.f));
	panel.setPosition(mScoreBoardOrigin.mX - panelPadX, mScoreBoardOrigin.mY - panelPadY);
	panel.setFillColor(sf::Color(0, 0, 0, 160));
	WindowManager::sInstance->draw(panel);

	Vector3 offset = mScoreBoardOrigin;
	int rank = 1;

	for (const auto& entry : sorted)
	{
		//Build ranked labels
		char buffer[256];
		snprintf(buffer, 256, "%d. %s %.2f m", rank, entry.GetPlayerName().c_str(), entry.GetSize());
		string label(buffer);

		RenderText(label, offset, entry.GetColor());

		offset.mY += rowHeight;
		++rank;
	}
}

void HUD::RenderText(const string& inStr, const Vector3& origin, const Vector3& inColor)
{
	sf::Text text;
	text.setString(inStr);
	text.setFillColor(sf::Color(inColor.mX, inColor.mY, inColor.mZ, 255));
	text.setCharacterSize(22);
	text.setPosition(origin.mX, origin.mY);
	text.setFont(*FontManager::sInstance->GetFont("carlito"));
	WindowManager::sInstance->draw(text);
}