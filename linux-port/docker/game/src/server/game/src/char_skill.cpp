#include "stdafx.h"
#include <sstream>

#include "utils.h"
#include "config.h"
#include "vector.h"
#include "char.h"
#include "char_manager.h"
#include "battle.h"
#include "desc.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "item.h"
#include "sectree_manager.h"
#include "mob_manager.h"
#include "start_position.h"
#include "party.h"
#include "buffer_manager.h"
#include "guild.h"
#include "log.h"
#include "unique_item.h"
#include "questmanager.h"
#include "../../common/CommonDefines.h"

#define ENABLE_FORCE2MASTERSKILL
// #define ENABLE_NULLIFYAFFECT_LIMIT

static const DWORD s_adwSubSkillVnums[] =
{
	SKILL_LEADERSHIP,
	SKILL_COMBO,
	SKILL_MINING,
	SKILL_LANGUAGE1,
	SKILL_LANGUAGE2,
	SKILL_LANGUAGE3,
	SKILL_POLYMORPH,
	SKILL_HORSE,
	SKILL_HORSE_SUMMON,
	SKILL_HORSE_WILDATTACK,
	SKILL_HORSE_CHARGE,
	SKILL_HORSE_ESCAPE,
	SKILL_HORSE_WILDATTACK_RANGE,
	SKILL_ADD_HP,
	SKILL_RESIST_PENETRATE
};

int GetSkillMaxTargetCount(DWORD dwSkillVnum) // unused
{
	switch (dwSkillVnum)
	{
	case SKILL_SHOOTING_DRAGON:
	case SKILL_HORSE_WILDATTACK_RANGE:
	case SKILL_HORSE_ESCAPE:
	case SKILL_CHARGE:
	case SKILL_REPETITIVE_SHOT:
	case SKILL_SWORD_STRIKE:
	case SKILL_POISON_CLOUD:
	case SKILL_DISPEL:
	case SKILL_DARK_STRIKE:
	case SKILL_FLAME_STRIKE:
	case SKILL_FLAME_SPIRIT:
	case SKILL_SPIRIT_ORB:
	case SKILL_DARK_ORB:
	case SKILL_FLYING_TALISMAN:
	case SKILL_DRAGON_ROAR:
	case SKILL_SUMMON_LIGHTNING:
	case SKILL_LIGHTNING_THROW:
		return 1;
	case SKILL_FINGER_STRIKE:
	case SKILL_THREE_WAY_CUT:
	case SKILL_FAST_ATTACK:
	case SKILL_LIGHTNING_CLAW:
		return 3;
	default:
		return 5;
	}
}

int GetMaxAttackCountForSkill(DWORD dwSkillVnum, int skillPower)
{
	switch (dwSkillVnum)
	{
	case SKILL_THREE_WAY_CUT:
	case SKILL_ROLLING_DAGGER:
		return 3;
	case SKILL_SHOOTING_DRAGON:
	case SKILL_HORSE_WILDATTACK_RANGE:
		return 5;
	case SKILL_HORSE_ESCAPE:
		return 15;
	case SKILL_REPETITIVE_SHOT:
		return 2 + 6 * skillPower / 100;
	default:
		return 1;
	}
}

time_t CHARACTER::GetSkillNextReadTime(DWORD dwVnum) const
{
	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("vnum overflow (vnum: %u)", dwVnum);
		return 0;
	}

	return m_pSkillLevels ? m_pSkillLevels[dwVnum].tNextRead : 0;
}

void CHARACTER::SetSkillNextReadTime(DWORD dwVnum, time_t time, bool bSuccess)
{
	if (m_pSkillLevels && !bSuccess && FindAffect(AFFECT_LEARN_NO_FAIL_TIME))
	{
		RemoveAffect(AFFECT_LEARN_NO_FAIL_TIME);
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("LEARN_NO_FAIL_TIME"));
		m_pSkillLevels[dwVnum].tNextRead = 0;
		return;
	}

	if (m_pSkillLevels && dwVnum < SKILL_MAX_NUM)
		m_pSkillLevels[dwVnum].tNextRead = time;
}

bool TSkillUseInfo::HitOnce(DWORD dwVnum)
{
	if (!bUsed)
		return false;

	sys_log(1, "__HitOnce NextUse %u current %u count %d scount %d", dwNextSkillUsableTime, get_dword_time(), iHitCount, iSplashCount);

	if (dwNextSkillUsableTime && dwNextSkillUsableTime<get_dword_time() && dwVnum != SKILL_FLAME_SPIRIT && dwVnum != SKILL_HORSE_WILDATTACK)
	{
		sys_log(1, "__HitOnce can't hit");

		return false;
	}

	if (iHitCount == -1)
	{
		sys_log(1, "__HitOnce OK %d %d %d", dwNextSkillUsableTime, get_dword_time(), iHitCount);
		return true;
	}

	if (iHitCount)
	{
		sys_log(1, "__HitOnce OK %d %d %d", dwNextSkillUsableTime, get_dword_time(), iHitCount);
		iHitCount--;
		return true;
	}
	return false;
}

bool TSkillUseInfo::UseSkill(bool isGrandMaster, DWORD vid, DWORD dwCooltime, int splashcount, int hitcount, int range)
{
	this->isGrandMaster = isGrandMaster;
	DWORD dwCur = get_dword_time();

	if (bUsed && dwNextSkillUsableTime > dwCur)
	{
		sys_log(1, "cooltime is not over delta %u", dwNextSkillUsableTime - dwCur);
		iHitCount = 0;
		return false;
	}

	bUsed = true;
	#ifdef ENABLE_SKILL_COOLDOWN_CHECK
	this->cooldown = true;
	this->skillCount = 0;
	#endif

	if (dwCooltime)
		dwNextSkillUsableTime = dwCur + dwCooltime;
	else
		dwNextSkillUsableTime = 0;

	iRange = range;
	iMaxHitCount = iHitCount = hitcount;

	if (test_server)
		sys_log(0, "UseSkill NextUse %u  current %u cooltime %d hitcount %d/%d", dwNextSkillUsableTime, dwCur, dwCooltime, iHitCount, iMaxHitCount);

	dwVID = vid;
	iSplashCount = splashcount;
	return true;
}

#ifdef ENABLE_SKILL_COOLDOWN_CHECK
bool TSkillUseInfo::IsOnCooldown(int vnum)
{
	const int maxSkillCount = GetMaxAttackCountForSkill(vnum);

	this->skillCount++;
	if (maxSkillCount)
		return this->skillCount > maxSkillCount;

	const auto ret = !this->cooldown;
	this->cooldown = false;
	return ret;
}
#endif

int CHARACTER::GetChainLightningMaxCount() const
{
	return aiChainLightningCountBySkillLevel[MIN(SKILL_MAX_LEVEL, GetSkillLevel(SKILL_LIGHTNING_CLAW))];
}

void CHARACTER::SetAffectedEunhyung()
{
	m_dwAffectedEunhyungLevel = GetSkillPower(SKILL_STEALTH);
}

bool CHARACTER::IsInvisible()
{
	return IsAffectFlag(AFF_INVISIBILITY) || IsAffectFlag(AFF_SKILL_STEALTH);
}

void CHARACTER::SetSkillGroup(BYTE bSkillGroup)
{
	if (bSkillGroup > 2)
		return;

	if (GetLevel() < 5)
		return;

	m_points.skill_group = bSkillGroup;

	TPacketGCChangeSkillGroup p;
	p.header = HEADER_GC_SKILL_GROUP;
	p.skill_group = m_points.skill_group;

	GetDesc()->Packet(&p, sizeof(TPacketGCChangeSkillGroup));
}

int CHARACTER::ComputeCooltime(int time)
{
	return CalculateDuration(GetPoint(POINT_CASTING_SPEED), time);
}

void CHARACTER::SkillLevelPacket()
{
	if (!GetDesc() || !m_pSkillLevels)
		return;

	TPacketGCSkillLevel pack;

	pack.bHeader = HEADER_GC_SKILL_LEVEL;
	thecore_memcpy(&pack.skills, m_pSkillLevels, sizeof(TPlayerSkill) * SKILL_MAX_NUM);
	GetDesc()->Packet(&pack, sizeof(TPacketGCSkillLevel));
}

void CHARACTER::SetSkillLevel(DWORD dwVnum, BYTE bLev)
{
	if (NULL == m_pSkillLevels)
		return;

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("vnum overflow (vnum %u)", dwVnum);
		return;
	}

	m_pSkillLevels[dwVnum].bLevel = MIN(40, bLev);

	if (bLev >= 40)
		m_pSkillLevels[dwVnum].bMasterType = SKILL_PERFECT_MASTER;
	else if (bLev >= 30)
		m_pSkillLevels[dwVnum].bMasterType = SKILL_GRAND_MASTER;
	else if (bLev >= 20)
		m_pSkillLevels[dwVnum].bMasterType = SKILL_MASTER;
	else
		m_pSkillLevels[dwVnum].bMasterType = SKILL_NORMAL;
}

bool CHARACTER::IsLearnableSkill(DWORD dwSkillVnum) const
{
	const CSkillProto * pkSkill = CSkillManager::instance().Get(dwSkillVnum);

	if (!pkSkill)
		return false;

	if (GetSkillLevel(dwSkillVnum) >= SKILL_MAX_LEVEL)
		return false;

	if (pkSkill->dwType == 0)
	{
		if (GetSkillLevel(dwSkillVnum) >= pkSkill->bMaxLevel)
			return false;

		return true;
	}

	if (pkSkill->dwType == 5)
	{
		if (dwSkillVnum == SKILL_HORSE_WILDATTACK_RANGE && GetJob() != JOB_ASSASSIN)
			return false;

		return true;
	}

	if (GetSkillGroup() == 0)
		return false;

	if (pkSkill->dwType - 1 == GetJob())
		return true;
#ifdef ENABLE_WOLFMAN_CHARACTER

	if (7 == pkSkill->dwType && JOB_WOLFMAN == GetJob())
		return true;
#endif
	if (6 == pkSkill->dwType)
	{
		if (SKILL_7_A_ANTI_TANHWAN <= dwSkillVnum && dwSkillVnum <= SKILL_7_D_ANTI_YONGBI)
		{
			for (int i=0 ; i < 4 ; i++)
			{
				if (unsigned(SKILL_7_A_ANTI_TANHWAN + i) != dwSkillVnum)
				{
					if (0 != GetSkillLevel(SKILL_7_A_ANTI_TANHWAN + i))
					{
						return false;
					}
				}
			}

			return true;
		}

		if (SKILL_8_A_ANTI_GIGONGCHAM <= dwSkillVnum && dwSkillVnum <= SKILL_8_D_ANTI_BYEURAK)
		{
			for (int i=0 ; i < 4 ; i++)
			{
				if (unsigned(SKILL_8_A_ANTI_GIGONGCHAM + i) != dwSkillVnum)
				{
					if (0 != GetSkillLevel(SKILL_8_A_ANTI_GIGONGCHAM + i))
						return false;
				}
			}

			return true;
		}
	}

	return false;
}

// ADD_GRANDMASTER_SKILL
bool CHARACTER::LearnGrandMasterSkill(DWORD dwSkillVnum)
{
	CSkillProto * pkSk = CSkillManager::instance().Get(dwSkillVnum);

	if (!pkSk)
		return false;

	if (!IsLearnableSkill(dwSkillVnum))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill."));
		return false;
	}

	sys_log(0, "learn grand master skill[%d] cur %d, next %d", dwSkillVnum, get_global_time(), GetSkillNextReadTime(dwSkillVnum));

	if (pkSk->dwType == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill up to Grand Master level."));
		return false;
	}

	if (GetSkillMasterType(dwSkillVnum) != SKILL_GRAND_MASTER)
	{
		if (GetSkillMasterType(dwSkillVnum) > SKILL_GRAND_MASTER)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already are a Master of this skill. You cannot train this skill any further."));
		else
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your Skill is not high enough to become a Grand Master."));
		return false;
	}

	std::string strTrainSkill;
	{
		std::ostringstream os;
		os << "training_grandmaster_skill.skill" << dwSkillVnum;
		strTrainSkill = os.str();
	}

	BYTE bLastLevel = GetSkillLevel(dwSkillVnum);

	int idx = MIN(9, GetSkillLevel(dwSkillVnum) - 30);

	int iTotalReadCount = GetQuestFlag(strTrainSkill) + 1;
	SetQuestFlag(strTrainSkill, iTotalReadCount);

	int iMinReadCount = aiGrandMasterSkillBookMinCount[idx];

	int learn_bonus = GetPoint(POINT_LEARN_CHANCE);

	if (FindAffect(AFFECT_SKILL_BOOK_BONUS))
		RemoveAffect(AFFECT_SKILL_BOOK_BONUS);

	CAffect* pkAff = FindAffect(AFFECT_POTION_GENERAL_USE, POINT_LEARN_CHANCE);
	if (pkAff)
		RemoveAffect(pkAff);

	DWORD nextTime = get_global_time() + 3600 * 12;
	DWORD chance = 30;

	if (iTotalReadCount < iMinReadCount)
		chance = 4;

	chance = chance * (100 + learn_bonus) / 100;

	ChatDebug("chance %d minReadCount %d totalReadCount %d", chance, iMinReadCount, iTotalReadCount);
	bool bSuccess = number(1, 100) <= chance;
	if (bSuccess)
	{
		SkillLevelUp(dwSkillVnum, SKILL_UP_BY_QUEST);
	}

	SetSkillNextReadTime(dwSkillVnum, nextTime, true); // czas sprawdzany jest w quescie

	if (bLastLevel == GetSkillLevel(dwSkillVnum))
	{
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("That did not work. Damn!"));
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Training has failed. Please try again later."));
		LogManager::instance().CharLog(this, dwSkillVnum, "GM_READ_FAIL", "");
		return false;
	}

	ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("My body is full of energy!"));
	ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("The training seems to be working already..."));
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You successfully finished your training with the book."));
	LogManager::instance().CharLog(this, dwSkillVnum, "GM_READ_SUCCESS", "");
	return true;
}
// END_OF_ADD_GRANDMASTER_SKILL

static bool FN_should_check_exp(LPCHARACTER ch)
{
	// @warme005
	return ch->GetLevel() < gPlayerMaxLevel;
}

