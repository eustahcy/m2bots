#ifndef __INC_FISHING_H
#define __INC_FISHING_H

#include "item.h"

namespace fishing
{
	enum
	{
		CAMPFIRE_MOB = 12000,
		FISHER_MOB = 9009,
		FISH_MIND_PILL_VNUM = 27610,
	};

	EVENTINFO(fishing_event_info)
	{
		DWORD	pid;
		int		step;
		int		cooldown;

		BYTE	bar_speed;
		BYTE	bar_position;
		BYTE	bar_height;
		int		update_count;
		int		event_type;
		int		event_length;

		fishing_event_info()
			: pid(0)
			, step(0)
			, cooldown(0)
			, bar_speed(0)
			, bar_position(0)
			, bar_height(0)
			, update_count(0)
			, event_type(0)
			, event_length(0)
		{}
	};

	EVENTINFO(pre_fishing_event_info)
	{
		DWORD	pid;
		DWORD	step;

		pre_fishing_event_info()
			: pid(0), step(0)
		{}
	};

	extern LPITEM CheckRod(LPCHARACTER ch);
	extern LPEVENT CreatePreFishingEvent(LPCHARACTER ch);
	extern LPEVENT CreateFishingEvent(LPCHARACTER ch);
	extern void FishingPractice(LPCHARACTER ch, bool is_simulation = false);
	extern void Take(LPCHARACTER ch);
	extern void Take(fishing_event_info* info, LPCHARACTER ch);
	extern void UseFish(LPCHARACTER ch, LPITEM item);
	extern void Grill(LPCHARACTER ch, LPITEM item);

	extern bool RefinableRod(LPITEM rod);
	extern int RealRefineRod(LPCHARACTER ch, LPITEM rod);
}
#endif

// Files shared by GameCore.top
