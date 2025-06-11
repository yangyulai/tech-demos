#include "JsonConfig.h"
#include <zLogger.h>
#include "LocalDB.h"

bool JsonConfig::LoadAllData()
{

	// 为每个数据成员创建备份
	auto spawn_data_backup = spawn_data;
	auto npc_data_backup = npc_data;
	auto m_lvlAbilityData_backup = m_lvlAbilityData;
	auto m_mapGateData_backup = m_mapGateData;
	auto m_chatConfig_backup = m_chatConfig;
	auto buffMap_backup = buffMap;
	auto m_mapConfig_backup = m_mapConfig;
	auto m_monsterConfig_backup = m_monsterConfig;
	auto m_specialEffectConfig_backup = m_specialEffectConfig;
	auto m_subMonDropItem_backup = m_subMonDropItem;
	auto m_monDropItem_backup = m_monDropItem;
	auto m_effectData_backup = m_effectData;
	auto m_itemData_backup = m_itemData;
	auto m_magicData_backup = m_magicData;

	try
	{
		LoadSpawnData();
		LoadNpcSpawnData();
		LoadLvlAbilityData();
		LoadMapGateData();
		LoadChatConfig();
		LoadMonsterConfig();
		LoadMapConfig();
		LoadEffectConfig();
		LoadSpecialEffectConfig();
		LoadItemConfig();
		LoadBuffConfig();
		LoadSubMonDropItemConfig();
		LoadMonDropItemConfig();
		LoadMagicConfig();
	}
	catch (const std::exception& ex)
	{
		g_logger.error(__FUNC_LINE__ " %s", ex.what());
		if (!is_initial) {
			// 如果是重载阶段，恢复数据
			spawn_data = std::move(spawn_data_backup);
			npc_data = std::move(npc_data_backup);
			m_lvlAbilityData = std::move(m_lvlAbilityData_backup);
			m_mapGateData = std::move(m_mapGateData_backup);
			m_chatConfig = std::move(m_chatConfig_backup);
			buffMap = std::move(buffMap_backup);
			m_mapConfig = std::move(m_mapConfig_backup);
			m_monsterConfig = std::move(m_monsterConfig_backup);
			m_specialEffectConfig = std::move(m_specialEffectConfig_backup);
			m_subMonDropItem = std::move(m_subMonDropItem_backup);
			m_monDropItem = std::move(m_monDropItem_backup);
			m_effectData = std::move(m_effectData_backup);
			m_itemData = std::move(m_itemData_backup);
			m_magicData = std::move(m_magicData_backup);
		}
		return false;
	}
	g_logger.debug("加载怪物刷新表数据：%I64d条", spawn_data.size());
	g_logger.debug("加载NPC刷新表数据：%I64d条", npc_data.size());
	g_logger.debug("加载等级能力表数据：%I64d条", m_lvlAbilityData.size());
	g_logger.debug("加载地图传送门数据：%I64d条", m_mapGateData.size());
	g_logger.debug("加载聊天配置表数据：%I64d条", m_chatConfig.size());
	g_logger.debug("加载BUFF配置表数据：%I64d条", buffMap.size());
	g_logger.debug("加载地图配置表数据：%I64d条", m_mapConfig.size());
	g_logger.debug("加载怪物配置表数据：%I64d条", m_monsterConfig.size());
	g_logger.debug("加载特效配置表数据：%I64d条", m_specialEffectConfig.size());
	g_logger.debug("加载怪物掉落子表数据：%I64d条", m_subMonDropItem.size());
	g_logger.debug("加载怪物掉落表数据：%I64d条", m_monDropItem.size());
	g_logger.debug("加载效果表数据：%I64d条", m_effectData.size());
	g_logger.debug("加载道具表表数据：%I64d条", m_itemData.size());
	g_logger.debug("加载魔法表表数据：%I64d条", m_magicData.size());
	is_initial = false;		// 第一次加载完成后设置为false
	return true;
}