// #define ENABLE_MASTER_SKILLBOOK_NO_STEPS
bool CHARACTER::LearnSkillByBook(DWORD dwSkillVnum, DWORD dwProb)
{
	const CSkillProto* pkSk = CSkillManager::instance().Get(dwSkillVnum);

	if (!pkSk)
		return false;

	if (!IsLearnableSkill(dwSkillVnum))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill."));
		return false;
	}

	DWORD need_exp = 0;

	if (FN_should_check_exp(this))
	{
		need_exp = 20000;

		if ( GetExp() < need_exp )
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot read this due to your lack of experience."));
			return false;
		}
	}

	if (pkSk->dwType != 0)
	{
		if (GetSkillMasterType(dwSkillVnum) != SKILL_MASTER)
		{
			if (GetSkillMasterType(dwSkillVnum) > SKILL_MASTER)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill with a Book."));
			else
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This skill's level is not high enough to be trained with a Book."));
			return false;
		}
	}

	if (get_global_time() < GetSkillNextReadTime(dwSkillVnum))
	{
		if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
		{
			if (!RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY))
				return false;

			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have escaped the evil ghost curse with the help of an Exorcism Scroll."));
		}
		else
		{
			SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
			return false;
		}
	}

	BYTE bLastLevel = GetSkillLevel(dwSkillVnum);


	DWORD dwLearnChanceBonus = GetPoint(POINT_LEARN_CHANCE);
	bool is_skill_book_bonus_affect = false;
	if (FindAffect(AFFECT_SKILL_BOOK_BONUS))
	{
		is_skill_book_bonus_affect = true;
		RemoveAffect(AFFECT_SKILL_BOOK_BONUS);
	}

	CAffect* pkAff = FindAffect(AFFECT_POTION_GENERAL_USE, POINT_LEARN_CHANCE);
	if (pkAff)
		RemoveAffect(pkAff);

	if (dwProb != 0) // WIEKSZOSC PASYWNYCH SKILLI
	{
		dwProb = dwProb * (100 + dwLearnChanceBonus) / 100;

		ChatDebug("LearnSkillByBook Pct %d prob %d", dwSkillVnum, dwProb);

		bool isSuccess = number(1, 100) <= dwProb;
		if (isSuccess)
		{
			if (test_server)
				sys_log(0, "LearnSkillByBook %u SUCC", dwSkillVnum);

			SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
		}
		else
		{
			if (test_server)
				sys_log(0, "LearnSkillByBook %u FAIL", dwSkillVnum);
		}
		SetSkillNextReadTime(dwSkillVnum, get_global_time() + SKILLBOOK_LEARN_DELAY, isSuccess);
	}
	else // WSZYSTKIE KLASOWE SKILLE
	{
		int idx = MIN(9, GetSkillLevel(dwSkillVnum) - 20);

		sys_log(0, "LearnSkillByBook %s table idx %d value %d", GetName(), idx, aiSkillBookCountForLevelUp[idx]);

		int grade = GetSkillMasterType(dwSkillVnum);
		int need_bookcount = 0;
		if (grade == SKILL_MASTER)
		{
			need_bookcount = GetSkillLevel(dwSkillVnum) - 20;
		}
		else if (grade == SKILL_GRAND_MASTER)
		{
			need_bookcount = GetSkillLevel(dwSkillVnum) - 30;

			if (dwSkillVnum == SKILL_LEADERSHIP)
			{
				int index = need_bookcount;
				int books_needed_for_leadership_grandmaster[10] = { 3, 3, 3, 4, 4, 5, 6, 7, 8, 9 };
				need_bookcount = books_needed_for_leadership_grandmaster[index];
			}
		}

		PointChange(POINT_EXP, -need_exp);

		quest::CQuestManager& q = quest::CQuestManager::instance();
		quest::PC* pPC = q.GetPC(GetPlayerID());

		if (pPC)
		{
			char flag[128+1];
			memset(flag, 0, sizeof(flag));
			snprintf(flag, sizeof(flag), "traning_master_skill.%u.read_count", dwSkillVnum);

			int read_count = pPC->GetFlag(flag);
			int percent = 35;
			percent = is_skill_book_bonus_affect ? 100 : percent * (100 + dwLearnChanceBonus) / 100;

			ChatDebug("LearnSkillByBook grade %d need_bookcount %d percent %d", grade, need_bookcount, percent);

			bool isSuccess = number(1, 100) <= percent;
			SetSkillNextReadTime(dwSkillVnum, get_global_time() + SKILLBOOK_LEARN_DELAY, isSuccess);
			if (isSuccess)
			{
				if (read_count >= need_bookcount)
				{
					SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
					pPC->SetFlag(flag, 0);

					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have successfully finished your training with the Book."));
					LogManager::instance().CharLog(this, dwSkillVnum, "READ_SUCCESS", "");
					return true;
				}
				else
				{
					pPC->SetFlag(flag, read_count + 1);

					switch (number(1, 3))
					{
						case 1:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("I'm making progress, but I still haven't understood everything."));
							break;

						case 2:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("These instructions are difficult to understand. I have to carry on studying."));
							break;

						case 3:
						default:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("I understand this chapter. But I've got to carry on working hard."));
							break;
					}

					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to read %d more skill books to improve this skill."), need_bookcount - read_count);
					return true;
				}
			}
			else
			{
				ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("That did not work. Damn!"));
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Training has failed. Please try again later."));
				LogManager::instance().CharLog(this, dwSkillVnum, "READ_FAIL", "");
			}
			return true;
		}
	}

	if (bLastLevel != GetSkillLevel(dwSkillVnum))
	{
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("My body is full of energy!"));
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("The training seems to be working already..."));
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have successfully finished your training with the Book."));
		LogManager::instance().CharLog(this, dwSkillVnum, "READ_SUCCESS", "");
	}
	else
	{
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("That did not work. Damn!"));
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Training has failed. Please try again later."));
		LogManager::instance().CharLog(this, dwSkillVnum, "READ_FAIL", "");
	}

	return true;
}

bool CHARACTER::SkillLevelDown(DWORD dwVnum)
{
	if (NULL == m_pSkillLevels)
		return false;

	if (g_bSkillDisable)
		return false;

	if (IsPolymorphed())
		return false;

	CSkillProto * pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("There is no such skill by number %u", dwVnum);
		return false;
	}

	if (!IsLearnableSkill(dwVnum))
		return false;

	if (GetSkillMasterType(pkSk->dwVnum) != SKILL_NORMAL)
		return false;

	if (!GetSkillGroup())
		return false;

	if (pkSk->dwVnum >= SKILL_MAX_NUM)
		return false;

	if (m_pSkillLevels[pkSk->dwVnum].bLevel == 0)
		return false;

	int idx = POINT_SKILL;
	switch (pkSk->dwType)
	{
		case 0:
			idx = POINT_SUB_SKILL;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 6:
#ifdef ENABLE_WOLFMAN_CHARACTER
		case 7:
#endif
			idx = POINT_SKILL;
			break;
		case 5:
			idx = POINT_HORSE_SKILL;
			break;
		default:
			sys_err("Wrong skill type %d skill vnum %d", pkSk->dwType, pkSk->dwVnum);
			return false;

	}

	PointChange(idx, +1);
	SetSkillLevel(pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].bLevel - 1);

	sys_log(0, "SkillDown: %s %u %u %u type %u", GetName(), pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].bMasterType, m_pSkillLevels[pkSk->dwVnum].bLevel, pkSk->dwType);
	Save();

	ComputePoints();
	SkillLevelPacket();
	return true;
}

void CHARACTER::SkillLevelUp(DWORD dwVnum, BYTE bMethod)
{
	if (NULL == m_pSkillLevels)
		return;

	if (g_bSkillDisable)
		return;

	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change your status while you are transformed."));
		return;
	}

	if (SKILL_7_A_ANTI_TANHWAN <= dwVnum && dwVnum <= SKILL_8_D_ANTI_BYEURAK)
	{
		if (0 == GetSkillLevel(dwVnum))
			return;
	}

	const CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("There is no such skill by number (vnum %u)", dwVnum);
		return;
	}

	if (pkSk->dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("Skill Vnum overflow (vnum %u)", dwVnum);
		return;
	}

	if (!IsLearnableSkill(dwVnum))
		return;

	if (pkSk->dwType != 0)
	{
		switch (GetSkillMasterType(pkSk->dwVnum))
		{
			case SKILL_GRAND_MASTER:
				if (bMethod != SKILL_UP_BY_QUEST)
					return;
				break;

			case SKILL_PERFECT_MASTER:
				return;
		}
	}

	if (bMethod == SKILL_UP_BY_POINT)
	{
		if (GetSkillMasterType(pkSk->dwVnum) != SKILL_NORMAL)
			return;

		if (IS_SET(pkSk->dwFlag, SKILL_FLAG_DISABLE_BY_POINT_UP))
			return;
	}
	else if (bMethod == SKILL_UP_BY_BOOK)
	{
		if (pkSk->dwType != 0)
			if (GetSkillMasterType(pkSk->dwVnum) != SKILL_MASTER)
				return;
	}

	if (GetLevel() < pkSk->bLevelLimit)
		return;

	if (pkSk->preSkillVnum)
		if (GetSkillMasterType(pkSk->preSkillVnum) == SKILL_NORMAL &&
			GetSkillLevel(pkSk->preSkillVnum) < pkSk->preSkillLevel)
			return;

	if (!GetSkillGroup())
		return;

	if (pkSk->dwType != 0 &&
		pkSk->dwType != 5 && // horse skill
		GetSkillLevel(pkSk->dwVnum) >= 17 && GetSkillLevel(pkSk->dwVnum) < 20
		&& GetLevel() > 30)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't advance this skill further. Use Skill Reset Scroll or Band of Oblivion."));
		return;
	}

	if (bMethod == SKILL_UP_BY_POINT)
	{
		int idx;

		switch (pkSk->dwType)
		{
			case 0:
				idx = POINT_SUB_SKILL;
				break;

			case 1:
			case 2:
			case 3:
			case 4:
			case 6:
#ifdef ENABLE_WOLFMAN_CHARACTER
			case 7:
#endif
				idx = POINT_SKILL;
				break;

			case 5:
				idx = POINT_HORSE_SKILL;
				break;

			default:
				sys_err("Wrong skill type %d skill vnum %d", pkSk->dwType, pkSk->dwVnum);
				return;
		}

		if (GetPoint(idx) < 1)
			return;

		PointChange(idx, -1);
	}

	int SkillPointBefore = GetSkillLevel(pkSk->dwVnum);
	SetSkillLevel(pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].bLevel + 1);

	if (pkSk->dwType != 0)
	{
		switch (GetSkillMasterType(pkSk->dwVnum))
		{
			case SKILL_NORMAL:

				if (GetSkillLevel(pkSk->dwVnum) >= 17)
				{
					int forceToMasterSkillCount = GetQuestFlag("reset_status_items.force_to_master_skill");
					if (forceToMasterSkillCount > 0)
					{
						SetSkillLevel(pkSk->dwVnum, 20);
						SetQuestFlag("reset_status_items.force_to_master_skill", forceToMasterSkillCount-1);
					}
					else
					{
						const int reset_count = GetQuestFlag("skill_reset2.reset_count");

						int chance = MIN(100, 25 + reset_count * 25);

						// wbijanie pierwszego skilla na M1
						// ponizej 35 nie da sie miec 2 skilli na M1
						if (GetLevel() < 35) 
						{
							chance += 20;
						}

						if (number(1,100) <= chance)
						{
							SetSkillLevel(pkSk->dwVnum, 20);
							if (reset_count > 0)
								SetQuestFlag("skill_reset2.reset_count", 0);
						}
					}
				}
				break;

			case SKILL_MASTER:
				if (GetSkillLevel(pkSk->dwVnum) >= 30)
				{
					if (number(1, 31 - MIN(30, GetSkillLevel(pkSk->dwVnum))) == 1)
						SetSkillLevel(pkSk->dwVnum, 30);
				}
				break;

			case SKILL_GRAND_MASTER:
				if (GetSkillLevel(pkSk->dwVnum) >= 40)
				{
					SetSkillLevel(pkSk->dwVnum, 40);
				}
				break;
		}
	}

	char szSkillUp[1024];

	snprintf(szSkillUp, sizeof(szSkillUp), "SkillUp: %s %u %d %d[Before:%d] type %u",
			GetName(), pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].bMasterType, m_pSkillLevels[pkSk->dwVnum].bLevel, SkillPointBefore, pkSk->dwType);

	sys_log(0, "%s", szSkillUp);

	LogManager::instance().CharLog(this, pkSk->dwVnum, "SKILLUP", szSkillUp);
	Save();

	ComputePoints();
	SkillLevelPacket();
}

void CHARACTER::ComputeSkillPoints()
{
	if (g_bSkillDisable)
		return;
}

void CHARACTER::ResetSkill()
{
	if (NULL == m_pSkillLevels)
		return;

	std::vector<std::pair<DWORD, TPlayerSkill> > vec;
	size_t count = sizeof(s_adwSubSkillVnums) / sizeof(s_adwSubSkillVnums[0]);

	for (size_t i = 0; i < count; ++i)
	{
		if (s_adwSubSkillVnums[i] >= SKILL_MAX_NUM)
			continue;

		vec.emplace_back(s_adwSubSkillVnums[i], m_pSkillLevels[s_adwSubSkillVnums[i]]);
	}

	memset(m_pSkillLevels, 0, sizeof(TPlayerSkill) * SKILL_MAX_NUM);

	std::vector<std::pair<DWORD, TPlayerSkill> >::const_iterator iter = vec.begin();

	while (iter != vec.end())
	{
		const std::pair<DWORD, TPlayerSkill>& pair = *(iter++);
		m_pSkillLevels[pair.first] = pair.second;
	}

	ComputePoints();
	SkillLevelPacket();
}

void CHARACTER::ComputePassiveSkill(DWORD dwVnum)
{
	if (g_bSkillDisable)
		return;

	if (GetSkillLevel(dwVnum) == 0)
		return;

	CSkillProto * pkSk = CSkillManager::instance().Get(dwVnum);
	pkSk->SetPointVar("k", GetSkillLevel(dwVnum));
	int iAmount = (int) pkSk->kPointPoly.Eval();

	sys_log(2, "%s passive #%d on %d amount %d", GetName(), dwVnum, pkSk->bPointOn, iAmount);
	PointChange(pkSk->bPointOn, iAmount);
}

struct FFindNearVictim
{
	FFindNearVictim(LPCHARACTER center, LPCHARACTER attacker, const CHARACTER_SET& excepts_set = empty_set_)
		: m_pkChrCenter(center),
	m_pkChrNextTarget(NULL),
	m_pkChrAttacker(attacker),
	m_count(0),
	m_excepts_set(excepts_set)
	{
	}

	void operator ()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER) ent;

		if (!m_excepts_set.empty()) {
			if (m_excepts_set.find(pkChr) != m_excepts_set.end())
				return;
		}

		if (m_pkChrCenter == pkChr)
			return;

		if (!battle_is_attackable(m_pkChrAttacker, pkChr))
		{
			return;
		}

		if (abs(m_pkChrCenter->GetX() - pkChr->GetX()) > 1000 || abs(m_pkChrCenter->GetY() - pkChr->GetY()) > 1000)
			return;

		if (pkChr->IsInvisible())
			return;

		float fDist = DISTANCE_APPROX(m_pkChrCenter->GetX() - pkChr->GetX(), m_pkChrCenter->GetY() - pkChr->GetY());

		if (fDist < 1000)
		{
			++m_count;

			if ((m_count == 1) || number(1, m_count) == 1)
				m_pkChrNextTarget = pkChr;
		}
	}

	LPCHARACTER GetVictim()
	{
		return m_pkChrNextTarget;
	}

	LPCHARACTER m_pkChrCenter;
	LPCHARACTER m_pkChrNextTarget;
	LPCHARACTER m_pkChrAttacker;
	int		m_count;
	const CHARACTER_SET & m_excepts_set;
private:
	static CHARACTER_SET empty_set_;
};

CHARACTER_SET FFindNearVictim::empty_set_;

EVENTINFO(chain_lightning_event_info)
{
	DWORD			dwVictim;
	DWORD			dwChr;

	chain_lightning_event_info()
	: dwVictim(0)
	, dwChr(0)
	{
	}
};

EVENTFUNC(ChainLightningEvent)
{
	chain_lightning_event_info * info = dynamic_cast<chain_lightning_event_info *>( event->info );

	LPCHARACTER pkChrVictim = CHARACTER_MANAGER::instance().Find(info->dwVictim);
	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().Find(info->dwChr);
	LPCHARACTER pkTarget = NULL;

	if (!pkChr || !pkChrVictim)
	{
		sys_log(1, "use chainlighting, but no character");
		return 0;
	}

	sys_log(1, "chainlighting event %s", pkChr->GetName());

	if (pkChrVictim->GetParty())
	{
		pkTarget = pkChrVictim->GetParty()->GetNextOwnership(NULL, pkChrVictim->GetX(), pkChrVictim->GetY());
		if (pkTarget == pkChrVictim || !number(0, 2) || pkChr->GetChainLightingExcept().find(pkTarget) != pkChr->GetChainLightingExcept().end())
			pkTarget = NULL;
	}

	if (!pkTarget)
	{
		// 1. Find Next victim
		FFindNearVictim f(pkChrVictim, pkChr, pkChr->GetChainLightingExcept());

		if (pkChrVictim->GetSectree())
		{
			pkChrVictim->GetSectree()->ForEachAround(f);
			// 2. If exist, compute it again
			pkTarget = f.GetVictim();
		}
	}

	if (pkTarget)
	{
		pkChrVictim->CreateFly(FLY_CHAIN_LIGHTNING, pkTarget);
		pkChr->ComputeSkill(SKILL_LIGHTNING_CLAW, pkTarget);
		pkChr->AddChainLightningExcept(pkTarget);
	}
	else
	{
		sys_log(1, "%s use chainlighting, but find victim failed near %s", pkChr->GetName(), pkChrVictim->GetName());
	}

	return 0;
}

void SetPolyVarForAttack(LPCHARACTER ch, CSkillProto * pkSk, LPITEM pkWeapon)
{
	if (!ch || !pkSk)
		return;

	if (ch->IsPC())
	{
		if (pkWeapon && pkWeapon->GetType() == ITEM_WEAPON)
		{
			int iWep = number(pkWeapon->GetValue(3), pkWeapon->GetValue(4));
			iWep += pkWeapon->GetValue(5);

			int iMtk = number(pkWeapon->GetValue(1), pkWeapon->GetValue(2));
			iMtk += pkWeapon->GetValue(5);

			int iAMtk = (pkWeapon->GetValue(1) + pkWeapon->GetValue(2))/2;
			iAMtk += pkWeapon->GetValue(5);

			pkSk->SetPointVar("wep", iWep);
			pkSk->SetPointVar("mtk", iMtk);
			pkSk->SetPointVar("mwep", iMtk);
			pkSk->SetPointVar("amwep", iAMtk);
		}
		else
		{
			pkSk->SetPointVar("wep", 0);
			pkSk->SetPointVar("mtk", 0);
			pkSk->SetPointVar("mwep", 0);
			pkSk->SetPointVar("amwep", 0);
		}
	}
	else
	{
		int iWep = number(ch->GetMobDamageMin(), ch->GetMobDamageMax());
		pkSk->SetPointVar("wep", iWep);
		pkSk->SetPointVar("mwep", iWep);
		pkSk->SetPointVar("mtk", iWep);
		pkSk->SetPointVar("amwep", iWep);
	}
}

