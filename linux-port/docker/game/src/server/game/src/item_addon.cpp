#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "item.h"
#include "item_addon.h"

CItemAddonManager::CItemAddonManager()
{
}

CItemAddonManager::~CItemAddonManager()
{
}

void CItemAddonManager::ApplyAverageTo(LPITEM pItem)
{
	int iSkillBonus = MINMAX(-30, (int)(gauss_random(0, 5) + 0.5f), 30);

	int iNormalHitBonus = 0;
	if (abs(iSkillBonus) <= 20)
		iNormalHitBonus = -2 * iSkillBonus + abs(number(-8, 8) + number(-8, 8)) + number(1, 4);
	else
		iNormalHitBonus = -2 * iSkillBonus + number(1, 5);

	pItem->RemoveAttributeType(POINT_SKILL_DAMAGE_BONUS);
	pItem->RemoveAttributeType(POINT_NORMAL_HIT_DAMAGE_BONUS);
	pItem->AddAttribute(POINT_NORMAL_HIT_DAMAGE_BONUS, iNormalHitBonus);
	pItem->AddAttribute(POINT_SKILL_DAMAGE_BONUS, iSkillBonus);
}

void CItemAddonManager::ApplyAddonTo(int iAddonType, LPITEM pItem)
{
	if (!pItem)
	{
		sys_err("ITEM pointer null");
		return;
	}

	if (iAddonType == ATTR_DAMAGE_ADDON)
		ApplyAverageTo(pItem);
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
