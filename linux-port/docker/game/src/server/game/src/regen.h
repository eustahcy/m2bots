#include "dungeon.h"

enum
{
	REGEN_TYPE_MOB,
	REGEN_TYPE_GROUP,
	REGEN_TYPE_EXCEPTION,
	REGEN_TYPE_GROUP_GROUP,
	REGEN_TYPE_ANYWHERE,
	REGEN_TYPE_MAX_NUM
};

typedef struct regen
{
	LPREGEN	prev, next;
	long	lMapIndex;
	int		type;
	int		sx, sy, ex, ey;
	BYTE	z_section;

	BYTE	direction;

	DWORD	time;
	DWORD	endTime;

	int		max_count;
	int		count;
	int 	vnum;

	bool	is_aggressive;

	bool	is_high_traffic_hours;
	bool	is_first_spawn_delayed;

	bool	is_boss_or_stone;

	LPEVENT	event;

	size_t id; // to help dungeon regen identification

	regen() :
		prev(NULL), next(NULL),
		lMapIndex(0),
		type(0),
		sx(0), sy(0), ex(0), ey(0),
		z_section(0),
		direction(0),
		time(0),
		endTime(0),
		max_count(0),
		count(0),
		vnum(0),
		is_aggressive(0),
		is_high_traffic_hours(0),
		is_first_spawn_delayed(0),
		event(NULL),
		id(0),
		is_boss_or_stone(false)
	{}

	DWORD GetSpawnDelay()
	{
		if (endTime > 0)
			return number(time, endTime);
		else
			return time;
	}

	DWORD GetInitialSpawnDelay()
	{
		if (is_first_spawn_delayed)
			return number(300, 1200);
		else
			return number(0, 16);
	}
} REGEN;

EVENTINFO(regen_event_info)
{
	LPREGEN 	regen;

	regen_event_info()
	: regen( 0 )
	{
	}
};

typedef regen_event_info REGEN_EVENT_INFO;

typedef struct regen_exception
{
	LPREGEN_EXCEPTION prev, next;

	int		sx, sy, ex, ey;
	BYTE	z_section;
} REGEN_EXCEPTION;

class CDungeon;

EVENTINFO(dungeon_regen_event_info)
{
	LPREGEN 	regen;
	CDungeon::IdType dungeon_id;

	dungeon_regen_event_info()
	: regen( 0 )
	, dungeon_id( 0 )
	{
	}
};

extern bool	regen_load(const char *filename, long lMapIndex, int base_x, int base_y);
extern bool	regen_do(const char* filename, long lMapIndex, int base_x, int base_y, LPDUNGEON pDungeon, bool bOnce = true );
extern bool	regen_load_in_file(const char* filename, long lMapIndex, int base_x, int base_y );
extern void	regen_free();

extern bool	is_regen_exception(long x, long y);
extern void	regen_reset(int x, int y);
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
