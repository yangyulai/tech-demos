#include "Config.h"
#include "BaseCreature.h"

ConfigMgr& ConfigMgr::instance()
{
    static ConfigMgr instance;
    return instance;
}

bool ConfigMgr::LoadLuaConfig(const sol::table& table)
{
	if (!table.valid()) return false;
	int interval = table.get_or("mapRunIdle", 400);
	sMapRunIdleInterval = interval <= 0 ? 1 : interval;
	interval = table.get_or("mapRunHasBaby", 100);
	sMapRunHasBabyInterval = interval <= 0 ? 1 : interval;
	sProcessMonInterval = table.get_or("procmoninterval", 2);
	sProcessNpcInterval = table.get_or("procnpcinterval", 3);
	sWarriorMoveInterval = table.get_or("moveInterval1", 400);
	sRushTime = table.get_or("rushTime", 185);
	sBackTime = table.get_or("backTime", 185);
	closePetRun = table.get_or("closePetRun", false);
	closeMonsterSearch = table.get_or("closeMonsterSearch", false);
	closePlayerEvent = table.get_or("closePlayerEvent", false);
	openPacketPrint = table.get_or("openPacketPrint", false);
	m_boOpenQuestEvent = table.get_or("openQuestEvent", false);
	m_fastTransfer = table.get_or("fastTransfer", true);
	m_gMoveIntervalTime = table.get_or("moveIntervalTime", 520);
	m_searchViewIntervalTime = table.get_or("searchViewIntervalTime", 3600 * 24 * 1000);
	m_fastestAttackSpeed = table.get_or("fastestAttackSpeed", 230);
	m_fastestReleaseSpeed = table.get_or("fastestReleaseSpeed", 276);
	m_shareDrugBaseCd = table.get_or("shareDrugBaseCd", 800);
	m_shareDrugMinCd = table.get_or("shareDrugMinCd", 184);
	m_drugStageChangePer = table.get_or("drugStageChangePer", 1100);
    return false;
}
void ConfigMgr::LoadAttackSpeedConfig(const sol::table& table)
{
	m_AttackSpeed.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadAttckSpeedConfig");
	}
	const size_t length = table.size();
	m_AttackSpeed.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<int>()) {
			m_AttackSpeed.push_back(elem.as<int>());
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected int");
		}
	}
}

int ConfigMgr::GetAttackSpeed(int index) const
{
	index--;
	if (index < 0 || index >= m_AttackSpeed.size()) {
		return 0;
	}
	return m_AttackSpeed[index];
}

int ConfigMgr::GetFastestAttackSpeed() const {
	return m_fastestAttackSpeed;
}

int ConfigMgr::GetFastestReleaseSpeed() const {
	return m_fastestReleaseSpeed;
}

int ConfigMgr::GetShareDrugBaseCd() const {
	return m_shareDrugBaseCd;
}

int ConfigMgr::GetShareDrugMinCd() const {
	return m_shareDrugMinCd;
}

int ConfigMgr::GetDrugStageChange() const {
	return m_drugStageChangePer;
}

void ConfigMgr::LoadMoveSpeedConfig(const sol::table& table)
{
	m_MoveSpeed.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadMoveSpeedConfig");
	}
	const size_t length = table.size();
	m_MoveSpeed.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<int>()) {
			m_MoveSpeed.push_back(elem.as<int>());
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected int");
		}
	}
}

int ConfigMgr::GetMoveSpeed(int index) const
{
	index--;
	if (index < 0 || index >= m_MoveSpeed.size()) {
		return 0;
	}
	return m_MoveSpeed[index];
}

void ConfigMgr::LoadReleaseSpeedConfig(const sol::table& table)
{
	m_ReleaseSpeed.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadReleaseSpeedConfig");
	}
	const size_t length = table.size();
	m_ReleaseSpeed.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<int>()) {
			m_ReleaseSpeed.push_back(elem.as<int>());
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected int");
		}
	}
}

int ConfigMgr::GetReleaseSpeed(int index) const
{
	index--;
	if (index < 0 || index >= m_ReleaseSpeed.size()) {
		return 0;
	}
	return m_ReleaseSpeed[index];
}

void ConfigMgr::LoadWeaponBoxerAttConfig(const sol::table& table)
{
	m_WeaponBoxerAtt.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadReleaseSpeedConfig");
	}
	const size_t length = table.size();
	m_WeaponBoxerAtt.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<sol::table>()) {
			stEquipAtt att;
			att.btFrom = table[i]["from"];
			att.btType = table[i]["type"];
			att.nMinNum = table[i]["minNum"];
			att.nMaxNum = table[i]["maxNum"];
			m_WeaponBoxerAtt.push_back(std::move(att));
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected table");
		}
	}
}

void ConfigMgr::LoadWeaponMageAttConfig(const sol::table& table)
{
	m_WeaponMageAtt.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadWeaponMageAttConfig");
	}
	const size_t length = table.size();
	m_WeaponMageAtt.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<sol::table>()) {
			stEquipAtt att;
			att.btFrom = table[i]["from"];
			att.btType = table[i]["type"];
			att.nMinNum = table[i]["minNum"];
			att.nMaxNum = table[i]["maxNum"];
			m_WeaponMageAtt.push_back(std::move(att));
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected table");
		}
	}
}

