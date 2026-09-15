#ifndef __INC_ITEM_MANAGER__
#define __INC_ITEM_MANAGER__

#ifdef M2_USE_POOL
#include "pool.h"
#endif

#define ENABLE_ITEM_PROTO_MAP
class CSpecialAttrGroup
{
public:
	CSpecialAttrGroup(DWORD vnum)
		: m_dwVnum(vnum)
	{}
	struct CSpecialAttrInfo
	{
		CSpecialAttrInfo (DWORD _apply_type, DWORD _apply_value)
			: apply_type(_apply_type), apply_value(_apply_value)
		{}
		DWORD apply_type;
		DWORD apply_value;

	};
	DWORD m_dwVnum;
	std::string	m_stEffectFileName;
	std::vector<CSpecialAttrInfo> m_vecAttrs;
};

struct TSpecialItemGroupDropInfo
{
	DWORD dwVnum;
	DWORD dwCount;
	int iRarePct;
};

class CSpecialItemGroup
{
	public:
		enum EGiveType
		{
			NONE,
			GOLD,
			EXP,
			MOB,
			SLOW,
			DRAIN_HP,
			POISON,
			MOB_GROUP,
#ifdef ENABLE_WOLFMAN_CHARACTER
			BLEEDING,
#endif
		};

		enum ESIGType { NORMAL, PCT, QUEST, SPECIAL };

		struct CSpecialItemInfo
		{
			DWORD vnum;
			int count;
			int rare;
			bool isSpecial;

			CSpecialItemInfo(DWORD _vnum, int _count, int _rare, bool is_special)
				: vnum(_vnum), count(_count), rare(_rare), isSpecial(is_special)
				{}
		};

		CSpecialItemGroup(DWORD vnum, BYTE type=0)
			: m_dwVnum(vnum), m_bType(type)
			{}

		void AddItem(DWORD vnum, int count, int prob, int rare, bool is_special)
		{
			if (!prob)
				return;
			if (!m_vecProbs.empty())
				prob += m_vecProbs.back();
			m_vecProbs.emplace_back(prob);
			m_vecItems.emplace_back(CSpecialItemInfo(vnum, count, rare, is_special));
		}

		bool IsEmpty() const
		{
			return m_vecProbs.empty();
		}

		int GetMultiIndex(std::vector <int> &idx_vec) const
		{
			idx_vec.clear();
			if (m_bType == PCT)
			{
				int count = 0;
				if (number(1,100) <= m_vecProbs[0])
				{
					idx_vec.emplace_back(0);
					count++;
				}
				for (uint i = 1; i < m_vecProbs.size(); i++)
				{
					if (number(1,100) <= m_vecProbs[i] - m_vecProbs[i-1])
					{
						idx_vec.emplace_back(i);
						count++;
					}
				}
				return count;
			}
			else
			{
				idx_vec.emplace_back(GetOneIndex());
				return 1;
			}
		}

		int GetOneIndex() const
		{
			int n = number(1, m_vecProbs.back());
			itertype(m_vecProbs) it = lower_bound(m_vecProbs.begin(), m_vecProbs.end(), n);
			return std::distance(m_vecProbs.begin(), it);
		}

		std::vector<CSpecialItemInfo> GetItems() const
		{
			return m_vecItems;
		}

		std::vector<int> GetProbs() const
		{
			return m_vecProbs;
		}

		int GetVnum(int idx) const
		{
			return m_vecItems[idx].vnum;
		}

		int GetCount(int idx) const
		{
			return m_vecItems[idx].count;
		}

		int GetRarePct(int idx) const
		{
			return m_vecItems[idx].rare;
		}

		bool IsSpecial(int idx) const
		{
			return m_vecItems[idx].isSpecial;
		}

		// playerbot: the group type, for the chest pass (playerbotify.py).
		BYTE GetGroupType() const { return m_bType; }

		bool Contains(DWORD dwVnum) const
		{
			for (DWORD i = 0; i < m_vecItems.size(); i++)
			{
				if (m_vecItems[i].vnum == dwVnum)
					return true;
			}
			return false;
		}

		DWORD GetAttrVnum(DWORD dwVnum) const
		{
			if (CSpecialItemGroup::SPECIAL != m_bType)
				return 0;
			for (itertype(m_vecItems) it = m_vecItems.begin(); it != m_vecItems.end(); it++)
			{
				if (it->vnum == dwVnum)
				{
					return it->count;
				}
			}
			return 0;
		}

