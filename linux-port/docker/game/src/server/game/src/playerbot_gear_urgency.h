#ifndef __INC_METIN2_PLAYERBOT_GEAR_URGENCY_H__
#define __INC_METIN2_PLAYERBOT_GEAR_URGENCY_H__

// Co robi bot, ktoremu przestalo isc.
//
// Do tej pory odpowiedz byla jedna: po smierci, przez minute, obnizyc gorny
// poziom celu do "moj poziom minus jeden" i polowac na cos slabszego
// (playerbot_targeting.h, komentarz przy bRecentDeath). Dla pojedynczej
// wpadki to rozsadne. Dla bota, ktory ginie trzeci raz w ciagu dziesieciu
// minut, to zla odpowiedz: przyczyna nie jest w tym, ze potwory sa za silne,
// tylko w tym, ze jego ekwipunek zostal w tyle - a schodzenie nizej utrwala
// ten stan, bo slabsze potwory daja mniej i na lepszy sprzet nie zarobi.
//
// Gracz w tej sytuacji idzie do kowala, przerzuca bony albo kupuje bron.
// Ta regula mowi to samo: dopoki jest co poprawic - poprawiaj. Schodzenie na
// slabsze potwory zostaje, ale jako OSTATECZNOSC, gdy nie ma juz zadnego
// ruchu do wykonania (brak zlota, nic do ulepszenia, nic do przerzucenia).
//
// Czysta polityka w sensie, ktory ustanowil playerbot_world_rules.h: zadnego
// CHARACTER, zadnych singletonow, zwykle typy. Testowana bez rdzenia gry
// przez tests/playerbot_gear_urgency_test.cpp.
namespace playerbot_gear_urgency
{
	// Okno, w ktorym liczymy smierci, i ile ich znaczy klopoty. Dziesiec minut
	// i trzy smierci: jedna wpadka to pech, dwie to moze zly pull, trzy w tym
	// samym oknie to juz nie przypadek.
	const unsigned int STRUGGLE_WINDOW_MS = 600000;
	const int STRUGGLE_DEATHS = 3;

	// Licznik smierci w oknie. Osobny od bDeathCount, ktory jest licznikiem
	// dozywotnim (BYTE, zawija sie po 255) i sluzy do czego innego.
	struct TStruggleWindow
	{
		int deaths;
		unsigned int startedAt;
	};

	// Stan okna po kolejnej smierci. Okno starsze niz windowMs zaczyna sie od
	// nowa - inaczej bot, ktory zginal raz na godzine przez caly dzien,
	// zostalby uznany za bezradnego.
	inline TStruggleWindow NoteDeath(TStruggleWindow window, unsigned int now,
			unsigned int windowMs)
	{
		TStruggleWindow out;
		const bool expired = window.deaths <= 0 || window.startedAt == 0 ||
				now < window.startedAt ||             // zegar poszedl wstecz
				now - window.startedAt > windowMs;
		out.deaths = expired ? 1 : window.deaths + 1;
		out.startedAt = expired ? now : window.startedAt;
		return out;
	}

	inline bool IsStruggling(const TStruggleWindow& window, unsigned int now,
			unsigned int windowMs, int threshold)
	{
		if (window.deaths < threshold || window.startedAt == 0)
			return false;
		if (now < window.startedAt)
			return false;
		return now - window.startedAt <= windowMs;
	}

	enum EAnswer
	{
		ANSWER_HUNT_ON = 0,   // nic sie nie dzieje, poluj normalnie
		ANSWER_FIX_GEAR,      // przestan polowac, zajmij sie ekwipunkiem
		ANSWER_HUNT_SAFER     // nie ma czego poprawic - dopiero teraz schodz nizej
	};

	struct TContext
	{
		bool struggling;
		// Trzy ruchy, ktore bot moze wykonac ze swoim ekwipunkiem. Kazdy jest
		// pytaniem, na ktore reszta AI juz umie odpowiedziec.
		bool canRefine;       // jest co ulepszyc u kowala
		bool canRerollBonus;  // stac go na kamien i ma na czym go uzyc
		bool canBuyBetter;    // stac go na lepsza czesc u kupca
	};

	inline EAnswer Decide(const TContext& context)
	{
		if (!context.struggling)
			return ANSWER_HUNT_ON;
		if (context.canRefine || context.canRerollBonus || context.canBuyBetter)
			return ANSWER_FIX_GEAR;
		return ANSWER_HUNT_SAFER;
	}
}

#endif
