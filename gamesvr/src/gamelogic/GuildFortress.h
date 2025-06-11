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

	DynamicMap<int, int> m_ballDamage1; //���о��Ŷ�����˺���ֻ��һ��ͼ
	DynamicMap<int, int> m_ballDamage2; //���о��Ŷ�����˺���ֻ��һ��ͼ
	DynamicMap<int, int> m_ballDamage3; //���о��Ŷ�����˺���ֻ��һ��ͼ
	DynamicMap<int, int> m_ballDamage4; //���о��Ŷ�����˺���ֻ��һ��ͼ
	DynamicMap<int64_t, DynamicArray<2>> m_contributeMap; // ��ҹ��ױ�
	DynamicMap<int, DynamicArray<3>> m_ballPowerRank; //�������а�
	std::unordered_map<uint32_t, std::set<uint64_t>> m_stage1Player; // 1�׶β���� guildid,set<onlyid>
	std::unordered_map<uint32_t, std::set<uint64_t>> m_stage2Player; // 2�׶β���� guildid,set<onlyid>

	void ActivityStatus(BYTE btStatus);
	sol::table SignUpTab(sol::this_state ts);	// �����ѱ����б�
	BYTE GetAuto(DWORD dwGuildId);
	bool SignUp(DWORD dwGuildId, __int64 i64OnlyId);
	bool SetAutoSignUp(DWORD dwGuildId, bool isAutoSign, __int64 i64OnlyId);
	BYTE GetSignUp(DWORD dwGuildId);
	void UpdateDoorDam(CGameMap* pMap, DWORD dwGuildId, DWORD dwAddDamage);
	void UpdateDoorDamRank(CGameMap* pMap); //���¾��Ŷ����˺�����
	uint8_t GetMyDoorRank(uint32_t guildid);
	sol::table GetDoorRankTab(uint8_t maxCount, sol::this_state ts);
	DWORD GetCurEmblemId();
	void SetCurEmblemId(DWORD dwEmblemId);	//����ռ����Ż���
	DWORD GetCurGuildId();	//��ȡ��ǰռ�����id
	void SetCurGuildId(DWORD dwGuildId);	//����ռ�����
	DWORD GetZoneDonate();	//��ȡȫ������
	void ChangeZoneDonate(int dwDonagte);
	void AllocCloneMap(int64_t i64Onlyid, DWORD dwGuildId, DWORD dwMapid, DWORD dwKeepTime); //��global ���븱��
	void UpdateDonateRank(CPlayerObj* pPlayer, int nDonate, BYTE btMaxRankCount);
	BYTE GetMyDonateRank(CPlayerObj* pPlayer, BYTE btMaxRankCount);
	bool ClearDonateRank();
	sol::table GetDonateRankTab(BYTE btMaxRankCount, sol::this_state ts);
	void AddStageOnePlayer(uint32_t dwGuildId, uint64_t onlyid);
	void ClearStageOneMap();
	void SendStageOneReward(uint32_t dwGuildId, std::string_view title, std::string_view content, sol::table rewardtab);
private:

};
