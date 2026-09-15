enum BLUEDRAGON_STONE_EFFECT
{
	DEF_BONUS	=	1,
	ATK_BONUS	=	2,
	REGEN_TIME_BONUS	=	3,
	REGEN_PECT_BONUS	=	4,
};

extern unsigned int BlueDragon_GetRangeFactor (const char* key, const int val);
extern unsigned int BlueDragon_GetSkillFactor (const size_t cnt, ...);
extern unsigned int BlueDragon_GetIndexFactor (const char* container, const size_t idx, const char* key);
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
