#include "GuildFortress.h"
#include "UsrEngn.h"
#include "ActivityCommon.h"

void CGuildFortress::ActivityStatus(BYTE btStatus)
{
	stGlobalGuildFortressStatus cmd;
	cmd.btStatus = btStatus;
	CUserEngine::getMe().SendMsg2GlobalSvr(&cmd, sizeof(cmd));
}

sol::table CGuildFortress::SignUpTab(sol::this_state ts)
{
	sol::state_view lua(ts);
	sol::table tab = lua.create_table();
	int index = 1;
	for (auto it = CUserEngine::getMe().m_mGuilds.begin(); it != CUserEngine::getMe().m_mGuilds.end(); it++)
	{
		if (it->second.btFortressSignUp)
		{
			tab[index++] = lua.create_table_with(
				"name", GTU(it->second.szGuildName),
				"count", it->second.dwCurGuildPlayerCount
			);
		}
	}
	return tab;
}

BYTE CGuildFortress::GetAuto(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.btFortressAuto;
	}
	return 0;
}

bool CGuildFortress::SignUp(DWORD dwGuildId, __int64 i64OnlyId)
{
	if (dwGuildId > 0)
	{
		std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
		if (it != CUserEngine::getMe().m_mGuilds.end())
		{
			stGlobalGuildFortressSign cmd;
			cmd.dwGuildId = dwGuildId;
			cmd.i64OnlyId = i64OnlyId;
			CUserEngine::getMe().SendMsg2GlobalSvr(&cmd, sizeof(cmd));
		}
	}
	return false;
}

bool CGuildFortress::SetAutoSignUp(DWORD dwGuildId, bool isAutoSign, __int64 i64OnlyId)
{
	if (dwGuildId > 0)
	{
		std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
		if (it != CUserEngine::getMe().m_mGuilds.end())
		{
			stGlobalGuildFortressAutoSign cmd;
			cmd.dwGuildId = dwGuildId;
			cmd.isAutoSign = isAutoSign;
			cmd.i64OnlyId = i64OnlyId;
			CUserEngine::getMe().SendMsg2GlobalSvr(&cmd, sizeof(cmd));
		}
	}
	return false;
}

BYTE CGuildFortress::GetSignUp(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.btFortressSignUp;
	}
	return 0;
}

void CGuildFortress::UpdateDoorDam(CGameMap* pMap, DWORD dwGuildId, DWORD dwAddDamage)
{
	if (pMap && dwGuildId && dwAddDamage)
	{
		auto nCurDam = pMap->quest_vars_get_var_n("FortressDoorDamage");
		pMap->quest_vars_set_var_n("FortressDoorDamage", nCurDam + dwAddDamage, false);
		if (pMap->quest_vars_get_var_n("FortressDoorMapGuildId") == 0)
		{
			pMap->quest_vars_set_var_n("FortressDoorMapGuildId", dwGuildId, false);
		}
	}
}

void CGuildFortress::UpdateDoorDamRank(CGameMap* pMap)
{
	if (!pMap) return;
	if (DWORD dwGuildId = pMap->quest_vars_get_var_n("FortressDoorMapGuildId")) {
		if (auto nCurDam = pMap->quest_vars_get_var_n("FortressDoorDamage")) {
			auto& sharedrank = CUserEngine::getMe().m_shareData.data()->guildFortressDamage;
			for (size_t i = 0; i < sharedrank.size(); i++){
				auto& rankinfo = sharedrank[i];
				if (rankinfo.dwGuildId == dwGuildId){
					rankinfo.dwDamage = nCurDam;
					if (i > 0 && sharedrank[i].dwDamage > sharedrank[i - 1].dwDamage){
						std::sort(sharedrank.begin(), sharedrank.end(), [](stGuildFortressDoorDam& a, stGuildFortressDoorDam& b)->bool {return a.dwDamage > b.dwDamage; });
					}
					return;
				}
			}
			// ÐÂÉÏ°ñ
			if (nCurDam > sharedrank[sharedrank.size() - 1].dwDamage){
				sharedrank[sharedrank.size() - 1].dwDamage = nCurDam;
				sharedrank[sharedrank.size() - 1].dwGuildId = dwGuildId;
				std::sort(sharedrank.begin(), sharedrank.end(), [](stGuildFortressDoorDam& a, stGuildFortressDoorDam& b)->bool {return a.dwDamage > b.dwDamage; });
			}
		}
	}
}

