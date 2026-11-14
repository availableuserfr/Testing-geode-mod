#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace
{
	// held[button][0 = player 1, 1 = player 2]
	// buttons: 1 = jump, 2 = left, 3 = right (platformer)
	bool g_held[4][2] = {};

	// buttons the game itself pressed since the last respawn
	bool g_pressedSinceReset[4][2] = {};

	// true while the game is resetting the level by itself
	bool g_ignoreInput = false;

	// set on respawn, handled once on the next frame
	bool g_replayPending = false;

	void clearHeld()
	{
		for (auto& row : g_held) row[0] = row[1] = false;
	}

	void clearPressed()
	{
		for (auto& row : g_pressedSinceReset) row[0] = row[1] = false;
	}
}

class $modify(HoldGameLayer, GJBaseGameLayer)
{
	void handleButton(bool down, int button, bool isPlayer1)
	{
		if (!g_ignoreInput && button >= 1 && button <= 3)
		{
			int p = isPlayer1 ? 0 : 1;
			g_held[button][p] = down;
			if (down) g_pressedSinceReset[button][p] = true;
		}

		GJBaseGameLayer::handleButton(down, button, isPlayer1);
	}
};

class $modify(HoldPlayLayer, PlayLayer)
{
	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects)
	{
		clearHeld();
		clearPressed();
		g_replayPending = false;
		return PlayLayer::init(level, useReplay, dontCreateObjects);
	}

	void pauseGame(bool p0)
	{
		clearHeld(); // while paused we can't see you letting go
		g_replayPending = false;
		PlayLayer::pauseGame(p0);
	}

	void resetLevel()
	{
		g_ignoreInput = true;
		PlayLayer::resetLevel(); // respawns you, checkpoint included
		g_ignoreInput = false;

		clearPressed();
		g_replayPending = this->m_isPracticeMode;
	}

	void postUpdate(float dt)
	{
		PlayLayer::postUpdate(dt);

		if (!g_replayPending) return;
		g_replayPending = false;
		if (!this->m_isPracticeMode) return;

		// press what is held, but only if the game didn't already register it
		for (int b = 1; b <= 3; b++)
		{
			for (int p = 0; p < 2; p++)
			{
				if (g_held[b][p] && !g_pressedSinceReset[b][p])
					this->handleButton(true, b, p == 0);
			}
		}
	}
};
