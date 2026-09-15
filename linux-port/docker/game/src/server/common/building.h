#ifndef __METIN_II_COMMON_BUILDING_H__
#define __METIN_II_COMMON_BUILDING_H__

namespace building
{
	enum
	{
		OBJECT_MATERIAL_MAX_NUM = 5,
	};

	typedef struct SLand
	{
		DWORD	dwID;
		long	lMapIndex;
		long	x, y;
		long	width, height;
		DWORD	dwGuildID;
		BYTE	bGuildLevelLimit;
		YANG	dwPrice;
	} TLand;

	typedef struct SObjectMaterial
	{
		DWORD	dwItemVnum;
		ITEM_COUNT	dwCount;
	} TObjectMaterial;

	typedef struct SObjectProto
	{
		DWORD	dwVnum;
		YANG	dwPrice;

		TObjectMaterial kMaterials[OBJECT_MATERIAL_MAX_NUM];

		DWORD	dwUpgradeVnum;
		DWORD	dwUpgradeLimitTime;
		long	lLife;
		long	lRegion[4];

		DWORD	dwNPCVnum;
		long	lNPCX;
		long	lNPCY;

		DWORD	dwGroupVnum;
		DWORD	dwDependOnGroupVnum;
	} TObjectProto;

	typedef struct SObject
	{
		DWORD	dwID;
		DWORD	dwLandID;
		DWORD	dwVnum;
		long	lMapIndex;
		long	x, y;

		float	xRot;
		float	yRot;
		float	zRot;
		long	lLife;
	} TObject;
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by Metin2hub.com
