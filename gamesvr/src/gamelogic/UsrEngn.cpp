#include "UsrEngn.h"
#include "LocalDB.h"
#include "timeMonitor.h"
#include "../gamesvr.h"
#include "../dbsvrGameConnecter.h"
#include "Chat.h"
#include "NpcTrade.h"
#include "MagicRange.h"
#include "GlobalVarSession.h"
#include "cmd/TencentApi_cmd.h"
#include "JsonConfig.h"
#include "JsonStructEx.h"

extern fnSaveGMOrder g_SaveGMOrder;

CUserEngine::CUserEngine() : m_shareData("ServerShareData" + std::to_string(GameService::getMe().m_nTrueZoneid)),
                             m_lasthintshutdowntime(0)
{
	FUNCTION_BEGIN;
	m_dwThisGameSvrVersion = strtotime(__BUILD_DATE_TIME__);
	m_boStartup = false;
	m_globaltimer.m_mainlist = &m_globalquestlist;
	m_processloadokplayertick = 0;
	m_gLogLevel = 10;
	m_boIsShutDown = false;
	m_shutdowntime = 0;
	m_mKeepHpSet.clear();
	m_mKeepHpSet[1] = 0x1;
	m_mKeepHpSet[11] = 0x2;
	m_mKeepHpSet[16] = 0x4;
	m_mGuilds.clear();
	m_cListV.resize(Rank_Max_Count);
	m_cGGListV.resize(Rank_Max_Count);
	m_i64LevelMaxPlayer = 0;
	m_dwFPS = 0;
	if (m_FBmapid.size() <= 0)
	{
		for (DWORD i = 1; i < _MAX_MAPCLONEID_; i++)
		{
			m_FBmapid.push_back(i);
		}
	}
	m_CheckMinSec = 100;
	RMBUnusual = false;
	for (int i = 0; i < 12; i++)
	{
		stItem* item = CLD_DEBUG_NEW stItem;
		m_mailedstItems.push_back(item);
	}
	m_nNengLimit = false;
	m_nMiningLimit = 0;
	m_aucPlayer.clear();
}

CUserEngine::~CUserEngine(){

}

CPlayerObj* CUserEngine::AddLoadOkPlayer(CPlayerObj* PlayObject,bool isselok){
	FUNCTION_BEGIN;
	if (PlayObject){ 
		AILOCKT(m_loadokplayers);
		if ( IsPlaying(PlayObject->getName())==NULL && m_loadokplayers.addValue(PlayObject) ){
			if (!PlayObject->m_boIsSelOk){ PlayObject->m_boIsSelOk=isselok; }
			g_logger.debug("%s [%s]正确添加到加载成功玩家列表!%d",PlayObject->getName(),GameService::getMe().m_svrcfg.szHomeMap,(BYTE)PlayObject->m_boIsSelOk);
			PlayObject->m_addloadoktime=time(NULL);

			if (isCrossSvr()) {
				stLoadAccountDataCmd loadcmd;
				loadcmd.checknum = 0;
				loadcmd.tmpid = PlayObject->m_pGateUser->m_tmpid;
				strcpy_s(loadcmd.szAccount, sizeof(loadcmd.szAccount) - 1, PlayObject->m_pGateUser->m_szAccount);
				strcpy_s(loadcmd.szName, sizeof(loadcmd.szName) - 1, PlayObject->getName());
				loadcmd.nCrossZoneId = GameService::getMe().m_nTrueZoneid;
				loadcmd.wCrossTradeid = GameService::getMe().m_nTradeid;
				BroadcastGameSvr(&loadcmd, sizeof(loadcmd), PlayObject->m_dwSrcGameSvrIdType, true, PlayObject->m_dwSrcZoneId, PlayObject->m_wSrcTrade);
			}else{
				if (PlayObject->m_pGateUser && PlayObject->m_pGateUser->m_OwnerLoginSvr && PlayObject->m_pGateUser->m_OwnerLoginSvr->IsConnected()) {
					stLoadAccountDataCmd loadcmd;
					loadcmd.checknum = 0;
					loadcmd.tmpid = PlayObject->m_pGateUser->m_tmpid;
					strcpy_s(loadcmd.szAccount, sizeof(loadcmd.szAccount) - 1, PlayObject->m_pGateUser->m_szAccount);
					strcpy_s(loadcmd.szName, sizeof(loadcmd.szName) - 1, PlayObject->getName());
					loadcmd.nCrossZoneId = 0;
					loadcmd.wCrossTradeid = 0;
					PlayObject->m_pGateUser->m_OwnerLoginSvr->sendcmd(&loadcmd, sizeof(loadcmd));
				}
			}
			return PlayObject;
		}else{
			g_logger.debug("%s [%s]无法正确添加到玩家列表!",PlayObject->getName(),GameService::getMe().m_svrcfg.szHomeMap);
			if (PlayObject->m_pTmpGamedata!=NULL){
				__mt_char_dealloc(PlayObject->m_pTmpGamedata);
				PlayObject->m_pTmpGamedata=NULL;
			}
			SAFE_DELETE(PlayObject);
			return NULL;
		}
	}
	return NULL;
}

CPlayerObj* CUserEngine::MakeNewHuman(CGameGateWayUser* pGateUser,stLoadPlayerData* pgamedata,bool boIsChangeSvr){
	FUNCTION_BEGIN;
	CPlayerObj* nullPlayObject=CLD_DEBUG_NEW CPlayerObj(pgamedata->x, pgamedata->y, pGateUser,pgamedata->szName);
	pgamedata->btgmlvl=0;
	if (pgamedata->nlevel==0){
		pgamedata->nlevel=1;
		nullPlayObject->m_boIsNewHum=true;
		if (pgamedata->whomamapid==0){
			pgamedata->dwmapid=0;
			pgamedata->wclonemapid=0;
			pgamedata->whomeclonemapid=0;
			
		}

#define _login_fmt_str_		",\'%s\',\'%s\',%I64d,%d,\'%s\',\'%s\',%d,%d,%d,%d,'%s',%d,%d,%d"	
		GameService::getMe().Send2LogSvr(_SERVERLOG_USERLOGIN_,0,0,nullPlayObject,"\'createnewplayer\'" _login_fmt_str_,
			pGateUser->m_szAccount,
			nullPlayObject->getName(),
			pgamedata->i64UserOnlyId,
			pgamedata->dwPlayerOnlineTime,
			timetostr(pgamedata->dwPlayerCreateTime),
			timetostr(pgamedata->tLoginOutTime),
			pgamedata->nlevel,
			pgamedata->siFeature.job,
			pgamedata->siFeature.sex,
			0,
			pGateUser?inet_ntoa(pGateUser->clientip):"",
			0,
			0,
			0,
			0);

		GameService::getMe().Send2LogSvr(_SERVERLOG_LEVELUP_,0,0,nullPlayObject,"'%s','%s','%s',%I64d,%d,%d,%I64u,'%s'",
			"levelup",
			pGateUser->m_szAccount,
			nullPlayObject->getName(),
			pgamedata->i64UserOnlyId,
			0,
			1,
			(__int64)0,
			"new");

	}
	int nsavedatasize=pgamedata->getSaveDataSize();
	stLoadPlayerData* pgamedatbuffer= (stLoadPlayerData*)__mt_char_alloc( nsavedatasize+32 );
	if (pgamedatbuffer){
		CopyMemory(pgamedatbuffer,pgamedata,nsavedatasize);
		nullPlayObject->m_pTmpGamedata=pgamedatbuffer;
		nullPlayObject->m_i64UserOnlyID = pgamedata->i64UserOnlyId;
		return nullPlayObject;
	}
	SAFE_DELETE(nullPlayObject);
	return NULL;
}

void CUserEngine::LoadGM(CPlayerObj* PlayObject, stLoadPlayerData* pgamedata)
{
	if (GameService::getMe().m_btAllisGm > 0 && PlayObject->m_btGmLvl <= 0) {
		PlayObject->m_btGmLvl = GameService::getMe().m_btAllisGm;
	}
	if (PlayObject->m_btGmLvl > 0 && GameService::getMe().m_btGmOnlineWDYS != 0) {
		//PlayObject->m_nViewRangeLvl=0x7F;
		PlayObject->m_boGmHide = true;
		PlayObject->m_btWudi = 1;
	}
}


bool CUserEngine::InitNewHuman(CPlayerObj* PlayObject){
	CGameMap* pMap=NULL;
	stLoadPlayerData* pgamedata=PlayObject->m_pTmpGamedata;
	if (!pgamedata){ return false; }
	PlayObject->LoadHumanBaseData(pgamedata);
	LoadGM(PlayObject, pgamedata);
	if (!PlayObject->LoadMap(pgamedata))
	{
		g_logger.error("%s [%d : %d : %d]无法找到地图!", pgamedata->szName, pgamedata->dwmapid, 0, pgamedata->wclonemapid);
		return false;
	}
	if (!PlayObject->LoadHumanData(pgamedata)) {
		g_logger.error("%s [%d : %d : %d]数据加载异常!", pgamedata->szName, pgamedata->dwmapid, 0, pgamedata->wclonemapid);
		return false;
	}
	// 必须等var变量加载以后才能获取值
	PlayObject->m_siAbility.dec(PlayObject->quest_vars_get_var_n("ap1"), PlayObject->quest_vars_get_var_n("ap2"), PlayObject->quest_vars_get_var_n("ap3"), PlayObject->quest_vars_get_var_n("ap4"), PlayObject->quest_vars_get_var_n("ap5"));
	CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("ClearBadDate",PlayObject);
	PlayObject->GetBaseProperty();
	PlayObject->SetAbilityFlag();
	PlayObject->DoChangeProperty(PlayObject->m_stAbility, true, __FUNC_LINE__);
	if (PlayObject->m_boIsNewHum) {
		PlayObject->m_LifeState = NOTDIE;
		PlayObject->m_btPkModel = PKMODEL_PEACEMODE;
		PlayObject->StatusValueChange(stCretStatusValueChange::hp, PlayObject->m_stAbility[AttrID::MaxHP], __FUNC_LINE__);
		PlayObject->StatusValueChange(stCretStatusValueChange::mp, PlayObject->m_stAbility[AttrID::MaxMP], __FUNC_LINE__);
		PlayObject->StatusValueChange(stCretStatusValueChange::pp, PlayObject->m_stAbility[AttrID::MaxPP], __FUNC_LINE__);
	}
	PlayObject->UpdateToSuperSvr();
	PlayObject->UpdateToGlobalSvr();
	PlayObject->m_btReadyState = _READYSTATE_SVR_READY_;
	return true;
}

bool CUserEngine::SendMsg2SuperSvr(void *pbuf,int nLen){
	FUNCTION_BEGIN;
	if (GameService::getMe().m_supergamesvrconnecter){
		return GameService::getMe().m_supergamesvrconnecter->sendcmd(pbuf,nLen);
	}
	return false;
}

#ifndef _SKIP_GAME_PROXY_
bool CUserEngine::SendMsg2GameSvrProxy(void *pbuf,int nLen){
	FUNCTION_BEGIN;
	if (GameService::getMe().m_pGameSvrProxyConnecter){
		return GameService::getMe().m_pGameSvrProxyConnecter->sendcmd(pbuf,nLen);
	}
	return false;
}
#endif

bool CUserEngine::BroadcastGameSvr(void *pbuf,int nLen,DWORD dwSvrIdType,bool exceptme,DWORD dwZoneid,WORD wTradeId){
	FUNCTION_BEGIN;
	BUFFER_CMD(stProxyMsg2Gamesvr,sendCmd,stBasePacket::MAX_PACKET_SIZE * 3);
	sendCmd->exceptme=exceptme;
	sendCmd->gamesvr_id_type=dwSvrIdType;
	sendCmd->i64SrcOnlyId=0;
	sendCmd->wDstTradeId=wTradeId;
	sendCmd->dwDstZoneID = (dwZoneid == 0xFFFFFFFF?GameService::getMe().m_nTrueZoneid:dwZoneid);
	//sendCmd->dwSrcSvr_id_type=GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value;
	sendCmd->msg.push_back((char*)pbuf,nLen,__FUNC_LINE__);
#ifndef _SKIP_GAME_PROXY_
	return CUserEngine::getMe().SendMsg2GameSvrProxy(sendCmd,sizeof(*sendCmd)+sendCmd->msg.getarraysize());
#else
	return CUserEngine::getMe().SendMsg2GlobalProxy(sendCmd,sizeof(*sendCmd)+sendCmd->msg.getarraysize());
#endif
}

#ifdef _SKIP_GAME_PROXY_
bool CUserEngine::SendMsg2GlobalProxy(void *pbuf, int nLen)
{
	FUNCTION_BEGIN;
	if (GameService::getMe().m_globalProxyConnecter){
		return GameService::getMe().m_globalProxyConnecter->sendcmd(pbuf,nLen);
	}
	return false;
}
#endif

bool CUserEngine::SendMsg2GmManageSvr(void* pbuf, int nLen){
	if (GameService::getMe().m_gmservermanageconnecter){
		return GameService::getMe().m_gmservermanageconnecter->sendcmd(pbuf,nLen);
	}
	return false;
}

void CUserEngine::SendMsg2AllUser(void* pbuf, int nLen)
{
	FUNCTION_BEGIN;
	BUFFER_CMD(stProxyBroadcastUserIndex, useridx, stBasePacket::MAX_PACKET_SIZE);
	int userCnt = 0;
	{
		AILOCKT(m_playerhash);
		for (auto it = m_playerhash.begin(), itnext = it; it != m_playerhash.end(); it = itnext) {
			++itnext;
			auto pUser = it->second;

			if (pUser)
			{
				useridx[userCnt].gatewayobj = (CGateWaySession*)pUser->m_pGateUser->GateWay();
				useridx[userCnt].userindex.wgindex = pUser->m_pGateUser->Gateidx();
				useridx[userCnt].userindex.sidx = pUser->m_pGateUser->ClientSocketIdx();
				useridx[userCnt].userindex.wsvridx = pUser->m_pGateUser->Svridx();
				userCnt++;
			}
		}
	}
	if (userCnt > 0) {
		GameService::getMe().BatchSendToGate(useridx, userCnt, pbuf, nLen);
	}
}

bool CUserEngine::SendProxyMsg2Gamesvr(void *pbuf,int nLen,DWORD dwSvrIdType,bool exceptme){
	FUNCTION_BEGIN;
	BUFFER_CMD(stProxyMsg2Gamesvr,sendCmd,stBasePacket::MAX_PACKET_SIZE);
	sendCmd->exceptme=exceptme;
	sendCmd->gamesvr_id_type=dwSvrIdType;
	sendCmd->i64SrcOnlyId=0;
	//sendCmd->dwSrcSvr_id_type=GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value;
	sendCmd->msg.push_back((char*)pbuf,nLen,__FUNC_LINE__);
	return CUserEngine::getMe().SendMsg2SuperSvr(sendCmd,sizeof(*sendCmd)+sendCmd->msg.getarraysize());
}

bool CUserEngine::SendProxyMsg2User(const char* szName,void *pbuf,int nLen){
	FUNCTION_BEGIN;
	BUFFER_CMD(stProxyMsg2User,sendCmd,stBasePacket::MAX_PACKET_SIZE);
	strcpy_s(sendCmd->szName,sizeof(sendCmd->szName)-1,szName);
	sendCmd->msg.push_back((char*)pbuf,nLen,__FUNC_LINE__);
	return CUserEngine::getMe().SendMsg2SuperSvr(sendCmd,sizeof(*sendCmd)+sendCmd->msg.getarraysize());
}

bool CUserEngine::SendProxyMsg2User(int64_t i64OnlyId,void *pbuf,int nLen){
	FUNCTION_BEGIN;
	BUFFER_CMD(stProxyMsg2User,sendCmd,stBasePacket::MAX_PACKET_SIZE);
	sendCmd->i64SrcOnlyId=i64OnlyId;
	sendCmd->msg.push_back((char*)pbuf,nLen,__FUNC_LINE__);
	return CUserEngine::getMe().SendMsg2SuperSvr(sendCmd,sizeof(*sendCmd)+sendCmd->msg.getarraysize());
}

bool CUserEngine::SendMsg2GlobalSvr(void *pbuf,int nLen){
	FUNCTION_BEGIN;
	if(GameService::getMe().m_globalsvrconnecter){
		return GameService::getMe().m_globalsvrconnecter->sendcmd(pbuf,nLen);
	}
	return false;
}

