#pragma once	
#include "JsonStruct.h"
#include "stringex.h"
#include "GameDef.h"

struct stItemSkill {
	uint8_t btJob =0;
	uint32_t dwSkillId=0;
	uint8_t btSkillLevel=0;
};
struct stEffectDataLoaderBase {
	uint32_t dwID = 0;						//效果ID
	std::string szName;	//效果名字
	uint32_t dwNextId = 0;					//升级后效果ID
	std::array<int, static_cast<size_t>(AttrID::Count)> attrs = {};
	int GetAttr(AttrID id) const
	{
		return attrs[static_cast<size_t>(id)];
	}
};
struct stItemDataBase :public mydb_item_base_tbl {
	std::string name_gb;
	std::vector<stItemSkill> vitemskills;
	mutable std::weak_ptr<stEffectDataLoaderBase> m_effectDataBase;
	std::shared_ptr<stEffectDataLoaderBase> GetEffectDataBase() const;
	uint8_t GetBagType(bool bAbType = false)
	{
		return 1;
	}
	stItemDataBase(const mydb_item_base_tbl& base)
		: mydb_item_base_tbl(base)  // 调用基类拷贝构造，把 base 的成员都搬过来
	{
		name_gb = UTG(szName.c_str());
	}
};

struct stBuffDataBase :public mydb_magicbuff_tbl {
	mutable std::weak_ptr<stEffectDataLoaderBase> m_effectDataBase;
	std::shared_ptr<stEffectDataLoaderBase> GetEffectDataBase() const;
	std::vector<int> vMutexBuff;	//互斥BUFF
	std::string name;
	stBuffDataBase(const mydb_magicbuff_tbl& base)
		: mydb_magicbuff_tbl(base)
	{
		name = UTG(szName.c_str());
	}
};

struct stMagicDataBase :public mydb_magic_tbl {
	mutable std::weak_ptr<stEffectDataLoaderBase> m_effectDataBase;
	std::shared_ptr<stEffectDataLoaderBase> GetEffectDataBase() const;
	stMagicDataBase(const mydb_magic_tbl& base)
		: mydb_magic_tbl(base)
	{
	}
};

struct ItemBasePtr
{
	int itemId = 0;
	mutable std::weak_ptr<stItemDataBase> m_itemDataBase;
	std::shared_ptr<stItemDataBase> GetItemDataBase() const;
};
struct stSubDropItemDataBase :public mydb_submondropitem_tbl {
	int weight = 0;
	std::vector<ItemBasePtr> m_itemDataBases;
	stSubDropItemDataBase(const mydb_submondropitem_tbl& base)
		: mydb_submondropitem_tbl(base)
	{
		m_itemDataBases.resize(szSubDropItems.size());
		for (size_t i = 0;i<szSubDropItems.size();++i)
		{
			m_itemDataBases[i].itemId = szSubDropItems[i][0];
			weight += szSubDropItems[i][1];
		}
	}
};
struct stMonsterDataBase :public mydb_monster_tbl {
	int MonAiID = 0;
	std::string name;
	stMonsterDataBase(const mydb_monster_tbl& base)
		: mydb_monster_tbl(base)
	{
		name = UTG(szName.c_str());
	}
};

struct stDropItem:stDropItemBase {
	mutable std::weak_ptr<stItemDataBase> pDropItemData;
	mutable std::weak_ptr<stSubDropItemDataBase> m_subDropData;
	std::shared_ptr<stItemDataBase> GetItemDataBase() const;
	std::shared_ptr<stSubDropItemDataBase> GetSubDropData() const;
	stDropItem(const stDropItemBase& base)
		: stDropItemBase(base)
	{
	}
};

struct stDropItemDataBase :public mydb_mondropitem_tbl {
	std::vector<stDropItem> dropItemSet;
	stDropItemDataBase(const mydb_mondropitem_tbl& base)
		: mydb_mondropitem_tbl(base)
	{
	}
};

struct stServerMapInfo:mydb_mapinfo13824_tbl {
	std::string name;
	std::string filename;			// 地图文件名字
	stServerMapInfo(const mydb_mapinfo13824_tbl& base)
		: mydb_mapinfo13824_tbl(base)
	{
		filename = UTG(base.szMapFileName.c_str());
	}
};