struct FuncSplashDamage
{
	FuncSplashDamage(int x, int y, CSkillProto * pkSk, LPCHARACTER pkChr, int iAmount, int iAG, int iMaxHit, LPITEM pkWeapon, bool bDisableCooltime, TSkillUseInfo* pInfo, BYTE bUseSkillPower)
		:
		m_x(x), m_y(y), m_pkSk(pkSk), m_pkChr(pkChr), m_iAmount(iAmount), m_iAG(iAG), m_iCount(0), m_iMaxHit(iMaxHit), m_pkWeapon(pkWeapon), m_bDisableCooltime(bDisableCooltime), m_pInfo(pInfo), m_bUseSkillPower(bUseSkillPower)
		{
		}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
		{
			//if (m_pkSk->dwVnum == SKILL_LIGHTNING_CLAW) sys_log(0, "CHAIN target not character %s", m_pkChr->GetName());
			return;
		}

		LPCHARACTER pkChrVictim = (LPCHARACTER) ent;

		int distance = DISTANCE_APPROX(m_x - pkChrVictim->GetX(), m_y - pkChrVictim->GetY());
		if (distance > m_pkSk->iSplashRange)
		{
			if(test_server)
				sys_log(0, "XXX target too far %s", m_pkChr->GetName());
			return;
		}

		if (!battle_is_attackable(m_pkChr, pkChrVictim))
		{
			if(test_server)
				sys_log(0, "XXX target not attackable %s", m_pkChr->GetName());
			return;
		}

		if (m_pkChr->IsPC())

			if (!(m_pkSk->dwVnum >= GUILD_SKILL_START && m_pkSk->dwVnum <= GUILD_SKILL_END))
				if (!m_bDisableCooltime && m_pInfo && !m_pInfo->HitOnce(m_pkSk->dwVnum) && m_pkSk->dwVnum != SKILL_FLAME_SPIRIT)
				{
					bool isMultiHitSkill = m_pkSk->dwVnum == SKILL_FLAME_SPIRIT || m_pkSk->dwVnum == SKILL_REPETITIVE_SHOT;
					if (!m_bDisableCooltime && m_pInfo && !m_pInfo->HitOnce(m_pkSk->dwVnum) && !isMultiHitSkill)
					{
						if (test_server)
							sys_log(0, "check guild skill %s", m_pkChr->GetName());
						return;
					}
				}

		++m_iCount;

		int iDam;

		////////////////////////////////////////////////////////////////////////////////
		//float k = 1.0f * m_pkChr->GetSkillPower(m_pkSk->dwVnum) * m_pkSk->bMaxLevel / 100;
		//m_pkSk->kPointPoly2.SetVar("k", 1.0 * m_bUseSkillPower * m_pkSk->bMaxLevel / 100);
		m_pkSk->SetPointVar("k", 1.0 * m_bUseSkillPower * m_pkSk->bMaxLevel / 100);
		m_pkSk->SetPointVar("lv", m_pkChr->GetLevel());
		m_pkSk->SetPointVar("iq", m_pkChr->GetPoint(POINT_IQ));
		m_pkSk->SetPointVar("str", m_pkChr->GetPoint(POINT_ST));
		m_pkSk->SetPointVar("dex", m_pkChr->GetPoint(POINT_DX));
		m_pkSk->SetPointVar("con", m_pkChr->GetPoint(POINT_HT));
		m_pkSk->SetPointVar("def", m_pkChr->GetPoint(POINT_DEF_GRADE));
		m_pkSk->SetPointVar("odef", m_pkChr->GetPoint(POINT_DEF_GRADE) - m_pkChr->GetPoint(POINT_DEF_GRADE_BONUS));
		m_pkSk->SetPointVar("horse_level", m_pkChr->GetHorseLevel());

		//int iPenetratePct = (int)(1 + k*4);
		bool bIgnoreDefense = false;

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_PENETRATE))
		{
			int iPenetratePct = (int) m_pkSk->kPointPoly2.Eval();

			if (number(1, 100) <= iPenetratePct)
				bIgnoreDefense = true;
		}

		if (!bIgnoreDefense)
		{
			bIgnoreDefense = m_pkChr->PenetrateHit(pkChrVictim);
		}

		bool bIgnoreTargetRating = false;

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_IGNORE_TARGET_RATING))
		{
			int iPct = (int) m_pkSk->kPointPoly2.Eval();

			if (number(1, 100) <= iPct)
				bIgnoreTargetRating = true;
		}

		m_pkSk->SetPointVar("ar", CalcAttackRating(m_pkChr, pkChrVictim, bIgnoreTargetRating));

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
			m_pkSk->SetPointVar("atk", CalcMeleeDamage(m_pkChr, pkChrVictim, true, bIgnoreTargetRating));
		else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
		{
			LPITEM pkBow, pkArrow;

			if (1 == m_pkChr->GetArrowAndBow(&pkBow, &pkArrow, 1))
				m_pkSk->SetPointVar("atk", CalcArrowDamage(m_pkChr, pkChrVictim, pkBow, pkArrow, bIgnoreDefense, m_pkSk->dwVnum));
			else
				m_pkSk->SetPointVar("atk", 0);
		}

		if (m_pkSk->bPointOn == POINT_MOV_SPEED)
			m_pkSk->kPointPoly.SetVar("maxv", pkChrVictim->GetLimitPoint(POINT_MOV_SPEED));

		m_pkSk->SetPointVar("maxhp", pkChrVictim->GetMaxHP());
		m_pkSk->SetPointVar("maxsp", pkChrVictim->GetMaxSP());

		m_pkSk->SetPointVar("chain", m_pkChr->GetChainLightningIndex());
		m_pkChr->IncChainLightningIndex();

		bool bUnderEunhyung = m_pkChr->GetAffectedEunhyung() > 0;

		//m_pkSk->SetPointVar("ek", m_pkChr->GetAffectedEunhyung()*1./100);
		//m_pkChr->ClearAffectedEunhyung();
		SetPolyVarForAttack(m_pkChr, m_pkSk, m_pkWeapon);

		int iAmount = 0;

		if (m_pkChr->GetUsedSkillMasterType(m_pkSk->dwVnum) >= SKILL_GRAND_MASTER)
		{
			iAmount = (int) m_pkSk->kMasterBonusPoly.Eval();
		}
		else
		{
			iAmount = (int) m_pkSk->kPointPoly.Eval();
		}

		if (test_server && iAmount == 0 && m_pkSk->bPointOn != POINT_NONE)
		{
			m_pkChr->ChatPacket(CHAT_TYPE_INFO, "It doesn't work. Check the skill formula.");
		}
		////////////////////////////////////////////////////////////////////////////////
		iAmount = -iAmount;

		if (m_pkSk->dwVnum == SKILL_AMBUSH)
		{
			float fDelta = GetDegreeDelta(m_pkChr->GetRotation(), pkChrVictim->GetRotation());
			float adjust;

			if (fDelta < 35.0f)
			{
				adjust = 1.5f;

				if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
				{
					adjust += 0.5f;
				}
			}
			else
			{
				adjust = 1.0f;

				if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
					adjust += 0.5f;
			}

			if (bUnderEunhyung)
				adjust += 0.4f * (float)m_pkChr->GetAffectedEunhyung() / 100;

			//m_pkChr->ChatDebug("ambush adjust %.2f %f", adjust, (float)m_pkChr->GetAffectedEunhyung() / 100);

			iAmount = (int) (iAmount * adjust);
		}
		else if (m_pkSk->dwVnum == SKILL_FAST_ATTACK)
		{
			float adjust = 1.0;

			if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
			{
				adjust = 1.35f;
			}

			iAmount = (int) (iAmount * adjust);
		}
#ifdef ENABLE_WOLFMAN_CHARACTER
		else if (m_pkSk->dwVnum == SKILL_GONGDAB)
		{
			float adjust = 1.0;

			if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_CLAW)
			{
				adjust = 1.35f;
			}

			iAmount = (int)(iAmount * adjust);
		}
#endif
		////////////////////////////////////////////////////////////////////////////////
		//sys_log(0, "name: %s skill: %s amount %d to %s", m_pkChr->GetName(), m_pkSk->szName, iAmount, pkChrVictim->GetName());

		iDam = CalcBattleDamage(iAmount, m_pkChr->GetLevel(), pkChrVictim->GetLevel());

		if (m_pkChr->IsPC() && m_pkChr->m_SkillUseInfo[m_pkSk->dwVnum].GetMainTargetVID() != (DWORD) pkChrVictim->GetVID())
		{
			iDam = (int) (iDam * m_pkSk->kSplashAroundDamageAdjustPoly.Eval());
		}

		EDamageType dt = DAMAGE_TYPE_NONE;

		auto skillAttrType = m_pkSk->bSkillAttrType;

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_COMPUTE_MAGIC_DAMAGE))
			skillAttrType = SKILL_ATTR_TYPE_MAGIC;

		switch (skillAttrType)
		{
			case SKILL_ATTR_TYPE_NORMAL:
				break;

			case SKILL_ATTR_TYPE_MELEE:
				{
					dt = DAMAGE_TYPE_MELEE;

					iDam = CalcDamBonus(m_pkChr, pkChrVictim, iDam);
					iDam = CalcPlayerBonus(m_pkChr, pkChrVictim, iDam);

					if (!bIgnoreDefense)
						iDam = MAX(iDam - pkChrVictim->GetDefence(), 1);

				}
				break;

			case SKILL_ATTR_TYPE_RANGE:
				dt = DAMAGE_TYPE_RANGE;
				// SKILL_FLAG_USE_ARROW_DAMAGE already calculated AttBonus
				iDam = CalcDamBonus(m_pkChr, pkChrVictim, iDam);
				break;

			case SKILL_ATTR_TYPE_MAGIC:
				dt = DAMAGE_TYPE_MAGIC;

				if (!bIgnoreDefense)
				{
					iDam = MAX(iDam - pkChrVictim->GetMagicDefenceRating() * iDam / 100, 1);
				}

				iDam = CalcAttBonus(m_pkChr, pkChrVictim, iDam);
#ifdef ENABLE_MAGIC_REDUCTION_SYSTEM
				{
					const int resist_magic = MINMAX(0, pkChrVictim->GetPoint(POINT_RESIST_MAGIC), 100);
					const int resist_magic_reduction = MINMAX(0, (m_pkChr->GetJob()==JOB_SURA) ? m_pkChr->GetPoint(POINT_RESIST_MAGIC_REDUCTION)/2 : m_pkChr->GetPoint(POINT_RESIST_MAGIC_REDUCTION), 50);
					const int total_res_magic = MINMAX(0, resist_magic - resist_magic_reduction, 100);
					iDam = iDam * (100 - total_res_magic) / 100;
				}
#else
				iDam = iDam * (100 - pkChrVictim->GetResistPoint(POINT_RESIST_MAGIC, m_pkChr)) / 100;
#endif
				break;

			default:
				sys_err("Unknown skill attr type %u vnum %u", m_pkSk->bSkillAttrType, m_pkSk->dwVnum);
				break;
		}

		//if (pkChrVictim->IsNPC())
		//{
		//	if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_WIND))
		//	{
		//		iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_WIND)) / 100;
		//	}

		//	if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_ELEC))
		//	{
		//		iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_ELEC)) / 100;
		//	}

		//	if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_FIRE))
		//	{
		//		iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_FIRE)) / 100;
		//	}
		//}

		if (m_pkSk->dwVnum == SKILL_HORSE_ESCAPE)
		{
			if (!pkChrVictim->IsNPC())
				return;

			if (pkChrVictim->GetMobRank() >= MOB_RANK_BOSS)
				return;

			if (pkChrVictim->GetMobAttackRange() < 500)
				return;

			if (pkChrVictim->GetVictim() != m_pkChr)
				return;
		}

		if (pkChrVictim->CanBeginFight())
			pkChrVictim->BeginFight(m_pkChr);

		if (m_pkSk->dwVnum == SKILL_LIGHTNING_CLAW)
			sys_log(0, "%s CHAIN INDEX %d DAM %d DT %d", m_pkChr->GetName(), m_pkChr->GetChainLightningIndex() - 1, iDam, dt);

		{
			BYTE AntiSkillID = 0;

			switch (m_pkSk->dwVnum)
			{
				case SKILL_CHARGE:		AntiSkillID = SKILL_7_A_ANTI_TANHWAN;		break;
				case SKILL_AMBUSH:		AntiSkillID = SKILL_7_B_ANTI_AMSEOP;		break;
				case SKILL_FINGER_STRIKE:	AntiSkillID = SKILL_7_C_ANTI_SWAERYUNG;		break;
				case SKILL_SHOOTING_DRAGON:		AntiSkillID = SKILL_7_D_ANTI_YONGBI;		break;
				case SKILL_SPIRIT_STRIKE:	AntiSkillID = SKILL_8_A_ANTI_GIGONGCHAM;	break;
				case SKILL_REPETITIVE_SHOT:		AntiSkillID = SKILL_8_B_ANTI_YEONSA;		break;
				case SKILL_DARK_ORB:		AntiSkillID = SKILL_8_C_ANTI_MAHWAN;		break;
				case SKILL_SUMMON_LIGHTNING:		AntiSkillID = SKILL_8_D_ANTI_BYEURAK;		break;
			}

			if (0 != AntiSkillID)
			{
				BYTE AntiSkillLevel = pkChrVictim->GetSkillLevel(AntiSkillID);

				if (0 != AntiSkillLevel)
				{
					CSkillProto* pkSk = CSkillManager::instance().Get(AntiSkillID);
					if (!pkSk)
					{
						sys_err ("There is no anti skill(%d) in skill proto", AntiSkillID);
					}
					else
					{
						pkSk->SetPointVar("k", 1.0f * pkChrVictim->GetSkillPower(AntiSkillID) * pkSk->bMaxLevel / 100);

						double ResistAmount = pkSk->kPointPoly.Eval();

						sys_log(0, "ANTI_SKILL: Resist(%lf) Orig(%d) Reduce(%d)", ResistAmount, iDam, int(iDam * (ResistAmount/100.0)));

						iDam -= iDam * (ResistAmount/100.0);
					}
				}
			}
		}

		if (m_pkSk->dwVnum == SKILL_FLAME_SPIRIT)
		{
			if (distance > 0)
			{
				if (pkChrVictim->IsPC())
					return;

				iDam = iDam * 60 / 100;
			}
		}

		if (!pkChrVictim->Damage(m_pkChr, iDam, dt, m_pkSk->dwVnum, bIgnoreDefense) && !pkChrVictim->IsStun() && !pkChrVictim->IsDead()) // @fixme312 IsDead
		{
			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_REMOVE_GOOD_AFFECT))
			{
#ifdef ENABLE_NULLIFYAFFECT_LIMIT
				int iLevel = m_pkChr->GetLevel();
				int yLevel = pkChrVictim->GetLevel();
				// const float k = 1.0 * m_pkChr->GetSkillPower(m_pkSk->dwVnum, bSkillLevel) * m_pkSk->bMaxLevel / 100;
				int iDifLev = 9;
				if ((iLevel-iDifLev <= yLevel) && (iLevel+iDifLev >= yLevel))
#endif
				{
					int iAmount2 = (int) m_pkSk->kPointPoly2.Eval();
					int iDur2 = (int) m_pkSk->kDurationPoly2.Eval();

					int bonus = m_pkChr->GetPoint(POINT_PARTY_BUFFER_BONUS);
					if (bonus != 0)
						iDur2 += bonus/5;

					if (number(1, 100) <= iAmount2)
					{
						pkChrVictim->RemoveGoodAffect();
						pkChrVictim->AddAffect(m_pkSk->dwVnum, POINT_NONE, 0, AFF_SKILL_DISPEL, iDur2, 0, true);
					}
				}
			}
#ifdef ENABLE_WOLFMAN_CHARACTER
			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SLOW | SKILL_FLAG_STUN | SKILL_FLAG_FIRE_CONT | SKILL_FLAG_POISON | SKILL_FLAG_BLEEDING))
#else
			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SLOW | SKILL_FLAG_STUN | SKILL_FLAG_FIRE_CONT | SKILL_FLAG_POISON | SKILL_FLAG_STUN_MOB))
