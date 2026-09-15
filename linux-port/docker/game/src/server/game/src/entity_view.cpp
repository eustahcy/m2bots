#include "stdafx.h"

#include "utils.h"
#include "char.h"
#include "sectree_manager.h"
#include "config.h"

void CEntity::ViewCleanup(
	#ifdef ENABLE_GOTO_LAG_FIX
	bool recursive
	#endif
)
{
	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = it->first;
		++it;

		if (entity)
		{
#ifdef ENABLE_GOTO_LAG_FIX
			entity->ViewRemove(this, recursive);
#else
			entity->ViewRemove(this, false);
#endif
		}
	}

	m_map_view.clear();
}

void CEntity::ViewReencode()
{
	if (m_bIsObserver)
		return;

	EncodeRemovePacket(this);
	EncodeInsertPacket(this);

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		if (entity)
		{
			EncodeRemovePacket(entity);
			if (!m_bIsObserver)
				EncodeInsertPacket(entity);

			if (!entity->m_bIsObserver)
				entity->EncodeInsertPacket(this);
		}
	}
}

void CEntity::ViewInsert(LPENTITY entity, bool recursive)
{
	if (!entity)
		return;

	if (this == entity)
		return;

	ENTITY_MAP::iterator it = m_map_view.find(entity);

	if (m_map_view.end() != it)
	{
		it->second = m_iViewAge;
		return;
	}

	m_map_view.emplace(entity, m_iViewAge);

	if (!entity->m_bIsObserver)
		entity->EncodeInsertPacket(this);

	if (recursive)
		entity->ViewInsert(this, false);
}

void CEntity::ViewRemove(LPENTITY entity, bool recursive)
{
	if (!entity)
		return;

	ENTITY_MAP::iterator it = m_map_view.find(entity);
	if (it == m_map_view.end())
		return;

	m_map_view.erase(it);

	if (!(entity->m_bIsObserver  || entity->m_bIsSpy))
		entity->EncodeRemovePacket(this);

	if (recursive)
		entity->ViewRemove(this, false);
}

class CFuncViewInsert
{
	private:
		int dwViewRange;

	public:
		LPENTITY m_me;

		CFuncViewInsert(LPENTITY ent) :
			dwViewRange(VIEW_RANGE + VIEW_BONUS_RANGE),
			m_me(ent)
		{
		}

		void operator () (LPENTITY ent)
		{
			if (!ent->IsType(ENTITY_OBJECT))
			{
				if (DISTANCE_APPROX(ent->GetX() - m_me->GetX(), ent->GetY() - m_me->GetY()) > dwViewRange)
					return;
			}

			#ifdef ENABLE_REDUCED_ENTITY_VIEW
			if (m_me->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER chMe = (LPCHARACTER) m_me;

				if (!chMe)
					return;

				LPCHARACTER chEnt = (LPCHARACTER)ent;
				if (!chEnt)
					return;

				// players view every entity
				if (chMe->IsPC())
				{
					m_me->ViewInsert(ent);
				}

				// npcs view only a restricted amount of entities
				else if (chMe->IsNPC() && ent->IsType(ENTITY_CHARACTER))
				{
					constexpr auto DefenseWaveMast = 20434;

					// mobs view every player
					if (chEnt->IsPC())
						m_me->ViewInsert(ent);

					// aiflag healers view their party (unused)
					//else if (IS_SET(chMe->GetAIFlag(), AIFLAG_HEALER) && chMe->GetParty() && chMe->GetParty() == chEnt->GetParty())
					//	m_me->ViewInsert(ent);

					// hydra mast is seen by other mobs
					else if (chEnt->GetRaceNum() == DefenseWaveMast)
						m_me->ViewInsert(ent);

					// city guardians see all mobs
					else if (chMe->IsGuardNPC())
						m_me->ViewInsert(ent);

					else if (IS_SET(chMe->GetAIFlag(), AIFLAG_ATTACKMOB | AIFLAG_SEE_OTHER) || chMe->IsCustomNpcFlag(CUSTOM_NPC_FLAG_SEE_OTHER))
					{
						m_me->ViewInsert(ent);
					}
				}
			}
			else if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER chEnt = (LPCHARACTER) ent;

				// other entities see every player
				if (chEnt->IsPC())
					m_me->ViewInsert(ent);
			}
			#else
			m_me->ViewInsert(ent);
			#endif

