#include "constants.h"

class CCraftingManager : public singleton<CCraftingManager>
{
	typedef std::map<DWORD, TCraftingItem> TCraftingRecipeMap;
public:
	CCraftingManager();
	virtual ~CCraftingManager();

	bool	Initialize(TCraftingItem* table, int size);
	const TCraftingItem* GetCraftingRecipe(DWORD id);

private:
	TCraftingRecipeMap    m_map_CraftingRecipe;

};

// Files shared by GameCore.top
