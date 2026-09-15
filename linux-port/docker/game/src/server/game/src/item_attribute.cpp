#include "stdafx.h"
#include "constants.h"
#include "log.h"
#include "item.h"
#include "char.h"
#include "desc.h"
#include "item_manager.h"
#include "questmanager.h"
#ifdef ENABLE_NEWSTUFF
#include "config.h"
#endif

const int MAX_NORM_ATTR_NUM = ITEM_MANAGER::MAX_NORM_ATTR_NUM;
const int MAX_RARE_ATTR_NUM = ITEM_MANAGER::MAX_RARE_ATTR_NUM;

int CItem::GetAttributeSetIndex()
{
	if (GetType() == ITEM_WEAPON)
	{
		if (GetSubType() == WEAPON_ARROW)
			return -1;

#ifdef ENABLE_QUIVER_SYSTEM
		if (GetSubType() == WEAPON_QUIVER)
			return -1;
#endif
		return ATTRIBUTE_SET_WEAPON;
	}

	if (GetType() == ITEM_ARMOR)
	{
		switch (GetSubType())
		{
			case ARMOR_BODY:
				return ATTRIBUTE_SET_BODY;

			case ARMOR_WRIST:
				return ATTRIBUTE_SET_WRIST;

			case ARMOR_FOOTS:
				return ATTRIBUTE_SET_FOOTS;

			case ARMOR_NECK:
				return ATTRIBUTE_SET_NECK;

			case ARMOR_HEAD:
				return ATTRIBUTE_SET_HEAD;

			case ARMOR_SHIELD:
				return ATTRIBUTE_SET_SHIELD;

			case ARMOR_EAR:
				return ATTRIBUTE_SET_EAR;

			#ifdef ENABLE_PENDANT_SYSTEM
			case ARMOR_PENDANT:
				return ATTRIBUTE_SET_PENDANT;
			#endif

			#ifdef ENABLE_GLOVE_SYSTEM
			case ARMOR_GLOVE:
				return ATTRIBUTE_SET_GLOVE;
			#endif
		}
	}
	else if (GetType() == ITEM_COSTUME)
	{
		switch (GetSubType())
		{
			case COSTUME_BODY:
				#ifdef ENABLE_ITEM_ATTR_COSTUME
				return ATTRIBUTE_SET_COSTUME_BODY;
				#else
				return ATTRIBUTE_SET_BODY;
				#endif

			case COSTUME_HAIR:
				#ifdef ENABLE_ITEM_ATTR_COSTUME
				return ATTRIBUTE_SET_COSTUME_HAIR;
				#else
				return ATTRIBUTE_SET_HEAD;
				#endif

			#ifdef ENABLE_MOUNT_COSTUME_SYSTEM
			case COSTUME_MOUNT:
				break;
			#endif

			#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
			case COSTUME_WEAPON:
				#ifdef ENABLE_ITEM_ATTR_COSTUME
				return ATTRIBUTE_SET_COSTUME_WEAPON;
				#else
				return ATTRIBUTE_SET_WEAPON;
				#endif
			#endif

		}
	}

	return -1;
}

bool CItem::HasAnyAttr()
{
	for (int i = 0; i < MAX_NORM_ATTR_NUM; ++i)
		if (GetAttributeType(i) > POINT_NONE)
			return true;
	return false;
}

bool CItem::HasAttr(BYTE bApply)
{
	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
		if (m_pProto->aApplies[i].bType == bApply)
			return true;

	for (int i = 0; i < MAX_NORM_ATTR_NUM + 1; ++i)
		if (GetAttributeType(i) == bApply)
			return true;

	return false;
}

bool CItem::HasRareAttr(BYTE bApply)
{
	for (int i = 0; i < MAX_RARE_ATTR_NUM; ++i)
		if (GetAttributeType(i + 5) == bApply)
			return true;

	return false;
}

void CItem::AddAttribute(BYTE bApply, short sValue)
{
	if (HasAttr(bApply))
		return;

	int i = GetAttributeCount();

	if (i >= MAX_NORM_ATTR_NUM)
		sys_err("item attribute overflow!");
	else
	{
		if (sValue)
			SetAttribute(i, bApply, sValue);
	}
}

void CItem::AddAttr(BYTE bApply, BYTE bLevel)
{
	if (HasAttr(bApply))
		return;

	if (bLevel <= 0)
		return;

	int i = GetAttributeCount();

	if (i == MAX_NORM_ATTR_NUM)
		sys_err("item attribute overflow!");
	else
	{
		const TItemAttrTable & r = g_map_itemAttr[bApply];
		long lVal = r.lValues[MIN(4, bLevel - 1)];

		if (lVal)
			SetAttribute(i, bApply, lVal);
	}
}

