#pragma once
#include "CrawlingLadder.h"
#include "UsrEngn.h"


void CrawlingLadderMgr::Sort()
{
	if (boNeedSort)
	{
		SharedData* data = CUserEngine::getMe().m_shareData.data();
		std::sort(data->CrawlingLRank.begin(), data->CrawlingLRank.end(), [](CrawlingLadderRank& a, CrawlingLadderRank& b) {
			if (a.btFloor != b.btFloor)
			{
				return a.btFloor > b.btFloor;
			}
			else if (a.dwRecordTime != b.dwRecordTime)
			{
				return a.dwRecordTime < b.dwRecordTime;
			}
			else if (a.dwFinishTime != b.dwFinishTime)
			{
				return a.dwFinishTime < b.dwFinishTime;
			}
			});
	}

}


bool CrawlingLadderMgr::Update(__int64 PlayerId,BYTE btFloor, DWORD dwRecordTime, DWORD dwFinishTime, std::string_view strName)
{
	if (PlayerId <= 0) { return false; }
	if (!strName.data()) { return false; }
	boNeedSort = false;
	//在榜上，更新数据
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	for (size_t i = 0; i < data->CrawlingLRank.size(); i++)
	{
		auto& mem = data->CrawlingLRank[i];
		if (PlayerId == mem.i64UserOnlyId)
		{
			boNeedSort = true;
			strcpy_s(data->CrawlingLRank[i].szName, sizeof(data->CrawlingLRank[i].szName), UTG(strName.data()));
			data->CrawlingLRank[i].btFloor = btFloor;
			data->CrawlingLRank[i].dwRecordTime = dwRecordTime;
			data->CrawlingLRank[i].dwFinishTime = dwFinishTime;
		}
	}
	if (!boNeedSort) {
		for (size_t i = 0; i < data->CrawlingLRank.size(); i++)
		{
			auto& mem = data->CrawlingLRank[i];
			if (btFloor > mem.btFloor)
			{
				boNeedSort = true;
			}
			else if (mem.btFloor == btFloor)
			{
				if (mem.dwRecordTime > dwRecordTime)
				{
					boNeedSort = true;
				}
				else if (mem.dwRecordTime == dwRecordTime && mem.dwFinishTime > dwFinishTime)
				{
					boNeedSort = true;
				} 
			}
			if (boNeedSort)
			{
				data->CrawlingLRank[data->CrawlingLRank.size() - 1] = mem;
				strcpy_s(data->CrawlingLRank[i].szName, sizeof(data->CrawlingLRank[i].szName), UTG(strName.data()));
				data->CrawlingLRank[i].i64UserOnlyId = PlayerId;
				data->CrawlingLRank[i].btFloor = btFloor;
				data->CrawlingLRank[i].dwRecordTime = dwRecordTime;
				data->CrawlingLRank[i].dwFinishTime = dwFinishTime;
				break;
			}
		}
	}
	Sort();
	return true;
}


int CrawlingLadderMgr::GetRank(__int64 PlayerId)
{
	if (PlayerId <= 0) { return 0; }
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	for (size_t i = 0; i < data->CrawlingLRank.size(); i++)
	{
		auto& mem = data->CrawlingLRank[i];
		if (PlayerId == mem.i64UserOnlyId)
		{
			return i + 1;
		}
	}
	return 0;
}

sol::table CrawlingLadderMgr::GetAssignedRank(int num, sol::this_state ts)
{
	if (num > 0 && num <= _MAX_CRAWLINGLADDER_RANK)
	{
		sol::state_view lua(ts);

		SharedData* data = CUserEngine::getMe().m_shareData.data();
		auto& mem = data->CrawlingLRank[num-1];
		sol::table rankInfo = lua.create_table();
		rankInfo["PlayerId"] = mem.i64UserOnlyId;
		rankInfo["Floor"] = mem.btFloor;
		rankInfo["RecordTime"] = mem.dwRecordTime;
		rankInfo["FinishTime"] = mem.dwFinishTime;
		return rankInfo;
	}
	return sol::nil;

}

sol::table CrawlingLadderMgr::GetRankInfo(BYTE btType, sol::this_state ts)
{
	sol::state_view lua(ts);
	sol::table rankInfo = lua.create_table();

	SharedData* data = CUserEngine::getMe().m_shareData.data();
	for (size_t i = 0; i < data->CrawlingLRank.size(); ++i) {
		auto& mem = data->CrawlingLRank[i];
		sol::table rankEntry = lua.create_table();
		if (btType == 0) {
			rankEntry["Name"] = GTU(mem.szName);
		}else {
			rankEntry["Name"] = mem.szName;
		}
		rankEntry["PlayerId"] = mem.i64UserOnlyId;
		rankEntry["Floor"] = mem.btFloor;
		rankEntry["RecordTime"] = mem.dwRecordTime;
		rankEntry["FinishTime"] = mem.dwFinishTime;
		rankInfo[i + 1] = rankEntry;
	}
	return rankInfo;
}


void CrawlingLadderMgr::SetRankInfo(const sol::table& table) {
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	data->CrawlingLRank.fill(CrawlingLadderRank{}); // 清空当前排名数据

	size_t index = 0;
	for (const auto& pair : table) {
		if (index >= _MAX_CRAWLINGLADDER_RANK) break;

		sol::table rankEntry = pair.second.as<sol::table>();
		CrawlingLadderRank& mem = data->CrawlingLRank[index];

		mem.i64UserOnlyId = rankEntry["PlayerId"].get_or(__int64(0));
		mem.btFloor = rankEntry["Floor"].get_or(BYTE(0));
		mem.dwRecordTime = rankEntry["RecordTime"].get_or(DWORD(0));
		mem.dwFinishTime = rankEntry["FinishTime"].get_or(DWORD(0));
		std::string name = rankEntry["Name"].get_or<std::string>("");
		strcpy_s(mem.szName, sizeof(mem.szName), name.c_str());

		++index;
	}
	boNeedSort = true;
	Sort();
}


bool CrawlingLadderMgr::Clear()
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	data->CrawlingLRank.fill(CrawlingLadderRank{}); // 使用默认构造函数初始化数组中的每个元素
	boNeedSort = false;
	return true;
}