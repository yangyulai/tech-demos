#pragma once
#include "GameDef.h"
#include "JsonStructEx.h"
#include "HashKey.h"


class JsonConfig {
public:
	JsonConfig(const JsonConfig& other) = delete;
	JsonConfig(JsonConfig&& other) noexcept = delete;
	JsonConfig& operator=(const JsonConfig& other) = delete;
	JsonConfig& operator=(JsonConfig&& other) noexcept = delete;
	JsonConfig() = default;
	~JsonConfig() = default;
	static JsonConfig& instance()
	{
		static JsonConfig instance;
		return instance;
	}

	bool Reload(const std::string& name);
	void LoadSpawnData();
	void LoadNpcSpawnData();
	bool LoadAllData();
	void LoadChatConfig();
	void LoadEffectConfig();
	void LoadSubMonDropItemConfig();
	void LoadMonDropItemConfig();
	void LoadItemConfig();
	void LoadSpecialEffectConfig();
	void LoadMonsterConfig();
	void LoadMapConfig();
	void LoadBuffConfig();
	void LoadMagicConfig();
	bool LoadLvlAbilityData();
	bool LoadMapGateData();

	mydb_mongen_tbl* GetMonGenData(uint32_t index);
	mydb_npcgen_tbl* GetNpcGen(uint32_t index);
	const mydb_playerability_tbl* GetLvlAbilityData(int16_t lvl, int16_t job) const;
	const mydb_mapgate_tbl* GetGateData(int mapId, int16_t x, int16_t y) const;
	std::shared_ptr<mydb_specialeffect_tbl> GetSpecialEffectDataById(int effectId);
	std::shared_ptr<stEffectDataLoaderBase> GetEffectDataById(int effectId);
	std::shared_ptr<stItemDataBase> GetItemDataById(int itemId);
	std::shared_ptr<stBuffDataBase> GetBuffDataBase(BuffKey key);
	std::shared_ptr<stMagicDataBase> GetMagicDataBase(uint32_t id, uint8_t lv);
	std::shared_ptr<stMonsterDataBase> GetMonsterDataBase(int id);
	std::shared_ptr<stDropItemDataBase> GetDropDataBase(int id);
	std::shared_ptr<stSubDropItemDataBase> GetSubDropDataBase(int id);
	std::shared_ptr<stServerMapInfo> GetMapDataBase(int id);

	std::vector<mydb_mongen_tbl> spawn_data;//怪物刷新表
	std::vector<mydb_npcgen_tbl> npc_data;//npc刷新表
	std::unordered_map<LvlAbilityKey, mydb_playerability_tbl, LvlAbilityKeyHash> m_lvlAbilityData;//等级属性表
	std::unordered_map<MapGateKey, mydb_mapgate_tbl> m_mapGateData;//地图门点表
	std::vector<Config_Chat> m_chatConfig;//聊天配置表
	std::unordered_map<BuffKey, std::shared_ptr<stBuffDataBase>, BuffKeyHash> buffMap;//buff表
	std::unordered_map<BuffKey, std::shared_ptr<stMagicDataBase>, BuffKeyHash> m_magicData;//技能表
	std::unordered_map<int, std::shared_ptr<stServerMapInfo>> m_mapConfig;
	std::unordered_map<int, std::shared_ptr<stMonsterDataBase>> m_monsterConfig;
	std::unordered_map<int, std::shared_ptr<mydb_specialeffect_tbl>> m_specialEffectConfig;
	std::unordered_map<int, std::shared_ptr<stSubDropItemDataBase>> m_subMonDropItem;
	std::unordered_map<int, std::shared_ptr<stDropItemDataBase>> m_monDropItem;
	std::unordered_map<int, std::shared_ptr<stItemDataBase>> m_itemData;
	std::unordered_map<int, std::shared_ptr<stEffectDataLoaderBase>> m_effectData;
	std::time_t last_spawn_time = 0;
private:
	bool is_initial = true;			// 是否启动服务器
};

#define sJsonConfig JsonConfig::instance()
