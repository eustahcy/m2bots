#include "stdafx.h"
#include "refine.h"

CRefineManager::CRefineManager()
{
}

CRefineManager::~CRefineManager()
{
}

bool CRefineManager::Initialize(TRefineTable * table, int size)
{
	for (int i = 0; i < size; ++i, ++table)
	{
		sys_log(0, "REFINE %d prob %d cost %d", table->id, table->prob, table->cost);
		m_map_RefineRecipe.emplace(table->id, *table);
	}

	sys_log(0, "REFINE: COUNT %d", m_map_RefineRecipe.size());
	return true;
}

const TRefineTable* CRefineManager::GetRefineRecipe(DWORD vnum)
{
	if (vnum == 0)
		return NULL;

	itertype(m_map_RefineRecipe) it = m_map_RefineRecipe.find(vnum);
	// (the "REFINE: FIND" trace is gone: one line per lookup, hundreds of bots)

	if (it == m_map_RefineRecipe.end())
	{
		return NULL;
	}

	return &it->second;
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