#endif
			{
				int iPct = (int) m_pkSk->kPointPoly2.Eval();
				int iDur = (int) m_pkSk->kDurationPoly2.Eval();

				int can_stun = IS_SET(m_pkSk->dwFlag, SKILL_FLAG_STUN) || m_pkSk->dwVnum == SKILL_FINGER_STRIKE;
				if (m_pkSk->dwVnum == SKILL_FINGER_STRIKE)
				{
					if (m_pkChr->GetSkillLevel(SKILL_FINGER_STRIKE) >= 20)
					{
						iPct = 32 * m_pkChr->GetSkillPower(SKILL_FINGER_STRIKE) / 10;
						iDur = 3;
					}
					else
						iPct = 0;
				}

				int bonus = m_pkChr->GetPoint(POINT_PARTY_BUFFER_BONUS);
				if (bonus != 0)
				{
					if (can_stun)
						bonus /= 5;
					else
						bonus /= 3;
				}

				if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_STUN_MOB) && !pkChrVictim->IsPC())
				{
					int stunPct = 500;
					int stunDuration = 4;
					SkillAttackAffect(m_pkChr, pkChrVictim, stunPct, IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, stunDuration, m_pkSk->szName);
				}
				else if (can_stun)
				{
					int iStunPct = iPct;
					if (m_pkSk->dwVnum == SKILL_CHARGE || m_pkSk->dwVnum == SKILL_POISON_ARROW)
						iStunPct = 1000;

					SkillAttackAffect(m_pkChr, pkChrVictim, iStunPct, IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, iDur, m_pkSk->szName);
				}

				if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SLOW))
				{
					SkillAttackAffect(m_pkChr, pkChrVictim, iPct, IMMUNE_SLOW, AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, iDur, m_pkSk->szName);
				}
				else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_FIRE_CONT))
				{
					m_pkSk->SetDurationVar("k", 1.0 * m_bUseSkillPower * m_pkSk->bMaxLevel / 100);
					m_pkSk->SetDurationVar("iq", m_pkChr->GetPoint(POINT_IQ));

					iDur = (int)m_pkSk->kDurationPoly2.Eval();
					int bonus = m_pkChr->GetPoint(POINT_PARTY_BUFFER_BONUS);

					if (bonus != 0)
					{
						iDur += bonus / 3;
					}
					m_pkChr->ChatDebug("fire chance %d", iDur);
					if (number(1, 100) <= iDur)
					{
						pkChrVictim->AttackedByFire(m_pkChr, iPct, 5);
					}
				}
				else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_POISON))
				{
					if (number(1, 100) <= iPct)
						pkChrVictim->AttackedByPoison(m_pkChr);
				}
#ifdef ENABLE_WOLFMAN_CHARACTER
				else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_BLEEDING))
				{
					if (number(1, 100) <= iPct)
						pkChrVictim->AttackedByBleeding(m_pkChr);
				}
#endif
			}

			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_CRUSH | SKILL_FLAG_CRUSH_LONG | SKILL_FLAG_CRUSH_MOB) &&
				!IS_SET(pkChrVictim->GetAIFlag(), AIFLAG_NOMOVE))
			{
				if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_CRUSH_MOB) && pkChrVictim->IsPC())
					return;

				if (pkChrVictim->IsResistPush())
					return;

				float fCrushSlidingLength = 200;

				if (m_pkChr->IsNPC())
					fCrushSlidingLength = 400;

				if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_CRUSH_LONG))
					fCrushSlidingLength *= 2;

				float fx, fy;
				float degree = GetDegreeFromPositionXY(m_pkChr->GetX(), m_pkChr->GetY(), pkChrVictim->GetX(), pkChrVictim->GetY());

				if (m_pkSk->dwVnum == SKILL_HORSE_WILDATTACK)
				{
					degree -= m_pkChr->GetRotation();
					degree = fmod(degree, 360.0f) - 180.0f;

					if (degree > 0)
						degree = m_pkChr->GetRotation() + 90.0f;
					else
						degree = m_pkChr->GetRotation() - 90.0f;
				}

				GetDeltaByDegree(degree, fCrushSlidingLength, &fx, &fy);
				sys_log(0, "CRUSH! %s -> %s (%d %d) -> (%d %d)", m_pkChr->GetName(), pkChrVictim->GetName(), pkChrVictim->GetX(), pkChrVictim->GetY(), (long)(pkChrVictim->GetX()+fx), (long)(pkChrVictim->GetY()+fy));
				long tx = (long)(pkChrVictim->GetX()+fx);
				long ty = (long)(pkChrVictim->GetY()+fy);

				pkChrVictim->Sync(tx, ty);
				pkChrVictim->Goto(tx, ty);
				pkChrVictim->CalculateMoveDuration();

				pkChrVictim->SyncPacket();
			}
		}

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_HP_ABSORB))
		{
			int iPct = (int) m_pkSk->kPointPoly2.Eval();
			m_pkChr->PointChange(POINT_HP, iDam * iPct / 100);
		}

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SP_ABSORB))
		{
			int iPct = (int) m_pkSk->kPointPoly2.Eval();
			m_pkChr->PointChange(POINT_SP, iDam * iPct / 100);
		}

		if (m_pkSk->dwVnum == SKILL_LIGHTNING_CLAW && m_pkChr->GetChainLightningIndex() < m_pkChr->GetChainLightningMaxCount())
		{
			chain_lightning_event_info* info = AllocEventInfo<chain_lightning_event_info>();

			info->dwVictim = pkChrVictim->GetVID();
			info->dwChr = m_pkChr->GetVID();

			event_create(ChainLightningEvent, info, passes_per_sec / 5);
		}
		if(test_server)
			sys_log(0, "FuncSplashDamage End :%s ", m_pkChr->GetName());
	}

	int		m_x;
	int		m_y;
	CSkillProto * m_pkSk;
	LPCHARACTER	m_pkChr;
	int		m_iAmount;
	int		m_iAG;
	int		m_iCount;
	int		m_iMaxHit;
	LPITEM	m_pkWeapon;
	bool m_bDisableCooltime;
	TSkillUseInfo* m_pInfo;
	BYTE m_bUseSkillPower;
};

bool CanSkillAddThirdPoint(CSkillProto* skill, LPCHARACTER user, LPCHARACTER victim)
{
	if (IS_SET(skill->dwFlag, SKILL_FLAG_THIRD_POINT_SELFONLY) && user != victim)
		return false;

	return true;
}

struct FuncSplashAffect
{
	FuncSplashAffect(LPCHARACTER ch, int x, int y, int iDist, DWORD dwVnum, BYTE bPointOn, int iAmount, DWORD dwAffectFlag, int iDuration, int iSPCost, bool bOverride, int iMaxHit)
	{
		m_x = x;
		m_y = y;
		m_iDist = iDist;
		m_dwVnum = dwVnum;
		m_bPointOn = bPointOn;
		m_iAmount = iAmount;
		m_dwAffectFlag = dwAffectFlag;
		m_iDuration = iDuration;
		m_iSPCost = iSPCost;
		m_bOverride = bOverride;
		m_pkChrAttacker = ch;
		m_iMaxHit = iMaxHit;
		m_iCount = 0;
	}

	void operator () (LPENTITY ent)
	{
		if (m_iMaxHit && m_iMaxHit <= m_iCount)
			return;

		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pkChr = (LPCHARACTER) ent;

			if (test_server)
				sys_log(0, "FuncSplashAffect step 1 : name:%s vnum:%d iDur:%d", pkChr->GetName(), m_dwVnum, m_iDuration);
			if (DISTANCE_APPROX(m_x - pkChr->GetX(), m_y - pkChr->GetY()) < m_iDist)
			{
				if (test_server)
					sys_log(0, "FuncSplashAffect step 2 : name:%s vnum:%d iDur:%d", pkChr->GetName(), m_dwVnum, m_iDuration);
				if (m_dwVnum == SKILL_SPIRIT_ORB)
					if (pkChr->CanBeginFight())
						pkChr->BeginFight(m_pkChrAttacker);

				if (pkChr->IsPC() && m_dwVnum == SKILL_SPIRIT_ORB)
					pkChr->AddAffect(m_dwVnum, m_bPointOn, m_iAmount, m_dwAffectFlag, m_iDuration/3, m_iSPCost, m_bOverride);
				else
					pkChr->AddAffect(m_dwVnum, m_bPointOn, m_iAmount, m_dwAffectFlag, m_iDuration, m_iSPCost, m_bOverride);

				m_iCount ++;
			}
		}
	}

	LPCHARACTER m_pkChrAttacker;
	int		m_x;
	int		m_y;
	int		m_iDist;
	DWORD	m_dwVnum;
	BYTE	m_bPointOn;
	int		m_iAmount;
	DWORD	m_dwAffectFlag;
	int		m_iDuration;
	int		m_iSPCost;
	bool	m_bOverride;
	int         m_iMaxHit;
	int         m_iCount;
};

EVENTINFO(skill_gwihwan_info)
{
	DWORD pid;
	BYTE bsklv;

	skill_gwihwan_info()
	: pid( 0 )
	, bsklv( 0 )
	{
	}
};

EVENTFUNC(skill_gwihwan_event)
{
	skill_gwihwan_info* info = dynamic_cast<skill_gwihwan_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "skill_gwihwan_event> <Factor> Null pointer" );
		return 0;
	}

	DWORD pid = info->pid;
	BYTE sklv= info->bsklv;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pid);

	if (!ch)
		return 0;

	int percent = 20 * sklv - 1;

	if (number(1, 100) <= percent)
	{
		PIXEL_POSITION pos;

		if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
		{
			sys_log(1, "Recall: %s %d %d -> %d %d", ch->GetName(), ch->GetX(), ch->GetY(), pos.x, pos.y);
			ch->WarpSet(pos.x, pos.y);
		}
		else
		{
			sys_err("CHARACTER::UseItem : cannot find spawn position (name %s, %d x %d)", ch->GetName(), ch->GetX(), ch->GetY());
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Teleportation has failed."));
	}
	return 0;
}

bool IsBuffSkill(DWORD skillVnum)
{
	switch (skillVnum)
	{
	case SKILL_BLESSING:
	case SKILL_REFLECT:
	case SKILL_DRAGON_HELP:
	case SKILL_SWIFTNESS:
	case SKILL_CURE:
	case SKILL_ATTACK_UP:
		return true;
	default:
		return false;
	}
}

