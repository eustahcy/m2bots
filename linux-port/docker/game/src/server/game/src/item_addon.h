#ifndef __ITEM_ADDON_H
#define __ITEM_ADDON_H

class CItemAddonManager : public singleton<CItemAddonManager>
{
	public:
		CItemAddonManager();
		virtual ~CItemAddonManager();

		void ApplyAverageTo(LPITEM pItem);
		void ApplyAddonTo(int iAddonType, LPITEM pItem);
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