		int GetGroupSize() const
		{
			return m_vecProbs.size();
		}

		DWORD m_dwVnum;
		BYTE	m_bType;
		std::vector<int> m_vecProbs;
		std::vector<CSpecialItemInfo> m_vecItems; // vnum, count
};

class CMobItemGroup
{
	public:
		struct SMobItemGroupInfo
		{
			DWORD dwItemVnum;
			int iCount;
			int iRarePct;

			SMobItemGroupInfo(DWORD dwItemVnum, int iCount, int iRarePct)
				: dwItemVnum(dwItemVnum),
			iCount(iCount),
			iRarePct(iRarePct)
			{
			}
		};

		CMobItemGroup(DWORD dwMobVnum, int iKillDrop, const std::string& r_stName)
			:
			m_dwMobVnum(dwMobVnum),
		m_iKillDrop(iKillDrop),
		m_stName(r_stName)
		{
		}

		int GetKillPerDrop() const
		{
			return m_iKillDrop;
		}

		void AddItem(DWORD dwItemVnum, int iCount, int iPartPct, int iRarePct)
		{
			if (!m_vecProbs.empty())
				iPartPct += m_vecProbs.back();
			m_vecProbs.emplace_back(iPartPct);
			m_vecItems.emplace_back(SMobItemGroupInfo(dwItemVnum, iCount, iRarePct));
		}

		// MOB_DROP_ITEM_BUG_FIX
		bool IsEmpty() const
		{
			return m_vecProbs.empty();
		}

		int GetOneIndex() const
		{
			int n = number(1, m_vecProbs.back());
			itertype(m_vecProbs) it = lower_bound(m_vecProbs.begin(), m_vecProbs.end(), n);
			return std::distance(m_vecProbs.begin(), it);
		}
		// END_OF_MOB_DROP_ITEM_BUG_FIX

		const SMobItemGroupInfo& GetOne() const
		{
			return m_vecItems[GetOneIndex()];
		}

		std::vector<SMobItemGroupInfo> GetItems()
		{
			return m_vecItems;
		}

		std::vector<int> GetProbs()
		{
			return m_vecProbs;
		}

	private:
		[[maybe_unused]] DWORD m_dwMobVnum;
		int m_iKillDrop;
		std::string m_stName;
		std::vector<int> m_vecProbs;
		std::vector<SMobItemGroupInfo> m_vecItems;
};

class CDropItemGroup
{
	struct SDropItemGroupInfo
	{
		DWORD	dwVnum;
		DWORD	dwPct;
		int	iCount;
		bool bSpecialGroup;

		SDropItemGroupInfo(DWORD dwVnum, DWORD dwPct, int iCount, bool bSpecialGroup = false)
			: dwVnum(dwVnum), dwPct(dwPct), iCount(iCount), bSpecialGroup(bSpecialGroup)
			{}
	};

	public:
	CDropItemGroup(DWORD dwVnum, DWORD dwMobVnum, const std::string& r_stName)
		:
		m_dwVnum(dwVnum),
	m_dwMobVnum(dwMobVnum),
	m_stName(r_stName)
	{
	}

	const std::vector<SDropItemGroupInfo> & GetVector()
	{
		return m_vec_items;
	}

	void AddItem(DWORD dwItemVnum, DWORD dwPct, int iCount, bool isSpecial=false)
	{
		m_vec_items.emplace_back(SDropItemGroupInfo(dwItemVnum, dwPct, iCount, isSpecial));
	}

	private:
	[[maybe_unused]] DWORD m_dwVnum;
	[[maybe_unused]] DWORD m_dwMobVnum;
	std::string m_stName;
	std::vector<SDropItemGroupInfo> m_vec_items;
};

class CLevelItemGroup
{
	struct SLevelItemGroupInfo
	{
		DWORD dwVNum;
		DWORD dwPct;
		int iCount;

		SLevelItemGroupInfo(DWORD dwVnum, DWORD dwPct, int iCount)
			: dwVNum(dwVnum), dwPct(dwPct), iCount(iCount)
		{ }
	};

	private :
		DWORD m_dwLevelLimit;
		std::string m_stName;
		std::vector<SLevelItemGroupInfo> m_vec_items;

	public :
		CLevelItemGroup(DWORD dwLevelLimit)
			: m_dwLevelLimit(dwLevelLimit)
		{}