int CHARACTER::ComputeSkillAtPosition(DWORD dwVnum, const PIXEL_POSITION& posTarget, BYTE bSkillLevel)
{
	if (GetMountVnum())
		return BATTLE_NONE;

	if (IsPolymorphed())
		return BATTLE_NONE;

	if (g_bSkillDisable)
		return BATTLE_NONE;

	CSkillProto * pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
		return BATTLE_NONE;

	if (test_server)
	{
		sys_log(0, "ComputeSkillAtPosition %s vnum %d x %d y %d level %d",
				GetName(), dwVnum, posTarget.x, posTarget.y, bSkillLevel);
	}

	if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
		return BATTLE_NONE;

	if (0 == bSkillLevel)
	{
		if ((bSkillLevel = GetSkillLevel(pkSk->dwVnum)) == 0)
		{
			return BATTLE_NONE;
		}
	}

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum, bSkillLevel) * pkSk->bMaxLevel / 100;

	pkSk->SetPointVar("k", k);
	pkSk->kSplashAroundDamageAdjustPoly.SetVar("k", k);

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
	{
		pkSk->SetPointVar("atk", CalcMeleeDamage(this, this, true, false));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		pkSk->SetPointVar("atk", CalcMagicDamage(this, this));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
	{
		LPITEM pkBow, pkArrow;
		if (1 == GetArrowAndBow(&pkBow, &pkArrow, 1))
		{
			pkSk->SetPointVar("atk", CalcArrowDamage(this, this, pkBow, pkArrow, true, pkSk->dwVnum));
		}
		else
		{
			pkSk->SetPointVar("atk", 0);
		}
	}

	if (pkSk->bPointOn == POINT_MOV_SPEED)
	{
		pkSk->SetPointVar("maxv", this->GetLimitPoint(POINT_MOV_SPEED));
	}

	if (IsBuffSkill(pkSk->dwVnum))
	{
		CAffect* aff = FindAffect(pkSk->dwVnum);
		if (aff)
		{
			RemoveAffect(aff);
		}
	}

	pkSk->SetPointVar("gr", GetSkillMasterType(pkSk->dwVnum));
	pkSk->SetPointVar("sl", GetSkillLevel(pkSk->dwVnum));
	pkSk->SetPointVar("lv", GetLevel());
	pkSk->SetPointVar("iq", GetPoint(POINT_IQ));
	pkSk->SetPointVar("str", GetPoint(POINT_ST));
	pkSk->SetPointVar("dex", GetPoint(POINT_DX));
	pkSk->SetPointVar("con", GetPoint(POINT_HT));
	pkSk->SetPointVar("maxhp", this->GetMaxHP());
	pkSk->SetPointVar("maxsp", this->GetMaxSP());
	pkSk->SetPointVar("chain", 0);
	pkSk->SetPointVar("ar", CalcAttackRating(this, this));
	pkSk->SetPointVar("def", GetPoint(POINT_DEF_GRADE));
	pkSk->SetPointVar("odef", GetPoint(POINT_DEF_GRADE) - GetPoint(POINT_DEF_GRADE_BONUS));
	pkSk->SetPointVar("horse_level", GetHorseLevel());

	if (pkSk->bSkillAttrType != SKILL_ATTR_TYPE_NORMAL)
		OnMove(true);

	LPITEM pkWeapon = GetWear(WEAR_WEAPON);

	SetPolyVarForAttack(this, pkSk, pkWeapon);

	pkSk->SetDurationVar("k", k/*bSkillLevel*/);

	int iAmount = (int) pkSk->kPointPoly.Eval();
	int iAmount2 = (int) pkSk->kPointPoly2.Eval();

	// ADD_GRANDMASTER_SKILL
	int iAmount3 = (int) pkSk->kPointPoly3.Eval();

	if (GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER)
	{
		/*
		   if (iAmount >= 0)
		   iAmount += (int) m_pkSk->kMasterBonusPoly.Eval();
		   else
		   iAmount -= (int) m_pkSk->kMasterBonusPoly.Eval();
		 */
		iAmount = (int) pkSk->kMasterBonusPoly.Eval();
	}

	if (test_server && iAmount == 0 && pkSk->bPointOn != POINT_NONE)
	{
		ChatPacket(CHAT_TYPE_INFO, "효과가 없습니다. 스킬 공식을 확인하세요");
	}

	bool canDurationBeExtended = IsBuffSkill(dwVnum) ||
		IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY);

	int iDur = (int)pkSk->kDurationPoly.Eval();
	int iDur2 = (int)pkSk->kDurationPoly2.Eval();
	int iDur3 = (int)pkSk->kDurationPoly3.Eval();
	if (canDurationBeExtended)
	{
		int pointSkillDuration = GetPoint(POINT_SKILL_DURATION);
		if (iDur > 0)
		{
			iDur += iDur * pointSkillDuration / 100;
			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
		}

		if (iDur2 > 0)
		{
			iDur2 += iDur2 * pointSkillDuration / 100;
			iDur2 += GetPoint(POINT_PARTY_BUFFER_BONUS);
		}

		if (iDur3 > 0)
		{
			iDur3 += iDur3 * pointSkillDuration / 100;
			iDur3 += GetPoint(POINT_PARTY_BUFFER_BONUS);
		}
	}

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_REMOVE_BAD_AFFECT))
	{
		if (number(1, 100) <= iAmount2)
		{
			RemoveBadAffect();
		}
	}
	// END_OF_ADD_GRANDMASTER_SKILL

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_ATTACK | SKILL_FLAG_USE_MELEE_DAMAGE | SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		bool bAdded = false;

		if (pkSk->bPointOn == POINT_HP && iAmount < 0)
		{
			int iAG = 0;

			FuncSplashDamage f(posTarget.x, posTarget.y, pkSk, this, iAmount, iAG, pkSk->lMaxHit, pkWeapon, m_bDisableCooltime, IsPC()?&m_SkillUseInfo[dwVnum]:NULL, GetSkillPower(dwVnum, bSkillLevel));

			if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
			{
				if (GetSectree())
					GetSectree()->ForEachAround(f);
			}
			else
			{
				//if (dwVnum == SKILL_LIGHTNING_CLAW) sys_log(0, "CHAIN skill call FuncSplashDamage %s", GetName());
				f(this);
			}
		}
		else
		{
			//if (dwVnum == SKILL_LIGHTNING_CLAW) sys_log(0, "CHAIN skill no damage %d %s", iAmount, GetName());

			if (IsPC())
				if (!(dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END))
					if (!m_bDisableCooltime && !m_SkillUseInfo[dwVnum].HitOnce(dwVnum) && dwVnum != SKILL_FLAME_SPIRIT)
					{
						//if (dwVnum == SKILL_LIGHTNING_CLAW) sys_log(0, "CHAIN skill cannot hit %s", GetName());
						return BATTLE_NONE;
					}

			if (iDur > 0)
			{
				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true, pkSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
		}

		if (pkSk->bPointOn2 != POINT_NONE)
		{

			sys_log(1, "try second %u %d %d", pkSk->dwVnum, pkSk->bPointOn2, iDur2);

			if (iDur2 > 0)
			{
				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, !bAdded);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, !bAdded, pkSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
			else
			{
				PointChange(pkSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (pkSk->bPointOn3 != POINT_NONE)
		{
			if (iDur3 > 0)
			{
				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH) || IS_SET(pkSk->dwFlag, SKILL_FLAG_THIRD_POINT_SELFONLY))
					AddAffect(pkSk->dwVnum, pkSk->bPointOn3, iAmount3, pkSk->dwAffectFlag3, iDur3, 0, !bAdded);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn3, iAmount3, pkSk->dwAffectFlag3, iDur3, 0, !bAdded, pkSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
			}
			else
			{
				PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_DAMAGE;
	}
	else
	{
		bool bAdded = false;

		if (iDur > 0)
		{
			pkSk->kDurationSPCostPoly.SetVar("k", k/*bSkillLevel*/);

			AddAffect(pkSk->dwVnum,
					  pkSk->bPointOn,
					  iAmount,
					  pkSk->dwAffectFlag,
					  iDur,
					  (long) pkSk->kDurationSPCostPoly.Eval(),
					  !bAdded);

			bAdded = true;
		}
		else
		{
			PointChange(pkSk->bPointOn, iAmount);
		}

		if (pkSk->bPointOn2 != POINT_NONE)
		{
			if (iDur2 > 0)
			{
				AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, !bAdded);
				bAdded = true;
			}
			else
			{
				PointChange(pkSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (pkSk->bPointOn3 != POINT_NONE)
		{
			if (iDur3 > 0)
			{
				AddAffect(pkSk->dwVnum, pkSk->bPointOn3, iAmount3, pkSk->dwAffectFlag3, iDur3, 0, !bAdded);
			}
			else
			{
				PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		// point 4
		if (pkSk->dwVnum == SKILL_ATTACK_UP)
		{
			AddAffect(pkSk->dwVnum,
				POINT_MAGIC_ATT,
				8 * GetSkillPower(pkSk->dwVnum) / 100,
				pkSk->dwAffectFlag,
				iDur,
				(long)pkSk->kDurationSPCostPoly.Eval(),
				!bAdded);
		}

		return BATTLE_NONE;
	}
}

#ifdef ENABLE_SKILL_FLAG_PARTY
struct FComputeSkillParty
{
	FComputeSkillParty(DWORD dwVnum, LPCHARACTER pkAttacker, BYTE bSkillLevel = 0)
		: m_dwVnum(dwVnum), m_pkAttacker(pkAttacker), m_bSkillLevel(bSkillLevel)
		{
		}

	void operator () (LPCHARACTER ch)
	{
		m_pkAttacker->ComputeSkill(m_dwVnum, ch, m_bSkillLevel);
	}

	DWORD m_dwVnum;
	LPCHARACTER m_pkAttacker;
	BYTE m_bSkillLevel;
};

int CHARACTER::ComputeSkillParty(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel)
{
	FComputeSkillParty f(dwVnum, pkVictim, bSkillLevel);
	if (GetParty() && GetParty()->GetNearMemberCount())
		GetParty()->ForEachNearMember(f);
	else
		f(this);

	return BATTLE_NONE;
}
#endif


int CHARACTER::ComputeSkill(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel)
{
	bool isPlayerSkillUsableOnMount = dwVnum == SKILL_FLAME_SPIRIT;
	const bool bCanUseHorseSkill = isPlayerSkillUsableOnMount || CanUseHorseSkill();

	if (false == bCanUseHorseSkill && true == IsRiding())
		return BATTLE_NONE;

	if (IsPolymorphed())
		return BATTLE_NONE;

	if (g_bSkillDisable)
		return BATTLE_NONE;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
		return BATTLE_NONE;

	if (bCanUseHorseSkill && pkSk->dwType != SKILL_TYPE_HORSE)
		if (!isPlayerSkillUsableOnMount)
			return BATTLE_NONE;

	if (!bCanUseHorseSkill && pkSk->dwType == SKILL_TYPE_HORSE)
		return BATTLE_NONE;

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY))
		pkVictim = this;

	if (!pkVictim)
	{
		if (test_server)
			sys_log(0, "ComputeSkill: %s Victim == null, skill %d", GetName(), dwVnum);

		return BATTLE_NONE;
	}

	if (pkSk->dwTargetRange && DISTANCE_SQRT(GetX() - pkVictim->GetX(), GetY() - pkVictim->GetY()) >= pkSk->dwTargetRange + 50)
	{
		if (test_server)
			sys_log(0, "ComputeSkill: Victim too far, skill %d : %s to %s (distance %u limit %u)",
					dwVnum,
					GetName(),
					pkVictim->GetName(),
					(long)DISTANCE_SQRT(GetX() - pkVictim->GetX(), GetY() - pkVictim->GetY()),
					pkSk->dwTargetRange);

		return BATTLE_NONE;
	}

	if (0 == bSkillLevel)
	{
		if ((bSkillLevel = GetSkillLevel(pkSk->dwVnum)) == 0)
		{
			if (test_server)
				sys_log(0, "ComputeSkill : name:%s vnum:%d  skillLevelBySkill : %d ", GetName(), pkSk->dwVnum, bSkillLevel);
			return BATTLE_NONE;
		}
	}

	if (pkVictim->IsAffectFlag(AFF_SKILL_DISPEL) && pkVictim->IsGoodAffect(dwVnum))
	{
		return BATTLE_NONE;
	}

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum, bSkillLevel) * pkSk->bMaxLevel / 100;

	pkSk->SetPointVar("k", k);
	pkSk->kSplashAroundDamageAdjustPoly.SetVar("k", k);

	if (pkSk->dwType == SKILL_TYPE_HORSE)
	{
		LPITEM pkBow, pkArrow;
		if (1 == GetArrowAndBow(&pkBow, &pkArrow, 1))
		{
			pkSk->SetPointVar("atk", CalcArrowDamage(this, pkVictim, pkBow, pkArrow, true, pkSk->dwVnum));
		}
		else
		{
			pkSk->SetPointVar("atk", CalcMeleeDamage(this, pkVictim, true, false));
		}
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
	{
		pkSk->SetPointVar("atk", CalcMeleeDamage(this, pkVictim, true, false));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		pkSk->SetPointVar("atk", CalcMagicDamage(this, pkVictim));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
	{
		LPITEM pkBow, pkArrow;
		if (1 == GetArrowAndBow(&pkBow, &pkArrow, 1))
		{
			pkSk->SetPointVar("atk", CalcArrowDamage(this, pkVictim, pkBow, pkArrow, true, pkSk->dwVnum));
		}
		else
		{
			pkSk->SetPointVar("atk", 0);
		}
	}

	if (pkSk->bPointOn == POINT_MOV_SPEED)
	{
		pkSk->SetPointVar("maxv", pkVictim->GetLimitPoint(POINT_MOV_SPEED));
	}

	pkSk->SetPointVar("gr", GetSkillMasterType(pkSk->dwVnum));
	pkSk->SetPointVar("sl", GetSkillLevel(pkSk->dwVnum));
	pkSk->SetPointVar("lv", GetLevel());
	pkSk->SetPointVar("iq", GetPoint(POINT_IQ));
	pkSk->SetPointVar("str", GetPoint(POINT_ST));
	pkSk->SetPointVar("dex", GetPoint(POINT_DX));
	pkSk->SetPointVar("con", GetPoint(POINT_HT));
	pkSk->SetPointVar("maxhp", pkVictim->GetMaxHP());
	pkSk->SetPointVar("maxsp", pkVictim->GetMaxSP());
	pkSk->SetPointVar("chain", 0);
	pkSk->SetPointVar("ar", CalcAttackRating(this, pkVictim));
	pkSk->SetPointVar("def", GetPoint(POINT_DEF_GRADE));
	pkSk->SetPointVar("odef", GetPoint(POINT_DEF_GRADE) - GetPoint(POINT_DEF_GRADE_BONUS));
	pkSk->SetPointVar("horse_level", GetHorseLevel());

	if (pkSk->bSkillAttrType != SKILL_ATTR_TYPE_NORMAL)
		OnMove(true);

	LPITEM pkWeapon = GetWear(WEAR_WEAPON);

	SetPolyVarForAttack(this, pkSk, pkWeapon);

	pkSk->kDurationPoly.SetVar("k", k/*bSkillLevel*/);
	pkSk->kDurationPoly2.SetVar("k", k/*bSkillLevel*/);
	pkSk->kDurationPoly3.SetVar("k", k/*bSkillLevel*/);

	int iAmount = (int) pkSk->kPointPoly.Eval();
	int iAmount2 = (int) pkSk->kPointPoly2.Eval();
	int iAmount3 = (int) pkSk->kPointPoly3.Eval();

	if (test_server && IsPC())
		sys_log(0, "iAmount: %d %d %d , atk:%f skLevel:%f k:%f GetSkillPower(%d) MaxLevel:%d Per:%f",
				iAmount, iAmount2, iAmount3,
				pkSk->kPointPoly.GetVar("atk"),
				pkSk->kPointPoly.GetVar("k"),
				k,
				GetSkillPower(pkSk->dwVnum, bSkillLevel),
				pkSk->bMaxLevel,
				pkSk->bMaxLevel/100
				);

	// ADD_GRANDMASTER_SKILL
	if (GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER)
	{
		iAmount = (int) pkSk->kMasterBonusPoly.Eval();
	}

	if (test_server && iAmount == 0 && pkSk->bPointOn != POINT_NONE)
	{
		ChatPacket(CHAT_TYPE_INFO, "ComputeSkill trying to add POINT with amount 0.");
	}
	// END_OF_ADD_GRANDMASTER_SKILL

	if (IsBuffSkill(pkSk->dwVnum))
	{
		if (pkVictim) {
			CAffect* aff = pkVictim->FindAffect(pkSk->dwVnum);
			if (aff)
			{
				pkVictim->RemoveAffect(aff);
			}
		}

		if (this != pkVictim)
		{
			int percent1 = 75;
			int percent2 = 75;
			int percent3 = 75;

			if (pkSk->dwVnum == SKILL_CURE)
			{
				percent1 = 110; // leczenie HP
				percent3 = 70; // absorpcja dmg
			}

			iAmount = iAmount * percent1 / 100;
			iAmount2 = iAmount2 * percent2 / 100;
			iAmount3 = iAmount3 * percent3 / 100;
		}
	}

	int iDur = (int)pkSk->kDurationPoly.Eval();
	int iDur2 = (int)pkSk->kDurationPoly2.Eval();
	int iDur3 = (int)pkSk->kDurationPoly3.Eval();

	bool canDurationBeExtended = IsBuffSkill(dwVnum) ||
		IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY);

	if (canDurationBeExtended)
	{
		int pointSkillDuration = GetPoint(POINT_SKILL_DURATION);
		if (iDur > 0)
		{
			iDur += iDur * pointSkillDuration / 100;
			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
		}

		if (iDur2 > 0)
		{
			iDur2 += iDur2 * pointSkillDuration / 100;
			iDur2 += GetPoint(POINT_PARTY_BUFFER_BONUS);
		}

		if (iDur3 > 0)
		{
			iDur3 += iDur3 * pointSkillDuration / 100;
			iDur3 += GetPoint(POINT_PARTY_BUFFER_BONUS);
		}
	}

	//sys_log(0, "XXX SKILL Calc %d Amount %d", dwVnum, iAmount);

	// REMOVE_BAD_AFFECT_BUG_FIX
	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_REMOVE_BAD_AFFECT))
	{
		if (number(1, 100) <= iAmount2)
		{
			pkVictim->RemoveBadAffect();
		}
	}
	// END_OF_REMOVE_BAD_AFFECT_BUG_FIX

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_ATTACK | SKILL_FLAG_USE_MELEE_DAMAGE | SKILL_FLAG_USE_MAGIC_DAMAGE) &&
		!(pkSk->dwVnum == SKILL_FLAME_SPIRIT && pkVictim == this) && !(pkSk->IsChargeSkill() && pkVictim == this))
	{
		bool bAdded = false;

		if (pkSk->bPointOn == POINT_HP && iAmount < 0)
		{
			int iAG = 0;

			FuncSplashDamage f(pkVictim->GetX(), pkVictim->GetY(), pkSk, this, iAmount, iAG, pkSk->lMaxHit, pkWeapon, m_bDisableCooltime, IsPC()?&m_SkillUseInfo[dwVnum]:NULL, GetSkillPower(dwVnum, bSkillLevel));
			if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
			{
				if (pkVictim->GetSectree())
					pkVictim->GetSectree()->ForEachAround(f);
			}
			else
			{
				f(pkVictim);
			}
		}
		else
		{
			if (IsPC())
				if (!(dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END))
					if (!m_bDisableCooltime && !m_SkillUseInfo[dwVnum].HitOnce(dwVnum) && dwVnum != SKILL_FLAME_SPIRIT)
					{
						return BATTLE_NONE;
					}

			if (iDur > 0)
			{
				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
		}

		if (pkSk->bPointOn2 != POINT_NONE && !pkSk->IsChargeSkill())
		{
			if (iDur2 > 0)
			{
				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, !bAdded);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, !bAdded, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pkVictim->PointChange(pkSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (pkSk->bPointOn3 != POINT_NONE && !pkSk->IsChargeSkill())
		{
			if (iDur3 > 0)
			{
				DWORD affectType = pkSk->dwVnum;
				if (affectType == SKILL_CURE)
				{
					// absorpcja inny affectType
					affectType = AFFECT_HEAVEN_PROTECTION;
				}

				LPCHARACTER addPointVictim = pkVictim;
				bool isThirdPointSelfOnly = IS_SET(pkSk->dwFlag, SKILL_FLAG_THIRD_POINT_SELFONLY);
				if (isThirdPointSelfOnly) {
					addPointVictim = this;
				}

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH) || isThirdPointSelfOnly)
					addPointVictim->AddAffect(affectType, pkSk->bPointOn3, iAmount3, pkSk->dwAffectFlag3, iDur3, 0, !bAdded);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, affectType, pkSk->bPointOn3, iAmount3, pkSk->dwAffectFlag3, iDur3, 0, !bAdded, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pkVictim->PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_DAMAGE;
	}
	else
	{
		if (dwVnum == SKILL_FLAME_SPIRIT)
		{
			pkSk->kDurationSPCostPoly.SetVar("k", k/*bSkillLevel*/);

			if (pkVictim == this)
				AddAffect(dwVnum,
						POINT_NONE, 0,
					AFF_SKILL_FLAME_SPIRIT,
						iDur,
						(long) pkSk->kDurationSPCostPoly.Eval(),
						true);

			return BATTLE_NONE;
		}

		bool bAdded = false;
		if (iDur > 0)
		{
			pkSk->kDurationSPCostPoly.SetVar("k", k/*bSkillLevel*/);

			if (pkSk->bPointOn2 != POINT_NONE)
			{
				pkVictim->RemoveAffect(pkSk->dwVnum);

				if (iDur2 > 0)
				{
					if (test_server)
						sys_log(0, "SKILL_AFFECT: %s %s Dur:%d To:%d Amount:%d",
								GetName(),
								pkSk->szName,
								iDur2,
								pkSk->bPointOn2,
								iAmount2);

					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, false);
				}
				else
				{
					pkVictim->PointChange(pkSk->bPointOn2, iAmount2);
				}

				DWORD affact_flag = pkSk->dwAffectFlag;

				// ADD_GRANDMASTER_SKILL
				if (pkSk->dwVnum == SKILL_STRONG_BODY)
				{
					if (GetUsedSkillMasterType(pkSk->dwVnum) < SKILL_GRAND_MASTER)
						affact_flag = AFF_SKILL_STRONG_BODY_WITH_FALL;
					else
						affact_flag = AFF_SKILL_STRONG_BODY;
				}
				// END_OF_ADD_GRANDMASTER_SKILL

				pkVictim->AddAffect(pkSk->dwVnum,
						pkSk->bPointOn,
						iAmount,
						affact_flag,
						iDur,
						(long) pkSk->kDurationSPCostPoly.Eval(),
						false);
			}
			else
			{
				if (test_server)
					sys_log(0, "SKILL_AFFECT: %s %s Dur:%d To:%d Amount:%d",
							GetName(),
							pkSk->szName,
							iDur,
							pkSk->bPointOn,
							iAmount);

				pkVictim->AddAffect(pkSk->dwVnum,
						pkSk->bPointOn,
						iAmount,
						pkSk->dwAffectFlag,
						iDur,
						(long) pkSk->kDurationSPCostPoly.Eval(),
						// ADD_GRANDMASTER_SKILL
						!bAdded);
				// END_OF_ADD_GRANDMASTER_SKILL
			}

			bAdded = true;
		}
		else
		{
			if (!pkSk->IsChargeSkill())
				pkVictim->PointChange(pkSk->bPointOn, iAmount);

			if (pkSk->bPointOn2 != POINT_NONE)
			{
				pkVictim->RemoveAffect(pkSk->dwVnum);

				if (iDur2 > 0)
				{
					if (pkSk->IsChargeSkill())
						pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, AFF_CHARGE_DASH, iDur2, 0, false);
					else
						pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, false);
				}
				else
				{
					pkVictim->PointChange(pkSk->bPointOn2, iAmount2);
				}

			}
		}

		if (pkSk->bPointOn3 != POINT_NONE && !pkSk->IsChargeSkill() && CanSkillAddThirdPoint(pkSk, this, pkVictim))
		{
			if (iDur3 > 0)
			{
				DWORD affectType = pkSk->dwVnum;
				if (affectType == SKILL_CURE)
				{
					// absorpcja inny affectType
					affectType = AFFECT_HEAVEN_PROTECTION;
				}

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(affectType, pkSk->bPointOn3, iAmount3, pkSk->dwAffectFlag3, iDur3, 0, !bAdded);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, affectType, pkSk->bPointOn3, iAmount3, pkSk->dwAffectFlag3, iDur3, 0, !bAdded, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pkVictim->PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		// point 4
		if (pkSk->dwVnum == SKILL_ATTACK_UP)
		{
			int value = 8 * GetSkillPower(pkSk->dwVnum) / 100;
			if (this != pkVictim)
				value = value * 75 / 100;
			pkVictim->AddAffect(pkSk->dwVnum,
				POINT_MAGIC_ATT,
				value, // 125 bo to buff na kogos innego
				pkSk->dwAffectFlag,
				iDur,
				(long)pkSk->kDurationSPCostPoly.Eval(),
				!bAdded);
		}

		return BATTLE_NONE;
	}
}

bool IsSkillUsableWhileRiding(DWORD dwSkillVnum)
{
	switch (dwSkillVnum) {
	case SKILL_SPRINT:
		return true;
	default:
		return false;
	}
}

bool CHARACTER::UseSkill(DWORD dwVnum, LPCHARACTER pkVictim, bool bUseGrandMaster)
{
	if (false == CanUseSkill(dwVnum))
		return false;

	// NO_GRANDMASTER
	if (test_server)
	{
		if (quest::CQuestManager::instance().GetEventFlag("no_grand_master"))
		{
			bUseGrandMaster = false;
		}
	}
	// END_OF_NO_GRANDMASTER

	if (g_bSkillDisable)
		return false;

	if (IsObserverMode())
		return false;

	if (!CanMove())
		return false;

	if (IsPolymorphed())
		return false;


	if (dwVnum == SKILL_HORSE_SUMMON)
	{
		if (GetSkillLevel(dwVnum) == 0)
			return false;

		if (GetHorseLevel() <= 0)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No Horse here. Ask the Stable Boy."));
		else
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Please use an item to call a Horse."));

		return true;
	}

	const bool bCanUseHorseSkill = CanUseHorseSkill();

	if (IsRiding())
	{
		if (dwVnum != SKILL_SPRINT && !bCanUseHorseSkill)
			return false;
	}

	CSkillProto * pkSk = CSkillManager::instance().Get(dwVnum);
	sys_log(1, "%s: USE_SKILL: %d pkVictim %p", GetName(), dwVnum, get_pointer(pkVictim));

	if (!pkSk)
		return false;

	if (bCanUseHorseSkill && pkSk->dwType != SKILL_TYPE_HORSE && !IsSkillUsableWhileRiding(dwVnum))
		return BATTLE_NONE;

	if (!bCanUseHorseSkill && pkSk->dwType == SKILL_TYPE_HORSE)
		return BATTLE_NONE;

	if (IsBuffSkill(dwVnum) && pkVictim)
	{
		if (pkVictim->IsNPC())
			return false;

		if (pkVictim->IsPC())
		{
			if (pkVictim->GetEmpire() != GetEmpire())
				return false;
		}
	}

	if (GetSkillLevel(dwVnum) == 0 && dwVnum != SKILL_SPRINT)
		return false;

	// NO_GRANDMASTER
	if (GetSkillMasterType(dwVnum) < SKILL_GRAND_MASTER)
		bUseGrandMaster = false;
	// END_OF_NO_GRANDMASTER

	// MINING
	if (GetWear(WEAR_WEAPON) && (GetWear(WEAR_WEAPON)->GetType() == ITEM_ROD || GetWear(WEAR_WEAPON)->GetType() == ITEM_PICK || GetWear(WEAR_WEAPON)->GetType() == ITEM_HERB_KNIFE) && dwVnum != SKILL_SPRINT)
		return false;
	// END_OF_MINING

	m_SkillUseInfo[dwVnum].TargetVIDMap.clear();

	if (pkSk->IsChargeSkill())
	{
		if ((IsAffectFlag(AFF_CHARGE_DASH)) || (pkVictim && (pkVictim != this)))
		{
			if (!pkVictim)
				return false;

			if (!IsAffectFlag(AFF_CHARGE_DASH))
			{
				if (!UseSkill(dwVnum, this))
					return false;
			}

			m_SkillUseInfo[dwVnum].SetMainTargetVID(pkVictim->GetVID());

			ComputeSkill(dwVnum, pkVictim);
			RemoveAffect(dwVnum);
			return true;
		}
	}

	if (dwVnum == SKILL_COMBO)
	{
		if (m_bComboIndex)
			m_bComboIndex = 0;
		else
			m_bComboIndex = GetSkillLevel(SKILL_COMBO);

		ChatPacket(CHAT_TYPE_COMMAND, "combo %d", m_bComboIndex);
		return true;
	}

	if (dwVnum == SKILL_SPRINT) {
		if (!IsSprint()) {

			if (!HasPlayerData())
				return false;

			if (GetStamina() < 100)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("USE_SKILL_STAMINA%d"), 100);
				return false;
			}

			if (!IsRiding() && GetPoint(POINT_MOV_SPEED) >= 150)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("USE_SKILL_STAMINA_TOO_HIGH_MOVEMENT%d"), 150);
				return false;
			}

			if (IsWalking())
			{
				return false;
			}

			if (IsAffectFlag(AFF_QUEST_BOX))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_SPRINT_WHILE_CARRY_ITEM"));
				return false;
			}

			if (IsAffectFlag(AFF_SLOW))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("CANNOT_SPRINT_WHILE_SLOW"));
				return false;
			}

			if (IsRiding() && GetHorseHealthGrade() <= 1 && !FindAffect(AFFECT_MOUNT))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("HORSE_IS_HUNGRY_CANNOT_SPRINT"));
				return false;
			}

			if (playerData->m_dwSkillUseLastTime[SKILL_SPRINT] + 800 >= get_dword_time()) // 800ms delay between activation
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("NEXT_ACTION_WAIT_NO_FORMAT"));
				return false;
			}

			playerData->m_dwSkillUseLastTime[SKILL_SPRINT] = get_dword_time();
		}


		SetSprint(!m_bIsSprint);
		return true;
	}

	if (dwVnum >= SKILL_ARCHER_START && dwVnum <= SKILL_ARCHER_END && dwVnum != SKILL_FEATHER_WALK)
	{
		// reset targets on skill use
		// it prevents from visual bugs
		// and also there was a problem with repetitive shot conflicting
		// with cheatwarden
		m_vec_dwFlyTargets.clear();
		TPacketGCFlyClearTargeting pack;
		pack.bHeader = HEADER_GC_CLEAR_FLY_SHOOT_TARGETING;
		pack.dwShooterVID = GetVID();
		PacketAround(&pack, sizeof(TPacketGCFlyClearTargeting), NULL);
	}

	if ((0 != pkSk->dwAffectFlag || pkSk->dwVnum == SKILL_FLAME_SPIRIT) && (pkSk->dwFlag & SKILL_FLAG_TOGGLE) && RemoveAffect(pkSk->dwVnum))
	{
		return true;
	}

	if (IsAffectFlag(AFF_REVIVE_INVISIBLE))
		RemoveAffect(AFFECT_REVIVE_INVISIBLE);

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum) * pkSk->bMaxLevel / 100;

	pkSk->SetPointVar("k", k);
	pkSk->kSplashAroundDamageAdjustPoly.SetVar("k", k);

	pkSk->kCooldownPoly.SetVar("k", k);
	int iCooltime = (int) pkSk->kCooldownPoly.Eval();
	int lMaxHit = pkSk->lMaxHit ? pkSk->lMaxHit : -1;

	pkSk->SetSPCostVar("k", k);

	DWORD dwCur = get_dword_time();

	if (dwVnum == SKILL_TERROR && m_SkillUseInfo[dwVnum].bUsed && m_SkillUseInfo[dwVnum].dwNextSkillUsableTime > dwCur )
	{
		sys_log(0, " SKILL_TERROR's Cooltime is not delta over %u", m_SkillUseInfo[dwVnum].dwNextSkillUsableTime  - dwCur );
		return false;
	}

	int iNeededSP = 0;

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_HP_AS_COST))
	{
		pkSk->SetSPCostVar("maxhp", GetMaxHP());
		pkSk->SetSPCostVar("v", GetHP());
		iNeededSP = (int) pkSk->kSPCostPoly.Eval();

		// ADD_GRANDMASTER_SKILL
		if (GetSkillMasterType(dwVnum) >= SKILL_GRAND_MASTER && bUseGrandMaster)
		{
			iNeededSP = (int) pkSk->kGrandMasterAddSPCostPoly.Eval();
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		if (GetHP() < iNeededSP)
			return false;

		PointChange(POINT_HP, -iNeededSP);
	}
	else
	{
		// SKILL_FOMULA_REFACTORING
		pkSk->SetSPCostVar("maxhp", GetMaxHP());
		pkSk->SetSPCostVar("maxv", GetMaxSP());
		pkSk->SetSPCostVar("v", GetSP());
		pkSk->SetSPCostVar("lv", GetLevel());

		iNeededSP = (int) pkSk->kSPCostPoly.Eval();

		if (GetSkillMasterType(dwVnum) >= SKILL_GRAND_MASTER && bUseGrandMaster)
		{
			iNeededSP = (int) pkSk->kGrandMasterAddSPCostPoly.Eval();
		}
		// END_OF_SKILL_FOMULA_REFACTORING

		if (GetSP() < iNeededSP)
			return false;

		if (test_server)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s FP-Consumption: %d"), pkSk->szName, iNeededSP);

		PointChange(POINT_SP, -iNeededSP);
	}

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY))
		pkVictim = this;
