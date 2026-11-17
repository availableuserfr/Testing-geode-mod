#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

namespace
{
	// how many physics ticks to wait after a respawn before checking
	// (the game starts moving after 2, so 4 is a safe margin)
	constexpr int kWaitTicks = 4;

	// What the 3rd parameter of queueButton means.
	// true  = it is "isPlayer2"  (my best guess)
	// false = it is "isPlayer1"
	// If the mod stops pressing for you after this update, flip this value.
	constexpr bool kThirdParamIsPlayer2 = true;

	// held[button][0 = player 1, 1 = player 2]
	// buttons: 1 = jump, 2 = left, 3 = right (platformer)
	bool g_held[4][2] = {};

	// true while the game handles death/reset by itself,
	// so its internal button events aren't mistaken for you
	bool g_ignoreInput = false;

	// -1 = idle, otherwise ticks counted since the last respawn
	int g_ticksSinceRespawn = -1;

	// set once enough ticks have passed; handled at the end of the frame
	bool g_readyToCheck = false;

	void clearHeld()
	{
		for (auto& row : g_held) row[0] = row[1] = false;
	}

	void cancelCheck()
	{
		g_ticksSinceRespawn = -1;
		g_readyToCheck = false;
	}
}

class $modify(HoldGameLayer, GJBaseGameLayer)
{
	// queueButton is called the moment you press or release, even while you are dead,
	// so it always reflects what your finger / mouse / keyboard is really doing.
	// (handleButton only runs when the game processes the input, which can be
	//  delayed or skipped while dead, so releases were getting lost there.)
	void queueButton(int button, bool push, bool thirdParam)
	{
		if (!g_ignoreInput && button >= 1 && button <= 3)
		{
			int p = (thirdParam == kThirdParamIsPlayer2) ? 1 : 0;
			g_held[button][p] = push;
		}

		GJBaseGameLayer::queueButton(button, push, thirdParam);
	}
};

class $modify(HoldPlayerObject, PlayerObject)
{
	void update(float dt)
	{
		PlayerObject::update(dt);

		if (g_ticksSinceRespawn < 0) return;

		auto pl = PlayLayer::get();
		if (!pl || this != pl->m_player1) return;

		g_ticksSinceRespawn++;
		if (g_ticksSinceRespawn >= kWaitTicks)
		{
			g_ticksSinceRespawn = -1;
			g_readyToCheck = true;
		}
	}
};

class $modify(HoldPlayLayer, PlayLayer)
{
	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects)
	{
		clearHeld();
		cancelCheck();
		return PlayLayer::init(level, useReplay, dontCreateObjects);
	}

	void pauseGame(bool p0)
	{
		clearHeld(); // while paused we can't see you letting go
		cancelCheck();
		PlayLayer::pauseGame(p0);
	}

	void destroyPlayer(PlayerObject* player, GameObject* object)
	{
		g_ignoreInput = true;
		PlayLayer::destroyPlayer(player, object);
		g_ignoreInput = false;
	}

	void resetLevel()
	{
		g_ignoreInput = true;
		PlayLayer::resetLevel(); // respawns you, checkpoint included
		g_ignoreInput = false;

		cancelCheck();
		if (this->m_isPracticeMode) g_ticksSinceRespawn = 0;
	}

	void postUpdate(float dt)
	{
		PlayLayer::postUpdate(dt);

		if (!g_readyToCheck) return;
		g_readyToCheck = false;
		if (!this->m_isPracticeMode) return;

		// press only what you are holding but the game is NOT holding
		for (int b = 1; b <= 3; b++)
		{
			if (g_held[b][0] && this->m_player1 && !this->m_player1->m_holdingButtons[b])
				this->handleButton(true, b, true);

			if (g_held[b][1] && this->m_player2 && !this->m_player2->m_holdingButtons[b])
				this->handleButton(true, b, false);
		}
	}
};