		DWORD GetLevelLimit() { return m_dwLevelLimit; }

		void AddItem(DWORD dwItemVnum, DWORD dwPct, int iCount)
		{
			m_vec_items.emplace_back(SLevelItemGroupInfo(dwItemVnum, dwPct, iCount));
		}

		const std::vector<SLevelItemGroupInfo> & GetVector()
		{
			return m_vec_items;
		}
};

class CBuyerThiefGlovesItemGroup
{
	struct SThiefGroupInfo
	{
		DWORD	dwVnum;
		DWORD	dwPct;
		int	iCount;

		SThiefGroupInfo(DWORD dwVnum, DWORD dwPct, int iCount)
			: dwVnum(dwVnum), dwPct(dwPct), iCount(iCount)
			{}
	};

	public:
	CBuyerThiefGlovesItemGroup(DWORD dwVnum, DWORD dwMobVnum, const std::string& r_stName)
		:
		m_dwVnum(dwVnum),
	m_dwMobVnum(dwMobVnum),
	m_stName(r_stName)
	{
	}

	const std::vector<SThiefGroupInfo> & GetVector()
	{
		return m_vec_items;
	}

	void AddItem(DWORD dwItemVnum, DWORD dwPct, int iCount)
	{
		m_vec_items.emplace_back(SThiefGroupInfo(dwItemVnum, dwPct, iCount));
	}

	private:
	[[maybe_unused]] DWORD m_dwVnum;
	[[maybe_unused]] DWORD m_dwMobVnum;
	std::string m_stName;
	std::vector<SThiefGroupInfo> m_vec_items;
};

class ITEM;

class ITEM_MANAGER : public singleton<ITEM_MANAGER>
{
	public:
		ITEM_MANAGER();
		virtual ~ITEM_MANAGER();

		bool                    Initialize(TItemTable * table, int size);
		void			Destroy();
		void			Update();
		void			GracefulShutdown();

		DWORD			GetNewID();
		bool			SetMaxItemID(TItemIDRangeTable range);
		bool			SetMaxSpareItemID(TItemIDRangeTable range);

		void			DelayedSave(LPITEM item);
		void			FlushDelayedSave(LPITEM item);
		void			SaveSingleItem(LPITEM item);
		void			RemoveFromDelayedSave(LPITEM item);

		LPITEM                  CreateItem(DWORD vnum, ITEM_COUNT count = 1, DWORD dwID = 0, bool bTryMagic = false, int iRarePct = -1, bool bSkipSave = false, int addRarePct = 0);
#ifndef DEBUG_ALLOC
		void DestroyItem(LPITEM item);
#else
		void DestroyItem(LPITEM item, const char* file, size_t line);
#endif
		void			RemoveItem(LPITEM item, const char * c_pszReason=NULL);

		LPITEM			Find(DWORD id);
		LPITEM                  FindByVID(DWORD vid);
		TItemTable *            GetTable(DWORD vnum);
		bool			GetVnum(const char * c_pszName, DWORD & r_dwVnum);
		bool			GetVnumByOriginalName(const char * c_pszName, DWORD & r_dwVnum);

		template <typename T>
		bool GetValidVnum(const char * str, T & outVnum) // @fixme303
		{
			try {
				std::size_t pos{};
				outVnum = std::stoi(str, &pos);
				if (strlen(str) != pos)
					return false;
				return GetTable(outVnum);
			} catch(...) {
				return false;
			}
		}

		bool			GetDropPct(LPCHARACTER pkKiller, DWORD race, BYTE bMobRank, int iMobLevel, bool IsStone, OUT int& iDeltaPercent, OUT int& iRandRange);
		bool			GetDropPct(LPCHARACTER pkChr, LPCHARACTER pkKiller, OUT int& iDeltaPercent, OUT int& iRandRange);
		bool			CreateDropItem(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM> & vec_item);

		void			ListSpecialItemDrop(LPCHARACTER ch, DWORD vnum);
		void			ListMobItemDrop(LPCHARACTER ch, DWORD mobVnum);

		bool			ReadCommonDropItemFile(const char * c_pszFileName);
		bool			ReadEtcDropItemFile(const char * c_pszFileName);
		bool			ReadDropItemGroup(const char * c_pszFileName);
		bool			ReadMonsterDropItemGroup(const char * c_pszFileName);
		bool			ReadSpecialDropItemFile(const char * c_pszFileName);

