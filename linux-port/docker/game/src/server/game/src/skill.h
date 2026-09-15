#ifndef __INC_METIN_II_GAME_CSkillManager_H__
#define __INC_METIN_II_GAME_CSkillManager_H__

#include "../../libpoly/Poly.h"
#include "../../common/CommonDefines.h"

class CSkillProto
{
	public:
		char	szName[64];
		DWORD	dwVnum;

		DWORD	dwType;
		BYTE	bMaxLevel;
		BYTE	bLevelLimit;
		int	iSplashRange;

		BYTE	bPointOn;
		CPoly	kPointPoly;

		CPoly	kSPCostPoly;
		CPoly	kDurationPoly;
		CPoly	kDurationSPCostPoly;
		CPoly	kCooldownPoly;
		CPoly	kMasterBonusPoly;
		CPoly	kSplashAroundDamageAdjustPoly;

		DWORD	dwFlag;
		DWORD	dwAffectFlag;

		BYTE	bLevelStep;
		DWORD	preSkillVnum;
		BYTE	preSkillLevel;

		long	lMaxHit;

		BYTE	bSkillAttrType;

		BYTE	bPointOn2;
		CPoly	kPointPoly2;
		CPoly	kDurationPoly2;
		DWORD	dwFlag2;
		DWORD	dwAffectFlag2;

		DWORD   dwTargetRange;

		bool	IsChargeSkill()
		{
			return dwVnum == SKILL_CHARGE || dwVnum == SKILL_HORSE_CHARGE;
		}

		BYTE bPointOn3;
		CPoly kPointPoly3;
		CPoly kDurationPoly3;
		DWORD	dwAffectFlag3;

		CPoly kGrandMasterAddSPCostPoly;

		void SetPointVar(const std::string& strName, double dVar);
		void SetDurationVar(const std::string& strName, double dVar);
		void SetSPCostVar(const std::string& strName, double dVar);
};

class CSkillManager : public singleton<CSkillManager>
{
	public:
		CSkillManager();
		virtual ~CSkillManager();

		bool Initialize(TSkillTable * pTab, int iSize);
		CSkillProto * Get(DWORD dwVnum);
		CSkillProto * Get(const char * c_pszSkillName);

	protected:
		std::map<DWORD, CSkillProto *> m_map_pkSkillProto;
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