uint8_t CGuildFortress::GetMyDoorRank(uint32_t guildid) {
	auto& sharedrank = CUserEngine::getMe().m_shareData.data()->guildFortressDamage;
	for (size_t i = 0; i < sharedrank.size(); i++) {
		auto& rankinfo = sharedrank[i];
		if (rankinfo.dwGuildId == guildid) {			
			return i+1;
		}
	}
	return 0;
}

sol::table CGuildFortress::GetDoorRankTab( uint8_t maxCount, sol::this_state ts) {
	auto& sharedrank = CUserEngine::getMe().m_shareData.data()->guildFortressDamage;
	if (maxCount <= 0 || maxCount > sharedrank.size())
	{
		return sol::nil;
	}

	sol::state_view lua(ts);
	sol::table tab = lua.create_table();

	int nOffset = sharedrank.size() - maxCount;
	for (size_t i = 0; i < sharedrank.size()- nOffset; i++)
	{
		auto& rankinfo = sharedrank[i];
		auto it = CUserEngine::getMe().m_mGuilds.find(rankinfo.dwGuildId);
		if (rankinfo.dwGuildId > 0 && it != CUserEngine::getMe().m_mGuilds.end())
		{
			tab[tab.size() + 1] = lua.create_table_with(
				"guildid", rankinfo.dwGuildId,
				"name", GTU(it->second.szGuildName),
				"dam", rankinfo.dwDamage
			);
		}
	}
	return tab;
}

DWORD CGuildFortress::GetCurEmblemId()
{
	return CUserEngine::getMe().m_shareData.data()->dwFortressEmblemId;
}

void CGuildFortress::SetCurEmblemId(DWORD dwEmblemId)
{
	CUserEngine::getMe().m_shareData.data()->dwFortressEmblemId = dwEmblemId;
}

DWORD CGuildFortress::GetCurGuildId()
{
	return CUserEngine::getMe().m_shareData.data()->dwFortressGuildId;
}

void CGuildFortress::SetCurGuildId(DWORD dwGuildId)
{
	CUserEngine::getMe().m_shareData.data()->dwFortressGuildId = dwGuildId;
}

DWORD CGuildFortress::GetZoneDonate()
{
	return CUserEngine::getMe().m_shareData.data()->dwFortressDonate;
}

void CGuildFortress::ChangeZoneDonate(int dwDonagte)
{
	CUserEngine::getMe().m_shareData.data()->dwFortressDonate += dwDonagte;
}

void CGuildFortress::AllocCloneMap(int64_t i64Onlyid, DWORD dwGuildId, DWORD dwMapid, DWORD dwKeepTime)
{
	if (dwGuildId)
	{
		stGuildCloneMap cmd;
		cmd.dwGuildId = dwGuildId;
		cmd.i64Onlyid = i64Onlyid;
		cmd.dwMapid = dwMapid;
		cmd.dwKeepTime = dwKeepTime;
		CUserEngine::getMe().SendMsg2GlobalSvr(&cmd, sizeof(cmd));
	}
}

