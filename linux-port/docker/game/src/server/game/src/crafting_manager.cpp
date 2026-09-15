#include "stdafx.h"
#include "crafting_manager.h"

CCraftingManager::CCraftingManager()
{
}

CCraftingManager::~CCraftingManager()
{
}

bool CCraftingManager::Initialize(TCraftingItem* table, int size)
{
	m_map_CraftingRecipe.clear();
	for (int i = 0; i < size; ++i, ++table)
	{
		m_map_CraftingRecipe.insert(std::make_pair(table->vnum, *table));
	}

	sys_log(0, "CRAFTING: COUNT %d", m_map_CraftingRecipe.size());
	return true;
}

const TCraftingItem* CCraftingManager::GetCraftingRecipe(DWORD vnum)
{
	if (vnum == 0)
		return NULL;

	itertype(m_map_CraftingRecipe) it = m_map_CraftingRecipe.find(vnum);

	if (it == m_map_CraftingRecipe.end())
	{
		return NULL;
	}

	return &it->second;
}

// Files shared by GameCore.top
