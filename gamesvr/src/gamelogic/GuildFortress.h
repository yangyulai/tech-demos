#pragma once
#include "CMD/Guild_cmd.h"
#include "lua_base.h"
#include "DynamicMap.h"

class CGameMap;
class CPlayerObj;

class CGuildFortress
{
public:
	~CGuildFortress() = default;

	DynamicMap<int, int> m_ballDamage1; //所有军团对球的伤害，只有一个图
	DynamicMap<int, int> m_ballDamage2; //所有军团对球的伤害，只有一个图
	DynamicMap<int, int> m_ballDamage3; //所有军团对球的伤害，只有一个图
	DynamicMap<int, int> m_ballDamage4; //所有军团对球的伤害，只有一个图
	DynamicMap<int64_t, DynamicArray<2>> m_contributeMap; // 玩家贡献表
	DynamicMap<int, DynamicArray<3>> m_ballPowerRank; //能量排行榜
	std::unordered_map<uint32_t, std::set<uint64_t>> m_stage1Player; // 1阶段参与表 guildid,set<onlyid>
	std::unordered_map<uint32_t, std::set<uint64_t>> m_stage2Player; // 2阶段参与表 guildid,set<onlyid>

	void ActivityStatus(BYTE btStatus);
	sol::table SignUpTab(sol::this_state ts);	// 军团已报名列表
	BYTE GetAuto(DWORD dwGuildId);
	bool SignUp(DWORD dwGuildId, __int64 i64OnlyId);
	bool SetAutoSignUp(DWORD dwGuildId, bool isAutoSign, __int64 i64OnlyId);
	BYTE GetSignUp(DWORD dwGuildId);
	void UpdateDoorDam(CGameMap* pMap, DWORD dwGuildId, DWORD dwAddDamage);
	void UpdateDoorDamRank(CGameMap* pMap); //更新军团对门伤害排行
	uint8_t GetMyDoorRank(uint32_t guildid);
	sol::table GetDoorRankTab(uint8_t maxCount, sol::this_state ts);
	DWORD GetCurEmblemId();
	void SetCurEmblemId(DWORD dwEmblemId);	//设置占领军团徽章
	DWORD GetCurGuildId();	//获取当前占领军团id
	void SetCurGuildId(DWORD dwGuildId);	//设置占领军团
	DWORD GetZoneDonate();	//获取全服捐献
	void ChangeZoneDonate(int dwDonagte);
	void AllocCloneMap(int64_t i64Onlyid, DWORD dwGuildId, DWORD dwMapid, DWORD dwKeepTime); //向global 申请副本
	void UpdateDonateRank(CPlayerObj* pPlayer, int nDonate, BYTE btMaxRankCount);
	BYTE GetMyDonateRank(CPlayerObj* pPlayer, BYTE btMaxRankCount);
	bool ClearDonateRank();
	sol::table GetDonateRankTab(BYTE btMaxRankCount, sol::this_state ts);
	void AddStageOnePlayer(uint32_t dwGuildId, uint64_t onlyid);
	void ClearStageOneMap();
	void SendStageOneReward(uint32_t dwGuildId, std::string_view title, std::string_view content, sol::table rewardtab);
private:

};