			if (ent->IsType(ENTITY_CHARACTER) && m_me->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER chMe = (LPCHARACTER) m_me;
				LPCHARACTER chEnt = (LPCHARACTER) ent;

				if (chMe->IsPC() && !chEnt->IsPC() && !chEnt->IsWarp() && !chEnt->IsGoto())
					chEnt->StartStateMachine();
			}
		}
};

void CEntity::UpdateSectree()
{
	if (!GetSectree())
	{
		if (IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER tch = (LPCHARACTER) this;
			if (tch)
				sys_err("null sectree name: %s %d %d",  tch->GetName(), GetX(), GetY());
		}

		return;
	}

	++m_iViewAge;

	CFuncViewInsert f(this);
	GetSectree()->ForEachAround(f);

	ENTITY_MAP::iterator it, this_it;

	if (m_bObserverModeChange)
	{
		if (m_bIsObserver)
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;
				if (this_it->second < m_iViewAge)
				{
					LPENTITY ent = this_it->first;

					ent->EncodeRemovePacket(this);
					m_map_view.erase(this_it);

					ent->ViewRemove(this, false);
				}
				else
				{
					LPENTITY ent = this_it->first;

					//ent->EncodeRemovePacket(this);
					//m_map_view.erase(this_it);

					//ent->ViewRemove(this, false);
					EncodeRemovePacket(ent);
				}
			}
		}
		else
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;

				if (this_it->second < m_iViewAge)
				{
					LPENTITY ent = this_it->first;

					if (ent)
					{
						ent->EncodeRemovePacket(this);
						m_map_view.erase(this_it);

						ent->ViewRemove(this, false);
					}
				}
				else
				{
					LPENTITY ent = this_it->first;
					if (ent)
					{
						ent->EncodeInsertPacket(this);
						EncodeInsertPacket(ent);

						ent->ViewInsert(this, true);
					}
				}
			}
		}

		m_bObserverModeChange = false;
	}
	else
	{
		if (!m_bIsObserver)
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;

				if (this_it->second < m_iViewAge)
				{
					LPENTITY ent = this_it->first;

					if (!ent)
					{
						m_map_view.erase(this_it);
						continue;
					}

					ent->ViewRemove(this, false);
					m_map_view.erase(this_it);

					ent->EncodeRemovePacket(this);
				}
			}
		}
	}
}

class CFuncViewRemove
{
public:
	LPENTITY m_me;

	CFuncViewRemove(LPENTITY ent) :
		m_me(ent)
	{
	}

	void operator () (LPENTITY ent)
	{
		{
			LPCHARACTER chEnt = (LPCHARACTER)ent;
			if (ent != m_me && chEnt && chEnt->IsPC() && !chEnt->IsGM())
			{
				ent->ViewRemove(m_me, false);
			}
		}
	}
};

void CEntity::RemoveViewAllViewers()
{
	ENTITY_MAP::iterator it, this_it;

	it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		this_it = it++;
		LPENTITY ent = this_it->first;
		if (ent == nullptr)
		{
			m_map_view.erase(this_it);
			continue;
		}

		if (this_it->second < m_iViewAge)
		{
			ent->EncodeRemovePacket(this);
			m_map_view.erase(this_it);

			ent->ViewRemove(this, false);
		}
		else
		{
			EncodeRemovePacket(ent);
		}
	}
	m_bIsSpy = true;
}

//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