#ifdef ENABLE_SKILL_FLAG_PARTY
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_PARTY))
		pkVictim = this;
#endif

	if ((pkSk->dwVnum == SKILL_FLAME_SPIRIT) || (pkSk->IsChargeSkill() && !IsAffectFlag(AFF_CHARGE_DASH) && !pkVictim))
	{
		pkVictim = this;
	}

	int iSplashCount = 1;

	if (false == m_bDisableCooltime)
	{
		if (false ==
				m_SkillUseInfo[dwVnum].UseSkill(
					bUseGrandMaster,
				   	(NULL != pkVictim && SKILL_HORSE_WILDATTACK != dwVnum) ? pkVictim->GetVID() : VID(),
				   	ComputeCooltime(iCooltime * 1000),
				   	iSplashCount,
				   	lMaxHit))
		{
			if (test_server)
				ChatPacket(CHAT_TYPE_NOTICE, "cooltime not finished %s %d", pkSk->szName, iCooltime);

			return false;
		}
	}

	// anti cheat fix error
	if (HasPlayerData() && dwVnum == SKILL_HORSE_WILDATTACK)
	{
		playerData->m_iMoveBonusPoint = playerData->GetAllowedMovementDistance(this, true) * 2 + 1500;
	}

	if (dwVnum == SKILL_LIGHTNING_CLAW)
	{
		ResetChainLightningIndex();
		AddChainLightningExcept(pkVictim);
	}

	if (dwVnum == SKILL_STEALTH) {
		if (number(1, 40) <= GetSkillLevel(SKILL_STEALTH))
			ForgetMyAttacker(false);
	}

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY))
		ComputeSkill(dwVnum, this);
#ifdef ENABLE_SKILL_FLAG_PARTY
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_PARTY))
		ComputeSkillParty(dwVnum, this);
#endif
	else if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_ATTACK))
		ComputeSkill(dwVnum, pkVictim);
	else if (dwVnum == SKILL_SUMMON_LIGHTNING)
		ComputeSkill(dwVnum, pkVictim);
	else if (dwVnum == SKILL_FLAME_SPIRIT || pkSk->IsChargeSkill())
		ComputeSkill(dwVnum, pkVictim);

	if (dwVnum == SKILL_AMBUSH &&
		pkVictim && pkVictim->IsNPC()
		&& GetWear(WEAR_WEAPON) && GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER) {
		int iDuration = 9 + GetSkillLevel(SKILL_AMBUSH);

		int grade = GetSkillMasterType(SKILL_AMBUSH);
		int multiplierByGrade[4] = { 5, 10, 11, 12 };
		int iVal = 8 + GetSkillLevel(SKILL_AMBUSH) * multiplierByGrade[grade] / 10;
		bool addBloodthirst = false;

		CAffect* affect = FindAffect(AFFECT_BLOODTHIRST);
		if (affect && affect->lDuration > 0)
		{
			addBloodthirst = true;

			int maxStackDamage = iVal * 30 / 100 + GetPoint(POINT_DX) / 5;
			int damagePerStack = maxStackDamage / 2;
			int maxValue = iVal + maxStackDamage;

			iVal = MIN(affect->lApplyValue + damagePerStack, maxValue);
		}
		AddAffect(AFFECT_BLOODTHIRST, POINT_DAGGER_ATT_GRADE_MONSTER, iVal, AFF_BLOODTHIRST, iDuration, 0, true);
	}

	m_dwLastSkillTime = get_dword_time();

	return true;
}

int CHARACTER::GetUsedSkillMasterType(DWORD dwVnum)
{
	const TSkillUseInfo& rInfo = m_SkillUseInfo[dwVnum];

	if (GetSkillMasterType(dwVnum) < SKILL_GRAND_MASTER)
		return GetSkillMasterType(dwVnum);

	if (rInfo.isGrandMaster)
		return GetSkillMasterType(dwVnum);

	return MIN(GetSkillMasterType(dwVnum), SKILL_MASTER);
}

int CHARACTER::GetSkillMasterType(DWORD dwVnum) const
{
	if (!IsPC())
		return 0;

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("%s skill vnum overflow %u", GetName(), dwVnum);
		return 0;
	}

	return m_pSkillLevels ? m_pSkillLevels[dwVnum].bMasterType:SKILL_NORMAL;
}

int CHARACTER::GetSkillPower(DWORD dwVnum, BYTE bLevel) const
{
	if (dwVnum >= SKILL_LANGUAGE1 && dwVnum <= SKILL_LANGUAGE3 && IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE))
	{
		return 100;
	}

	if (dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END)
	{
		if (GetGuild())
			return 100 * GetGuild()->GetSkillLevel(dwVnum) / 7 / 7;
		else
			return 0;
	}

	if (bLevel)
	{
		//SKILL_POWER_BY_LEVEL
		return GetSkillPowerByLevel(bLevel, true);
		//END_SKILL_POWER_BY_LEVEL;
	}

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("%s skill vnum overflow %u", GetName(), dwVnum);
		return 0;
	}

	//SKILL_POWER_BY_LEVEL
	return GetSkillPowerByLevel(GetSkillLevel(dwVnum));
	//SKILL_POWER_BY_LEVEL
}

int CHARACTER::GetSkillLevel(DWORD dwVnum) const
{
	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("%s skill vnum overflow %u", GetName(), dwVnum);
		sys_log(0, "%s skill vnum overflow %u", GetName(), dwVnum);
		return 0;
	}

	return MIN(SKILL_MAX_LEVEL, m_pSkillLevels ? m_pSkillLevels[dwVnum].bLevel : 0);
}

EVENTFUNC(skill_muyoung_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "skill_muyoung_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->IsAffectFlag(AFF_SKILL_FLAME_SPIRIT))
	{
		ch->StopMuyeongEvent();
		return 0;
	}

	LPCHARACTER victim = ch->GetTarget();

	LPSECTREE ch_sectree = ch->GetSectree();

	if (!victim && ch_sectree)
	{
		FFindNearVictim f(ch, ch);
		ch_sectree->ForEachAround(f);
		victim = f.GetVictim();
	}

	if (victim && ch->DistanceTo(victim) <= 800 &&
		battle_is_attackable(ch, victim) &&
		!victim->IsInvisible())
	{
		ch->CreateFly(FLY_SKILL_FLAME_SPIRIT, victim);
		ch->ComputeSkill(SKILL_FLAME_SPIRIT, victim);
	}
	return PASSES_PER_SEC(3);
}

