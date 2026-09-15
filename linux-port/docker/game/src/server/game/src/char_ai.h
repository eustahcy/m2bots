extern void AIOnSpawn(LPCHARACTER ch);
extern void AIOnDead(LPCHARACTER ch, LPCHARACTER attacker);
extern void AIOnStartBattle(LPCHARACTER ch, LPCHARACTER victim);
extern int AIOnDamage(LPCHARACTER attacker, LPCHARACTER victim, int damage, BYTE damageType);

// Files shared by GameCore.top
