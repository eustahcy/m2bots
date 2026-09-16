#include <cassert>
#include "../linux-port/overlays/playerbot/src/game/src/playerbot_gear_urgency.h"

using namespace playerbot_gear_urgency;

int main()
{
	const unsigned int W = STRUGGLE_WINDOW_MS;

	// --- okno smierci --------------------------------------------------------
	TStruggleWindow w = { 0, 0 };
	w = NoteDeath(w, 1000, W);
	assert(w.deaths == 1 && w.startedAt == 1000);

	w = NoteDeath(w, 2000, W);
	w = NoteDeath(w, 3000, W);
	assert(w.deaths == 3 && w.startedAt == 1000);   // okno sie nie przesuwa
	assert(IsStruggling(w, 3000, W, STRUGGLE_DEATHS));

	// Dwie smierci to jeszcze nie klopoty.
	TStruggleWindow dwie = { 2, 1000 };
	assert(!IsStruggling(dwie, 2000, W, STRUGGLE_DEATHS));

	// Okno wygasa: bot ginacy raz na godzine nie jest bezradny.
	assert(!IsStruggling(w, 1000 + W + 1, W, STRUGGLE_DEATHS));
	TStruggleWindow po = NoteDeath(w, 1000 + W + 1, W);
	assert(po.deaths == 1 && po.startedAt == 1000 + W + 1);

	// Zegar w tyl (restart rdzenia) nie moze dac ujemnego czasu ani utrwalic
	// stanu "bezradny" na zawsze.
	TStruggleWindow wstecz = { 3, 500000 };
	assert(!IsStruggling(wstecz, 1000, W, STRUGGLE_DEATHS));
	TStruggleWindow naprawione = NoteDeath(wstecz, 1000, W);
	assert(naprawione.deaths == 1 && naprawione.startedAt == 1000);

	// --- decyzja -------------------------------------------------------------
	TContext c = { false, false, false, false };
	assert(Decide(c) == ANSWER_HUNT_ON);          // nic sie nie dzieje

	// Klopoty i jest co poprawic -> ekwipunek, a NIE slabsze potwory.
	// To jest cala rzecz, dla ktorej ten plik istnieje, wiec sprawdzana jest
	// kazda z trzech mozliwosci osobno.
	c.struggling = true;
	c.canRefine = true;
	assert(Decide(c) == ANSWER_FIX_GEAR);
	c.canRefine = false; c.canRerollBonus = true;
	assert(Decide(c) == ANSWER_FIX_GEAR);
	c.canRerollBonus = false; c.canBuyBetter = true;
	assert(Decide(c) == ANSWER_FIX_GEAR);

	// Dopiero gdy nie ma zadnego ruchu - schodzimy nizej.
	c.canBuyBetter = false;
	assert(Decide(c) == ANSWER_HUNT_SAFER);

	// Bez klopotow zadna z mozliwosci nie zatrzymuje polowania: bot, ktory ma
	// co ulepszyc, ma je ulepszac po drodze, a nie zamiast grac.
	c.struggling = false;
	c.canRefine = true; c.canRerollBonus = true; c.canBuyBetter = true;
	assert(Decide(c) == ANSWER_HUNT_ON);

	return 0;
}