bool JsonConfig::Reload(const std::string& name)
{
	// 为每个数据成员创建备份
	auto spawn_data_backup = spawn_data;
	auto npc_data_backup = npc_data;
	auto m_lvlAbilityData_backup = m_lvlAbilityData;
	auto m_mapGateData_backup = m_mapGateData;
	auto m_chatConfig_backup = m_chatConfig;
	auto buffMap_backup = buffMap;
	auto m_mapConfig_backup = m_mapConfig;
	auto m_monsterConfig_backup = m_monsterConfig;
	auto m_specialEffectConfig_backup = m_specialEffectConfig;
	auto m_subMonDropItem_backup = m_subMonDropItem;
	auto m_monDropItem_backup = m_monDropItem;
	auto m_effectData_backup = m_effectData;
	auto m_itemData_backup = m_itemData;
	auto m_magicData_backup = m_magicData;

	static const std::unordered_map<std::string, std::function<void()>> JsonConfigMap = {
		{"mon", [this] { LoadMonsterConfig(); }},
		{"mongen", [this] { LoadSpawnData(); }},
		{"mondropitem", [this] { LoadMonDropItemConfig(); }},
		{"npcgen", [this] { LoadNpcSpawnData(); }},
		{"magicbuff", [this] { LoadBuffConfig(); }},
		{"effectbase", [this] { LoadEffectConfig(); }},
		{"specialeffect", [this] { LoadSpecialEffectConfig(); }},
		{"itembase", [this] { LoadItemConfig(); }},
		{"mapgate", [this] { LoadMapGateData(); }},
		{"magic", [this] { LoadMagicConfig(); }},
		{"chat", [this] { LoadChatConfig(); }},
		{"level", [this] { LoadLvlAbilityData(); }},
		{"subdrop", [this] { LoadSubMonDropItemConfig(); }},
		{"map", [this] { LoadMapConfig(); }},
	};
	// 还原操作
	const std::unordered_map<std::string, std::function<void()>> JsonConfigMapBackUp = {
		{"mon", [&] { m_monsterConfig = std::move(m_monsterConfig_backup); }},
		{"mongen", [&] { spawn_data = std::move(spawn_data_backup); }},
		{"mondropitem", [&] { m_monDropItem = std::move(m_monDropItem_backup); }},
		{"npcgen", [&] { npc_data = std::move(npc_data_backup); }},
		{"magicbuff", [&] { buffMap = std::move(buffMap_backup); }},
		{"effectbase", [&] { m_effectData = std::move(m_effectData_backup); }},
		{"specialeffect", [&] { m_specialEffectConfig = std::move(m_specialEffectConfig_backup); }},
		{"itembase", [&] { m_itemData = std::move(m_itemData_backup); }},
		{"mapgate", [&] { m_mapGateData = std::move(m_mapGateData_backup); }},
		{"magic", [&] { m_magicData = std::move(m_magicData_backup); }},
		{"chat", [&] { m_chatConfig = std::move(m_chatConfig_backup); }},
		{"level", [&] { m_lvlAbilityData = std::move(m_lvlAbilityData_backup); }},
		{"subdrop", [&] { m_subMonDropItem = std::move(m_subMonDropItem_backup); }},
		{"map", [&] { m_mapConfig = std::move(m_mapConfig_backup); }},
	};
	try {
		auto it = JsonConfigMap.find(name);
		if (it != JsonConfigMap.end())
		{
			it->second();
		}
	}
	catch (const std::exception& ex)
	{
		g_logger.error(__FUNC_LINE__ " %s", ex.what());
		auto it = JsonConfigMapBackUp.find(name);
		if (it != JsonConfigMapBackUp.end())
		{
			it->second();
		}
		return false;
	}
	return true;
}
void JsonConfig::LoadChatConfig()
{
	m_chatConfig.clear();
	iguana::from_json_file(m_chatConfig, R"(.\data\Config_Chat.json)");
}