bool CUserEngine::SendWrapperMsg2GGS( void *pbuf,int nLen, int64_t i64Onlyid)
{
	FUNCTION_BEGIN;
	if (GameService::getMe().m_globalsvrconnecter) {
		BUFFER_CMD(stGSSend2GGSvr,retcmd,stBasePacket::MAX_PACKET_SIZE);
		retcmd->i64OnlyId = i64Onlyid;
		retcmd->msg.push_back((char*)pbuf,nLen,__FUNC_LINE__);
		return GameService::getMe().m_globalsvrconnecter->sendcmd(retcmd,sizeof(stGSSend2GGSvr)+retcmd->msg.getarraysize());
	}
	return false;
}

bool CUserEngine::SendMailMsg2Super(void* pbuf, int nLen, int64_t i64Onlyid)
{
	FUNCTION_BEGIN;
	if (CUserEngine::getMe().isCrossSvr()) {
		BUFFER_CMD(stGsToOtherSuper, retcmd, stBasePacket::MAX_PACKET_SIZE);
		retcmd->trueZoneID = GameService::getMe().m_nTrueZoneid;
		retcmd->i64Onlyid = i64Onlyid;
		BUFFER_CMD(stSendMailMsgSuperSrv, sendcmd, stBasePacket::MAX_PACKET_SIZE);
		sendcmd->i64SrcOnlyId = i64Onlyid;
		sendcmd->msg.push_back((char*)pbuf, nLen, __FUNC_LINE__);
		retcmd->msg.push_back((char*)sendcmd, nLen + sizeof(stSendMailMsgSuperSrv), __FUNC_LINE__);
		return SendWrapperMsg2GGS(retcmd, sizeof(*retcmd) + retcmd->msg.getarraysize(), i64Onlyid);
	}
	else {
		SENDMSG2SUPERBYONLYID(stSendMailMsgSuperSrv, i64Onlyid, pbuf, nLen);
	}
	return true;
}

bool CUserEngine::SendMsg2TencentApiSvr(void *pbuf,int nLen){
	FUNCTION_BEGIN;
	if(GameService::getMe().m_tencentapigamesvrconnecter){
		return GameService::getMe().m_tencentapigamesvrconnecter->sendcmd(pbuf,nLen);
	}
	return false;
}

void CUserEngine::addLuaErrorPlayer(CPlayerObj* pPlayer){
	if(pPlayer){
		do 
		{
			AILOCKT(m_vGetLuaErrorPlayer);
			for(int i=0;i<m_vGetLuaErrorPlayer.size();i++){
				if(m_vGetLuaErrorPlayer[i] == pPlayer->m_i64UserOnlyID){
					return ;
				}
			}
			m_vGetLuaErrorPlayer.push_back(pPlayer->m_i64UserOnlyID);
		} while (false);
		
	}
}

bool CUserEngine::SendProxyMsg2GlobalSvrUser(const char* szName,void *pbuf,int nLen)
{
	BUFFER_CMD(stProxyMsg2OneGlobalSrv,sendCmd,stBasePacket::MAX_PACKET_SIZE);
	strcpy_s(sendCmd->szName,sizeof(sendCmd->szName)-1,szName);
	sendCmd->msg.push_back((char*)pbuf,nLen,__FUNC_LINE__);
	return SendMsg2GlobalSvr(sendCmd,sizeof(*sendCmd)+sendCmd->msg.getarraysize());
}

bool CUserEngine::SendCrossMsg(WORD trueZoneID, WORD svrID, void *pBuf, int nLen){
	FUNCTION_BEGIN;
	BUFFER_CMD(stCrossCmd, cmd, stBasePacket::MAX_PACKET_SIZE);
	cmd->trueZoneID = trueZoneID;
	cmd->svrID = svrID;
	cmd->msg.push_back((char*)pBuf, nLen, __FUNC_LINE__);
	return SendWrapperMsg2GGS(cmd, sizeof(*cmd) + cmd->msg.getarraysize());
}

CPlayerObj* CUserEngine::IsPlaying(const char* szName){
	FUNCTION_BEGIN;
	CPlayerObj* player=NULL;
	do {
		AILOCKT(m_loadokplayers);
		player=m_loadokplayers.FindByName(szName);
		if(player!=NULL){	return player;	}

		player=m_playerhash.FindByName(szName);
		if(player!=NULL){ return player; }
	} while (false);
	return NULL;
}

DWORD CUserEngine::getMapCloneId(){
	FUNCTION_BEGIN;
	do 
	{
		AILOCKT(m_FBmapid);
		DWORD mapcloneid = m_FBmapid.front();
		m_FBmapid.pop_front();
		return mapcloneid;
	} while (false);
	
}

void CUserEngine::putMapCloneid(DWORD id){
	FUNCTION_BEGIN;
	if (id>0 && id<=_MAX_MAPCLONEID_){
		do 
		{
			AILOCKT(m_FBmapid);
			m_FBmapid.push_back(id);
		} while (false);
		
	}
}

bool CUserEngine::Startup(){
	FUNCTION_BEGIN;
	GameService* service=GameService::instance();
	service->reloadStrRes();
	m_processloadokplayertick=0;
	if(s_psystem_virtual_palyer==NULL){
		s_psystem_virtual_palyer=CLD_DEBUG_NEW CPlayerObj(0,0,NULL, (char*)"_!system_palyer_");
		if (!s_psystem_virtual_palyer){ return false; }
	}
	if (!m_shareData.IsValid())
	{
		g_logger.error("共享内存初始化失败!");
		return false;
	}
	if (!sJsonConfig.LoadAllData())
	{
		g_logger.error("加载Json配置文件失败!");
		return false;
	}
	do {
		GameService* gamesvr=GameService::instance();
		gamesvr->m_NotifMapListChangeCmd->svrmaps.clear();
		gamesvr->m_NotifMapListChangeCmdSize=sizeof(*gamesvr->m_NotifMapListChangeCmd);
		for (auto& [mapId,ptr] : sJsonConfig.m_mapConfig)
		{
			if (ptr->deleted == 1)
				continue;
			stSvrMapId svrMapId(ptr->dwMapID, GameService::getMe().m_lineId, 0);
			CGameMap* pMap = CLD_DEBUG_NEW CGameMap(ptr, svrMapId);
			if (pMap) {
				if (pMap->Init(ptr->dwMapFileID)) {
					if (!m_maphash.m_gameMaps.s_insert(svrMapId, pMap)) {
						SAFE_DELETE(pMap);
						g_logger.error("加载了重复的地图 %s(%d)", ptr->szName.c_str(), ptr->dwMapID);
					}
					else {
						pMap->GenNpc();
						stMapInfo mapinfo;
						mapinfo.wmapid = pMap->getMapId();
						mapinfo.w = pMap->m_nWidth;
						mapinfo.h = pMap->m_nHeight;
						mapinfo.btmapcountryid = 0;
						mapinfo.btmapsublineid = 0;

						AILOCKT(gamesvr->cfg_lock);
						gamesvr->m_NotifMapListChangeCmd->svrmaps.push_back(mapinfo, __FUNC_LINE__);
						if (s_psystem_virtual_palyer && !s_psystem_virtual_palyer->GetEnvir())
							s_psystem_virtual_palyer->SetEnvir(pMap);
					}
				}
			}
		}
		do {
			AILOCKT(gamesvr->cfg_lock);
			if (gamesvr->m_NotifMapListChangeCmd->svrmaps.size>0){
				gamesvr->m_NotifMapListChangeCmdSize=sizeof(*gamesvr->m_NotifMapListChangeCmd)+gamesvr->m_NotifMapListChangeCmd->svrmaps.getarraysize();
			}
		} while (false);
	} while (false);
	for (auto& [it, gateInfo] : sJsonConfig.m_mapGateData)
	{
		if (CGameMap* map = m_maphash.FindById(gateInfo.dwSrcMapId, 0, 0); map) {
			if (auto cell = map->GetMapCellInfo(gateInfo.wSx, gateInfo.wSy); cell) {
				cell->is_gate = true;
			}
		}
	}
	CBUFFManager::initBuffFun();
	if (!CChat::initgmcmdfunc()){g_logger.error("GM命令 没有加载成功!");return false;}
	if (!CMagicRangeDefine::getMe().init()){g_logger.error("魔法范围MagicRange 没有加载成功!");return false;}

	if(!m_scriptsystem.InitScript(GameService::getMe().m_szQuestScriptPath,eScript_init)){
		return false;
	}
	m_boStartup=true;
	return true;
}

bool CUserEngine::Cleanup(){
	FUNCTION_BEGIN;
	do {
		do {
			AILOCKT(m_syncmsglist);
			m_runsyscmsglist.swap(m_syncmsglist);
		} while (false);

		std::CSyncList< stQueueMsg* >::iterator it;
		stQueueMsg* pmsg=NULL;
		for (it= m_runsyscmsglist.begin();it!= m_runsyscmsglist.end();it++){
			pmsg=(*it);
			if (pmsg){	FreePacketBuffer(pmsg);	}
		}
		m_runsyscmsglist.clear();
	} while (false);
	m_maphash.ClearCloneMap();
	m_maphash.s_ForEach([](CGameMap*& map)
		{
			CN_DEL(map);
		});
	m_maphash.s_clear();
 
	if(!m_scriptsystem.InitScript(GameService::getMe().m_szQuestScriptPath,eScript_uninit)){
		return false;
	}
	m_boStartup=false;
	SAFE_DELETE(s_psystem_virtual_palyer);
	return true;
}

bool CUserEngine::RemoveCreature(CCreature* pCret){//TODO
	FUNCTION_BEGIN;
	if (!pCret) return false;
	if (pCret->getSwitchSvrInfo()){ pCret->getSwitchSvrInfo()->pCret=NULL; }
	switch (pCret->GetType())
	{
	case CRET_PLAYER:
		{
			if (CPlayerObj* player = dynamic_cast<CPlayerObj*>(pCret) ){
				do {
					AILOCKT(m_loadokplayers);
					m_loadokplayers.removeValue(player);
					m_playerhash.removeValue(player);
				} while (false);
			}

		}
		break;
	default:
		{

		}
		break;
	}
	return true;
}

bool CUserEngine::ProcessSyncMsgs(){
	static ULONGLONG s_dwSyncMsgTick=0;
	if ((::GetTickCount64()-s_dwSyncMsgTick)>50){
		s_dwSyncMsgTick=::GetTickCount64();
		//////////////////////////////////////////////////////////////////////////
		if (m_scriptsystem.m_LuaVM){
			std::CSyncList< stRmbAdd >::iterator it;
			bool bohavermb=false;
			stRmbAdd rmbadd;
			do {
				bohavermb=false;
				AILOCKT(m_rmbaddlist);
				it=m_rmbaddlist.begin();
				if (it!=m_rmbaddlist.end()){
					rmbadd=(*it);
					bohavermb=true;
					m_rmbaddlist.erase(it);
				}
			} while (false);
			while (bohavermb){
				//m_scriptsystem.m_LuaVM->VCall_LuaStr("RmbAdd",rmbadd.szAccount,rmbadd.Point,rmbadd.Order_No,rmbadd.szName,rmbadd.GameType, rmbadd.rechargetype);
				//m_scriptsystem.m_LuaVM->VCall_LuaStr("PreRmbAdd", rmbadd.szAccount);
				m_scriptsystem.m_LuaVM->VCall_LuaStr("RmbAdd", rmbadd.szAccount, rmbadd.Order_No, rmbadd.szName, vformat("%d+%d+%d", rmbadd.Point, rmbadd.GameType, rmbadd.rechargetype), true);
				do {
					bohavermb=false;
					AILOCKT(m_rmbaddlist);
					it=m_rmbaddlist.begin();
					if (it!=m_rmbaddlist.end()){
						rmbadd=(*it);
						bohavermb=true;
						m_rmbaddlist.erase(it);
					}
				} while (false);
			}
		}
		//////////////////////////////////////////////////////////////////////////
		std::CSyncList< stQueueMsg* >::iterator it;
		stQueueMsg* pmsg=NULL;
		stRmbAdd rmbadd;

		do {
			AILOCKT(m_syncmsglist);
			m_runsyscmsglist.swap(m_syncmsglist);
		} while (false);

		pmsg=NULL;
		it= m_runsyscmsglist.begin();
		if (it!= m_runsyscmsglist.end()){
			pmsg=(*it);
			m_runsyscmsglist.erase(it);
		}
		while (pmsg!=NULL){
			stBaseCmd* pcmd=pmsg->pluscmd();
			unsigned int ncmdlen=pmsg->pluscmdsize();
			bool bofreebuffer=true;
			switch (pcmd->value)
			{
			case stLoadAccountDataRetCmd::_value:
				{
					stLoadAccountDataRetCmd* pdstcmd=(stLoadAccountDataRetCmd*)pcmd;
					if (pdstcmd->nCrossZoneId > 0) {
						CUserEngine::getMe().BroadcastGameSvr(pcmd, ncmdlen, 0, true, pdstcmd->nCrossZoneId, pdstcmd->wCrossTradeid);
					}
					else {
						GameService* gamesvr=GameService::instance();
						if (pdstcmd->nLoadErrorCode!=0){
							g_logger.error("玩家 %s %s 账号存档数据 加载失败(%d)!",pdstcmd->szAccount,pdstcmd->szName,pdstcmd->nLoadErrorCode);
						}else{
							do {
								AILOCKT(CUserEngine::getMe().m_loadokplayers);
								CPlayerObj* player=CUserEngine::getMe().m_playerhash.FindByAccount(pdstcmd->szAccount);
								if (!player){ player=CUserEngine::getMe().m_loadokplayers.FindByAccount(pdstcmd->szAccount);	}
								if (player && player->m_pGateUser && strcmp(player->getName(),pdstcmd->szName)==0){
									player->m_res[ResID::charge] =0;
									player->UpdateChargeNoChange(pdstcmd->gamedata.nRMB, "onlineupdate");//更新当前元宝数量 不需要更新账号数据库
									player->m_res[ResID::hischarge] =pdstcmd->gamedata.nRMBHistory;
									player->m_res[ResID::daycharge] =pdstcmd->gamedata.nDayRMBHistory;
									player->m_dwLastChargeCcyTime=pdstcmd->gamedata.lastAddRMBTime;
									player->m_dwAccountDataSaveCount=pdstcmd->gamedata.savecount;
									player->m_boAccountDataLoadOk=true;
									player->m_emClientVer=pdstcmd->emClientVer;
									strcpy_s(player->m_szUserId, sizeof(player->m_szUserId)-1, pdstcmd->szUserId);
									strcpy_s(player->m_szAppId, sizeof(player->m_szAppId) - 1, pdstcmd->zsAppId);
									g_logger.debug("玩家 %s %s 账号存档数据 加载成功 RMB:%d!",pdstcmd->szAccount,pdstcmd->szName,pdstcmd->gamedata.nRMB);
									return true;
								}
							} while (false);
							g_logger.debug("玩家 %s %s 账号存档数据 加载成功 RMB:%d! 设置失败!",pdstcmd->szAccount,pdstcmd->szName,pdstcmd->gamedata.nRMB);
						}
					}
				}break;
			case stPlayerChangeNameRetCmd::_value:
				{
					stPlayerChangeNameRetCmd* pdstcmd=(stPlayerChangeNameRetCmd*)pcmd;
					if (pdstcmd->dwTrueZoneId == GameService::getMe().m_nTrueZoneid && pdstcmd->btOptType != 1) {
						CPlayerObj* player = NULL;
						do {
							AILOCKT(CUserEngine::getMe().m_loadokplayers);
							player = CUserEngine::getMe().m_playerhash.FindByAccount(pdstcmd->szAccount);
							if (player && player->m_pGateUser && !player->m_pGateUser->isSocketClose() && strcmp(pdstcmd->szOldName, player->getName()) == 0 && strcmp(pdstcmd->szNewName, player->getName()) != 0) {
								if (pdstcmd->nErrorCode == _CREATEPLAYER_RET_SUCCESS_) {
									CUserEngine::getMe().m_playerhash.removeValue(player);
									if (player->m_boCanChangeName) { player->m_boCanChangeName = false; }
									sprintf_s(player->m_szOldName, sizeof(player->m_szOldName) - 1, "%s+%s", player->m_szOldName, pdstcmd->szOldName);
									player->SetName(pdstcmd->szNewName);
									player->NameChanged();
									CUserEngine::getMe().m_playerhash.addValue(player);
									player->m_dwNextSaveRcdTime = 0;
								}
							}
						} while (false);
						stAutoSetScriptParam autoparam((CPlayerObj*)player);
						std::string sOptMode = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<std::string>("ChangeNameOptModeGet", "", player);
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("changenameret", pdstcmd->nErrorCode, (double)pdstcmd->i64onlyid, pdstcmd->szAccount, pdstcmd->szOldName, pdstcmd->szNewName, sOptMode.c_str());
						g_logger.forceLog(zLogger::zINFO, "%s 角色 %s 改名结果 %s errorcode:%d 改名方式：%s", pdstcmd->szAccount, pdstcmd->szOldName, pdstcmd->szNewName, pdstcmd->nErrorCode, sOptMode.c_str());
						GameService::getMe().Send2LogSvr(_SERVERLOG_CHANGENAME_, 0, 0, player, "\'%s\', \'%s\', %I64d, \'%s\', \'%s\', %d, \'%s\'",
							sOptMode.c_str(),
							pdstcmd->szAccount,
							pdstcmd->i64onlyid,
							pdstcmd->szOldName,
							pdstcmd->szNewName,
							pdstcmd->nErrorCode,
							(player ? player->m_pGateUser->szclientip : ""));
					}
				}break;
			}
			if (pmsg && bofreebuffer){	FreePacketBuffer(pmsg);	}

			pmsg=NULL;
			it= m_runsyscmsglist.begin();
			if (it!= m_runsyscmsglist.end()){
				pmsg=(*it);
				m_runsyscmsglist.erase(it);
			}
		}
		//////////////////////////////////////////////////////////////////////////
	}

	return true;
}