		// convert name -> vnum special_item_group.txt
		bool			ConvSpecialDropItemFile();
		// convert name -> vnum special_item_group.txt

		DWORD			GetRefineFromVnum(DWORD dwVnum);

		static void		CopyAllAttrTo(LPITEM pkOldItem, LPITEM pkNewItem);

		const CSpecialItemGroup* GetSpecialItemGroup(DWORD dwVnum);
		const TSpecialItemGroupDropInfo GetSpecialItemGroupDropInfo(DWORD dwVnum);
		const CSpecialAttrGroup* GetSpecialAttrGroup(DWORD dwVnum);

		const std::vector<TItemTable> & GetTable() { return m_vec_prototype; }

		// CHECK_UNIQUE_GROUP
		int			GetSpecialGroupFromItem(DWORD dwVnum) const { itertype(m_ItemToSpecialGroup) it = m_ItemToSpecialGroup.find(dwVnum); return (it == m_ItemToSpecialGroup.end()) ? 0 : it->second; }
		// END_OF_CHECK_UNIQUE_GROUP

	protected:
		int                     RealNumber(DWORD vnum);
		void			CreateQuestDropItem(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM> & vec_item, int iDeltaPercent, int iRandRange);

	protected:
		typedef std::map<DWORD, LPITEM> ITEM_VID_MAP;

		std::vector<TItemTable>		m_vec_prototype;
		std::vector<TItemTable*> m_vec_item_vnum_range_info;
		#ifdef ENABLE_ITEM_PROTO_MAP
		std::unordered_map<DWORD,TItemTable*> m_mapItemProto;
		#endif
		std::map<DWORD, DWORD>		m_map_ItemRefineFrom;
		int				m_iTopOfTable;

		ITEM_VID_MAP			m_VIDMap;
		DWORD				m_dwVIDCount;
		DWORD				m_dwCurrentID;
		TItemIDRangeTable	m_ItemIDRange;
		TItemIDRangeTable	m_ItemIDSpareRange;

		TR1_NS::unordered_set<LPITEM> m_set_pkItemForDelayedSave;
		std::map<DWORD, LPITEM>		m_map_pkItemByID;
		std::map<DWORD, DWORD>		m_map_dwEtcItemDropProb;
		std::map<DWORD, CDropItemGroup*> m_map_pkDropItemGroup;
		std::map<DWORD, CSpecialItemGroup*> m_map_pkSpecialItemGroup;
		std::map<DWORD, CSpecialItemGroup*> m_map_pkQuestItemGroup;
		std::map<DWORD, CSpecialAttrGroup*> m_map_pkSpecialAttrGroup;
		std::map<DWORD, CMobItemGroup*> m_map_pkMobItemGroup;
		std::map<DWORD, CLevelItemGroup*> m_map_pkLevelItemGroup;
		std::map<DWORD, CBuyerThiefGlovesItemGroup*> m_map_pkGloveItemGroup;

		// CHECK_UNIQUE_GROUP
		std::map<DWORD, int>		m_ItemToSpecialGroup;
		// END_OF_CHECK_UNIQUE_GROUP

	private:

		typedef std::map <DWORD, DWORD> TMapDW2DW;
		TMapDW2DW	m_map_new_to_ori;

	public:
		DWORD	GetMaskVnum(DWORD dwVnum);
		std::map<DWORD, TItemTable>  m_map_vid;
		std::map<DWORD, TItemTable>&  GetVIDMap() { return m_map_vid; }
		std::vector<TItemTable>& GetVecProto() { return m_vec_prototype; }

		const static int MAX_NORM_ATTR_NUM = ITEM_ATTRIBUTE_NORM_NUM;
		const static int MAX_RARE_ATTR_NUM = ITEM_ATTRIBUTE_RARE_NUM;
		bool ReadItemVnumMaskTable(const char * c_pszFileName);
	private:
#ifdef M2_USE_POOL
		ObjectPool<CItem> pool_;
#endif
};

#ifndef DEBUG_ALLOC
#define M2_DESTROY_ITEM(ptr) ITEM_MANAGER::instance().DestroyItem(ptr)
#else
#define M2_DESTROY_ITEM(ptr) ITEM_MANAGER::instance().DestroyItem(ptr, __FILE__, __LINE__)
#endif

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