void JsonConfig::LoadEffectConfig()
{
	std::vector<mydb_effect_base_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_effect_base_tbl.json)");
	m_effectData.clear();
	m_effectData.reserve(tmp_data.size());
	for (auto const& entry : tmp_data) {
		stEffectDataLoaderBase tmp;
		tmp.dwID = entry.dwID;
		tmp.dwNextId = entry.dwNextId;
		tmp.szName = UTG(entry.szName.c_str());
		tmp.attrs[tosizet(AttrID::Strength)] = entry.attrs_Strength;
		tmp.attrs[tosizet(AttrID::Physique)] = entry.attrs_Physique;
		tmp.attrs[tosizet(AttrID::Agility)] = entry.attrs_Agility;
		tmp.attrs[tosizet(AttrID::Wisdom)] = entry.attrs_Wisdom;
		tmp.attrs[tosizet(AttrID::Intelligence)] = entry.attrs_Intelligence;
		tmp.attrs[tosizet(AttrID::MinPAtk)] = entry.attrs_MinPAtk;
		tmp.attrs[tosizet(AttrID::MaxPAtk)] = entry.attrs_MaxPAtk;
		tmp.attrs[tosizet(AttrID::MinMAtk)] = entry.attrs_MinMAtk;
		tmp.attrs[tosizet(AttrID::MaxMAtk)] = entry.attrs_MaxMAtk;
		tmp.attrs[tosizet(AttrID::PAtkPer)] = entry.attrs_PAtkPer;
		tmp.attrs[tosizet(AttrID::MAtkPer)] = entry.attrs_MAtkPer;
		tmp.attrs[tosizet(AttrID::AllAttackAdd)] = entry.attrs_AllAttackAdd;
		tmp.attrs[tosizet(AttrID::PAtkInc)] = entry.attrs_PAtkInc;
		tmp.attrs[tosizet(AttrID::MAtkInc)] = entry.attrs_MAtkInc;
		tmp.attrs[tosizet(AttrID::AttackAdd)] = entry.attrs_AttackAdd;
		tmp.attrs[tosizet(AttrID::AttackDec)] = entry.attrs_AttackDec;
		tmp.attrs[tosizet(AttrID::PDef)] = entry.attrs_PDef;
		tmp.attrs[tosizet(AttrID::MDef)] = entry.attrs_MDef;
		tmp.attrs[tosizet(AttrID::PDefPer)] = entry.attrs_PDefPer;
		tmp.attrs[tosizet(AttrID::MDefPer)] = entry.attrs_MDefPer;
		tmp.attrs[tosizet(AttrID::MaxHP)] = entry.attrs_MaxHP;
		tmp.attrs[tosizet(AttrID::MaxHpPer)] = entry.attrs_MaxHpPer;
		tmp.attrs[tosizet(AttrID::HpRestore)] = entry.attrs_HpRestore;
		tmp.attrs[tosizet(AttrID::HpRestorePer)] = entry.attrs_HpRestorePer;
		tmp.attrs[tosizet(AttrID::FinalMaxHp)] = entry.attrs_FinalMaxHp;
		tmp.attrs[tosizet(AttrID::MaxMP)] = entry.attrs_MaxMP;
		tmp.attrs[tosizet(AttrID::MaxMpPer)] = entry.attrs_MaxMpPer;
		tmp.attrs[tosizet(AttrID::MpRestore)] = entry.attrs_MpRestore;
		tmp.attrs[tosizet(AttrID::MpRestorePer)] = entry.attrs_MpRestorePer;
		tmp.attrs[tosizet(AttrID::MpCostPer)] = entry.attrs_MpCostPer;
		tmp.attrs[tosizet(AttrID::MpCost)] = entry.attrs_MpCost;
		tmp.attrs[tosizet(AttrID::FinalMaxMp)] = entry.attrs_FinalMaxMp;
		tmp.attrs[tosizet(AttrID::MaxPP)] = entry.attrs_MaxPP;
		tmp.attrs[tosizet(AttrID::MaxPpPer)] = entry.attrs_MaxPpPer;
		tmp.attrs[tosizet(AttrID::PpRestore)] = entry.attrs_PpRestore;
		tmp.attrs[tosizet(AttrID::PpRestorePer)] = entry.attrs_PpRestorePer;
		tmp.attrs[tosizet(AttrID::AttackSpeedPer)] = entry.attrs_AttackSpeedPer;
		tmp.attrs[tosizet(AttrID::AttackSpeedPhase)] = entry.attrs_AttackSpeedPhase;
		tmp.attrs[tosizet(AttrID::MoveSpeedPer)] = entry.attrs_MoveSpeedPer;
		tmp.attrs[tosizet(AttrID::MoveSpeedPhase)] = entry.attrs_MoveSpeedPhase;
		tmp.attrs[tosizet(AttrID::ReleaseSpeedPer)] = entry.attrs_ReleaseSpeedPer;
		tmp.attrs[tosizet(AttrID::ReleaseSpeedPhase)] = entry.attrs_ReleaseSpeedPhase;
		tmp.attrs[tosizet(AttrID::CritRate)] = entry.attrs_CritRate;
		tmp.attrs[tosizet(AttrID::CritMul)] = entry.attrs_CritMul;
		tmp.attrs[tosizet(AttrID::CritResist)] = entry.attrs_CritResist;
		tmp.attrs[tosizet(AttrID::CritDec)] = entry.attrs_CritDec;
		tmp.attrs[tosizet(AttrID::PunctureRate)] = entry.attrs_PunctureRate;
		tmp.attrs[tosizet(AttrID::MPunctureRate)] = entry.attrs_MPunctureRate;
		tmp.attrs[tosizet(AttrID::PunctureDecRate)] = entry.attrs_PunctureDecRate;
		tmp.attrs[tosizet(AttrID::MPunctureDecRate)] = entry.attrs_MPunctureDecRate;
		tmp.attrs[tosizet(AttrID::ReflectMul)] = entry.attrs_ReflectMul;
		tmp.attrs[tosizet(AttrID::ReflectDecRate)] = entry.attrs_ReflectDecRate;
		tmp.attrs[tosizet(AttrID::RealDamageInc)] = entry.attrs_RealDamageInc;
		tmp.attrs[tosizet(AttrID::RealDamageDec)] = entry.attrs_RealDamageDec;
		tmp.attrs[tosizet(AttrID::RealDamagePer)] = entry.attrs_RealDamagePer;
		tmp.attrs[tosizet(AttrID::RealDamageDecPer)] = entry.attrs_RealDamageDecPer;
		tmp.attrs[tosizet(AttrID::RangeAttackMul)] = entry.attrs_RangeAtkMul;
		tmp.attrs[tosizet(AttrID::SuckBloodRate)] = entry.attrs_SuckBloodRate;
		tmp.attrs[tosizet(AttrID::SuckBloodMul)] = entry.attrs_SuckBloodMul;
		tmp.attrs[tosizet(AttrID::MonDamagePer)] = entry.attrs_MonDamagePer;
		tmp.attrs[tosizet(AttrID::PvpMul)] = entry.attrs_PvpMul;
		tmp.attrs[tosizet(AttrID::PveMul)] = entry.attrs_PveMul;
		tmp.attrs[tosizet(AttrID::FixAddDamage)] = entry.attrs_FixAddDamage;
		tmp.attrs[tosizet(AttrID::FixDecDamage)] = entry.attrs_FixDecDamage;
		tmp.attrs[tosizet(AttrID::DamageAddPer)] = entry.attrs_DamageAddPer;
		tmp.attrs[tosizet(AttrID::DamageDecPer)] = entry.attrs_DamageDecPer;
		tmp.attrs[tosizet(AttrID::PDropRate)] = entry.attrs_PDropRate;
		tmp.attrs[tosizet(AttrID::ExpMul)] = entry.attrs_ExpMul;
		tmp.attrs[tosizet(AttrID::BattleExpMul)] = entry.attrs_BattleExpMul;
		tmp.attrs[tosizet(AttrID::BlueEquipRate)] = entry.attrs_BlueEquipRate;
		tmp.attrs[tosizet(AttrID::GoldEquipRate)] = entry.attrs_GoldEquipRate;
		tmp.attrs[tosizet(AttrID::WeaponStrengthenLv)] = entry.attrs_WeaponStrengthenLv;
		tmp.attrs[tosizet(AttrID::ArmorStrengthenLv)] = entry.attrs_ArmorStrengthenLv;
		tmp.attrs[tosizet(AttrID::BossDamageInc)] = entry.attrs_BossDamageInc;
		tmp.attrs[tosizet(AttrID::BossDamageIncPer)] = entry.attrs_BossDamageIncPer;
		tmp.attrs[tosizet(AttrID::BossDamageDec)] = entry.attrs_BossDamageDec;
		tmp.attrs[tosizet(AttrID::BossDamageDecPer)] = entry.attrs_BossDamageDecPer;
		tmp.attrs[tosizet(AttrID::Hit)] = entry.attrs_Hit;
		tmp.attrs[tosizet(AttrID::Juck)] = entry.attrs_Juck;
		tmp.attrs[tosizet(AttrID::HitVal)] = entry.attrs_HitVal;
		tmp.attrs[tosizet(AttrID::JuckVal)] = entry.attrs_JuckVal;
		tmp.attrs[tosizet(AttrID::HPPotionPer)] = entry.attrs_HPPotionPer;
		tmp.attrs[tosizet(AttrID::MPPotionPer)] = entry.attrs_MPPotionPer;
		tmp.attrs[tosizet(AttrID::PPPotionPer)] = entry.attrs_PPPotionPer;
		tmp.attrs[tosizet(AttrID::StrengthToAtk)] = entry.attrs_StrToAtk;
		tmp.attrs[tosizet(AttrID::StrengthToMaxPP)] = entry.attrs_StrToPP;
		tmp.attrs[tosizet(AttrID::PhysiqueToMaxHP)] = entry.attrs_PhyToHP;
		tmp.attrs[tosizet(AttrID::PhysiqueToDef)] = entry.attrs_PhyToDef;
		tmp.attrs[tosizet(AttrID::AgilityToAtk)] = entry.attrs_AgiToAtk;
		tmp.attrs[tosizet(AttrID::AgilityToHit)] = entry.attrs_AgiToHit;
		tmp.attrs[tosizet(AttrID::AgilityToJuck)] = entry.attrs_AgiToJuck;
		tmp.attrs[tosizet(AttrID::WisdomToAtk)] = entry.attrs_WisToAtk;
		tmp.attrs[tosizet(AttrID::WisdomToDef)] = entry.attrs_WisToDef;
		tmp.attrs[tosizet(AttrID::WisdomToMaxMP)] = entry.attrs_WisToMP;
		tmp.attrs[tosizet(AttrID::CounterCoeff)] = entry.attrs_CounterCoeff;
		tmp.attrs[tosizet(AttrID::CounterInc)] = entry.attrs_CounterInc;
		tmp.attrs[tosizet(AttrID::AgilityToCounterDec)] = entry.attrs_AgilityToCounterDec;
		tmp.attrs[tosizet(AttrID::PhysiqueToCounterDec)] = entry.attrs_PhysiqueToCounterDec;
		tmp.attrs[tosizet(AttrID::HpRestoreFiveSec)] = entry.attrs_HpRestoreFiveSec;
		tmp.attrs[tosizet(AttrID::MpRestoreFiveSec)] = entry.attrs_MpRestoreFiveSec;
		tmp.attrs[tosizet(AttrID::PpRestoreFiveSec)] = entry.attrs_PpRestoreFiveSec;
		tmp.attrs[tosizet(AttrID::GainTime)] = entry.attrs_GainTime;
		tmp.attrs[tosizet(AttrID::WisdomToGainTimePer)] = entry.attrs_WisdomToGainTimePer;
		tmp.attrs[tosizet(AttrID::MagicDmgAdd)] = entry.attrs_MagicDmgAdd;
		tmp.attrs[tosizet(AttrID::PhysDmgAdd)] = entry.attrs_PhysDmgAdd;
		tmp.attrs[tosizet(AttrID::MagicDmgDec)] = entry.attrs_MagicDmgDec;
		tmp.attrs[tosizet(AttrID::PhysDmgDec)] = entry.attrs_PhysDmgDec;
		tmp.attrs[tosizet(AttrID::DrugRestoreSpeed)] = entry.attrs_DrugRestoreSpeed;
		tmp.attrs[tosizet(AttrID::DrugCdPhase)] = entry.attrs_DrugCdPhase;
		tmp.attrs[tosizet(AttrID::FiveDimAttr)] = entry.attrs_FiveDimAttr;
		tmp.attrs[tosizet(AttrID::GoldAddPer)] = entry.attrs_GoldAddPer;
		auto value = std::make_shared<stEffectDataLoaderBase>(tmp);
		if (!m_effectData.emplace(entry.dwID, value).second)
		{
			return;
		}
	}
}

