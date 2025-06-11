#include <register_lua.hpp>
#include "Script.h"
#include "BaseCreature.h"
#include "UsrEngn.h"
#include "Chat.h"
#include "Trade.h"
#include <winapiini.h>
#include "Config.h"
#include "GlobalVarSession.h"
#include "cmd/GuildPackage.h"
#include "cmd/TencentApi_cmd.h"
#include "DynamicMap.h"


void LuaAddRelationDegree(CPlayerObj* player,double dRelationOnlyId,int nAddDegree)
{
	if (player) {
		stRelationAddRelationDegree retcmd;
		retcmd.i64OnlyId = (__int64)dRelationOnlyId;
		retcmd.nAddDegree = nAddDegree;
		retcmd.btType = LIST_FRIEND;
		SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv,player->m_i64UserOnlyID,&retcmd,sizeof(retcmd));
	}
}

void LuaResetRelationDegree(CPlayerObj* player)
{
	if (player) {
		stRelationResetRelationDegree retcmd;
		SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv,player->m_i64UserOnlyID,&retcmd,sizeof(retcmd));
	}
}
void LuaExplorePlayer(CPlayerObj* player,const char* name)
{
	if (player) {
		stRelationExplorePlayer retcmd;
		strcpy_s(retcmd.szName,_MAX_NAME_LEN_-1,name);
		SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv,player->m_i64UserOnlyID,&retcmd,sizeof(retcmd));
	}
}

CGameMap* LuaCreateForCloneMap(CGameMap* pSrcMap,bool boAuto,DWORD dwExsitTime, uint8_t lineId){
	FUNCTION_BEGIN;
	if (pSrcMap){
		uint16_t dwCloneid=CUserEngine::getMe().getMapCloneId();
		if (dwCloneid==0){
			g_logger.error("副本克隆ID获取错误(%d)",dwCloneid);
			return NULL;
		}
		stSvrMapId svrMapId{ pSrcMap->getMapId(),lineId,dwCloneid };
		auto mapDataBase = pSrcMap->GetMapDataBase();
		if (!mapDataBase) return nullptr;
		CGameMap* pMap=CLD_DEBUG_NEW CGameMap(mapDataBase, svrMapId);
		if (pMap->Clone(pSrcMap)){
			pMap->m_dwCloneMapExistTime = time(NULL)+dwExsitTime;
			if (!CUserEngine::getMe().m_maphash.m_gameMaps.s_insert(svrMapId,pMap)){
				g_logger.error("加载了重复的副本地图 %s(%d:%d)-%.8x", mapDataBase->szName.c_str(), mapDataBase->dwMapID,pMap->getMapCloneId(),pSrcMap);
				SAFE_DELETE(pMap);
			}else{
				stUpdateExistCloneMap sendmapcmd;
				sendmapcmd.boAdd = true;
				sendmapcmd.nMapID = MAKELONG(pMap->getMapId(),pMap->getMapCloneId());
				GameService* gamesvr=GameService::instance();
				gamesvr->Send2LoginSvrs(&sendmapcmd,sizeof(sendmapcmd));
				gamesvr->Send2DBSvrs(&sendmapcmd,sizeof(sendmapcmd));
				return pMap;
			}
		}
	}
	return NULL;
}

CGameMap* LuaCreateForCloneMapId(DWORD dwmapid, uint8_t lineId,bool boAuto,DWORD dwExsitTime){
	FUNCTION_BEGIN;
	CGameMap* pSrcMap=CUserEngine::getMe().m_maphash.FindById(dwmapid, lineId);
	if (pSrcMap){
		return LuaCreateForCloneMap(pSrcMap,boAuto,dwExsitTime, lineId);
	}
	return NULL;
}

void reloadrank(int ranktype)
 {
	 stReloadRankByType ReloadRankCmd;
	 ReloadRankCmd.nRankType = ranktype;
	 //sprintf_s(ReloadRankCmd.szOpt,sizeof(ReloadRankCmd.szOpt),szopt);
	 BUFFER_CMD(stSendRankMsgSuperSrv,rankcmd,stBasePacket::MAX_PACKET_SIZE);
	 rankcmd->msg.push_back((char*)&ReloadRankCmd,sizeof(ReloadRankCmd),__FUNC_LINE__);
	 CUserEngine::getMe().SendMsg2SuperSvr(rankcmd,sizeof(*rankcmd)+rankcmd->msg.getarraysize());
	 g_logger.debug("脚本加载排行榜!");
 }

void resyncrank(int ranktype)
{
	BUFFER_CMD(stSendRankMsgSuperSrv, AllRankCmd, stBasePacket::MAX_PACKET_SIZE);
	stGameSvrGetRankTopTen ranktopcmd;
	ranktopcmd.nRankType = ranktype; // Rank_Max_Count
	AllRankCmd->msg.push_back((char*)&ranktopcmd, sizeof(stGameSvrGetRankTopTen), __FUNC_LINE__);
	CUserEngine::getMe().SendMsg2SuperSvr(AllRankCmd, sizeof(stSendRankMsgSuperSrv) + AllRankCmd->msg.getarraysize());
	g_logger.debug("脚本同步排行榜!");
}

void LuaSendGmManageMsg(BYTE btRetType,BYTE btErrorCode,const char* szName,const char* szBuf){
	FUNCTION_BEGIN;
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	retcmd->nErrorcode = btErrorCode;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,szName);
	retcmd->btRetType=btRetType;
	retcmd->retStr.push_back((char*)szBuf, strlen(szBuf),__FUNC_LINE__);
	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
}

double LuaGetTickCount(){
	return (double)GetTickCount64();
}

void LuaSetLogLevel(DWORD dwLogLevel){
	CUserEngine::getMe().m_gLogLevel=dwLogLevel;
}

const char* LuaI64ToChar(const char* szi64){
	__int64 tmpid=0;
	sscanf(szi64,"%I64d",&tmpid);
	if (tmpid){
		char szhhex[32]={0};
		char szlhex[32]={0};
		int nh=( (tmpid>>32) & 0xffffffff );
		if(nh!=0){
			sprintf_s( szhhex,sizeof(szhhex)-1,"%x", nh );
			sprintf_s( szlhex,sizeof(szlhex)-1,"%x", ( tmpid & 0xffffffff ) );
			return vformat("%s_%s",szlhex,szhhex );
		}else{
			return vformat("%x",( tmpid & 0xffffffff ) );
		}
	}
	return szi64;
}

CItem* LuaCreateItem(DWORD dwBaseid,DWORD num,int frommapid,const char* bornfrom,const char *szmaker){
	FUNCTION_BEGIN;
	if (!dwBaseid){return NULL;}
	return CItem::CreateItem(dwBaseid,_CREATE_MON_DROP,num,0,__FUNC_LINE__,frommapid,bornfrom,szmaker);
}

const char* LuaGetMapInfoName(){
	FUNCTION_BEGIN;
	return GameService::getMe().m_szMapTableName;
}

void LuaGetOnlineNum(sol::table table){
	FUNCTION_BEGIN;
	if (!table.valid()){return;}
	do 
	{
		stOnlineNum onlinenum;
		std::CSyncList< stOnlineNum >::iterator it;
		AILOCKT(CUserEngine::getMe().m_onlinenumlist);
		it=CUserEngine::getMe().m_onlinenumlist.begin();
		if (it!=CUserEngine::getMe().m_onlinenumlist.end()){
			onlinenum=(*it);
			table[1]=onlinenum.dwAllOnlineNum;
			for (DWORD i=0;i<onlinenum.vOnlineNumArr.size();i++)
			{
				stlink2<WORD,WORD> slink=onlinenum.vOnlineNumArr[i];
				table[slink._p1]=slink._p2;
			}
		}
	} while (false);
}

void luaclearcrossrank()
{
	stClearCrossRank retcmd;
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(retcmd));
}

