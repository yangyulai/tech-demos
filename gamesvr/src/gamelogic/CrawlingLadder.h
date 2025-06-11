#pragma once
#include "ActivityCommon.h"

class CrawlingLadderMgr
{
public:
	inline static bool boNeedSort = false;

	static bool Update(__int64 PlayerId, BYTE btFloor,DWORD dwRecordTime, DWORD dwFinishTime, std::string_view strName);
	inline static void Sort();
	static int GetRank(__int64 PlayerId);
	static sol::table GetRankInfo(BYTE btType, sol::this_state ts); // 0 utf-8; 1 gbk
	static sol::table GetAssignedRank(int num, sol::this_state ts);
	static void SetRankInfo(const sol::table& table);
	static bool Clear();

};