void JsonConfig::LoadSubMonDropItemConfig()
{
	std::vector<mydb_submondropitem_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_submondropitem_tbl.json)");
	m_subMonDropItem.clear();
	m_subMonDropItem.reserve(tmp_data.size());
	for (auto const& entry : tmp_data) {
		auto value = std::make_shared<stSubDropItemDataBase>(entry);
		for (auto &it : value->m_itemDataBases)
		{
			auto fresh = sJsonConfig.GetItemDataById(it.itemId);
			if (!fresh)
				throw std::runtime_error("子掉落装备数据加载失败, 物品编号: " + std::to_string(it.itemId) + ", 子掉落包ID: " + std::to_string(entry.nID));
			it.m_itemDataBase = fresh;
		}
		if (!m_subMonDropItem.emplace(entry.nID, value).second)
		{
			//throw std::runtime_error("重复的 submondropitem ID: " + std::to_string(entry.nID));
		}
	}
}

void JsonConfig::LoadMonDropItemConfig()
{
	std::vector<mydb_mondropitem_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_mondropitem_tbl.json)");
	m_monDropItem.clear();
	m_monDropItem.reserve(tmp_data.size());
	for (auto const& entry : tmp_data) {
		auto value = std::make_shared<stDropItemDataBase>(entry);
		for (auto it:entry.vItems)
		{
			value->dropItemSet.emplace_back(it);
			if (it.tr)
			{
				auto fresh = sJsonConfig.GetSubDropDataBase(it.tr);
				if (!fresh)
					throw std::runtime_error("获取子掉落数据失败, 怪物编号: " + std::to_string(entry.nID) + ", 子掉落ID: " + std::to_string(it.tr));
				value->dropItemSet.back().m_subDropData = fresh;
			}
		}
		if (!m_monDropItem.emplace(entry.nID, value).second)
		{
			//throw std::runtime_error("重复的 mondropitem ID: " + std::to_string(entry.nID));
		}
	}
}