void luadividecrossgroup(bool boForce,int nHour,int nMin)
{
	stDivideCrossGroup retcmd;
	retcmd.boForce = boForce;
	retcmd.nHour = nHour;
	retcmd.nMin = nMin;
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(retcmd));
}

//////////////////////////////////////////////////////////////////////////
void lua_supersvr_doscript(const char* szscript){
	if (!szscript || szscript[0] == '\0') return;
	BUFFER_CMD(stNotifyExecScript,pscriptcmd,stBasePacket::MAX_PACKET_SIZE);
	pscriptcmd->szScript.push_str(szscript);
	CUserEngine::getMe().SendMsg2SuperSvr(pscriptcmd,sizeof(*pscriptcmd)+pscriptcmd->szScript.getarraysize()+1);
}

void lua_gamesvr_doscript(const char* szscript,DWORD dwSvrIdType=0,bool exceptme=true){
	if (!szscript || szscript[0] == '\0') return;
	BUFFER_CMD(stNotifyExecScript,pscriptcmd,stBasePacket::MAX_PACKET_SIZE);
	pscriptcmd->szScript.push_str(szscript);
	CUserEngine::getMe().SendProxyMsg2Gamesvr(pscriptcmd,sizeof(*pscriptcmd)+pscriptcmd->szScript.getarraysize()+1,dwSvrIdType,exceptme);
}

void lua_user_doscript(const char* szName,const char* szscript){
	if (!szName || szName[0] == '\0' || !szscript || szscript[0] == '\0') return;
	BUFFER_CMD(stNotifyExecScript,pscriptcmd,stBasePacket::MAX_PACKET_SIZE);
	strcpy_s(pscriptcmd->szSrcName,sizeof(pscriptcmd->szSrcName)-1,szName);
	pscriptcmd->szScript.push_str(szscript);
	CUserEngine::getMe().SendProxyMsg2User(szName,pscriptcmd,sizeof(*pscriptcmd)+pscriptcmd->szScript.getarraysize()+1);
}

void lua_useronlyid_doscript(double dOnlyId,const char* szScript){
	if (!szScript || szScript[0] == '\0') return;
	BUFFER_CMD(stNotifyExecScript,pscriptcmd,stBasePacket::MAX_PACKET_SIZE);
	pscriptcmd->i64OnlyId=(__int64)dOnlyId;
	pscriptcmd->szScript.push_str(szScript);
	CUserEngine::getMe().SendProxyMsg2User((__int64)dOnlyId,pscriptcmd,sizeof(*pscriptcmd)+pscriptcmd->szScript.getarraysize()+1);
}

void lua_alluser_doscript(const char* szcript) {
	FUNCTION_BEGIN;
	for (auto it = CUserEngine::getMe().m_playerhash.begin(); it != CUserEngine::getMe().m_playerhash.end(); it++) {
		stAutoSetScriptParam autoparm(it->second);
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(szcript);
	}
}

void lua_mapuser_doscript(CGameMap* pEnvir,const char* szcript) {
	FUNCTION_BEGIN;
	if (pEnvir) {
		pEnvir->ForeachObjectsByType(CRET_PLAYER, [&](MapObject* pObj) {
			if (CPlayerObj* pPlayer = dynamic_cast<CPlayerObj*>(pObj)) {
				stAutoSetScriptParam autoparm(pPlayer);
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(szcript);
			}
			});
	}
}

bool SendGroupScriptMsg(DWORD dwGroupId,const char* szScriptMsg)
{
	if (!szScriptMsg || szScriptMsg[0] == '\0') return false;
	if(dwGroupId>0 && szScriptMsg[0]!=0){
		BUFFER_CMD(stGlobalSendGroupScriptMsg,retcmd,stBasePacket::MAX_PACKET_SIZE);
		retcmd->dwGroupId=dwGroupId;
		retcmd->szScriptMsg.push_str(szScriptMsg);
		return CUserEngine::getMe().SendMsg2GlobalSvr(retcmd,sizeof(stGlobalSendGroupScriptMsg)+retcmd->szScriptMsg.getarraysize()+1);
	}
	return false;
}

bool SendFriendScriptMsg(CPlayerObj* pPlayer,BYTE btType, const char* szScriptMsg,bool boSendToMe)
{
	if (!szScriptMsg || szScriptMsg[0] == '\0') return false;
	if (pPlayer) {
		BUFFER_CMD(stGlobalSendFriendScriptMsg,retcmd,stBasePacket::MAX_PACKET_SIZE);
		retcmd->i64OnlyId = pPlayer->m_i64UserOnlyID;
		retcmd->boSendToMe = boSendToMe;
		retcmd->btType = btType;
		retcmd->szScriptMsg.push_str(szScriptMsg);
		return CUserEngine::getMe().SendMsg2GlobalSvr(retcmd,sizeof(stGlobalSendFriendScriptMsg)+retcmd->szScriptMsg.getarraysize()+1);
	}
	return false;
}

void setGlobalVar(const char* szVarName,const char* szVarValue,bool bosave,const char* szVarRetFuc){
	if(szVarName[0]!=0){
		stGlobalVars retcmd;
		retcmd.btType=0;
		strcpy_s(retcmd.szVarName,sizeof(retcmd.szVarName)-1,szVarName);
		strcpy_s(retcmd.szVarValue,sizeof(retcmd.szVarValue)-1,szVarValue);
		strcpy_s(retcmd.szVarRetFuc,sizeof(retcmd.szVarRetFuc)-1,szVarRetFuc);
		retcmd.boSave=bosave;
		CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGlobalVars));
	}
}

void getGlobalVar(const char* szVarName,const char* szVarRetFuc){
	if(szVarName[0]!=0){
		stSysVars* pVar=GlobalVars::getMe().FindByName(szVarName);
		if(pVar){
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(szVarRetFuc,0,pVar->szVarValue);
			return;
		}else{
			stGlobalVars retcmd;
			retcmd.btType=1;
			strcpy_s(retcmd.szVarName,sizeof(retcmd.szVarName)-1,szVarName);
			strcpy_s(retcmd.szVarRetFuc,sizeof(retcmd.szVarRetFuc)-1,szVarRetFuc);
			CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGlobalVars));
		}
	}
}

const char* getLocalGlobalVar(const char* szVarName ){
	if (szVarName && szVarName[0] != 0) {
		stSysVars *pVar = GlobalVars::getMe().FindByName(szVarName);
		if (pVar) {
			return pVar->szVarValue;
		}
	}
	return "";
}

void removeGlobalVar(const char* szVarName,const char* szVarRetFuc){
	if(szVarName[0]!=0){
		stGlobalVars retcmd;
		retcmd.btType=2;
		strcpy_s(retcmd.szVarName,sizeof(retcmd.szVarName)-1,szVarName);
		strcpy_s(retcmd.szVarRetFuc,sizeof(retcmd.szVarRetFuc)-1,szVarRetFuc);
		CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGlobalVars));
	}
}

void GuildAddExp(DWORD dwGuildId,DWORD dwAdd,const char* pszEvent){
	if(dwGuildId && dwAdd){
		stGlobalGuildChangeExp retcmd;
		retcmd.btType=1;
		retcmd.dwGuild=dwGuildId;
		retcmd.dwAdd=dwAdd;
		if(pszEvent)
			sprintf_s(retcmd.szEvent,sizeof(retcmd.szEvent)-1,pszEvent);
		CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGlobalGuildChangeExp));
	}
}

void LuaGuildEventPush(DWORD dwGuildId, DWORD dwEventType, const char* szEvent){
	if(dwGuildId > 0 && szEvent){
		stGloalGuildEventPush retCmd;
		retCmd.dwGuildId = dwGuildId;
		retCmd.dwEventType = dwEventType;
		sprintf_s(retCmd.szEvent, sizeof(retCmd.szEvent)-1, szEvent);
		CUserEngine::getMe().SendMsg2GlobalSvr(&retCmd, sizeof(stGloalGuildEventPush));
	}
}

