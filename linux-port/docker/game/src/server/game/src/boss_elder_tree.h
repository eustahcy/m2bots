EVENTINFO(boss_elder_tree_event_info)
{
	DynamicCharacterPtr ch;
	int value1;
	int value2;
};

EVENTINFO(boss_elder_tree_healer_event_info)
{
	DynamicCharacterPtr boss_ch;
	DynamicCharacterPtr ch;
};


EVENTINFO(boss_elder_tree_post_kill_info)
{
	DynamicCharacterPtr ch;
	long dest_x;
	long dest_y;
};

extern void BossElderTree_Spawn(LPCHARACTER);
extern void BossElderTree_Dead(LPCHARACTER);
extern const int ELDER_TREE_BOSS_VNUM;

// Files shared by GameCore.top