void JsonConfig::LoadItemConfig() {
	std::vector<mydb_item_base_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_item_base_tbl.json)");

	m_itemData.clear();
	m_itemData.reserve(tmp_data.size());

	for (auto const& row : tmp_data) {
		auto value = std::make_shared<stItemDataBase>(row);
		auto fresh = sJsonConfig.GetEffectDataById(value->dwWarriorEffectId);
		if (!fresh)
			throw std::runtime_error("获取物品表效果数据失败, 物品ID: " + std::to_string(value->nID) + ",效果ID: " + std::to_string(value->dwWarriorEffectId));
		value->m_effectDataBase = fresh;
		auto [it, inserted] = m_itemData.emplace(row.nID, value);
		if (!inserted)
			throw std::runtime_error("重复的 item ID: " + std::to_string(row.nID));
	}
}

void JsonConfig::LoadSpecialEffectConfig()
{
	std::vector<mydb_specialeffect_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_specialeffect_tbl.json)");
	m_specialEffectConfig.clear();
	m_specialEffectConfig.reserve(tmp_data.size());
	for (auto const& entry : tmp_data) {
		auto value = std::make_shared<mydb_specialeffect_tbl>(entry);
		if (!m_specialEffectConfig.emplace(entry.dwID, std::move(value)).second)
		{
			throw std::runtime_error("重复的 specialeffect ID: " + std::to_string(entry.dwID));
		}
	}
}