void LuaGuildEventsFetch(CPlayerObj* pPlayer, const char* szRetFunc) {
	if (pPlayer && szRetFunc && szRetFunc[0]) {
		stGloalGuildEventsFetch retCmd;
		strcpy_s(retCmd.szRetFunc, sizeof(retCmd.szRetFunc) - 1, szRetFunc);
		retCmd.i64OnlyId = pPlayer->m_i64UserOnlyID;
		retCmd.dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
		CUserEngine::getMe().SendMsg2GlobalSvr(&retCmd, sizeof(stGloalGuildEventsFetch));
	}
}

void GuildAddFlag(DWORD dwGuildId,DWORD dwAdd){
	if(dwGuildId && dwAdd){
		stGlobalGuildChangeFlag retcmd;
		retcmd.dwGuildId=dwGuildId;
		retcmd.dwAdd=dwAdd;
		CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGlobalGuildChangeFlag));
	}
}

void GuildAddWeekFieldBoss(DWORD dwGuildId,DWORD dwAdd){
	if(dwGuildId && dwAdd){
		stGlobalGuildChangeWeekBoss retcmd;
		retcmd.btType=1;
		retcmd.dwGuildId=dwGuildId;
		retcmd.dwAdd=dwAdd;
		CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGlobalGuildChangeWeekBoss));
	}
}

void GuildAddWeekGuildBoss(DWORD dwGuildId,DWORD dwAdd){
	if(dwGuildId && dwAdd){
		stGlobalGuildChangeWeekBoss retcmd;
		retcmd.btType=2;
		retcmd.dwGuildId=dwGuildId;
		retcmd.dwAdd=dwAdd;
		CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGlobalGuildChangeWeekBoss));
	}
}

void GuildCallMemberScript(DWORD dwGuild,const char* szScript)
{
	BUFFER_CMD(stGlobalGuildCallMemberScript,retcmd,stBasePacket::MAX_PACKET_SIZE);
	retcmd->dwGuildId = dwGuild;
	retcmd->szScript.push_str(szScript);
	CUserEngine::getMe().SendMsg2GlobalSvr(retcmd,sizeof(stGlobalGuildCallMemberScript)+retcmd->szScript.getarraysize()+1);
}


void GuildMembersFetch(DWORD dwGuildId, const char* szParam)
{
	stGlobalGuildMembersFetch retcmd;
	retcmd.dwGuildId = dwGuildId;
	DWORD paramLen = (DWORD)strlen(szParam) + 1;
	if (paramLen > _MAX_SCRIPTTIP_LEN_) {
		g_logger.error("GuildMembersFetch, 不支持参数超过:%d, 参数长度:%d", _MAX_SCRIPTTIP_LEN_, paramLen);
		return;
	}
	strcpy_s(retcmd.szParam, paramLen, szParam);
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGlobalGuildMembersFetch));
}

void LuaGuildDonate(sol::table const& table) {
	if (!table.valid()) {
		return;
	}
	BUFFER_CMD(stGuildDonate, retCmd, stBasePacket::MAX_PACKET_SIZE);
	retCmd->i64OnlyId = table.get_or<int64_t>("onlyId", 0);
	retCmd->nIndex = table.get_or("index", 0);
	retCmd->dwGuildId = table.get_or("guildId", 0);
	retCmd->nBuildDegree = table.get_or("buildDegree", 0);
	CUserEngine::getMe().SendMsg2GlobalSvr(retCmd, sizeof(*retCmd));
}

void LuaGuildLeveluUp(sol::table const& table) {
	if (!table.valid()) {
		return;
	}
	BUFFER_CMD(stGuildLevelUpGs, retCmd, stBasePacket::MAX_PACKET_SIZE);
	retCmd->i64OnlyId = table.get_or<int64_t>("onlyId", 0);
	retCmd->dwGuildId = table.get_or("guildId", 0);
	retCmd->nBuildDegree = table.get_or("buildDegree", 0);
	CUserEngine::getMe().SendMsg2GlobalSvr(retCmd, sizeof(*retCmd));
}

void LuaGuildClearCd(CPlayerObj* p) {
	if (p){
		BUFFER_CMD(stGuildClearCd, retCmd, stBasePacket::MAX_PACKET_SIZE);
		retCmd->i64OnlyId = p->m_i64UserOnlyID;
		CUserEngine::getMe().SendMsg2GlobalSvr(retCmd, sizeof(*retCmd));
	}
}

void LuaGuildQuest(sol::table const& table) {
	if (!table.valid()) {
		return;
	}
	BUFFER_CMD(stGuildQuest, retCmd, stBasePacket::MAX_PACKET_SIZE);
	retCmd->i64OnlyId = table.get_or<int64_t>("onlyId", 0);
	retCmd->nIndex = table.get_or("index", 0);
	retCmd->dwGuildId = table.get_or("guildId", 0);
	retCmd->nBuildDegree = table.get_or("buildDegree", 0);
	retCmd->nContribution = table.get_or("contribution", 0);
	retCmd->nAddContribution = table.get_or("addcontribution", 0);
	CUserEngine::getMe().SendMsg2GlobalSvr(retCmd, sizeof(*retCmd));
}

void LuaGuildRefreshMall(sol::table const& table) {
	if (!table.valid()) {
		return;
	}
	BUFFER_CMD(stGuildRefreshMall, retCmd, stBasePacket::MAX_PACKET_SIZE);
	retCmd->dwGuildId = table.get_or("guildId", 0);
	CUserEngine::getMe().SendMsg2GlobalSvr(retCmd, sizeof(*retCmd));
}

void GuildMasterInfo(DWORD dwGuild,DWORD dwType)
{
	stGlobalGuildMaster retcmd; 
	retcmd.dwGuildId = dwGuild;
	retcmd.dwType = dwType;
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGlobalGuildMaster));
}
const char* LuaGetGuildNameById(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return GTU(it->second.szGuildName);
	}
	return "";
}

DWORD LuaGetGuildLevelById(DWORD dwGuildId)
{
	std::map<DWORD,stGSALLGuild>::iterator it=CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if(it!=CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.dwGuildLevel;
	}
	return 0;
}

DWORD LuaGetCurGuildPlayerCountById(DWORD dwGuildId)
{
	std::map<DWORD,stGSALLGuild>::iterator it=CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if(it!=CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.dwCurGuildPlayerCount;
	}
	return 0;
}

const char* LuaGetGuildMasterNameById(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return GTU(it->second.szMasterName);
	}
	return "";
}
const char* LuaGetGuildMasterAccountById(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.szMasterAccount;
	}
	return "";
}

BYTE LuaGetGuildMasterJobById(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.btMasterJob;
	}
	return NO_JOB;
}

DWORD LuaGetGuildCreateTimeById(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.dwCreateTime;
	}
	return 0;
}

DWORD LuaGetGuildJoinNeedLvlById(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.dwJoinNeedLvl;
	}
	return 0;
}

BYTE LuaGetGuildJoinNeedZsLvlById(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.dwJoinNeedZsLvl;
	}
	return 0;
}

BYTE LuaGetGuildAutoJoinById(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.AutoJoin;
	}
	return 0;
}

DWORD LuaGetGuildMaxPlayerCountById(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.dwMaxPlayerCount;
	}
	return 0;
}

DWORD LuaGetGuildBuildDegree(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.dwBuildDegree;
	}
	return 0;
}

WORD LuaGetGuildMapSvrId(DWORD dwGuildId)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.wMapSvrId;
	}
	return 0;
}

