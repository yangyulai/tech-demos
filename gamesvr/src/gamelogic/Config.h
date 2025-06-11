#pragma once

#include <sol/table.hpp>
#include "cmd/cret_misc_cmd.h"

class ConfigMgr
{
public:
	ConfigMgr(const ConfigMgr& other) = delete;
	ConfigMgr(ConfigMgr&& other) noexcept = delete;
	ConfigMgr& operator=(const ConfigMgr& other) = delete;
	ConfigMgr& operator=(ConfigMgr&& other) noexcept = delete;
	ConfigMgr() = default;
	~ConfigMgr() = default;

	static ConfigMgr& instance();
	bool LoadLuaConfig(const sol::table& table);
	void LoadAttackSpeedConfig(const sol::table& table);
	void LoadMoveSpeedConfig(const sol::table& table);
	void LoadReleaseSpeedConfig(const sol::table& table);
	int GetAttackSpeed(int index) const;
	int GetFastestAttackSpeed() const;
	int GetMoveSpeed(int index) const;
	int GetReleaseSpeed(int index) const;
	int GetFastestReleaseSpeed() const;
	int GetShareDrugBaseCd() const;
	int GetShareDrugMinCd() const;
	int GetDrugStageChange() const;
	void LoadWeaponBoxerAttConfig(const sol::table& table);
	void LoadWeaponMageAttConfig(const sol::table& table);
	void LoadWeaponSwordAttConfig(const sol::table& table);
	void LoadWeaponGunAttConfig(const sol::table& table);
	void LoadArmorAttConfig(const sol::table& table);
	void LoadGroupExpAddConfig(const sol::table& table);
	void LoadDebugConfig(const sol::table& table);
	void LoadGuardAttackConfig(const sol::table& table);
	void LoadGuardDefenseConfig(const sol::table& table);
	void LoadGuardSupportConfig(const sol::table& table);
	int GetGroupExpAdd(int index) const;
	bool IsAtkDebug() const { return m_bDebug && m_bAtkDebug; };
	bool IsDefDebug() const { return m_bDebug && m_bDefDebug;};
	bool IsBaseValDebug() const { return m_bDebug && m_bBaseDebug; };
	bool IsCounterDebug() const { return m_bDebug && m_bCounterDebug; };
	void LoadProtectionlvConfig(int nLevel);

	int sMapRunIdleInterval = 0;
	int sMapRunHasBabyInterval = 0;
	int sProcessMonInterval = 0;
	int sProcessNpcInterval = 0;
	bool m_boOpenQuestEvent = false; //是否开启脚本事件
	bool m_fastTransfer = true; //快速切服模式
	uint32_t m_gMoveIntervalTime = 520; //移动
	uint32_t m_searchViewIntervalTime = 3600 * 24 * 1000; //主动搜索视野间隔时间
	int sWarriorMoveInterval = 216;
	int sRushTime = 65;
	int sBackTime = 65;
	BYTE PlayerProtectionlv;					//新手保护等级
	bool closePetRun;
	bool closeMonsterSearch;
	bool closePlayerEvent;
	bool openPacketPrint;
	bool m_bDebug;								// 伤害打印总开关
	bool m_bAtkDebug;							// 攻击时伤害打印开关
	bool m_bDefDebug;							// 被攻击时伤害打印开关
	bool m_bBaseDebug;							// 基础攻击值打印开关
	bool m_bCounterDebug;						// 反击伤害打印开关
	bool m_bRobot;								// 机器人打印开关
	int m_fastestAttackSpeed;					// 最快攻击速度
	int m_fastestReleaseSpeed;					// 最快释放速度
	int m_shareDrugBaseCd;						// 公共药品基础cd
	int m_shareDrugMinCd;						// 公共药品最小cd
	int m_drugStageChangePer;					// 公共药品单个阶段改变%

	std::vector<int> m_AttackSpeed;			     // 攻击速度阶段
	std::vector<int> m_MoveSpeed;			     // 移动速度阶段
	std::vector<int> m_ReleaseSpeed;			 // 释放速度阶段
	std::vector<stEquipAtt> m_WeaponBoxerAtt;    // 武器随机属性配置 拳师
    std::vector<stEquipAtt> m_WeaponMageAtt;     // 武器随机属性配置 法师
    std::vector<stEquipAtt> m_WeaponSwordAtt;    // 武器随机属性配置 剑师
    std::vector<stEquipAtt> m_WeaponGunRndAtt;   // 武器随机属性配置 枪师
	std::vector<stEquipAtt> m_ArmorRndAtt;       // 防具随机属性配置
	std::vector<int> m_GroupExpAdd;              // 组队经验加成配置
	std::vector<stEquipAtt> m_GuardAttackAtt;    // 守护攻击型随机属性配置
	std::vector<stEquipAtt> m_GuardDefenseAtt;   // 守护防御型随机属性配置
	std::vector<stEquipAtt> m_GuardSupportAtt;   // 守护辅助型随机属性配置
};

#define sConfigMgr ConfigMgr::instance()