void CHARACTER::StartMuyeongEvent()
{
	if (m_pkMuyeongEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;
	m_pkMuyeongEvent = event_create(skill_muyoung_event, info, PASSES_PER_SEC(1));
}

void CHARACTER::StopMuyeongEvent()
{
	event_cancel(&m_pkMuyeongEvent);
}

void CHARACTER::SkillLearnWaitMoreTimeMessage(DWORD ms)
{
	//const char* str = "";

	if (ms < 3 * 60)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I am burning inside, but it is calming down my body. My Chi has to stabilise."));
	else if (ms < 5 * 60)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("A little slow, but steady... Without stopping!"));
	else if (ms < 10 * 60)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Yes, that feels great. My body is full of Chi."));
	else if (ms < 30 * 60)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I have read it! Now the Chi will spread through my body."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("The training is completed."));
	}
	else if (ms < 1 * 3600)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I am on the last page of the book. The training is nearly finished!"));
	else if (ms < 2 * 3600)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Nearly finished! Just a little bit more to go!"));
	else if (ms < 3 * 3600)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Eureka! I have nearly finished reading it!"));
	else if (ms < 6 * 3600)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Only a few more pages and then I'll have read everything."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I feel refreshed."));
	}
	else if (ms < 12 * 3600)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Now I understand it!"));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("Okay I have to stay concentrated!"));
	}
	else if (ms < 18 * 3600)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I keep reading the same line over and over again."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("I do not want to learn any more."));
	}
	else //if (ms < 2 * 86400)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("It is a lot more complicated and more difficult to understand than I thought."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT("It's hard for me to concentrate. I should take a break."));
	}

	ChatPacket(CHAT_TYPE_INFO, LC_TEXT("LEARN_SKILL_DELAY"), seconds_to_smart_time(ms));
	/*
	   str = "30%";
	   else if (ms < 3 * 86400)
	   str = "10%";
	   else if (ms < 4 * 86400)
	   str = "5%";
	   else
	   str = "0%";*/

	//ChatPacket(CHAT_TYPE_TALKING, "%s", str);
}

void CHARACTER::DisableCooltime()
{
	m_bDisableCooltime = true;
}

bool CHARACTER::HasMobSkill() const
{
	return CountMobSkill() > 0;
}

size_t CHARACTER::CountMobSkill() const
{
	if (!m_pkMobData)
		return 0;

	size_t c = 0;

	for (size_t i = 0; i < MOB_SKILL_MAX_NUM; ++i)
		if (m_pkMobData->m_table.Skills[i].dwVnum)
			++c;

	return c;
}

const TMobSkillInfo* CHARACTER::GetMobSkill(unsigned int idx) const
{
	if (idx >= MOB_SKILL_MAX_NUM)
		return NULL;

	if (!m_pkMobData)
		return NULL;

	if (0 == m_pkMobData->m_table.Skills[idx].dwVnum)
		return NULL;

	return &m_pkMobData->m_mobSkillInfo[idx];
}

bool CHARACTER::CanUseMobSkill(unsigned int idx) const
{
	const TMobSkillInfo* pInfo = GetMobSkill(idx);

	if (!pInfo)
		return false;

	if (m_adwMobSkillCooltime[idx] > get_dword_time())
		return false;

	if (number(0, 1))
		return false;

	return true;
}

EVENTINFO(mob_skill_event_info)
{
	DynamicCharacterPtr ch;
	PIXEL_POSITION pos;
	DWORD vnum;
	int index;
	BYTE level;

	mob_skill_event_info()
	: ch()
	, pos()
	, vnum(0)
	, index(0)
	, level(0)
	{
	}
};

EVENTFUNC(mob_skill_hit_event)
{
	mob_skill_event_info * info = dynamic_cast<mob_skill_event_info *>( event->info );

	if ( info == NULL )
	{
		sys_err( "mob_skill_event_info> <Factor> Null pointer" );
		return 0;
	}

	// <Factor>
	LPCHARACTER ch = info->ch;
	if (ch == NULL) {
		return 0;
	}

	ch->ComputeSkillAtPosition(info->vnum, info->pos, info->level);
	ch->m_mapMobSkillEvent.erase(info->index);

	return 0;
}

bool CHARACTER::UseMobSkill(unsigned int idx)
{
	if (IsPC())
		return false;

	const TMobSkillInfo* pInfo = GetMobSkill(idx);

	if (!pInfo)
		return false;

	DWORD dwVnum = pInfo->dwSkillVnum;
	CSkillProto * pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
		return false;

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum, pInfo->bSkillLevel) * pkSk->bMaxLevel / 100;

	pkSk->kCooldownPoly.SetVar("k", k);
	int iCooltime = (int) (pkSk->kCooldownPoly.Eval() * 1000);

	m_adwMobSkillCooltime[idx] = get_dword_time() + iCooltime;

	sys_log(0, "USE_MOB_SKILL: %s idx %d vnum %u cooltime %d", GetName(), idx, dwVnum, iCooltime);

	if (m_pkMobData->m_mobSkillInfo[idx].vecSplashAttack.empty())
	{
		sys_err("No skill hit data for mob %s index %d", GetName(), idx);
		return false;
	}

	for (size_t i = 0; i < m_pkMobData->m_mobSkillInfo[idx].vecSplashAttack.size(); i++)
	{
		PIXEL_POSITION pos = GetXYZ();
		const TMobSplashAttackInfo& rInfo = m_pkMobData->m_mobSkillInfo[idx].vecSplashAttack[i];

		if (rInfo.dwHitDistance)
		{
			float fx, fy;
			GetDeltaByDegree(GetRotation(), rInfo.dwHitDistance, &fx, &fy);
			pos.x += (long) fx;
			pos.y += (long) fy;
		}

		if (rInfo.dwTiming)
		{
			if (test_server)
				sys_log(0, "               timing %ums", rInfo.dwTiming);

			mob_skill_event_info* info = AllocEventInfo<mob_skill_event_info>();

			info->ch = this;
			info->pos = pos;
			info->level = pInfo->bSkillLevel;
			info->vnum = dwVnum;
			info->index = i;

			// <Factor> Cancel existing event first
			itertype(m_mapMobSkillEvent) it = m_mapMobSkillEvent.find(i);
			if (it != m_mapMobSkillEvent.end()) {
				LPEVENT existing = it->second;
				event_cancel(&existing);
				m_mapMobSkillEvent.erase(it);
			}

			m_mapMobSkillEvent.emplace(i, event_create(mob_skill_hit_event, info, PASSES_PER_SEC(rInfo.dwTiming) / 1000));
		}
		else
		{
			ComputeSkillAtPosition(dwVnum, pos, pInfo->bSkillLevel);
		}
	}

	return true;
}

void CHARACTER::ResetMobSkillCooltime()
{
	memset(m_adwMobSkillCooltime, 0, sizeof(m_adwMobSkillCooltime));
}

bool CHARACTER::IsUsableSkillMotion(DWORD dwMotionIndex) const
{
	DWORD selfJobGroup = (GetJob()+1) * 10 + GetSkillGroup();
#ifdef ENABLE_WOLFMAN_CHARACTER
	const DWORD SKILL_NUM = 176;
#else
	const DWORD SKILL_NUM = 158;
#endif
	static DWORD s_anSkill2JobGroup[SKILL_NUM] = {
		0, // common_skill 0
		11, // job_skill 1
		11, // job_skill 2
		11, // job_skill 3
		11, // job_skill 4
		11, // job_skill 5
		11, // job_skill 6
		0, // common_skill 7
		0, // common_skill 8
		0, // common_skill 9
		0, // common_skill 10
		0, // common_skill 11
		0, // common_skill 12
		0, // common_skill 13
		0, // common_skill 14
		0, // common_skill 15
		12, // job_skill 16
		12, // job_skill 17
		12, // job_skill 18
		12, // job_skill 19
		12, // job_skill 20
		12, // job_skill 21
		0, // common_skill 22
		0, // common_skill 23
		0, // common_skill 24
		0, // common_skill 25
		0, // common_skill 26
		0, // common_skill 27
		0, // common_skill 28
		0, // common_skill 29
		0, // common_skill 30
		21, // job_skill 31
		21, // job_skill 32
		21, // job_skill 33
		21, // job_skill 34
		21, // job_skill 35
		21, // job_skill 36
		0, // common_skill 37
		0, // common_skill 38
		0, // common_skill 39
		0, // common_skill 40
		0, // common_skill 41
		0, // common_skill 42
		0, // common_skill 43
		0, // common_skill 44
		0, // common_skill 45
		22, // job_skill 46
		22, // job_skill 47
		22, // job_skill 48
		22, // job_skill 49
		22, // job_skill 50
		22, // job_skill 51
		0, // common_skill 52
		0, // common_skill 53
		0, // common_skill 54
		0, // common_skill 55
		0, // common_skill 56
		0, // common_skill 57
		0, // common_skill 58
		0, // common_skill 59
		0, // common_skill 60
		31, // job_skill 61
		31, // job_skill 62
		31, // job_skill 63
		31, // job_skill 64
		31, // job_skill 65
		31, // job_skill 66
		0, // common_skill 67
		0, // common_skill 68
		0, // common_skill 69
		0, // common_skill 70
		0, // common_skill 71
		0, // common_skill 72
		0, // common_skill 73
		0, // common_skill 74
		0, // common_skill 75
		32, // job_skill 76
		32, // job_skill 77
		32, // job_skill 78
		32, // job_skill 79
		32, // job_skill 80
		32, // job_skill 81
		0, // common_skill 82
		0, // common_skill 83
		0, // common_skill 84
		0, // common_skill 85
		0, // common_skill 86
		0, // common_skill 87
		0, // common_skill 88
		0, // common_skill 89
		0, // common_skill 90
		41, // job_skill 91
		41, // job_skill 92
		41, // job_skill 93
		41, // job_skill 94
		41, // job_skill 95
		41, // job_skill 96
		0, // common_skill 97
		0, // common_skill 98
		0, // common_skill 99
		0, // common_skill 100
		0, // common_skill 101
		0, // common_skill 102
		0, // common_skill 103
		0, // common_skill 104
		0, // common_skill 105
		42, // job_skill 106
		42, // job_skill 107
		42, // job_skill 108
		42, // job_skill 109
		42, // job_skill 110
		42, // job_skill 111
		0, // common_skill 112
		0, // common_skill 113
		0, // common_skill 114
		0, // common_skill 115
		0, // common_skill 116
		0, // common_skill 117
		0, // common_skill 118
		0, // common_skill 119
		0, // common_skill 120
		0, // common_skill 121
		0, // common_skill 122
		0, // common_skill 123
		0, // common_skill 124
		0, // common_skill 125
		0, // common_skill 126
		0, // common_skill 127
		0, // common_skill 128
		0, // common_skill 129
		0, // common_skill 130
		0, // common_skill 131
		0, // common_skill 132
		0, // common_skill 133
		0, // common_skill 134
		0, // common_skill 135
		0, // common_skill 136
		0, // job_skill 137
		0, // job_skill 138
		0, // job_skill 139
		0, // job_skill 140
		0, // common_skill 141
		0, // common_skill 142
		0, // common_skill 143
		0, // common_skill 144
		0, // common_skill 145
		0, // common_skill 146
		0, // common_skill 147
		0, // common_skill 148
		0, // common_skill 149
		0, // common_skill 150
		0, // common_skill 151
		0, // job_skill 152
		0, // job_skill 153
		0, // job_skill 154
		0, // job_skill 155
		0, // job_skill 156
		0, // job_skill 157
#ifdef ENABLE_WOLFMAN_CHARACTER
		0, // empty(reserved) 158
		0, // empty(reserved) 159
		0, // empty(reserved) 160
		0, // empty(reserved) 161
		0, // empty(reserved) 162
		0, // empty(reserved) 163
		0, // empty(reserved) 164
		0, // empty(reserved) 165
		0, // empty(reserved) 166
		0, // empty(reserved) 167
		0, // empty(reserved) 168
		0, // empty(reserved) 169
		51, // job_skill(WOLFMAN SKILL) 170
		51, // job_skill(WOLFMAN SKILL) 171
		51, // job_skill(WOLFMAN SKILL) 172
		51, // job_skill(WOLFMAN SKILL) 173
		51, // job_skill(WOLFMAN SKILL) 174
		51, // job_skill(WOLFMAN SKILL) 175
#endif
	}; // s_anSkill2JobGroup

	const DWORD MOTION_MAX_NUM 	= 124;
#ifdef ENABLE_WOLFMAN_CHARACTER
	const DWORD SKILL_LIST_MAX_COUNT	= 6;
#else
	const DWORD SKILL_LIST_MAX_COUNT	= 5;
#endif
	static DWORD s_anMotion2SkillVnumList[MOTION_MAX_NUM][SKILL_LIST_MAX_COUNT] =
	{
		{   0,		0,			0,			0,			0		}, //  0

#ifdef ENABLE_WOLFMAN_CHARACTER
		{   5,		1,			31,			61,			91,	170		}, //  1
		{   5,		2,			32,			62,			92,	171		}, //  2
		{   5,		3,			33,			63,			93,	172		}, //  3
		{   5,		4,			34,			64,			94,	173		}, //  4
		{   5,		5,			35,			65,			95,	174		}, //  5
		{   5,		6,			36,			66,			96,	175		}, //  6
#else
		{   4,		1,			31,			61,			91		}, //  1
		{   4,		2,			32,			62,			92		}, //  2
		{   4,		3,			33,			63,			93		}, //  3
		{   4,		4,			34,			64,			94		}, //  4
		{   4,		5,			35,			65,			95		}, //  5
		{   4,		6,			36,			66,			96		}, //  6
#endif
		{   0,		0,			0,			0,			0		}, //  7
		{   0,		0,			0,			0,			0		}, //  8

		{   0,		0,			0,			0,			0		}, //  9
		{   0,		0,			0,			0,			0		}, //  10
		{   0,		0,			0,			0,			0		}, //  11
		{   0,		0,			0,			0,			0		}, //  12
		{   0,		0,			0,			0,			0		}, //  13
		{   0,		0,			0,			0,			0		}, //  14
		{   0,		0,			0,			0,			0		}, //  15

		{   4,		16,			46,			76,			106		}, //  16
		{   4,		17,			47,			77,			107		}, //  17
		{   4,		18,			48,			78,			108		}, //  18
		{   4,		19,			49,			79,			109		}, //  19
		{   4,		20,			50,			80,			110		}, //  20
		{   4,		21,			51,			81,			111		}, //  21
		{   0,		0,			0,			0,			0		}, //  22
		{   0,		0,			0,			0,			0		}, //  23

		{   0,		0,			0,			0,			0		}, //  24
		{   0,		0,			0,			0,			0		}, //  25

#ifdef ENABLE_WOLFMAN_CHARACTER
		{   5,		1,			31,			61,			91,	170		}, //  26
		{   5,		2,			32,			62,			92,	171		}, //  27
		{   5,		3,			33,			63,			93,	172		}, //  28
		{   5,		4,			34,			64,			94,	173		}, //  29
		{   5,		5,			35,			65,			95,	174		}, //  30
		{   5,		6,			36,			66,			96,	175		}, //  31
#else
		{   4,		1,			31,			61,			91		}, //  26
		{   4,		2,			32,			62,			92		}, //  27
		{   4,		3,			33,			63,			93		}, //  28
		{   4,		4,			34,			64,			94		}, //  29
		{   4,		5,			35,			65,			95		}, //  30
		{   4,		6,			36,			66,			96		}, //  31
#endif
		{   0,		0,			0,			0,			0		}, //  32
		{   0,		0,			0,			0,			0		}, //  33

		{   0,		0,			0,			0,			0		}, //  34
		{   0,		0,			0,			0,			0		}, //  35
		{   0,		0,			0,			0,			0		}, //  36
		{   0,		0,			0,			0,			0		}, //  37
		{   0,		0,			0,			0,			0		}, //  38
		{   0,		0,			0,			0,			0		}, //  39
		{   0,		0,			0,			0,			0		}, //  40

		{   4,		16,			46,			76,			106		}, //  41
		{   4,		17,			47,			77,			107		}, //  42
		{   4,		18,			48,			78,			108		}, //  43
		{   4,		19,			49,			79,			109		}, //  44
		{   4,		20,			50,			80,			110		}, //  45
		{   4,		21,			51,			81,			111		}, //  46
		{   0,		0,			0,			0,			0		}, //  47
		{   0,		0,			0,			0,			0		}, //  48

		{   0,		0,			0,			0,			0		}, //  49
		{   0,		0,			0,			0,			0		}, //  50

#ifdef ENABLE_WOLFMAN_CHARACTER
		{   5,		1,			31,			61,			91,	170		}, //  51
		{   5,		2,			32,			62,			92,	171		}, //  52
		{   5,		3,			33,			63,			93,	172		}, //  53
		{   5,		4,			34,			64,			94,	173		}, //  54
		{   5,		5,			35,			65,			95,	174		}, //  55
		{   5,		6,			36,			66,			96,	175		}, //  56
#else
		{   4,		1,			31,			61,			91		}, //  51
		{   4,		2,			32,			62,			92		}, //  52
		{   4,		3,			33,			63,			93		}, //  53
		{   4,		4,			34,			64,			94		}, //  54
		{   4,		5,			35,			65,			95		}, //  55
		{   4,		6,			36,			66,			96		}, //  56
#endif
		{   0,		0,			0,			0,			0		}, //  57
		{   0,		0,			0,			0,			0		}, //  58

		{   0,		0,			0,			0,			0		}, //  59
		{   0,		0,			0,			0,			0		}, //  60
		{   0,		0,			0,			0,			0		}, //  61
		{   0,		0,			0,			0,			0		}, //  62
		{   0,		0,			0,			0,			0		}, //  63
		{   0,		0,			0,			0,			0		}, //  64
		{   0,		0,			0,			0,			0		}, //  65

		{   4,		16,			46,			76,			106		}, //  66
		{   4,		17,			47,			77,			107		}, //  67
		{   4,		18,			48,			78,			108		}, //  68
		{   4,		19,			49,			79,			109		}, //  69
		{   4,		20,			50,			80,			110		}, //  70
		{   4,		21,			51,			81,			111		}, //  71
		{   0,		0,			0,			0,			0		}, //  72
		{   0,		0,			0,			0,			0		}, //  73

		{   0,		0,			0,			0,			0		}, //  74
		{   0,		0,			0,			0,			0		}, //  75

#ifdef ENABLE_WOLFMAN_CHARACTER
		{   5,		1,			31,			61,			91,	170		}, //  76
		{   5,		2,			32,			62,			92,	171		}, //  77
		{   5,		3,			33,			63,			93,	172		}, //  78
		{   5,		4,			34,			64,			94,	173		}, //  79
		{   5,		5,			35,			65,			95,	174		}, //  80
		{   5,		6,			36,			66,			96,	175		}, //  81
#else
		{   4,		1,			31,			61,			91		}, //  76
		{   4,		2,			32,			62,			92		}, //  77
		{   4,		3,			33,			63,			93		}, //  78
		{   4,		4,			34,			64,			94		}, //  79
		{   4,		5,			35,			65,			95		}, //  80
		{   4,		6,			36,			66,			96		}, //  81
#endif
		{   0,		0,			0,			0,			0		}, //  82
		{   0,		0,			0,			0,			0		}, //  83

		{   0,		0,			0,			0,			0		}, //  84
		{   0,		0,			0,			0,			0		}, //  85
		{   0,		0,			0,			0,			0		}, //  86
		{   0,		0,			0,			0,			0		}, //  87
		{   0,		0,			0,			0,			0		}, //  88
		{   0,		0,			0,			0,			0		}, //  89
		{   0,		0,			0,			0,			0		}, //  90

		{   4,		16,			46,			76,			106		}, //  91
		{   4,		17,			47,			77,			107		}, //  92
		{   4,		18,			48,			78,			108		}, //  93
		{   4,		19,			49,			79,			109		}, //  94
		{   4,		20,			50,			80,			110		}, //  95
		{   4,		21,			51,			81,			111		}, //  96
		{   0,		0,			0,			0,			0		}, //  97
		{   0,		0,			0,			0,			0		}, //  98

		{   0,		0,			0,			0,			0		}, //  99
		{   0,		0,			0,			0,			0		}, //  100

		{   1,  152,    0,    0,    0}, //  101
		{   1,  153,    0,    0,    0}, //  102
		{   1,  154,    0,    0,    0}, //  103
		{   1,  155,    0,    0,    0}, //  104
		{   1,  156,    0,    0,    0}, //  105
		{   1,  157,    0,    0,    0}, //  106

		{   0,    0,    0,    0,    0}, //  107
		{   0,    0,    0,    0,    0}, //  108
		{   0,    0,    0,    0,    0}, //  109
		{   0,    0,    0,    0,    0}, //  110
		{   0,    0,    0,    0,    0}, //  111
		{   0,    0,    0,    0,    0}, //  112
		{   0,    0,    0,    0,    0}, //  113
		{   0,    0,    0,    0,    0}, //  114
		{   0,    0,    0,    0,    0}, //  115
		{   0,    0,    0,    0,    0}, //  116
		{   0,    0,    0,    0,    0}, //  117
		{   0,    0,    0,    0,    0}, //  118
		{   0,    0,    0,    0,    0}, //  119
		{   0,    0,    0,    0,    0}, //  120

		{   2,  137,  140,    0,    0}, //  121
		{   1,  138,    0,    0,    0}, //  122
		{   1,  139,    0,    0,    0}, //  123

	};

	if (dwMotionIndex >= MOTION_MAX_NUM)
	{
		sys_err("OUT_OF_MOTION_VNUM: name=%s, motion=%d/%d", GetName(), dwMotionIndex, MOTION_MAX_NUM);
		return false;
	}

	DWORD* skillVNums = s_anMotion2SkillVnumList[dwMotionIndex];

	DWORD skillCount = *skillVNums++;
	if (skillCount >= SKILL_LIST_MAX_COUNT)
	{
		sys_err("OUT_OF_SKILL_LIST: name=%s, count=%d/%d", GetName(), skillCount, SKILL_LIST_MAX_COUNT);
		return false;
	}

	for (DWORD skillIndex = 0; skillIndex != skillCount; ++skillIndex)
	{
		if (skillIndex >= SKILL_MAX_NUM)
		{
			sys_err("OUT_OF_SKILL_VNUM: name=%s, skill=%d/%d", GetName(), skillIndex, SKILL_MAX_NUM);
			return false;
		}

		DWORD eachSkillVNum = skillVNums[skillIndex];
		if ( eachSkillVNum != 0 )
		{
			DWORD eachJobGroup = s_anSkill2JobGroup[eachSkillVNum];

			if (0 == eachJobGroup || eachJobGroup == selfJobGroup)
			{
				// GUILDSKILL_BUG_FIX
				DWORD eachSkillLevel = 0;

				if (eachSkillVNum >= GUILD_SKILL_START && eachSkillVNum <= GUILD_SKILL_END)
				{
					if (GetGuild())
						eachSkillLevel = GetGuild()->GetSkillLevel(eachSkillVNum);
					else
						eachSkillLevel = 0;
				}
				else
				{
					eachSkillLevel = GetSkillLevel(eachSkillVNum);
				}

				if (eachSkillLevel > 0)
				{
					return true;
				}
				// END_OF_GUILDSKILL_BUG_FIX
			}
		}
	}

	return false;
}

