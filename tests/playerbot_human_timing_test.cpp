#include <cassert>
#include "../linux-port/overlays/playerbot/src/game/src/playerbot_human_timing.h"

using namespace playerbot_human_timing;

int main()
{
	// --- the rule this whole file exists to protect --------------------------
	// Never below the table value. Sending early makes the client cut the swing
	// that is playing, which is the bug the motion table was introduced to fix;
	// a jitter that could subtract would quietly bring it back. Swept across
	// every roll the callers can produce, for several bots.
	for (unsigned int playerID = 0; playerID < 40U; ++playerID)
		for (int jitter = 0; jitter <= SWING_JITTER_PERCENT_MAX; ++jitter)
			for (int miss = 1; miss <= SWING_MISS_ONE_IN; ++miss)
			{
				const unsigned int got = HumanSwingInterval(932U, playerID, jitter, miss, SWING_MISS_MAX_MS);
				assert(got >= 932U);
			}

	// Out-of-range rolls must clamp, not wrap into a shorter interval.
	assert(HumanSwingInterval(600U, 0U, -5, 2, 0) >= 600U);
	assert(HumanSwingInterval(600U, 0U, 99, 2, 0) <= 600U + (600U * SWING_JITTER_PERCENT_MAX) / 100U);

	// A zero base stays zero: the caller uses it to mean "no swing to pace".
	assert(HumanSwingInterval(0U, 7U, 3, 2, 300) == 0U);

	// --- the bot's own tempo is stable ---------------------------------------
	// Same bot, same rolls, same answer - twice running. A tempo that changed
	// per swing would be noise, not a person.
	assert(HumanSwingInterval(533U, 12U, 2, 5, 0) == HumanSwingInterval(533U, 12U, 2, 5, 0));

	// ...and different bots really do differ. playerID 0 sits on the floor,
	// playerID 9 carries the full spread.
	assert(HumanSwingInterval(1000U, 0U, 0, 5, 0) == 1000U);
	assert(HumanSwingInterval(1000U, 9U, 0, 5, 0) == 1090U);

	// --- the broken chain ----------------------------------------------------
	// missRoll 1 is the late click; anything else is not.
	const unsigned int clean = HumanSwingInterval(600U, 0U, 0, 2, 400);
	const unsigned int missed = HumanSwingInterval(600U, 0U, 0, 1, 400);
	assert(clean == 600U);
	assert(missed == 1000U);
	// The extra is clamped into the band even if the caller rolls outside it.
	assert(HumanSwingInterval(600U, 0U, 0, 1, 10) == 600U + SWING_MISS_MIN_MS);
	assert(HumanSwingInterval(600U, 0U, 0, 1, 99999) == 600U + SWING_MISS_MAX_MS);

	// --- reaction ------------------------------------------------------------
	// Attention already on the fight beats coming to it cold, whatever the bot.
	for (unsigned int playerID = 0; playerID < 20U; ++playerID)
	{
		const unsigned int engagedWorst = ReactionDelay(true, playerID, REACTION_ENGAGED_MAX_MS);
		const unsigned int coldBest = ReactionDelay(false, playerID, REACTION_COLD_MIN_MS);
		assert(engagedWorst < coldBest);
	}

	// Nobody reacts instantly, and nobody takes absurdly long.
	for (unsigned int playerID = 0; playerID < 20U; ++playerID)
	{
		assert(ReactionDelay(true, playerID, REACTION_ENGAGED_MIN_MS) >= (unsigned int)REACTION_ENGAGED_MIN_MS);
		const unsigned int ceiling = (unsigned int)REACTION_COLD_MAX_MS +
				(REACTION_OWN_SPREAD - 1U) * REACTION_OWN_STEP_MS;
		assert(ReactionDelay(false, playerID, REACTION_COLD_MAX_MS) <= ceiling);
	}

	// A roll outside the band clamps to it rather than escaping it.
	assert(ReactionDelay(true, 0U, -1000) == (unsigned int)REACTION_ENGAGED_MIN_MS);
	assert(ReactionDelay(true, 0U, 999999) == (unsigned int)REACTION_ENGAGED_MAX_MS);

	// The bands the caller rolls in are the ones ReactionDelay clamps to;
	// if these ever drift apart the roll silently stops mattering.
	assert(ReactionMinMs(true) == REACTION_ENGAGED_MIN_MS);
	assert(ReactionMaxMs(true) == REACTION_ENGAGED_MAX_MS);
	assert(ReactionMinMs(false) == REACTION_COLD_MIN_MS);
	assert(ReactionMaxMs(false) == REACTION_COLD_MAX_MS);

	return 0;
}