bool CUserEngine::ProcessExecCmd(){
	FUNCTION_BEGIN;
	FUNCTION_MONITOR(48, "");
	FUNCTION_WRAPPER(true,"");
	static DWORD s_dwLastExecCmdTick=0;
	if ((::GetTickCount64()-s_dwLastExecCmdTick)>100){
		s_dwLastExecCmdTick=::GetTickCount64();
		std::CSyncList< stGmCmd >::iterator it;
		bool hascurcmd=false;
		stGmCmd curcmd;
		do {
			AILOCKT(m_cmdstrlist);
			it=m_cmdstrlist.begin();
			if (it!=m_cmdstrlist.end()){
				curcmd=(*it);
				hascurcmd=true;
				m_cmdstrlist.erase(it);
			}
		} while (false);

		s_psystem_virtual_palyer->m_btGmLvl=0;
		while(hascurcmd){
			hascurcmd=false;
			const char* pcmd=curcmd.szCmdStr;
			if (pcmd[0]=='@'){
				CChat::sendGmCmd(s_psystem_virtual_palyer,false, curcmd.szName,curcmd.btGmLv,_GMCMD_INPUT_OTHER_,&pcmd[1]);
			}else if (pcmd[0]=='!'){
				if (pcmd[1]=='@'){
					if (pcmd[2]!='!'){
						CChat::sendGmCmd(s_psystem_virtual_palyer,true, curcmd.szName,curcmd.btGmLv,_GMCMD_INPUT_OTHER_,&pcmd[2]);
					}
				}else{
					CChat::sendGmToAll( true,true,curcmd.szName,curcmd.btGmLv,&pcmd[1] );
				}
			}else{
				CChat::sendGmCmd(s_psystem_virtual_palyer,false, curcmd.szName,curcmd.btGmLv,_GMCMD_INPUT_OTHER_,pcmd);
			}
			do {
				AILOCKT(m_cmdstrlist);
				it=m_cmdstrlist.begin();
				if (it!=m_cmdstrlist.end()){
					curcmd=(*it);
					hascurcmd=true;
					m_cmdstrlist.erase(it);
				}
			} while (false);
		}
	}
	return true;
}

bool CUserEngine::ProcessLoadOkPlayers(){
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true,"");
	FUNCTION_MONITOR(48,"");
	static std::vector< CPlayerObj* > g_removed(128);
	g_removed.clear();
	do {

		AILOCKT(m_loadokplayers);
		CPlayerObj* player=NULL;
		for (auto it=m_loadokplayers.begin(),itnext=it;it!=m_loadokplayers.end();it=itnext){
			++itnext;
			player=it->second;
			if (player && player->m_boIsSelOk){
				m_loadokplayers.removeValue(player);
				bool addok=false;
				bool boisrelogin=(player->m_btSelUserState==_PLAYER_STATE_NETCUT_ && player->m_pGateUser->m_Player==player);
				bool boinit=false;
				if (player->m_pTmpGamedata!=NULL && !boisrelogin){
					boinit=InitNewHuman(player);
				}else{
					g_logger.debug("已经初始化");
					boinit=true;
				}
				if (player->m_pTmpGamedata){
					__mt_char_dealloc(player->m_pTmpGamedata);
					player->m_pTmpGamedata=NULL;
				}
				if (boinit){
					g_logger.debug( "%s 玩家数据初始化完成!",player->getName() );
					if (m_playerhash.addValue(player)) {
						player->m_pGateUser->m_Player = player;
						addok = true;
					}
					else {
						g_logger.error("%s 加入玩家列表失败!", player->getName());
					}
					if(!addok){
						if (!boisrelogin){
							g_removed.push_back(player);
						}else{
							g_logger.error( "%s 重登陆失败,进入等待删除列表",player->getName() );
							player->MakeGhost(true,__FUNC_LINE__);		//从loadok list增加到玩家列表失败
						}
					}else{
						g_logger.debug( "%s %.8x玩家进入游戏成功!",player->getName(),player );
						player->m_btSelUserState=_PLAYER_STATE_NONE_;
						stUserReSetState setstate(player->m_pGateUser->m_tmpid,player->m_btSelUserState);
						player->m_pGateUser->m_OwnerLoginSvr->sendcmd(&setstate,sizeof(setstate));
						player->TriggerEvent(player, HADCHANGEDSVRENTER, 1);
					}
				}else{
					g_logger.error( "%s 玩家数据初始化失败!",player->getName() );
					if (!boisrelogin){
						g_removed.push_back(player);
					}else{
						g_logger.debug( "%s 重登陆失败,进入等待删除列表",player->getName() );
						player->MakeGhost(true,__FUNC_LINE__);		//从loadok list增加到玩家列表失败
					}
				}
			}else if (player && (time(NULL)-player->m_addloadoktime)>10 ){
				if (player->m_pTmpGamedata){
					__mt_char_dealloc(player->m_pTmpGamedata);
					player->m_pTmpGamedata=NULL;
				}
				m_loadokplayers.removeValue(player);
				g_removed.push_back(player);
				g_logger.debug( "%s 等待玩家选择超时!",player->getName() );
			}
		}
	} while (false);

	if (g_removed.size()>0){
		for (size_t i=0;i<g_removed.size();i++){
			CPlayerObj* player=g_removed[i];
			CGameGateWayUser* pGateUser=player->m_pGateUser;
			player->m_pGateUser=NULL;
			do {
				AILOCKT(GameService::getMe().m_gatewaysession );
				if (pGateUser)
				{
					pGateUser->m_Player = NULL;
				}
			} while (false);
			GameService::getMe().Add2Delete(pGateUser);
			g_logger.debug( "%s %.8x玩家进入游戏错误,删除对象!",player->getName(),player );
			SAFE_DELETE(player);
		}
	}
	m_processloadokplayertick=GetTickCount64()+200;
	return true;
}