void ConfigMgr::LoadWeaponSwordAttConfig(const sol::table& table)
{
	m_WeaponSwordAtt.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadWeaponSwordAttConfig");
	}
	const size_t length = table.size();
	m_WeaponSwordAtt.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<sol::table>()) {
			stEquipAtt att;
			att.btFrom = table[i]["from"];
			att.btType = table[i]["type"];
			att.nMinNum = table[i]["minNum"];
			att.nMaxNum = table[i]["maxNum"];
			m_WeaponSwordAtt.push_back(std::move(att));
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected table");
		}
	}
}

void ConfigMgr::LoadWeaponGunAttConfig(const sol::table& table)
{
	m_WeaponGunRndAtt.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadWeaponGunAttConfig");
	}
	const size_t length = table.size();
	m_WeaponGunRndAtt.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<sol::table>()) {
			stEquipAtt att;
			att.btFrom = table[i]["from"];
			att.btType = table[i]["type"];
			att.nMinNum = table[i]["minNum"];
			att.nMaxNum = table[i]["maxNum"];
			m_WeaponGunRndAtt.push_back(std::move(att));
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected table");
		}
	}
}

void ConfigMgr::LoadArmorAttConfig(const sol::table& table)
{
	m_ArmorRndAtt.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadArmorAttConfig");
	}
	const size_t length = table.size();
	m_ArmorRndAtt.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<sol::table>()) {
			stEquipAtt att;
			att.btFrom = table[i]["from"];
			att.btType = table[i]["type"];
			att.nMinNum = table[i]["minNum"];
			att.nMaxNum = table[i]["maxNum"];
			m_ArmorRndAtt.push_back(std::move(att));
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected table");
		}
	}
}

void ConfigMgr::LoadGuardAttackConfig(const sol::table& table)
{
	m_GuardAttackAtt.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadGuardAttackConfig");
	}
	const size_t length = table.size();
	m_GuardAttackAtt.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<sol::table>()) {
			stEquipAtt att;
			att.btFrom = table[i]["from"];
			att.btType = table[i]["type"];
			att.nMinNum = table[i]["minNum"];
			att.nMaxNum = table[i]["maxNum"];
			m_GuardAttackAtt.push_back(std::move(att));
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected table");
		}
	}
}

void ConfigMgr::LoadGuardDefenseConfig(const sol::table& table)
{
	m_GuardDefenseAtt.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadGuardDefenseConfig");
	}
	const size_t length = table.size();
	m_GuardDefenseAtt.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<sol::table>()) {
			stEquipAtt att;
			att.btFrom = table[i]["from"];
			att.btType = table[i]["type"];
			att.nMinNum = table[i]["minNum"];
			att.nMaxNum = table[i]["maxNum"];
			m_GuardDefenseAtt.push_back(std::move(att));
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected table");
		}
	}
}

void ConfigMgr::LoadGuardSupportConfig(const sol::table& table)
{
	m_GuardSupportAtt.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadGuardSupportConfig");
	}
	const size_t length = table.size();
	m_GuardSupportAtt.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<sol::table>()) {
			stEquipAtt att;
			att.btFrom = table[i]["from"];
			att.btType = table[i]["type"];
			att.nMinNum = table[i]["minNum"];
			att.nMaxNum = table[i]["maxNum"];
			m_GuardSupportAtt.push_back(std::move(att));
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected table");
		}
	}
}


// 组队经验加成配置
void ConfigMgr::LoadGroupExpAddConfig(const sol::table& table)
{
	m_GroupExpAdd.clear();
	if (!table.valid()) {
		throw std::runtime_error("Invalid table passed to LoadGroupExpAddConfig");
	}
	const size_t length = table.size();
	m_GroupExpAdd.reserve(length);
	for (size_t i = 1; i <= length; ++i) {
		sol::object elem = table[i];
		if (elem.is<int>()) {
			m_GroupExpAdd.push_back(elem.as<int>());
		}
		else {
			throw std::runtime_error("Invalid element type in table, expected int");
		}
	}
}

// 伤害打印配置
void ConfigMgr::LoadDebugConfig(const sol::table& table)
{
	if (!table.valid()) return;
	m_bDebug = table.get_or("isopen", false);
	m_bAtkDebug = table.get_or("atk", false);
	m_bDefDebug = table.get_or("def", false);
	m_bBaseDebug = table.get_or("baseval", false);
	m_bCounterDebug = table.get_or("counterval", false);
	m_bRobot = table.get_or("robot", false);
	return;
}

int ConfigMgr::GetGroupExpAdd(int index) const
{
	index--;
	if (index < 0 || index >= m_GroupExpAdd.size()) {
		return 0;
	}
	return m_GroupExpAdd[index];
}

void ConfigMgr::LoadProtectionlvConfig(int nLevel)
{
	if (nLevel) {
		PlayerProtectionlv = nLevel;
	}
}