void LuaGetGuildEmblemTab(CPlayerObj *p, sol::table table) {
	FUNCTION_BEGIN;
	if (!table.valid()) { return; }
	if (p)
	{
		for (auto data : p->m_GuildInfo.szEmblem)
		{
			if (data > 0) {
				table[data] = 1;
			}
			else
				break;
		}
	}
}

 void ClearRank(int nRank){
	 stClearRank retcmd;
	 retcmd.nRank = nRank;
	 CUserEngine::getMe().SendMsg2SuperSvr(&retcmd,sizeof(stClearRank));
 }

 bool isCrossSvr() {//当前区ID 65000-65535 的就是跨服服务器
	 return CUserEngine::getMe().isCrossSvr();
 }

 void UpdateWorld(DWORD dwTrueZoneid) {
	 stUpdateWorld retcmd;
	 retcmd.dwTrueZoneid = dwTrueZoneid;
	 CUserEngine::getMe().SendMsg2SuperSvr(&retcmd, sizeof(stUpdateWorld));
 }

bool SaveItemNpData(stItem* item, BYTE btFrom,BYTE bttype,int npnum){
	FUNCTION_BEGIN;
	bool isOk = false;
	for (int i=0;i<item->btNpPropertyCount && i < _MAX_NP_ALL_COUNT;i++) {
		if(btFrom == item->stNpProperty[i].ntNpFrom && item->stNpProperty[i].btNpType==bttype){
			item->stNpProperty[i].dwNpNum+=npnum;
			if (item->stNpProperty[i].dwNpNum == 0 && item->btNpPropertyCount <= _MAX_NP_ALL_COUNT){
				std::swap(item->stNpProperty[i], item->stNpProperty[item->btNpPropertyCount-1]);
				item->btNpPropertyCount--;
			}
			isOk = true;
			break;
		}
	}
	if (!isOk && item->btNpPropertyCount < _MAX_NP_ALL_COUNT) {
		int pos=item->btNpPropertyCount;
		item->stNpProperty[pos].ntNpFrom=btFrom;
		item->stNpProperty[pos].btNpType=bttype;
		item->stNpProperty[pos].dwNpNum=npnum;
		item->btNpPropertyCount++;
		isOk = true;
	}

	return isOk;
}