void CUserEngine::ProgressShutDown() {
	if (isCrossSvr() && m_hCrossShutDownKick.size() > 0) {
		ATLOCK(CUserEngine::getMe().m_hCrossShutDownKick);
		auto curtime = time(nullptr);
		for (auto it= m_hCrossShutDownKick.begin();it!= m_hCrossShutDownKick.end();)
		{
			auto& dwCrossShutDownTime = it->second;
			int64_t i64ShutDownZone = it->first;
			if (dwCrossShutDownTime>0 )
			{
				for (auto itt : m_playerhash)
				{
					CPlayerObj* player = itt.second;
					if (!player) 
						continue;
					if (i64ShutDownZone!= MAKELONGLONG(player->m_wSrcTrade, player->m_dwSrcZoneId))
						continue;

					if (dwCrossShutDownTime <= curtime) {
						if ((curtime - dwCrossShutDownTime) < 60 * 2) {
							g_logger.warn("%s踢人状态设置%d" __FUNCTION__, player->getName(), KICK_RELOGIN);
							player->m_btKickState = KICK_RELOGIN;
							player->m_dwDoKickTick = GetTickCount64() + 500;
						}
						else {
							dwCrossShutDownTime = 0;
						}
					}
					else {
						DWORD dwLeftTime = dwCrossShutDownTime - curtime;
						if (dwLeftTime <= 60) {
							if ((curtime - player->m_dwShutDownCheckTime) > 20) {
								CChat::sendGmToUser(player, "您所在源服务器即将维护，请您在 %d秒 内离开跨服服务器，否则将默认进行踢下线处理。", dwLeftTime);
								player->m_dwShutDownCheckTime = curtime;
							}
						}
						else {
							if ((curtime - player->m_dwShutDownCheckTime) > 60) {
								CChat::sendGmToUser(player, "您所在源服务器即将维护，请您在 %d秒 内离开跨服服务器，否则将默认进行踢下线处理。", dwLeftTime);
								player->m_dwShutDownCheckTime = curtime;
							}
						}
					}
				}
				++it;
			}else
			{
				it = m_hCrossShutDownKick.erase(it);
			}
		}
	}

}
void CUserEngine::run(){
	FUNCTION_BEGIN;
	FUNCTION_WRAPPERADD_EXECCOUNT();
	FUNCTION_WRAPPER(true,"");
	FUNCTION_MONITOR(256,"");
	ULONGLONG dwRunStartTick=::GetTickCount64();
	auto curTime = time(nullptr);
	bool borun=false;
	sendLuaError();
	ProcessSyncMsgs();
	ProcessExecCmd();
	ProcessLoadOkPlayers();							//200ms
	m_maphash.run(curTime);				//50ms
	m_SkyLadder.run();
	doChatCmdList();
	doDelayLuaCall();
	m_scriptsql.RunFunc();	
	if((::GetTickCount64()-dwRunStartTick)<200 || borun){
		m_scriptsystem.run();
	}
	time_t shutdowntime = m_shutdowntime;
	time_t lasthintshutdowntime = m_lasthintshutdowntime;
	std::string shutdowndis = m_shutdowndis;
	if (shutdowntime>0 && !m_boIsShutDown){
		if (time(NULL)>shutdowntime){
			m_boIsShutDown=true;
		}else if ( (shutdowntime-time(NULL))<60*3 ){
			if ( (time(NULL)-lasthintshutdowntime)>15 ){
				CChat::sendGmToAll(false,true,"",0x7F,GameService::getMe().GetStrRes(RES_LANG_OTHER_SERVER_RUN_SC),(shutdowntime-time(NULL)),shutdowndis.c_str());
				m_lasthintshutdowntime=time(NULL);
			}
		}else if ( (shutdowntime-time(NULL))<60*10 ){
			if ( (time(NULL)-lasthintshutdowntime)>60 ){
				CChat::sendGmToAll(false,true,"",0x7F,GameService::getMe().GetStrRes(RES_LANG_OTHER_SERVER),timetostr(shutdowntime),shutdowndis.c_str());
				m_lasthintshutdowntime=time(NULL);
			}
		}else{
			if ( (time(NULL)-lasthintshutdowntime)>60*3 ){
				CChat::sendGmToAll(false,true,"",0x7F,GameService::getMe().GetStrRes(RES_LANG_OTHER_SERVER),timetostr(shutdowntime),shutdowndis.c_str());
				m_lasthintshutdowntime=time(NULL);
			}
		}
	}
	static DWORD dwFrameCount = 0;
	dwFrameCount++;
	static time_t m_tFrameTime = GetTickCount64();
	if (GetTickCount64() - m_tFrameTime >= 2000)
	{
		m_dwFPS = dwFrameCount/2;
		m_tFrameTime = GetTickCount64();
		dwFrameCount = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
void CUserEngine::addDelayLuaCall(emDelayLuaCall delaycall,int paramsCount, ...)
{
	stDelayCallFunction *pcmd = CLD_DEBUG_NEW stDelayCallFunction();
	pcmd->type = delaycall;
	va_list ap;
	va_start(ap, paramsCount);
	for(int i = 0; i < paramsCount; i++)          
	{			
		std::string str = va_arg(ap, char *);
		pcmd->params.push_back(str);
	}
	va_end(ap);
	m_delaycallfunction.Push(pcmd);
}

void CUserEngine::doChatCmdList()
{
	while (!m_delaychatcmdlist.empty())
	{
		stDelayChatCMD* pchatcmd;
		m_delaychatcmdlist.Pop(pchatcmd);
		CPlayerObj* player = m_playerhash.FindByName(pchatcmd->playerName.c_str());
		if (player) {
			CChat::sendGmCmd(player, pchatcmd->toallsvr, player->getName(), player->m_btGmLvl, _GMCMD_INPUT_GMINGAME_, pchatcmd->szCMD.c_str());
		}
		SAFE_DELETE(pchatcmd);
	}
}

void CUserEngine::doDelayLuaCall(){

	while (m_delaycallfunction.size()>0)
	{
		stDelayCallFunction* cmd;
		m_delaycallfunction.Pop(cmd);

		switch(cmd->type){
		case reloadRes:
			{
				GameService::getMe().reloadStrRes();
			}break;
		case excutecommand:
			{
				if(cmd->params.size()>=1){
					char szcmdbuf[1024*8];
					ZeroMemory(szcmdbuf,sizeof(stGmRetExecCmd)*2);
					stGmRetExecCmd* retcmd=(stGmRetExecCmd*)&szcmdbuf;
					constructInPlace(retcmd);
					char* szCmd = (char*)cmd->params[0].c_str();
					g_logger.forceLog(zLogger::zINFO,"服务器管理员请求执行命令 %s ",szCmd);
					if(GameService::getMe().exec_input_cmd(NULL,szCmd,retcmd,sizeof(szcmdbuf)-sizeof(stGmRetExecCmd)-16) && g_SaveGMOrder){
						g_SaveGMOrder(szCmd);
					}
					g_logger.forceLog(zLogger::zINFO,"服务器管理员执行GM命令 %s : %s",szCmd,&retcmd->szcmdret[0]);
				}
			}break;

		}

		SAFE_DELETE(cmd);
	}
}

void CUserEngine::doGameSvrProxyCmd(stBaseCmd* pcmd, unsigned int ncmdlen){
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case stProxyMsgRet::_value:
		{
			stProxyMsgRet *pdstcmd = (stProxyMsgRet *)pcmd;
			if (pdstcmd->errorcode!=0){
				stBaseCmd* ppluscmd=(stBaseCmd*)&pdstcmd->msg[0];
				int npluscmdlen=pdstcmd->msg.size;
				switch (ppluscmd->value)
				{
				case stProxyMsg2Gamesvr::_value:
					{
						stProxyMsg2Gamesvr *pdstcmd1 = (stProxyMsg2Gamesvr *)ppluscmd;
						doProxyMsgRet(ppluscmd,npluscmdlen,(stBaseCmd*)&pdstcmd1->msg[0],pdstcmd1->msg.size);
					}break;
				case stProxyMsg2User::_value:
					{
						stProxyMsg2User *pdstcmd1 = (stProxyMsg2User *)ppluscmd;
						doProxyMsgRet(ppluscmd,npluscmdlen,(stBaseCmd*)&pdstcmd1->msg[0],pdstcmd1->msg.size);
					}break;
				}
			}
		}break;
		//////////////////////////////////////////////////////////////////////////
		//跨服地图转移
	case stPreChangeGameSvrCmd::_value:
		{
			//目标服务器
			stPreChangeGameSvrCmd *pdstcmd = (stPreChangeGameSvrCmd *)pcmd;
			CGameMap* pDstMap=m_maphash.FindByOnlyId(pdstcmd->full_mapid);
			stPreChangeGameSvrCmdRet checkret;
			checkret.nerrorcode=(BYTE)-1;
			checkret.oldPlayerOnlyId =pdstcmd->oldPlayerOnlyId;
			checkret.space_move_tick=pdstcmd->space_move_tick;
			checkret.dx=pdstcmd->dx;
			checkret.dy=pdstcmd->dy;
			checkret.dwSrcZoneId = pdstcmd->dwSrcZoneId;
			checkret.dwDestZoneId = pdstcmd->dwDestZoneId;
			checkret.wSrcTradeid=pdstcmd->wSrcTradeid;
			checkret.wDestTradeid=pdstcmd->wDestTradeid;
			if(m_dwThisGameSvrVersion==pdstcmd->dwGameSvrVersion){
				if (!pDstMap)
				{
					checkret.nerrorcode = (BYTE)-3;
				}
				else
				{
					checkret.nerrorcode = 0;
				}
			}else{
				checkret.nerrorcode=(BYTE)-4;
				g_logger.error("服务器版本不匹配,当前版本 %s,%d区 源服务器版本 %s",timetostr(m_dwThisGameSvrVersion.load()),pdstcmd->dwSrcZoneId,timetostr(pdstcmd->dwGameSvrVersion));
			}
			BroadcastGameSvr(&checkret,sizeof(checkret),pdstcmd->old_gamesvr_id_type,true,pdstcmd->dwSrcZoneId,pdstcmd->wSrcTradeid);
			g_logger.debug( "目标服务器: onlyId= %I64d 跨服换地图 %I64u( %d:%d ) err=%d",pdstcmd->oldPlayerOnlyId,pdstcmd->full_mapid,pdstcmd->dx,pdstcmd->dy,checkret.nerrorcode );
		}break;
	case stPreChangeGameSvrCmdRet::_value:
		{
			//源服务器
			stPreChangeGameSvrCmdRet *pdstcmd = (stPreChangeGameSvrCmdRet *)pcmd;
			CPlayerObj *pUser = m_playerhash.FindByOnlyId(pdstcmd->oldPlayerOnlyId);
			if ( pUser && pUser->getSwitchSvrInfo() ){
				stSpaveMoveInfo* pSpaveMoveInfo=pUser->getSwitchSvrInfo();
				if (pdstcmd->nerrorcode==0){
					if (pSpaveMoveInfo->btMoveState==stSpaveMoveInfo::_SPACE_WAIT_SWITCHSVR_CHECK_ 
						&& pUser->m_pGateUser 
						&& pUser->m_pGateUser->m_OwnerDbSvr){
							pSpaveMoveInfo->btMoveState=stSpaveMoveInfo::_SPACE_WAIT_SWITCHSVR_; 
							pUser->m_boIsWaitChangeSvr=true;
							//保存目标地图ID
							//存档前执行,切换服务器之前的一些操作
							pUser->OnPlayerBeforeChangeSvrSaveData();
							bool push2save = false;
							if (isCrossSvr()) {
								pUser->CrossSvrSavePlayerData(_SAVE_TYPE_CHANGESVR_);
								push2save = true;
							}
							else
							{
								push2save = pUser->m_pGateUser->m_OwnerDbSvr->push_back2save(pUser, _SAVE_TYPE_CHANGESVR_);
							}
							if (push2save) {
								pUser->m_dwNextSaveRcdTime=time(NULL)+GameService::getMe().m_svrcfg.dwsavercdintervaltime;

								PTR_CMD(stSvrChangeGameSvrCmd,psendcmd,getsafepacketbuf());
								if (psendcmd){
									psendcmd->ip_type=pUser->m_pGateUser->m_iptype;
									psendcmd->old_gamesvr_id_type=GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value;
									psendcmd->oldPlayerOnlyId =pUser->m_i64UserOnlyID;
									psendcmd->spacemovetype=pSpaveMoveInfo->btMoveType;
									psendcmd->dwSrcZoneId = pdstcmd->dwSrcZoneId;
									psendcmd->dwDestZoneId = pdstcmd->dwDestZoneId;
									psendcmd->Activatetime=time(NULL);
									psendcmd->wSrcTradeid=pdstcmd->wSrcTradeid;
									psendcmd->wDestTradeid=pdstcmd->wDestTradeid;
									if (pUser->fullPlayerSaveData(&psendcmd->data,getsafepacketbuflen()-sizeof(*psendcmd)-1024,_SAVE_TYPE_CHANGESVR_ ,pdstcmd->wDestTradeid)>0){
										pUser->fullPlayerChangeSvrData(&psendcmd->data,getsafepacketbuflen()-sizeof(*psendcmd)-1024,_SAVE_TYPE_CHANGESVR_ );
										psendcmd->data.gamedata.dwmapid=pSpaveMoveInfo->DstMap->getMapId();
										psendcmd->data.gamedata.wclonemapid =pSpaveMoveInfo->dwCloneMapId;
										psendcmd->dwSrcTrueZoneId = GameService::getMe().m_nTrueZoneid;
										
										if (BroadcastGameSvr( psendcmd,psendcmd->getSize() ,MAKELONG(pSpaveMoveInfo->dwSvrIdx,_GAMESVR_TYPE)/*pSpaveMoveInfo->DstMap->getSvrIdType()*/,true,psendcmd->dwDestZoneId,pdstcmd->wDestTradeid) ){	
											//存档后执行,切换服务器之前的一些操作
											pUser->OnPlayerBeforeChangeGameSvr();
											g_logger.debug( "源服务器: %s(%I64d) 跨服换地图数据转移",pUser->getName(),pUser->m_i64UserOnlyID);
											if(isCrossSvr(psendcmd->dwDestZoneId)){
												pUser->m_dwCrossKickZoneId=psendcmd->dwDestZoneId;
												pUser->m_wCrossKickTradeid=psendcmd->wDestTradeid;
											}
											return;
										}	
									}
								}
							}else{
								g_logger.debug( "源服务器: %s(%I64d) push_back2save error",pUser->getName(),pUser->m_i64UserOnlyID );
							}
					}
				}
				if(pdstcmd->nerrorcode==(BYTE)-4){
					stAutoSetScriptParam autoparam(pUser);
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("GameSvrVersionNotSame");
				}
				//只有无法移动过去的时候 清除跨服移动指针~不保存目标地图ID
				if (pUser->getSwitchSvrInfo()){ pUser->getSwitchSvrInfo()->btMoveState=stSpaveMoveInfo::_SPACEMOVE_FINISH_;pUser->setSwitchSvrInfo(NULL); }
			}else{
				g_logger.debug( "源服务器: onlyId= %I64d  getSwitchSvrInfo err=%d",pdstcmd->oldPlayerOnlyId,pdstcmd->nerrorcode );
			}
			g_logger.debug( "源服务器: onlyId= %I64d 跨服换地图失败 err=%d",pdstcmd->oldPlayerOnlyId,pdstcmd->nerrorcode );
		}break;
	case stSvrChangeGameSvrCmd::_value:
		{
			//目标服务器
			stSvrChangeGameSvrCmd *pdstcmd = (stSvrChangeGameSvrCmd *)pcmd;
			if (pdstcmd ){
				GameService * gamesvr=GameService::instance();
				stSvrChangeGameSvrCmdRet checkret;
				checkret.nerrorcode=(BYTE)-1;
				checkret.oldPlayerOnlyId =pdstcmd->oldPlayerOnlyId;
				checkret.dst_gamesvr_id_type=gamesvr->m_Svr2SvrLoginCmd.svr_id_type_value;
				checkret.dwSrcZoneId = pdstcmd->dwSrcZoneId;
				checkret.dwDestZoneId = pdstcmd->dwDestZoneId;
				checkret.wSrcTradeid=pdstcmd->wSrcTradeid;
				checkret.wDestTradeid=pdstcmd->wDestTradeid;
				if ( pdstcmd->getSize()<=(int)ncmdlen && pdstcmd->data.wver==_DBCOL_PLAYERDAT_VER_  && pdstcmd->data.wtype==_DBCOL_PLAYERDAT_TYPE_ ){
					stSvrChangeGameSvrCmd* pbuffer=((stSvrChangeGameSvrCmd*)__mt_char_alloc(pdstcmd->getSize()+32));
					if (pbuffer){
						CopyMemory(pbuffer,pdstcmd,pdstcmd->getSize());
						pbuffer->Activatetime=time(NULL);
						do {
							AILOCKT(gamesvr->m_waitchangesvrhash);
							stSvrChangeGameSvrCmd* poldbuffer=gamesvr->m_waitchangesvrhash.FindByAccount(pbuffer->data.szAccount);
							if (poldbuffer){ 
								gamesvr->m_waitchangesvrhash.removeValue(poldbuffer); 
								__mt_char_dealloc(poldbuffer);
								poldbuffer=NULL;
							}
							poldbuffer=gamesvr->m_waitchangesvrhash.FindByTmpid(pbuffer->data.tmpid.tmpid_value);
							if (poldbuffer){ 
								gamesvr->m_waitchangesvrhash.removeValue(poldbuffer);
								__mt_char_dealloc(poldbuffer);
								poldbuffer=NULL;
							}
						} while (false);
						if ( gamesvr->m_waitchangesvrhash.addValue(pbuffer) ){
							checkret.nerrorcode=(BYTE)-2;
							g_logger.debug("目标服务器: %s 角色切换服务器,游戏服务器重置有效换服时间为%d分钟!",pbuffer->data.szAccount,_WAIT_INTOGAMESVR_TIME_/60);
							//角色数据添加到等待连接列表后通知原服务器
							if ( gamesvr->GetALoginIpInfo(checkret.ip,checkret.port,pdstcmd->ip_type,checkret.szTGWip) ){
								checkret.nerrorcode=0;
								checkret.dwTrueZoneid = gamesvr->m_nTrueZoneid;
								g_logger.debug("%s %s 重定向游戏服务器(%d) %s:%d",pbuffer->data.szAccount,pbuffer->data.gamedata.szName,gamesvr->m_Svr2SvrLoginCmd.svr_id,inet_ntoa(checkret.ip),checkret.port);
							}
						}else{
							__mt_char_dealloc(pbuffer);
							pbuffer=NULL;
						}
					}
				}else{
					g_logger.debug( "目标服务器: %s 角色切换服务器,角色数据版本不一致!",pdstcmd->data.szAccount );
				}
				BroadcastGameSvr(&checkret,sizeof(checkret),pdstcmd->old_gamesvr_id_type,true,pdstcmd->dwSrcZoneId,pdstcmd->wSrcTradeid);
				g_logger.debug( "目标服务器:%s : onlyId= %I64d 跨服换地图接收数据 err=%d",pdstcmd->data.szAccount,pdstcmd->oldPlayerOnlyId,checkret.nerrorcode );
			}
		}break;
	case stSvrChangeGameSvrCmdRet::_value:
		{
			//源服务器
			stSvrChangeGameSvrCmdRet *pdstcmd = (stSvrChangeGameSvrCmdRet *)pcmd;
			CPlayerObj *pUser = m_playerhash.FindByOnlyId(pdstcmd->oldPlayerOnlyId);
			stSvrChangeGameSvrOkCmd retcmd;
			retcmd.nerrorcode=(BYTE)-1;
			retcmd.oldPlayerOnlyId =pdstcmd->oldPlayerOnlyId;
			if ( pUser){
				stSpaveMoveInfo* pSpaveMoveInfo=pUser->getSwitchSvrInfo();
				if (pdstcmd->nerrorcode==0){
					if (pSpaveMoveInfo
						&& pSpaveMoveInfo->btMoveState==stSpaveMoveInfo::_SPACE_WAIT_SWITCHSVR_ 
						&& (MAKELONG(pSpaveMoveInfo->dwSvrIdx,_GAMESVR_TYPE)==pdstcmd->dst_gamesvr_id_type/*pSpaveMoveInfo->DstMap->getSvrIdType()==pdstcmd->dst_gamesvr_id_type*/ || pdstcmd->dwDestZoneId != pdstcmd->dwSrcZoneId)){
							retcmd.nerrorcode=0;
							BroadcastGameSvr( &retcmd,sizeof(retcmd),MAKELONG(pSpaveMoveInfo->dwSvrIdx,_GAMESVR_TYPE)/*pSpaveMoveInfo->DstMap->getSvrIdType()*/ );
							pUser->OnPlayerChangeSvrSuccess();

							if (pUser->m_boWaitChangeSvrSaveData){;};
							pUser->m_nWaitChangeSvr_newsvr =pdstcmd->dst_gamesvr_id_type;
							pUser->m_nWaitChangeSvr_oldsvr =pUser->GetEnvir()->getSvrIdType();
							strcpy_s(pUser->m_szTGWip,sizeof(pUser->m_szTGWip)-1,pdstcmd->szTGWip);
							pUser->m_nWaitChangeSvr_ip =pdstcmd->ip;
							pUser->m_nWaitChangeSvr_port =pdstcmd->port;
							pUser->m_dwChangeZoneid = pdstcmd->dwDestZoneId;
							pUser->m_dwChangeTrueZoneid = pdstcmd->dwTrueZoneid;
							pUser->m_nWaitChangeSvr_mapid =pSpaveMoveInfo->DstMap->getMapId();
							//切换服务器完成
							g_logger.debug( "源服务器:%s : onlyId= %I64d 跨服换地图数据转移成功,切换服务器!",pUser->getName(),pdstcmd->oldPlayerOnlyId);
							if(isCrossSvr(pdstcmd->dwDestZoneId) && !isCrossSvr()){
								stCrossZoneToLogin retcmd;
								strcpy_s(retcmd.szAccount,sizeof(retcmd.szAccount)-1,pUser->getAccount());
								retcmd.nCrossZoneId=pdstcmd->dwDestZoneId;
								retcmd.wCrossTradeid=pdstcmd->wDestTradeid;
								GameService* gamesvr=GameService::instance();
								gamesvr->Send2LoginSvrs(&retcmd,sizeof(stCrossZoneToLogin));
							}
					}
				}
				if (pUser->getSwitchSvrInfo()){ pUser->getSwitchSvrInfo()->btMoveState=stSpaveMoveInfo::_SPACEMOVE_FINISH_; }
			}
			g_logger.debug( "源服务器: onlyId= %I64d 跨服换地图数据转移结果 err=%d",pdstcmd->oldPlayerOnlyId,pdstcmd->nerrorcode );
		}break;
	case stSvrChangeGameSvrOkCmd::_value:
		{
			//目标服务器
			//跨服结束 允许玩家登陆目标服务器
		}break;
	case stCrossSavePlayerData::_value:
		{
			stCrossSavePlayerData* pdstcmd=(stCrossSavePlayerData*)pcmd;
			GameService * gamesvr=GameService::instance();
			char szOnlyidStr[64]={0}; sprintf_s(szOnlyidStr,sizeof(szOnlyidStr)-1,"%I64d\0",pdstcmd->data.gamedata.i64UserOnlyId);
			CDBSvrGameConnecter* DbSvr= GameService::getMe().m_dbsvrconnecter;
			if(DbSvr){
				DbSvr->push_back2save(&pdstcmd->data,pdstcmd->btSaveType);
			}
			if(pdstcmd->btSaveType==_SAVE_TYPE_LOGIN_OUT_){
				stCrossZoneToLogin retcmd;
				strcpy_s(retcmd.szAccount,sizeof(retcmd.szAccount)-1,pdstcmd->data.szAccount);
				retcmd.nCrossZoneId=0;
				retcmd.wCrossTradeid=0;
				gamesvr->Send2LoginSvrs(&retcmd,sizeof(stCrossZoneToLogin));
			}
		}break;
	case stCrossKickPlayer::_value:
		{
			stCrossKickPlayer* pdstcmd=(stCrossKickPlayer*)pcmd;
			do 
			{
				AILOCKT(m_playerhash);
				CPlayerObj *pUser = m_playerhash.FindByAccount(pdstcmd->szAccount);
				if(pUser){
					pUser->m_boIsWaitChangeSvr = false;
					g_logger.warn("%s踢人状态设置%d" __FUNCTION__,pUser->getName(), KICK_RELOGIN);
					pUser->m_btKickState=KICK_RELOGIN;
					pUser->m_dwDoKickTick=GetTickCount64()+500;
					pUser->m_btCrossKickType=pdstcmd->btKickType;
					if (pdstcmd->btKickType == 0) {
						BUFFER_CMD(stForceOffLine, offlineCmd, 512);
						offlineCmd->btErrorCode = 3;
						char reasonDesc[256];
						ZeroMemory(reasonDesc, 256);
						sprintf_s(reasonDesc, 255, "强制断线，您的账号在其他设备需重新登陆！!");
						offlineCmd->reasonDesc.push_str(reasonDesc);
						pUser->SendMsgToMe(offlineCmd, sizeof(*offlineCmd) + offlineCmd->reasonDesc.getarraysize());
					}
				}
			} while (false);
			
			do 
			{
				AILOCKT(m_loadokplayers);
				CPlayerObj *pUser=m_loadokplayers.FindByAccount(pdstcmd->szAccount);
				if(pUser)
				{
					m_loadokplayers.removeValue(pUser);
				}
			} while (false);
			
			do 
			{
				GameService * gamesvr=GameService::instance();
				AILOCKT(gamesvr->m_waitchangesvrhash);
				stSvrChangeGameSvrCmd* poldbuffer=gamesvr->m_waitchangesvrhash.FindByAccount(pdstcmd->szAccount);
				if (poldbuffer){ 
					gamesvr->m_waitchangesvrhash.removeValue(poldbuffer); 
					__mt_char_dealloc(poldbuffer);
					poldbuffer=NULL;
				}
			} while (false);
		}break;
	case stCrossShutDownKickPlayer::_value:
		{
			stCrossShutDownKickPlayer* pdstcmd=(stCrossShutDownKickPlayer*)pcmd;
			__int64 i64ShutDownZone=MAKELONGLONG(pdstcmd->nShutDownTradeId,pdstcmd->nShutDownZoneId);
			do 
			{
				ATLOCK(m_hCrossShutDownKick);
				auto it =m_hCrossShutDownKick.find(i64ShutDownZone);
				if(it!=m_hCrossShutDownKick.end()){
					m_hCrossShutDownKick.erase(it);
				}
				if(pdstcmd->dwShutDownLeftTime){
					DWORD dwShutDownTime=(pdstcmd->dwShutDownLeftTime+time(NULL));
					m_hCrossShutDownKick[i64ShutDownZone] = dwShutDownTime;
				}

			} while (false);
			
		

		}break;
	case stLoadAccountDataCmd::_value:
		{
			stLoadAccountDataCmd* pchatcmd = (stLoadAccountDataCmd*)pcmd;
			GameService::instance()->m_loginsvrconnter->sendcmd(pcmd, ncmdlen);
		}break;
	case stLoadAccountDataRetCmd::_value:
		{
			stLoadAccountDataRetCmd* pdstcmd = (stLoadAccountDataRetCmd*)pcmd;
			GameService* gamesvr = GameService::instance();
			if (pdstcmd->nLoadErrorCode != 0) {
				g_logger.error("玩家 %s %s 账号存档数据 加载失败(%d)!", pdstcmd->szAccount, pdstcmd->szName, pdstcmd->nLoadErrorCode);
			}
			else {
				do {
					AILOCKT(CUserEngine::getMe().m_loadokplayers);
					CPlayerObj* player = CUserEngine::getMe().m_playerhash.FindByAccount(pdstcmd->szAccount);
					if (!player) { player = CUserEngine::getMe().m_loadokplayers.FindByAccount(pdstcmd->szAccount); }
					if (player && player->m_pGateUser && /*player->m_pGateUser->m_tmpid.tmpid_value==pdstcmd->tmpid.tmpid_value &&*/ strcmp(player->getName(), pdstcmd->szName) == 0) {

						player->m_res[ResID::charge] = 0;
						player->UpdateChargeNoChange(pdstcmd->gamedata.nRMB, "onlineupdate");//更新当前元宝数量 不需要更新账号数据库

						player->m_res[ResID::hischarge] = pdstcmd->gamedata.nRMBHistory;
						player->m_res[ResID::daycharge] = pdstcmd->gamedata.nDayRMBHistory;
						player->m_dwLastChargeCcyTime = pdstcmd->gamedata.lastAddRMBTime;
						player->m_dwAccountDataSaveCount = pdstcmd->gamedata.savecount;
						player->m_boAccountDataLoadOk = true;
						player->m_emClientVer = pdstcmd->emClientVer;
						//player->UpdateToGlobalSvr();

						strcpy_s(player->m_szUserId, sizeof(player->m_szUserId) - 1, pdstcmd->szUserId);
						strcpy_s(player->m_szAppId, sizeof(player->m_szAppId) - 1, pdstcmd->zsAppId);


						g_logger.debug("玩家 %s %s 账号存档数据 加载成功 RMB:%d!", pdstcmd->szAccount, pdstcmd->szName, pdstcmd->gamedata.nRMB);
					}
				} while (false);
			}
		}break;
	case stUpdateAccountRmbCmd::_value:
		{
			stUpdateAccountRmbCmd* pdstcmd = (stUpdateAccountRmbCmd*)pcmd;
			if (GameService::instance()->m_loginsvrconnter->IsConnected()) {
				if (!GameService::instance()->m_loginsvrconnter->sendcmd(pcmd, ncmdlen)) {
					CUserEngine::getMe().RMBUnusual = true;
				}
			}
			else {
				CUserEngine::getMe().RMBUnusual = true;
			}
		}break;
	case stCretChat::_value:
		{
			stCretChat* pchatcmd =  (stCretChat*)pcmd;
			DWORD dwZoneid=HIWORD(pchatcmd->dwZoneId);
			DWORD dwTradeId=LOWORD(pchatcmd->dwZoneId);
			if(dwZoneid && (dwZoneid!=GameService::getMe().m_nZoneid || pchatcmd->i64DestOnlyId!=0)){//由跨服发回源服的聊天
				switch(pchatcmd->btChatType)
				{
				case CHAT_TYPE_PRIVATE:
					{
						CPlayerObj* pToPlayer=m_playerhash.FindByName(pchatcmd->szTargetName);
						if(pToPlayer){
							if(!(pToPlayer->m_dwUserConfig & USERCONFIG_CANPRIVITECHAT)){
								BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
								CChat::SETSENDCMD(retcmd,CHAT_TYPE_SYSTEM,vformat(GameService::getMe().GetStrRes(RES_LANG_USERCONFIGNOTPRIVITE),pToPlayer->getName()),pchatcmd->szName,NULL,0);
								BroadcastGameSvr(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize(),LODWORD(pchatcmd->i64SrcOnlyId),true,dwZoneid,dwTradeId);
								return;
							}
							pchatcmd->i64DestOnlyId=0;
							pToPlayer->SendMsgToMe(pchatcmd,sizeof(*pchatcmd)+pchatcmd->szZeroChatMsg.getarraysize());
							pchatcmd->dwVip=pToPlayer->getVipType();
							pchatcmd->dwGuildId = pToPlayer->m_GuildInfo.dwGuildId;
							BroadcastGameSvr(pchatcmd,sizeof(*pchatcmd)+pchatcmd->szZeroChatMsg.getarraysize(),LODWORD(pchatcmd->i64SrcOnlyId),true,dwZoneid,dwTradeId);
						}else{
							if(pchatcmd->i64DestOnlyId==0){
								CUserEngine::getMe().SendMsg2SuperSvr(pchatcmd,sizeof(*pchatcmd)+pchatcmd->szZeroChatMsg.getarraysize());
							}
						}
					}break;
				case CHAT_TYPE_CLAN:
					{
						if(pchatcmd->dwGuildId){
							CChat::SENDTOGLOBAL(vformat("%d",pchatcmd->dwGuildId),"",0,CHAT_TYPE_CLAN,pchatcmd);
						}
					}break;
				}
			}else{//由源服发到跨服的聊天
				switch(pchatcmd->btChatType)
				{
				case CHAT_TYPE_PRIVATE:
					{
						CPlayerObj* pPlayer=m_playerhash.FindByName(pchatcmd->szName);
						if(pPlayer){
							pPlayer->SendMsgToMe(pchatcmd,sizeof(*pchatcmd)+pchatcmd->szZeroChatMsg.getarraysize());
						}
					}break;
				case CHAT_TYPE_CLAN:
					{
						CPlayerObj* pPlayer=m_playerhash.FindByName(pchatcmd->szTargetName);
						if(pPlayer){
							pPlayer->SendMsgToMe(pchatcmd,sizeof(*pchatcmd)+pchatcmd->szZeroChatMsg.getarraysize());
						}
					}break;
				}
			}
		}break;
	case stCrossPack::_value: {
		stCrossPack* pSrcCmd = (stCrossPack*)pcmd;
		packstr.assign(pSrcCmd->szPack.getptr(), pSrcCmd->dwLength);
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("OnCrossPack", pSrcCmd->ntype);

	}break;
	}
}

void CUserEngine::doSuperSvrCmd(stBaseCmd* pcmd, unsigned int ncmdlen){
	FUNCTION_BEGIN;
	stHumanRankInfo * pc = NULL;
	stPetDetailInfo *pc1 = NULL;
	switch (pcmd->value)
	{
	case stSendRankMsgSuperSrv::_value:
		{
			stSendRankMsgSuperSrv *pCmd = (stSendRankMsgSuperSrv *)pcmd;
			stBaseCmd* pBaseCmd = (stBaseCmd*)pCmd->msg.getptr();
			switch(pBaseCmd->value)
			{
			case stGameSvrGetRankTopTenRet::_value:
				{
					stGameSvrGetRankTopTenRet* pSrcCmd=(stGameSvrGetRankTopTenRet*)pBaseCmd;
					if(pSrcCmd->nRankType>=0 && pSrcCmd->nRankType<Rank_Max_Count){
						m_cListV[pSrcCmd->nRankType].clear();
						m_cListV[pSrcCmd->nRankType].resize(pSrcCmd->stHumanInfo.size);
						if (m_cListV[pSrcCmd->nRankType].size())
						{
							CopyMemory((void*)m_cListV[pSrcCmd->nRankType].data(), pSrcCmd->stHumanInfo.getptr(), sizeof(stHumanRankInfo) * m_cListV[pSrcCmd->nRankType].size());
							//最强
							if(pSrcCmd->nRankType == Cret_Level_Rank)
							{
								m_i64LevelMaxPlayer = m_cListV[pSrcCmd->nRankType][0].i64OnlyId;
							}
							if (pSrcCmd->nRankType == Cret_FightScore_Rank) {
								for (int i = 0; i < 3; i++) {
									CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("RankRefresh", pSrcCmd->nRankType, m_cListV[pSrcCmd->nRankType][i].szName, (double)m_cListV[pSrcCmd->nRankType][i].i64OnlyId, m_cListV[pSrcCmd->nRankType][i].btJob,i + 1);
								}
							} else {
								CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("RankRefresh", pSrcCmd->nRankType, m_cListV[pSrcCmd->nRankType][0].szName, (double)m_cListV[pSrcCmd->nRankType][0].i64OnlyId, m_cListV[pSrcCmd->nRankType][0].btJob, 1);
							}
						}
					}
				}break;
			case stGameSvrGetGGRankRet::_value:
				{
					stGameSvrGetGGRankRet* pSrcCmd = (stGameSvrGetGGRankRet*)pBaseCmd;
					if(pSrcCmd->nRankType>=0 && pSrcCmd->nRankType<Rank_Max_Count){
						m_cGGListV[pSrcCmd->nRankType].clear();
						m_cGGListV[pSrcCmd->nRankType].resize(pSrcCmd->stHumanInfo.size);
						if (m_cGGListV[pSrcCmd->nRankType].size()){
							CopyMemory((void*)m_cGGListV[pSrcCmd->nRankType].data(), pSrcCmd->stHumanInfo.getptr(), sizeof(stHumanRankInfo) * m_cGGListV[pSrcCmd->nRankType].size());
						}
					}
				}break;
			case stGetCrossSvrRankTopTenRet::_value:
				{
					stGetCrossSvrRankTopTenRet* pSrcCmd=(stGetCrossSvrRankTopTenRet*)pBaseCmd;
					m_UserKillV.clear();
					for (int i = 0; i < pSrcCmd->stUserKillInfo.size; ++i) {
						m_UserKillV.push_back(pSrcCmd->stUserKillInfo[i]);
					}
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("ChangeStatueNpcName");
				}break;
			case stSendFengshenChallengeRealRet::_value:
				{
					stSendFengshenChallengeRealRet *pdstcmd = (stSendFengshenChallengeRealRet *)pBaseCmd;
					if (pdstcmd)
					{
						for (int i = 0; i < pdstcmd->idarry.size; ++i)
						{
							CPlayerObj *pUser = m_playerhash.FindByOnlyId(pdstcmd->idarry[i]);
							if (pUser)
							{
								stCretRefreshFengshen refreshFengshen;

								//attacter & defender refresh after fight
								pUser->SendMsgToMe(&refreshFengshen, sizeof(refreshFengshen));
							}
						}	
					}
				}break;
			}
		}break;
	case stProxyMsg2OneSuperSrv::_value:
		{
			stProxyMsg2OneSuperSrv *pSrcCmd = (stProxyMsg2OneSuperSrv *)pcmd;
			CPlayerObj *pUser = m_playerhash.FindByName(pSrcCmd->szName);
			if(pUser)
			{
				stBaseCmd* pBaseCmd = (stBaseCmd*)pSrcCmd->msg.getptr();
				switch(pBaseCmd->value)
				{
				case stCretChat::_value:
					{
						stCretChat *pChat = (stCretChat *)pBaseCmd;
						if(pChat->btChatType == CHAT_TYPE_PRIVATE){
							if(!(pUser->m_dwUserConfig & USERCONFIG_CANPRIVITECHAT)){
								CChat::sendSystem(pChat->szName, GameService::getMe().GetStrRes(RES_LANG_USERCONFIGNOTPRIVITE),pUser->getName());
								return;
							}
						}
					}break;
				}
				pUser->SendMsgToMe(pSrcCmd->msg.getptr(),pSrcCmd->msg.getarraysize());
			}
		}break;
	case stProxyMsg2AllUser::_value:
		{
			stProxyMsg2AllUser *pSrcCmd = (stProxyMsg2AllUser *)pcmd;
			stBaseCmd* pBaseCmd = (stBaseCmd*)pSrcCmd->msg.getptr();
			switch(pBaseCmd->value)
			{
			case stMarqueeNotice::_value://针对不同包进行发送的
				{
					stMarqueeNotice *notice = (stMarqueeNotice*)pBaseCmd;
					if(notice){
						CPlayersHashManager::iterator it,itnext;
						CPlayerObj* pUser=NULL;
						AILOCKT(m_playerhash);
						for (it=m_playerhash.begin(),itnext=it;it!=m_playerhash.end();it=itnext){
							itnext++;
							pUser=it->second;
							if(pUser && pUser->m_pGateUser && pUser->m_pGateUser->m_szClientBundleId[0]) {
								if(strcmp(notice->szBundleIds,"All") == 0 ||  strstr(notice->szBundleIds,pUser->m_pGateUser->m_szClientBundleId)){
									pUser->SendMsgToMe(notice->chatmsg.getptr(),notice->chatmsg.getarraysize());
								}
							}
						}
					}

				}break;
			default:
				SendMsg2AllUser(pSrcCmd->msg.getptr(),pSrcCmd->msg.getarraysize());
				break;
			}

		}break;
	case stSendOhterMsgSuperSrv::_value:
		{
			stSendOhterMsgSuperSrv *pSrcCmd = (stSendOhterMsgSuperSrv *)pcmd;
			stBaseCmd* pBaseCmd = (stBaseCmd*)pSrcCmd->msg.getptr();
			int nBaseLen = pSrcCmd->msg.getarraysize();
			switch(pBaseCmd->value)
			{
			case stBanplayer::_value:
				{
					CPlayerObj *pUser = m_playerhash.FindByName(pSrcCmd->szName);
					if(pUser){
						stBanplayer* pBanCmd = (stBanplayer*)pBaseCmd;
						if(pBanCmd->dwBanType == 0){
							pUser->m_dwBanChatTime=time(NULL)+pBanCmd->dwBanTime;
						}
					}
				}break;
			case stCretViewEquip::_value:
				{
					stCretViewEquip* pViewCmd = (stCretViewEquip*)pBaseCmd;
					BUFFER_CMD(stCretViewEquipRet , pViewRet, stBasePacket::MAX_PACKET_SIZE);
					pViewRet->btErrorCode = 0;
					pViewRet->btType=pViewCmd->btType;
					pViewRet->btFlag=pViewCmd->btFlag;
					CPlayerObj * player = CUserEngine::getMe().m_playerhash.FindByName(pSrcCmd->szName);
					if (player){
						pViewRet->simpleAbility = player->m_stAbility;
						pViewRet->btSex = player->m_siFeature.sex;
						pViewRet->btJob = player->m_siFeature.job;
						pViewRet->btFace = player->m_siFeature.face;	//脸部
						pViewRet->btHair = player->m_siFeature.hair;	//头发
						pViewRet->btEye = player->m_siFeature.eye;	//眼睛
						pViewRet->btNose = player->m_siFeature.nose;	//鼻子
						pViewRet->btMouth = player->m_siFeature.mouth;	//嘴巴
						pViewRet->dwLevel = player->m_dwLevel;
						pViewRet->i64FightScore = player->m_stAbility.i64FightScore;
						pViewRet->dwHeadPortrait = player->m_dwHeadPortrait;
						pViewRet->dwDressId = CALL_LUARET<DWORD>("GetFeatureId", 0, player,1); //时装
						strcpy_s(pViewRet->szName, sizeof(pViewRet->szName), player->getName());
						strcpy_s(pViewRet->szClanName, sizeof(pViewRet->szClanName), player->m_GuildInfo.szGuildName);
						if(pViewRet->btType == 0){
							for (int i = 0; i < EQUIP_MAX_COUNT; i++) {
								if (player->m_Packet.m_stEquip[i]) {
									pViewRet->items.push_back(player->m_Packet.m_stEquip[i]->m_Item, __FUNC_LINE__);
								}
							}
						}				
					}else{
						pViewRet->btErrorCode = 1;
					}
					SENDMSG2SUPER(stSendOhterMsgSuperSrv,pViewCmd->szName, pViewRet,sizeof(*pViewRet) + pViewRet->items.getarraysize());
				}break;
			}
		}break;
	case stSendMailMsgSuperSrv::_value:
		{
			stSendMailMsgSuperSrv *pSrcCmd = (stSendMailMsgSuperSrv *)pcmd;			
			if(CPlayerObj* pUser = m_playerhash.FindByOnlyId(pSrcCmd->i64SrcOnlyId))  
				pUser->doMailFromSuperSvr((stBaseCmd*)pSrcCmd->msg.getptr(),pSrcCmd->msg.getarraysize());
		}break;
	case stSendConsignmentMsgSuperSrv::_value:		//拍卖行消息
		{
			stSendConsignmentMsgSuperSrv *pSrcCmd = (stSendConsignmentMsgSuperSrv *)pcmd;
			CPlayerObj *pUser = m_playerhash.FindByOnlyId(pSrcCmd->i64SrcOnlyId);
			if(pUser)  pUser->doConsignFromSuperSvr((stBaseCmd*)pSrcCmd->msg.getptr(),pSrcCmd->msg.getarraysize());
		}break;
	case stSendStallMsgSuperSrv::_value://随身摊位
		{
			stSendStallMsgSuperSrv *pSrcCmd = (stSendStallMsgSuperSrv *)pcmd;
			CPlayerObj *pUser = m_playerhash.FindByOnlyId(pSrcCmd->i64SrcOnlyId); 
			if(pUser){
				pUser->doStallCmdFromSupSvr((stBaseCmd*)pSrcCmd->msg.getptr(),pSrcCmd->msg.getarraysize());
			}
		}break;
	case stNotifyExecCmd::_value:
		{
			stNotifyExecCmd *pdstcmd = (stNotifyExecCmd *)pcmd;
			stGmCmd curcmd;
			curcmd.btGmLv=pdstcmd->btSrcGmLv;
			strcpy_s(curcmd.szName,sizeof(curcmd.szName)-1,pdstcmd->szSrcName);
			strcpy_s(curcmd.szCmdStr,_MAX_RETCMD_LEN_-1,pdstcmd->szCmd);
			do {
				AILOCKT(m_cmdstrlist);
				m_cmdstrlist.push_back(curcmd);
			} while (false);
		}break;
	case stProxyMsg2UserByOnlyId::_value:
		{
			stProxyMsg2UserByOnlyId *pdstcmd = (stProxyMsg2UserByOnlyId *)pcmd;
			if (pdstcmd->i64OnlyId!=0){
				CPlayerObj *pUser =m_playerhash.FindByOnlyId(pdstcmd->i64OnlyId);
				if (pUser){
					pUser->SendMsgToMe(&pdstcmd->msg[0],pdstcmd->msg.size);
				}
			}
		}break;
	case stProxyMsg2ByType_ID::_value:
		{
			stProxyMsg2ByType_ID* pdstcmd=(stProxyMsg2ByType_ID*)pcmd;
			switch (pdstcmd->btIdType)
			{
			case PROXY_TYPE_GUILD:
				{
				}break;
			}
		}break;
	case stNotifyExecScript::_value:
		{
			stNotifyExecScript *pdstcmd = (stNotifyExecScript *)pcmd;
			if (pdstcmd->szScript[0]!=0 && CUserEngine::getMe().m_scriptsystem.m_LuaVM){
				CPlayerObj *pUser =NULL;
				if(pdstcmd->szSrcName[0]!=0){
					pUser=m_playerhash.FindByName(pdstcmd->szSrcName);
				}
				if(!pUser && pdstcmd->i64OnlyId){
					pUser=m_playerhash.FindByOnlyId(pdstcmd->i64OnlyId);
				}
				if(pUser){
					stAutoSetScriptParam autoparm(pUser);
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(&pdstcmd->szScript[0]);
				}else{
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(&pdstcmd->szScript[0]);
				}
			}
		}break;
	case stSuperScriptSqlRetFunc::_value:
		{
			stSuperScriptSqlRetFunc* pDstCmd=(stSuperScriptSqlRetFunc*)pcmd;
			if (pDstCmd && CUserEngine::getMe().m_scriptsystem.m_LuaVM){
				CPlayerObj* pCurPlayer=CUserEngine::getMe().m_playerhash.FindByOnlyId(pDstCmd->SqlRetFunc.SqlStatement.onlyId);
				stAutoSetScriptParam autoparam(pCurPlayer);
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(pDstCmd->SqlRetFunc.SqlStatement.szRetFunc,pDstCmd->SqlRetFunc.btRetFuncErrorType,pDstCmd->SqlRetFunc.nCount);
			}
		}break;
	case stSuperScriptSqlRetFuncSelect::_value:
		{
			stSuperScriptSqlRetFuncSelect* pDstCmd=(stSuperScriptSqlRetFuncSelect*)pcmd;
			if(pDstCmd && CUserEngine::getMe().m_scriptsystem.m_LuaVM){
				CPlayerObj* pCurPlayer=CUserEngine::getMe().m_playerhash.FindByOnlyId(pDstCmd->SqlRetFunc.SqlStatement.onlyId);
				stAutoSetScriptParam autoparam(pCurPlayer);
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(pDstCmd->SqlRetFunc.SqlStatement.szRetFunc,pDstCmd->SqlRetFunc.btRetFuncErrorType,pDstCmd->SqlRetFunc.nCount,pDstCmd->selectdata);
			}
		}break;
	case stCretChat::_value:
		{
			stCretChat* pchatcmd=(stCretChat*)pcmd;
			DWORD dwZoneId=HIWORD(pchatcmd->dwZoneId);
			DWORD dwTradeId=LOWORD(pchatcmd->dwZoneId);
			if(dwZoneId && (dwZoneId!=GameService::getMe().m_nZoneid || pchatcmd->i64DestOnlyId!=0)){
				switch (pchatcmd->btChatType)
				{
				case CHAT_TYPE_PRIVATE:
					{
						CPlayerObj* pToPlayer=m_playerhash.FindByName(pchatcmd->szTargetName);
						if(pToPlayer){
							if(!(pToPlayer->m_dwUserConfig & USERCONFIG_CANPRIVITECHAT)){
								BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
								CChat::SETSENDCMD(retcmd,CHAT_TYPE_SYSTEM,vformat(GameService::getMe().GetStrRes(RES_LANG_USERCONFIGNOTPRIVITE),pToPlayer->getName()),pchatcmd->szName,NULL,0);
								BroadcastGameSvr(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize(),LODWORD(pchatcmd->i64SrcOnlyId),true,dwZoneId,dwTradeId);
								return;
							}
							pToPlayer->SendMsgToMe(pchatcmd,sizeof(*pchatcmd)+pchatcmd->szZeroChatMsg.getarraysize());
							pchatcmd->dwVip=pToPlayer->getVipType();
							pchatcmd->dwGuildId = pToPlayer->m_GuildInfo.dwGuildId;
							BroadcastGameSvr(pchatcmd,sizeof(*pchatcmd)+pchatcmd->szZeroChatMsg.getarraysize(),LODWORD(pchatcmd->i64SrcOnlyId),true,dwZoneId,dwTradeId);
						}else{
							dwZoneId=HIWORD(HIDWORD(pchatcmd->i64DestOnlyId));
							dwTradeId=LOWORD(HIDWORD(pchatcmd->i64DestOnlyId));
							if(dwZoneId && dwZoneId!=GameService::getMe().m_nZoneid){
								BroadcastGameSvr(pchatcmd,sizeof(*pchatcmd)+pchatcmd->szZeroChatMsg.getarraysize(),LODWORD(pchatcmd->i64DestOnlyId),true,dwZoneId,dwTradeId);
							}
						}
					}break;
				}
			}
		}break;
	case stZoneList::_value:
		{
			stZoneList* pSrcCmd = (stZoneList*)pcmd;
			AILOCKT(CUserEngine::getMe().m_scriptsystem.m_LuaVM->m_luacalllock);
			lua_State* L = CUserEngine::getMe().m_scriptsystem.m_LuaVM->GetHandle();
			if (L) {
				sol::state_view lua(L);
				sol::table table = lua.create_table();
				if (pSrcCmd->msg.size > 0) {
					for (int i = 0; i < pSrcCmd->msg.size; i++) {
						sol::table subtable = lua.create_table();
							stZone info = pSrcCmd->msg[i];
							subtable["tZoneid"] = info.ZoneId;
							subtable["ZoneIndex"] = info.ZoneIndex;
							subtable["szkaiqu_time"] = info.szkaiqu_time;
							table[info.ZoneId] = subtable;
					}
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("SetWorld", table);
				}
			}
		}break;	
	case stGameFindPlayerByNameRet::_value:
		{
			stGameFindPlayerByNameRet* pdstcmd = (stGameFindPlayerByNameRet*)pcmd;
			if(pdstcmd->btErrorCode == 1){
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("CrossRankNumberOne", pdstcmd->szName);
			}
		}break;
	case stAuctionPlayerOnlyid::_value:
		{
			stAuctionPlayerOnlyid* pCmd = (stAuctionPlayerOnlyid*)pcmd;
			auto& aucPlayer = m_aucPlayer[pCmd->index];
			for(auto i = 0; i < pCmd->onlyids.size; i++){
				aucPlayer.insert(pCmd->onlyids[i]);
			}
		}break;
	}

}

void CUserEngine::doGlobalSvrCmd(stBaseCmd* pcmd, unsigned int ncmdlen){
	FUNCTION_BEGIN;
	switch(pcmd->value)
	{
	case stGlobalPlayerGroupChange::_value:
		{
			stGlobalPlayerGroupChange* pDstCmd=(stGlobalPlayerGroupChange*)pcmd;
			CPlayerObj* pUser=m_playerhash.FindByOnlyId(pDstCmd->i64OnlyId);
			if(pUser){
				auto groupIdChange = pUser->m_GroupInfo.dwGroupId!= pDstCmd->groupinfo.dwGroupId;
				auto groupMasterChange = pUser->m_GroupInfo.boMaster != pDstCmd->groupinfo.boMaster;
				if(pUser->GetEnvir()){
					if(pDstCmd->groupinfo.dwGroupId){
						pUser->m_GroupInfo=pDstCmd->groupinfo;
						pUser->GetEnvir()->mapGroupAddNum(pUser);
						pUser->SendAllPosToGroupMember();
					}else{
						pUser->GetEnvir()->mapGroupRemoveNum(pUser);
						pUser->m_GroupInfo=pDstCmd->groupinfo;
						pUser->SendAllPosToGroupMember();
					}
				}else{
					pUser->m_GroupInfo=pDstCmd->groupinfo;
				}
				pUser->m_cGroupMemberHash.clear();
				for (int i = 0; i < pDstCmd->members.size; i++) {
					pUser->m_cGroupMemberHash.push(pDstCmd->members[i].i64CretOnlyId, pDstCmd->members[i]);
				}
				pUser->TriggerEvent(pUser,INGROUP,1);
				if (groupIdChange) pUser->UpdateAppearance(FeatureIndex::group, pUser->m_GroupInfo.dwGroupId);
				if (groupMasterChange) pUser->UpdateAppearance(FeatureIndex::isGroupMaster, pUser->m_GroupInfo.boMaster);
				pUser->StatusValueChange(stCretStatusValueChange::hp, 0, "groupchange", true);
			}
		}break;
	case stProxyMsg2OneGlobalSrv::_value:
		{
			stProxyMsg2OneGlobalSrv *pSrcCmd = (stProxyMsg2OneGlobalSrv *)pcmd;
			CPlayerObj *pUser = m_playerhash.FindByName(pSrcCmd->szName);
			if(pUser)
			{
				stBaseCmd* pBaseCmd = (stBaseCmd*)pSrcCmd->msg.getptr();
				switch(pBaseCmd->value)
				{
				case stCretChat::_value:
					{
						stCretChat *pChat = (stCretChat *)pBaseCmd;
						if(pChat->btChatType == CHAT_TYPE_PRIVATE){
							if(!(pUser->m_dwUserConfig & USERCONFIG_CANPRIVITECHAT)){
								CChat::sendSystem(pChat->szName, GameService::getMe().GetStrRes(RES_LANG_USERCONFIGNOTPRIVITE),pUser->getName());
								return;
							}
						}
					}break;
				case strGlobalGuildSendFightList::_value:
					{
						strGlobalGuildSendFightList* pdstcmd=(strGlobalGuildSendFightList*)pBaseCmd;
						pUser->m_WarGuildSet.clear();
						for(int i=0;i<pdstcmd->guilds.size;i++){
							pUser->m_WarGuildSet.insert(pdstcmd->guilds[i]);
						}
						return;
					}break;
				case strGlobalGuildSendAllianceList::_value:
					{
						strGlobalGuildSendAllianceList* pdstcmd=(strGlobalGuildSendAllianceList*)pBaseCmd;
						pUser->m_AllianceGuildSet.clear();
						for(int i=0;i<pdstcmd->guilds.size;i++){
							pUser->m_AllianceGuildSet.insert(pdstcmd->guilds[i]);
						}
						return;
					}break;
				case stCreatGameGuild::_value:
					{
						stCreatGameGuild* pdstcmd=(stCreatGameGuild*)pBaseCmd;
						CPlayerObj *pUser = m_playerhash.FindByOnlyId(pdstcmd->i64OnlyId);

						if (pUser)
						{
							bool huanghui = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("IsGuildCreateEnable", 0, pUser, pdstcmd->szGuildName);
							if(huanghui){
								CUserEngine::getMe().SendMsg2GlobalSvr(pdstcmd,sizeof(stCreatGameGuild));
							}
						}
						return;
					}break;
				case stGuildChangeNotice::_value:
					{
						auto* pSrcCmd = (stGuildChangeNotice*)pBaseCmd;
						CPlayerObj* pUser = m_playerhash.FindByOnlyId(pSrcCmd->i64OnlyId);

						if (pUser)
						{
							bool IsCan = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("IsGuildNoticeEnable", 0, pUser, pSrcCmd->btType, pSrcCmd->szGuildNotice);
							if (IsCan) {
								CUserEngine::getMe().SendMsg2GlobalSvr(pSrcCmd, sizeof(stGuildChangeNotice));
							}
						}
						return;
					}break;
				case stGuildEmblemActive::_value:
					{
						auto* pSrcCmd = (stGuildEmblemActive*)pBaseCmd;
						CPlayerObj* pUser = m_playerhash.FindByOnlyId(pSrcCmd->i64OnlyId);

						if (pUser)
						{
							bool IsCan = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("IsEmbledActive", 0, pUser, pSrcCmd->dwEmblemId);
							if (IsCan) {
								CUserEngine::getMe().SendMsg2GlobalSvr(pSrcCmd, sizeof(*pSrcCmd));
							}
						}
						return;
					}break;
				case stGloalGuildEventsFetchRet::_value:
					{
						stGloalGuildEventsFetchRet* pDstCmd = (stGloalGuildEventsFetchRet*)pBaseCmd;
						do
						{
							AILOCKT(CUserEngine::getMe().m_scriptsystem.m_LuaVM->m_luacalllock);
							lua_State* L = CUserEngine::getMe().m_scriptsystem.m_LuaVM->GetHandle();
							if (!L) {
								break;
							}
							if (pDstCmd->szRetFunc[0] == 0) {
								break;
							}
							CPlayerObj* pUser = m_playerhash.FindByOnlyId(pDstCmd->i64OnlyId);
							if (!pUser) {
								break;
							}
							sol::state_view lua(L);
							sol::table tabEvents = lua.create_table();
							int tableIndex = 1;
							for (auto i = 0; i < pDstCmd->vGuildEvent.size; i++)
							{
								sol::table tabEvent = lua.create_table();
								stGuildEventEx& eventInfo = pDstCmd->vGuildEvent[i];
								tabEvent["id"] = eventInfo.dwId;
								tabEvent["guildId"] = eventInfo.dwGuildId;
								tabEvent["eventTime"] = eventInfo.dwEventTime;
								tabEvent["eventText"] = GTU(eventInfo.szEventText);
								tabEvent["eventType"] = eventInfo.dwEventType;
								tabEvents[tableIndex++] = tabEvent;
							}
							CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall(pDstCmd->szRetFunc, pUser, tabEvents);
						} while (false);
					}
				}
				pUser->SendMsgToMe(pSrcCmd->msg.getptr(),pSrcCmd->msg.getarraysize());
			}
		}break;
	case stGlobalGetGroupPlayerRet::_value:
		{
			stGlobalGetGroupPlayerRet* pDstCmd=(stGlobalGetGroupPlayerRet*)pcmd;
			CPlayerObj* pUser=m_playerhash.FindByOnlyId(pDstCmd->i64OnlyId);
			if(pUser && pDstCmd->szRetFunc[0]!=0){
				if (CUserEngine::getMe().m_scriptsystem.m_LuaVM){
					stAutoSetScriptParam autoparam(pUser);
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(vformat(pDstCmd->szRetFunc,(double)pDstCmd->i64GroupPlayers[0],
						(double)pDstCmd->i64GroupPlayers[1],
						(double)pDstCmd->i64GroupPlayers[2],
						(double)pDstCmd->i64GroupPlayers[3],
						(double)pDstCmd->i64GroupPlayers[4],
						(double)pDstCmd->i64GroupPlayers[5],
						(double)pDstCmd->i64GroupPlayers[6],
						(double)pDstCmd->i64GroupPlayers[7])
						);
				}
			}
		}break;
	case stGlobalGuildChangeGuild::_value:
		{
			stGlobalGuildChangeGuild* pSrcCmd = (stGlobalGuildChangeGuild*)pcmd;
			CPlayerObj* pUser = m_playerhash.FindByOnlyId(pSrcCmd->i64OnlyId);
			if(pUser){
				stGSGuildInfo oldGuildInfo = pUser->m_GuildInfo;
				pUser->m_GuildInfo = pSrcCmd->guildinfo;
				if(oldGuildInfo.dwGuildId != pUser->m_GuildInfo.dwGuildId || 
					oldGuildInfo.dwPowerLevel != pUser->m_GuildInfo.dwPowerLevel ||
					oldGuildInfo.dwEmblemId != pUser->m_GuildInfo.dwEmblemId){
					pUser->UpdateAppearance(FeatureIndex::guild, pUser->m_GuildInfo.dwGuildId);
					pUser->UpdateAppearance(FeatureIndex::emblem, pUser->m_GuildInfo.dwEmblemId);
				}
				if (pSrcCmd->szScript[0] != 0)
				{
					stAutoSetScriptParam autoparam(pUser);
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(pSrcCmd->szScript.getptr());
				}

				if (oldGuildInfo.dwGuildId != pUser->m_GuildInfo.dwGuildId) {
					stAutoSetScriptParam autoparam(pUser);
					if (pUser->m_GuildInfo.dwGuildId > 0) {
						pUser->SendGroupChangeInfo(1, pUser->m_GuildInfo.dwGuildId, pUser->m_GuildInfo.szGuildName);
					}else {
						pUser->SendGroupChangeInfo(0, oldGuildInfo.dwGuildId, oldGuildInfo.szGuildName);
					}
				}
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("playerguildchange",oldGuildInfo,pUser->m_GuildInfo);
			}
			else {
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("playerguildkickout", (double)pSrcCmd->i64OnlyId);
			}
		}break;
	case stSendFriendMsgSuperSrv::_value:
		{
			stSendFriendMsgSuperSrv *pSrcCmd = (stSendFriendMsgSuperSrv *)pcmd;
			CPlayerObj *pUser = m_playerhash.FindByOnlyId(pSrcCmd->i64SrcOnlyId);
			if (pUser) pUser->doRelationFromSuperSvr((stBaseCmd*)pSrcCmd->msg.getptr(),pSrcCmd->msg.getarraysize());
		}break;	
	case stNotifyExecScript::_value:
		{
			stNotifyExecScript *pdstcmd = (stNotifyExecScript *)pcmd;
			if (pdstcmd->szScript[0]!=0 && CUserEngine::getMe().m_scriptsystem.m_LuaVM){
				CPlayerObj *pUser =NULL;
				if(pdstcmd->szSrcName[0]!=0){
					pUser=m_playerhash.FindByName(pdstcmd->szSrcName);
				}
				if(!pUser && pdstcmd->i64OnlyId){
					pUser=m_playerhash.FindByOnlyId(pdstcmd->i64OnlyId);
				}
				if(pUser){
					stAutoSetScriptParam autoparm(pUser);
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(&pdstcmd->szScript[0]);
				}else{
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(&pdstcmd->szScript[0]);
				}
			}
		}break;
	case stGlobalVarsRet::_value:
		{
			stGlobalVarsRet* pSrcCmd=(stGlobalVarsRet*)pcmd;
			if(pSrcCmd->btError==0){
				switch(pSrcCmd->btType){
				case 0:
				case 1:
					{
						GlobalVars::getMe().save(pSrcCmd->szVarName,pSrcCmd->szVarValue);
					}break;
				case 2:
					{
						GlobalVars::getMe().del(pSrcCmd->szVarName);
					}break;
				}
			}
			if(pSrcCmd->boThisSvr && pSrcCmd->szVarRetFuc[0]!=0 && CUserEngine::getMe().m_scriptsystem.m_LuaVM){
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(pSrcCmd->szVarRetFuc,pSrcCmd->btError,pSrcCmd->szVarValue);
			}
		}break;;
	case stSynsGlobalVars::_value:
		{
			stSynsGlobalVars* pdstcmd = (stSynsGlobalVars*)pcmd;
			for(int i = 0; i < pdstcmd->sysvars.size; i++){
				GlobalVars::getMe().save(pdstcmd->sysvars[i].szVarName, pdstcmd->sysvars[i].szVarValue);
			}
		}break;
	case stGlobalGuildGameSvrGetGuildRet::_value:
		{
			stGlobalGuildGameSvrGetGuildRet* pdstcmd=(stGlobalGuildGameSvrGetGuildRet*)pcmd;
			m_mGuilds.clear();
			for(int i=0;i<pdstcmd->guilds.size;i++){
				m_mGuilds[pdstcmd->guilds[i].dwGuildId]=pdstcmd->guilds[i];
			}
		}break;
	case stUpdateGlobalGuildToGameSvr::_value:
		{
			stUpdateGlobalGuildToGameSvr* pdstcmd = (stUpdateGlobalGuildToGameSvr*)pcmd;
			if (!pdstcmd->btFlag) {
				m_mGuilds[pdstcmd->guild.dwGuildId] = pdstcmd->guild;
			}
			else {
				m_mGuilds.erase(pdstcmd->guild.dwGuildId);
			}
		}break;
	case stSvrSendGMCmd::_value:
		{
			stSvrSendGMCmd* sendcmd=(stSvrSendGMCmd*)pcmd;
			stGmCmd curcmd;
			curcmd.btGmLv=sendcmd->lv;
			strcpy_s(curcmd.szName,_MAX_NAME_LEN_-1,sendcmd->szPlayerName);
			strncpy_s(curcmd.szCmdStr,_MAX_RETCMD_LEN_-1,sendcmd->stSendStr.getptr(),sendcmd->stSendStr.getarraysize());
			do {
				AILOCKT(CUserEngine::getMe().m_cmdstrlist);
				CUserEngine::getMe().m_cmdstrlist.push_back(curcmd);
			} while (false);
		}break;
	case stUserSend2GGSvr::_value:
		{
		}break;
	case stCretChat::_value:
		{
			stCretChat* pchatcmd =  (stCretChat*)pcmd;
			switch(pchatcmd->btChatType)
			{
			case CHAT_TYPE_CLAN:
				{
					if(pchatcmd->i64DestOnlyId!=0 && pchatcmd->dwGuildId){
						DWORD dwZoneid=HIWORD(HIDWORD(pchatcmd->i64DestOnlyId));
						DWORD dwTradeId=LOWORD(HIDWORD(pchatcmd->i64DestOnlyId));
						if(dwZoneid && dwZoneid!=GameService::getMe().m_nZoneid){
							pchatcmd->dwZoneId=0;
							BroadcastGameSvr(pchatcmd,sizeof(*pchatcmd)+pchatcmd->szZeroChatMsg.getarraysize(),LODWORD(pchatcmd->i64DestOnlyId),true,dwZoneid,dwTradeId);
						}
					}
				}break;
			}
		}break;
	case stCreteRoleYaoQingMaCmdRet::_value:
	{
		stCreteRoleYaoQingMaCmdRet* pdstcmd = (stCreteRoleYaoQingMaCmdRet*)pcmd;
		GameService::instance()->m_loginsvrconnter->sendcmd(pdstcmd, sizeof(stCreteRoleYaoQingMaCmdRet));
	}break;
	case stGlobalGuildMembersFetchRet::_value:
		{
			stGlobalGuildMembersFetchRet* pDstCmd = (stGlobalGuildMembersFetchRet*)pcmd;
			DWORD dwGuildID = pDstCmd->dwGuildId;
			__int64 matserId = pDstCmd->i64Masterid;
			do 
			{
				AILOCKT(CUserEngine::getMe().m_scriptsystem.m_LuaVM->m_luacalllock);
				lua_State* L = CUserEngine::getMe().m_scriptsystem.m_LuaVM->GetHandle();
				if (!L) {
					break;
				}
				if (pDstCmd->MemberInfos.size <= 0) {
					break;
				}
				sol::state_view lua(L);
				sol::table tabMembers = lua.create_table();
				int tableIndex = 1;
				for (auto i = 0; i < pDstCmd->MemberInfos.size; i++)
				{
					sol::table tabMember = lua.create_table();
					stGuildMemberInfo& memberInfo = pDstCmd->MemberInfos[i];
					tabMember["onlyid"] = (double)memberInfo.i64UserOnlyId;
					tabMember["name"] = memberInfo.szName;
					tabMember["alianame"] = memberInfo.szAliaName;
					tabMember["job"] = memberInfo.btJob;
					tabMember["sex"] = memberInfo.btSex;
					tabMember["level"] = memberInfo.dwLevel;
					tabMember["zslevel"] = memberInfo.dwZsLevel;
					tabMember["guildpowerlvl"] = memberInfo.dwGuildPowerLvl;
					tabMember["online"] = memberInfo.boOnline;
					tabMember["viplvl"] = memberInfo.btViplvl;
					tabMember["intime"] = memberInfo.tInTime;
					tabMember["rank"] = memberInfo.dwRank;
					tabMember["builddegree"] = memberInfo.dwBuildDegree;
					tabMember["offlinetime"] = memberInfo.dwOfflineTime;
					tabMembers[tableIndex++] = tabMember;
				}
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("LuaGuildMembersFetchRet", dwGuildID, pDstCmd->szParam, (double)matserId, tabMembers);
			} while (false);
			
		}break;
	case stGlobalGuildMasterShow::_value:
		{
			stGlobalGuildMasterShow* pdstcmd = (stGlobalGuildMasterShow*)pcmd;
			if(CUserEngine::getMe().m_scriptsystem.m_LuaVM){
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("LuaGetMasterInfoByID",pdstcmd->dwGuildId,pdstcmd->szGuildName,pdstcmd->szName,(double)pdstcmd->i64OnlyId,pdstcmd->btSex,pdstcmd->btJob,pdstcmd->dwType);
			}
		}break;
	case stOfflineTimeRet::_value:
		{
			stOfflineTimeRet* pdstcmd = (stOfflineTimeRet*)pcmd;
			if(CUserEngine::getMe().m_scriptsystem.m_LuaVM){
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("LuaGetOfflineTime",pdstcmd->offlinetime,pdstcmd->i64OnlyId,pdstcmd->szName);
			}
		}break;
	}
}



void CUserEngine::doTencentApiSvrCmd(stBaseCmd* pcmd,unsigned int ncmdlen){
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case stOpenApiWeiXingActivityCardId::_value:
	{
		stOpenApiWeiXingActivityCardId* pdstcmd = (stOpenApiWeiXingActivityCardId*)pcmd;
		if (pdstcmd->nErrorCode == 408) {

			}
	}break;
	}
}