void CItem::PutAttributeWithLevel(BYTE bLevel)
{
	int iAttributeSet = GetAttributeSetIndex();
	if (iAttributeSet < 0)
		return;

	if (bLevel > ITEM_ATTRIBUTE_MAX_LEVEL)
		return;

	std::vector<int> avail;

	int total = 0;

	for (int i = 0; i < POINT_MAX_NUM; ++i)
	{
		const TItemAttrTable & r = g_map_itemAttr[i];

		if (r.bMaxLevelBySet[iAttributeSet] && !HasAttr(i))
		{
			avail.emplace_back(i);
			total += r.dwProb;
		}
	}

	unsigned int prob = number(1, total);
	int attr_idx = POINT_NONE;

	for (DWORD i = 0; i < avail.size(); ++i)
	{
		const TItemAttrTable & r = g_map_itemAttr[avail[i]];

		if (prob <= r.dwProb)
		{
			attr_idx = avail[i];
			break;
		}

		prob -= r.dwProb;
	}

	if (!attr_idx)
	{
		sys_err("Cannot put item attribute %d %d", iAttributeSet, bLevel);
		return;
	}

	const TItemAttrTable & r = g_map_itemAttr[attr_idx];

	if (bLevel > r.bMaxLevelBySet[iAttributeSet])
		bLevel = r.bMaxLevelBySet[iAttributeSet];

	AddAttr(attr_idx, bLevel);
}

void CItem::PutAttribute(const int * aiAttrPercentTable)
{
	int iAttrLevelPercent = number(1, 100);
	int i;

	for (i = 0; i < ITEM_ATTRIBUTE_MAX_LEVEL; ++i)
	{
		if (iAttrLevelPercent <= aiAttrPercentTable[i])
			break;

		iAttrLevelPercent -= aiAttrPercentTable[i];
	}

	PutAttributeWithLevel(i + 1);
}

void CItem::ChangeAttribute(const int* aiChangeProb)
{
	SetChangingAttr(true);
	int iAttributeCount = GetAttributeCount();

	TPlayerItemAttribute attributes[ITEM_ATTRIBUTE_MAX_NUM]{};
	memcpy(attributes, GetAttributes(), sizeof(TPlayerItemAttribute) * ITEM_ATTRIBUTE_MAX_NUM);

	ClearAttribute();

	if (iAttributeCount == 0)
		return;

	TItemTable const * pProto = GetProto();

	if (pProto && pProto->sAddonType)
	{
		ApplyAddon(pProto->sAddonType);
	}

	int tmpChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
	{
		0, 10, 40, 35, 15,
	};

	if (g_dwClientVersion < 1000000 && quest::CQuestManager::instance().GetEventFlag("beta_server") >= 5) // etap pvp
	{
		aiChangeProb = NULL;
		tmpChangeProb[1] = 0;
		tmpChangeProb[2] = 0;
		tmpChangeProb[3] = 0;
		tmpChangeProb[4] = 100;
	}

	for (int i = GetAttributeCount(); i < iAttributeCount; ++i)
	{
		int retry = 0;
		while (retry++ < 5)
		{
			if (aiChangeProb == NULL)
			{
				PutAttribute(tmpChangeProb);
			}
			else
			{
				PutAttribute(aiChangeProb);
			}

			if (retry < 5 && attributes[i] == GetAttribute(i))
			{
				ClearAttribute(i);
			}
			else
				break;
		}
	}

	SetChangingAttr(false);
	UpdatePacket();

	LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MAX);
	char buf[96 + 1];
	std::ostringstream attrs;
	for (int i = 0; i < iAttributeCount; i++) {
		auto attr = GetAttribute(i);
		attrs << static_cast<int>(attr.bType) << "," << attr.sValue;
		if (i < iAttributeCount - 1) {
			attrs << "|";
		}
	}
	snprintf(buf, sizeof(buf), "%s", attrs.str().c_str());

	DWORD dwPID = 0;
	if (GetOwner())
		dwPID = GetOwner()->GetPlayerID();
	LogManager::instance().ItemMetaLog(dwPID, GetID(), GetOriginalVnum(), buf, "CHANGE_ATTR");
}

void CItem::AddAttribute()
{
	static const int aiItemAddAttributePercent[ITEM_ATTRIBUTE_MAX_LEVEL] =
	{
		40, 50, 10, 0, 0
	};

	int attrCountBefore = GetAttributeCount();
	if (attrCountBefore < MAX_NORM_ATTR_NUM)
	{
		PutAttribute(aiItemAddAttributePercent);

		int attrCountAfter = GetAttributeCount();
		if (attrCountBefore < attrCountAfter)
		{
			LOG_LEVEL_CHECK_N_RET(LOG_LEVEL_MAX);
			int index = attrCountAfter - 1;

			DWORD dwPID = 0;
			if (GetOwner())
				dwPID = GetOwner()->GetPlayerID();
			char buf[96 + 1];
			snprintf(buf, sizeof(buf), "%u %u %d", index, GetAttributeType(index), GetAttributeValue(index));
			LogManager::instance().ItemMetaLog(dwPID, GetID(), GetOriginalVnum(), buf, "ADD_ATTR");
		}
	}
}