void ClearMailItem() {
	CUserEngine::getMe().ClearMailedItem();
}

 stItem* GetMailedItem(DWORD id, DWORD num, BYTE binding,int frommapid,const char* bornfrom,const char *szmaker)
 {
	 FUNCTION_BEGIN;
	auto pItemLoadBase= sJsonConfig.GetItemDataById(id);
	 if(pItemLoadBase == NULL)
	 {
		 return NULL;
	 }

	 if(num > max(pItemLoadBase->dwMaxCount, pItemLoadBase->nVariableMaxCount))
	 {
		 return NULL;
	 }

	 stItem* pItem = CUserEngine::getMe().GetMailedItem();
	 if (pItem){
		pItem->dwBaseID = id;
		pItem->dwBinding = binding;
		pItem->btBornFrom = _CREATE_MAIL;
		pItem->dwCount = num;
		pItem->BornFromMapid=frommapid;
		strcpy_s(pItem->bornfrom,sizeof(pItem->bornfrom)-1,UTG(bornfrom));
		strcpy_s(pItem->szMaker,sizeof(pItem->szMaker)-1, UTG(szmaker));
		pItem->borntime=(DWORD)time(NULL);
		FUNCTION_BEGIN;	
		pItem->nDura = pItemLoadBase->dwMaxDura;
		pItem->nMaxDura = pItemLoadBase->dwMaxDura;
		if(pItemLoadBase->btItemLimitTimeType==1)
		{
			pItem->dwExpireTime = (DWORD)time(NULL) + pItemLoadBase->dwItemLimitTime;
		}else
		{
			pItem->dwExpireTime = pItemLoadBase->dwItemLimitTime;
		}		
		if (pItem->btBornFrom != _CREATE_NPC_CHEST) {
			if (pItemLoadBase->dwType != ITEM_TYPE_GOLD) {
				pItem->i64ItemID = CItem::GenerateItemID();
			}
			else {
				pItem->i64ItemID = CItem::GenerateVirtualItemID();
			}
		}
		else {
			pItem->i64ItemID = CItem::GenerateVirtualItemID();
		}
	}
	return pItem;
 }


 bool SendSysMail(double dReceiveId,const char* szReceiveName,const char* szMailTitle,const char* szMailNotice, sol::table const& table){
	 FUNCTION_BEGIN;
	 if (szReceiveName == NULL || szMailTitle == NULL || szMailNotice == NULL ) return false;

	 BUFFER_CMD(stMailSendNewMailInner, newMail, stBasePacket::MAX_PACKET_SIZE);
	 newMail->MailDetail.i64ReceiverID = (__int64)dReceiveId;
	 CopyString(newMail->MailDetail.szReceiverName, UTG(szReceiveName));
	 CopyString(newMail->MailDetail.szTitle, UTG(szMailTitle));
	 CopyString(newMail->MailDetail.szNotice, UTG(szMailNotice));
	 newMail->MailDetail.btGoldType = 0;
	 newMail->MailDetail.dwGold = 0;

	 if (table.valid())
	 {
		 int nMaxMailItemSize = min(table.size(), 12);
		 for (int i = 0; i < nMaxMailItemSize; i++) {
			 sol::object obj = table[i + 1];
			 if (obj.get_type() == sol::type::userdata) {
				 stItem* item = obj.as<stItem*>();
				 if (item)
				 {
					 newMail->MailDetail.ItemArr.push_back(*item, __FUNC_LINE__);
				 }
			 }
		 }
		 for (int i = 0; i < table.size(); i++) {
			 sol::object obj = table[i + 1];
			 if (obj.get_type() == sol::type::userdata) {
				 stItem* pItem = obj.as<stItem*>();
				 if (pItem) {
					 CUserEngine::getMe().PushMailedItem(pItem);
				 }
			 }
		 }
	 }

	 CUserEngine::getMe().SendMailMsg2Super(newMail, sizeof(*newMail) + newMail->MailDetail.ItemArr.getarraysize(), CUserEngine::getMe().isCrossSvr() ? dReceiveId : 0xFFFFFFFFFFFFFFFF);
 }

 void SendToGlobalStyledTipMsg(CPlayerObj* pPlayer,const char* szTip,BYTE btStyle){
	 if (pPlayer) {
		 stRelationTipMsg tipcmd;
		 tipcmd.btType = btStyle;
		 strcpy_s(tipcmd.szTip,sizeof(tipcmd.szTip)-1,szTip);
		 SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv,pPlayer->m_i64UserOnlyID,&tipcmd,sizeof(tipcmd));
	 }
 }

 const char* getRelationName(stRelation* pRelation)
 {
	 return pRelation->szName;
 }

 void LuaGetFriendTable(CPlayerObj* player, BYTE btType, sol::table table)
 {
	 if (!table.valid()) { return; }
	 lua_State* L = table.lua_state();
	 if (L) {
		 sol::state_view lua(L);
		 int table_index = 1;
		 for (CRelationList::iter it = player->m_xFriendList.begin(); it != player->m_xFriendList.end(); ++it) {
			 sol::table subtable = lua.create_table();
			 if (subtable.valid()) {
				 stRelation* pRelation = it->second;
				 if (pRelation) {
					 subtable["onlyid"] = (double)pRelation->i64OnlyId;
					 subtable["name"] = pRelation->szName;
					 subtable["kill"] = pRelation->nKill;
					 subtable["addtime"] = pRelation->tAddTime;
					 subtable["death"] = pRelation->nDeath;
					 subtable["relationdegree"] = pRelation->nRelationDegree;
					 table[table_index++] = subtable;
				 }
			 }
		 }
	 }
 }
 void getfriendlist2table(CRelationList* friendlist, BYTE btType, sol::table& table)
 {
	 if (!table.valid()) { return; }
	 lua_State* L = table.lua_state();
	 if (L) {
		 sol::state_view lua(L);
		 int table_index = 1;
		 for (CRelationList::iter it = friendlist->begin(); it != friendlist->end(); ++it) {
			 sol::table subtable = lua.create_table();
			 if (table.valid()) {
				 stRelation* pRelation = it->second;
				 if (pRelation) {
					 subtable["onlyid"] = (double)pRelation->i64OnlyId;
					 subtable["name"] = pRelation->szName;
					 subtable["kill"] = pRelation->nKill;
					 subtable["addtime"] = pRelation->tAddTime;
					 subtable["death"] = pRelation->nDeath;
					 subtable["relationdegree"] = pRelation->nRelationDegree;
					 table[table_index++] = subtable;
				 }
			 }
		 }
	 }
 }

 void LuaGuildCrossDivide(int nDay,bool boCreateCloneMap)		//开启第1天的跨服行会战
 {
	 stGuildCrossDivide retcmd;
	 retcmd.nDay = nDay;
	 retcmd.boCreateCloneMap = boCreateCloneMap;
	  CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGuildCrossDivide));
 }

 void LuaSetGuildWin(DWORD dwGuildId,int nDay)
 {
	 stSetWinGuild retcmd;
	 retcmd.dwGuildId = dwGuildId;
	 retcmd.nDay = nDay;
	  CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stSetWinGuild));
 }

 void LuaClearGuildCrossBattle(int nType)		//清理跨服战行会信息,1 清理当天报名数据，2清理全部数据
 {
	 stClearGuildWait retcmd;
	 retcmd.nType = nType;
	  CUserEngine::getMe().SendWrapperMsg2GGS(&retcmd,sizeof(retcmd));
 }

 void LuaCrossBattleGuildInfo(CPlayerObj* pPlayer,const char *szScript)
 {
	stGuildCrossBattleInfo retcmd;
	 retcmd.i64OnlyId = pPlayer->m_i64UserOnlyID;
	 strcpy_s(retcmd.szScript,sizeof(retcmd.szScript)-1,szScript);
	 CUserEngine::getMe().SendWrapperMsg2GGS(&retcmd,sizeof(retcmd));
 }

 void LuaAddItemDefenseDropRecord(CPlayerObj* player,const char* szContent)			//增加防爆记录
 {
	 BUFFER_CMD(stAddItemDefendDropRecord,retcmd,stBasePacket::MAX_PACKET_SIZE);
	 retcmd->i64OnlyId = player->m_i64UserOnlyID;
	 strcpy_s(retcmd->szName,sizeof(retcmd->szName)-1,player->getName());
	 retcmd->Record.push_str(szContent);
	 CUserEngine::getMe().SendMsg2SuperSvr(retcmd,sizeof(stAddItemDefendDropRecord)+retcmd->Record.getarraysize()+1);
 }

 void LuaSendBeginDonateEquip(CPlayerObj * player,const char* itemid)
 {
	 stBeginDonateEquip cmd;
	 cmd.dwStoreId = 0;
	 cmd.i64ItemId = CItem::strToi642(itemid);
	 CItem *item  = player->m_Packet.FindItemInBag(cmd.i64ItemId);
	 if (item) {
		 SENDMSG2BLOBALBYONLYID(stSendGuildPackageMsgSuperSrv,player->m_i64UserOnlyID,&cmd,sizeof(cmd));
	 }
 }

 void LuaAddItem2GuildPackage(CPlayerObj* player, DWORD dwBaseId, DWORD dwCount, const char* szRetFunc)
 {
	 stGuildAddItem cmd;
	 cmd.stEquip.i64ItemID = CItem::GenerateItemID();
	 cmd.stEquip.dwBaseID = dwBaseId;
	 cmd.stEquip.dwCount = dwCount;
	 if (szRetFunc && szRetFunc[0]){
		 strcpy_s(cmd.szRetFunc, sizeof(cmd.szRetFunc), szRetFunc);
	 }
	 if (player) {
		 SENDMSG2BLOBALBYONLYID(stSendGuildPackageMsgSuperSrv,player->m_i64UserOnlyID,&cmd,sizeof(cmd));
	 }
 }

 void LuaAddGuildGold(CPlayerObj* player,int nAddGold,const char* callbackfun)
 {
	stGuildAddGold cmd;
	cmd.nAddGold = nAddGold;
	strcpy_s(cmd.szLuaFunc,sizeof(cmd.szLuaFunc),callbackfun);
	if (player) {
		SENDMSG2BLOBALBYONLYID(stSendGuildPackageMsgSuperSrv,player->m_i64UserOnlyID,&cmd,sizeof(cmd));
	}
 }

 void LuaOnLineFunc(double onlyid,const char* name,const char* func)
 {
	 if (func) {
		 BUFFER_CMD(stCallUserOnlineFunc,stCmd,stBasePacket::MAX_PACKET_SIZE);
		 stCmd->i64OnlyId = (__int64)onlyid;
		 if (name && name[0]) {
			 strcpy_s(stCmd->szName,sizeof(stCmd->szName)-1,name);
		 }
		 stCmd->func.push_str(func);
		 CUserEngine::getMe().SendMsg2GlobalSvr(stCmd,sizeof(stCallUserOnlineFunc)+stCmd->func.getarraysize()+1);
	 }
 }
 
 void CrossCheckConnect(WORD wZoneID, WORD wSvrID, WORD wTradeId, const char* szScript){
	BUFFER_CMD(stNotifyExecScript, cmd, stBasePacket::MAX_PACKET_SIZE);
	cmd->szScript.push_str(szScript);
	CUserEngine::getMe().BroadcastGameSvr(&cmd,sizeof(*cmd)+cmd->szScript.getarraysize()+1,wSvrID,0,wZoneID,wTradeId);
 }

 // 跨服执行 脚本
 void Cross_DoPack(DWORD dwTradeId, WORD trueZoneID, WORD srvID, int ntype, const char* szstr, DWORD dwLength) {
	 BUFFER_CMD(stCrossPack, cmd, stBasePacket::MAX_PACKET_SIZE);
	 cmd->ntype = ntype;
	 cmd->dwLength = dwLength;
	 cmd->szPack.push_packetstr(szstr, dwLength);
	 CUserEngine::getMe().BroadcastGameSvr(cmd, sizeof(*cmd) + cmd->szPack.getarraysize(), srvID, true, trueZoneID, dwTradeId);
 }

// 跨服执行 脚本
 void Cross_DoScript(WORD trueZoneID, WORD srvID, const char* szScript) {
	 BUFFER_CMD(stNotifyExecScript, cmd, stBasePacket::MAX_PACKET_SIZE);
	 cmd->szScript.push_str(szScript);
	 CUserEngine::getMe().SendCrossMsg(trueZoneID, srvID, cmd, sizeof(*cmd) + cmd->szScript.getarraysize() + 1);
 }

//跨服发送脚本给玩家
void SendCmd2CrossUser(double onlyid,const char* cmdFunc)
{
	if (!isCrossSvr()) 
		return;

	if (cmdFunc == NULL || cmdFunc[0] == 0) 
		return;

	BUFFER_CMD(stSendGameScript,scriptcmd,stBasePacket::MAX_PACKET_SIZE);
	int cmdLen = strlen(cmdFunc);
	int maxCmdLen = stBasePacket::MAX_PACKET_SIZE - sizeof(stSendGameScript) - 1024;
	if (cmdLen > maxCmdLen)
	{
		g_logger.error("SendCmd2CrossUser 命令长度 %d 超出 %d",cmdLen,maxCmdLen);
		return;
	}

	scriptcmd->i64OnlyId = onlyid;
	scriptcmd->szFunc.push_str(cmdFunc,__FUNC_LINE__);
	CUserEngine::getMe().SendWrapperMsg2GGS(scriptcmd,sizeof(stSendGameScript)+scriptcmd->szFunc.getarraysize()+1);
}

