#include "SkyLadder.h"
#include "PlayerObj.h"
#include "UsrEngn.h"
#include "../dbsvrGameConnecter.h"
#include "Robot.h"

CSkyLadder::CSkyLadder()
{
	m_nextRunTickCount = 0;
}

void CSkyLadder::run()
{
	FUNCTION_BEGIN;
	ULONGLONG dwRunStartTick = ::GetTickCount64();
	if (dwRunStartTick > m_nextRunTickCount) {
		AILOCKT(m_waitCreateRobot);
		while (m_waitCreateRobot.size() > 0) {
			auto createcmd = m_waitCreateRobot.front();
			
			CPlayerObj* pPlayerUser = CUserEngine::getMe().m_playerhash.FindByOnlyId(createcmd->i64OnlyId);
			if (pPlayerUser)
			{
				if (CRobot* robot = CreateRobotMon(pPlayerUser, &createcmd->gamedata)) {
					pPlayerUser->quest_vars_set_var_n("SkyLadderMonObjectId", robot->GetObjectId(), false);
					CALL_LUA("SkyLadderRobotDrug", robot);
				}
			}
			__mt_char_dealloc(createcmd);
			m_waitCreateRobot.pop_front();
		}
		m_nextRunTickCount = GetTickCount64() + 50;
	}
}

void CSkyLadder::SendLoadDataCmdToDB(CPlayerObj* pPlayer, int64_t i64DstOnlyid, std::string strAcctount)
{
	if (i64DstOnlyid <= 0 || strAcctount.length() <=0 ) return;
	stLoadDataToRobot sendcmd;
	sendcmd.i64OnlyId = pPlayer->m_i64UserOnlyID;
	sendcmd.i64DstOnlyid = i64DstOnlyid;
	sendcmd.dwZoneid = GameService::getMe().m_nZoneid;
	sendcmd.wTradeId = GameService::getMe().m_nTradeid;	
	sendcmd.m_tmpid = pPlayer->m_pGateUser->m_tmpid;
	strcpy_s(sendcmd.szAccount, sizeof(sendcmd.szAccount), strAcctount.c_str());
	pPlayer->m_pGateUser->m_OwnerDbSvr->sendcmdex(sendcmd);
}

void CSkyLadder::doLoadRobotData(stLoadDataToRobotRet* pCmd)
{
	int nsavedatasize = pCmd->getSize();
	stLoadDataToRobotRet* pgamedatbuffer = (stLoadDataToRobotRet*)__mt_char_alloc(nsavedatasize + 32);
	if (pgamedatbuffer) {
		CopyMemory(pgamedatbuffer, pCmd, nsavedatasize);
		AILOCKT(m_waitCreateRobot);
		m_waitCreateRobot.emplace_back(pgamedatbuffer);
	}
}

CRobot* CSkyLadder::CreateRobotMon(CPlayerObj* pPlayer, stLoadPlayerData* pGameData) {
	FUNCTION_BEGIN;

	if (int64_t mapfullid = pPlayer->quest_vars_get_var_n("SkyLadder_MapFullId")) {
		if (auto pMap = CUserEngine::getMe().m_maphash.FindByFullId(mapfullid)) {
			pGameData->wclonemapid = pMap->getMapCloneId();
			pGameData->dwmapid = pMap->getMapId();
			pGameData->x = CALL_LUARET<int>("SkyLadder_GetGenXY", 0, 0);
			pGameData->y = CALL_LUARET<int>("SkyLadder_GetGenXY", 0, 1);
			pGameData->btgmlvl = 0;
			pGameData->i64UserOnlyId = 0;
			return pMap->AddRobotMon(pGameData, true);
		}
	}
	return nullptr;
}

CRobot* CSkyLadder::CreateRobotMon(CGameMap* pMap, PosType x, PosType y, sol::table tab) {
	FUNCTION_BEGIN;
	if (!tab.valid() || !pMap) { return nullptr; }
	stLoadPlayerData pGameData;
	ZEROOBJ(&pGameData);
	pGameData.wclonemapid = pMap->getMapCloneId();
	pGameData.dwmapid = pMap->getMapId();
	pGameData.x = x;
	pGameData.y = y;
	strcpy_s(pGameData.szName, sizeof(pGameData.szName), UTG(tab["name"].get_or<std::string>("").c_str()));
	pGameData.siFeature.sex = tab["sex"].get_or(1);
	pGameData.siFeature.job = tab["job"].get_or(1);
	pGameData.siFeature.face = tab["face"].get_or(0);
	pGameData.siFeature.hair = tab["hair"].get_or(0);
	pGameData.siFeature.eye = tab["eye"].get_or(0);
	pGameData.siFeature.nose = tab["nose"].get_or(0);
	pGameData.siFeature.mouth = tab["mouth"].get_or(0);

	BYTE btSex = pGameData.siFeature.sex;
	BYTE btJob = pGameData.siFeature.job;
	auto db = sJsonConfig.GetItemDataById(tab["weapon"].get_or(0));
	if (db) pGameData.siFeature.weapon = CALL_LUARET<DWORD>("buildClientResIdWithJobSex", 0, btJob, btSex, db);
	db =  sJsonConfig.GetItemDataById(tab["dress"].get_or(0));
	if (db) pGameData.siFeature.dress = CALL_LUARET<DWORD>("buildClientResIdWithJobSex", 0, btJob, btSex, db);
	db =  sJsonConfig.GetItemDataById(tab["shoe"].get_or(0));
	if (db) pGameData.siFeature.shoe = CALL_LUARET<DWORD>("buildClientResIdWithJobSex", 0, btJob, btSex, db);
	db =  sJsonConfig.GetItemDataById(tab["back"].get_or(0));
	if (db) pGameData.siFeature.back = CALL_LUARET<DWORD>("buildClientResIdWithJobSex", 0, btJob, btSex, db);
	db =  sJsonConfig.GetItemDataById(tab["helmet"].get_or(0));
	if (db) pGameData.siFeature.helmet = CALL_LUARET<DWORD>("buildClientResIdWithJobSex", 0, btJob, btSex, db);
	db =  sJsonConfig.GetItemDataById(tab["pants"].get_or(0));
	if (db) pGameData.siFeature.pants = CALL_LUARET<DWORD>("buildClientResIdWithJobSex", 0, btJob, btSex, db);

	pGameData.nlevel = tab["lv"].get_or(0);
	auto pRobot = pMap->AddRobotMon(&pGameData, false, tab["attr"].get_or(0));
	if (pRobot)
	{
		pRobot->m_stAbility.i64FightScore = tab["score"].get_or(0);
		if (tab["skill"].valid() && tab["skill"].is<sol::table>())
		{
			sol::table skilltab = tab["skill"];
			for (size_t i = 0; i < skilltab.size(); i++)
			{
				auto elem = skilltab[i + 1];
				if (elem.is<sol::table>())
				{
					DWORD nSkillId = elem[1].get_or(0);
					DWORD nLevel = elem[2].get_or(0);
					if (nSkillId > 0 && nLevel > 0)
					{
						pRobot->learnSkill(nSkillId, nLevel);
					}
				}
			}
		}
		//pRobot->FeatureChanged(true);
	}
	
	return pRobot;
}