void JsonConfig::LoadMonsterConfig()
{
	std::vector<mydb_monster_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_monster_tbl.json)");
	m_monsterConfig.clear();
	m_monsterConfig.reserve(tmp_data.size());
	for (auto const& entry : tmp_data) {
		auto value = std::make_shared<stMonsterDataBase>(entry);
		if (!m_monsterConfig.emplace(entry.nID, value).second)
		{
			return;
		}
	}
}

void JsonConfig::LoadMapConfig()
{
	std::vector<mydb_mapinfo13824_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_mapinfo13824_tbl.json)");
	m_mapConfig.clear();
	m_mapConfig.reserve(tmp_data.size());
	for (auto const& entry : tmp_data) {
		auto value = std::make_shared<stServerMapInfo>(entry);
		value->name = UTG(entry.szName.c_str());
		if (!m_mapConfig.emplace(entry.dwMapID, value).second)
		{
			return;
		}
	}
}

void JsonConfig::LoadBuffConfig()
{
	std::vector<mydb_magicbuff_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_magicbuff_tbl.json)");
	buffMap.clear();
	buffMap.reserve(tmp_data.size());
	for (auto const& entry : tmp_data) {
		auto value = std::make_shared<stBuffDataBase>(entry);
		auto fresh = sJsonConfig.GetEffectDataById(value->dwEffectId);
		if (!fresh)
			throw std::runtime_error("获取BUFF表效果数据失败, BUFF_ID: " + std::to_string(value->dwID) + ",效果ID: " + std::to_string(value->dwEffectId));
		value->m_effectDataBase = fresh;
		if (!buffMap.emplace(BuffKey(entry.dwID, entry.btLevel), value).second)
			return;
	}
}