void CUserEngine::doProxyMsgRet(stBaseCmd* pcmd, unsigned int ncmdlen,stBaseCmd* ppluscmd,int npluscmdlen){
	switch (ppluscmd->value)
	{
	case stPreChangeGameSvrCmd::_value:
		{
			stPreChangeGameSvrCmd *pdstcmd = (stPreChangeGameSvrCmd *)ppluscmd;
			CPlayerObj *pUser = m_playerhash.FindByOnlyId(pdstcmd->oldPlayerOnlyId);
			if ( pUser && pUser->getSwitchSvrInfo() ){
				if(m_dwThisGameSvrVersion==pdstcmd->dwGameSvrVersion){
					stSpaveMoveInfo* pSpaveMoveInfo=pUser->getSwitchSvrInfo();
					if ( pSpaveMoveInfo->space_move_tick!=pdstcmd->space_move_tick ){
						return;
					}else{
						pUser->getSwitchSvrInfo()->btMoveState=stSpaveMoveInfo::_SPACEMOVE_FINISH_;
						pUser->setSwitchSvrInfo(NULL);
						g_logger.debug( "源服务器: onlyId= %I64d 跨服换地图失败,proxymsg转发失败!",pdstcmd->oldPlayerOnlyId);
					}
				}
				else
				{
					pUser->getSwitchSvrInfo()->btMoveState=stSpaveMoveInfo::_SPACEMOVE_FINISH_;
					pUser->setSwitchSvrInfo(NULL);
					g_logger.error("服务器版本不匹配,当前版本 %s,%d区 源服务器版本 %s",timetostr(m_dwThisGameSvrVersion.load()),pdstcmd->dwSrcZoneId,timetostr(pdstcmd->dwGameSvrVersion));
				}
			}
		}break;
	case stSvrChangeGameSvrCmd::_value:
		{
			stSvrChangeGameSvrCmd *pdstcmd = (stSvrChangeGameSvrCmd *)ppluscmd;
			CPlayerObj *pUser = m_playerhash.FindByOnlyId(pdstcmd->oldPlayerOnlyId);
			if ( pUser && pUser->getSwitchSvrInfo() ){
				stSpaveMoveInfo* pSpaveMoveInfo=pUser->getSwitchSvrInfo();
				if ( pSpaveMoveInfo	&& pSpaveMoveInfo->btMoveState==stSpaveMoveInfo::_SPACE_WAIT_SWITCHSVR_ ){
					pUser->getSwitchSvrInfo()->btMoveState=stSpaveMoveInfo::_SPACEMOVE_FINISH_;
					g_logger.debug( "源服务器: onlyId= %I64d 跨服换地图失败,proxymsg转发失败!",pdstcmd->oldPlayerOnlyId);
				}
			}
		}break;
	}
}