void CItem::ClearAttribute()
{
	for (int i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		ClearAttribute(i);
	}
}

void CItem::ClearAttribute(int index)
{
	m_aAttr[index].bType = 0;
	m_aAttr[index].sValue = 0;
}


int CItem::GetAttributeCount()
{
	int i;

	for (i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		if (GetAttributeType(i) == 0)
			break;
	}

	return i;
}

int CItem::FindAttribute(BYTE bType)
{
	for (int i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		if (GetAttributeType(i) == bType)
			return i;
	}

	return -1;
}

bool CItem::RemoveAttributeAt(int index)
{
	if (GetAttributeCount() <= index)
		return false;

	for (int i = index; i < MAX_NORM_ATTR_NUM - 1; ++i)
	{
		SetAttribute(i, GetAttributeType(i + 1), GetAttributeValue(i + 1));
	}

	SetAttribute(MAX_NORM_ATTR_NUM - 1, POINT_NONE, 0);
	return true;
}

bool CItem::RemoveAttributeType(BYTE bType)
{
	int index = FindAttribute(bType);
	return index != -1 && RemoveAttributeType(index);
}

void CItem::SetAttributes(const TPlayerItemAttribute* c_pAttribute)
{
	thecore_memcpy(m_aAttr, c_pAttribute, sizeof(m_aAttr));

	Save();
}

void CItem::SetAttribute(int i, BYTE bType, short sValue)
{
	assert(i < MAX_NORM_ATTR_NUM);

	m_aAttr[i].bType = bType;
	m_aAttr[i].sValue = sValue;
	UpdatePacket();
	Save();
}

void CItem::SetForceAttribute(int i, BYTE bType, short sValue)
{
	assert(i < ITEM_ATTRIBUTE_MAX_NUM);

	m_aAttr[i].bType = bType;
	m_aAttr[i].sValue = sValue;
	UpdatePacket();
	Save();

	if (bType)
	{
		DWORD dwPID = 0;
		if (GetOwner())
			dwPID = GetOwner()->GetPlayerID();

		LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemAttrLog(dwPID, GetID(), GetOriginalVnum(), bType, sValue, "ATTR_FORCE"));
	}
}

void CItem::CopyAttributeTo(LPITEM pItem)
{
	if (!pItem)
		return;

	pItem->SetAttributes(m_aAttr);

	if (IsUnique70LevelWeapon(GetVnum()))
	{
		auto currentApply = m_aAttr[5];
		auto newApply = GetUniqueApplyWeapon70(currentApply.bType, pItem->GetRefineLevel());
		pItem->SetForceAttribute(5, newApply.bType, static_cast<short>(newApply.lValue));
	}
}

int CItem::GetRareAttrCount()
{
	int ret = 0;

	for (DWORD dwIdx = ITEM_ATTRIBUTE_RARE_START; dwIdx < ITEM_ATTRIBUTE_RARE_END; dwIdx++)
	{
		if (m_aAttr[dwIdx].bType != 0)
			ret++;
	}

	return ret;
}

bool CItem::ChangeRareAttribute()
{
	if (GetRareAttrCount() == 0)
		return false;

	int cnt = GetRareAttrCount();

	for (int i = 0; i < cnt; ++i)
	{
		m_aAttr[i + ITEM_ATTRIBUTE_RARE_START].bType = 0;
		m_aAttr[i + ITEM_ATTRIBUTE_RARE_START].sValue = 0;
	}

	if (GetOwner() && GetOwner()->GetDesc())
		LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(GetOwner(), this, "SET_RARE_CHANGE", ""))
	else
		LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(0, 0, 0, GetID(), "SET_RARE_CHANGE", "", "", GetOriginalVnum()))

	for (int i = 0; i < cnt; ++i)
	{
		AddRareAttribute();
	}

	return true;
}

bool CItem::ChangeRareAttribute2()
{
	if (GetRareAttrCount() == 0)
		return false;

	int cnt = GetRareAttrCount();

	for (int i = 0; i < cnt; ++i)
	{
		m_aAttr[i + ITEM_ATTRIBUTE_RARE_START].bType = 0;
		m_aAttr[i + ITEM_ATTRIBUTE_RARE_START].sValue = 0;
	}

	if (GetOwner() && GetOwner()->GetDesc())
		LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(GetOwner(), this, "SET_RARE_CHANGE", ""))
	else
		LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(0, 0, 0, GetID(), "SET_RARE_CHANGE", "", "", GetOriginalVnum()))

	for (int i = 0; i < cnt; ++i)
	{
		AddRareAttribute2();
	}

	return true;
}