void JsonConfig::LoadMagicConfig()
{
	std::vector<mydb_magic_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_magic_tbl.json)");
	m_magicData.clear();
	m_magicData.reserve(tmp_data.size());
	for (auto const& entry : tmp_data) {
		auto value = std::make_shared<stMagicDataBase>(entry);
		auto fresh = sJsonConfig.GetEffectDataById(value->dwEffectid);
		if (!fresh)
			throw std::runtime_error("获取魔法表效果数据失败, 技能ID: " + std::to_string(value->nID) + ",效果ID: " + std::to_string(value->dwEffectid));
		value->m_effectDataBase = fresh;
		if (!m_magicData.emplace(BuffKey(entry.nID, entry.btlevel), value).second)
			return;
	}
}


void JsonConfig::LoadSpawnData()
{
	spawn_data.clear();
	iguana::from_json_file(spawn_data, R"(.\data\mydb_mongen_tbl.json)");
	last_spawn_time = time(nullptr);
}

void JsonConfig::LoadNpcSpawnData()
{
	npc_data.clear();
	iguana::from_json_file(npc_data, R"(.\data\mydb_npcgen_tbl.json)");
}

bool JsonConfig::LoadLvlAbilityData()
{
	std::vector<mydb_playerability_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_playerability_tbl.json)");
	m_lvlAbilityData.clear();
	m_lvlAbilityData.reserve(tmp_data.size());
	for (auto const& entry : tmp_data) {
		if (!m_lvlAbilityData.emplace(LvlAbilityKey(entry.dwLevel, entry.dwJob), entry).second)
		{
			return false;
		}
	}
	return true;
}