//发送跨服脚本到指定服务器 
void SendCmd2CrossSvr(DWORD dwGameSvrId,const char* cmdFunc)
{
	if (cmdFunc == NULL || cmdFunc[0] == 0) 
		return;

	BUFFER_CMD(stSendScriptCmd2CrossSvr,scrpitCmd,stBasePacket::MAX_PACKET_SIZE);
	int cmdLen = strlen(cmdFunc);
	int cmdMaxLen = stBasePacket::MAX_PACKET_SIZE - sizeof(stSendScriptCmd2CrossSvr) - 1024; 
	if (cmdLen > cmdMaxLen){
		g_logger.error("SendCmd2CrossSvr 命令长度 %d 超出 %d",cmdLen,cmdMaxLen);
		return;
	}

	scrpitCmd->dwSvrId = dwGameSvrId;
	scrpitCmd->szScriptCmd.push_str(cmdFunc,__FUNC_LINE__);
	CUserEngine::getMe().SendWrapperMsg2GGS(scrpitCmd,sizeof(stSendScriptCmd2CrossSvr)+scrpitCmd->szScriptCmd.getarraysize()+1);
}

void CheckOfflineTime(double onlyid) {
	 stOfflineTime retcmd;
	 retcmd.i64OnlyId = onlyid;
	 CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stOfflineTime));
}
void LuaGuildExpand(CPlayerObj* pPlayer){
	if(pPlayer){
		stGloalGuildExpand retcmd;
		retcmd.i64OnlyId = pPlayer->m_i64UserOnlyID;
		retcmd.dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
		CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGloalGuildExpand));
	}
}

stCdKeyCount g_stCdKeyCount;
stCdKeyCount* getCdKeyCount(){
	return &g_stCdKeyCount;
}

void CdKeySendReward(double onlyid, const char* szName, int nType, stCdKeyCount* stcdkey){
	if(szName && stcdkey){
		stSetNewPlayerCardScore retcmd;
		retcmd.i64OnlyId = onlyid;
		strcpy_s( retcmd.szName, sizeof(retcmd.szName)-1, szName);
		retcmd.nType = nType;
		retcmd.nZoneId =  GameService::getMe().m_nZoneid;

		retcmd.stCount.nDayMaxCnt = stcdkey->nDayMaxCnt;
		retcmd.stCount.nDayDate = stcdkey->nDayDate;
		retcmd.stCount.nWeekMaxCnt = stcdkey->nWeekMaxCnt;
		retcmd.stCount.nWeekDate = stcdkey->nWeekDate;
		retcmd.stCount.nMonthMaxCnt = stcdkey->nMonthMaxCnt;
		retcmd.stCount.nMonthDate = stcdkey->nMonthDate;
		retcmd.stCount.nMaxCnt = stcdkey->nMaxCnt;

		CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stSetNewPlayerCardScore));
	}
}

void CdKeyRewardCheck(double onlyid, int nType) {
	stGetNewPlayerCardScore retcmd;
	retcmd.i64OnlyId = onlyid;
	retcmd.nType = nType;
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd, sizeof(stGetNewPlayerCardScore));
}

void LuaGetWeiXinCardId(CPlayerObj* pPlayer, DWORD dwServerId, int nPid, int nAid, int nTid) {
	if (pPlayer) {
		stOpenApiWeiXinActivity retcmd;
		strcpy_s(retcmd.szRoleName, _MAX_NAME_LEN_ - 1, pPlayer->getName());
		retcmd.i64id = pPlayer->m_i64UserOnlyID;
		retcmd.nPid = nPid;
		retcmd.nAid = nAid;
		retcmd.nTid = nTid;
		CUserEngine::getMe().SendMsg2TencentApiSvr(&retcmd, sizeof(stOpenApiWeiXinActivity));
	}
}

void LuaSendGlobalMail(const char* szTitle, const char* szNotice) {
	if (szTitle && szNotice) {
		stSendGolbalMail retcmd;
		strcpy_s(retcmd.szTitle, sizeof(retcmd.szTitle) - 1, szTitle);
		strcpy_s(retcmd.szNotice, sizeof(retcmd.szNotice) - 1, szNotice);
		CUserEngine::getMe().SendMsg2SuperSvr(&retcmd, sizeof(stSendGolbalMail));
	}
}

WORD LuaGetGuildTerritory(DWORD dwGuild)
{
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuild);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.wTerritory;
	}
	return 0;
}

DWORD LuaGetGuildOccupyTime(DWORD dwGuild) { 
	std::map<DWORD, stGSALLGuild>::iterator it = CUserEngine::getMe().m_mGuilds.find(dwGuild);
	if (it != CUserEngine::getMe().m_mGuilds.end())
	{
		return it->second.dwOccupyTime;
	}
	return 0;
}

void LuaTerritoryGlobal(BYTE btType, DWORD dwGuild, double onlyid, DWORD dwMapid)
{
	stGlobalGuildTerritory retcmd;
	retcmd.btType = btType;
	retcmd.dwGuildId = dwGuild;
	retcmd.i64OnlyId = (__int64)onlyid;
	retcmd.dwMapid = dwMapid;
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd, sizeof(stGlobalGuildTerritory));
}

void LuaOreEnergy(int index, int value) {
	FUNCTION_BEGIN;
	CUserEngine::getMe().m_OreEnergy[index] = value;
}


stGSALLGuild* LuaGetGsGuildInfo(DWORD dwGuildId) {
	auto guild = CUserEngine::getMe().m_mGuilds.find(dwGuildId);
	if (guild != CUserEngine::getMe().m_mGuilds.end())
	{
		return &guild->second;
	}
	return nullptr;
}

void LuaSetAuctionPlayer(BYTE index, __int64 onlyid)
{
	stAuctionPlayer retcmd;
	retcmd.index = index;
	retcmd.onlyid = onlyid;
	CUserEngine::getMe().SendMsg2SuperSvr(&retcmd, sizeof(retcmd));
}

void LuaAuctionPlayerDividend(BYTE index, BYTE ccy, int num, const char* title, const char* content)
{
	stAuctionPlayerDividend retcmd;
	retcmd.index = index;
	retcmd.ccy = ccy;
	retcmd.num = num;
	strcpy_s(retcmd.szTitle, sizeof(retcmd.szTitle) - 1, UTF8ToGB2312(title));
	strcpy_s(retcmd.szContent, sizeof(retcmd.szContent) - 1, UTF8ToGB2312(content));
	CUserEngine::getMe().SendMsg2SuperSvr(&retcmd, sizeof(retcmd));
}

void LuaGetAuctionPlayerOnlyid(BYTE index, sol::table table)
{
	FUNCTION_BEGIN;
	if (!table.valid()) { return; }
	int tabidx = 1;
	for (auto onlyid : CUserEngine::getMe().m_aucPlayer[index]) {
		table[tabidx] = onlyid;
		++tabidx;
	}
}

void LuaClearAuctionPlayer() {
	FUNCTION_BEGIN;
	CUserEngine::getMe().m_aucPlayer.clear();
}

