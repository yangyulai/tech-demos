#include "WorldBoss.h"
#include "UsrEngn.h"

void WorldBossMgr::SetOpen(bool isOpen)
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	data->worldBoss.isOpen = isOpen;
}

bool WorldBossMgr::GetOpen()
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	return data->worldBoss.isOpen;
}

std::tuple<int, DWORD> WorldBossMgr::GetSort(std::string_view strName)
{
	if (!strName.data()) { return {0,0}; }
	SharedData* data = CUserEngine::getMe().m_shareData.data();

	WorldBossMgr::Sort();

	int nSort = 0;
	DWORD dwDamage = 0;

	for (size_t i = 0; i < data->worldBoss.worldBossRank.size(); i++)
	{
		if (strcmp(data->worldBoss.worldBossRank[i].szName, "") == 0)
		{
			break;
		}
		if (strcmp(data->worldBoss.worldBossRank[i].szName, strName.data()) == 0)
		{
			nSort = i + 1;
			dwDamage = data->worldBoss.worldBossRank[i].dwDamage;
		}
	}
	return { nSort,dwDamage };
}

bool WorldBossMgr::Update(std::string_view strPlayername, DWORD dwDamage)
{
	if (!strPlayername.data()) { return false; }

	SharedData* data = CUserEngine::getMe().m_shareData.data();
	for (size_t i = 0; i < data->worldBoss.worldBossRank.size(); i++)
	{
		auto& mem = data->worldBoss.worldBossRank[i];
		if (strcmp(mem.szName, strPlayername.data()) == 0 || strcmp(mem.szName, "") == 0)
		{
			boNeedSort = true;
			data->worldBoss.worldBossRank[i].dwDamage = dwDamage;
			strcpy_s(data->worldBoss.worldBossRank[i].szName, sizeof(data->worldBoss.worldBossRank[i].szName), strPlayername.data());
			return true;
		}
	}

	//不在榜上，检测可以上榜
	Sort();
	for (size_t i = 0; i < data->worldBoss.worldBossRank.size(); i++)
	{
		auto& mem = data->worldBoss.worldBossRank[i];
		if (mem.dwDamage < dwDamage)
		{
			boNeedSort = true;
			data->worldBoss.worldBossRank[data->worldBoss.worldBossRank.size() - 1] = mem;

			strcpy_s(mem.szName, sizeof(mem.szName), strPlayername.data());
			mem.dwDamage = dwDamage;
			break;
		}
	}

	return true;
}

void WorldBossMgr::Sort()
{
	if (boNeedSort)
	{
		SharedData* data = CUserEngine::getMe().m_shareData.data();
		std::sort(data->worldBoss.worldBossRank.begin(), data->worldBoss.worldBossRank.end(), [](auto& a, auto& b) {
			return a.dwDamage > b.dwDamage;
			});
		boNeedSort = false;
	}
}

bool WorldBossMgr::Clear()
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	ZeroMemory(&data->worldBoss, sizeof(data->worldBoss));
	return true;
}

sol::table WorldBossMgr::GetN(int num, sol::this_state ts)
{
	if (num > 0)
	{
		SharedData* data = CUserEngine::getMe().m_shareData.data();
		if (num <= data->worldBoss.worldBossRank.size())
		{
			sol::state_view lua(ts);
			auto table = lua.create_table(num);

			WorldBossMgr::Sort();

			for (size_t i = 0; i < num; i++)
			{
				if (strcmp(data->worldBoss.worldBossRank[i].szName, "") == 0)
				{
					break;
				}
				table[i+1] = lua.create_table_with(
					"n", data->worldBoss.worldBossRank[i].szName,
					"d", data->worldBoss.worldBossRank[i].dwDamage
				);
			}
			return table;
		}
	}
	return sol::nil;
}

void WorldBossMgr::SendRankReward(sol::function func, sol::this_state ts)
{
	if (!func.valid()){	return;}
	WorldBossMgr::Sort();
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	try {
		sol::state_view lua(ts);
		auto table = lua.create_table();

		for (int i = 0; i < data->worldBoss.worldBossRank.size(); i++) {

			if (strcmp(data->worldBoss.worldBossRank[i].szName, "") == 0)
			{
				break;
			}
			int sort = i + 1;
			auto& item = data->worldBoss.worldBossRank[i];
			func(sort, item.szName, table);
			if (table.valid())
			{
				std::string title = table["title"];
				std::string content = table["content"];
				ActivityCommon::SendMail(0, item.szName, title.c_str(), content.c_str(), table["items"]);
			}
		}
	}
	catch (const sol::error& e) {
		g_logger.error("WorldBossMgr::SendRankReward error %s", e.what());
	}
}

bool WorldBossMgr::GetSendReward()
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	return data->worldBoss.isWBNeedReward;
}

void WorldBossMgr::SetSendReward(bool isSend)
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	data->worldBoss.isWBNeedReward = isSend;
}

DWORD WorldBossMgr::GetBossHp()
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	return data->worldBoss.dwWorldBossHp;
}

void WorldBossMgr::SetBossHp(DWORD dwHp)
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	if (dwHp >= 0)
	{
		data->worldBoss.dwWorldBossHp = dwHp;
	}
}

bool WorldBossMgr::GetAlive()
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	return data->worldBoss.isAlive;
}

void WorldBossMgr::SetAlive(bool isAllive)
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	data->worldBoss.isAlive = isAllive;
}

BYTE WorldBossMgr::GetStage()
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	return data->worldBoss.btStage;
}

void WorldBossMgr::SetStage(BYTE btStage)
{
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	data->worldBoss.btStage = btStage;
}