void CHARACTER::ClearSkill()
{
	PointChange(POINT_SKILL, 4 + (GetLevel() - 5) - GetPoint(POINT_SKILL));
	RemoveSkillAffect(); // @fixme305
	ResetSkill();
}

void CHARACTER::ClearSubSkill()
{
	PointChange(POINT_SUB_SKILL, GetLevel() < 10 ? 0 : (GetLevel() - 9) - GetPoint(POINT_SUB_SKILL));

	if (m_pSkillLevels == NULL)
	{
		sys_err("m_pSkillLevels nil (name: %s)", GetName());
		return;
	}

	TPlayerSkill CleanSkill;
	memset(&CleanSkill, 0, sizeof(TPlayerSkill));

	size_t count = sizeof(s_adwSubSkillVnums) / sizeof(s_adwSubSkillVnums[0]);

	for (size_t i = 0; i < count; ++i)
	{
		if (s_adwSubSkillVnums[i] >= SKILL_MAX_NUM)
			continue;

		m_pSkillLevels[s_adwSubSkillVnums[i]] = CleanSkill;
	}

	ComputePoints();
	SkillLevelPacket();
}

bool CHARACTER::ResetOneSkill(DWORD dwVnum)
{
	if (NULL == m_pSkillLevels)
	{
		sys_err("m_pSkillLevels nil (name %s, vnum %u)", GetName(), dwVnum);
		return false;
	}

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("vnum overflow (name %s, vnum %u)", GetName(), dwVnum);
		return false;
	}

	BYTE level = m_pSkillLevels[dwVnum].bLevel;

	m_pSkillLevels[dwVnum].bLevel = 0;
	m_pSkillLevels[dwVnum].bMasterType = 0;
	m_pSkillLevels[dwVnum].tNextRead = 0;

	if (level > 17)
		level = 17;

	PointChange(POINT_SKILL, level);

	LogManager::instance().CharLog(this, dwVnum, "ONE_SKILL_RESET_BY_SCROLL", "");

	ComputePoints();
	SkillLevelPacket();

	return true;
}

eMountType GetMountLevelByVnum(DWORD dwMountVnum, bool IsNew) // updated to 2014/12/10
{
	if (!dwMountVnum)
		return MOUNT_TYPE_NONE;

	switch (dwMountVnum)
	{
		// ### YES SKILL
		// @fixme116 begin
		case 20107: // normal military horse (no guild)
		case 20108: // normal military horse (guild member)
		case 20109: // normal military horse (guild master)
			if (IsNew)
				return MOUNT_TYPE_NONE;
		// @fixme116 end
		// Classic
		case 20110: // Classic Boar
		case 20111: // Classic Wolf
		case 20112: // Classic Tiger
		case 20113: // Classic Lion
		case 20114: // White Lion
		// Special Lv2
		case 20115: // Wild Battle Boar
		case 20116: // Fight Wolf
		case 20117: // Storm Tiger
		case 20118: // Battle Lion (bugged)
		case 20205: // Wild Battle Boar (alternative)
		case 20206: // Fight Wolf (alternative)
		case 20207: // Storm Tiger (alternative)
		case 20208: // Battle Lion (bugged) (alternative)
		// Royal Tigers
		case 20120: // blue
		case 20121: // dark red
		case 20122: // gold
		case 20123: // green
		case 20124: // pied
		case 20125: // white
		// Royal mounts (Special Lv3)
		case 20209: // Royal Boar
		case 20210: // Royal Wolf
		case 20211: // Royal Tiger
		case 20212: // Royal Lion

		case 20215: // Rudolph m Lv3 (yes skill, yes atk)
		case 20218: // Rudolph f Lv3 (yes skill, yes atk)
		case 20225: // Dyno Lv3 (yes skill, yes atk)
		case 20230: // Turkey Lv3 (yes skill, yes atk)
			return MOUNT_TYPE_MILITARY;
			break;
		// ### NO SKILL YES ATK
		// @fixme116 begin
		case 20104: // normal combat horse (no guild)
		case 20105: // normal combat horse (guild member)
		case 20106: // normal combat horse (guild master)
			if (IsNew)
				return MOUNT_TYPE_NONE;
		// @fixme116 end
		case 20119: // Black Horse (no skill, yes atk)
		case 20214: // Rudolph m Lv2 (no skill, yes atk)
		case 20217: // Rudolph f Lv2 (no skill, yes atk)
		case 20219: // Equus Porphyreus (no skill, yes atk)
		case 20220: // Comet (no skill, yes atk)
		case 20221: // Polar Predator (no skill, yes atk)
		case 20222: // Armoured Panda (no skill, yes atk)
		case 20224: // Dyno Lv2 (no skill, yes atk)
		case 20226: // Nightmare (no skill, yes atk)
		case 20227: // Unicorn (no skill, yes atk)
		case 20229: // Turkey Lv2 (no skill, yes atk)
		case 20231: // Leopard (no skill, yes atk)
		case 20232: // Black Panther (no skill, yes atk)
			return MOUNT_TYPE_COMBAT;
			break;
		// ### NO SKILL NO ATK
		// @fixme116 begin
		case 20030: // normal beginner horse (no guild)
		case 20101: // normal beginner horse (no guild)
		case 20102: // normal beginner horse (guild member)
		case 20103: // normal beginner horse (guild master)
			if (IsNew)
				return MOUNT_TYPE_NONE;
		// @fixme116 end
		case 20213: // Rudolph m Lv1 (no skill, no atk)
		case 20216: // Rudolph f Lv1 (no skill, no atk)
		// Special Lv1
		case 20201: // Boar Lv1 (no skill, no atk)
		case 20202: // Wolf Lv1 (no skill, no atk)
		case 20203: // Tiger Lv1 (no skill, no atk)
		case 20204: // Lion Lv1 (no skill, no atk)

		case 20223: // Dyno Lv1 (no skill, no atk)
		case 20228: // Turkey Lv1 (no skill, no atk)
			return MOUNT_TYPE_NORMAL;
			break;
		default:
			return MOUNT_TYPE_NONE;
			break;
	}
}

const int SKILL_COUNT = 6;
static const DWORD SkillList[JOB_MAX_NUM][SKILL_GROUP_MAX_NUM][SKILL_COUNT] =
{
	{ {	1,	2,	3,	4,	5,	6	}, {	16,	17,	18,	19,	20,	21	} },
	{ {	31,	32,	33,	34,	35,	36	}, {	46,	47,	48,	49,	50,	51	} },
	{ {	61,	62,	63,	64,	65,	66	}, {	76,	77,	78,	79,	80,	81	} },
	{ {	91,	92,	93,	94,	95,	96	}, {	106,107,108,109,110,111	} },
#ifdef ENABLE_WOLFMAN_CHARACTER
	{ {	170,171,172,173,174,175	}, {	0,	0,	0,	0,	0,	0	} },
#endif
};

const DWORD GetRandomSkillVnum(BYTE bJob)
{
	// the chosen skill
	DWORD dwSkillVnum = 0;
	do
	{
		// tmp stuff
		DWORD tmpJob = (bJob != JOB_MAX_NUM)?MINMAX(0, bJob, JOB_MAX_NUM-1):number(0, JOB_MAX_NUM-1);
		DWORD tmpSkillGroup = number(0, SKILL_GROUP_MAX_NUM-1);
		DWORD tmpSkillCount = number(0, SKILL_COUNT-1);
		// set skill
		dwSkillVnum = SkillList[tmpJob][tmpSkillGroup][tmpSkillCount];

		if (dwSkillVnum == SKILL_ARROW_SHOWER) // arrow shower is added but not used
			continue;

#if defined(ENABLE_WOLFMAN_CHARACTER) && !defined(USE_WOLFMAN_BOOKS)
		if (tmpJob==JOB_WOLFMAN)
			continue;
#endif

		if (dwSkillVnum != 0 && NULL != CSkillManager::instance().Get(dwSkillVnum))
			break;
	} while (true);
	return dwSkillVnum;
}

bool CHARACTER::CanUseSkill(DWORD dwSkillVnum) const
{
	if (0 == dwSkillVnum) return false;

	if (0 < GetSkillGroup())
	{
		const DWORD* pSkill = SkillList[ GetJob() ][ GetSkillGroup()-1 ];

		for (int i=0 ; i < SKILL_COUNT ; ++i)
		{
			if (pSkill[i] == dwSkillVnum) return true;
		}
	}

	//if (true == IsHorseRiding())

	if (dwSkillVnum != SKILL_SPRINT && true == IsRiding())
	{
#ifndef ENABLE_NO_MOUNT_CHECK
		eMountType eIsMount = GetMountLevelByVnum(GetMountVnum(), false);
		if (eIsMount != MOUNT_TYPE_MILITARY)
		{
			if (test_server)
				sys_log(0, "CanUseSkill: Mount can't skill. vnum(%u) type(%d)", GetMountVnum(), static_cast<int>(eIsMount));
			return false;
		}
#endif
		switch(dwSkillVnum)
		{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:
				return true;
		}
	}

	switch( dwSkillVnum )
	{
		case 121: case 122: case 124: case 126: case 127: case 128: case 129: case 130:
		case 131:
		case 151: case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159:
		case SKILL_SPRINT:
			return true;
	}

	return false;
}

bool CHARACTER::CheckSkillHitCount(const BYTE SkillID, const VID TargetVID)
{
	std::map<int, TSkillUseInfo>::iterator iter = m_SkillUseInfo.find(SkillID);

	if (iter == m_SkillUseInfo.end())
	{
		sys_log(0, "SkillHack: Skill(%u) is not in container", SkillID);
		return false;
	}

	TSkillUseInfo& rSkillUseInfo = iter->second;

	if (false == rSkillUseInfo.bUsed)
	{
		sys_log(0, "SkillHack: not used skill(%u)", SkillID);
		return false;
	}

	if (IsShootSkill(SkillID))
	{
		sys_log(0, "SkillHack: cannot use attack packet (pid %d) for skill(%u)", GetPlayerID(), SkillID);
		return false;
	}

	switch (SkillID)
	{
		case SKILL_DRAGON_SWIRL:
		case SKILL_FLAME_STRIKE:
		case SKILL_STUMP:
		case SKILL_DRAGON_ROAR:
			sys_log(0, "SkillHack: cannot use attack packet (pid %d)  for skill(%u)", GetPlayerID(), SkillID);
			return false;
	}

	target_map::iterator iterTargetMap = rSkillUseInfo.TargetVIDMap.find(TargetVID);

	if (rSkillUseInfo.TargetVIDMap.end() != iterTargetMap)
	{
		size_t MaxAttackCountPerTarget = GetMaxAttackCountForSkill(SkillID, GetSkillPower(SkillID));

		if (iterTargetMap->second >= MaxAttackCountPerTarget)
		{
			sys_log(0, "SkillHack: Too Many Hit count from PID %d SkillID(%u) count(%u)", GetPlayerID(), SkillID, iterTargetMap->second);
			return false;
		}

		iterTargetMap->second++;
	}
	else
	{
		rSkillUseInfo.TargetVIDMap.emplace(TargetVID, 1);
	}

	return true;
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