bool CUserEngine::isCrossSvr(DWORD dwZoneid/* =0 */){//当前区ID 65000-65535 的就是跨服服务器
	if(dwZoneid){
		if(dwZoneid==0xFFFFFFFF)return false;
		return (dwZoneid>=65000 && dwZoneid<=65535)?true:false;
	}else{
		return (GameService::getMe().m_nZoneid>=65000 && GameService::getMe().m_nZoneid<=65535)?true:false;
	}
}

void CUserEngine::SendSystemMail(int64_t i64Onlyid, const char* szName, const char* szTitle, const char* szNotice, CItem* pItem/* =NULL */){
	FUNCTION_BEGIN;
	SendSystemMail(i64Onlyid, szName, szTitle, szNotice, pItem ? &pItem->m_Item : nullptr);
}

void CUserEngine::SendSystemMail(int64_t i64Onlyid,const char* szName,const char* szTitle,const char* szNotice,stItem* item){
	FUNCTION_BEGIN;
	BUFFER_CMD(stMailSendNewMailInner,newMail,stBasePacket::MAX_PACKET_SIZE);
	newMail->MailDetail.dwGold = 0;
	if (item)
		newMail->MailDetail.ItemArr.push_back(*item,__FUNC_LINE__);
	strcpy_s(newMail->MailDetail.szReceiverName,_MAX_NAME_LEN_,szName);
	sprintf_s(newMail->MailDetail.szTitle,_MAX_MAIL_TITLE_LEN,szTitle);
	sprintf_s(newMail->MailDetail.szNotice,_MAX_MAILNOTICE_LEN,szNotice);

	CUserEngine::getMe().SendMailMsg2Super(newMail, sizeof(*newMail) + newMail->MailDetail.ItemArr.getarraysize(), CUserEngine::getMe().isCrossSvr() ? i64Onlyid : 0xFFFFFFFFFFFFFFFF);
}