void CScriptSystem::BindOther(CLuaVM *luavm){
	sol::state_view lua(luavm->lua());

	RegisterDynamicMap<int,bool>(lua,"dynamic_int_bool");
	RegisterDynamicMap<std::string,int>(lua,"dynamic_string_int");
	RegisterDynamicMap<int,int>(lua,"dynamic_int_int");
	RegisterSetWrapper<std::string>(lua, "set_string");
	RegisterDynamicArray<2>(lua);
	RegisterDynamicMap<int64_t, DynamicArray<2>>(lua, "dynamic_i64_DArr2");
	RegisterDynamicArray<3>(lua);
	RegisterDynamicMap<int, DynamicArray<3>>(lua, "dynamic_int_DArr3");
	RegisterVectorOfArrays<int, 2>(lua, "Array2Int", "RecycleItemVector");

	REGISTER_LUA_FUNCTION("createforclonemap",&LuaCreateForCloneMap);
	REGISTER_LUA_FUNCTION("createforclonemapid",&LuaCreateForCloneMapId);
	REGISTER_LUA_FUNCTION("sendgmsvrmsg",&LuaSendGmManageMsg);
	REGISTER_LUA_FUNCTION("gettickcount",&LuaGetTickCount);
	REGISTER_LUA_FUNCTION("setloglevel",&LuaSetLogLevel);
	REGISTER_LUA_FUNCTION("i64tochar",&LuaI64ToChar);
	REGISTER_LUA_FUNCTION("createitem",&LuaCreateItem);
	REGISTER_LUA_FUNCTION("getmaileditem",&GetMailedItem);
	REGISTER_LUA_FUNCTION("saveitemnpdata",&SaveItemNpData);
	REGISTER_LUA_FUNCTION("sendsysmailwithitem",&SendSysMail);
	REGISTER_LUA_FUNCTION("clearmailitem",&ClearMailItem);
	REGISTER_LUA_FUNCTION("getmapinfoname",&LuaGetMapInfoName);
	REGISTER_LUA_FUNCTION("getonlinenum",&LuaGetOnlineNum);
	REGISTER_LUA_FUNCTION("dosupersvrlua",&lua_supersvr_doscript);
	REGISTER_LUA_FUNCTION("dogamesvrlua",&lua_gamesvr_doscript);
	REGISTER_LUA_FUNCTION("doluabyusername",&lua_user_doscript);
	REGISTER_LUA_FUNCTION("doluabyuserid",&lua_useronlyid_doscript);
	REGISTER_LUA_FUNCTION("doluaalluser", &lua_alluser_doscript);
	REGISTER_LUA_FUNCTION("doluamapuser", &lua_mapuser_doscript);
	REGISTER_LUA_FUNCTION("setglobalvar",&setGlobalVar);
	REGISTER_LUA_FUNCTION("getglobalvar",&getGlobalVar);
	REGISTER_LUA_FUNCTION("getlocalglobalvar",&getLocalGlobalVar);
	REGISTER_LUA_FUNCTION("removeglobalvar",&removeGlobalVar);
	REGISTER_LUA_FUNCTION("guildaddexp",&GuildAddExp);
	REGISTER_LUA_FUNCTION("luaguildeventpush", &LuaGuildEventPush);
	REGISTER_LUA_FUNCTION("luaguildeventsfetch", &LuaGuildEventsFetch);
	REGISTER_LUA_FUNCTION("guildaddfalg",&GuildAddFlag);
	REGISTER_LUA_FUNCTION("guildaddweekfieldboss",&GuildAddWeekFieldBoss);
	REGISTER_LUA_FUNCTION("guildaddweekguildboss",&GuildAddWeekGuildBoss);
	REGISTER_LUA_FUNCTION("guildcallmemberscript",&GuildCallMemberScript);
	REGISTER_LUA_FUNCTION("guildmembersfetch",&GuildMembersFetch);
	REGISTER_LUA_FUNCTION("luaguilddonate",&LuaGuildDonate);
	REGISTER_LUA_FUNCTION("luaguildlevelup", &LuaGuildLeveluUp);
	REGISTER_LUA_FUNCTION("luaguildclearcd", &LuaGuildClearCd);
	REGISTER_LUA_FUNCTION("luaguildquest", &LuaGuildQuest);
	REGISTER_LUA_FUNCTION("luaguildrefreshmall", &LuaGuildRefreshMall);
	REGISTER_LUA_FUNCTION("guildmastebyguildid",&GuildMasterInfo);
	REGISTER_LUA_FUNCTION("getguildnamebyid",&LuaGetGuildNameById);
	REGISTER_LUA_FUNCTION("getguildlevelbyid",&LuaGetGuildLevelById);
	REGISTER_LUA_FUNCTION("getcurguildplayercountbyid",&LuaGetCurGuildPlayerCountById);
	REGISTER_LUA_FUNCTION("getguildmasternamebyid", &LuaGetGuildMasterNameById);
	REGISTER_LUA_FUNCTION("getguildmasteraccountbyid", &LuaGetGuildMasterAccountById);
	REGISTER_LUA_FUNCTION("getguildmasterjobbyid", &LuaGetGuildMasterJobById);
	REGISTER_LUA_FUNCTION("getguildcreatetimebyid", &LuaGetGuildCreateTimeById);
	REGISTER_LUA_FUNCTION("getguildjoinneedlvlbyid", &LuaGetGuildJoinNeedLvlById);
	REGISTER_LUA_FUNCTION("getguildjoinneedzslvlbyid", &LuaGetGuildJoinNeedZsLvlById);
	REGISTER_LUA_FUNCTION("getguildautojoinbyid", &LuaGetGuildAutoJoinById);
	REGISTER_LUA_FUNCTION("getguildmaxplayercountbyid", &LuaGetGuildMaxPlayerCountById);
	REGISTER_LUA_FUNCTION("getguildbuilddegree", &LuaGetGuildBuildDegree);
	REGISTER_LUA_FUNCTION("getguildmapsvrid", &LuaGetGuildMapSvrId);
	REGISTER_LUA_FUNCTION("getguildemblemtab", &LuaGetGuildEmblemTab);
	REGISTER_LUA_FUNCTION("clearrank",&ClearRank);
	REGISTER_LUA_FUNCTION("updateworld", &UpdateWorld);
	REGISTER_LUA_FUNCTION("sendtoglobalstyledtipmsg",SendToGlobalStyledTipMsg);
	REGISTER_LUA_FUNCTION("sendgroupscriptmsg",&SendGroupScriptMsg);
	REGISTER_LUA_FUNCTION("sendfriendscriptmsg",&SendFriendScriptMsg);
	REGISTER_LUA_FUNCTION("getfriendtable",&LuaGetFriendTable);
	REGISTER_LUA_FUNCTION("addrelationdegree",&LuaAddRelationDegree);
	REGISTER_LUA_FUNCTION("resetrelationdegree",&LuaResetRelationDegree);
	REGISTER_LUA_FUNCTION("exploreplayer",&LuaExplorePlayer);
	REGISTER_LUA_FUNCTION("luaclearcrossrank",&luaclearcrossrank);
	REGISTER_LUA_FUNCTION("luadividecrossgroup",&luadividecrossgroup);
	REGISTER_LUA_FUNCTION("luaguildcrossdivide",&LuaGuildCrossDivide);
	REGISTER_LUA_FUNCTION("luasetguildwin",&LuaSetGuildWin);
	REGISTER_LUA_FUNCTION("luacrossbattleguildinfo",&LuaCrossBattleGuildInfo);
	REGISTER_LUA_FUNCTION("luaclearguildcrossbattle",&LuaClearGuildCrossBattle);
	REGISTER_LUA_FUNCTION("luaadditemdefensedroprecord",&LuaAddItemDefenseDropRecord);
	REGISTER_LUA_FUNCTION("LuaSendBeginDonateEquip",&LuaSendBeginDonateEquip);
	REGISTER_LUA_FUNCTION("luaaddguildgold",&LuaAddGuildGold);
	REGISTER_LUA_FUNCTION("luaonlinefunc",&LuaOnLineFunc);
	REGISTER_LUA_FUNCTION("crosscheckconnect",&CrossCheckConnect);
	REGISTER_LUA_FUNCTION("cross_dopack",&Cross_DoPack);
	REGISTER_LUA_FUNCTION("cross_doscript",&Cross_DoScript);
	REGISTER_LUA_FUNCTION("sendcmd2crossuser",&SendCmd2CrossUser);
	REGISTER_LUA_FUNCTION("sendcmd2crosssvr",&SendCmd2CrossSvr);
	REGISTER_LUA_FUNCTION("additem2guildpackage",&LuaAddItem2GuildPackage);
	REGISTER_LUA_FUNCTION("reloadrank",&reloadrank);
	REGISTER_LUA_FUNCTION("resyncrank",&resyncrank);
	REGISTER_LUA_FUNCTION("checkofflinetime",&CheckOfflineTime);
	REGISTER_LUA_FUNCTION("luaguildexpand", &LuaGuildExpand);
	REGISTER_LUA_FUNCTION("cdkeyrewardcheck", &CdKeyRewardCheck);
	REGISTER_LUA_FUNCTION("cdkeysendreward", &CdKeySendReward);
	REGISTER_LUA_FUNCTION("getcdkeycount", &getCdKeyCount);
	REGISTER_LUA_FUNCTION("getweixincardid", &LuaGetWeiXinCardId);
	REGISTER_LUA_FUNCTION("sendglobalmail", &LuaSendGlobalMail);
	REGISTER_LUA_FUNCTION("getguildterritory", &LuaGetGuildTerritory);
	REGISTER_LUA_FUNCTION("getguildoccupytime", &LuaGetGuildOccupyTime);
	REGISTER_LUA_FUNCTION("territoryglobal", &LuaTerritoryGlobal);
	REGISTER_LUA_FUNCTION("luaoreenergypush", &LuaOreEnergy);
	REGISTER_LUA_FUNCTION("getgsguildbyid", &LuaGetGsGuildInfo);
	REGISTER_LUA_FUNCTION("setauctionplayer", &LuaSetAuctionPlayer);
	REGISTER_LUA_FUNCTION("auctionplayerdividend", &LuaAuctionPlayerDividend);
	REGISTER_LUA_FUNCTION("getauctionplayeronlyid", &LuaGetAuctionPlayerOnlyid);
	REGISTER_LUA_FUNCTION("clearauctionplayer", &LuaClearAuctionPlayer);
	registerClass<ConfigMgr>(lua, "ConfigMgr",
		"LoadLuaConfig", &ConfigMgr::LoadLuaConfig,
		"LoadAttackSpeedConfig", &ConfigMgr::LoadAttackSpeedConfig,
		"LoadMoveSpeedConfig", &ConfigMgr::LoadMoveSpeedConfig,
		"LoadReleaseSpeedConfig", &ConfigMgr::LoadReleaseSpeedConfig,
		"LoadBoxerAttConfig", &ConfigMgr::LoadWeaponBoxerAttConfig,
		"LoadMageAttConfig", &ConfigMgr::LoadWeaponMageAttConfig,
		"LoadSwordAttConfig", &ConfigMgr::LoadWeaponSwordAttConfig,
		"LoadGunAttConfig", &ConfigMgr::LoadWeaponGunAttConfig,
		"LoadArmorAttConfig", &ConfigMgr::LoadArmorAttConfig,
		"LoadGroupExpAddConfig", &ConfigMgr::LoadGroupExpAddConfig,
		"LoadDebugConfig", &ConfigMgr::LoadDebugConfig,
		"LoadProtectLvConfig", &ConfigMgr::LoadProtectionlvConfig,
		"LoadGuardAtkConfig", &ConfigMgr::LoadGuardAttackConfig,
		"LoadGuardDefConfig", &ConfigMgr::LoadGuardDefenseConfig,
		"LoadGuardSupConfig", &ConfigMgr::LoadGuardSupportConfig
	);
	registerClass<CUserEngine>(lua, "CUserEngine",
		"GetSavedSharedData", &CUserEngine::GetSavedSharedData,
		"SetSavedSharedData", &CUserEngine::SetSavedSharedData,
		"GetAuc", &CUserEngine::GetAuc,

		"QuestInfo", &CUserEngine::m_globalquestinfo,
		"openDay", &CUserEngine::m_openDay,
		"isRebooting", &CUserEngine::m_isRebooting,
		"abilityKeys", &CUserEngine::m_abilityKeys,
		"abilityTimeKeys", &CUserEngine::m_abilityTimeKeys,
		"specialAbilityKeys", &CUserEngine::m_specialAbilityKeys,
		"updateWorld", &CUserEngine::m_updateWorld,
		"worldDayCall", &CUserEngine::m_worldDayCall,
		"crossVersion", &CUserEngine::m_crossVersion,
		"world", &CUserEngine::m_world,
		"SkyLadder", &CUserEngine::m_SkyLadder,
		"actFirstOpen", &CUserEngine::m_actFirstOpen,
		"actFirstClose", &CUserEngine::m_actFirstClose,
		"globalVars", &CUserEngine::m_globalVars,
		"GuildFortress", &CUserEngine::m_GuildFortress,
		"bossRefreshTimes",& CUserEngine::m_bossRefreshTime
	);
	registerClass<stTempOutItem>(lua, "stTempOutItem",
		"item", &stTempOutItem::pItem,
		"num", &stTempOutItem::nNum
		);
	registerClass<stGSGuildInfo>(lua,"stGSGuildInfo",
		"guildid", &stGSGuildInfo::dwGuildId,
		"guildlevel", &stGSGuildInfo::dwGuildLevel,
		"guildpower", &stGSGuildInfo::dwPowerLevel,
		"guildrequest", &stGSGuildInfo::IsExistAskJoin,
		"intime", &stGSGuildInfo::tInTime
	);

	lua.new_usertype<CRelationList>("CRelationList",
		"findbyname", &CRelationList::FindByName,
		"findbyonlyid", &CRelationList::FindByDoubleOnlyId
	);
	lua.new_usertype<stRelation>("stRelation",
		"onlyid", sol::property(&stRelation::i64OnlyId),
		"name", sol::property([](const stRelation& self) {return self.szName; }),
		"addtime", sol::readonly(&stRelation::tAddTime),
		"kill", sol::readonly(&stRelation::nKill),
		"death", sol::readonly(&stRelation::nDeath),
		"relationdegree", sol::readonly(&stRelation::nRelationDegree)
	);
	lua.new_usertype<stCdKeyCount>("stCdKeyCount",
		"daycnt", &stCdKeyCount::nDayCnt,
		"daymaxcnt", &stCdKeyCount::nDayMaxCnt,
		"daydate", &stCdKeyCount::nDayDate,
		"weekcnt", &stCdKeyCount::nWeekCnt,
		"weekmaxcnt", &stCdKeyCount::nWeekMaxCnt,
		"weekdate", &stCdKeyCount::nWeekDate,
		"monthcnt", &stCdKeyCount::nMonthCnt,
		"monthmaxcnt", &stCdKeyCount::nMonthMaxCnt,
		"monthdate", &stCdKeyCount::nMonthDate,
		"nowcnt", &stCdKeyCount::nNowCnt,
		"maxcnt", &stCdKeyCount::nMaxCnt
	);

	registerClass<stGSALLGuild>(lua,"stGSALLGuild",
		"getname", [](stGSALLGuild& self)->std::string {return GTU(self.szGuildName); },
		"lv", &stGSALLGuild::dwGuildLevel,
		"count", &stGSALLGuild::dwCurGuildPlayerCount,
		"maxcount", &stGSALLGuild::dwMaxPlayerCount,
		"getmastername", [](stGSALLGuild& self)->std::string {return GTU(self.szMasterName); },
		"masterfeature", &stGSALLGuild::stMasterFeature,
		"build", &stGSALLGuild::dwBuildDegree,
		"createtime", &stGSALLGuild::dwCreateTime,
		"joinlv", &stGSALLGuild::dwJoinNeedLvl,
		"joinzslv", &stGSALLGuild::dwJoinNeedZsLvl,
		"autojoin", &stGSALLGuild::AutoJoin,
		"emblemid", &stGSALLGuild::dwEmblemId,
		"fortressauto", &stGSALLGuild::btFortressAuto,
		"fortresssignup", &stGSALLGuild::btFortressSignUp,
		"mapsvrid", &stGSALLGuild::wMapSvrId,
		"mapunionid", &stGSALLGuild::dwMapUnionId
	);
}