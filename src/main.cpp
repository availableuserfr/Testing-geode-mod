#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace
{
	// held[button][0 = player 1, 1 = player 2]
	// buttons: 1 = jump, 2 = left, 3 = right (platformer)
	bool g_held[4][2] = {};

	// true while the game handles death/reset by itself,
	// so its internal button releases aren't mistaken for you letting go
	bool g_ignoreInput = false;

	// set on respawn, handled once on the next frame
	bool g_replayPending = false;

	void clearHeld()
	{
		for (auto& row : g_held) row[0] = row[1] = false;
	}
}

class $modify(HoldGameLayer, GJBaseGameLayer)
{
	void handleButton(bool down, int button, bool isPlayer1)
	{
		if (!g_ignoreInput && button >= 1 && button <= 3)
			g_held[button][isPlayer1 ? 0 : 1] = down;

		GJBaseGameLayer::handleButton(down, button, isPlayer1);
	}
};

class $modify(HoldPlayLayer, PlayLayer)
{
	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects)
	{
		clearHeld();
		g_replayPending = false;
		return PlayLayer::init(level, useReplay, dontCreateObjects);
	}

	void pauseGame(bool p0)
	{
		clearHeld(); // while paused we can't see you letting go
		g_replayPending = false;
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

		g_replayPending = this->m_isPracticeMode;
	}

	void postUpdate(float dt)
	{
		PlayLayer::postUpdate(dt);

		if (!g_replayPending) return;
		g_replayPending = false;
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