void CUserEngine::SendSystemMailByIDAndCount(int64_t i64Onlyid, const char* szName, const char* szTitle, const char* szNotice, DWORD dwBaseid, DWORD dwCount, int frommapid, const char* bornfrom, const char* szmaker) {
	FUNCTION_BEGIN;
	BUFFER_CMD(stMailSendNewMailInner, newMail, stBasePacket::MAX_PACKET_SIZE);
	newMail->MailDetail.i64ReceiverID = i64Onlyid;
	strcpy_s(newMail->MailDetail.szReceiverName, _MAX_NAME_LEN_, szName);
	sprintf_s(newMail->MailDetail.szTitle, _MAX_MAIL_TITLE_LEN, szTitle);
	sprintf_s(newMail->MailDetail.szNotice, _MAX_MAILNOTICE_LEN, szNotice);
	newMail->MailDetail.btGoldType = 0;
	newMail->MailDetail.dwGold = 0;
	if (auto pItemLoadBase = sJsonConfig.GetItemDataById(dwBaseid)) {
		DWORD dwMaxCount = max(pItemLoadBase->dwMaxCount, pItemLoadBase->nVariableMaxCount);
		if (dwCount <= dwMaxCount) {
			stItem* pItem = GetMailedItem(dwBaseid, dwCount, 0, frommapid, bornfrom, szmaker);
			if (pItem) {
				newMail->MailDetail.ItemArr.push_back(*pItem, __FUNC_LINE__);
				PushMailedItem(pItem);
			}
		}
		else{
			DWORD dwSplitcnt = floor(dwCount / dwMaxCount);
			DWORD dwLeftCnt = floor(dwCount % dwMaxCount);
			for (DWORD i = 0; i < dwSplitcnt; i++){
				stItem* pItem = GetMailedItem(dwBaseid, dwMaxCount, 0, frommapid, bornfrom, szmaker);
				if (pItem) {
					newMail->MailDetail.ItemArr.push_back(*pItem, __FUNC_LINE__);
					PushMailedItem(pItem);
				}
				if (dwLeftCnt){
					stItem* pItem = GetMailedItem(dwBaseid, dwLeftCnt, 0, frommapid, bornfrom, szmaker);
					if (pItem) {
						newMail->MailDetail.ItemArr.push_back(*pItem, __FUNC_LINE__);
						PushMailedItem(pItem);
					}
				}
			}
		}

	CUserEngine::getMe().SendMailMsg2Super(newMail, sizeof(*newMail) + newMail->MailDetail.ItemArr.getarraysize(), CUserEngine::getMe().isCrossSvr() ? i64Onlyid : 0xFFFFFFFFFFFFFFFF);
	}
}

