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
	bool m_boOpenQuestEvent = false; //�Ƿ����ű��¼�
	bool m_fastTransfer = true; //�����з�ģʽ
	uint32_t m_gMoveIntervalTime = 520; //�ƶ�
	uint32_t m_searchViewIntervalTime = 3600 * 24 * 1000; //����������Ұ���ʱ��
	int sWarriorMoveInterval = 216;
	int sRushTime = 65;
	int sBackTime = 65;
	BYTE PlayerProtectionlv;					//���ֱ����ȼ�
	bool closePetRun;
	bool closeMonsterSearch;
	bool closePlayerEvent;
	bool openPacketPrint;
	bool m_bDebug;								// �˺���ӡ�ܿ���
	bool m_bAtkDebug;							// ����ʱ�˺���ӡ����
	bool m_bDefDebug;							// ������ʱ�˺���ӡ����
	bool m_bBaseDebug;							// ��������ֵ��ӡ����
	bool m_bCounterDebug;						// �����˺���ӡ����
	bool m_bRobot;								// �����˴�ӡ����
	int m_fastestAttackSpeed;					// ��칥���ٶ�
	int m_fastestReleaseSpeed;					// ����ͷ��ٶ�
	int m_shareDrugBaseCd;						// ����ҩƷ����cd
	int m_shareDrugMinCd;						// ����ҩƷ��Сcd
	int m_drugStageChangePer;					// ����ҩƷ�����׶θı�%

	std::vector<int> m_AttackSpeed;			     // �����ٶȽ׶�
	std::vector<int> m_MoveSpeed;			     // �ƶ��ٶȽ׶�
	std::vector<int> m_ReleaseSpeed;			 // �ͷ��ٶȽ׶�
	std::vector<stEquipAtt> m_WeaponBoxerAtt;    // ��������������� ȭʦ
    std::vector<stEquipAtt> m_WeaponMageAtt;     // ��������������� ��ʦ
    std::vector<stEquipAtt> m_WeaponSwordAtt;    // ��������������� ��ʦ
    std::vector<stEquipAtt> m_WeaponGunRndAtt;   // ��������������� ǹʦ
	std::vector<stEquipAtt> m_ArmorRndAtt;       // ���������������
	std::vector<int> m_GroupExpAdd;              // ��Ӿ���ӳ�����
	std::vector<stEquipAtt> m_GuardAttackAtt;    // �ػ������������������
	std::vector<stEquipAtt> m_GuardDefenseAtt;   // �ػ������������������
	std::vector<stEquipAtt> m_GuardSupportAtt;   // �ػ������������������
};

#define sConfigMgr ConfigMgr::instance()
