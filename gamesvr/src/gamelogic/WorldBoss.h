#pragma once
#include "ActivityCommon.h"

class WorldBossMgr
{
public:
	inline static bool boNeedSort = false;

	static void SetOpen(bool isOpen);
	static bool GetOpen();
	static std::tuple<int,DWORD> GetSort(std::string_view strName);
	static bool Update(std::string_view strPlayername, DWORD dwDamage);
	static void Sort();
	static bool Clear();
	static sol::table GetN(int num, sol::this_state ts);
	static void SendRankReward(sol::function func, sol::this_state ts);
	static bool GetSendReward();
	static void SetSendReward(bool isSend);
	static DWORD GetBossHp();
	static void SetBossHp(DWORD dwHp);
	static bool GetAlive();
	static void SetAlive(bool isAllive);
	static BYTE GetStage();
	static void SetStage(BYTE btStage);

};