DWORD CUserEngine::GetDiffDayNow(DWORD dwTime){
	time_t lasttime=dwTime;
	time_t nowtime=time(NULL);
	tm lasttm=*localtime(&lasttime);
	tm nowtm=*localtime(&nowtime);
	DWORD lastday=lasttm.tm_year+1990+lasttm.tm_mon+1+lasttm.tm_mday;
	DWORD nowday=nowtm.tm_year+1990+nowtm.tm_mon+1+nowtm.tm_mday;
	return nowday-lastday;
}

void CUserEngine::sendLuaError()
{
	FUNCTION_BEGIN;
	FUNCTION_MONITOR(32,"sendLuaError");
	bool boSendLog = false;
	do 
	{
		AILOCKT(CLuaVM::ErrorlstLock);
		if(m_scriptsystem.m_LuaVM->m_Errorlst.size()>0){
			do 
			{
				AILOCKT(m_vGetLuaErrorPlayer);
				for(int i = 0;i<m_vGetLuaErrorPlayer.size();i++){
					CPlayerObj* pPlayer = m_playerhash.FindByOnlyId(m_vGetLuaErrorPlayer[i]);
					if(pPlayer){
						for(int j = 0;j<m_scriptsystem.m_LuaVM->m_Errorlst.size();j++){
							CChat::getMe().sendSystem(pPlayer,"lua error:: %s",m_scriptsystem.m_LuaVM->m_Errorlst[j].c_str());
							if(boSendLog == false){
								std::string strerror = m_scriptsystem.m_LuaVM->m_Errorlst[j];
								std::string::size_type pos = 0;
								while( (pos = strerror.find("\'", pos)) != std::string::npos){
									strerror.replace(pos, 1, " ");
									pos += 1;
								}
								GameService::getMe().Send2LogSvr(_SERVERLOG_LUAERROR,0,0,NULL,"'lua error',\'%s\'",
									strerror.c_str());
							}
						}
						boSendLog = true;
					}
				}
			} while (false);
			

			if(boSendLog == false){
				for(int j = 0;j<m_scriptsystem.m_LuaVM->m_Errorlst.size();j++){
					std::string strerror = m_scriptsystem.m_LuaVM->m_Errorlst[j];
					std::string::size_type pos = 0;
					while( (pos = strerror.find("\'", pos)) != std::string::npos){
						strerror.replace(pos, 1, " ");
						pos += 1;
					}
					GameService::getMe().Send2LogSvr(_SERVERLOG_LUAERROR,0,0,NULL,"'lua error',\'%s\'",
						strerror.c_str());
				}
			}

			m_scriptsystem.m_LuaVM->m_Errorlst.clear();
		}
	} while (false);
}
void CUserEngine::ReleasePItem(CItem *pItem,const char* szReleaseFrom){		//须删除的的物品加入缓冲
	FUNCTION_BEGIN;
	if(pItem){
		static zLogger::zLevel tmploglvl(zLogger::zDEBUG.name,zLogger::zDEBUG.writelevel,6,zLogger::zDEBUG.realtimewrite,zLogger::zDEBUG.showcolor);
		g_logger.log(tmploglvl,"删除物品对象 %s %.8x %I64d %s",pItem->GetItemName(),pItem,pItem->m_Item.i64ItemID,szReleaseFrom);
		m_itemPool.destroy(pItem);
	}
}

stItem* CUserEngine::GetMailedItem()
{
	stItem* item = NULL;
	if(m_mailedstItems.size())
	{
		item = m_mailedstItems.front();
		m_mailedstItems.pop_front();
	}else
	{
		item = CLD_DEBUG_NEW stItem;
	}
	if(item){
		item->Clear();
		return item;
	}
	return NULL;
}
stItem* CUserEngine::GetMailedItem(DWORD dwBaseid, DWORD dwCount, BYTE binding, int frommapid, const char* bornfrom, const char* szmaker) {
	if (auto pItemLoadBase = sJsonConfig.GetItemDataById(dwBaseid)) {
		stItem* pItem = GetMailedItem();
		if (pItem) {
			pItem->dwBaseID = dwBaseid;
			pItem->dwBinding = binding;
			pItem->btBornFrom = _CREATE_MAIL;
			pItem->dwCount = dwCount;
			pItem->BornFromMapid = frommapid;
			strcpy_s(pItem->bornfrom, sizeof(pItem->bornfrom) - 1, bornfrom);
			strcpy_s(pItem->szMaker, sizeof(pItem->szMaker) - 1, szmaker);
			pItem->borntime = (DWORD)time(NULL);
			pItem->nDura = pItemLoadBase->dwMaxDura;
			pItem->nMaxDura = pItemLoadBase->dwMaxDura;
			if (pItemLoadBase->btItemLimitTimeType == 1)
			{
				pItem->dwExpireTime = (DWORD)time(NULL) + pItemLoadBase->dwItemLimitTime;
			}
			else
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
	return nullptr;
}

void CUserEngine::PushMailedItem(stItem* item)
{
	if(item)
	{
		m_mailedstItems.push_back(item);
	}
}
	
void CUserEngine::ClearMailedItem()
{
	for (auto it = m_mailedstItems.begin(); it != m_mailedstItems.end(); it++) {
		stItem* ptmpItem = *it;
		if (ptmpItem) {
			SAFE_DELETE(ptmpItem);
		}
	}
	m_mailedstItems.clear();
}

void CUserEngine::ClearMagicPoint()
{
	m_mapMagicPoint.clear();
	m_mapMagicPointRand.clear();
}

void CUserEngine::SetMagicPoint(int nMagicID, const char* pSrc)
{
	std::string str = pSrc;
	CEasyStrParse cesp;
	CEasyStrParse subcesp;	
	cesp.SetParseStr((char*)str.c_str(), "=");
	for (int i = 0; i < cesp.ParamCount(); i++)
	{
		subcesp.SetParseStr(cesp[i], "+");
		if (subcesp.ParamCount() == 5)
		{
			AddOneMagicPoint(nMagicID, atoi(subcesp[0]), atoi(subcesp[1]), atoi(subcesp[2]), atoi(subcesp[3]), atoi(subcesp[4]));
		}
		else
		{
			g_logger.error("SetMagicPoint错误，技能id:%d,第%d个,%s", nMagicID, i, __FUNC_LINE__);
		}
	}
}

void CUserEngine::AddOneMagicPoint(int nMagicID, int nRound, int nxMin, int nxMax, int nyMin, int nYmax)
{
	if (nxMin > nxMax || nyMin > nYmax)
	{
		g_logger.error("AddOneMagicPoint错误，技能id:%d,轮次%d个坐标大小错误,%s", nMagicID, nRound, __FUNC_LINE__);
		return;
	}
	std::map<int, std::map<int, stMagicPoint> >::iterator it_Big = m_mapMagicPoint.find(nMagicID);
	if (it_Big == m_mapMagicPoint.end())
	{
		std::map<int, stMagicPoint> tmpmap;
		m_mapMagicPoint.insert(std::pair<int, std::map<int, stMagicPoint> > (nMagicID, tmpmap));
		it_Big = m_mapMagicPoint.find(nMagicID);
	}
	if (it_Big == m_mapMagicPoint.end())
	{
		return;
	}
	std::map<int, stMagicPoint>::iterator it = it_Big->second.find(nRound);
	if (it == it_Big->second.end())
	{
		stMagicPoint MagicPoint;
		MagicPoint.nMagicID = nMagicID;
		MagicPoint.nRound = nRound;
		MagicPoint.Pointxy.clear();
		it_Big->second.insert(std::pair<int, stMagicPoint>(MagicPoint.nRound, MagicPoint));
		it = it_Big->second.find(nRound);
	}
	if (it == it_Big->second.end())
	{
		return;
	}
	int x = 0;
	int y = 0;
	for (x = nxMin; x <= nxMax; x++)
	{
		for (y = nyMin; y <= nYmax; y++)
		{
			stPointxy onepoint;
			onepoint.x = x;
			onepoint.y = y;
			it->second.Pointxy.push_back(onepoint);
		}
	}	
}

stMagicPoint* CUserEngine::GetMagicPoint(int nMagicID, int nRound)
{
	std::map<int, std::map<int, stMagicPoint> >::iterator it_Big = m_mapMagicPoint.find(nMagicID);
	if (it_Big != m_mapMagicPoint.end())
	{
		std::map<int, stMagicPoint>::iterator it = it_Big->second.find(nRound);
		if (it != it_Big->second.end())
		{
			return &(it->second);
		}
	}	
	return NULL;
}

void CUserEngine::GetRandom(std::vector<int> RandSorce, int nRandNum, std::list<int>& Result)
{//RandSorce随机的范围，值是随机的id，nRandNum随机的数量，Result随机的结果
	Result.clear();
	int nRandStep = 0;

	int nRand = 0;
	std::vector<int>::iterator it;
	int i = 0;
	for (nRandStep = 0; nRandStep < nRandNum; nRandStep++)
	{
		nRand = _random(RandSorce.size(), 1);
		Result.push_back(RandSorce[nRand - 1]);

		i = 0;
		for (it = RandSorce.begin(); it != RandSorce.end(); it++)
		{
			i++;
			if (i == nRand)
			{
				RandSorce.erase(it);
				break;
			}
		}
	}
}

void CUserEngine::SetMagicPointRand(int nMagicID, int nRandNum, const char* pSrc)
{
	std::string str = pSrc;
	CEasyStrParse cesp;
	CEasyStrParse subcesp;
	cesp.SetParseStr((char*)str.c_str(), "=");
	for (int i = 0; i < cesp.ParamCount(); i++)
	{
		subcesp.SetParseStr(cesp[i], "+");
		if (subcesp.ParamCount() == 4)
		{
			AddOneMagicPointRand(nMagicID, nRandNum, atoi(subcesp[0]), atoi(subcesp[1]), atoi(subcesp[2]), atoi(subcesp[3]));
		}
		else
		{
			g_logger.error("SetMagicPoint错误，技能id:%d,第%d个,%s", nMagicID, i, __FUNC_LINE__);
		}
	}
}

void CUserEngine::AddOneMagicPointRand(int nMagicID, int nRandNum, int nxMin, int nxMax, int nyMin, int nYmax)
{
	if (nxMin > nxMax || nyMin > nYmax)
	{
		g_logger.error("AddOneMagicPointRand错误，技能id:%d,个坐标大小错误,%s", nMagicID, __FUNC_LINE__);
		return;
	}
	std::map<int, stMagicPointRand>::iterator it_Big = m_mapMagicPointRand.find(nMagicID);
	if (it_Big == m_mapMagicPointRand.end())
	{
		stMagicPointRand tmpmap;
		tmpmap.nRandNum = nRandNum;
		m_mapMagicPointRand.insert(std::pair<int, stMagicPointRand>(nMagicID, tmpmap));
		it_Big = m_mapMagicPointRand.find(nMagicID);
	}
	if (it_Big == m_mapMagicPointRand.end())
	{
		return;
	}
	
	int x = 0;
	int y = 0;
	int nSize = 0;
	for (x = nxMin; x <= nxMax; x++)
	{
		for (y = nyMin; y <= nYmax; y++)
		{
			stPointxy onepoint;
			onepoint.x = x;
			onepoint.y = y;
			nSize = it_Big->second.Pointxy.size() + 1;
			it_Big->second.Pointxy.insert(std::pair<int, stPointxy>(nSize, onepoint));
		}
	}
}

bool CUserEngine::GetMagicPointRand(int nMagicID, std::list<stPointxy>& Result)
{
	std::map<int, stMagicPointRand>::iterator it_Big = m_mapMagicPointRand.find(nMagicID);
	if (it_Big == m_mapMagicPointRand.end())
	{
		return false;
	}
	stMagicPointRand* pRand = &(it_Big->second);
	if (!pRand)
	{
		return false;
	}

	std::vector<int> RandSorce;
	RandSorce.clear();
	std::map<int, stPointxy>::iterator it;
	for (it = pRand->Pointxy.begin(); it != pRand->Pointxy.end(); it++)
	{
		RandSorce.push_back(it->first);
	}

	std::list<int> ResultID;
	ResultID.clear();
	GetRandom(RandSorce, pRand->nRandNum, ResultID);

	std::list<int>::iterator ih;
	
	//将结果压入map里面，方便下面罗列时查找
	std::map<int, bool> mapResultID;
	for (ih = ResultID.begin(); ih != ResultID.end(); ih++)
	{
		mapResultID[*ih] = true;
	}

	//最终的结果列表
	Result.clear();
	for (it = pRand->Pointxy.begin(); it != pRand->Pointxy.end(); it++)
	{
		if (mapResultID.find(it->first) != mapResultID.end())
		{
			Result.push_back(it->second);
		}
	}
	return true;
}

DWORD CUserEngine::GetSavedSharedData(SavedSharedData type)
{
	return m_shareData.data()->saveArrayData[static_cast<SharedData::U>(type)];
}

void CUserEngine::SetSavedSharedData(SavedSharedData type, uint32_t value)
{
	m_shareData.data()->saveArrayData[static_cast<SharedData::U>(type)] = value;
}


stAuction& CUserEngine::GetAuc(BYTE index)
{
	if (index == 1)
		return m_shareData.data()->auc;
	else
		return m_shareData.data()->aucSpecial;
}