bool JsonConfig::LoadMapGateData()
{
	std::vector<mydb_mapgate_tbl> tmp_data;
	iguana::from_json_file(tmp_data, R"(.\data\mydb_mapgate_tbl.json)");
	m_mapGateData.reserve(tmp_data.size());
	for (auto const& entry : tmp_data) {
		if (!m_mapGateData.emplace(MapGateKey(entry.dwSrcMapId, entry.wSx, entry.wSy), entry).second)
		{
			//return false;
		}
	}
	return true;
}

mydb_mongen_tbl* JsonConfig::GetMonGenData(uint32_t index)
{
	index--;
	if (index < spawn_data.size()) {
		return &spawn_data[index];
	}
	return nullptr;
}

mydb_npcgen_tbl* JsonConfig::GetNpcGen(uint32_t index)
{
	for (auto& npc:npc_data)
	{
		if (npc.id == index)
		{
			return &npc;
		}
	}
	return nullptr;
}

const mydb_playerability_tbl* JsonConfig::GetLvlAbilityData(int16_t lvl, int16_t job) const
{
	if (auto it = m_lvlAbilityData.find({ lvl, job }); it != m_lvlAbilityData.end()) {
		return &it->second;
	}
	return nullptr;
}

const mydb_mapgate_tbl* JsonConfig::GetGateData(int mapId,int16_t x, int16_t y) const
{
	if (auto it = m_mapGateData.find({ mapId, x,y }); it != m_mapGateData.end()) {
		return &it->second;
	}
	return nullptr;
}

std::shared_ptr<mydb_specialeffect_tbl> JsonConfig::GetSpecialEffectDataById(int effectId)
{
	if (auto it = m_specialEffectConfig.find(effectId); it != m_specialEffectConfig.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<stEffectDataLoaderBase> JsonConfig::GetEffectDataById(int effectId)
{
	if (auto it = m_effectData.find(effectId); it != m_effectData.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<stItemDataBase> JsonConfig::GetItemDataById(int itemId)
{
	if (auto it = m_itemData.find(itemId); it != m_itemData.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<stBuffDataBase> JsonConfig::GetBuffDataBase(BuffKey key)
{
	if (auto it = buffMap.find(key); it != buffMap.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<stMagicDataBase> JsonConfig::GetMagicDataBase(uint32_t id,uint8_t lv)
{
	if (auto it = m_magicData.find({id,lv}); it != m_magicData.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<stMonsterDataBase> JsonConfig::GetMonsterDataBase(int id)
{
	if (auto it = m_monsterConfig.find(id); it != m_monsterConfig.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<stDropItemDataBase> JsonConfig::GetDropDataBase(int id)
{
	if (auto it = m_monDropItem.find(id); it != m_monDropItem.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<stSubDropItemDataBase> JsonConfig::GetSubDropDataBase(int id)
{
	if (auto it = m_subMonDropItem.find(id); it != m_subMonDropItem.end()) {
		return it->second;
	}
	return nullptr;
}

std::shared_ptr<stServerMapInfo> JsonConfig::GetMapDataBase(int id)
{
	if (auto it = m_mapConfig.find(id); it != m_mapConfig.end()) {
		return it->second;
	}
	return nullptr;
}
