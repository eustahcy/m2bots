#ifndef __INC_METIN2_PLAYERBOT_HUMAN_TIMING_H__
#define __INC_METIN2_PLAYERBOT_HUMAN_TIMING_H__

// The difference between a correct swing and a human one.
//
// playerbot_swing_timing.h says how long a swing *may* take: the earliest
// millisecond the client will accept the next combo input, read out of the
// motion data. That made the bots mechanically right and, at the same time,
// unmistakable - every warrior with a one-handed sword hit at 533, 543, 418,
// 1058 ms, forever, and so did every other warrior with a one-handed sword.
// Plot the gaps between blows and a bot is a straight line where a person is
// a cloud.
//
// This file adds the cloud. Pure policy in the sense playerbot_world_rules.h
// established: no CHARACTER, no server singletons, no RNG of its own - the
// caller rolls and passes the roll in. That is what lets
// tests/playerbot_human_timing_test.cpp check it without booting a game core.
//
// THE ONE RULE THAT MATTERS HERE: jitter is only ever ADDED.
//
// The table value is a floor, not an average. Sending an attack before it
// means the client cannot chain and cuts the swing that is playing - that is
// the exact bug the old flat 480 ms caused, where bots never finished a
// strike. So a "human" early click must not become a shorter interval. It is
// modelled the way the client already treats it: the input is swallowed, the
// chain breaks, and the next blow lands *later*. Late is realistic and safe;
// early is neither.
namespace playerbot_human_timing
{
	// --- swing jitter --------------------------------------------------------

	// Each bot's own hands, as a percentage added to every swing it makes.
	// Constant for the life of the character, so one bot is consistently brisk
	// and another consistently draggy - people differ from each other more
	// than they differ from themselves.
	const unsigned int TEMPO_SPREAD_PERCENT = 10;   // 0..9%

	// The wobble within one bot, rolled per swing.
	const int SWING_JITTER_PERCENT_MAX = 6;         // 0..6%

	// How often the player is late enough to break the chain, and by how much.
	// One swing in forty is roughly twice a minute at a sword's cadence.
	const int SWING_MISS_ONE_IN = 40;
	const int SWING_MISS_MIN_MS = 150;
	const int SWING_MISS_MAX_MS = 500;

	// baseMs      the interval from the motion table, already scaled for attack
	//             speed and floored - the earliest legal moment.
	// playerID    picks this bot's own tempo; any stable id will do.
	// jitterRoll  0..SWING_JITTER_PERCENT_MAX
	// missRoll    1..SWING_MISS_ONE_IN; 1 means the chain broke
	// missMs      SWING_MISS_MIN_MS..SWING_MISS_MAX_MS
	//
	// Returns a value that is never below baseMs.
	inline unsigned int HumanSwingInterval(unsigned int baseMs, unsigned int playerID,
			int jitterRoll, int missRoll, int missMs)
	{
		if (baseMs == 0)
			return 0;
		if (jitterRoll < 0)
			jitterRoll = 0;
		if (jitterRoll > SWING_JITTER_PERCENT_MAX)
			jitterRoll = SWING_JITTER_PERCENT_MAX;

		const unsigned int ownTempo = (baseMs * (playerID % TEMPO_SPREAD_PERCENT)) / 100U;
		const unsigned int wobble = (baseMs * (unsigned int)jitterRoll) / 100U;
		unsigned int interval = baseMs + ownTempo + wobble;

		if (missRoll == 1)
		{
			if (missMs < SWING_MISS_MIN_MS)
				missMs = SWING_MISS_MIN_MS;
			if (missMs > SWING_MISS_MAX_MS)
				missMs = SWING_MISS_MAX_MS;
			interval += (unsigned int)missMs;
		}
		return interval;
	}

	// --- reaction ------------------------------------------------------------

	// A bot used to strike in the same tick it decided to. Nobody does that:
	// there is an eye, a decision and a hand between a monster appearing and a
	// blow landing. How long depends on whether the bot was already looking -
	// someone mid-fight whose target just died turns on the next one far faster
	// than someone who has been walking through empty field.
	const int REACTION_ENGAGED_MIN_MS = 120;
	const int REACTION_ENGAGED_MAX_MS = 260;
	const int REACTION_COLD_MIN_MS = 280;
	const int REACTION_COLD_MAX_MS = 650;

	// Some people are simply slower, every time. Same idea as the swing tempo
	// and deliberately the same direction: added, never subtracted.
	const unsigned int REACTION_OWN_STEP_MS = 20;
	const unsigned int REACTION_OWN_SPREAD = 7;     // 0..6 steps, so 0..120 ms

	// How long after a combat action a bot still counts as paying attention.
	const unsigned int REACTION_ENGAGED_WINDOW_MS = 3000;

	inline int ReactionMinMs(bool engaged)
	{
		return engaged ? REACTION_ENGAGED_MIN_MS : REACTION_COLD_MIN_MS;
	}

	inline int ReactionMaxMs(bool engaged)
	{
		return engaged ? REACTION_ENGAGED_MAX_MS : REACTION_COLD_MAX_MS;
	}

	// engaged   the bot acted in combat recently - attention already there
	// playerID  picks this bot's own slowness
	// roll      a roll inside ReactionMinMs..ReactionMaxMs for the same `engaged`
	inline unsigned int ReactionDelay(bool engaged, unsigned int playerID, int roll)
	{
		const int low = ReactionMinMs(engaged);
		const int high = ReactionMaxMs(engaged);
		if (roll < low)
			roll = low;
		if (roll > high)
			roll = high;
		const unsigned int own = (playerID % REACTION_OWN_SPREAD) * REACTION_OWN_STEP_MS;
		return (unsigned int)roll + own;
	}
}

#endif