bool CItem::AddRareAttribute()
{
	int count = GetRareAttrCount();

	if (count >= ITEM_ATTRIBUTE_RARE_NUM)
		return false;

	int pos = count + ITEM_ATTRIBUTE_RARE_START;
	TPlayerItemAttribute & attr = m_aAttr[pos];

	int nAttrSet = GetAttributeSetIndex();
	std::vector<int> avail;

	for (int i = 0; i < POINT_MAX_NUM; ++i)
	{
		const TItemAttrTable & r = g_map_itemRare[i];

		if (r.dwApplyIndex != 0 && r.bMaxLevelBySet[nAttrSet] > 0 && HasRareAttr(i) != true)
		{
			avail.emplace_back(i);
		}
	}

	const TItemAttrTable& r = g_map_itemRare[avail[number(0, avail.size() - 1)]];
	int nAttrLevel = 5;

	if (nAttrLevel > r.bMaxLevelBySet[nAttrSet])
		nAttrLevel = r.bMaxLevelBySet[nAttrSet];

	attr.bType = r.dwApplyIndex;
	attr.sValue = r.lValues[nAttrLevel - 1];

	UpdatePacket();

	Save();

	const char * pszIP = NULL;

	if (GetOwner() && GetOwner()->GetDesc())
		pszIP = GetOwner()->GetDesc()->GetHostName();

	LOG_LEVEL_CHECK(LOG_LEVEL_MAX, LogManager::instance().ItemLog(pos, attr.bType, attr.sValue, GetID(), "SET_RARE", "", pszIP ? pszIP : "", GetOriginalVnum()));
	return true;
}

bool CItem::AddRareAttribute2(const int * aiAttrPercentTable)
{
	int count = GetRareAttrCount();
	if (count >= ITEM_ATTRIBUTE_RARE_NUM)
		return false;

	static const int aiItemAddAttributePercent[ITEM_ATTRIBUTE_MAX_LEVEL] =
	{
		40, 50, 10, 0, 0
	};
	if (aiAttrPercentTable == NULL)
		aiAttrPercentTable = aiItemAddAttributePercent;

	if (GetRareAttrCount() < MAX_RARE_ATTR_NUM)
		PutRareAttribute(aiAttrPercentTable);

	return true;
}

void CItem::PutRareAttribute(const int * aiAttrPercentTable)
{
	int iAttrLevelPercent = number(1, 100);
	int i;

	for (i = 0; i < ITEM_ATTRIBUTE_MAX_LEVEL; ++i)
	{
		if (iAttrLevelPercent <= aiAttrPercentTable[i])
			break;

		iAttrLevelPercent -= aiAttrPercentTable[i];
	}

	PutRareAttributeWithLevel(i + 1);
}

void CItem::PutRareAttributeWithLevel(BYTE bLevel)
{
	int iAttributeSet = GetAttributeSetIndex();
	if (iAttributeSet < 0)
		return;

	if (bLevel > ITEM_ATTRIBUTE_MAX_LEVEL)
		return;

	std::vector<int> avail;

	int total = 0;

	for (int i = 0; i < POINT_MAX_NUM; ++i)
	{
		const TItemAttrTable & r = g_map_itemRare[i];

		if (r.bMaxLevelBySet[iAttributeSet] && !HasRareAttr(i))
		{
			avail.emplace_back(i);
			total += r.dwProb;
		}
	}

	unsigned int prob = number(1, total);
	int attr_idx = POINT_NONE;

	for (DWORD i = 0; i < avail.size(); ++i)
	{
		const TItemAttrTable & r = g_map_itemRare[avail[i]];

		if (prob <= r.dwProb)
		{
			attr_idx = avail[i];
			break;
		}

		prob -= r.dwProb;
	}

	if (!attr_idx)
	{
		sys_err("Cannot put item rare attribute %d %d", iAttributeSet, bLevel);
		return;
	}

	const TItemAttrTable & r = g_map_itemRare[attr_idx];

	if (bLevel > r.bMaxLevelBySet[iAttributeSet])
		bLevel = r.bMaxLevelBySet[iAttributeSet];

	AddRareAttr(attr_idx, bLevel);
}

void CItem::AddRareAttr(BYTE bApply, BYTE bLevel)
{
	if (HasRareAttr(bApply))
		return;

	if (bLevel <= 0)
		return;

	int i = ITEM_ATTRIBUTE_RARE_START + GetRareAttrCount();

	if (i == ITEM_ATTRIBUTE_RARE_END)
		sys_err("item rare attribute overflow!");
	else
	{
		const TItemAttrTable & r = g_map_itemRare[bApply];
		long lVal = r.lValues[MIN(4, bLevel - 1)];

		if (lVal)
			SetForceAttribute(i, bApply, lVal);
	}
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
