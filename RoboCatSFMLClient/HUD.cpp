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
	mRespawnDuration(0.f)
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

void HUD::Render()
{
	if (mRespawnTimeRemaining > 0.f)
	{
		mRespawnTimeRemaining -= Timing::sInstance.GetDeltaTime();
		if (mRespawnTimeRemaining < 0.f)
			mRespawnTimeRemaining = 0.f;
	}
	//RenderBandWidth();
	//RenderRoundTripTime();
	RenderScoreBoard();

	if (mRespawnTimeRemaining > 0.f)
	{
		RenderDeathScreen();
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