void CGuildFortress::UpdateDonateRank(CPlayerObj* pPlayer, int nDonate, BYTE btMaxRankCount) {
	auto& ranklist = CUserEngine::getMe().m_shareData.data()->fortressDonateRank;
	if (pPlayer && btMaxRankCount > 0 && btMaxRankCount <= ranklist.size())
	{
		for (size_t i = 0; i < btMaxRankCount; i++)
		{
			auto& rankinfo = ranklist[i];
			if (pPlayer->m_i64UserOnlyID == rankinfo.i64UserOnlyId)
			{
				rankinfo.dwDonate = nDonate;
				if (i > 0 && ranklist[i].dwDonate > ranklist[i - 1].dwDonate) {
					std::sort(ranklist.begin(), ranklist.end(), [](stFortressDonateRank& a, stFortressDonateRank& b) {return a.dwDonate > b.dwDonate; });
				}
				return;
			}
		}

		auto& minDonateInfo = ranklist[btMaxRankCount - 1];
		if (nDonate > minDonateInfo.dwDonate) {
			std::vector<uint64_t> vecOnlyId;
			for (auto rit = ranklist.rbegin() - (ranklist.size() - btMaxRankCount); rit != ranklist.rend(); rit++)
			{
				if (nDonate > rit->dwDonate) {
					vecOnlyId.push_back(rit->i64UserOnlyId);
				}
			}
			minDonateInfo.dwDonate = nDonate;
			minDonateInfo.i64UserOnlyId = pPlayer->m_i64UserOnlyID;
			strcpy_s(minDonateInfo.szName, sizeof(minDonateInfo.szName), pPlayer->GetName().data());
			std::sort(ranklist.begin(), ranklist.end(), [](stFortressDonateRank& a, stFortressDonateRank& b) {return a.dwDonate > b.dwDonate; });	

			for (auto id : vecOnlyId) {
				CALL_LUA("GuildFortressDonateRankChange", id);
			}

			return;
		}
	}
}

BYTE CGuildFortress::GetMyDonateRank(CPlayerObj* pPlayer, BYTE btMaxRankCount) {
	auto& ranklist = CUserEngine::getMe().m_shareData.data()->fortressDonateRank;
	if (btMaxRankCount > 0 && btMaxRankCount <= ranklist.size()){
		for (size_t i = 0; i < btMaxRankCount; i++)
		{
			auto& rankinfo = ranklist[i];
			if (pPlayer->m_i64UserOnlyID == rankinfo.i64UserOnlyId)
			{
				return i + 1;
			}
		}
	}
	return 0;
}

bool CGuildFortress::ClearDonateRank() {
	auto& ranklist = CUserEngine::getMe().m_shareData.data()->fortressDonateRank;
	ranklist.fill({});
	return true;
}


sol::table CGuildFortress::GetDonateRankTab(BYTE btMaxRankCount, sol::this_state ts) {
	auto& ranklist = CUserEngine::getMe().m_shareData.data()->fortressDonateRank;
	if (btMaxRankCount > 0 && btMaxRankCount <= ranklist.size()) {
		sol::state_view lua(ts);
		sol::table tab = lua.create_table();
		for (size_t i = 0; i < btMaxRankCount; i++)
		{
			auto& rankinfo = ranklist[i];
			tab[i+1] = lua.create_table_with(
				"name",GTU(rankinfo.szName),
				"donate",rankinfo.dwDonate
			);
		}
		return tab;
	}
	return sol::nil;
}

void CGuildFortress::AddStageOnePlayer(uint32_t dwGuildId, uint64_t onlyid)
{
	m_stage1Player[dwGuildId].insert(onlyid);
}

void CGuildFortress::ClearStageOneMap()
{
	m_stage1Player.clear();
}

void CGuildFortress::SendStageOneReward(uint32_t dwGuildId, std::string_view title, std::string_view content, sol::table rewardtab)
{
	if (title.empty() || content.empty() || !rewardtab.size()) return;
	for (auto onlyid : m_stage1Player[dwGuildId])
	{
		if (onlyid > 0)
		{
			ActivityCommon::SendMail(onlyid, "", title.data(), content.data(), rewardtab);
		}
	}
}
