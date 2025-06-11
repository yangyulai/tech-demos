#include "PlayerObj.h"
#include "UsrEngn.h"
#include "Chat.h"
#include "Config.h"
#include "JsonConfig.h"
#include "MonsterObj.h"
#include "server_cmd.h"
#include "../gamesvr.h"
#include "cmd/Quest_cmd.h"
#include "cmd/Guild_cmd.h"
#include "cmd/ARPG_Mail_cmd.h"
#include "cmd/consignment_cmd.h"
#include "cmd/GuildPackage.h"
#include "timeMonitor.h"
#include "NpcTrade.h"
#include "../dbsvrGameConnecter.h"
#include "cmd/TencentApi_cmd.h"
#include "MapMagicEvent.h"
#include "MapItemEvent.h"

static_assert(sizeof(stPetSvrData) == TEMP_DATA_LEN, "sizeof(stPetSvrData) != TEMP_DATA_LEN, 存档会有问题");

CPlayerObj::CPlayerObj(PosType x, PosType y, CGameGateWayUser* pGateUser, char* szName)
	: CCreature(szName, CRET_PLAYER, x, y), m_btSelUserState(_PLAYER_STATE_NONE_)
	, m_boIsSelOk(false)
	, m_addloadoktime(0)
	, m_pGateUser(pGateUser)
	, m_pTmpGamedata(NULL)
	, m_btReadyState(_READYSTATE_NONE_)
	, m_bFakePlayer(false)
	, m_boIsNewHum(false)
	, m_dwSaveCount(0)
	, m_dwAccountDataSaveCount(0)
	, m_Trade(this)
	, m_tiShi(this)
	, m_tuBiao(this)
	, m_Petm(this)
{
	FUNCTION_BEGIN;
	//没在逻辑线程调用 注意线程安全
	m_res.clear();
	m_dwCreateTime = time(NULL);
	m_dwLineOutTime = 0;
	m_dwLoginTime = 0;
	m_i64UserOnlyID = 0;
	m_szLegion[0] = 0;
	m_btKickState = NONE_KICK;
	m_dwDoKickTick = 0;
	m_dwDoKickTickCount = 0;
	m_boIsChangeSvr = false;
	m_btChangeSvrSpaceMoveType = 0;
	m_nViewRangeX = maxPlayerViewRangeX;
	m_nViewRangeY = maxPlayerViewRangeY;
	m_dwNextSaveRcdTime = time(NULL) + GameService::getMe().m_svrcfg.dwsavercdintervaltime; //下次保存数据的时间
	m_dwSaveCount = 0;
	m_dwAccountDataSaveCount = 0;
	m_pVisitNPC = NULL;
	m_dwChangeTrueZoneid = 0;
	m_boCanChangeName = false;
	m_szOldName[0] = 0;
	m_dwPlayerOnlineTime = 0; //角色在线时间
	m_dwAccountOnlineTime = 0; //账号在线时间
	m_dwLastfullSaveRefTime = time(NULL);
	m_dwVisitNpcIntervalTick = 0;
	m_nMoveIv = 0;
	m_dwChatModel = 0; //客户端聊天过滤限制
	m_dwUserConfig = 0xFFFFFFFF;
	m_btPkModel = 0;
	m_dwGold = 0;
	m_dwLastChargeCcyTime = 0; //最后一次充值时间
	m_boAccountDataLoadOk = false;
	ZeroMemory(&clientip, sizeof(clientip));
	m_dwWaitChangeSvrSaveDataTime = 0;
	m_boWaitChangeSvrSaveData = false;
	m_boIsWaitChangeSvr = false;
	m_nWaitChangeSvr_newsvr = 0; //=pSpaveMoveInfo->DstMap->getSvrIdType();
	m_nWaitChangeSvr_oldsvr = 0; //=pUser->GetEnvir()->getSvrIdType();
	m_nWaitChangeSvr_ip.s_addr = -1;
	m_nWaitChangeSvr_port = 0;
	m_nWaitChangeSvr_mapid = 0;
	m_nWaitChangeSvr_mapsublineid = 0;
	m_szTGWip[0] = 0;
	m_dwChangeZoneid = 0xFFFFFFFF;
	m_dwSrcZoneId = 0;
	m_dwSrcTrueZoneId = 0;
	m_wSrcTrade = 0;
	m_btCrossKickType = 0;
	m_dwCrossKickZoneId = 0;
	m_wCrossKickTradeid = 0;
	m_dwSaveMoveTimeForNext = 0;
	m_dwSaveAttackTimeForNext = 0;
	m_dwSaveReleaseTimeForNext = 0;
	m_dwBossTmpid = 0;
	m_i64MaxExp = 0;
	m_dwLastOperateTime = time(NULL);;
	ZeroMemory(&m_dwLastChatTime, sizeof(m_dwLastChatTime));
	ZeroMemory(&m_dwChatCount, sizeof(m_dwChatCount));
	m_dwBanChatTime = 0;
	m_dwBanPlayerTime = 0;
	m_ShortCut.Init(this);
	ZeroMemory(&m_GroupInfo, sizeof(stGSGroupInfo));
	ZeroMemory(&m_GuildInfo, sizeof(stGSGuildInfo));
	m_boMapEvt = false;
	m_pCurrItem = NULL;
	m_i64CurrItemTmpId = 0;
	m_dwSrcGameSvrIdType = 0;
	m_dwDoubleTime = 0;
	m_dwDoubleRate = 0;
	m_boTmpGm = false;
	m_dwTmpGmTime = 0;
	m_dwSelLineTime = 0;
	m_tAutoKeepHpTime = 0;
	m_boAutoOpenHp = false;
	m_dwKeepHp = 0;
	m_dwForceKeepHp = 0;
	m_dwHpSet = 0;
	m_boSecondPassOk = true;
	ZeroMemory(&m_szSecondPassWord, sizeof(m_szSecondPassWord));
	ZeroMemory(&m_szSecondMailAddress, sizeof(m_szSecondMailAddress));
	m_btLeader = 0;
	m_tLeaderPeriodTime = 0;
	m_boSendPlayerInfo = false;
	m_nPlayerMin = -1;
	m_nPlayerHour = -1;
	m_nPlayerDay = -1;
	m_nPlayerWeek = -1;
	m_dwOriginalZoneid = 0;
	m_isOpenPacketLog = false; //发包记录
	m_dwJyRate = 0;
	m_emClientVer = _NULL_CLIENT_;
	m_dwShutDownCheckTime = time(NULL);
	ZeroMemory(&szclientip, sizeof(szclientip));
	m_dwHeadPortrait = 0;
	m_nOriginalZone = 0;
	m_ChangeRmbTick = GetTickCount64();
	m_nVipLevel = 0;
	m_dwSaveMoveTimeBefore = GetTickCount64();
	m_i64BattleSec = 0;
	m_i64RunSec = 0;
	m_i64PkSec = 0;
	m_btDayTradeCnt = 0;
	m_isRun = false;
	InitTimer();

}

CPlayerObj::~CPlayerObj() {
	FUNCTION_BEGIN;
	m_xBlockList.deleteall();
	m_xFriendList.deleteall();
	m_xEnemyList.deleteall();
	ClearDelayMsg(0);
	CGameGateWayUser* pGateUser = m_pGateUser;
	m_pGateUser = NULL;
	do {
		AILOCKT(GameService::getMe().m_gatewaysession);
		if (pGateUser)
			pGateUser->m_Player = NULL;
	} while (false);
	GameService::getMe().Add2Delete(pGateUser);
}

void CPlayerObj::InitTimer()
{
	timer_.AddTimer(1000, [this]()
		{
			//g_logger.debug("玩家 %s 1秒定时器", getName());
			stAutoSetScriptParam autoparam(this);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerOneSec");
			m_tuBiao.Run();
			auto curTime = time(nullptr);
			if (m_i64PkSec > 0 && curTime == m_i64PkSec)
				sendTipMsg(this, vformat("`6`已退出pk战斗"));
			if (m_i64PkSec - curTime > 10){  // PK时间超过，自动修正时间
				m_i64PkSec = 0;
			}
			if (m_i64BattleSec - curTime > 10){ // 战斗时间超过，自动修正时间
				m_i64BattleSec = 0;
			}
		});
	timer_.AddTimer(2000, [this]()
		{
			//g_logger.debug("玩家 %s 2秒定时器", getName());
			stAutoSetScriptParam autoparam(this);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerTowSec");
		});
	timer_.AddTimer(5000, [this]()
		{
			//g_logger.debug("玩家 %s 5秒定时器", getName());
			stAutoSetScriptParam autoparam(this);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerFiveSecones");
			if (m_stAbility[AttrID::PpRestoreFiveSec]) {
				if (m_nNowHP > 0 && !isDie()) {
					StatusValueChange(stCretStatusValueChange::pp, m_stAbility[AttrID::PpRestoreFiveSec], "每5秒恢复体力");
				}
			}
		});
	timer_.AddTimer(10000, [this]()
		{
			if (!isDie() &&  m_nNowPP < m_stAbility[AttrID::MaxPP]) {
				if (auto restore_pp = CalculatingRestorePp()) {
					StatusValueChange(stCretStatusValueChange::pp, restore_pp, "每秒恢复体力");
				}
			}
		});
	timer_.AddTimer(15000, [this]()
		{
			//g_logger.debug("玩家 %s 15秒定时器", getName());
			stAutoSetScriptParam autoparam(this);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerFifteenseconds");
		});
	timer_.AddTimer(60000, [this]()
		{
			LoopLimitItemPacket();//限时物品删除 
			StatusValueChange(stCretStatusValueChange::hp, CalculatingRestoreHp(), __FUNC_LINE__);
			StatusValueChange(stCretStatusValueChange::mp, CalculatingRestoreMp(), __FUNC_LINE__);
		});
	timer_.AddTimer(600000, [this]()
		{
			//g_logger.debug("玩家 %s 10分钟定时器", getName());
			RankTopToSuper(Rank_Max_Count);
			stAutoSetScriptParam autoparam(this);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("Playershifenzhong");
		});
	InitEvent();
}
bool CPlayerObj::isSwitchSvr() {
	//FUNCTION_BEGIN;
	return  (m_boIsWaitChangeSvr || __super::isSwitchSvr());
}
bool CPlayerObj::LocalMapTransfer(PosType x, PosType y)
{
	auto curMap = GetEnvir();
	if (!curMap)
		return false;
	if (CCreature::LocalMapTransfer(x, y, 2, false)) {
		return true;
	}
	g_logger.error("传送失败，坐标【%d,%d】无法行走" __FUNC_LINE__, x, y);
	return false;
}
bool CPlayerObj::TransferToMapId(uint16_t mapId, uint16_t cloneId, PosType x, PosType y)
{
	auto curMap = GetEnvir();
	if (!curMap)
		return false;
	auto lineId = GameService::getMe().m_lineId;
	auto targetMap = CUserEngine::getMe().m_maphash.FindById(mapId, lineId, cloneId);
	if (!targetMap)
	{
		g_logger.error("CPlayerObj::MoveLocal() 目标地图不存在 mapid=%d cloneId=%d lineid=%d", mapId, cloneId, lineId);
		return false;
	}
	return TransferToMap(targetMap, x, y);
}
bool CPlayerObj::TransferToMap(CGameMap* targetMap, PosType x, PosType y)
{
	auto curMap = GetEnvir();
	if (!curMap)
		return false;
	m_changeMapId.all = targetMap->getFullMapId();
	m_changeMapPos = { x,y };
	return curMap->DoLeaveMap(this);
}
bool CPlayerObj::MoveToMap(uint16_t mapId, uint16_t cloneId, PosType x, PosType y, uint16_t svrId)
{
	auto curMap = GetEnvir();
	if (!curMap)
		return false;
	bool sameMap = mapId == curMap->getMapId() && cloneId == curMap->getMapCloneId();
	bool sameSvr = svrId == 0 || svrId == 300 || svrId == GameService::getMe().m_svridx;
	if (sameMap && sameSvr)
	{
		return LocalMapTransfer(x, y);
	}
	if (sameSvr)
	{
		return TransferToMapId(mapId, cloneId, x, y);
	}
	return CrossServerTransfer(mapId, cloneId, x, y, svrId);
}
//在同一个服务器传送
bool CPlayerObj::TransferMap(CGameMap* targetMap, PosType x, PosType y)
{
	auto curMap = GetEnvir();
	if (!curMap)
		return false;
	if (curMap == targetMap)
	{
		return LocalMapTransfer(x, y);
	}
	return TransferToMap(targetMap, x, y);
}
bool CPlayerObj::CrossServerTransfer(uint16_t mapId, uint16_t cloneId,
	PosType x, PosType y, uint16_t svrId)
{
	if (svrId < 300) {
		g_logger.error("CrossServerTransfer: 无效的服务器ID %u", svrId);
		return false;
	}

	m_changeMapId.part.mapid = mapId;
	m_changeMapId.part.line = svrId - 300;
	m_changeMapId.part.cloneId = cloneId;
	m_changeMapPos = { x, y };

	g_logger.info("玩家 %llu(%s) 跨服传送: 目标服务器=%u, 地图=%u, 副本=%u, 坐标=(%d,%d)",
		m_i64UserOnlyID, getName(), svrId, mapId, cloneId, x, y);
	if (auto curMap = GetEnvir()) {
		return curMap->DoLeaveMap(this);
	}

	return false;
}
bool  CPlayerObj::ProcessUserMessage(stBaseCmd* pcmd, int ncmdlen, stQueueMsgParam* bufferparam) {
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case 0:
	{

	}break;

	default:
	{
		bufferparam->pQueueMsgBuffer->pluscmdoffset = (size_t)pcmd - (size_t)(&bufferparam->pQueueMsgBuffer->cmdBuffer);
		pushMsg(this, bufferparam->pQueueMsgBuffer);
		//不释放
		bufferparam->bofreebuffer = false;
	}break;
	}
	return true;
}

void CPlayerObj::MakeGhost(bool delaydispose, const char* ff) {
	FUNCTION_BEGIN;
	g_logger.debug("CPlayerObj %.8x  %s ID=%d标记为离开游戏! %d : %s", this, getName(), GetObjectId(), (BYTE)delaydispose, ff);
	if (isClientReady()) { m_tLoginOuttime = time(NULL); }
	bool boisSwitchSvr = isSwitchSvr();
	if (m_pGateUser && m_pGateUser->m_OwnerDbSvr && !boisSwitchSvr) {
		if (CUserEngine::getMe().isCrossSvr()) {
			CrossSvrSavePlayerData(_SAVE_TYPE_LOGIN_OUT_);
		}
		else {
			m_pGateUser->m_OwnerDbSvr->push_back2save(this, _SAVE_TYPE_LOGIN_OUT_);
			//m_pGateUser->m_OwnerLoginSvr->push_back2save(this,_SAVE_TYPE_LOGIN_OUT_);
			m_dwNextSaveRcdTime = time(NULL) + GameService::getMe().m_svrcfg.dwsavercdintervaltime;
		}
	}

#define  _offline_fmt_str_ 		",\'%s\',\'%s\',%I64d,%d,\'%s\',\'%s\',%d,%d,%d,%d,'%s',%d,%d,%d"
	GameService::getMe().Send2LogSvr(_SERVERLOG_USERLOGIN_, 0, 0, this, boisSwitchSvr ? "\'changesvr\'" _offline_fmt_str_ : "\'offline\'" _offline_fmt_str_,
		m_pGateUser->m_szAccount, getName(), m_i64UserOnlyID, m_dwPlayerOnlineTime,
		timetostr(m_dwPlayerCreateTime), timetostr(m_tLoginOuttime), m_dwLevel
		, m_siFeature.job, m_siFeature.sex, m_dwGold, inet_ntoa(clientip), 0, (m_dwLevel >= 0 ? 0 : 0), 0);

	do {
		//玩家涉及到重新登录 不能等z待对象被删除 所以玩家被标记为删除的时候就需要从列表中移出
		CPlayerObj* thisCret = this;
		if (!isSwitchSvr())
		{
			stSendDelUser2SuperSrv retcmd;
			fullToSuperSvrPlayerBaseData(retcmd.basedata);
			CUserEngine::getMe().SendMsg2SuperSvr(&retcmd, sizeof(retcmd));
			RankTopToSuper(Rank_Max_Count);

			stGlobalDelPlayerData globalretcmd;
			globalretcmd.i64OnlyId = m_i64UserOnlyID;
			s_strncpy_s(globalretcmd.szName, getName(), _MAX_NAME_LEN_ - 1);
			s_strncpy_s(globalretcmd.szAccount, getAccount(), _MAX_ACCOUT_LEN_ - 1);
			globalretcmd.dwLevel = m_dwLevel;
			globalretcmd.dwZSLevel = 0;
			globalretcmd.btSex = m_siFeature.sex;
			globalretcmd.btJob = m_siFeature.job;
			globalretcmd.dwMapID = GetEnvir() ? GetEnvir()->getMapId() : 0;
			globalretcmd.dwGuildId = m_GuildInfo.dwGuildId;
			strcpy_s(globalretcmd.szMapName, sizeof(globalretcmd.szMapName) - 1, GetEnvir() ? GetEnvir()->getMapName() : "");
			globalretcmd.dwGateWayId = m_pGateUser->svr_id_type_value;
			globalretcmd.Feature = m_siFeature;
			globalretcmd.emClientVer = m_emClientVer;
			globalretcmd.btPlatForm = getPlatFormType();
			globalretcmd.btVipLvl = (BYTE)m_nVipLevel;
			CUserEngine::getMe().SendMsg2GlobalSvr(&globalretcmd, sizeof(stGlobalDelPlayerData));
		}

		m_btSelUserState = _PLAYER_STATE_OUTGAME_;
		stUserReSetState setstate(m_pGateUser->m_tmpid, m_btSelUserState);
		m_pGateUser->m_OwnerLoginSvr->sendcmd(&setstate, sizeof(setstate));

		do {
			AILOCKT(CUserEngine::getMe().m_loadokplayers);
			CUserEngine::getMe().m_loadokplayers.removeValue(thisCret);
			CUserEngine::getMe().m_playerhash.removeValue(thisCret);
		} while (false);


	} while (false);

	ClearDelayMsg(0);
	__super::MakeGhost(delaydispose, ff);
	if (m_pGateUser) {
		g_logger.debug("%.8x:%s:%s通知网关删除玩家!", this, getAccount(), getName());
		m_pGateUser->notifyGateRemoveUser();
	}
}

void CPlayerObj::Disappear() {
	FUNCTION_BEGIN;
	CCreature::Disappear();
}

void CPlayerObj::OnPlayerBeforeChangeSvrSaveData() {
	FUNCTION_BEGIN;
	g_logger.debug("玩家 %s 切换游戏服务器存档前", getName());
	if (isClientReady()) { m_tLoginOuttime = time(NULL); }
	TriggerEvent(this, CHANGESVROFFLINE, 1);
}

void CPlayerObj::OnPlayerBeforeChangeGameSvr() {
	FUNCTION_BEGIN;
	g_logger.debug("玩家 %s 切换游戏服务器前", getName());
	if (m_GroupInfo.dwGroupId) {
		if (this->getSwitchSvrInfo()) {
			stSpaveMoveInfo* pSpaveMoveInfo = this->getSwitchSvrInfo();
			if (pSpaveMoveInfo
				&& GetEnvir()
				&& pSpaveMoveInfo->DstMap)
			{
				if (pSpaveMoveInfo->dwZoneid != GameService::getMe().m_nZoneid)
				{
					ServerGetLeaveGroup();
					GetEnvir()->mapGroupRemoveNum(this);
					m_GroupInfo.dwGroupId = 0;
					g_logger.debug("玩家 %.8x  %s 跨服退队!", this, getName());
				}
			}
		}
	}
	m_Petm.clear();
}

void CPlayerObj::OnPlayerChangeSvrSuccess() {
	FUNCTION_BEGIN;
	stWaitPlayerChangeSvrSuccess retcmd;
	retcmd.i64CretOnlyId = m_i64UserOnlyID;
	CUserEngine::getMe().SendMsg2SuperSvr(&retcmd, sizeof(retcmd));
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd, sizeof(retcmd));
}

void  CPlayerObj::OnPlayerOffLine() {
	FUNCTION_BEGIN;
	if (IsOffline())
		return;
	SetReadyState(PLAYER_READY_OFFLINE, __FUNC_LINE__);
	if (m_Trade.GetTradeState()) m_Trade.AllCancelTrade(getName(), 1);
	m_dwLineOutTime = time(NULL);
	if (isSwitchSvr()) { return; }
	if (m_GroupInfo.dwGroupId) {
		ServerGetLeaveGroup();
		g_logger.debug("玩家 %.8x  %s 下线退队!", this, getName());
	}
	g_logger.debug("玩家 %.8x  %s 掉线!", this, getName());
	TriggerEvent(this, OFFLINE, 1);
	MakeGhost(true, __FUNC_LINE__); 			//PreMakeGhost()要存档的东西请在这个之前处理
	m_Petm.clear();
	if (m_btSelUserState == _PLAYER_STATE_NONE_ && m_pGateUser) {
		m_btSelUserState = _PLAYER_STATE_NETCUT_;
		stUserReSetState setstate(m_pGateUser->m_tmpid, m_btSelUserState);
		m_pGateUser->m_OwnerLoginSvr->sendcmd(&setstate, sizeof(setstate));
	}
	getMsgFromTxServer("tx_playeroffline", true);
}

void CPlayerObj::OnPlayerReOnLine() {
	FUNCTION_BEGIN;
	g_logger.debug("玩家 %s 重新上线!", getName());
	if (!IsOffline()) return;
	SetReadyState(PLAYER_READY_ONLINE, __FUNC_LINE__);
}
bool CPlayerObj::DealDelayMsg(int flag) {
	bool isTimeValid = false;
	void* cmd = nullptr;
	if (flag == 1) {
		if (!m_cDelayMoveList.empty()) {
			isTimeValid = (GetTickCount64() >= m_dwSaveMoveTimeForNext);
			if (isTimeValid) {
				cmd = m_cDelayMoveList.front();
				m_cDelayMoveList.pop_front();
			}
		}
	}
	else {
		uint64_t& saveTime = (flag == 2) ? m_dwSaveAttackTimeForNext : m_dwSaveReleaseTimeForNext;
		if (!m_cDelayAttackList.empty()) {
			isTimeValid = (GetTickCount64() >= saveTime);
			if (isTimeValid) {
				cmd = m_cDelayAttackList.front();
				m_cDelayAttackList.pop_front();
			}
		}
	}
	if (isTimeValid) {
		m_cDelayDealList.pop_front();
		if (flag == 1) {
			OnstCretMove(this, static_cast<stCretMove*>(cmd), sizeof(*static_cast<stCretMove*>(cmd)), nullptr);
			CUserEngine::getMe().m_cretMovePool.destroy(static_cast<stCretMove*>(cmd));
		}
		else {
			OnstCretAttack(this, static_cast<stCretAttack*>(cmd), sizeof(*static_cast<stCretAttack*>(cmd)), nullptr);
			CUserEngine::getMe().m_cretAttackPool.destroy(static_cast<stCretAttack*>(cmd));
		}
	}
	else {
		if ((flag == 1 && m_cDelayMoveList.empty()) || (flag != 1 && m_cDelayAttackList.empty())) {
			m_cDelayDealList.pop_front();
		}
		else {
			return false;
		}
	}
	return true;
}

void CPlayerObj::DealDelayList()
{
	if (!m_cDelayDealList.empty()) {
		int nDealType = m_cDelayDealList.front();
		while ((nDealType == 1 && !m_cDelayMoveList.empty() && GetTickCount64() >= m_dwSaveMoveTimeForNext) ||
			(nDealType == 2 && !m_cDelayAttackList.empty() && GetTickCount64() >= m_dwSaveAttackTimeForNext) ||
			(nDealType == 3 && !m_cDelayAttackList.empty() && GetTickCount64() >= m_dwSaveReleaseTimeForNext)) {
			if (DealDelayMsg(nDealType)) {
				if (!m_cDelayDealList.empty())
				{
					nDealType = m_cDelayDealList.front();
				}
			}
		}
	}
}

void CPlayerObj::ChangeDayLoginLog()
{
#define _offline_fmt_str_		",\'%s\',\'%s\',%I64d,%d,\'%s\',\'%s\',%d,%d,%d,%d,'%s',%d,%d,%d"
	GameService::getMe().Send2LogSvr(_SERVERLOG_USERLOGIN_, 0, 0, this, "\'changeday\'" _offline_fmt_str_,
		m_pGateUser->m_szAccount, getName(), m_i64UserOnlyID, m_dwPlayerOnlineTime,
		timetostr(m_dwPlayerCreateTime), timetostr(m_tLoginOuttime), m_dwLevel
		, m_siFeature.job, m_siFeature.sex, m_dwGold, inet_ntoa(clientip), 0, 0, 0);

	///切天后记录一次login日志
	stChangeDayLoginLog loginLogCmd;
	loginLogCmd.svr_id_type_value = GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value;
	strcpy_s(loginLogCmd.szAccount, sizeof(loginLogCmd.szAccount) - 1, m_pGateUser->m_szAccount);
	loginLogCmd.trueZoneId = GameService::getMe().m_nTrueZoneid;
	m_pGateUser->m_OwnerLoginSvr->sendcmd(&loginLogCmd, sizeof(loginLogCmd));
}

void CPlayerObj::player_run() {
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true, "");
	FUNCTION_MONITOR(48, "");
	time_t curtime = time(NULL);
	SaveData();
	if (isDie()) {
		ClearDelayMsg();
	}
	else {
		DealDelayList();
	}
	if (m_dwPkRunTime != 0 && (DWORD)curtime >= (m_dwPkRunTime + 60)) {
		m_dwPkRunTime = 0;
	}
	zTime ztime;
	int nLoginCall = 0;
	if (m_nPlayerMin != ztime.getMin()) {
		nLoginCall = (m_nPlayerMin == -1) ? 1 : 0;
		m_nPlayerMin = ztime.getMin();
		if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
			stAutoSetScriptParam autoparam(this);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerMin", nLoginCall);
		}
	}
	if (m_nPlayerHour != ztime.getHour()) {
		nLoginCall = (m_nPlayerHour == -1) ? 1 : 0;
		m_nPlayerHour = ztime.getHour();
		if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
			stAutoSetScriptParam autoparam(this);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerHour", nLoginCall);
		}
	}
	if (m_nPlayerDay != ztime.GetYDay()) {
		nLoginCall = (m_nPlayerDay == -1) ? 1 : 0;
		m_nPlayerDay = ztime.GetYDay();
		if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
			stAutoSetScriptParam autoparam(this);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerDay", nLoginCall);
		}
		if (nLoginCall == 0) {
			ChangeDayLoginLog();
		}
	}
	if (m_nPlayerWeek != ztime.getnWeek()) {
		nLoginCall = (m_nPlayerWeek == -1) ? 1 : 0;
		m_nPlayerWeek = ztime.getnWeek();
		if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
			stAutoSetScriptParam autoparam(this);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerWeek", nLoginCall);
		}
	}
	if (!m_boSendPlayerInfo && m_dwLoginTime <= (curtime - 60 * 2)) {
		m_boSendPlayerInfo = true;
	}
	if (m_boAutoOpenHp && m_tAutoKeepHpTime < curtime) {
		m_Packet.ServerAutoKeepHp(m_dwKeepHp, m_dwForceKeepHp);
		m_tAutoKeepHpTime = curtime;
	}
	m_Trade.Run();
	m_tiShi.Run();
	if (!sConfigMgr.closePlayerEvent) {
		m_Timer->run();
	}
	if (m_abilityFlag) {
		DoChangeProperty(m_stAbility, true, __FILE_FUNC__);
	}
}

bool CPlayerObj::isClientReady() {
	//FUNCTION_BEGIN;
	return (m_btReadyState >= _READYSTATE_ISALL_READY_);
}

bool CPlayerObj::Die() {
	FUNCTION_BEGIN;
	if (!isClientReady()) {
		return false;
	}
	g_logger.debug("%s 玩家死亡 %s", getName(), "");
	if (CCreature::Die()) {
		CPlayerObj* pPlayer = NULL;
		if (m_pLastHiter) {
			if (m_pLastHiter->isPlayer()) {
				pPlayer = m_pLastHiter->toPlayer();
			}
			else if (m_pLastHiter->isPet()) {
				if (m_pLastHiter->toPet()->getMaster()->isPlayer()) {
					pPlayer = m_pLastHiter->toPet()->getPlayerMaster();
				}
			}
		}
		CalculatingPk(pPlayer, PK_STATUS_PKSWORD);
		GameService::getMe().Send2LogSvr(_SERVERLOG_USERDIE_, 0, 0, this->toPlayer(), "'playerdie','%s','%s',%I64d,'%s',%d,%d,%d,'%s','%s','%s'",
			getAccount(),
			this->getName(),
			m_i64UserOnlyID,
			this->GetEnvir()->getMapName(),
			this->m_nCurrX,
			this->m_nCurrY,
			0,
			"",
			(pPlayer) ? pPlayer->getName() : "",
			(m_pLastHiter) ? m_pLastHiter->getName() : "");
		if (m_GroupInfo.dwGroupId) {
			stGroupMemberDie iamdie;
			iamdie.i64OnlyId = m_i64UserOnlyID;
			CUserEngine::getMe().SendMsg2GlobalSvr(&iamdie, sizeof(stGroupMemberDie));
		}

		if (m_Petm.getpetcount()) {
			m_Petm.DieClear();
		}
		if (m_Trade.GetTradeState())
		{
			m_Trade.AllCancelTrade(getName(), 1);
		}
		ClearDelayMsg();
		TriggerEvent(this, DIE, 1);
	}
	PlayDeathDropped();
	return true;
}
void CPlayerObj::SaveData()
{
	//存档
	if (time(NULL) > m_dwNextSaveRcdTime && m_btReadyState >= _READYSTATE_SVR_READY_) {
		if (CUserEngine::getMe().isCrossSvr()) {//跨服存档
			CrossSvrSavePlayerData(_SAVE_TYPE_TIMER_);
		}
		else {
			if (m_pGateUser && m_pGateUser->m_OwnerDbSvr && !isSwitchSvr()) {
				if (m_pGateUser->m_OwnerDbSvr->push_back2save(this, _SAVE_TYPE_TIMER_)/* && (!m_boAccountDataLoadOk || m_pGateUser->m_OwnerLoginSvr->push_back2save(player,_SAVE_TYPE_TIMER_))*/) {
					m_dwNextSaveRcdTime = time(NULL) + GameService::getMe().m_svrcfg.dwsavercdintervaltime;
				}
				else { m_dwNextSaveRcdTime = time(NULL) + _random(10); }
			}
			else { m_dwNextSaveRcdTime = time(NULL) + _random(10); }
		}
	}
}

void CPlayerObj::NotifyMap()
{
	CGameMap* map = CUserEngine::getMe().m_maphash.FindByFullId(m_changeMapId.all);
	if (map) {
		if (!IsOnline())
		{
			g_logger.debug("%s 通知 %s 加载玩家上线", getName(), map->getMapName());
		}
		else
		{
			SetPoint(m_changeMapPos.x, m_changeMapPos.y);
			g_logger.debug("%s 通知 %s 加载玩家切图", getName(), map->getMapName());
		}
		SetEnvir(map);//TODO
		map->EnterMap(this);
		m_changeMapId.all = 0;
	}
	else {
		g_logger.debug("%s 通知地图加载玩家切图失败 %I64d  ", getName(), m_changeMapId.all);
		//NotifyClose(CLOSEUSER_ENTERMAP);
	}
}
void CPlayerObj::Update() {
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true, "");
	FUNCTION_MONITOR(48, "");
	if (GetTickCount64() > m_dwNextRunTick) {

		if (m_btKickState != NONE_KICK && GetTickCount64() > m_dwDoKickTick) {
			if (m_btKickState == KICK_CHANGEGAMESVROK) {
				this->MakeGhost(true, __FUNC_LINE__);							//切换服务器完成
			}
			else if (m_pGateUser && !m_pGateUser->isSocketClose()) {
				g_logger.debug("服务器踢人 %s:%s:%I64d,踢人代码 %d", m_pGateUser->m_szAccount, getName(), m_i64UserOnlyID, m_btKickState);
				m_dwDoKickTick = GetTickCount() + 5000;
				m_dwDoKickTickCount++;
				if (m_dwDoKickTickCount <= 3) {
					m_pGateUser->notifyGateKiceUser();
				}
				else {
					g_logger.debug("%s 网关负载很大或连接已移除,直接当掉线处理", getName());
					m_pGateUser->OnclientCloseUser();
				}
			}
		}
		bool boSwitchSvr = isSwitchSvr();
		if (boSwitchSvr) {
			if (m_boWaitChangeSvrSaveData) {
				//等待数据保存结果
				if (CUserEngine::getMe().m_boIsShutDown && (time(NULL) - m_dwWaitChangeSvrSaveDataTime) > 60 * 2) {
					g_logger.error("%s:%s:%I64d 服务器关闭后存档等待超时!", m_pGateUser->m_szAccount, getName(), m_i64UserOnlyID);
				}
				else {
					return;
				}
			}
			if (m_nWaitChangeSvr_ip.s_addr != -1 && m_nWaitChangeSvr_port != 0) {
				if (m_btKickState == NONE_KICK) {
					//已经保存成功
					stNotifyPlayerChangeGameSvrCmd ncc;
					ncc.newsvr = m_nWaitChangeSvr_newsvr;
					ncc.oldsvr = m_nWaitChangeSvr_oldsvr;
					ncc.tmpid = this->m_pGateUser->m_tmpid;
					strcpy_s(ncc.szName, sizeof(ncc.szName) - 1, this->getName());
					strcpy_s(ncc.szAccount, sizeof(ncc.szAccount), this->m_pGateUser->m_szAccount);
					this->m_pGateUser->m_OwnerLoginSvr->sendcmd(&ncc, sizeof(ncc));

					stChangeGameSvrCmd changecmd;
					changecmd.ip = m_nWaitChangeSvr_ip;
					changecmd.port = m_nWaitChangeSvr_port;
					changecmd.mapid = m_nWaitChangeSvr_mapid;
					changecmd.dwTrueZoneid = m_dwChangeTrueZoneid;
					strcpy_s(changecmd.szTGWip, sizeof(changecmd.szTGWip) - 1, m_szTGWip);
					if (m_dwChangeZoneid != 0xFFFFFFFF && m_dwChangeZoneid != GameService::getMe().m_nZoneid) {
						changecmd.changesvr_type = 1;
					}
					else {
						changecmd.changesvr_type = 0;
					}
					changecmd.gamesvr_id_type = m_nWaitChangeSvr_newsvr;
					changecmd.btmapsubline = m_nWaitChangeSvr_mapsublineid;

					strcpy_s(changecmd.szAccount, sizeof(changecmd.szAccount) - 1, getAccount());
					strcpy_s(changecmd.szName, sizeof(changecmd.szName) - 1, getName());
					this->SendMsgToMe(&changecmd, sizeof(changecmd));
				}

				m_szTGWip[0] = 0;
				m_dwChangeZoneid = 0xFFFFFFFF;
				m_nWaitChangeSvr_port = 0;
				m_nWaitChangeSvr_ip.s_addr = 0;
				m_btKickState = KICK_CHANGEGAMESVROK;
				m_dwDoKickTick = GetTickCount64() + 50;
				g_logger.debug(" %s:%s 切换服务器成功! TrueZoneId:%d, ZoneId:%d, SvrId:%d", this->m_pGateUser->m_szAccount, this->getName(),
					GameService::getMe().m_nTrueZoneid, GameService::getMe().m_nZoneid, GameService::getMe().m_svridx);

				if (m_dwCrossKickZoneId && m_dwCrossKickZoneId != GameService::getMe().m_nZoneid && !CUserEngine::getMe().isCrossSvr()) {
					stCrossZoneToGlobalSuper corsstoglobalsupercmd;
					corsstoglobalsupercmd.i64OnlyId = m_i64UserOnlyID;
					corsstoglobalsupercmd.nCrossZoneId = m_dwCrossKickZoneId;
					corsstoglobalsupercmd.wCrossTradeId = m_wCrossKickTradeid;
					corsstoglobalsupercmd.dwCrossSvrType = m_nWaitChangeSvr_newsvr;
					CUserEngine::getMe().SendMsg2GlobalSvr(&corsstoglobalsupercmd, sizeof(stCrossZoneToGlobalSuper));
					CUserEngine::getMe().SendMsg2SuperSvr(&corsstoglobalsupercmd, sizeof(stCrossZoneToGlobalSuper));
				}
			}
			return;
		}

		m_dwNextRunTick = GetTickCount64() + m_nRunIntervalTick + _random(20);
		if (m_pVisitNPC) { m_pVisitNPC = NULL; }
		if (m_pGateUser && m_pGateUser->isSocketClose()) {
			if (m_btReadyState >= _READYSTATE_SVR_READY_) {
				OnPlayerOffLine();
			}
			else {
				MakeGhost(true, __FUNC_LINE__);
			}
		}
		else  if (m_btReadyState >= _READYSTATE_SVR_READY_) {
			CCreature::Update();
			if (isClientReady()) {
				player_run();
			}

		}
	}
}

bool CPlayerObj::CalculatingSpeed()
{
	DWORD oldhit = m_dwHitIntervalTime;
	DWORD oldcast = m_dwCastIntervalTime;
	DWORD oldmove = m_dwMoveIntervalTime;
	auto attakSpeedSum = (m_stAbility[AttrID::AttackSpeedPer] + sConfigMgr.GetAttackSpeed(m_stAbility[AttrID::AttackSpeedPhase])) / Denom;	//攻速加成和
	auto castSpeedSum = (m_stAbility[AttrID::ReleaseSpeedPer] + sConfigMgr.GetReleaseSpeed(m_stAbility[AttrID::ReleaseSpeedPhase])) / Denom;
	auto moveSpeedSum = (m_stAbility[AttrID::MoveSpeedPer] + sConfigMgr.GetMoveSpeed(m_stAbility[AttrID::MoveSpeedPhase])) / Denom;
	m_dwHitIntervalTime = attakSpeedSum < 1 ? m_nAttackInterval * (1 - attakSpeedSum) : m_nAttackInterval;
	m_dwHitIntervalTime = max(m_dwHitIntervalTime, sConfigMgr.GetFastestAttackSpeed());
	m_dwCastIntervalTime = castSpeedSum < 1 ? m_nReleaseInterval * (1 - castSpeedSum) : m_nReleaseInterval;
	m_dwCastIntervalTime = max(m_dwCastIntervalTime, sConfigMgr.GetFastestReleaseSpeed());
	m_dwMoveIntervalTime = moveSpeedSum < 1 ? m_nMoveInterval * (1 - moveSpeedSum) : m_nMoveInterval;
	m_dwWalkIntervalTime = m_dwMoveIntervalTime;
	if (oldhit != m_dwHitIntervalTime)
	{
		UpdateAppearance(FeatureIndex::atkSpeedPer, m_dwHitIntervalTime);
	}
	if (oldcast != m_dwCastIntervalTime)
	{
		UpdateAppearance(FeatureIndex::releaseInterval, m_dwCastIntervalTime);
	}
	if (oldmove != m_dwMoveIntervalTime)
	{
		UpdateAppearance(FeatureIndex::moveInterval, m_dwMoveIntervalTime);
	}
	return true;
}

void CPlayerObj::OnUserLoginReady() {
	FUNCTION_BEGIN;
	SetReadyState(PLAYER_READY_ONLINE, __FUNC_LINE__);
	auto curMap = GetEnvir();
	if (!curMap) return;
	m_dwLoginTime = time(NULL);
	if (!m_boIsChangeSvr) {
		SendPlayerProperty();
	}
	clientip = m_pGateUser->clientip;
	//请求好友
	stRelationGetList RelationGetListCmd;
	RelationGetListCmd.btType = LIST_FRIEND;
	doRelationCmd((stBaseCmd*)&RelationGetListCmd, sizeof(RelationGetListCmd));
	RelationGetListCmd.btType = LIST_BLOCK;
	doRelationCmd((stBaseCmd*)&RelationGetListCmd, sizeof(RelationGetListCmd));
	RelationGetListCmd.btType = LIST_ENEMY;
	doRelationCmd((stBaseCmd*)&RelationGetListCmd, sizeof(RelationGetListCmd));
	//登陆事件
	TriggerEvent(this, LOGIN, 1);
	//g_logger.forceLog(zLogger::zINFO, "LoginReady登陆事件调用");
	if (m_boIsChangeSvr) {
		//切换服务器

		m_btChangeSvrSpaceMoveType = 0;
	}
	else {
		stCretMoveRet cmd;
		fullMoveRet(&cmd, 0);
		SendRefMsg(&cmd, sizeof(cmd), true);
		//发送所有的装备
		m_Packet.SendEquipItems();
		m_Packet.SendBagItems();
		m_Packet.SendStorageItems();
		m_Packet.SendTmpBagItems();
		//--发送所有技能信息
		m_cMagic.SendAllCretSkill();
		//--发送所有快捷键信息
		m_ShortCut.SendShortCuts();
		//请求邮件
		stMailCheckNewMail CheckNewMailCmd;
		doCretMailCmd((stBaseCmd*)&CheckNewMailCmd, sizeof(CheckNewMailCmd));
		//技能冷却
		stSkillCDTime cdtimecmd;
		cdtimecmd.dwMagicId = 0;
		cdtimecmd.dwPublicTick = GetPublicCDTime();
		cdtimecmd.dwSelfTick = GetPublicCDTime();
		SendMsgToMe(&cdtimecmd, sizeof(cdtimecmd));
		m_cMagic.SendAllMagicCD();
	}


	time_t shutdowntime = CUserEngine::getMe().m_shutdowntime;
	if ((shutdowntime - time(NULL)) < 60 * 60) {
		if (shutdowntime > 0 && !CUserEngine::getMe().m_boIsShutDown) {
			std::string shutdowndis = CUserEngine::getMe().m_shutdowndis;
			CChat::sendGmToUser(this, GameService::getMe().GetStrRes(RES_LANG_OTHER_SERVER), timetostr(shutdowntime), shutdowndis.c_str());
		}
	}
	int bantest = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("BanTestZoneGM", 0, this);
	if (bantest == 1) {
		m_btGmLvl = 6;
	}
	if (bantest == 2) {
		m_btGmLvl = 0;
	}

	if (GameService::getMe().m_btAllisGm == m_btGmLvl && m_btGmLvl > 0) {
		CChat::sendSystem(this, "调试模式,GM等级自动调整为 %d", m_btGmLvl);
	}
	else if (m_btGmLvl > 0) {
		CChat::sendSystem(this, "GM进入游戏,等级为 %d", m_btGmLvl);
	}
	if (m_btGmLvl > 0) {
		CUserEngine::getMe().addLuaErrorPlayer(this);
	}

	if (m_btGmLvl > 0 && GameService::getMe().m_btGmOnlineWDYS != 0) {
		m_boGmHide = true;
		m_btWudi = 1;
		this->Disappear();
		CChat::sendSystem(this, "GM自动开启无敌模式,隐身模式");
	}
	if (m_btGmLvl > 0 && (!isWudi() || !m_boGmHide)) {
		CChat::sendCenterMsg(getName(), "您的无敌或者隐身已经消失！请注意！");
	}
	//给网关的玩家设置
	stPlayerInfo2GatewayCmd playerset;
	playerset.btGmLv = m_btGmLvl;
	SendMsgToMe(&playerset, sizeof(playerset));

	g_logger.debug("玩家 %s 上线，客户端准备完毕!", getName());
#define  _loginready_fmt_str_ 		",\'%s\',\'%s\',%I64d,%d,\'%s\',\'%s\',%d,%d,%d,%d,'%s',%d,%d,%d"
	GameService::getMe().Send2LogSvr(_SERVERLOG_USERLOGIN_, 0, 0, this,
		m_boIsChangeSvr ? "\'changesvronline\'" _loginready_fmt_str_ : "\'online\'" _loginready_fmt_str_,
		m_pGateUser->m_szAccount, getName(), m_i64UserOnlyID,
		m_dwPlayerOnlineTime, timetostr(m_dwPlayerCreateTime), timetostr(m_tLoginOuttime),
		m_dwLevel, m_siFeature.job, m_siFeature.sex, m_dwGold, inet_ntoa(clientip), 0, 0, 0);
}

void CPlayerObj::fullToSuperSvrPlayerBaseData(stUpdateSupersvrPlayerBaseData& data) {
	FUNCTION_BEGIN;
	data.stSiFeature = m_siFeature;
	data.btGamesvrState = 0;
	data.dwMapID = GetEnvir() ? GetEnvir()->getMapId() : 0;
	data.dwTmpId = GetObjectId();
	data.dwLevel = m_dwLevel;
	data.i64OnlyId = m_i64UserOnlyID;
	data.dwGateWayId = this->m_pGateUser->svr_id_type_value;
	strcpy_s(data.szclientip, sizeof(data.szclientip) - 1, m_pGateUser->szclientip);
	data.btGMlv = m_btGmLvl;
	data.dwVip = getVipType();
	getCharHumanInfo(data.szBinData);
}

static unsigned char g_szSaveSuperSvrBuffer[1024 * 1024];		//缓存数据
static unsigned char g_szComSuperBuffer[1024 * 1024];			//压缩数据

void CPlayerObj::getCharHumanInfo(char* szHumanInfo) {
	FUNCTION_BEGIN;
	unsigned char* pbindata = &g_szSaveSuperSvrBuffer[0];
	unsigned long bindatasize = sizeof(g_szSaveSuperSvrBuffer);

	Json::Value json_object;
	Json::FastWriter writer;

	json_object["PbinVer"] = _SAVE_DATA_VER_;
	json_object["GuildName"] = m_GuildInfo.szGuildName;
	json_object["i64FightScore"] = (double)m_stAbility.i64FightScore;
	json_object["HeadPortrait"] = (int)m_dwHeadPortrait; //头像
	json_object["DressId"] = (double)CALL_LUARET<DWORD>("GetFeatureId", 0, this, 1); //时装
	bool savesucc = false;
	if (m_Packet.saveEquip((char*)&g_szSaveSuperSvrBuffer, bindatasize)) {
		json_object["Equip"] = (char*)g_szSaveSuperSvrBuffer;
		//bindatasize = sizeof(g_szSaveSuperSvrBuffer);
		savesucc = true;
	}
	//bindatasize = sizeof(g_szSaveSuperSvrBuffer) - bindatasize;
	//if (SaveEquipPosInfo((char*)&g_szSaveSuperSvrBuffer, bindatasize)) {
	//	json_object["EquipPosInfo"] = (char*)g_szSaveSuperSvrBuffer;
	//	bindatasize = sizeof(g_szSaveSuperSvrBuffer);
	//	savesucc = true;
	//}
	if (!savesucc) {
		return;
	}
	std::string json_str = writer.write(json_object);

	unsigned long nOutLen = sizeof(g_szComSuperBuffer);
	if (compresszlib((unsigned char*)json_str.c_str(), json_str.size(), &g_szComSuperBuffer[0], nOutLen) != Z_OK) {
		return;
	}

	*((int*)(&g_szSaveSuperSvrBuffer[0])) = (int)json_str.size();
	memcpy(&g_szSaveSuperSvrBuffer[sizeof(int)], &g_szComSuperBuffer[0], nOutLen);

	nOutLen += sizeof(int);
	int retlen = ROUNDNUMALL(nOutLen, 3) / 3 * 4;	//当前长度，偏移用
	if (retlen > SUPER_PLAYER_SAVEDATA_LEN) {
		return;
	}

	base64_encode((char*)&g_szSaveSuperSvrBuffer[0], nOutLen, (char*)&g_szComSuperBuffer[0], retlen);
	memcpy((void*)szHumanInfo, (char*)&g_szComSuperBuffer[0], retlen);
}

void CPlayerObj::UpdateToSuperSvr()
{
	FUNCTION_BEGIN;
	stSendAddUser2SuperSrv retcmd;
	s_strncpy_s(retcmd.szName, getName(), _MAX_NAME_LEN_ - 1);
	s_strncpy_s(retcmd.szAccount, getAccount(), _MAX_ACCOUT_LEN_ - 1);
	fullToSuperSvrPlayerBaseData(retcmd.basedata);
	s_strncpy_s(retcmd.szSubPlatform, getSubPlatform(), _MAX_NAME_LEN_ - 1);
	CUserEngine::getMe().SendMsg2SuperSvr(&retcmd, sizeof(retcmd));
}

void CPlayerObj::UpdateMemberName(const char* szNewName)
{
	FUNCTION_BEGIN;
	stGuildMasterNameChange retcmd;
	strcpy_s(retcmd.szNewName, sizeof(retcmd.szNewName) - 1, szNewName);
	retcmd.dwGuildId = m_GuildInfo.dwGuildId;
	retcmd.i64OnlyId = m_i64UserOnlyID;
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd, sizeof(retcmd));
}

void CPlayerObj::UpdateToGlobalSvr()
{
	FUNCTION_BEGIN;
	stGlobalAddPlayerData retcmd;
	retcmd.i64OnlyId = m_i64UserOnlyID;
	s_strncpy_s(retcmd.szName, getName(), _MAX_NAME_LEN_ - 1);
	s_strncpy_s(retcmd.szAccount, getAccount(), _MAX_ACCOUT_LEN_ - 1);
	retcmd.dwLevel = m_dwLevel;
	retcmd.btSex = m_siFeature.sex;
	retcmd.btJob = m_siFeature.job;
	retcmd.dwGuildId = m_GuildInfo.dwGuildId;
	retcmd.dwMapID = GetEnvir() ? GetEnvir()->getMapId() : 0;
	strcpy_s(retcmd.szMapName, sizeof(retcmd.szMapName) - 1, GetEnvir() ? GetEnvir()->getMapName() : "");
	retcmd.dwGateWayId = m_pGateUser ? m_pGateUser->svr_id_type_value : 0;
	retcmd.boGM = (m_btGmLvl > 0) ? true : false;
	retcmd.dwlastloginouttime = m_tLoginOuttime;
	retcmd.Feature = m_siFeature;
	strcpy_s(retcmd.szGuildName, _MAX_NAME_LEN_ - 1, m_GuildInfo.szGuildName);
	retcmd.dwZSLevel = 0;
	retcmd.btVipLvl = (BYTE)m_nVipLevel;
	retcmd.dwSrcZoneId = m_dwSrcZoneId;
	retcmd.dwSrcTrueZoneId = m_dwSrcTrueZoneId;
	retcmd.dwHeadPortrait = m_dwHeadPortrait;
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd, sizeof(stGlobalAddPlayerData));
}

void CPlayerObj::SendMsgToMe(void* pcmd, int ncmdlen, int zliblvl) {
	//FUNCTION_BEGIN;
	switch (((stBaseCmd*)pcmd)->value)
	{
	case stCretChat::_value:
	{
		if (m_xBlockList.FindByName(((stCretChat*)pcmd)->szName)) {
			return;
		}
		bool reddot = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("chatreddot", false, ((stCretChat*)pcmd)->btChatType, this);
		if (m_pGateUser && !m_pGateUser->isSocketClose()) {
			m_pGateUser->SendProxyCmd(pcmd, ncmdlen, false, zliblvl);
		}
	}break;
	default:
	{
		if (m_pGateUser && !m_pGateUser->isSocketClose()) {
			m_pGateUser->SendProxyCmd(pcmd, ncmdlen, false, zliblvl);
		}
	}break;
	}
}

void CPlayerObj::EnterMapNotify(MapObject* obj)
{
	stMapCreatePlayer retcmd;

	retcmd.i64OnlyId = m_i64UserOnlyID;
	retcmd.location.mapid = GetEnvir()->getMapId();
	retcmd.location.ncurx = m_nCurrX;
	retcmd.location.ncury = m_nCurrY;
	retcmd.location.ncurz = m_nCurrZ;
	retcmd.dwTmpId = GetObjectId();
	retcmd.lifestate = m_LifeState;
	retcmd.dwLevel = m_dwLevel;
	retcmd.nNowHp = m_nNowHP;
	retcmd.nMaxHp = m_stAbility[AttrID::MaxHP];
	retcmd.nNowMp = m_nNowMP;
	retcmd.nMaxMp = m_stAbility[AttrID::MaxMP];
	retcmd.btDir = m_btDirection;
	retcmd.dwZSLevel = 0;
	retcmd.dwNoviceGuideId = m_dwNoviceGuideId;
	CopyString(retcmd.szShowName, m_displayName);
	BuildPlayerFeature(retcmd.feature);
	retcmd.transType = transTypeNone;
	obj->SendMsgToMe(&retcmd, sizeof(retcmd));
	AfterSpaceMove(obj);
}

void CPlayerObj::ChangeClientLoadMapData(CGameMap* map)
{
	if (map) {
		stMainPlayerChangeMap newmap;
		int nMapId = map->getMapId();
		newmap.location.mapid = nMapId;
		newmap.dwMapFileID = map->GetMapDataBase()->dwMapFileID;
		strcpy_s(newmap.szMapFileName, sizeof(newmap.szMapFileName) - 1, map->GetMapDataBase()->szMapFileName.c_str());
		newmap.location.ncurx = m_changeMapPos.x;
		newmap.location.ncury = m_changeMapPos.y;
		newmap.dwTmpId = map->GenObjId();
		SetObjectId(newmap.dwTmpId);
		getShowName(newmap.szName, sizeof(newmap.szName) - 1);
		newmap.dir = m_btDirection;
		newmap.minimapidx = map->GetMapDataBase()->nMinMapIdx;
		newmap.lifestate = m_LifeState;
		newmap.dwPlayerCreateTime = m_dwPlayerCreateTime;
		map->getMapShowName(newmap.szMapName, sizeof(newmap.szMapName) - 1);
		newmap.transMoveType = transTypeNone;
		SendMsgToMe(&newmap, sizeof(newmap));
	}
}


void CPlayerObj::MapChanged() {
	FUNCTION_BEGIN;
	if (!IsOnline() && !m_boIsChangeSvr) {
		//发送客户端设置
		SendClientSet(m_boIsNewHum);
	}
	EnterMapNotify(this);
	if (!IsOnline())
	{
		OnUserLoginReady();
	}
	m_dwMapChangedTime = time(NULL);
	testHomeXY();
	if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
		stAutoSetScriptParam auotparm(this);
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr("MapChanged");
	}
	AddLoadedBuff();
	ClearDelayMsg();
	m_tiShi.MapChange();
	stClientIsReady cisready;
	SendMsgToMe(&cisready, sizeof(cisready));
}
void CPlayerObj::AddLoadedBuff()
{
	if (!m_loadBuffData.empty()) {
		for (auto& it : m_loadBuffData) {
			if (!m_cBuff.AddBuff(it.buffid, it.level, it.timeLeft)) {
				g_logger.debug("玩家 %s 上线添加 BUFF %d 失败", getName(), it.buffid);
			}
		}
		m_loadBuffData.clear();
	}
}

void CPlayerObj::SendRefMsg(void* pcmd, int ncmdlen, bool exceptme) {
	if (!exceptme) {
		SendMsgToMe(pcmd, ncmdlen);
	}
	CCreature::SendRefMsg(pcmd, ncmdlen, true);
}

bool  CPlayerObj::Operate(stBaseCmd* pcmd, int ncmdlen, stProcessMessage* pMsg)
{
	FUNCTION_BEGIN;
	bool boRet = false;
	if (ncmdlen < sizeof(stBaseCmd)) {
		return false;
	}
	if (sConfigMgr.openPacketPrint) {
		g_logger.debug("CPlayerObj::Operate(%d,%d)", pcmd->cmd, pcmd->subcmd);
	}
	if (m_isOpenPacketLog)
	{
		char szmemhex[1024 * 7] = { 0 };
		Mem2Hex((char*)pcmd, min(ncmdlen, (sizeof(szmemhex) / 2) - 1024), szmemhex, sizeof(szmemhex));
		g_logger.debug("%s:%s", getName(), szmemhex);
	}

	char szfmdis[256];
	sprintf_s(szfmdis, sizeof(szfmdis) - 1, "CPlayerObj::Operate(%d,%d)", pcmd->cmd, pcmd->subcmd);
	FUNCTION_MONITOR(48, szfmdis);
	switch (pcmd->cmd) {
	case 0:
	{

	}break;
	case CMD_CLIENT_GAMESVR_ITEM:
	{
		doCretItemCmd(pcmd, ncmdlen);
	}break;
	case CMD_CLIENT_GAMESVR_TRADE:
	{
		doCretTradeCmd(pcmd, ncmdlen);
	}
	break;
	case CMD_CLIENT_GAMESVR_MAIL:
	{
		doCretMailCmd(pcmd, ncmdlen);
	}break;
	case CMD_CLIENT_GAMESVR_CONSIGNMENT:
	{
		doConsignCmd(pcmd, ncmdlen);
	}break;
	case CMD_CLIENT_GAMESVR_QUEST:
	{
		doCretQuestCmd(pcmd, ncmdlen);
	}break;
	case CMD_CLIENT_GAMESVR_GROUP:
	{
		doCretGroupCmd(pcmd, ncmdlen);
	}break;
	case CMD_CLIENT_GAMESVR_FRIEND:
	{
		doRelationCmd(pcmd, ncmdlen);
	}break;
	default:
	{
		switch (pcmd->value)
		{
		case stCheckSpeedCmd::_value:
		{
			_CHECK_PACKAGE_LEN(stCheckSpeedCmd, ncmdlen);
			stCheckSpeedCmd* pdstcmd = (stCheckSpeedCmd*)pcmd;
			pdstcmd->setLog(GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value);
			g_logger.forceLog(zLogger::zFORCE, "(%d)测速信号_player %d:%s : r:%d(l:%i64d)", pdstcmd->dwProxyCount, pdstcmd->dwCheckIndex, pdstcmd->szCheckStr, pdstcmd->dwLocalTick, ::GetTickCount64());
			if (!m_pGateUser->isSocketClose()) {
				m_pGateUser->SendProxyCmd(pcmd, ncmdlen);
			}
		}break;
		case stCretClientLog::_value:
		{
			_CHECK_PACKAGE_LEN(stCretClientLog, ncmdlen);
			stCretClientLog* pClientLog = (stCretClientLog*)pcmd;
			if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("ClientLog", pClientLog->szClientLog.getptr());
			}
		}break;
		case stCretAutoKeepHp::_value:
		{
			_CHECK_PACKAGE_LEN(stCretAutoKeepHp, ncmdlen);
			stCretAutoKeepHp* pAutoKeepHp = (stCretAutoKeepHp*)pcmd;
			m_boAutoOpenHp = pAutoKeepHp->boOpen;
			m_dwKeepHp = pAutoKeepHp->dwKeepHp;
			m_dwForceKeepHp = pAutoKeepHp->dwForceKeepHp;
			m_dwHpSet = pAutoKeepHp->dwHpSet;
		}break;
		case stCretGetAbility::_value:
		{
			_CHECK_PACKAGE_LEN(stCretGetAbility, ncmdlen);
			stCretGetAbility* pGetAbility = (stCretGetAbility*)pcmd;
			if (pGetAbility->dwTargetTmpId) {
				CPlayerObj* pTarget = GetEnvir()->GetPlayer(pGetAbility->dwTargetTmpId);
				if (pTarget) {
					pGetAbility->TargetAbility = pTarget->m_stAbility;
					SendMsgToMe(pGetAbility, sizeof(*pGetAbility));
				}
			}
		}break;
		case stCretMove::_value:
		{
			_CHECK_PACKAGE_LEN(stCretMove, ncmdlen);
			if (isDie() || !isClientReady())
			{
				return false;
			}
			m_dwLastOperateTime = time(NULL);
			ULONGLONG tickcount = GetTickCount64();
			if (m_dwMoveIntervalTime <= 333)	// 1.2倍移速加成以上
			{
				tickcount = tickcount + 266;	// 按1.5倍移速减少接收间隔
			}
			if (tickcount >= m_dwSaveMoveTimeForNext) {
				if (m_cDelayDealList.empty()) {
					OnstCretMove(this, (stCretMove*)pcmd, ncmdlen, NULL);
				}
				else {
					//g_logger.error("堆积 count:%d,移动堆积 count:%d", m_cDelayDealList.size(), m_cDelayMoveList.size());
					PushDelayMoveMsg((stCretMove*)pcmd);
					return false;
				}
			}
			else {
				//g_logger.error("移动间隔 count:%d", m_dwSaveMoveTimeForNext);
				PushDelayMoveMsg((stCretMove*)pcmd);

				//g_logger.error("【移动异常】：收到了移动包，但还未到下次可移动时间（玩家名=%s 坐标（%d, %d）->（%d, %d））", getName(), m_nCurrX, m_nCurrY, ((stCretMove*)pcmd)->ncurx, ((stCretMove*)pcmd)->ncury);
				//g_logger.error("【移动修正】：修正玩家 %s 位置坐标为（%d, %d）", getName(), m_nCurrX, m_nCurrY);
				//stCretMoveRet retcmd;
				//fullMoveRet(&retcmd, 0, -1, 0);
				//SendMsgToMe(&retcmd, sizeof(retcmd));

				return false;
			}
		}break;
		case stCretOtherMove::_value:
		{
			_CHECK_PACKAGE_LEN(stCretOtherMove, ncmdlen);
			stCretOtherMove* pDstCmd = (stCretOtherMove*)pcmd;
			if (pDstCmd->btMoveType != emMove && pDstCmd->btMoveType != emMove_Run && pDstCmd->btmovesetp > 0) {
				stCretOtherMove targetcmd;
				targetcmd = *pDstCmd;
				stCretMoveRet retcmd;
				m_moveStep = pDstCmd->btmovesetp;
				fullMoveRet(&retcmd, 0);
				retcmd.dir = pDstCmd->dir;
				retcmd.btMoveType = pDstCmd->btMoveType;
				int nallstep = pDstCmd->btmovesetp;
				int ncux = m_nCurrX;
				int ncuy = m_nCurrY;
				int ndx = 0, ndy = 0;
				bool boSendDelay = true;
				if (nallstep > 0 && !isDie()) {
					CCreature* pCretOne = NULL;
					if (GetEnvir()->GetNextPosition(ncux, ncuy, retcmd.dir, 1, ndx, ndy) && GetEnvir()->CanWalk(this, ndx, ndy, m_nCurrZ, true)) {
						pCretOne = GetEnvir()->GetCretInXY(ndx, ndy);
					}
					else {
						boSendDelay = false;
						retcmd.moveerrorcode = -2;
					}
					ncux = ndx;
					ncuy = ndy;
				}
				else {
					boSendDelay = false;
					retcmd.moveerrorcode = -2;
				}
				if (boSendDelay) {
					int moveStep = 0;
					for (int i = 1; i <= nallstep; i++)
					{
						int ndx1 = ncux;
						int ndy1 = ncuy;
						if (GetEnvir()->GetNextPosition(ncux, ncuy, retcmd.dir, 1, ndx1, ndy1) && GetEnvir()->CanWalk(this, ndx1, ndy1, m_nCurrZ, true))
						{
							ndx = ndx1;
							ndy = ndy1;
							moveStep++;
						}
					}
					if (GetEnvir()->MoveCretTo(this, ndx, ndy, m_nCurrZ, true)) {
						targetcmd.btmovesetp = moveStep;
						int delay = 360;
						//pushDelayMsg(this,&targetcmd,sizeof(targetcmd),delay);
						int delaysec = CALL_LUARET<int>("movestepsec", delay * moveStep, moveStep);
						m_cBuff.AddBuff(1002, 1, delaysec);
						retcmd.location.ncurx = m_nCurrX;
						retcmd.location.ncury = m_nCurrY;
						retcmd.location.ncurz = m_nCurrZ;
						retcmd.dir = (targetcmd.btMoveType == empushbak) ? CGameMap::GetReverseDirection(retcmd.dir) : retcmd.dir;
						retcmd.btmovesetp = moveStep;
						retcmd.moveerrorcode = 0;
					}
					else {
						boSendDelay = false;
						retcmd.moveerrorcode = (BYTE)-3;
					}
				}
				ClearDelayMsg();
				if (boSendDelay)SendRefMsg(&retcmd, sizeof(retcmd));
			}
		}break;
		case stCretAttack::_value:
		{
			_CHECK_PACKAGE_LEN(stCretAttack, ncmdlen);
			if (isDie() || !isClientReady())
			{
				return false;
			}
			ULONGLONG tickcount = GetTickCount64() + 50;
			stCretAttack* newcmd = (stCretAttack*)pcmd;
			auto pMagic = sJsonConfig.GetMagicDataBase(newcmd->dwMagicID, 1);
			if (pMagic)
			{
				uint64_t saveTime = (pMagic->nDamageType == 1) ? m_dwSaveAttackTimeForNext : m_dwSaveReleaseTimeForNext;
				if (tickcount >= saveTime) {
					if (m_cDelayDealList.empty()) {
						OnstCretAttack(this, (stCretAttack*)pcmd, ncmdlen, pMsg);
					}
					else {
						PushDelayAttackMsg((stCretAttack*)pcmd);
						g_logger.error("堆积 count:%d,攻击堆积 count:%d", m_cDelayDealList.size(), m_cDelayAttackList.size());
						return false;
					}
				}
			}
		}break;
		case stCretGetUseItem::_value:
		{
			_CHECK_PACKAGE_LEN(stCretGetUseItem, ncmdlen);
			/*if(!m_Trade.GetTradeState())
			g_logger.error("物品使用time:%d",GetTickCount64());*/
			OnstCretUseItem(this, (stCretGetUseItem*)pcmd, ncmdlen, pMsg);
		}break;
		case stCretChat::_value:
		{
			_CHECK_PACKAGE_LEN(stCretChat, ncmdlen);
			stCretChat* pCretChatCmd = (stCretChat*)pcmd;

			if (this->m_btGmLvl == 0
				&& (pCretChatCmd->szZeroChatMsg.getarraysize() < 0 || pCretChatCmd->szZeroChatMsg.getarraysize() > _MAX_CHAT_LEN_ + 20))
			{
				return false;
			}
			stAutoSetScriptParam autoparm(this);
			/*		bool boChat=CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("BoCanChat",true,((stCretChat*)pcmd)->btChatType);
					if (boChat){*/
			OnstCretChat(this, (stCretChat*)pcmd, ncmdlen, pMsg, false);
			if (CChat::m_dwSendGmManage != 0 && (CChat::m_dwSendGmManage & (1 << (((stCretChat*)pcmd)->btChatType - 1))) != 0) {
				BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
				//stSvrSendGMCmdRet retcmd;
				retcmd->nErrorcode = 0;
				retcmd->svr_marking = GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
				retcmd->dwTrueZoneId = GameService::getMe().m_nTrueZoneid;
				retcmd->btRetType = CHANNEL_MSG;
				retcmd->chat_type = ((stCretChat*)pcmd)->btChatType;

				char szTemp[_MAX_CHAT_LEN_ * 4] = { 0 };
				sprintf_s(szTemp, sizeof(szTemp) - 1, "%s", ((stCretChat*)pcmd)->szZeroChatMsg.getptr());
				szTemp[((stCretChat*)pcmd)->szZeroChatMsg.getarraysize()] = '\0';

				if (retcmd->chat_type == CHAT_TYPE_PRIVATE) {
					if (((stCretChat*)pcmd)->szZeroChatMsg[0] == 0x7B) {
						retcmd->retStr.push_str(vformat("{\"srcappid\":\"%s\",\"srcuserid\":\"%s\",\"srconlyid\":\"%I64d\",\"srclevel\":\"%d\",\"srcip\":\"%s\",\"srcaccount\":\"%s\",\"srcname\":\"%s\",\"dstname\":\"%s\",\"dstonlyid\":\"%I64d\",\"dstlevel\":\"%d\",\"mic\":\"%s\",\"chargehis\":\"%d\"}",
							m_szAppId, m_szUserId, m_i64UserOnlyID, m_dwLevel, inet_ntoa(m_pGateUser->clientip), getAccount(), getName(), ((stCretChat*)pcmd)->szTargetName, ((stCretChat*)pcmd)->i64DestOnlyId, 0, szTemp, m_res[ResID::hischarge]));
					}
					else {
						retcmd->retStr.push_str(vformat("{\"srcappid\":\"%s\",\"srcuserid\":\"%s\",\"srconlyid\":\"%I64d\",\"srclevel\":\"%d\",\"srcip\":\"%s\",\"srcaccount\":\"%s\",\"srcname\":\"%s\",\"dstname\":\"%s\",\"dstonlyid\":\"%I64d\",\"dstlevel\":\"%d\",\"text\":\"%s\",\"chargehis\":\"%d\"}",
							m_szAppId, m_szUserId, m_i64UserOnlyID, m_dwLevel, inet_ntoa(m_pGateUser->clientip), getAccount(), getName(), ((stCretChat*)pcmd)->szTargetName, ((stCretChat*)pcmd)->i64DestOnlyId, 0, szTemp, m_res[ResID::hischarge]));
					}
				}
				else {
					if (((stCretChat*)pcmd)->szZeroChatMsg[0] == 0x7B) {
						retcmd->retStr.push_str(vformat("{\"srcappid\":\"%s\",\"srcuserid\":\"%s\",\"srconlyid\":\"%I64d\",\"srclevel\":\"%d\",\"srcip\":\"%s\",\"srcaccount\":\"%s\",\"srcname\":\"%s\",\"mic\":\"%s\",\"chargehis\":\"%d\"}",
							m_szAppId, m_szUserId, m_i64UserOnlyID, m_dwLevel, inet_ntoa(m_pGateUser->clientip), getAccount(), getName(), szTemp, m_res[ResID::hischarge]));
					}
					else {
						retcmd->retStr.push_str(vformat("{\"srcappid\":\"%s\",\"srcuserid\":\"%s\",\"srconlyid\":\"%I64d\",\"srclevel\":\"%d\",\"srcip\":\"%s\",\"srcaccount\":\"%s\",\"srcname\":\"%s\",\"text\":\"%s\",\"chargehis\":\"%d\"}",
							m_szAppId, m_szUserId, m_i64UserOnlyID, m_dwLevel, inet_ntoa(m_pGateUser->clientip), getAccount(), getName(), szTemp, m_res[ResID::hischarge]));

						/*g_logger.debug(vformat("{\"srcappid\":\"%s\",\"srcuserid\":\"%s\",\"srconlyid\":\"%I64d\",\"srclevel\":\"%d\",\"srcip\":\"%s\",\"srcaccount\":\"%s\",\"srcname\":\"%s\",\"text\":\"%s\"}",
							m_szAppId,m_szUserId,m_i64UserOnlyID, m_dwLevel, inet_ntoa(m_pGateUser->clientip), getAccount(), getName(),((stCretChat*)pcmd)->szZeroChatMsg.getptr()));*/
					}
				}
				//CUserEngine::getMe().SendMsg2GmManageSvr(&retcmd,sizeof(stSvrSendGMCmdRet));
				CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet) + retcmd->retStr.getarraysize());
			}
			/*}
			else {
				SendMsgToMe((stCretChat*)pcmd,sizeof(*pCretChatCmd)+pCretChatCmd->szZeroChatMsg.getarraysize());
			}*/
		}break;
		case stCretActionRet::_value:
		{
			_CHECK_PACKAGE_LEN(stCretActionRet, ncmdlen);
			OnCretAction((stCretActionRet*)pcmd, ncmdlen, pMsg);
		}break;
		case stCretStruckFull::_value:
		{
			_CHECK_PACKAGE_LEN(stCretStruckFull, ncmdlen);
			OnCretStruck((stCretStruckFull*)pcmd, ncmdlen);
		}break;
		case stSetShortCuts::_value:case stDelShortCuts::_value:
		{
			doShortCutsCmd(pcmd, ncmdlen);
		}break;
		case stMapItemEventPick::_value:
		{
			_CHECK_PACKAGE_LEN(stMapItemEventPick, ncmdlen);
			stMapItemEventPick* pDstCmd = (stMapItemEventPick*)pcmd;
			stMapItemEventPick retcmd;
			std::string strlog;
			retcmd = *pDstCmd;
			int pickdis = 3; // 拾取距离
			int privilegelv = m_nVipLevel;
			int nAreaMark = GameService::getMe().m_nTradeid;
			if (privilegelv >= 4) {
				if (GetEnvir()) pickdis = GetEnvir()->isNoCleaner() ? 3 : 5;
			}
			if (abs(pDstCmd->wX - m_nCurrX) <= pickdis && abs(pDstCmd->wY - m_nCurrY) <= pickdis)
			{
				CMapItemEvent* pItemEvent = GetEnvir()->GetItemInXY(pDstCmd->wX, pDstCmd->wY, pDstCmd->i64Id);
				if (pItemEvent && pItemEvent->OwnerItem && pItemEvent->isCanContinue() && pItemEvent->GetDisappearType() == DISAPPEAR_TIME) {
					if (pItemEvent->CanPick(this)) {
						///判断是否能拾取，只判断
						if (pItemEvent->OwnerItem->GetType() != ITEM_TYPE_GOLD) {
							if (pItemEvent->GetOwnerType() == OWNER_NULL) {
								pItemEvent->OwnerItem->SetItemLog("临时拾取物品", this);
							}
							else {
								if (pItemEvent->GetOwnerType() == OWNER_PLAYER) {
									strlog = "playerdropgetitem";
								}
								else {
									strlog = "mondropgetitem";
								}
							}
						}
						if (pItemEvent->OwnerItem->GetType() == ITEM_TYPE_GOLD) {
							CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerPickCcy", this, pItemEvent->OwnerItem->GetItemBaseID(), pItemEvent->OwnerItem->GetItemCount(), strlog.c_str());
							GetEnvir()->DelItemFromMap(pItemEvent->GetX(), pItemEvent->GetY(), pItemEvent);
							retcmd.btErrorCode = PICK_SUCCESS;
						}
						else
						{
							bool isNotToPackage = stRes::IsValidID(pItemEvent->OwnerItem->GetItemBaseID());
							if (isNotToPackage || m_Packet.AddItemToBag(pItemEvent->OwnerItem, true, false, true, (char*)strlog.c_str()))
							{
								if (isNotToPackage) {
									ResChange(static_cast<ResID>(pItemEvent->OwnerItem->GetItemBaseID()), pItemEvent->OwnerItem->GetItemCount(), strlog.c_str());
									CUserEngine::getMe().ReleasePItem(pItemEvent->OwnerItem, __FUNC_LINE__);
								}
								pItemEvent->OwnerItem = NULL;
								pItemEvent->SetDisappearType(DISAPPEAR_ADD);
								GetEnvir()->DelItemFromMap(pItemEvent->GetX(), pItemEvent->GetY(), pItemEvent);
								retcmd.btErrorCode = PICK_SUCCESS;
							}
							else
							{
								retcmd.btErrorCode = PICK_NOBAG;
							}
						}
					}
					else {
						retcmd.btErrorCode = PICK_NOOWNER;
						if (pItemEvent->OwnerItem && pItemEvent->OwnerItem->m_Item.dwBaseID >= 154 && pItemEvent->OwnerItem->m_Item.dwBaseID <= 156) {
							if (m_GuildInfo.dwPowerLevel >= _GUILDMEMBER_POWERLVL_MASTER) {
								retcmd.btErrorCode = PICK_GUILDMASTARNOPICK;
							}
						}
					}
				}
				else {
					retcmd.btErrorCode = PICK_NOITEM;
				}
			}
			else {
				retcmd.btErrorCode = PICK_NOXY;
			}
			SendMsgToMe(&retcmd, sizeof(stMapItemEventPick));
		}break;
		case stMapItemEventBatchPick::_value:
		{
			_CHECK_PACKAGE_LEN(stMapItemEventBatchPick, ncmdlen);
			stMapItemEventBatchPick* pDstCmd = (stMapItemEventBatchPick*)pcmd;
			if (pDstCmd) {
				if (quest_vars_get_var_n("privilegeautopick") == 0 || pDstCmd->PickItemArr.size > 100) {
					g_logger.debug("%d发包攻击大包号:%d,小包号:%d", m_i64UserOnlyID, pDstCmd->cmd, pDstCmd->subcmd);
					return false;
				}
				std::string strlog;
				stPickItem* pickitem = NULL;
				//bool itemisbind = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("pickitemisbind", true, this);
				for (int i = 0; i < pDstCmd->PickItemArr.size; i++) {
					pickitem = &pDstCmd->PickItemArr[i];
					if (pickitem) {
						CMapItemEvent* pItemEvent = GetEnvir()->GetItemInXY(pickitem->wX, pickitem->wY, pickitem->i64Id);
						if (pItemEvent && pItemEvent->OwnerItem && pItemEvent->isCanContinue() && pItemEvent->GetDisappearType() == DISAPPEAR_TIME) {
							if (pItemEvent->CanPick(this) && !pItemEvent->boIsPlayerDrop) {
								if (pItemEvent->OwnerItem->GetType() != ITEM_TYPE_GOLD) {
									if (pItemEvent->GetOwnerType() == OWNER_NULL) {
										pItemEvent->OwnerItem->SetItemLog("临时拾取物品", this);
									}
									else {
										if (pItemEvent->GetOwnerType() == OWNER_PLAYER) {
											strlog = "playerdropgetitem";
										}
										else {
											strlog = "mondropgetitem";
										}
									}
								}
								if (pItemEvent->OwnerItem->GetType() == ITEM_TYPE_GOLD) {
									int nGold = pItemEvent->OwnerItem->GetItemCount();
									CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerPickCcy", this, pItemEvent->OwnerItem->GetItemBaseID(), pItemEvent->OwnerItem->GetItemCount(), strlog.c_str());
									GetEnvir()->DelItemFromMap(pItemEvent->GetX(), pItemEvent->GetY(), pItemEvent);
								}
								else
								{
									bool isNotToPackage = stRes::IsValidID(pItemEvent->OwnerItem->GetItemBaseID());
									if (isNotToPackage || m_Packet.AddItemToBag(pItemEvent->OwnerItem, true, false, true, (char*)strlog.c_str()))
									{
										if (isNotToPackage) {
											ResChange(static_cast<ResID>(pItemEvent->OwnerItem->GetItemBaseID()), pItemEvent->OwnerItem->GetItemCount(), strlog.c_str());
											CUserEngine::getMe().ReleasePItem(pItemEvent->OwnerItem, __FUNC_LINE__);
										}
										pItemEvent->OwnerItem = NULL;
										pItemEvent->SetDisappearType(DISAPPEAR_ADD);
										GetEnvir()->DelItemFromMap(pItemEvent->GetX(), pItemEvent->GetY(), pItemEvent);
									}
								}
							}
						}
					}
				}
			}
			SendMsgToMe(pDstCmd, sizeof(stMapItemEventBatchPick) + pDstCmd->PickItemArr.getarraysize());
		}break;
		case stCretBuffFeature::_value:
		{
			_CHECK_PACKAGE_LEN(stCretBuffFeature, ncmdlen);
			UpdateAppearance(FeatureIndex::boFeature, m_cBuff.BuffFeature());
		}break;
		case stCretPetOrder::_value:
		{
			_CHECK_PACKAGE_LEN(stCretPetOrder, ncmdlen);
			stCretPetOrder* pDstCmd = (stCretPetOrder*)pcmd;
			if (pDstCmd->btOrder == emPet_Stop) {
				m_Petm.SetPetState(emPet_Stop);
			}
			else if (pDstCmd->btOrder == emPet_Follow) {
				m_Petm.SetPetState(emPet_Follow);
			}
			else if (pDstCmd->btOrder == emPet_Attack) {
				m_Petm.SetPetState(emPet_Attack);
			}
			SendMsgToMe(pDstCmd, sizeof(stCretPetOrder));
		}break;
		case stCretPkModel::_value:
		{
			_CHECK_PACKAGE_LEN(stCretPkModel, ncmdlen);
			stCretPkModel* pDstCmd = (stCretPkModel*)pcmd;
			ChangePkMode(pDstCmd->btPkModel);
		}break;
		case stSkillSetCmd::_value:
		{
			_CHECK_PACKAGE_LEN(stSkillSetCmd, ncmdlen);
			stSkillSetCmd* pDstCmd = (stSkillSetCmd*)pcmd;
			stMagic* pMagic = m_cMagic.findskill(pDstCmd->dwMagicId);
			if (pMagic) {
				if (pMagic->savedata.boLockChange) { pMagic->savedata.boLocked = pDstCmd->boLocked; }
				if (pMagic->savedata.boContinuousCastChange) { pMagic->savedata.boContinuousCasting = pDstCmd->boContinuousCasting; }
				pDstCmd->boLocked = pMagic->savedata.boLocked;
				pDstCmd->boContinuousCasting = pMagic->savedata.boContinuousCasting;
				SendMsgToMe(pDstCmd, sizeof(stSkillSetCmd));
				m_cMagic.SetSkillSet(&pMagic->savedata);
			}
		}break;
		case stCretClientSet::_value:
		{
			_CHECK_PACKAGE_LEN(stCretClientSet, ncmdlen);
			stCretClientSet* pDstCmd = (stCretClientSet*)pcmd;
			if (pDstCmd->DataArr.getarraysize() < 4096) {
				m_vClientSet.resize(pDstCmd->DataArr.getarraysize());
				if (m_vClientSet.size()) {
					memcpy(&m_vClientSet[0], &pDstCmd->DataArr[0], pDstCmd->DataArr.getarraysize());
				}
			}

		}break;
		case stNoviceGuide::_value:
		{
			_CHECK_PACKAGE_LEN(stNoviceGuide, ncmdlen);
			stNoviceGuide* pDstCmd = (stNoviceGuide*)pcmd;
			m_dwNoviceGuideId = pDstCmd->dwGuideId ? pDstCmd->dwGuideId : 0;
		}break;
		default:
		{
			boRet = CCreature::Operate(pcmd, ncmdlen, pMsg);
		}break;
		}
	}break;
	}
	return boRet;
}

bool CPlayerObj::CanWalk() {
	FUNCTION_BEGIN;
	return __super::CanWalk();
}



void CPlayerObj::SendClientSet(bool boisnewhuman) {
	FUNCTION_BEGIN;
	BUFFER_CMD(stCretClientSet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->boNewHuman = boisnewhuman;
	if (m_vClientSet.size()) {
		retcmd->DataArr.push_back((char*)&m_vClientSet[0], m_vClientSet.size(), __FUNC_LINE__);
		retcmd->boNewHuman = false;
	}
	SendMsgToMe(retcmd, sizeof(*retcmd) + retcmd->DataArr.getarraysize());
}

bool CPlayerObj::isGuildWar() {
	FUNCTION_BEGIN;
	return (m_WarGuildSet.size()) ? true : false;
}

bool CPlayerObj::isEnemy(CCreature* pTarget) {
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true, "");
	if (pTarget && pTarget != this) {
		if (pTarget->isNpc()) return false;				//安全模式2 对所有单位有效
		CPlayerObj* pTargetPlayer = pTarget->getPlayer();
		if (pTargetPlayer) {
			if (pTarget->m_dwLevel < sConfigMgr.PlayerProtectionlv) return false; // 新手保护期
			if (GetEnvir()->isNoForcePk() || pTargetPlayer->m_bFakePlayer) return false;
			if (!GetEnvir()->isNoSafeZone() && GetEnvir()->getSafeType(m_nCurrX, m_nCurrY) != 0) return false;
			if (!pTarget->GetEnvir()->isNoSafeZone() && pTarget->GetEnvir()->getSafeType(pTarget->m_nCurrX, pTarget->m_nCurrY) != 0) return false;
			if (pTarget->quest_vars_get_var_n("PkProtectCard") > 0) { // pk保护卡
				bool boInvalidArea = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("isPkProtectInvalidArea", false, pTargetPlayer, pTargetPlayer->GetEnvir()->getMapId());
				if (!boInvalidArea) return false;
			}
		}
		if (pTarget->isPet()) {
			if (pTarget->toPet()->getMaster() == this) return false;	//自己的宠物
		}
		if (this->quest_vars_get_var_n("countertarget") == pTarget->GetObjectId()) //反击中
			return true;
		if (m_btBattleCamp != emBattle_Null) {
			if (pTarget->m_btBattleCamp != m_btBattleCamp && !pTarget->isNpc() && !pTarget->m_boBattleWuDi) {
				return true;
			}
		}
		bool bRet = false;
		switch (m_btPkModel)
		{
		case PKMODEL_PEACEMODE: //和平模式：只对怪物进行攻击起效（不包括道士的宝宝、被捕获的怪物）
			if (!pTargetPlayer && pTarget->isMonster())  bRet = true;
			break;
		case PKMODEL_TEAMMODE: //队伍模式：对怪物以及非本队伍的玩家进行攻击起效
			if (pTargetPlayer) {
				if (!(pTargetPlayer == this || (m_GroupInfo.dwGroupId && m_GroupInfo.dwGroupId == pTargetPlayer->m_GroupInfo.dwGroupId))) bRet = true;
			}
			else {
				if (pTarget->isMonster())  bRet = true;
			}
			break;
		case PKMODEL_GUILDMODE: //行会模式：对怪物、非本行会的玩家进行攻击起效
			if (pTargetPlayer) {
				if (!(pTargetPlayer == this || (m_GuildInfo.dwGuildId && pTargetPlayer->m_GuildInfo.dwGuildId == m_GuildInfo.dwGuildId) || GuildAllianceCheck(pTargetPlayer))) {
					if (m_btBattleCamp && pTarget->m_btBattleCamp == m_btBattleCamp && !pTarget->isNpc() && !pTarget->m_boBattleWuDi) {
						return false;
					}
					bRet = true;
				}
			}
			else {
				if (pTarget->isMonster()) {
					bRet = true;
				}
			}
			break;
		case PKMODEL_GUILDWARMODE: //行会战模式：对怪物以及非本行会的行会战玩家进行攻击起效
			if (pTargetPlayer) {
				if (pTargetPlayer != this && GuildWarCheck(pTargetPlayer)) bRet = true;
			}
			else {
				if (pTarget->isMonster())  bRet = true;
			}
			break;
		case PKMODEL_GOODANDEVILMODE: //善恶模式：对怪物以及无保护状态的玩家进行攻击起效（灰名，黄名，红名玩家,包含灰名和黄名，红名玩家的宝宝）
			if (pTargetPlayer) {
				if (pTargetPlayer != this && (pTargetPlayer->m_dwPkRunTime || pTargetPlayer->m_res[ResID::pk])) bRet = true;
			}
			else {
				if (pTarget->isMonster())  bRet = true;
			}
			break;
		case PKMODEL_ALLTHEMODE: //全体模式：对所有怪物和玩家进行攻击起效
		{
			return true;
		}break;
		}

		return bRet;
	}
	return false;
}

bool CPlayerObj::DamageSpell(int64_t nDamage) {
	FUNCTION_BEGIN;
	if (m_NoCdMode) {
		return true;
	}

	bool boRet = CCreature::DamageSpell(nDamage);
	if (boRet) {
		stCretSpellChange retcmd;
		retcmd.dwTempId = GetObjectId();
		retcmd.nChangeMp = nDamage;
		retcmd.nMp = m_nNowMP;
		retcmd.nMaxMp = m_stAbility[AttrID::MaxMP];
		SendMsgToMe(&retcmd, sizeof(retcmd));
		//GroupInfoChanged();
	}
	return boRet;
}

bool CPlayerObj::luaPlayerAttack(int nX, int nY, DWORD dwMagicID) {
	FUNCTION_BEGIN;
	return PlayerAttack(nX, nY, 0, dwMagicID);
	//g_logger.error("玩家 %s 尚未习得技能 %d", getName(), dwMagicID);
}

bool CPlayerObj::PlayerAttack(int nX, int nY, DWORD dwTemId, DWORD dwMagicID) {
	FUNCTION_BEGIN;
	stCretAttack attackcmd;
	attackcmd.dwTmpId = GetObjectId();
	attackcmd.stTarget.dwtmpid = dwTemId;
	attackcmd.stTarget.xDes = nX;
	attackcmd.stTarget.yDes = nY;
	attackcmd.dwMagicID = dwMagicID;
	stMagic* pMagic = m_cMagic.findskill(dwMagicID);
	if (pMagic)
	{
		pMagic->doSkill(this, &attackcmd);
		return true;
	}
	//g_logger.error("玩家 %s 尚未习得技能 %d", getName(), dwMagicID);
	return false;
}

bool CPlayerObj::OnstCretAttack(CPlayerObj* player, stCretAttack* pcmd, unsigned int ncmdlen, stProcessMessage* param) {
	FUNCTION_BEGIN;
	stMagic* pMagic = player->m_cMagic.findskill(pcmd->dwMagicID);
	if (pMagic) {
		player->m_btDirection = player->GetEnvir()->GetNextDirection(player->m_nCurrX, player->m_nCurrY, pcmd->stTarget.xDes, pcmd->stTarget.yDes);
		return pMagic->doSkill(player, pcmd);
	}
	else {
		sendTipMsgByXml(player, vformat("玩家使用未习得的技能进行了攻击（玩家名=%s 技能ID=%d）", player->getName(), pcmd->dwMagicID));
		g_logger.error("玩家使用未习得的技能进行了攻击（玩家名=%s 技能ID=%d）", player->getName(), pcmd->dwMagicID);
		return false;
	}
}

void CPlayerObj::PushDelayAttackMsg(stCretAttack* param) {
	if (!param)
		return;
	if (m_cDelayAttackList.size() >= 10)
	{
		g_logger.debug("攻击堆积=%d", m_cDelayAttackList.size());
		return;
	}
	stCretAttack* attack = CUserEngine::getMe().m_cretAttackPool.construct(*param);
	m_cDelayAttackList.push_back(attack);
	auto pMagic = sJsonConfig.GetMagicDataBase(attack->dwMagicID, 1);
	if (!pMagic)
	{
		m_cDelayDealList.push_back(2);
		return;
	}
	if (pMagic->nDamageType == 1) {
		m_cDelayDealList.push_back(2);
	}
	else {
		m_cDelayDealList.push_back(3);
	}
	return;
}

void CPlayerObj::PushDelayMoveMsg(stCretMove* param) {
	FUNCTION_BEGIN;
	if (param)
	{
		if (m_cDelayMoveList.size() < 100) {
			stCretMove* move = CUserEngine::getMe().m_cretMovePool.construct(*param);
			m_cDelayMoveList.push_back(move);
			m_cDelayDealList.push_back(1);
		}
	}
}

void CPlayerObj::ClearDelayMsg(int flag)
{
	FUNCTION_BEGIN;
	if (flag == 1 || flag == 0)
	{
		if (!m_cDelayMoveList.empty()) {
			for (auto it : m_cDelayMoveList)
			{
				CUserEngine::getMe().m_cretMovePool.destroy(it);
			}
			m_cDelayMoveList.clear();
		}
	}
	if (flag == 2 || flag == 0)
	{
		if (!m_cDelayAttackList.empty()) {
			for (auto it : m_cDelayAttackList)
			{
				CUserEngine::getMe().m_cretAttackPool.destroy(it);
			}
			m_cDelayAttackList.clear();
		}
	}
	if (flag == 0)
	{
		m_cDelayDealList.clear();
	}
}

bool CPlayerObj::OnstCretMove(CPlayerObj* player, stCretMove* pcmd, unsigned int ncmdlen, stProcessMessage* param) {
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true, "");
	if (ncmdlen >= sizeof(stCretMove)) {
		auto curMap = player->GetEnvir();
		if (!curMap)
			return false;
		int ncua = pcmd->ncura;
		int nCheck = HIWORD(ncua);
		if (pcmd->ncurx * pcmd->dir != nCheck) {
			return true;
		}
		player->m_nMoveIv = LOWORD(ncua);
		BYTE btmovetype = pcmd->btMoveType;
		if (btmovetype != emMove && btmovetype != emMove_Run) {
			btmovetype = emMove;
		}
		int nIntervalTime = player->m_dwMoveIntervalTime;
		if (btmovetype == emMove) {
			nIntervalTime = player->m_dwWalkIntervalTime;
		}
		switch (btmovetype) {
		case emMove:
		case emMove_Run:
		default:
			if (!player->m_cDelayMoveList.empty()) {
				if (GetTickCount64() - player->m_dwSaveMoveTimeForNext >= 1000 && player->m_cDelayMoveList.size() >= 5)
				{
					player->m_dwSaveMoveTimeForNext = GetTickCount64() + nIntervalTime;
				}
				else {
					player->m_dwSaveMoveTimeForNext += nIntervalTime;
				}
			}
			else {
				player->m_dwSaveMoveTimeForNext = GetTickCount64() + nIntervalTime;//m_dwMoveIntervalTime 受BUFF状态影响 
			}
			break;
		case emCollision://冲撞
			if (!player->m_cDelayMoveList.empty()) {
				player->m_dwSaveMoveTimeForNext += sConfigMgr.sRushTime;
			}
			else {
				player->m_dwSaveMoveTimeForNext = GetTickCount64() + sConfigMgr.sRushTime;
			}
			break;
		case emReverse://后退
			if (!player->m_cDelayMoveList.empty()) {
				player->m_dwSaveMoveTimeForNext += sConfigMgr.sBackTime;
			}
			else {
				player->m_dwSaveMoveTimeForNext = GetTickCount64() + sConfigMgr.sBackTime;
			}
			break;
		}
		player->m_moveStep = pcmd->btsetp;
		player->m_moveType = pcmd->btMoveType;
		if (pcmd->dir >= DRI_NUM) {
			return false;
		}
		if (pcmd->btsetp == 2 && player->m_nNowPP < player->m_nPpMoveCost) {
			return false;
		}
		if (pcmd->btsetp <= 0) {
			return true;
		}
		int nsetp = pcmd->btsetp;
		player->m_btDirection = pcmd->dir;
		player->m_moveStep = pcmd->btsetp;
		if (player->MoveTo(player->m_btDirection, nsetp, pcmd->ncurx, pcmd->ncury, true)) {
			if (nsetp == 2) {
				player->m_isRun = true;
				player->StatusValueChange(stCretStatusValueChange::pp, -player->m_nPpMoveCost, "移动消耗体力");
				player->m_i64RunSec = time(nullptr) + 1;
			}
			player->m_dwSaveMoveTimeBefore = GetTickCount64();
		}
	}
	return true;
}

bool CPlayerObj::OnstCretChat(CPlayerObj* player, stCretChat* pcmd, unsigned int ncmdlen, stProcessMessage* param, bool boStallBack)
{
	FUNCTION_BEGIN;

	if (sizeof(stCretChat) + pcmd->szZeroChatMsg.size > ncmdlen) {
		return false;
	}

	char szTemp[_MAX_CHAT_LEN_ * 4] = { 0 };
	sprintf_s(szTemp, sizeof(szTemp) - 1, "%s", pcmd->szZeroChatMsg.getptr());
	szTemp[pcmd->szZeroChatMsg.getarraysize()] = '\0';
	str_del(szTemp, '%');
	if (szTemp[0] == '@') {

		do
		{
			stDelayChatCMD* pchatcmd = CLD_DEBUG_NEW stDelayChatCMD();
			pchatcmd->playerName = player->getName();
			pchatcmd->szCMD = &szTemp[1];
			pchatcmd->toallsvr = false;

			CUserEngine::getMe().m_delaychatcmdlist.Push(pchatcmd);
			return true;
		} while (false);

		//return CChat::sendGmCmd(player,false,player->getName(),player->m_btGmLvl,_GMCMD_INPUT_GMINGAME_,&szTemp[1]);
	}
	else if (szTemp[0] == '!') {
		if (szTemp[1] == '@') {
			if (szTemp[2] != '!') {

				do
				{
					stDelayChatCMD* pchatcmd = CLD_DEBUG_NEW stDelayChatCMD();
					pchatcmd->playerName = player->getName();
					pchatcmd->szCMD = &szTemp[2];
					pchatcmd->toallsvr = true;

					CUserEngine::getMe().m_delaychatcmdlist.Push(pchatcmd);
					return true;
				} while (false);

				//return CChat::sendGmCmd(player,true,player->getName(),player->m_btGmLvl,_GMCMD_INPUT_GMINGAME_,&szTemp[2]);
			}
		}
		else {
			return CChat::sendGmToAll(true, true, player->getName(), player->m_btGmLvl, &szTemp[1]);
		}
	}

	if (szTemp[0] == '@') {
		return true;
	}
	bool boLeader = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("CheckNameGm", false, player->getName());
	if (pcmd->btChatType < 15 && pcmd->btChatType != CHAT_TYPE_GM && pcmd->btChatType != CHAT_TYPE_GMCMD && pcmd->btChatType != CHAT_TYPE_SYSTEMNOTICE && pcmd->btChatType != CHAT_TYPE_SYSTEM) {
		auto& chatConfig = sJsonConfig.m_chatConfig;
		if ((chatConfig[pcmd->btChatType].nVipLevel >= 0 && player->m_nVipLevel >= 0) || boLeader) {
			if (CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("SMRZ_Check", true, player) || boLeader) {
				std::shared_ptr<stItemDataBase> pItemBaseData;
				CItem* pFoundItem = NULL;
				if (0 != chatConfig[pcmd->btChatType].dwItemId && !player->m_btLeader) {
					pItemBaseData = sJsonConfig.GetItemDataById(chatConfig[pcmd->btChatType].dwItemId);
					pFoundItem = player->m_Packet.FindItemInBagByBaseID(chatConfig[pcmd->btChatType].dwItemId);
				}
				if ((0 == chatConfig[pcmd->btChatType].dwItemId) ||  //no need item
					(0 != chatConfig[pcmd->btChatType].dwItemId && NULL != pFoundItem) || //need item can be found
					(player->m_btLeader)) {
					if (player) {
						DWORD dwMaxChatLength = chatConfig[pcmd->btChatType].dwMaxChatLength;
						if (dwMaxChatLength > 0 && strlen(szTemp) > dwMaxChatLength) {
							CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(RES_LANG_CHAT_MSGTOOLONG), safe_max((DWORD)1, (DWORD)(dwMaxChatLength / 2)));

							return false;
						}

						if (timeGetTime() - player->m_dwLastChatTime[pcmd->btChatType] > chatConfig[pcmd->btChatType].dwTime * 1000) {
							player->m_dwLastChatTime[pcmd->btChatType] = timeGetTime();
							player->m_dwChatCount[pcmd->btChatType] = 0;
						}
						if (player->m_dwChatCount[pcmd->btChatType] >= chatConfig[pcmd->btChatType].dwCount) {
							CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(RES_LANG_CHAT_FAST));
							return false;
						}
						player->m_dwChatCount[pcmd->btChatType]++;
					}

					//delete chat need item
					if (0 != chatConfig[pcmd->btChatType].dwItemId && NULL != pFoundItem && !player->m_btLeader)
					{
						auto itemBase = pFoundItem->GetItemDataBase();
						if (!itemBase) return false;
						int privilegelv = player->m_nVipLevel;
						if (itemBase->nPrivilegeUseLimit && privilegelv < itemBase->nPrivilegeUseLimit) {
							CPlayerObj::sendTipMsgByXml(player, GameService::getMe().GetStrRes(3, "bag"));
							return false;
						}
						if (player->m_Packet.DeleteItemInBag(pFoundItem, 1, true)) {
							if (NULL != pItemBaseData && (emChatType)pcmd->btChatType != CHAT_TYPE_SPEAKER) {
								CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(RES_LANG_CHAT_HAVEITEMCONSUMPTION), pItemBaseData->szName);
							}
						}
						else { //?
							if (NULL != pItemBaseData) {
								CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(RES_LANG_CHAT_HAVEITEM), pItemBaseData->szName);
							}

							return false;
						}
					}
				}
				else {
					if (NULL != pItemBaseData) {
						CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(RES_LANG_CHAT_HAVEITEM), pItemBaseData->szName);
					}

					return false;
				}
			}
			else {
				if (pcmd->btChatType == CHAT_TYPE_SPEAKER)
					CChat::sendSystemByType(player->getName(), CHAT_TYPE_WORLD, GameService::getMe().GetStrRes(9, "chat"));
				else
					CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(9, "chat"));
				return false;
			}
		}
		else {
			if (chatConfig[pcmd->btChatType].nVipLevel) {
				CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(3, "chat"),
					chatConfig[pcmd->btChatType].dwLevel,
					GameService::getMe().GetStrRes(9 + chatConfig[pcmd->btChatType].nVipLevel, "chat"),
					chatConfig[pcmd->btChatType].szChatType);
			}
			else {
				CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(6, "chat"),
					chatConfig[pcmd->btChatType].dwLevel,
					chatConfig[pcmd->btChatType].szChatType);
			}
			return false;
		}
	}
	chatlogger.debug("type:%d,src:%s gmlvl:%d,desc:%s,msg:%s", pcmd->btChatType, player->getName(), player->m_btGmLvl, pcmd->szTargetName, szTemp);
	if (pcmd->btChatType != CHAT_TYPE_PRIVATE) {
		std::string msgstr = std::string(pcmd->szZeroChatMsg.getptr());
		replace_all(msgstr, "'", "''");
		GameService::getMe().Send2LogSvr(_SERVERLOG_GAMECHAT_, 0, 0, player, "%d, %d, \'%s\', \'%s\', %I64d, %d, \'%s\', \'%s\', \'%s\', %I64d, %d, \'%s\', \'%s\'",
			pcmd->btChatType,
			player->m_btGmLvl,
			player->getAccount(),
			player->getName(),
			player->m_i64UserOnlyID,
			player->m_dwLevel,
			inet_ntoa(player->clientip),
			"",
			pcmd->szTargetName,
			pcmd->i64DestOnlyId,
			0,
			"",
			msgstr.c_str());
	}
	switch (pcmd->btChatType)
	{
	case CHAT_TYPE_QQ:
	{
		if (CUserEngine::getMe().isCrossSvr())return false;
		return CChat::sendQQPrivate(player, pcmd->szTargetName, szTemp);
	}break;
	case CHAT_TYPE_PRIVATE:
	{
		if (CUserEngine::getMe().isCrossSvr()) {
			CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(14, "chat"));
			return false;
		}
		return CChat::sendPrivate(player, pcmd->szTargetName, szTemp);
	}break;
	case CHAT_TYPE_REFMSG:
	{
		if (CUserEngine::getMe().isCrossSvr()) {
			CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(14, "chat"));
			return false;
		}
		return CChat::sendRefMsg(player, szTemp);
	}break;
	case CHAT_TYPE_GROUP:
		if (player->m_GroupInfo.dwGroupId)
		{
			if (CUserEngine::getMe().isCrossSvr()) {
				CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(14, "chat"));
				return false;
			}
			CChat::sendGroupMsg((char*)player->getName(), NULL, player->m_GroupInfo.dwGroupId, szTemp);
			return true;
		}
		return false;
	case CHAT_TYPE_CLAN:
	{
		if (CUserEngine::getMe().isCrossSvr() && player->m_wSrcTrade && player->m_wSrcTrade != GameService::getMe().m_nTradeid) return false;
		return CChat::sendClanMsg(player, szTemp);
	}break;
	case CHAT_TYPE_PRINCES:
		return CChat::sendPrincesMsg(player, szTemp);
	case CHAT_TYPE_WORLD:
	{
		if (CUserEngine::getMe().isCrossSvr()) {
			CChat::sendSystemByType(player->getName(), (emChatType)pcmd->btChatType, GameService::getMe().GetStrRes(14, "chat"));
			return false;
		}
		return CChat::SendWorld(player, szTemp);
	}break;
	case CHAT_TYPE_WORLD_TEAM:
	{
		if (CUserEngine::getMe().isCrossSvr()) return false;
		return CChat::SendWorldTeam(player, szTemp);
	}break;
	case CHAT_TYPE_RUMORS:
	{
		if (CUserEngine::getMe().isCrossSvr())return false;
		return CChat::sendRumorsMsg(player, szTemp);
	}break;
	case CHAT_TYPE_BOHOU:
	{
		if (CUserEngine::getMe().isCrossSvr())return false;
		return CChat::sendBohouChat(player, szTemp);
	}break;
	case CHAT_TYPE_DEAL:
	{
		if (CUserEngine::getMe().isCrossSvr())return false;
		return CChat::sendTradeChat(player, szTemp);
	}break;
	case CHAT_TYPE_SPEAKER:
	{
		if (CUserEngine::getMe().isCrossSvr())return false;
		return CChat::sendSpeaker(player, szTemp);
	}break;
	return false;
	}
	return true;
}

bool CPlayerObj::OnstCretUseItem(CPlayerObj* player, stCretGetUseItem* pcmd, unsigned int ncmdlen, stProcessMessage* param)
{
	FUNCTION_BEGIN;
	if (player) {
		if (!player->GetEnvir()->isNoUseItem()) {
			player->m_Packet.ServerGetUseItem(pcmd->i64id, pcmd->dwCretTmpId, player->GetObjectId());
		}
		else {
			CChat::sendCenterMsg((char*)player->getName(), GameService::getMe().GetStrRes(RES_LANG_ITEM_NOUSEITEM));
		}
	}
	return true;
}

bool  CPlayerObj::doCretItemCmd(stBaseCmd* pcmd, int ncmdlen)
{
	FUNCTION_BEGIN;
	return m_Packet.doCretCmd(pcmd, ncmdlen);
}

bool CPlayerObj::doCretTradeCmd(stBaseCmd* pcmd, int ncmdlen)
{
	FUNCTION_BEGIN;
	return m_Trade.doCretCmd(pcmd, ncmdlen);
}

bool CPlayerObj::doCretQuestCmd(stBaseCmd* pcmd, int ncmdlen) {	//客户端提交任务
	FUNCTION_BEGIN;
	return m_QuestList.doQuestCmd(this, pcmd, ncmdlen);
}

bool CPlayerObj::doCretGroupCmd(stBaseCmd* pcmd, int ncmdlen)
{
	FUNCTION_BEGIN;
	return true;
}

bool CPlayerObj::doCretGroupFromSuperSvr(stBaseCmd* pcmd, int ncmdlen) {
	FUNCTION_BEGIN;
	return true;
}

bool CPlayerObj::doShortCutsCmd(stBaseCmd* pcmd, int ncmdlen) {
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case stSetShortCuts::_value:
	{
		_CHECK_PACKAGE_LEN(stSetShortCuts, ncmdlen);
		stSetShortCuts* pDstCmd = (stSetShortCuts*)pcmd;
		pDstCmd->ErrorCode = SETSHORTCUTS_SUCCESS;
		if (pDstCmd->oldrow == 255 && pDstCmd->oldcol == 255) {
			if (pDstCmd->shortcuts.emShortCuts == SHORTCUTS_ITEM) {
				stShortCuts* pItemShortCuts = m_ShortCut.FindShortCuts(pDstCmd->shortcuts.i64Id);
				if (pItemShortCuts) {
					stShortCuts ItemShort = *pItemShortCuts;
					if (m_ShortCut.DeleteShortCuts(pItemShortCuts->btRow, pItemShortCuts->btCol)) {
						pItemShortCuts = NULL;
						stDelShortCuts DelShortCmd;
						DelShortCmd.ErrorCode = SETSHORTCUTS_SUCCESS;
						DelShortCmd.shortcuts = ItemShort;
						SendMsgToMe(&DelShortCmd, sizeof(DelShortCmd));
					}
				}
			}
			if (!m_ShortCut.AddShortCuts(pDstCmd->shortcuts.i64Id, pDstCmd->shortcuts.emShortCuts, pDstCmd->shortcuts.btShortCuts, pDstCmd->shortcuts.btRow, pDstCmd->shortcuts.btCol)) {
				pDstCmd->ErrorCode = SETSHORTCUTS_NOSET;
			}
		}
		else if (pDstCmd->oldrow == pDstCmd->shortcuts.btRow && pDstCmd->oldcol == pDstCmd->shortcuts.btCol) {
			if (pDstCmd->shortcuts.emShortCuts == SHORTCUTS_ITEM) {
				stShortCuts* pItemShortCuts = m_ShortCut.FindShortCuts(pDstCmd->shortcuts.i64Id);
				if (pItemShortCuts) {
					stShortCuts ItemShort = *pItemShortCuts;
					if (m_ShortCut.DeleteShortCuts(pItemShortCuts->btRow, pItemShortCuts->btCol)) {
						pItemShortCuts = NULL;
						stDelShortCuts DelShortCmd;
						DelShortCmd.ErrorCode = SETSHORTCUTS_SUCCESS;
						DelShortCmd.shortcuts = ItemShort;
						SendMsgToMe(&DelShortCmd, sizeof(DelShortCmd));
					}
				}
			}
			if (!m_ShortCut.AddShortCuts(pDstCmd->shortcuts.i64Id, pDstCmd->shortcuts.emShortCuts, pDstCmd->shortcuts.btShortCuts, pDstCmd->shortcuts.btRow, pDstCmd->shortcuts.btCol)) {
				pDstCmd->ErrorCode = SETSHORTCUTS_NOSET;
			}
		}
		else {
			stShortCuts* pShortCuts = m_ShortCut.FindShortCuts(pDstCmd->oldrow, pDstCmd->oldcol);
			if (pShortCuts) {
				stShortCuts* pFindShortCuts = m_ShortCut.FindShortCuts(pDstCmd->shortcuts.btRow, pDstCmd->shortcuts.btCol);
				if (pFindShortCuts) {
					stShortCuts FindShort = *pFindShortCuts;
					if (m_ShortCut.DeleteShortCuts(pFindShortCuts->btRow, pFindShortCuts->btCol)) {
						stShortCuts Short = *pShortCuts;
						if (m_ShortCut.DeleteShortCuts(pShortCuts->btRow, pShortCuts->btCol)) {
							if (!m_ShortCut.AddShortCuts(FindShort.i64Id, FindShort.emShortCuts, Short.btShortCuts, Short.btRow, Short.btCol)) {
								pDstCmd->ErrorCode = SETSHORTCUTS_NOSET;
							}
							else {
								stSetShortCuts retcmd;
								retcmd = *pDstCmd;
								retcmd.shortcuts.i64Id = FindShort.i64Id;
								retcmd.shortcuts.emShortCuts = FindShort.emShortCuts;
								retcmd.shortcuts.btShortCuts = Short.btShortCuts;
								retcmd.shortcuts.btRow = Short.btRow;
								retcmd.shortcuts.btCol = Short.btCol;
								retcmd.oldrow = 255;
								retcmd.oldcol = 255;
								SendMsgToMe(&retcmd, sizeof(stSetShortCuts));
							}
							if (!m_ShortCut.AddShortCuts(Short.i64Id, Short.emShortCuts, FindShort.btShortCuts, FindShort.btRow, FindShort.btCol)) {
								pDstCmd->ErrorCode = SETSHORTCUTS_NOSET;
							}
							else {
								stSetShortCuts retcmd;
								retcmd = *pDstCmd;
								retcmd.shortcuts.i64Id = Short.i64Id;
								retcmd.shortcuts.emShortCuts = Short.emShortCuts;
								retcmd.shortcuts.btShortCuts = FindShort.btShortCuts;
								retcmd.shortcuts.btRow = FindShort.btRow;
								retcmd.shortcuts.btCol = FindShort.btCol;
								retcmd.oldrow = 255;
								retcmd.oldcol = 255;
								SendMsgToMe(&retcmd, sizeof(stSetShortCuts));
							}
							return true;
						}
						else {
							m_ShortCut.AddShortCuts(&FindShort);
							pDstCmd->ErrorCode = SETSHORTCUTS_NOSET;
						}
					}
				}
				else {
					stShortCuts Short = *pShortCuts;
					if (m_ShortCut.DeleteShortCuts(pShortCuts->btRow, pShortCuts->btCol)) {
						if (!m_ShortCut.AddShortCuts(pDstCmd->shortcuts.i64Id, pDstCmd->shortcuts.emShortCuts, pDstCmd->shortcuts.btShortCuts, pDstCmd->shortcuts.btRow, pDstCmd->shortcuts.btCol)) {
							m_ShortCut.AddShortCuts(&Short);
							pDstCmd->ErrorCode = SETSHORTCUTS_NOSET;
						}
					}
					else {
						pDstCmd->ErrorCode = SETSHORTCUTS_NOSET;
					}
				}
			}
			else {
				pDstCmd->ErrorCode = SETSHORTCUTS_NOSET;
			}
		}
		SendMsgToMe(pDstCmd, sizeof(stSetShortCuts));
	}break;
	case stDelShortCuts::_value:
	{
		_CHECK_PACKAGE_LEN(stDelShortCuts, ncmdlen);
		stDelShortCuts* pDstCmd = (stDelShortCuts*)pcmd;
		pDstCmd->ErrorCode = SETSHORTCUTS_SUCCESS;
		stShortCuts* pShorCuts = m_ShortCut.FindShortCuts(pDstCmd->shortcuts.btRow, pDstCmd->shortcuts.btCol);
		if (pShorCuts) {
			stShortCuts Short = *pShorCuts;
			if (!m_ShortCut.DeleteShortCuts(pDstCmd->shortcuts.btRow, pDstCmd->shortcuts.btCol)) {
				stSetShortCuts retcmd;
				retcmd.ErrorCode = SETSHORTCUTS_NODEL;
				retcmd.shortcuts = Short;
				SendMsgToMe(&retcmd, sizeof(stSetShortCuts));
				return true;
			}
		}
		else {
			pDstCmd->ErrorCode = SETSHORTCUTS_NODEL;
		}
		SendMsgToMe(pDstCmd, sizeof(stDelShortCuts));
	}break;
	}
	return true;
}

bool  CPlayerObj::doCretMailCmd(stBaseCmd* pcmd, int ncmdlen)
{
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case stMailRetGetItem_inner::_value:
	case stMailSendNewMailInner::_value:
	{
		return false;  //需要屏蔽的来自客户端的消息
	}break;
	case stMailGetItem::_value:
	{
		CUserEngine::getMe().SendMailMsg2Super(pcmd, ncmdlen, m_i64UserOnlyID);
	}break;
	default:
	{
		CUserEngine::getMe().SendMailMsg2Super(pcmd, ncmdlen, m_i64UserOnlyID);
	}break;
	}
	return true;
}

bool CPlayerObj::doMailFromSuperSvr(stBaseCmd* pcmd, int ncmdlen)
{
	FUNCTION_BEGIN;
	if (isSwitchSvr() == true || m_btKickState == KICK_RELOGIN) {
		return false;
	}
	switch (pcmd->value)
	{
	case stMailGetItemRet::_value:
	{
		stMailGetItemRet* getitem = (stMailGetItemRet*)pcmd;
		stMailRetGetItem_inner retgetitem;
		bool boCanGet = true;
		if (getitem->i64itemid == -1 || getitem->i64itemid == 0) {
			if (getitem->dwGold)
			{
				if (m_dwGold + getitem->dwGold > _MAX_CARRYGOLD_(m_dwLevel))
				{
					boCanGet = false;
					getitem->btErrorCode = MAIL_FAIL_FULLGOLD;
				}
			}
		}

		if ((getitem->i64itemid == -1 || getitem->i64itemid == 0) && m_Packet.GetFreeBagCellCount(1) < (DWORD)getitem->getItemArr.size) {
			boCanGet = false;
			getitem->btErrorCode = MAIL_FAIL_FULLBAG;
		}

		if (getitem->i64itemid > 0 && m_Packet.GetFreeBagCellCount(1) < 1) {
			boCanGet = false;
			getitem->btErrorCode = MAIL_FAIL_FULLBAG;
		}

		if (getitem->btGoldType == 1 && m_Packet.GetFreeBagCellCount(1) < 1) {
			boCanGet = false;
			getitem->btErrorCode = MAIL_FAIL_FULLBAG;
		}

		if (!(m_pGateUser && m_pGateUser->m_OwnerLoginSvr && m_pGateUser->m_OwnerLoginSvr->IsConnected()))
		{
			boCanGet = false;
			getitem->btErrorCode = MAIL_FAIL_SERVER_ERROR;
		}
		CItem* pItem = NULL;
		std::vector<CItem*> vTmpItem;
		stReveivedItem ReveItem;
		ReveItem.ReSet();
		for (int idx = 0; idx < getitem->getItemArr.size; idx++)
		{
			if ((getitem->i64itemid == -1 || getitem->i64itemid == 0 || getitem->i64itemid == getitem->getItemArr[idx].i64ItemID) && getitem->getItemArr[idx].Location.btLocation == MAIL_LOCATION_ID && (getitem->wReveivedItem & (1 << idx))) {
				pItem = CItem::LoadItem(&getitem->getItemArr[idx], __FUNC_LINE__);
				if (pItem) {
					vTmpItem.push_back(pItem);
					ReveItem.SetReve(idx);
					if (!pItem->IsCurrency()) { // 邮件物品是货币不需要检查是否能放进背包
						if (!m_Packet.CheckCanAddToBag(pItem)) {
							boCanGet = false;
							getitem->btErrorCode = MAIL_FAIL_FULLBAG;
							break;
						}
					}
				}
				else {
					boCanGet = false;
					getitem->btErrorCode = MAIL_FAIL_ITEMERROR;
					break;
				}
			}
		}
		if (!boCanGet && vTmpItem.size() == 1 && vTmpItem[0] && vTmpItem[0]->IsCurrency()) boCanGet = true; //邮件只有金币时可以领取
		if (boCanGet)
		{
			bool boGetGold = false;
			if (vTmpItem.size())
			{
				pItem = NULL;
				for (int idx = 0; idx < (int)vTmpItem.size(); idx++)
				{
					pItem = vTmpItem[idx];
					if (pItem) {
						stAutoSetScriptParam autoparam(this);
						if (CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("mailgetspecialitem", false, pItem))
						{
							CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
						}
						else {
							m_Packet.AddItemToBag(pItem, true, false, true, "mailget");
						}
					}

				}
			}
			getitem->btErrorCode = MAIL_SUCCESS;
			retgetitem.btErrorCode = MAIL_SUCCESS;
			retgetitem.dwMailID = getitem->dwMailID;
			retgetitem.dwGold = (boGetGold) ? 0 : getitem->dwGold;
			retgetitem.wReveivedItem = max(getitem->wReveivedItem - ReveItem.wReveItemId, 0);
			retgetitem.dwItemCount = max(getitem->getItemArr.size - vTmpItem.size(), 0);
			getitem->wReveivedItem = retgetitem.wReveivedItem;
			vTmpItem.clear();
			CUserEngine::getMe().SendMailMsg2Super(&retgetitem, sizeof(retgetitem), m_i64UserOnlyID);
		}
		else
		{
			if (vTmpItem.size())
			{
				pItem = NULL;
				for (int idx = 0; idx < (int)vTmpItem.size(); idx++)
				{
					pItem = vTmpItem[idx];
					if (pItem) {
						CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
					}
				}
			}
			retgetitem.btErrorCode = getitem->btErrorCode;
			retgetitem.dwMailID = getitem->dwMailID;
			retgetitem.dwGold = getitem->dwGold;
			retgetitem.wReveivedItem = getitem->wReveivedItem;
			retgetitem.dwItemCount = getitem->getItemArr.size;
			vTmpItem.clear();
			CUserEngine::getMe().SendMailMsg2Super(&retgetitem, sizeof(retgetitem), m_i64UserOnlyID);//通知superserver该邮件附件已经被收取
		}
	}break;
	}
	SendMsgToMe(pcmd, ncmdlen);
	return true;
}

bool  CPlayerObj::doConsignCmd(stBaseCmd* pcmd, int ncmdlen)
{
	return true;
}

bool CPlayerObj::doStallCmdFromSupSvr(stBaseCmd* pcmd, int ncmdlen)
{
	FUNCTION_BEGIN;
	if (isSwitchSvr() == true || m_btKickState == KICK_RELOGIN) {
		return false;
	}
	switch (pcmd->value)
	{
	case stStallBeginSellItem::_value://上架物品
	{
		_CHECK_PACKAGE_LEN(stStallBeginSellItem, ncmdlen);
		stStallBeginSellItem* stallItem = (stStallBeginSellItem*)pcmd;
		CItem* pItem = m_Packet.FindItemInBag(stallItem->i64Id);
		if (pItem)
		{
			const char* szItemName = pItem->GetItemName();
			const char* szItemLuaId = pItem->GetItemLuaId();
			stallItem->dwCount = pItem->GetItemCount();
			g_logger.debug("Stall->%s 随身摊位上架物品%s,唯一ID,%s,数量:%d", getName(), szItemName, szItemLuaId, stallItem->dwCount);

			stStallBeginSellRet stStallItemSend;
			stStallItemSend.stCurItem = pItem->m_Item;
			stStallItemSend.i64PlayerOnlyId = m_i64UserOnlyID;
			stStallItemSend.dwItemPrice = stallItem->dwPrice;
			stStallItemSend.btItemPriceType = stallItem->btPriceType;
			stStallItemSend.dwCount = stallItem->dwCount;
			strcpy_s(stStallItemSend.szItemName, sizeof(stStallItemSend.szItemName) - 1, szItemName);
			if (m_Packet.DeleteItemInBag(pItem, stallItem->dwCount, true, false, "StallSellItem")) {
				stStallRet retMsg(1);//1上架成功
				SendMsgToMe(&retMsg, sizeof(stStallRet));

				GameService::getMe().Send2LogSvr(_SERVERLOG_SUPERSTALL, 0, 0, this,
					"'stallsell','%s','%s','%I64d','%d','出售','出售%sx%d,收费%d'",
					this->getAccount(),
					this->getName(),
					this->m_i64UserOnlyID,
					this->m_dwLevel,
					szItemName,
					stallItem->dwCount,
					stallItem->dwPrice);

				SENDMSG2SUPERBYONLYID(stSendStallMsgSuperSrv, m_i64UserOnlyID, &stStallItemSend, sizeof(stStallItemSend));
				return true;
			}
			else {
				g_logger.debug("Stall->%s 随身摊位上架物品删除失败%s,唯一ID,%s,数量:%d", getName(), szItemName, szItemLuaId, stallItem->dwCount);
			}
		}
		stStallRet retMsg(2);//2上架失败
		SendMsgToMe(&retMsg, sizeof(stStallRet));
	}
	break;
	case stStallGetBackItem::_value://下架物品
	{
		_CHECK_PACKAGE_LEN(stStallGetBackItem, ncmdlen);
		stStallGetBackItem* stallItem = (stStallGetBackItem*)pcmd;
		CItem* pItem = CItem::LoadItem(&stallItem->stCurItem, __FUNC_LINE__);
		if (pItem)
		{
			const char* szItemName = pItem->GetItemName();
			const char* szItemLuaId = pItem->GetItemLuaId();
			int nItemCount = pItem->GetItemCount();
			CPlayerObj::sendTipMsgByXml(this, GameService::getMe().GetStrRes(5, "stall"), szItemName);//下架成功获得物品
			g_logger.debug("Stall->%s 随身摊位下架物品%s,唯一ID,%s,数量:%d", getName(), szItemName, szItemLuaId, nItemCount);
			if (this->m_Packet.AddItemToBag(pItem, true, false, true, "StallGetBackItem")) {
				stStallRet retMsg(7);//下架成功
				SendMsgToMe(&retMsg, sizeof(stStallRet));

				GameService::getMe().Send2LogSvr(_SERVERLOG_SUPERSTALL, 0, 0, this,
					"'stallsellcancel','%s','%s','%I64d','%d','取消出售','取消出售%sx%d'",
					this->getAccount(),
					this->getName(),
					this->m_i64UserOnlyID,
					this->m_dwLevel,
					szItemName,
					nItemCount);

				return true;
			}
			else {
				CUserEngine::getMe().SendSystemMail(m_i64UserOnlyID, getName(), "随身摊位", "下架物品", pItem);
				CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
			}
		}
		else {
			stStallRet retMsg(6);//下架失败
			SendMsgToMe(&retMsg, sizeof(stStallRet));
		}
	}
	break;
	case stStallSuccessBuyItem::_value:
	{
		bool bSuccessBuy = false;
		_CHECK_PACKAGE_LEN(stStallSuccessBuyItem, ncmdlen);
		stStallSuccessBuyItem* pStallSuccessBuyItem = (stStallSuccessBuyItem*)pcmd;

		if (m_Packet.GetFreeBagCellCount(1) >= 1)
		{
			int iSellPrice = pStallSuccessBuyItem->dwItemPrice;//售价
			int iSeverPrice = iSellPrice * (STALL_SEVER_COF / 10000.0f);//手续费
			iSeverPrice = max(iSeverPrice, STALL_MIN_SEVERS);
			iSeverPrice = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("stall_fee", iSeverPrice, iSellPrice);

			if (iSellPrice > 0)
			{
				if (m_res[ResID::charge] >= (DWORD)(iSellPrice + iSeverPrice))
				{
					CItem* pItem = CItem::LoadItem(&pStallSuccessBuyItem->stCurItem, __FUNC_LINE__);
					if (pItem)
					{
						ResChange(ResID::charge, -iSellPrice, "SuccessBuyItem");//扣售价
						ResChange(ResID::charge, -iSeverPrice, "Stall_ShuiShou");//买方 扣手续费

						char szName[_MAX_ITEM_NAME] = { 0 };
						CopyMemory(szName, pItem->GetItemName(), sizeof(szName) - 1);
						g_logger.debug("Stall->%s 购买随身摊位物品%s,唯一ID,%s,花费%d,数量:%d", getName(), pItem->GetItemName(), pItem->GetItemLuaId(), (iSellPrice + iSeverPrice), pItem->GetItemCount());
						GameService::getMe().Send2LogSvr(_SERVERLOG_SUPERSTALL, 0, 0, this,
							"'stallbuy','%s','%s','%I64d','%d','购买','从%s的摊位获得%sx%d,花费%d'",
							this->getAccount(),
							this->getName(),
							this->m_i64UserOnlyID,
							this->m_dwLevel,
							pStallSuccessBuyItem->szSellerName,
							pItem->GetItemName(),
							pItem->GetItemCount(),
							iSellPrice + iSeverPrice);

						m_Packet.AddItemToBag(pItem, true, false, true, "SuccessBuyItem");

						stStallRet retMsg(3);//1发货成功
						SendMsgToMe(&retMsg, sizeof(stStallRet));
						CPlayerObj::sendTipMsgByXml(this, GameService::getMe().GetStrRes(3, "stall"), iSellPrice + iSeverPrice, iSeverPrice, szName);//消耗了多少元宝,多少手续费 提示得到物品
						bSuccessBuy = true;

						{
							stStallAddYuanBao stAddYuanBaoCmd;//给卖家加元宝
							stAddYuanBaoCmd.i64SellerOnlyId = pStallSuccessBuyItem->i64SellerOnlyId;//卖方的唯一ID
							stAddYuanBaoCmd.dwYuanBao = iSellPrice;
							CopyMemory(stAddYuanBaoCmd.szItemName, szName, sizeof(szName) - 1);

							SENDMSG2SUPERBYONLYID(stSendStallMsgSuperSrv, pStallSuccessBuyItem->i64SellerOnlyId, &stAddYuanBaoCmd, sizeof(stAddYuanBaoCmd));
						}

						stStallGSAddLog stAddLog;//日志
						stAddLog.i64BuyerId = m_i64UserOnlyID;
						stAddLog.dwPrice = iSellPrice;
						stAddLog.dwPriceType = 1;
						stAddLog.i64SellerId = pStallSuccessBuyItem->i64SellerOnlyId;
						stAddLog.stCurItem = pStallSuccessBuyItem->stCurItem;
						CopyMemory(stAddLog.szItemName, szName, sizeof(stAddLog.szItemName));
						SENDMSG2SUPERBYONLYID(stSendStallMsgSuperSrv, this->m_i64UserOnlyID, &stAddLog, sizeof(stStallGSAddLog));
					}
					else
					{
						stStallRet retMsg(4);//购买失败,元宝不足
						SendMsgToMe(&retMsg, sizeof(stStallRet));
					}
				}
			}
		}
		else
		{
			CPlayerObj::sendTipMsgByXml(this, GameService::getMe().GetStrRes(6, "stall"));//背包空格不足
		}

		if (bSuccessBuy == false)
		{
			stStallBeginSellRet stStallItemSend;
			stStallItemSend.stCurItem = pStallSuccessBuyItem->stCurItem;
			stStallItemSend.i64PlayerOnlyId = pStallSuccessBuyItem->i64SellerOnlyId;
			stStallItemSend.dwItemPrice = pStallSuccessBuyItem->dwItemPrice;
			stStallItemSend.btItemPriceType = pStallSuccessBuyItem->btItemPriceType;
			stStallItemSend.dwCount = pStallSuccessBuyItem->stCurItem.dwCount;
			strcpy_s(&stStallItemSend.szItemName[0], sizeof(stStallItemSend.szItemName), GetLuaItemName(stStallItemSend.stCurItem.dwBaseID));

			SENDMSG2SUPERBYONLYID(stSendStallMsgSuperSrv, pStallSuccessBuyItem->i64SellerOnlyId, &stStallItemSend, sizeof(stStallItemSend));
		}
	}
	break;
	case stStallWantBuyItem::_value:
	{
		_CHECK_PACKAGE_LEN(stStallWantBuyItem, ncmdlen);
		stStallWantBuyItem* pWantItem = (stStallWantBuyItem*)pcmd;
		if (m_Packet.GetFreeBagCellCount(1) < 1)
		{
			stStallRet retMsg(5);//包裹没有空格
			SendMsgToMe(&retMsg, sizeof(stStallRet));
			CPlayerObj::sendTipMsgByXml(this, GameService::getMe().GetStrRes(6, "stall"));//背包空格不足
			return false;
		}


		int iSeverPrice = ceil((float)pWantItem->dwItemPrice / 10000.0f * STALL_SEVER_COF);//手续费
		iSeverPrice = min(iSeverPrice, max(iSeverPrice, STALL_MIN_SEVERS));

		if (m_res[ResID::charge] >= pWantItem->dwItemPrice + iSeverPrice)
		{
			stStallWantBuyItemRet stWantBuyItemRet;
			stWantBuyItemRet.btItemPriceType = pWantItem->btItemPriceType;
			stWantBuyItemRet.i64ItemOnlyId = pWantItem->i64ItemOnlyId;
			stWantBuyItemRet.dwItemPrice = pWantItem->dwItemPrice;
			stWantBuyItemRet.i64SellerOnlyId = pWantItem->i64SellerOnlyId;

			SENDMSG2SUPERBYONLYID(stSendStallMsgSuperSrv, m_i64UserOnlyID, &stWantBuyItemRet, sizeof(stWantBuyItemRet));
		}
		else
		{
			stStallRet retMsg(4);//购买失败,元宝不足
			SendMsgToMe(&retMsg, sizeof(stStallRet));
			//钱不够
		}
	}
	break;

	case stStallAddYuanBao::_value:
	{
		_CHECK_PACKAGE_LEN(stStallAddYuanBao, ncmdlen);
		stStallAddYuanBao* pStallAddYuanBao = (stStallAddYuanBao*)pcmd;
		if (pStallAddYuanBao->i64SellerOnlyId == m_i64UserOnlyID)
		{
			int srcYuanBao = quest_vars_get_var_n("StallYuanBao");
			if (srcYuanBao >= 0)
			{
				quest_vars_set_var_n("StallYuanBao", srcYuanBao + pStallAddYuanBao->dwYuanBao, true);
				if (pStallAddYuanBao->szItemName[0] != 0)
				{
					g_logger.debug("Stall->%s 随身摊位卖出物品增加摊位元宝%d,%s", getName(), pStallAddYuanBao->dwYuanBao, pStallAddYuanBao->szItemName);
				}
				else
				{
					g_logger.debug("Stall->%s 随身摊位卖出物品增加摊位元宝%d", getName(), pStallAddYuanBao->dwYuanBao);
				}
			}
			stStallRet retMsg(8);//东西卖出
			SendMsgToMe(&retMsg, sizeof(stStallRet));
		}
	}
	break;
	case stCretChat::_value:
	{
		_CHECK_PACKAGE_LEN(stCretChat, ncmdlen);
		OnstCretChat(this, (stCretChat*)pcmd, ncmdlen, NULL, true);
	}break;
	}
	return false;
}

bool CPlayerObj::doConsignFromSuperSvr(stBaseCmd* pcmd, int ncmdlen)
{
	FUNCTION_BEGIN;
	if (isSwitchSvr() == true || m_btKickState == KICK_RELOGIN || CUserEngine::getMe().isCrossSvr()) {
		return false;
	}
	switch (pcmd->value)
	{
	case stConsignSellItemIn2Gs::_value:	//supersvr转发
	{
		_CHECK_PACKAGE_LEN(stConsignSellItemIn2Gs, ncmdlen);
		stConsignSellItemIn2Gs* sellitem = (stConsignSellItemIn2Gs*)pcmd;
		if (sellitem->btType == 0) {
			BYTE btOk = 0;
			CItem* pItem = m_Packet.FindItemInBag(sellitem->i64Id);
			if (!pItem) {
				btOk = 1;
			}
			else
			{
				auto itemBase = pItem->GetItemDataBase();
				if (itemBase)
				{
					if (!pItem->GetCanTrade() || pItem->GetOutLock() || itemBase->dwActionType == 0) {
						btOk = 2;
					}
					else if (m_res[ResID::charge] < sellitem->dwCost) {
						btOk = 3;
					}
					else if (sellitem->dwCount == 0) {
						btOk = 5;
					}
					else if (sellitem->dwPrice / sellitem->dwCount < (DWORD)itemBase->nSellPrice) {
						btOk = 4;
					}
					else
					{
						do
						{
							AILOCKT(CUserEngine::getMe().m_scriptsystem.m_LuaVM->m_luacalllock);
							lua_State* L = CUserEngine::getMe().m_scriptsystem.m_LuaVM->GetHandle();
							if (L)
							{
								if (CALL_LUARET("ConsignSellCheck", true, this, pItem) == false)
								{
									btOk = 23;
								}
							}
						} while (false);
					}
				}

			}
			stConsignSellItemInner selliteminner;
			selliteminner.btErrorCode = btOk;
			if (btOk != 0) {
				this->sendTipMsgByXml(this, GameService::getMe().GetStrRes(btOk, "auction"));
			}
			else {
				if (pItem->GetItemCount() >= sellitem->dwCount) {
					auto itemBase = pItem->GetItemDataBase();
					if (itemBase) {
						selliteminner.dwItemSearchTypeId = itemBase->dwActionType;
						selliteminner.dwItemSearchSubTypeId = itemBase->dwActionSubType;
					}
					if (m_Packet.ServerGetSplitItem(pItem->m_Item.i64ItemID, pItem->GetItemCount() - sellitem->dwCount)) {
						selliteminner.item.SetItem(pItem->m_Item);
						strcpy_s(selliteminner.item.szName, sizeof(selliteminner.item.szName), itemBase->name_gb.c_str());
						selliteminner.item.i64SellOnlyId = m_i64UserOnlyID;
						strcpy_s(selliteminner.item.szSeller, sizeof(selliteminner.item.szSeller), getName());
						selliteminner.item.wType = (WORD)pItem->GetType();        //物品类型
						selliteminner.item.dwWearLevel = pItem->GetWearLevel();
						selliteminner.item.btRare = pItem->GetRare();
						selliteminner.item.dwConsignPrice = sellitem->dwPrice;
						selliteminner.item.tLeftTime = time(NULL) + sellitem->btDays * 60 * 60 * 24;
						g_logger.debug("%s 寄售物品 %s", getName(), pItem->GetItemName());
						m_Packet.DeleteItemInBag(pItem, pItem->GetItemCount(), true, false, "auctionsellitem");
						GoldChanged(-(int)sellitem->dwCost, false, "auctionuse");
					}
					SENDMSG2SUPERBYONLYID(stSendConsignmentMsgSuperSrv, m_i64UserOnlyID, &selliteminner, sizeof(selliteminner));
					return true;
				}
				else {
					this->sendTipMsgByXml(this, GameService::getMe().GetStrRes(21, "auction"));
				}
			}
			return false;
		}
		else {
			if (ResChange(ResID::charge, -(int)sellitem->dwPrice, "WaitBuy"))
			{
				auto itemBase = sJsonConfig.GetItemDataById(sellitem->i64Id);
				if (itemBase) {
					stConsignSellItemInner selliteminner;
					selliteminner.btErrorCode = 0;
					selliteminner.i64ItemId = sellitem->i64Id;
					selliteminner.dwItemSearchTypeId = itemBase->dwActionType;
					selliteminner.dwItemSearchSubTypeId = itemBase->dwActionSubType;

					// 物品结构一定要填充，其他地方都会用到
					stItem zeroItem;
					zeroItem.i64ItemID = sellitem->i64Id;
					zeroItem.dwBaseID = (DWORD)sellitem->i64Id;
					zeroItem.dwCount = sellitem->dwCount;
					selliteminner.item.SetItem(zeroItem);

					selliteminner.item.btType = 1;
					strcpy_s(selliteminner.item.szName, sizeof(selliteminner.item.szName), itemBase->name_gb.c_str());
					strcpy_s(selliteminner.item.szSeller, sizeof(selliteminner.item.szSeller), getName());
					selliteminner.item.i64SellOnlyId = m_i64UserOnlyID;
					selliteminner.item.wType = (WORD)(emTypeDef)itemBase->dwType;;        //物品类型
					selliteminner.item.dwWearLevel = itemBase->dwNeedLevel;
					selliteminner.item.btRare = itemBase->nRare;
					selliteminner.item.dwConsignPrice = sellitem->dwPrice;
					selliteminner.item.tLeftTime = time(NULL) + sellitem->btDays * 60 * 60 * 24;
					g_logger.debug("%s 求购物品 %s", getName(), itemBase->szName);
					GoldChanged(-(int)sellitem->dwCost, false, "auctionuse");
					SENDMSG2SUPERBYONLYID(stSendConsignmentMsgSuperSrv, m_i64UserOnlyID, &selliteminner, sizeof(selliteminner));
					return true;
				}
				else {
					this->sendTipMsgByXml(this, GameService::getMe().GetStrRes(10, "auction"));
					return false;
				}
			}
			else {
				this->sendTipMsgByXml(this, GameService::getMe().GetStrRes(9, "auction"));
				return false;
			}
		}
	}break;
	case stConsignBuyItemInner::_value:
	{
		_CHECK_PACKAGE_LEN(stConsignBuyItemInner, ncmdlen);
		stConsignBuyItemInner* BuyInnerCmd = (stConsignBuyItemInner*)pcmd;
		if (BuyInnerCmd->btErrorCode == 0) {
			if (BuyInnerCmd->btType == 0) {
				// 成功的话只有一个请求
				if (BuyInnerCmd->btDealState == 0) {
					if (m_res[ResID::charge] >= BuyInnerCmd->dwGold + BuyInnerCmd->dwCommissionGold) {
						BuyInnerCmd->btErrorCode = 0;
						BuyInnerCmd->btDealState = 1;
						char szlog[256] = { 0 };
						sprintf_s(szlog, sizeof(szlog) - 1, "auction buy item-%d", BuyInnerCmd->dwIndex);
						ResChange(ResID::charge, -(int)BuyInnerCmd->dwGold, szlog);
						sprintf_s(szlog, sizeof(szlog) - 1, "auction buy item comm-%d", BuyInnerCmd->dwIndex);
						ResChange(ResID::charge, -(int)BuyInnerCmd->dwCommissionGold, szlog);
					}
					else {
						BuyInnerCmd->btErrorCode = 1;
						this->sendTipMsgByXml(this, GameService::getMe().GetStrRes(22, "auction"));
					}
					SENDMSG2SUPERBYONLYID(stSendConsignmentMsgSuperSrv, m_i64UserOnlyID, BuyInnerCmd, sizeof(stConsignBuyItemInner));
					return true;
				}
			}
			else if (BuyInnerCmd->btType == 1) {	// 求购
				// 先扣除物品
				if (BuyInnerCmd->btDealState == 0) {
					DWORD dwCount = m_Packet.GetItemCountByBindType(BuyInnerCmd->item.dwBaseID, 0);
					if (dwCount >= BuyInnerCmd->item.dwCount) {
						BuyInnerCmd->btDealState = 1;
						BuyInnerCmd->btErrorCode = 0;
						char szlog[256] = { 0 };
						sprintf_s(szlog, sizeof(szlog) - 1, "auction wait buy item-%d", BuyInnerCmd->dwIndex);
						m_Packet.DelAllItemInBagByBindType(BuyInnerCmd->item.dwBaseID, BuyInnerCmd->item.dwCount, 0, szlog);
					}
					else {
						BuyInnerCmd->btErrorCode = 1;
						this->sendTipMsgByXml(this, GameService::getMe().GetStrRes(24, "auction"));
					}
					SENDMSG2SUPERBYONLYID(stSendConsignmentMsgSuperSrv, m_i64UserOnlyID, BuyInnerCmd, sizeof(stConsignBuyItemInner));
					return true;
				}
				// 发送奖励
				else if (BuyInnerCmd->btDealState == 2) {
					DWORD dwLingfu = BuyInnerCmd->dwGold - max((BuyInnerCmd->dwGold * 1000) / 10000, 1);
					stItem* pitem = GetMailedItem(1, dwLingfu, 0);
					CUserEngine::getMe().SendSystemMail(m_i64UserOnlyID, getName(), "求购系统邮件", vformat("您消耗了%s，满足了其他玩家的需求，以下是你获得的能源晶石", GetLuaItemName(BuyInnerCmd->item.dwBaseID)), pitem);

					CUserEngine::getMe().SendSystemMailByIDAndCount(BuyInnerCmd->m_i64UserOnlyID, BuyInnerCmd->szSeller, "求购系统邮件", "你的求购成功，以下是您求购的道具", BuyInnerCmd->item.dwBaseID, BuyInnerCmd->item.dwCount, GetEnvir()->getMapId(), "求购获得", BuyInnerCmd->szSeller);
				}
			}
		}
		else if (BuyInnerCmd->btErrorCode == 1) {
			if (BuyInnerCmd->btType == 0) {
				if (BuyInnerCmd->btDealState == 1) {
					char szlog[256] = { 0 };
					sprintf_s(szlog, sizeof(szlog) - 1, "auction buy item-%d,fail", BuyInnerCmd->dwIndex);
					ResChange(ResID::charge, (int)BuyInnerCmd->dwGold, szlog);
					sprintf_s(szlog, sizeof(szlog) - 1, "auction buy item comm-%d,fail", BuyInnerCmd->dwIndex);
					ResChange(ResID::charge, (int)BuyInnerCmd->dwCommissionGold, szlog);
				}
			}
			else if (BuyInnerCmd->btType == 1) {
				if (BuyInnerCmd->btDealState == 1) {
					CUserEngine::getMe().SendSystemMailByIDAndCount(m_i64UserOnlyID, getName(), "求购系统邮件", "你的交易失败，以下是您求购消耗的道具", BuyInnerCmd->item.dwBaseID, BuyInnerCmd->item.dwCount, GetEnvir()->getMapId(), "求购获得", getName());
				}
			}
		}
	}break;
	case stConsignTakeMyItemInner::_value:
	{
		_CHECK_PACKAGE_LEN(stConsignTakeMyItemInner, ncmdlen);
		stConsignTakeMyItemInner* TakeInnerCmd = (stConsignTakeMyItemInner*)pcmd;
		TakeInnerCmd->boOk = false;
		if (TakeInnerCmd->btType == em_Consignment_SearchBuyNoTake ||
			TakeInnerCmd->btType == em_Consignment_SearchMySell) {	//提取物品
			if (m_Packet.GetFreeBagCellCount(1) < 1) {
				if (TakeInnerCmd->btType == em_Consignment_SearchMySell) {
					CUserEngine::getMe().SendSystemMail(0xFFFFFFFFFFFFFFFF, getName(), "寄售系统邮件", "物品下架成功,由于您的包裹空间不足,系统通过邮件返还!", &TakeInnerCmd->item);
				}
				else {
					CUserEngine::getMe().SendSystemMail(0xFFFFFFFFFFFFFFFF, getName(), "寄售系统邮件", "物品购买成功,由于您的包裹空间不足,系统通过邮件返还!", &TakeInnerCmd->item);
				}
				TakeInnerCmd->boOk = true;
				CPlayerObj::sendTipMsgByXml(this, GameService::getMe().GetStrRes(20, "auction"));
			}
			else {
				CItem* pItem = CItem::LoadItem(&TakeInnerCmd->item, __FUNC_LINE__);
				if (pItem) {
					char szlog[256] = { 0 };
					sprintf_s(szlog, sizeof(szlog) - 1, "actiondownitem-%d", TakeInnerCmd->dwIdx);
					if (TakeInnerCmd->btType == em_Consignment_SearchBuyNoTake) {
						sprintf_s(szlog, sizeof(szlog) - 1, "auctionbuyitem-%d", TakeInnerCmd->dwIdx);
					}
					this->m_Packet.AddItemToBag(pItem, true, false, true, szlog, NULL, TakeInnerCmd->dwGold);
					TakeInnerCmd->boOk = true;
				}
			}
		}
		else if (TakeInnerCmd->btType == em_Consignment_SearchMyWaitBuy) {
			stItem* pitem = GetMailedItem(100, TakeInnerCmd->dwGold, 0);
			CUserEngine::getMe().SendSystemMail(m_i64UserOnlyID, getName(), "求购系统邮件", "你主动取消求购，以下是您发布求购的灵符", pitem);
			TakeInnerCmd->boOk = true;
		}

		SENDMSG2SUPERBYONLYID(stSendConsignmentMsgSuperSrv, m_i64UserOnlyID, TakeInnerCmd, sizeof(*TakeInnerCmd));
	}break;
	}
	return true;
}

bool CPlayerObj::doRelationCmd(stBaseCmd* pcmd, int ncmdlen)
{
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
		//case stRelationAdd::_value:
		//{
		//_CHECK_PACKAGE_LEN(stRelationAdd,ncmdlen);
		//if (((stRelationAdd*)pcmd)->btType!=LIST_FRIEND &&((stRelationAdd*)pcmd)->btType!=LIST_BLOCK)
		//{
		//return false;  //客户端发来的非添加好友和非黑名单的消息屏蔽掉
		//}
		//}break;
	case stRelationLocation::_value:
	{
		_CHECK_PACKAGE_LEN(stRelationLocation, ncmdlen);
		stRelationLocation* pDestCmd = (stRelationLocation*)pcmd;
		if (pDestCmd->btType == LIST_ENEMY)
		{
			stAutoSetScriptParam autoguard(this);
			if (CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("CanTrackingOrders", false) == false) {
				return true;
			}
		}
	}break;
	}
	SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv, m_i64UserOnlyID, pcmd, ncmdlen);
	return true;
}

bool CPlayerObj::doRelationFromSuperSvr(stBaseCmd* pcmd, int ncmdlen)
{
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case stRelationGetListRet::_value:
	{
		stRelationGetListRet* RelationGetRet = (stRelationGetListRet*)pcmd;
		switch (RelationGetRet->btType)
		{
		case LIST_FRIEND: //好友列表
		{
			m_xFriendList.deleteall();
			for (int i = 0; i < RelationGetRet->friendlist.size; i++)
			{
				stRelation* pRelation = CLD_DEBUG_NEW stRelation;
				CopyMemory(pRelation, &RelationGetRet->friendlist[i], sizeof(*pRelation));
				if (pRelation->szName[0])
				{
					m_xFriendList.addValue(pRelation);
				}
				else {
					SAFE_DELETE(pRelation)
				}
			}
		}break;
		case LIST_BLOCK:
		{//黑名单
			m_xBlockList.deleteall();
			for (int i = 0; i < RelationGetRet->friendlist.size; i++)
			{
				stRelation* pRelation = CLD_DEBUG_NEW stRelation;
				CopyMemory(pRelation, &RelationGetRet->friendlist[i], sizeof(*pRelation));
				if (pRelation->szName[0])
				{
					m_xBlockList.addValue(pRelation);
				}
				else {
					SAFE_DELETE(pRelation)
				}

			}
		}break;
		case LIST_ENEMY:
		{//仇人列表
			m_xEnemyList.deleteall();
			for (int i = 0; i < RelationGetRet->friendlist.size; i++)
			{
				stRelation* pRelation = CLD_DEBUG_NEW stRelation;
				CopyMemory(pRelation, &RelationGetRet->friendlist[i], sizeof(*pRelation));
				if (pRelation->szName[0])
				{
					m_xEnemyList.addValue(pRelation);
				}
				else {
					SAFE_DELETE(pRelation)
				}

			}
		}break;
		}
		SendMsgToMe(pcmd, sizeof(stRelationGetListRet) + RelationGetRet->friendlist.getarraysize());
		return true;
	}break;
	case stRelationListAddorChange::_value:
	{
		stRelationListAddorChange* RelationListAddorChange = (stRelationListAddorChange*)pcmd;
		switch (RelationListAddorChange->btType)
		{
		case LIST_FRIEND: //好友列表
		{
			stRelation* pRelation = m_xFriendList.FindByOnlyId(RelationListAddorChange->friendinfo.i64OnlyId);
			if (pRelation) {
				CopyMemory(pRelation, &RelationListAddorChange->friendinfo, sizeof(stRelation));
			}
			else {
				pRelation = CLD_DEBUG_NEW stRelation;
				if (pRelation) {
					CopyMemory(pRelation, &RelationListAddorChange->friendinfo, sizeof(stRelation));
					m_xFriendList.addValue(pRelation);
					if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
						stAutoSetScriptParam autoparam(this);
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("FirstRelation");
					}
				}
			}
		}break;
		case LIST_BLOCK:
		{//黑名单
			stRelation* pRelation = m_xBlockList.FindByOnlyId(RelationListAddorChange->friendinfo.i64OnlyId);
			if (pRelation) {
				CopyMemory(pRelation, &RelationListAddorChange->friendinfo, sizeof(stRelation));
			}
			else {
				pRelation = CLD_DEBUG_NEW stRelation;
				if (pRelation) {
					CopyMemory(pRelation, &RelationListAddorChange->friendinfo, sizeof(stRelation));
					m_xBlockList.addValue(pRelation);
				}
			}
		}break;
		case LIST_ENEMY:
		{//仇人列表
			stRelation* pRelation = m_xEnemyList.FindByOnlyId(RelationListAddorChange->friendinfo.i64OnlyId);
			if (pRelation) {
				CopyMemory(pRelation, &RelationListAddorChange->friendinfo, sizeof(stRelation));
			}
			else {
				pRelation = CLD_DEBUG_NEW stRelation;
				if (pRelation) {
					CopyMemory(pRelation, &RelationListAddorChange->friendinfo, sizeof(stRelation));
					m_xEnemyList.addValue(pRelation);
				}
			}
		}break;
		}
	}break;
	case stRelationListDelete::_value:
	{
		stRelationListDelete* RelationListDelete = (stRelationListDelete*)pcmd;
		switch (RelationListDelete->btType)
		{
		case LIST_FRIEND: //好友列表
		{
			m_xFriendList.DeleteByName(RelationListDelete->szName);
		}break;
		case LIST_BLOCK:
		{//黑名单
			m_xBlockList.DeleteByName(RelationListDelete->szName);
		}break;
		case LIST_ENEMY:
		{//仇人列表
			m_xEnemyList.DeleteByName(RelationListDelete->szName);
		}break;
		}
	}break;
	case stRelationInnerAdd::_value:
	{
		stRelationInnerAdd* RelationInnerAdd = (stRelationInnerAdd*)pcmd;
		switch (RelationInnerAdd->btType)
		{
		case LIST_FRIEND: //好友列表
		{
			CChat::sendClient(this, GameService::getMe().GetStrRes(RES_LANG_RELATION_FRIEND), RelationInnerAdd->szAddToName, RelationInnerAdd->szAddToName);
			this->sendTipMsg(this, vformat(GameService::getMe().GetStrRes(4, "relation"), RelationInnerAdd->szAddToName));//`6`您成功击杀了您的仇人%s，完美的完成了复仇。
		}break;
		}
		return true;
	}break;
	case stRelationAddRet::_value:
	{
		stRelationAddRet* pdstcmd = (stRelationAddRet*)pcmd;
		if (pdstcmd->btErrorCode == RELATION_SUCCESS) {
			if (pdstcmd->btType == LIST_FRIEND) {
				TriggerEvent(this, ADDFRIEND, 1);
				this->sendTipMsg(this, vformat(GameService::getMe().GetStrRes(4, "relation"), pdstcmd->szName));
			}
		}
	}break;
	case stRelationInnerLocation::_value:
	{
		stRelationInnerLocation* pDstCmd = (stRelationInnerLocation*)pcmd;
		stRelationInnerLocationRet retcmd;
		retcmd.i64QueryOnlyId = pDstCmd->i64QueryOnlyId;
		retcmd.btErrorCode = 0;
		retcmd.btType = pDstCmd->btType;
		retcmd.i64OnlyId = m_i64UserOnlyID;
		strcpy_s(retcmd.szName, sizeof(retcmd.szName) - 1, pDstCmd->szName);
		retcmd.nMapId = GetEnvir()->getMapId();;
		retcmd.nX = m_nCurrX;
		retcmd.nY = m_nCurrY;
		strcpy_s(retcmd.szMapName, sizeof(retcmd.szMapName), GetEnvir()->getFullMapName());
		SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv, m_i64UserOnlyID, &retcmd, sizeof(stRelationInnerLocationRet));
		return true;
	}break;
	case stRelationSetAddFriendTypeRet::_value:
	{
		//emFriendType_AllowAll = 0x1,		//允许所有人加我为好友
		//emFriendType_NeedVerify = 0x2,	//需要验证
		//emFriendType_Refuse = 0x4,		//拒绝所有
		//emFriendType_HideLocation = 0x8,	//对好友隐藏位置
		//emFriendType_HideRelationTip = 0x10,//不显示好友仇人提示
		//emFriendType_GuildMemberDie = 0x20,//显示行会成员死亡提示
		//emFriendType_GuildDonate=0x40,//显示行会捐献
		//emFriendType_GuildMemberJoin =0x80,//显示行会新成员加入
		//emFriendType_GuildMemberLeave = 0x100,//显示行会成员离开
		stRelationSetAddFriendTypeRet* pdestcmd = (stRelationSetAddFriendTypeRet*)pcmd;

		quest_vars_set_var_n("Config_GuildMemberDieTipHide", (pdestcmd->nType & 0x20) > 0 ? 1 : 0, false);
		quest_vars_set_var_n("Config_GuildDonateHide", (pdestcmd->nType & 0x40) > 0 ? 1 : 0, false);

	}break;
	}
	SendMsgToMe(pcmd, ncmdlen);
	return true;
}

bool CPlayerObj::OnCretStruck(stCretStruckFull* pcmd, unsigned int ncmdlen) {
	FUNCTION_BEGIN;
	if (ncmdlen >= sizeof(stCretStruckFull)) {
		auto curMap = GetEnvir();
		if (!curMap)
			return false;
		auto attacker = curMap->GetCreature(pcmd->dwSrcTmpId);
		if (!attacker)
			return false;
		if (attacker && !attacker->isDie()) {
			if (!attacker->isEnemy(this)) {
				return true;
			}
		}

		if (auto pMagic = attacker->FindSkill(pcmd->dwMagicID))
		{
			pMagic->OnCretStruck(attacker);
		}

		if (quest_vars_get_var_n("autostate") == 1 && pcmd && attacker && attacker->isMonster()) {
			// 寻路且怪物攻击不当成受击
		}
		else
			PlayerUnderBattle();
		if (attacker->isPlayer()) {
			CPlayerObj* pAtk = attacker->toPlayer();
			bool isAtkPking = pAtk->IsPking();
			if (!isAtkPking)
				pAtk->sendTipMsg(pAtk, vformat("`6`已进入pk模式"));
			pAtk->PlayerUnderPk();
			if (!IsPking())
				sendTipMsg(this, vformat("`6`已进入pk模式"));
			PlayerUnderPk();
		}
		StruckDamage(pcmd->npower + pcmd->nAddDamage, attacker, pcmd->btDamageSrc);
		if (attacker && attacker->isMonster())
			pcmd->btDamageSrc = 1;

		pcmd->nHp = m_nNowHP;
		pcmd->nMaxhp = m_stAbility[AttrID::MaxHP];
		SendRefMsg(pcmd, sizeof(stCretStruck));
		if (m_nNowHP <= 0 && !m_boHpToZero) {
			m_boHpToZero = true;
			if (attacker) {
				SetLastHitter(attacker);
			}
		}
	}
	return true;
}


bool CPlayerObj::OnCretAction(stCretActionRet* pcmd, unsigned int ncmdlen, stProcessMessage* param) {
	FUNCTION_BEGIN;
	if (ncmdlen >= sizeof(stCretActionRet)) {
		SendRefMsg(pcmd, ncmdlen);
	}
	return true;
}

void CPlayerObj::ChangePetTarget(CCreature* pTarget) {
	FUNCTION_BEGIN;
	if (pTarget && isEnemy(pTarget)) {
		//g_logger.error("给%s宠物英雄设定攻击目标 ",getName());
		for (auto it = m_Petm.m_pets.begin(); it != m_Petm.m_pets.end(); it++) {
			CPetObj* pPet = it->second;
			if (pPet) {
				if (pTarget->isPlayer()) {//目标是玩家
					pPet->ChangeTarget(pTarget);
				}
				else {
					pPet->ChangeTarget(pTarget);
				}
			}
		}
	}
}

bool CPlayerObj::checkFirstLvAtt(CItem* pItem) {
	FUNCTION_BEGIN;
	if (pItem) {
		auto itemBase = pItem->GetItemDataBase();
		if (!itemBase) return false;
		if (m_stAbility[AttrID::Strength] < itemBase->dwStrength)
			return false;
		if (m_stAbility[AttrID::Physique] < itemBase->dwPhysique)
			return false;
		if (m_stAbility[AttrID::Agility] < itemBase->dwAgility)
			return false;
		if (m_stAbility[AttrID::Wisdom] < itemBase->dwWisdom)
			return false;
		if (m_stAbility[AttrID::Intelligence] < itemBase->dwBrains)
			return false;
		return true;
	}
	return false;
}

void CPlayerObj::GetBaseProperty() {
	FUNCTION_BEGIN;
	m_i64MaxExp = 100000000000;
	auto pstPlayerBase = sJsonConfig.GetLvlAbilityData(m_dwLevel, GetJobType());
	if (pstPlayerBase) {
		m_i64MaxExp = pstPlayerBase->i64NeedExp;				//战斗经验	

		m_stBaseAbility[AttrID::MaxHP] = pstPlayerBase->dwHp;			//生命	
		m_stBaseAbility[AttrID::MaxMP] = pstPlayerBase->dwMp;			//精神
		m_stBaseAbility[AttrID::MaxPP] = pstPlayerBase->dwPP;             //体力
		m_stBaseAbility[AttrID::MinPAtk] = pstPlayerBase->dwMinPAtk;	//物攻下限
		m_stBaseAbility[AttrID::MaxPAtk] = pstPlayerBase->dwMaxPAtk;	//物攻上限
		m_stBaseAbility[AttrID::MaxMAtk] = pstPlayerBase->dwMaxMAtk;	//最大魔攻
		m_stBaseAbility[AttrID::PDef] = pstPlayerBase->dwPDef;			//物防
		m_stBaseAbility[AttrID::MDef] = pstPlayerBase->dwMDef;			//魔防
		m_stBaseAbility[AttrID::Hit] = pstPlayerBase->dwHit;			//命中率
		m_stBaseAbility[AttrID::Juck] = pstPlayerBase->dwJuck;			//闪避率
		m_stBaseAbility[AttrID::HpRestore] = pstPlayerBase->nHpRestore;		//生命回复
		m_stBaseAbility[AttrID::MpRestore] = pstPlayerBase->nMpRestore;		//精神回复
		m_stBaseAbility[AttrID::PpRestore] = pstPlayerBase->nPpRestore;		//体力回复
		m_nAttackInterval = pstPlayerBase->nAttackInterval;	//攻击间隔
		m_nReleaseInterval = pstPlayerBase->nReleaseInterval;	//释放间隔
		m_nMoveInterval = pstPlayerBase->nMoveInterval;		//移动间隔
		m_stBaseAbility[AttrID::PveMul] = pstPlayerBase->nPveCof;					//PVE
		m_stBaseAbility[AttrID::PvpMul] = pstPlayerBase->nPvpCof;					//PVP
		m_nPpMoveCost = pstPlayerBase->nPpMoveCost;			//移动消耗体力
		m_nKillMaxMonLv = pstPlayerBase->nKillMaxMonLv;		//怪物最大等级
		m_nKillMinMonLv = pstPlayerBase->nKillMinMonLv;		//怪物最小等级
	}
	else {
		g_logger.error("人物基本属性表 %d级基本数据不存在!", m_dwLevel);
	}

	//人物最大血量至少为1（防止属性表对应等级数据未配置，导致人物最大血量为0人物不停死亡的问题）
	if (m_stBaseAbility[AttrID::MaxHP] < 1)
	{
		g_logger.error("人物最大血量为0，强制修正为1!");
		m_stBaseAbility[AttrID::MaxHP] = 1;
	}
}

void CPlayerObj::ChangeProperty(bool bosend, const char* ff)
{
	SetAbilityFlag();
}

void CPlayerObj::SetAbilityFlag(emAbilityFlag type)
{
	if (type == ABILITY_FLAG_EQUIP) {
		m_abilityFlag |= ABILITY_FLAG_EQUIPBUFF;
		m_abilityFlag |= ABILITY_FLAG_GEM;
	}
	m_abilityFlag |= type;

	switch (type)
	{
	case ABILITY_FLAG_ALL:
	{
		SetSpecialAbilityFlag();
	}break;
	case ABILITY_FLAG_LUA:
	{
		SetSpecialAbilityFlag(SPECIALABILITY_LUA);
	}break;
	case ABILITY_FLAG_EQUIP:
	{
		SetSpecialAbilityFlag(SPECIALABILITY_EQUIP);
	}break;
	}
}

void CPlayerObj::SetSpecialAbilityFlag(emSpecialAbilityFlag type)
{
	if (type == ABILITY_FLAG_EQUIP) {
		m_specialAbiFlag |= SPECIALABILITY_EQUIP;
	}
	m_specialAbiFlag |= type;
}

void CPlayerObj::DoChangeProperty(stARpgAbility& abi, bool boNotif, const char* ff) {
	FUNCTION_BEGIN;
	//GuardCpuCost cpuCost1("1");
	//cpuCost1.SetThreshold(0);
	FUNCTION_WRAPPER(true, "");
	stARpgAbility stOldAbi = abi;
	int64_t oldFightScore = m_stAbility.i64FightScore;

	// 技能等级
	m_stSpecialAbility.Clear();
	bool bCalcSkillExtraLv = false;	//是否要计算技能的额外等级
	if (m_specialAbiFlag & SPECIALABILITY_EQUIP) {
		bCalcSkillExtraLv = true;
		// 获取装备上的特殊属性
		m_Packet.GetPlayerEquipSpecialProper(&m_stSpecialEquipAbility);
	}
	if (m_specialAbiFlag & SPECIALABILITY_LUA) {
		bCalcSkillExtraLv = true;
		calculateLuaSpecialAbility(&m_stSpecialLuaAbility);
	}
	if (m_specialAbiFlag & SPECIALABILITY_BUFF) {
		bCalcSkillExtraLv = true;

	}
	m_stSpecialAbility += m_stSpecialEquipAbility;
	m_stSpecialAbility += m_stSpecialLuaAbility;
	m_stSpecialAbility += m_stSpecialBuffAbility;

	if (bCalcSkillExtraLv) {
		m_cMagic.calcSkillExtarLv(m_stSpecialAbility);		// 计算技能的额外等级
	}
	if (bCalcSkillExtraLv || (m_abilityFlag & ABILITY_FLAG_PASSIVESKILL)) {
		m_cMagic.calcPassiveSkillAbility(&m_stPassiveSkillAbility, this->GetJobType());		// 重新计算被动技能的被动属性
	}

	// 属性
	abi.Clear();
	if (m_abilityFlag & ABILITY_FLAG_EQUIP) {
		m_Packet.GetPlayerEquipProper(&m_stEquipAbility);
	}
	if (m_abilityFlag & ABILITY_FLAG_LUA) {
		calculateLuaAbility(&m_stLuaAbility);
		calculateLuaTimeAbility(&m_stLuaTimeAbility);
	}
	if (m_abilityFlag & ABILITY_FLAG_ATTRPOINT) {
		calculateAttrPointAbility(&m_stAttrPointAbility);
	}
	abi += m_siAbility;
	abi += m_stBaseAbility;
	abi += m_stEquipAbility;
	abi += m_stLuaAbility;
	abi += m_stPassiveSkillAbility;
	abi += m_stLuaTimeAbility;
	abi += m_stAttrPointAbility;
	abi += m_stGmAbility;

	// 计算评分的属性,不加入计算的放在后面
	do {
		m_stTempAbility.Clear();
		m_stTempAbility = abi;
		FirstConvertAbility(&m_stTempAbility, &m_stFirstConvertAbility);
		m_stTempAbility += m_stFirstConvertAbility;
		m_stTempAbility.CalExtraAttr(m_stExtraAbility);
		m_stTempAbility += m_stExtraAbility;
	} while (false);
	calculateBuffAbility();
	abi += m_stBuffAbility;
	FirstConvertAbility(&abi, &m_stFirstConvertAbility);
	abi += m_stFirstConvertAbility;
	// 计算速度
	CalculatingSpeed();
	abi.CalExtraAttr(m_stExtraAbility);
	abi += m_stExtraAbility;

	// 计算战斗评分
	if (m_i64UserOnlyID > 0 && (memcmp(&stOldAbi, &abi, sizeof(abi)) != 0)) {
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("OnChangeFightScore", false, this, &m_stTempAbility, &stOldAbi);
	}
	if (boNotif) {
		stSimpleCretAbility tmpabi;
		tmpabi = abi;
		tmpabi = m_stSpecialAbility;
		if (memcmp(&m_stSimpleAbilityKey, &tmpabi, sizeof(m_stSimpleAbilityKey)) != 0 || oldFightScore != m_stAbility.i64FightScore)
		{
			m_stSimpleAbilityKey = tmpabi;
			stCertAbility retcmd;
			retcmd.dwTempId = GetObjectId();
			retcmd.Ability = tmpabi;
			retcmd.i64FightScore = m_stAbility.i64FightScore;
			SendMsgToMe(&retcmd, sizeof(retcmd));
		}
	}
	m_abilityFlag = 0;
	m_specialAbiFlag = 0;
}

bool CPlayerObj::MoveTo(int nDir, int nmovesetp, int ncx, int ncy, bool boNotCheckObj)
{
	auto curMap = GetEnvir();
	if (!curMap)
		return false;
	stCretMoveRet retcmd;
	if (!CanWalk())
	{
		fullMoveRet(&retcmd, (BYTE)-6);
		SendMsgToMe(&retcmd, sizeof(retcmd));
		g_logger.debug("不能移动 %d,目标(%s)坐标(%s) %d,%d", retcmd.moveerrorcode, getName(), curMap->getMapName(), m_nCurrX, m_nCurrY);
		return false;
	}
	fullMoveRet(&retcmd, (BYTE)-1);
	if (isDie()) {
		SendMsgToMe(&retcmd, sizeof(retcmd));
		g_logger.debug("死亡时移动出错 %d,目标(%s)坐标(%s) %d,%d", retcmd.moveerrorcode, getName(), curMap->getMapName(), m_nCurrX, m_nCurrY);
		return false;
	}
	int nsetp = nmovesetp;
	int ncux = m_nCurrX;
	int ncuy = m_nCurrY;
	int ndx = 0, ndy = 0;
	while (nsetp > 0) {
		if (!(curMap->GetNextPosition(ncux, ncuy, nDir, 1, ndx, ndy) && curMap->CanWalk(this, ndx, ndy, m_nCurrZ, boNotCheckObj))) {
			retcmd.moveerrorcode = (BYTE)-2;
			g_logger.debug("移动出错 %d,目标(%s)坐标(%s) %d,%d -> %d,%d(%d,%d)", retcmd.moveerrorcode, getName(), curMap->getMapName(), m_nCurrX, m_nCurrY, ndx, ndy, ncx, ncy);
			break;
		}
		ncux = ndx;
		ncuy = ndy;
		nsetp--;
	}
	if (nsetp == 0) {
		if (ndx != ncx || ndy != ncy) {
			retcmd.moveerrorcode = (BYTE)-5;//和客户端发上来的目标不一样，发送错误代码，让客户端改变其位置
			SendMsgToMe(&retcmd, sizeof(retcmd));
			g_logger.error("移动出错 和客户端发上来的目标坐标不一样 %d,目标(%s)坐标(%s) %d,%d -> %d,%d(%d,%d)", retcmd.moveerrorcode, getName(),
				curMap->getMapName(), m_nCurrX, m_nCurrY, ndx, ndy, ncx, ncy);
			return false;
		}
		if (nmovesetp > 0) {
			auto gate = curMap->GetGate(ndx, ndy);
			if (gate && gate->dwDstMapId != 0) {
				if (!curMap->CanWalk(ndx, ndy, m_nCurrZ, true)) { g_logger.warn("传送点不能行走,地图 %s 坐标 %d : %d", curMap->getMapName(), ndx, ndy); }
				bool bocanmove = true;
				if (quest_vars_get_var_n("autostate") == 3) {
					bocanmove = false;  //自动战斗不进入传送门
				}
				if (gate->nscriptidx > 0) {
					stAutoSetScriptParam autoparam(this);
					bocanmove = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("gatemovepass", true, this, gate->nscriptidx);
					if (bocanmove)
						if (CUserEngine::getMe().m_scriptsystem.m_LuaVM->IsExistFunctionEx(vformat("moveongate_%d", gate->nscriptidx)))
							bocanmove = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call(vformat("moveongate_%d", gate->nscriptidx), true);
				}
				if (bocanmove) {
					auto sublineid = 0;
					sublineid = CALL_LUARET<int>("GetCorrectLine", 1, this, gate->dwDstMapId, sublineid);
					return MoveToMap(gate->dwDstMapId,0,gate->wDx,gate->wDy,sublineid+300);
				}
			}
		}
		if (nmovesetp <= 0 || curMap->DoMapMove(this, ndx, ndy)) {
			return true;
		}
		retcmd.moveerrorcode = (BYTE)-3;
	}
	SendMsgToMe(&retcmd, sizeof(retcmd));

	return false;
}

void CPlayerObj::BuildPlayerFeature(stPlayerFeature& feature)
{
	feature.btCretType = GetType();
	feature.n_bo_AllFeature = m_cBuff.BuffFeature();
	feature.wTitleId = quest_vars_get_var_n("curtitleid");
	feature.btBattleCamp = m_btBattleCamp;
	feature.dwAtkSpeedPer = m_dwHitIntervalTime;
	feature.btVip = getVipType();
	feature.dwGroupId = m_GroupInfo.dwGroupId;
	feature.btGroupMaster = m_GroupInfo.boMaster ? 1 : 0;
	feature.dwClanId = m_GuildInfo.dwGuildId;
	feature.btNameColor = m_res[ResID::citizenLv];
	feature.dwMoveInterval = m_nMoveInterval / (1 + (m_stAbility[AttrID::MoveSpeedPer] + sConfigMgr.GetMoveSpeed(m_stAbility[AttrID::MoveSpeedPhase])) / 10000.0f);
	feature.dwHeadPortrait = m_dwHeadPortrait;
	feature.dwEmblem = m_GuildInfo.dwEmblemId;
	feature.btMilitaryRank = quest_vars_get_var_n("MilitaryRankId");
	feature.dwReleaseInterval = m_dwCastIntervalTime;
	feature.nGuard = GetGuardFeature();
	feature.siFeature = m_siFeature;
}

void CPlayerObj::fullCretSimpleAbility(stSimpleAbility* psa) {
	FUNCTION_BEGIN;
	m_siAbility.nMaxHP = m_stAbility[AttrID::MaxHP];
	m_siAbility.nMaxMP = m_stAbility[AttrID::MaxMP];
	m_siAbility.nMaxPP = m_stAbility[AttrID::MaxPP];
	if (memcmp(psa, &m_siAbility, sizeof(m_siAbility)) != 0) {
		(*psa) = m_siAbility;
		psa->add(this->quest_vars_get_var_n("ap1"), this->quest_vars_get_var_n("ap2"), this->quest_vars_get_var_n("ap3"), this->quest_vars_get_var_n("ap4"), this->quest_vars_get_var_n("ap5"));
	}
}

bool CPlayerObj::LoadMap(stLoadPlayerData* pgamedata)
{
	CGameMap* pMap = nullptr;
	if (m_wHomeMapID == 0) {
		DWORD homex = 0, homey = 0;
		if (!pMap) {
			pMap = CUserEngine::getMe().m_maphash.FindById(atoi(GameService::getMe().m_svrcfg.szHomeMap), 0, 0);
			homex = GameService::getMe().m_svrcfg.nHomeX;
			homey = GameService::getMe().m_svrcfg.nHomeY;
		}
		if (pMap) {
			m_wHomeMapID = pMap->getMapId();
			m_wHomeCloneMapId = pMap->getMapCloneId();
			m_nHomeX = homex;
			m_nHomeY = homey;
			SetPoint(m_nHomeX, m_nHomeY);
			m_btDirection = DRI_DOWN;
		}
		else {
			g_logger.debug("%s [%s]无法找到地图!", pgamedata->szName, GameService::getMe().m_svrcfg.szHomeMap);
			return false;
		}
	}
	if (pMap == NULL) {
		//获得地图
		pMap = CUserEngine::getMe().m_maphash.FindById(pgamedata->dwmapid, 0, pgamedata->wclonemapid);
		if (!pMap) {
			stHumMapInfo mapinfo;
			mapinfo.curx = pgamedata->x;
			mapinfo.cury = pgamedata->y;
			mapinfo.dwmapid = pgamedata->dwmapid;
			mapinfo.wmapcloneid = pgamedata->wclonemapid;
			mapinfo.btmapcountryid = 0;
			mapinfo.btmapsublineid = GameService::getMe().m_lineId;

			mapinfo.i64UserOnlyId = m_i64UserOnlyID;
			mapinfo.nlevel = m_dwLevel;
			mapinfo.job = m_siFeature.job;
			mapinfo.sex = m_siFeature.sex;
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("gethummapxy", this, &mapinfo);

			pMap = CUserEngine::getMe().m_maphash.FindById(mapinfo.dwmapid, mapinfo.btmapsublineid, mapinfo.wmapcloneid);
			SetPoint(mapinfo.curx, mapinfo.cury);
		}
	}
	if (!pMap)
	{
		pMap = CUserEngine::getMe().m_maphash.FindById(m_wHomeMapID, 0, m_wHomeCloneMapId);
		if (pMap) {
			PosType ndx = (m_nHomeX > pMap->m_nWidth) ? (pMap->m_nWidth / 2) : m_nHomeX;
			PosType ndy = (m_nHomeY > pMap->m_nHeight) ? (pMap->m_nHeight / 2) : m_nHomeY;
			if (pMap->GetNearXY(ndx, ndy, ndx, ndy, 5, 0)) {
				SetEnvir(pMap);
				SetPoint(ndx, ndy);
				m_nHomeX = ndx;
				m_nHomeY = ndy;
				m_btDirection = DRI_DOWN;
			}
			else {
				g_logger.debug("%s [%s]无法找到合适的出生点!", pgamedata->szName, GameService::getMe().m_svrcfg.szHomeMap);
				return false;
			}
		}
		else {
			g_logger.debug("%s [%s]无法找到合适的出生点!", pgamedata->szName, GameService::getMe().m_svrcfg.szHomeMap);
			return false;
		}
	}
	if (pMap) {

		m_changeMapId.all = pMap->getFullMapId();
		m_changeMapPos = { GetX(),GetY() };
		ChangeClientLoadMapData(pMap);
		return true;
	}
	return false;
}
bool CPlayerObj::LoadHumanBaseData(stLoadPlayerData* pgamedata)
{
	FUNCTION_BEGIN;
	stLoadPlayerData& gamedata = *pgamedata;

	m_i64UserOnlyID = gamedata.i64UserOnlyId;
	m_wHomeMapID = gamedata.whomamapid;
	m_wHomeCloneMapId = gamedata.whomeclonemapid;
	m_nHomeX = gamedata.homex;
	m_nHomeY = gamedata.homey;
	m_dwLevel = gamedata.nlevel;
	m_dwLastLevelUpTime = gamedata.dwLastLvupTime ? gamedata.dwLastLvupTime : (DWORD)time(0);
	m_btGmLvl = 0;
	m_dwSaveCount = gamedata.savecount;
	m_dwGold = min(gamedata.dwGold, _MAX_CARRYGOLD_(m_dwLevel));
	m_GroupInfo.dwGroupId = gamedata.dwGroupId;
	m_siFeature = gamedata.siFeature;
	m_siAbility = gamedata.siAbility;
	m_btPkModel = (BYTE)gamedata.dwPkModel;
	m_dwChatModel = gamedata.dwChatModel;
	m_nNowHP = gamedata.dwNowHP;
	m_LifeState = (m_nNowHP <= 0) ? ISDIE : NOTDIE;
	m_nNowMP = gamedata.dwNowMP;
	m_tLoginOuttime = gamedata.tLoginOutTime;
	m_dwPlayerCreateTime = gamedata.dwPlayerCreateTime;
	m_Packet.m_nOpenBagCount = 1;
	m_Packet.m_nOpenStorageCount = max(1, gamedata.nOpenStorageCount);
	m_Packet.m_nOpenBagCellCount = max(gamedata.nOpenBagCellCount, _MIN_BAGCELL_COUNT);
	m_Packet.m_nOpenStorageCellCount = max(gamedata.nOpenStorageCellCount, _MAX_STORAGECELL_COUNT);
	m_dwPlayerOnlineTime = gamedata.dwPlayerOnlineTime;		//角色在线时间
	m_GuildInfo.dwGuildId = gamedata.dwGuildID;
	m_boCanChangeName = (gamedata.btcanchangeusername != 0);
	m_szOldName[0] = 0;
	strcpy_s(m_szOldName, sizeof(m_szOldName) - 1, gamedata.szoldname);
	strcpy_s(m_szSecondPassWord, _MAX_SECONDPASS_LEN_ - 1, pgamedata->szSecondPass);
	strcpy_s(m_szSecondMailAddress, _MAX_SECONDPASS_LEN_ - 1, pgamedata->szSecondMailAddress);
	m_btLeader = gamedata.btLeader;
	m_tLeaderPeriodTime = gamedata.dwLeaderPeriodTime;
	m_dwOriginalZoneid = gamedata.dwOriginalZoneid;
	m_dwHeadPortrait = gamedata.dwHeadPortrait;
	m_nOriginalZone = gamedata.nOriginalZone;
	m_dwUseRmb = gamedata.dwUseRmb;
	m_nVipLevel = gamedata.dwVipLv;
	m_nAddStorePage = gamedata.nWinNum;
	m_dwNoviceGuideId = gamedata.dwNoviceGuideId;
	strcpy_s(m_szLegion, _MAX_NAME_LEN_ - 1, pgamedata->szLegion);
	m_nNowPP = gamedata.nNowPP;
	m_i64BattleSec = gamedata.i64BattleSec;
	m_i64PkSec = gamedata.i64PkSec;
	m_res = gamedata.m_res;
	return true;
}

const char* CPlayerObj::getShowName(char* szbuffer, int nmaxlen) {
	FUNCTION_BEGIN;
	__super::getShowName(szbuffer, nmaxlen);
	return szbuffer;
}

//注意 线程安全 多线程需要修改
static unsigned char g_szSaveDataBuffer[1024 * 1024 * 4];
static unsigned char g_szTmpSaveDataBuffer[1024 * 16];
#define _SAVE_DATA_CHECKNUM_			0x12345678

bool CPlayerObj::LoadHumanData(stLoadPlayerData* pgamedata) {
	FUNCTION_BEGIN;
#define _LOADMOVE(pbindata,bindatasize,len) {\
	pbindata+=len;\
	bindatasize-=len;\
	len=bindatasize;\
	}
	m_Packet.Init(this);
	m_Petm.Init();
	if (pgamedata) {
		m_dwDoubleTime = pgamedata->dwDoubleTime;
		m_dwDoubleRate = pgamedata->dwDoubleRate;
		m_dwBanChatTime = pgamedata->dwBanChatTime;
		m_dwBanPlayerTime = pgamedata->dwBanPlayerTime;
		if (m_dwBanPlayerTime != 0 && m_dwBanPlayerTime > ((DWORD)time(NULL)) && !m_pGateUser->isSocketClose()) {
			BUFFER_CMD(stForceOffLine, offlineCmd, 512);
			offlineCmd->btErrorCode = 1;
			char reasonDesc[256];
			ZeroMemory(reasonDesc, 256);
			sprintf_s(reasonDesc, 255, "您的角色已被封停!");
			offlineCmd->reasonDesc.push_str(reasonDesc);
			SendMsgToMe(offlineCmd, sizeof(*offlineCmd) + offlineCmd->reasonDesc.getarraysize());

			g_logger.debug("服务器踢人 %s:%s:%I64d,踢人代码 BanPlayer", m_pGateUser->m_szAccount, getName(), m_i64UserOnlyID);
			m_pGateUser->notifyGateKiceUser();
			return false;
		}
		else if (m_dwBanPlayerTime <= ((DWORD)time(NULL))) { m_dwBanPlayerTime = 0; }
		if (pgamedata->getBinData().size > 0 && pgamedata->getUncompressSize() > 0) {
			unsigned char* pbindata = &g_szSaveDataBuffer[0];
			unsigned long bindatasize = sizeof(g_szSaveDataBuffer);
			//////////////////////////////////////////////////////////////////////////
			//解压2进制流
			if (uncompresszlib(&pgamedata->getBinData()[0], pgamedata->getBinData().size, pbindata, bindatasize) != Z_OK || bindatasize != pgamedata->getUncompressSize()) {
				g_logger.error("用户 %s 角色 %s 读档数据解压缩失败( 压缩前:%d : 压缩后:%d )!", m_pGateUser->m_szAccount, getName(), pgamedata->getUncompressSize(), pgamedata->getBinData().size);
				pgamedata->getBinData().clear();
				return false;
			}
			//////////////////////////////////////////////////////////////////////////
			//读取2进制流数据 pbindata 数据首指针  bindatasize 数据长度
			pbindata[bindatasize] = 0;
			Json::Reader reader;
			Json::Value json_object;
			if (reader.parse((char*)pbindata, json_object)) {
				int nver = json_object["PbinVer"].asInt();
				m_cBuff.loadbuff(pgamedata->szTmpBuffData, strlen((char*)&pgamedata->szTmpBuffData[0]), nver);
				const char* szBagPacket = json_object["BagPacket"].asCString();
				if (szBagPacket && !m_Packet.load(szBagPacket, (int)strlen(szBagPacket), nver)) {
					g_logger.error("用户 %s 角色 %s 读取包裹失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szStoragePacket = json_object["StoragePacket"].asCString();
				if (szStoragePacket && !m_Packet.load(szStoragePacket, (int)strlen(szStoragePacket), nver)) {
					g_logger.error("用户 %s 角色 %s 读取仓库失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szEquip = json_object["Equip"].asCString();
				if (szEquip && !m_Packet.load(szEquip, (int)strlen(szEquip), nver)) {
					g_logger.error("用户 %s 角色 %s 读取装备失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szAnimalSoulEquip = json_object["AnimalSoulEquip"].asCString();
				if (szAnimalSoulEquip && !m_Packet.load(szAnimalSoulEquip, (int)strlen(szAnimalSoulEquip), nver)) {
					g_logger.error("用户 %s 角色 %s 读取兽魂装备失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szAnimalSoulPackage = json_object["AnimalSoulPacket"].asCString();
				if (szAnimalSoulPackage && !m_Packet.load(szAnimalSoulPackage, (int)strlen(szAnimalSoulPackage), nver)) {
					g_logger.error("用户 %s 角色 %s 读取兽魂包裹失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szSkill = json_object["Skill"].asCString();
				emJobType job = (emJobType)m_pTmpGamedata->siFeature.job;
				if (szSkill && !m_cMagic.loadskill(szSkill, (int)strlen(szSkill), nver, job)) {
					g_logger.error("用户 %s 角色 %s 读取技能失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szShortCut = json_object["ShortCut"].asCString();
				if (szShortCut && !m_ShortCut.Load(szShortCut, (int)strlen(szShortCut), nver)) {
					g_logger.error("用户 %s 角色 %s 读取快捷键失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szVars = json_object["Vars"].asCString();
				if (szVars && !m_vars.load(szVars, (int)strlen(szVars), nver)) {
					g_logger.error("用户 %s 角色 %s 读取全局变量失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}
				const char* szDayVars = json_object["DayVars"].asCString();
				if (szDayVars && !m_dayVars.load(szDayVars, (int)strlen(szDayVars), nver)) {
					g_logger.error("用户 %s 角色 %s 读取每日清除全局变量失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szQuestList = json_object["QuestList"].asCString();
				if (szQuestList && !m_QuestList.load(szQuestList, (int)strlen(szQuestList), nver)) {
					g_logger.error("用户 %s 角色 %s 读取任务列表失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szQuestCompleteMark = json_object["QuestCompleteMark"].asCString();
				if (szQuestCompleteMark && !m_QuestCompleteMark.load(szQuestCompleteMark, (int)strlen(szQuestCompleteMark), nver)) {
					g_logger.error("用户 %s 角色 %s 读取任务完成标记失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}
				const char* szClientSet = json_object["ClientSet"].asCString();
				if (szClientSet && !LoadClientSet(szClientSet, (int)strlen(szClientSet), nver)) {
					g_logger.error("用户 %s 角色 %s 读取玩家设置失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szSaveBuff = json_object["SaveBuff"].asCString();
				if (szSaveBuff && !m_cBuff.loadbuff(szSaveBuff, (int)strlen(szSaveBuff), nver)) {
					g_logger.error("用户 %s 角色 %s 读取玩家BUFF失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}
				const char* szTmpBagPacket = json_object["TmpBagPacket"].asCString();
				if (szTmpBagPacket && !m_Packet.load(szTmpBagPacket, (int)strlen(szTmpBagPacket), nver)) {
					g_logger.error("用户 %s 角色 %s 读取临时包裹失败!", m_pGateUser->m_szAccount, getName());
					pgamedata->getBinData().clear();
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

size_t g_savedatamaxsize = 0;
size_t g_savedatazlibmaxsize = 0;

int CPlayerObj::fullPlayerSaveData(stSavePlayerDataCmd* pcmdbuf, int nmaxlen, int saveType, int nCrossTradeId) {
	FUNCTION_BEGIN;

#define _SAVEMOVE(pbindata,bindatasize,nLen) {\
	pbindata+=nLen;\
	bindatasize-=nLen;\
	nLen= bindatasize;\
	g_savedatamaxsize=max( (size_t)(pbindata-&g_szSaveDataBuffer[0]),g_savedatamaxsize );\
	}

	pcmdbuf->gamedata.getBinData().clear();
	CPlayerObj* player = this;
	if (player) {
		if (player->m_btReadyState >= _READYSTATE_SVR_READY_ && player->GetEnvir()) {
			strcpy_s(pcmdbuf->szAccount, sizeof(pcmdbuf->szAccount) - 1, player->getAccount());
			strcpy_s(pcmdbuf->gamedata.szName, sizeof(pcmdbuf->gamedata.szName) - 1, player->getName());
			pcmdbuf->tmpid = player->m_pGateUser->m_tmpid;
			pcmdbuf->checknum = 0;
			//////////////////////////////////////////////////////////////////////////
			//填充需要保存的数据字段
			stLoadPlayerData& gamedata = pcmdbuf->gamedata;
			gamedata.i64UserOnlyId = player->m_i64UserOnlyID;
			gamedata.btgmlvl = 0;
			if (!GetEnvir()->isNoOffLineSaveHome()) {
				gamedata.dwmapid = player->GetEnvir()->getMapId();
				gamedata.wclonemapid = player->GetEnvir()->getMapCloneId();
				gamedata.x = player->m_nCurrX;
				gamedata.y = player->m_nCurrY;
				gamedata.z = player->m_nCurrZ;
			}
			else {
				gamedata.dwmapid = player->m_wHomeMapID;
				gamedata.wclonemapid = player->m_wHomeCloneMapId;
				gamedata.x = player->m_nHomeX;
				gamedata.y = player->m_nHomeY;
				gamedata.z = player->m_nCurrZ;
			}
			gamedata.whomamapid = player->m_wHomeMapID;
			gamedata.whomeclonemapid = player->m_wHomeCloneMapId;
			gamedata.homex = player->m_nHomeX;
			gamedata.homey = player->m_nHomeY;
			gamedata.nlevel = player->m_dwLevel;
			gamedata.dwLastLvupTime = player->m_dwLastLevelUpTime;
			gamedata.dwNowHP = player->m_nNowHP;
			gamedata.dwNowMP = player->m_nNowMP;
			gamedata.nNowPP = player->m_nNowPP;
			gamedata.dwGold = min(player->m_dwGold, _MAX_CARRYGOLD_(m_dwLevel));
			gamedata.tLoginOutTime = player->m_tLoginOuttime;
			gamedata.nOpenBagCount = player->m_Packet.m_nOpenBagCount;
			gamedata.nOpenStorageCount = player->m_Packet.m_nOpenStorageCount;
			gamedata.dwGroupId = 0;
			gamedata.dwChatModel = player->m_dwChatModel;
			gamedata.dwPkModel = player->m_btPkModel;
			gamedata.dwDoubleTime = player->m_dwDoubleTime;
			gamedata.dwDoubleRate = player->m_dwDoubleRate;
			gamedata.dwBanChatTime = player->m_dwBanChatTime;
			gamedata.dwBanPlayerTime = player->m_dwBanPlayerTime;
			gamedata.btcanchangeusername = player->m_boCanChangeName ? 1 : 0;
			gamedata.szoldname[0] = 0;
			strcpy_s(gamedata.szoldname, sizeof(gamedata.szoldname) - 1, player->m_szOldName);
			time_t curtime = time(NULL);
			m_dwPlayerOnlineTime = m_dwPlayerOnlineTime + (curtime - m_dwLastfullSaveRefTime);		//角色在线时间
			m_dwAccountOnlineTime = m_dwAccountOnlineTime + (curtime - m_dwLastfullSaveRefTime);		//账号在线时间
			m_dwLastfullSaveRefTime = curtime;
			gamedata.dwPlayerOnlineTime = m_dwPlayerOnlineTime;
			gamedata.dwGuildID = m_GuildInfo.dwGuildId;
			strcpy_s(gamedata.szSecondPass, _MAX_SECONDPASS_LEN_ - 1, m_szSecondPassWord);
			strcpy_s(gamedata.szSecondMailAddress, _MAX_SECONDPASS_LEN_ - 1, m_szSecondMailAddress);
			gamedata.btLeader = m_btLeader;
			gamedata.dwLeaderPeriodTime = m_tLeaderPeriodTime;
			gamedata.dwOriginalZoneid = m_dwOriginalZoneid;
			gamedata.nOpenBagCellCount = m_Packet.m_nOpenBagCellCount;
			gamedata.nOpenStorageCellCount = m_Packet.m_nOpenStorageCellCount;
			gamedata.dwHeadPortrait = m_dwHeadPortrait;
			gamedata.nOriginalZone = m_nOriginalZone;
			gamedata.dwUseRmb = m_dwUseRmb;
			gamedata.dwVipLv = m_nVipLevel;
			gamedata.nWinNum = m_nAddStorePage;
			gamedata.dwNoviceGuideId = player->m_dwNoviceGuideId;

			strcpy_s(gamedata.szLegion, _MAX_NAME_LEN_ - 1, m_szLegion);
			gamedata.i64BattleSec = m_i64BattleSec;				//脱离战斗时间
			gamedata.i64PkSec = m_i64PkSec;
			strcpy_s(gamedata.szTxSubPlatformName, sizeof(gamedata.szTxSubPlatformName) - 1, this->getSubPlatform());
			DWORD retlen = sizeof(g_szTmpSaveDataBuffer);
			player->m_cBuff.savetmpbuff((char*)g_szTmpSaveDataBuffer, retlen);
			if (sizeof(gamedata.szTmpBuffData) >= retlen)
			{
				memcpy(gamedata.szTmpBuffData, g_szTmpSaveDataBuffer, retlen);
			}
			gamedata.siFeature = m_siFeature;
			gamedata.m_res = player->m_res;
			fullCretSimpleAbility(&gamedata.siAbility);
			stSpaveMoveInfo* pSpaveMoveInfo = player->getSwitchSvrInfo();
			if (pSpaveMoveInfo && player->isSwitchSvr()) {
				gamedata.dwmapid = pSpaveMoveInfo->DstMap->getMapId();
				gamedata.wclonemapid = pSpaveMoveInfo->dwCloneMapId;

				gamedata.x = pSpaveMoveInfo->X;
				gamedata.y = pSpaveMoveInfo->Y;
				gamedata.z = gamedata.z;
			}
			//////////////////////////////////////////////////////////////////////////
			//2进制数据流填充
			unsigned char* pbindata = &g_szSaveDataBuffer[0];
			unsigned long bindatasize = sizeof(g_szSaveDataBuffer);
			DWORD len = 0;

			const char* PszBin = vformat("{\"PbinVer\":%d", _SAVE_DATA_VER_);
			memcpy(pbindata, PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = ",\"BagPacket\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_Packet.saveBagPacket((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存包裹数据失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"StoragePacket\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_Packet.saveStoragePacket((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存仓库数据失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"Equip\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_Packet.saveEquip((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存装备数据失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"Skill\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_cMagic.saveskill((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存技能数据失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"ShortCut\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_ShortCut.Save((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存快捷键数据失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"Vars\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_vars.save((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存全局变量失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"DayVars\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_dayVars.save((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存每日清除全局变量失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"QuestList\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_QuestList.save((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存任务列表失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"QuestCompleteMark\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_QuestCompleteMark.save((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存任务完成标记失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"ClientSet\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!SaveClientSet((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存玩家设置失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"SaveBuff\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_cBuff.savebuff((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存玩家BUFF失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\",\"TmpBagPacket\":\"";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);

			if (!m_Packet.saveTmpBagPacket((char*)pbindata, len)) {
				g_logger.error("用户 %s 角色 %s 保存玩家临时背包信息失败!", m_pGateUser->m_szAccount, getName());
				return 0;
			}
			_SAVEMOVE(pbindata, bindatasize, len);

			PszBin = "\"}";
			memcpy((void*)(pbindata), PszBin, strlen(PszBin));
			len = strlen(PszBin);
			_SAVEMOVE(pbindata, bindatasize, len);
			//////////////////////////////////////////////////////////////////////////
			//打包2进制流
			gamedata.getUncompressSize() = (pbindata - &g_szSaveDataBuffer[0]);
			unsigned long noutlen = nmaxlen;
			if (compresszlib(&g_szSaveDataBuffer[0], gamedata.getUncompressSize(), &gamedata.getBinData()[0], noutlen) != Z_OK) {
				g_logger.error("用户 %s 角色 %s 存档数据压缩失败( 压缩前:%d : 压缩缓冲:%d )!", player->m_pGateUser->m_szAccount, player->getName(), gamedata.getUncompressSize(), nmaxlen);
				pcmdbuf->gamedata.getBinData().clear();
				return 0;
			}
			gamedata.getBinData().size = noutlen;
			g_savedatazlibmaxsize = max(g_savedatazlibmaxsize, noutlen + sizeof(stSavePlayerDataCmd));
			//////////////////////////////////////////////////////////////////////////
			//存档数据的代码不要写在下面
			pcmdbuf->gamedata.savecount = player->m_dwSaveCount;
			player->m_dwSaveCount++;
			pcmdbuf->gamedata.lastsavetime = time(NULL);
			return pcmdbuf->getSize();
		}
		else {
			pcmdbuf->szAccount[0] = 0;
			pcmdbuf->gamedata.szName[0] = 0;
		}
	}
	return 0;
}
int CPlayerObj::fullPlayerChangeSvrData(stSavePlayerDataCmd* pcmdbuf, int nmaxlen, int saveType) {
	FUNCTION_BEGIN;
	CPlayerObj* player = this;
	if (player) {
		stLoadPlayerData& gamedata = pcmdbuf->gamedata;
		gamedata.dwGroupId = player->m_GroupInfo.dwGroupId;
		gamedata.dwPlayerCreateTime = player->m_dwPlayerCreateTime;
	}
	return 0;
}
bool CPlayerObj::HandleMoveCross() {
	DWORD dwSvrIdx = m_changeMapId.part.line + 300;
	if (sConfigMgr.m_fastTransfer) {
		if ((GameService::getMe().m_gamesvrStateMask & (1 << (dwSvrIdx - 300))) == 0) {
			g_logger.debug("玩家%s 传送失败! 目标游戏服:%d 检测到未开启", this->getName(), dwSvrIdx);
			return true;
		}
		OnPlayerBeforeChangeSvrSaveData();
		if (m_pGateUser->m_OwnerDbSvr->push_back2save(this, _SAVE_TYPE_CHANGESVR_)) {
			m_dwNextSaveRcdTime = time(NULL) + GameService::getMe().m_svrcfg.dwsavercdintervaltime;
			PTR_CMD(stFastTransferPlayer, cmd, getsafepacketbuf());
			if (fullPlayerSaveData(&cmd->playerBinData, getsafepacketbuflen() - sizeof(stFastTransferPlayer) - 1024, _SAVE_TYPE_CHANGESVR_) > 0) {
				FillFastTransferPlayerCmd(cmd, this);
				SendMsgToMe(cmd, sizeof(*cmd) - sizeof(cmd->playerBinData) + cmd->playerBinData.getSize());

				stNotifyPlayerChangeGameSvrCmd ncc;//发给login记录下信息
				FillNotifyPlayerChangeGameSvrCmd(&ncc, this);
				m_pGateUser->m_OwnerLoginSvr->sendcmd(&ncc, sizeof(ncc));
				m_boIsWaitChangeSvr = true;
				m_btKickState = KICK_CHANGEGAMESVROK;
				m_dwDoKickTick = 0;
				g_logger.debug(" %s:%s 切换服务器成功! ts:%d", getAccount(), this->getName(), timeGetTime());
			}
		}
		return true;
	}
	stPreChangeGameSvrCmd checkcmd;
	//FillPreChangeGameSvrCmd(&checkcmd, pSpaceMoveInfo);
	if (CUserEngine::getMe().BroadcastGameSvr(&checkcmd, sizeof(checkcmd), MAKELONG(dwSvrIdx, _GAMESVR_TYPE), true, checkcmd.dwDestZoneId, checkcmd.wDestTradeid)) {
	}
	return true;
}

void CPlayerObj::CrossSvrSavePlayerData(BYTE btSaveType) {
	FUNCTION_BEGIN;
	if (!CUserEngine::getMe().isCrossSvr() || !m_dwSrcZoneId || m_btCrossKickType == 1)return;
	PTR_CMD(stCrossSavePlayerData, psendcmd, getsafepacketbuf());
	psendcmd->btSaveType = btSaveType;
	if (fullPlayerSaveData(&psendcmd->data, getsafepacketbuflen() - sizeof(*psendcmd) - 1024, btSaveType, m_wSrcTrade) > 0) {
		stHumMapInfo mapinfo;
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("GetSaveZoneInfo", &mapinfo);
		CGameMap* pMap = CUserEngine::getMe().m_maphash.FindById(mapinfo.dwmapid, 0);
		if (pMap) {
			psendcmd->data.gamedata.dwmapid = mapinfo.dwmapid;
			psendcmd->data.gamedata.btCountryID = mapinfo.btCountryID;
			psendcmd->data.gamedata.wclonemapid = mapinfo.wmapcloneid;
			psendcmd->data.gamedata.btmapsublineid = mapinfo.btmapsublineid;
			psendcmd->data.gamedata.x = mapinfo.curx;
			psendcmd->data.gamedata.y = mapinfo.cury;
			CUserEngine::getMe().BroadcastGameSvr(psendcmd, psendcmd->getSize(), m_dwSrcGameSvrIdType/*pMap->getSvrIdType()*/, true, m_dwSrcZoneId, m_wSrcTrade);
			m_dwNextSaveRcdTime = time(NULL) + GameService::getMe().m_svrcfg.dwsavercdintervaltime;
		}
		else {
			m_dwNextSaveRcdTime = time(NULL) + _random(10);
		}
	}
	else {
		m_dwNextSaveRcdTime = time(NULL) + _random(10);
	}
}

bool CPlayerObj::AddExp(int& nNew, const char* szopt)	//经验改变
{
	FUNCTION_BEGIN;
	int LimitLv = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("LimitLv", 100, this); 
	if (LimitLv) {
		if (m_dwLevel >= (DWORD)LimitLv) {
			return false;
		}
	}
	while (nNew >= m_i64MaxExp) {
		nNew -= m_i64MaxExp;
		LevelUp(m_dwLevel + 1);
	}

	if (nNew - m_res[ResID::exp] > 30000) {
		GameService::getMe().Send2LogSvr(_SERVERLOG_GETEXP_, 0, 0, this, "'%s','%s','%s',%I64d,%d,%I64u,%d,'%s'",
			szopt ? szopt : "other-player",
			getAccount(),
			getName(),
			m_i64UserOnlyID,
			m_dwLevel,
			m_res[ResID::exp],
			nNew-m_res[ResID::exp],
			GetEnvir()->getMapName()
		);
	}
	return true;
}

void CPlayerObj::SendDropItem(CItem* pItem, int nX, int nY) {
	if (pItem) {
		stCretAddDropItemMsg dropMsg;
		dropMsg.wX = nX;
		dropMsg.wY = nY;
		dropMsg.item = pItem->m_Item;

		SendMsgToMe(&dropMsg, sizeof(dropMsg));
	}
}

bool CPlayerObj::LuaLevelUp(int lv)
{
	LevelUp(lv);
	return true;
}

void CPlayerObj::LevelUp(int newLv)
{
	FUNCTION_BEGIN;
	auto LimitLv = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("LimitLv", 100, this);
	newLv = max(newLv, 1);
	newLv = min(newLv, LimitLv);
	DWORD old = m_dwLevel;
	m_dwLastLevelUpTime = (DWORD)time(0);
	m_dwLevel = newLv;

	//m_cMagic.CurLevelSkillStudy(m_dwLevel, (emJobType)m_siFeature.job);
	GetBaseProperty();
	stCretLevelUp retcmd;
	retcmd.dwTempId = GetObjectId();
	retcmd.i64LeftExp = m_res[ResID::exp];
	retcmd.i64MaxExp = m_i64MaxExp;
	retcmd.dwLevel = m_dwLevel;
	retcmd.wMaxLevel = LimitLv;
	retcmd.dwLeveUpTime = time(NULL);
	retcmd.btShow = 0;
	SendRefMsg(&retcmd, sizeof(retcmd));
	SENDMSG2SUPER(stSendOhterMsgSuperSrv, getName(), &retcmd, sizeof(retcmd));
	DoChangeProperty(m_stAbility, true, __FUNC_LINE__);
	StatusValueChange(stCretStatusValueChange::hp, m_stAbility[AttrID::MaxHP], __FUNC_LINE__);
	StatusValueChange(stCretStatusValueChange::mp, m_stAbility[AttrID::MaxMP], __FUNC_LINE__);
	StatusValueChange(stCretStatusValueChange::pp, m_stAbility[AttrID::MaxPP], __FUNC_LINE__);
	if (old != newLv)
	{
		CALL_LUA("PlayerUpLv", this, old);
		GameService::getMe().Send2LogSvr(_SERVERLOG_LEVELUP_, 0, 0, this, "'%s','%s','%s',%I64d,%d,%d,%I64u,'%s'",
			"levelup",
			getAccount(),
			getName(),
			m_i64UserOnlyID,
			old,
			m_dwLevel,
			m_res[ResID::exp],
			GetEnvir()->getMapName()
		);
		GroupInfoChanged();
		UpdateToGlobalSvr();
		UpdateToSuperSvr();
	}
}

bool CPlayerObj::xfxDecode(int nCheckNum, const char* pszXfxMsg) {
	if (!pszXfxMsg) {
		return false;
	}
	MD5_DIGEST m_szMd5Pass;
	char tmppass[256];
	DWORD passkey[1];
	passkey[0] = LODWORD(m_i64UserOnlyID) * m_dwLevel;

	char szCheckPass[256] = { 0 };
	CopyMemory(szCheckPass + 4, &passkey, sizeof(passkey));
	szCheckPass[0] = szCheckPass[7];
	szCheckPass[1] = szCheckPass[6];
	szCheckPass[2] = szCheckPass[5];
	szCheckPass[3] = szCheckPass[4];

	MD5_DIGEST sztmpMd5Pass;

	sprintf_s(tmppass, 256 - 1, "%s", pszXfxMsg);
	MD5String(&tmppass[0], &sztmpMd5Pass);
	CopyMemory(&m_szMd5Pass, &sztmpMd5Pass, sizeof(m_szMd5Pass));


	CopyMemory(&szCheckPass[sizeof(passkey)], &sztmpMd5Pass, sizeof(sztmpMd5Pass));
	int nmd5len = sizeof(passkey) + sizeof(sztmpMd5Pass);
	if (MD5Data(&szCheckPass, nmd5len, &sztmpMd5Pass)) {
		int ncm = 0;
		for (int i = 0; i < sizeof(sztmpMd5Pass); i++) {
			ncm += sztmpMd5Pass[i];
		}

		if (ncm == nCheckNum) {
			return true;
		}
	}
	return false;
}

bool CPlayerObj::PkChange(int nChanged) {
	FUNCTION_BEGIN;
	if (nChanged < 0 && m_res[ResID::pk] < (DWORD)abs(nChanged)) return false;

	if (nChanged != 0)
	{
		m_res[ResID::pk] += nChanged;
	}
	return true;
}

void CPlayerObj::SendPlayerProperty()
{
	FUNCTION_BEGIN;
	stCretCharBase retcmd;
	retcmd.btStorageCount = this->m_Packet.m_nOpenStorageCount;
	retcmd.btPkModel = m_btPkModel;				//当前PK模式
	retcmd.nAgePoint = m_nAgePoint;				//年龄点数
	retcmd.Copy(m_res);							//资源
	retcmd.dwLevel = m_dwLevel;					//当前等级
	retcmd.wMaxLevel = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("LimitLv", 100, this);// +m_stAbility.LevelAdd;
	retcmd.nMaxExp = (int)m_i64MaxExp;			//最大经验
	//retcmd.movespeed=max((m_petm.m_curZuojiPet!=0 && m_nCurrZ!=0 && m_petm.getZuojiPet()!=0)?m_petm.m_curZuojiPet->GetFeijianSpeed():1,1);					//根据飞行器获得移动速度
	SendMsgToMe(&retcmd, sizeof(retcmd));
	StatusValueChange(stCretStatusValueChange::hp, 0, __FUNC_LINE__, true);
	StatusValueChange(stCretStatusValueChange::mp, 0, __FUNC_LINE__, true);
	StatusValueChange(stCretStatusValueChange::pp, 0, __FUNC_LINE__, true);
}

stShortCuts* CPlayerObj::GetShortCuts(DWORD dwId) {
	FUNCTION_BEGIN;
	return m_ShortCut.FindShortCuts(dwId);
}

bool CPlayerObj::AddShortCuts(DWORD dwId, emShortCutsType emType, BYTE btValue, BYTE btRaw, BYTE btCol) {
	FUNCTION_BEGIN;
	bool ret = m_ShortCut.AddShortCuts(dwId, emType, btValue, btRaw, btCol);
	if (ret) {
		stSetShortCuts retcmd;
		retcmd.ErrorCode = SETSHORTCUTS_SUCCESS;
		retcmd.shortcuts.i64Id = dwId;
		retcmd.shortcuts.emShortCuts = emType;
		retcmd.shortcuts.btShortCuts = btValue;
		retcmd.shortcuts.btRow = btRaw;
		retcmd.shortcuts.btCol = btCol;
		retcmd.oldrow = 255;
		retcmd.oldcol = 255;
		SendMsgToMe(&retcmd, sizeof(stSetShortCuts));
	}
	return ret;
}

void CPlayerObj::GroupInfoChanged()
{
	FUNCTION_BEGIN;
	if (m_GroupInfo.dwGroupId) {
		stGlobalGroupMemberInfoChange memberinfochange;
		if (!GetEnvir()) return;
		memberinfochange.memberinfo.i64OnlyId = m_i64UserOnlyID;
		strcpy_s(memberinfochange.memberinfo.szName, sizeof(memberinfochange.memberinfo.szName) - 1, getName());
		memberinfochange.memberinfo.dwZSLevel = 0;
		memberinfochange.memberinfo.dwLevel = m_dwLevel;
		memberinfochange.memberinfo.btViplvl = m_nVipLevel;
		strcpy_s(memberinfochange.memberinfo.szGuildName, sizeof(memberinfochange.memberinfo.szGuildName), m_GuildInfo.szGuildName);
		CUserEngine::getMe().SendMsg2GlobalSvr(&memberinfochange, sizeof(stGlobalGroupMemberInfoChange));
	}
}

void CPlayerObj::SendPosToGroupMember() {
	FUNCTION_BEGIN;
	if (m_GroupInfo.dwGroupId) {
		stGroupMemberPosition position;
		position.i64OnlyId = m_i64UserOnlyID;
		position.nX = m_nCurrX;
		position.nY = m_nCurrY;
		CUserEngine::getMe().SendMsg2GlobalSvr(&position, sizeof(stGroupMemberPosition));
	}
}

void CPlayerObj::SendAllPosToGroupMember() {
	FUNCTION_BEGIN;
	if (m_GroupInfo.dwGroupId) {
		stGroupMemberAllPosition postion;
		postion.i64OnlyId = m_i64UserOnlyID;
		postion.nX = m_nCurrX;
		postion.nY = m_nCurrY;
		postion.dwMapId = GetEnvir()->getMapId();
		strcpy_s(postion.szMapName, sizeof(postion.szMapName) - 1, GetEnvir()->getFullMapName());
		CUserEngine::getMe().SendMsg2GlobalSvr(&postion, sizeof(stGroupMemberAllPosition));

		std::unordered_set<CPlayerObj*> playerSet;
		if (GetEnvir()->GetGroupPlayer(m_GroupInfo.dwGroupId, playerSet)) {
			for (auto player : playerSet)
			{
				if (player)
					player->SendPosToGroupMember();
			}
		}
	}
}

void CPlayerObj::SendPosToRelationMember() {
	FUNCTION_BEGIN;
	stRelationInnerChangeMap cmd;
	cmd.nMapId = GetEnvir()->getMapId();
	strcpy_s(cmd.szMapName, sizeof(cmd.szMapName) - 1, GetEnvir()->getMapName());

	SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv, m_i64UserOnlyID, &cmd, sizeof(stRelationInnerChangeMap));
}

void CPlayerObj::UpdateSimpleFeatureToGlobalSvr() {
	FUNCTION_BEGIN;
	if (m_GroupInfo.dwGroupId) {
		stGlobalFeatureChange cmd;
		cmd.i64OnlyId = m_i64UserOnlyID;
		cmd.siFeature = m_siFeature;
		CUserEngine::getMe().SendMsg2GlobalSvr(&cmd, sizeof(stGlobalFeatureChange));
	}
}

void CPlayerObj::GroupMonGetReward(CMonster* pMonster, DWORD dwLevel, CGameMap* pMap,
	int nX, int nY, int nRange, bool selfGet) const
{
	if (!pMap || !pMonster) {
		return;
	}
	auto groupIt = pMap->m_groupMembers.find(m_GroupInfo.dwGroupId);
	if (groupIt == pMap->m_groupMembers.end()) {
		return;
	}
	const auto& groupPlayers = groupIt->second;
	const size_t groupSize = groupPlayers.size();
	if (groupSize == 0) {
		return;
	}
	const int expAddPercent = sConfigMgr.GetGroupExpAdd(groupSize);
	const float expAddMultiplier = expAddPercent ? expAddPercent / 100.f : 0.f;
	const double expDistributionFactor = (1.0 + expAddMultiplier) / groupSize;

	const std::string killEventPrefix = "kill-" + pMonster->Name() + "-gp";

	for (const auto& player : groupPlayers) {
		if (!player) {
			continue;
		}
		const bool canGetReward = (player == this && selfGet) ||
			(player != this && player->CanGetMonReward(pMonster));

		if (canGetReward) {
			const float expMultiplier = 1.0f + player->m_stAbility[AttrID::ExpMul] / 10000.0f;
			const int playerExp = static_cast<int>(pMonster->GetMonsterDataBase()->CanHaveExp * expDistributionFactor * expMultiplier);
			player->ResChange(ResID::exp, playerExp, killEventPrefix.c_str());
		}
		player->KillMonster(pMonster);
	}
}

void CPlayerObj::ServerGetLeaveGroup() {
	FUNCTION_BEGIN;
	stGlobalLeaveGroup retcmd;
	retcmd.i64Onlyid = m_i64UserOnlyID;
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd, sizeof(stGlobalLeaveGroup));
}

int CPlayerObj::getTotalTargetPlayerNum(std::vector<CCreature*> vTarget)
{
	int nTotalTargeNum = 0;
	for (std::vector<CCreature*>::iterator it = vTarget.begin(); it != vTarget.end(); it++)
	{
		CCreature* pTargetCret = (*it);
		if (pTargetCret->isPlayer())
			nTotalTargeNum++;
	}
	return nTotalTargeNum;
}

int CPlayerObj::getTotalTargetNum(std::vector<CCreature*> vTarget)
{
	int nTotalTargeNum = 0;
	for (std::vector<CCreature*>::iterator it = vTarget.begin(); it != vTarget.end(); it++)
	{
		CCreature* pTargetCret = (*it);
		if (pTargetCret)
			nTotalTargeNum++;
	}
	return nTotalTargeNum;
}

void CPlayerObj::RankTopToSuper(emRankType RankType) {
	FUNCTION_BEGIN;
	BUFFER_CMD(stSendRankMsgSuperSrv, rankcmd, stBasePacket::MAX_PACKET_SIZE);
	stAddRankHumanData addrankdata;
	switch (RankType)
	{
	case Rank_Max_Count:
	{
		addrankdata.nRankType = RankType;
		strcpy_s(addrankdata.stHuman.szName, _MAX_NAME_LEN_ - 1, getName());
		addrankdata.stHuman.dwLevel = m_dwLevel;
		addrankdata.stHuman.dwZSLevel = 0;
		addrankdata.stHuman.i64OnlyId = m_i64UserOnlyID;
		strcpy_s(addrankdata.stHuman.szGuildName, _MAX_NAME_LEN_ - 1, m_GuildInfo.szGuildName);
		addrankdata.stHuman.btSex = GetSexType();
		addrankdata.stHuman.btJob = GetJobType();
		addrankdata.stHuman.dwLevelUpTime = m_dwLastLevelUpTime;
		addrankdata.stHuman.dwPrestige = quest_vars_get_var_n("prestigelv");
		addrankdata.stHuman.wLadder = quest_vars_get_var_n("Ladder_MyRank");
		addrankdata.stHuman.btTitleLvl = quest_vars_get_var_n("titlelevel");
		DWORD dwIntensifyTime = quest_vars_get_var_n("intensifytime");
		DWORD dwShenZhuTime = quest_vars_get_var_n("shenzhutime");
		addrankdata.stHuman.dwIntensifyTime = dwIntensifyTime > dwShenZhuTime ? dwIntensifyTime : dwShenZhuTime;
		addrankdata.stHuman.dwTitleTime = quest_vars_get_var_n("titletime");
		addrankdata.stHuman.dwPrestigeTime = quest_vars_get_var_n("prestigelvuptime");
		addrankdata.stHuman.dwVipLevel = getVipLevel();
		addrankdata.stHuman.dwOfflineTime = time(NULL);
		addrankdata.stHuman.btMilitaryRank = quest_vars_get_var_n("MilitaryRank"); //军衔
		addrankdata.stHuman.i64FightScore = m_stAbility.i64FightScore;
	}break;
	}
	rankcmd->msg.push_back((char*)&addrankdata, sizeof(addrankdata), __FUNC_LINE__);
	CUserEngine::getMe().SendMsg2SuperSvr(rankcmd, sizeof(*rankcmd) + rankcmd->msg.getarraysize());
}

int CPlayerObj::EquipIsOk(DWORD dwBaseid)//检查人物身上是否装备了这个物品
{
	if (!dwBaseid) { return 0; }
	for (int i = 0; i < EQUIP_MAX_COUNT; i++)
	{
		if (m_Packet.m_stEquip[i] && m_Packet.m_stEquip[i]->GetItemBaseID() == dwBaseid)
		{
			return 1;
		}
	}
	return 0;
}

int CPlayerObj::PacketItemIsOk(DWORD dwBaseid, DWORD num)//检查人物包裹中是否有这个物品，和物品数量是否满足
{
	if (!dwBaseid) { return 0; }
	if (m_Packet.FindItemInBagAllByBaseId(dwBaseid, num))
	{
		return 1;
	}
	else { return 0; }
}

int CPlayerObj::PacketItemIsOkWithBindSta(DWORD dwBaseid, DWORD num, BYTE btBind)
{
	if (!dwBaseid) { return 0; }
	if (m_Packet.FindItemInBagAllByBaseIdWithBindSta(dwBaseid, num, btBind))
	{
		return 1;
	}
	else { return 0; }
}


int CPlayerObj::GroupIsOk()//检查是否组队
{
	if (m_GroupInfo.dwGroupId)
	{
		return 1;
	}
	return 0;
}

int CPlayerObj::ItemDeleteByBaseIdIsOk(DWORD dwBaseid, DWORD num, const char* pszLogStr)//删除人物身上物品，名字，数量
{
	if (!dwBaseid) { return 0; }
	if (m_Packet.DeleteItemInBagAllByBaseId(dwBaseid, num, pszLogStr))
	{
		return 1;
	}
	else { return 0; }
}

int CPlayerObj::ItemDeleteIDIsOk(const char* luatmpid, DWORD num, const char* pszLogStr) {

	FUNCTION_BEGIN;
	int64_t tmpid = 0;
	if (luatmpid && luatmpid[0] != 0) {
		tmpid = CItem::strToi642(luatmpid);
		//sscanf(luatmpid,"%I64d",&tmpid);
	}
	if (tmpid != 0 && m_Packet.DeleteItemTmpId(tmpid, num, pszLogStr)) {
		//m_Packet.SendBagDeleteItem(tmpid);
		return 1;
	}
	return 0;
}

int CPlayerObj::ItemDeleteInBody(CItem* pItem, const char* pszLogStr) {
	FUNCTION_BEGIN;
	if (!pItem) return 0;
	if (!m_Packet.DeleteItemInBody(pItem, pszLogStr)) return 0;
	return 1;
}


int CPlayerObj::ItemDeleteInBag(CItem* pItem, DWORD nCount, const char* pszLogStr) {
	FUNCTION_BEGIN;
	if (pItem) {
		if (m_Packet.DeleteItemInBag(pItem, nCount, true, false, pszLogStr)) {
			return 1;
		}
		else if (m_Packet.DeleteItemInTmpBag(pItem, nCount, true, false, pszLogStr)) {
			return 1;
		}
		else if (m_Packet.DeleteItemInActBag(pItem, nCount)) {
			return 1;
		}
	}
	return 0;
}

int CPlayerObj::ItemDeleteInAll(CItem* pItem, DWORD nCount, const char* pszLogStr) {
	FUNCTION_BEGIN;
	if (pItem) {
		bool boDelete = false;
		int64_t i64Id = pItem->GetItemID();
		if (!boDelete && m_Packet.DeleteItemInBody(pItem, pszLogStr)) {
			boDelete = true;
		}
		if (!boDelete && m_Packet.DeleteItemInStorage(pItem, pszLogStr)) {
			m_Packet.SendStorageDeleteItem(i64Id);
			boDelete = true;
		}
		if (!boDelete && m_Packet.DeleteItemInBag(pItem, nCount, true, false, pszLogStr)) {
			boDelete = true;
		}
		if (boDelete) {
			return 1;
		}
	}
	return 0;
}

DWORD CPlayerObj::GetVisitNpcId() {
	FUNCTION_BEGIN;
	if (m_pVisitNPC)
	{
		return m_pVisitNPC->m_dwScriptId;
	}
	return 0;
}
CCreature* CPlayerObj::GetVisitNpc() {
	FUNCTION_BEGIN;
	if (m_pVisitNPC)
	{
		return m_pVisitNPC;
	}
	return 0;
}

const char* CPlayerObj::GetLuaItemName(DWORD itembaseid) {
	FUNCTION_BEGIN;
	if (auto pstBase = sJsonConfig.GetItemDataById(itembaseid)) {
		return (const char*)&(pstBase->szName);
	}
	return NULL;
}

int CPlayerObj::CheckLattice(DWORD dwBaseid, DWORD num) {
	FUNCTION_BEGIN;
	if (!dwBaseid) { return 0; }
	if (auto itembase = sJsonConfig.GetItemDataById(dwBaseid)) {
		DWORD dwMaxCount = CItem::GetMaxDbCount(itembase->nID);
		if (itembase->nVariableMaxCount > 0) {
			bool IsNeedChangeMaxCnt = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("itemidchangemaxcnt", true, this);
			if (IsNeedChangeMaxCnt) {
				dwMaxCount = itembase->nVariableMaxCount;
			}
		}
		DWORD dwItems = 1;
		if (num > dwMaxCount)
		{
			dwItems = ((num % dwMaxCount) == 0) ? (num / dwMaxCount) : (num / dwMaxCount + 1);
		}
		if (m_Packet.GetFreeBagCellCount(itembase->GetBagType()) >= dwItems)
		{
			return m_Packet.GetFreeBagCellCount(itembase->GetBagType());
		}
		else { return 0; }
	}
	return 0;
}

CItem* CPlayerObj::GetCurrItem(const char* luatmpid) {
	FUNCTION_BEGIN;
	int64_t tmpid = 0;
	if (luatmpid && luatmpid[0] != 0) {
		tmpid = CItem::strToi642(luatmpid);
		//sscanf(luatmpid,"%I64d",&tmpid);
		if (tmpid != 0) {
			return m_Packet.FindItemInBag(tmpid);
		}
	}
	else {
		if (m_pCurrItem) { return m_pCurrItem; }
		if (m_i64CurrItemTmpId != 0) {
			return m_Packet.FindItemInBag(m_i64CurrItemTmpId);
		}
	}
	return NULL;
}

CItem* CPlayerObj::GetEquipItem(const char* luatmpid) {
	FUNCTION_BEGIN;
	int64_t tmpid = 0;
	if (luatmpid && luatmpid[0] != 0) {
		tmpid = CItem::strToi642(luatmpid);
		//sscanf(luatmpid,"%I64d",&tmpid);
		if (tmpid != 0) {
			return m_Packet.FindItemInBody(tmpid);
		}
	}
	return NULL;
}

CItem* CPlayerObj::CreateLuaItem(DWORD dwBaseid, DWORD num, int frommapid, const char* bornfrom, const char* szmaker) {
	FUNCTION_BEGIN;
	if (!dwBaseid) { return NULL; }
	return CItem::CreateItem(dwBaseid, _CREATE_MON_DROP, num, 0, __FUNC_LINE__, frommapid, bornfrom, szmaker);
}

int CPlayerObj::SendToBag(CItem* item, bool boLog, const char* pszLogstr) {
	FUNCTION_BEGIN;
	if (item)
	{
		if (m_Packet.AddItemToBag(item, false, false, true, (char*)pszLogstr))//解决脚本添加的物品不会触发得到物品事件，将boQuestItem改为false，原为true
		{
			return 1;
		}
	}

	CUserEngine::getMe().ReleasePItem(item, __FUNC_LINE__);
	return 0;
}

int CPlayerObj::SendToBagNoDel(CItem* item, bool boLog, const char* pszLogstr) {
	FUNCTION_BEGIN;
	if (item)
	{
		if (m_Packet.AddItemToBag(item, false, false, true, (char*)pszLogstr))//解决脚本添加的物品不会触发得到物品事件，将boQuestItem改为false，原为true
		{
			return 1;
		}
	}
	return 0;
}

void CPlayerObj::GetGroupPlayer(const char* szFunc) {
	FUNCTION_BEGIN;
	if (m_GroupInfo.dwGroupId) {
		stGlobalGetGroupPlayer retcmd;
		retcmd.i64OnlyId = m_i64UserOnlyID;
		strcpy_s(retcmd.szRetFunc, sizeof(retcmd.szRetFunc) - 1, szFunc);
		CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd, sizeof(stGlobalGetGroupPlayer));
	}
}

CItem* CPlayerObj::GetPosEquip(int pos) {
	FUNCTION_BEGIN;
	if (pos >= 0 && pos < EQUIP_MAX_COUNT) {
		return m_Packet.m_stEquip[pos];
	}
	return NULL;
}

void CPlayerObj::SendItem(CItem* pItem) {
	FUNCTION_BEGIN;
	if (pItem) {
		switch (pItem->m_Item.Location.btLocation)
		{
		case	ITEMCELLTYPE_EQUIP:			// 装备
		{
			m_Packet.SendEquipAddAndUpdateItem(pItem);
		}break;
		case	ITEMCELLTYPE_STORE:			// 仓库
		{
			m_Packet.SendStorageAddAndUpdateItem(pItem);
		}break;
		case	ITEMCELLTYPE_PACKAGE:		// 包裹的格子
		{
			m_Packet.SendBagAddAndUpdateItem(pItem);
		}break;
		case    ITEMCELLTYPE_TMPPACKAGE:
		{
			m_Packet.SendTmpBagAddAndUpdateItem(pItem);
		}break;
		}
		auto itemBase = pItem->GetItemDataBase();
		if (itemBase && itemBase->dwScriptid > 0) {//防止被回收的物品
			TriggerEvent(this, ITEMGET, pItem->GetItemBaseID());
		}
	}
}

bool CPlayerObj::SetVisitNpc(CCreature* pNpc) {
	FUNCTION_BEGIN;
	if (pNpc && pNpc->isNpc()) {
		m_pVisitNPC = pNpc;
		return true;
	}
	else {
		m_pVisitNPC = NULL;
		return false;
	}
}
bool CPlayerObj::WearItem(CItem* pItem, int nPos) {
	FUNCTION_BEGIN;
	if (pItem) {
		bool fanhui = m_Packet.ServerGetWearItem(pItem->GetItemID(), nPos, false, pItem);
		return fanhui;
	}
	return false;
}

bool CPlayerObj::TakeOffItem(CItem* pItem, int nPos) {
	FUNCTION_BEGIN;
	if (pItem) {
		stCretProcessingItem dstcmd;
		dstcmd.srcLocation.btIndex = nPos;
		dstcmd.nErrorCode = m_Packet.ServerGetTakeOffItem(pItem->GetItemID(), dstcmd.srcLocation, dstcmd.destLocation, true);
		return true;
	}
	return false;
}

bool CPlayerObj::ChangeName(const char* szNewName, const char* szOptMode) {
	FUNCTION_BEGIN;
	//m_boCanChangeName
	if (szNewName[0] != 0 && (strlen(szNewName) < _MAX_PLAYERNAMELENLIMIT_)
		&& (strnicmp(szNewName, _LOCAL_SVR_GMNAME_, sizeof(_LOCAL_SVR_GMNAME_) - 1) != 0)
		&& checknamestr(szNewName, strlen(szNewName), _NAMESTR_SAFELIMIT_, sizeof(_NAMESTR_SAFELIMIT_) - 1)
		&& checksqlstr(szNewName, _SQLSTR_SAFELIMIT_, sizeof(_SQLSTR_SAFELIMIT_) - 1) && m_pGateUser) {
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("ChangeNameOptModeSet", this, szOptMode);
		stPlayerChangeNameCmd changecmd;
		changecmd.svr_id_type_value = GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value;
		changecmd.i64onlyid = m_i64UserOnlyID;
		changecmd.dwTrueZoneId = GameService::getMe().m_nTrueZoneid;
		int oldlen = strlen(m_szOldName);
		int newlen = strlen(szNewName);
		if (oldlen + newlen > _MAX_OLDNAME_) {
			std::vector<std::string> result = StringSplit(m_szOldName, "+");
			int isize = result.size();
			int maxsize = min(isize, 10) - 1;
			sprintf_s(m_szOldName, sizeof(m_szOldName) - 1, "%s", result[isize - 1].c_str());
			if (maxsize > 1) {
				for (int i = 2; i <= maxsize; i++) {
					sprintf_s(m_szOldName, sizeof(m_szOldName) - 1, "%s+%s", result[isize - i].c_str(), m_szOldName);
				}
			}
		}
		strcpy_s(changecmd.szAccount, sizeof(changecmd.szAccount) - 1, m_pGateUser->m_szAccount);
		strcpy_s(changecmd.szOldName, sizeof(changecmd.szOldName) - 1, getName());
		strcpy_s(changecmd.szNewName, sizeof(changecmd.szNewName) - 1, szNewName);
		m_pGateUser->m_OwnerLoginSvr->sendcmd(&changecmd, sizeof(changecmd));
		return true;
	}
	this->sendTipMsg(this, GameService::getMe().GetStrRes(4, "other"));
	return false;
}

bool CPlayerObj::ChangeNameFinal(const char* szOldName) {
	FUNCTION_BEGIN;
	stPlayerChangeNameCmd changecmd;
	changecmd.svr_id_type_value = GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value;
	changecmd.i64onlyid = m_i64UserOnlyID;
	changecmd.btOptType = 1;
	changecmd.dwTrueZoneId = GameService::getMe().m_nTrueZoneid;
	strcpy_s(changecmd.szAccount, sizeof(changecmd.szAccount) - 1, m_pGateUser->m_szAccount);
	strcpy_s(changecmd.szOldName, sizeof(changecmd.szOldName) - 1, szOldName);
	strcpy_s(changecmd.szNewName, sizeof(changecmd.szNewName) - 1, getName());
	m_pGateUser->m_OwnerLoginSvr->sendcmd(&changecmd, sizeof(changecmd));
	return true;
}

void CPlayerObj::AddRelation(int nType, const char* szName) {
	FUNCTION_BEGIN;
	if (nType >= LIST_FRIEND && nType < LIST_ALL) {
		stRelationAdd RelationAddCmd;
		RelationAddCmd.btType = (emListType)nType;
		strcpy_s(RelationAddCmd.szName, _MAX_NAME_LEN_ - 1, szName);
		SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv, m_i64UserOnlyID, &RelationAddCmd, sizeof(RelationAddCmd));
	}
}

void CPlayerObj::LuaSetSecondPass(const char* szPass) {
	FUNCTION_BEGIN;
	if (szPass) {
		strcpy_s(m_szSecondPassWord, _MAX_SECONDPASS_LEN_ - 1, szPass);
	}
}

void CPlayerObj::LuaSetSecondMailAddress(const char* szMail) {
	FUNCTION_BEGIN;
	if (szMail) {
		strcpy_s(m_szSecondMailAddress, _MAX_SECONDPASS_LEN_ - 1, szMail);
	}
}

bool CPlayerObj::GetSecondOK() {
	FUNCTION_BEGIN;
	return m_boSecondPassOk;
}

void CPlayerObj::SetSecondOK(bool boSet) {
	FUNCTION_BEGIN;
	m_boSecondPassOk = boSet;
	if (!m_boSecondPassOk) {
		g_logger.debug("服务器踢人 %s:%s:%I64d,踢人代码 二级密码不对", m_pGateUser->m_szAccount, getName(), m_i64UserOnlyID);
		m_btKickState = KICK_SECONDPASSWORDERROR;
		m_dwDoKickTick = GetTickCount64() + 1000;
	}
}
void CPlayerObj::KickPlayer(BYTE cmd, BYTE subcmd) {
	FUNCTION_BEGIN;
	if (cmd == 1 && subcmd == 7) {
		return;
	}
	g_logger.debug("服务器踢人 %s:%s:%I64d,踢人代码 发包不对,大包号%d,小包号%d", m_pGateUser->m_szAccount, getName(), m_i64UserOnlyID, cmd, subcmd);
	m_btKickState = KICK_SECONDPASSWORDERROR;
	m_dwDoKickTick = GetTickCount64() + 1000;
}
void CPlayerObj::SendItemToClient(CItem* pItem) {
	FUNCTION_BEGIN;
	if (pItem) {
		stCretUpdateItem bagitem;
		bagitem.btPosition = ITEMCELLTYPE_VIRTUAL;
		bagitem.item = pItem->m_Item;
		SendMsgToMe(&bagitem, sizeof(bagitem));
	}
}

DWORD CPlayerObj::GetFreeCellCount() {
	FUNCTION_BEGIN;
	return m_Packet.GetFreeBagCellCount(1);
}

bool CPlayerObj::CheckVisitNpc() {
	FUNCTION_BEGIN;
	if (!m_pVisitNPC || !m_pVisitNPC->isNpc() || !m_pVisitNPC->isCanVisit() || !isInViewRange(m_pVisitNPC))
	{
		m_pVisitNPC = NULL;
		return false;
	}
	return true;
}

bool CPlayerObj::GuildWarKill(DWORD dwGuildId, int nKill, int nDie) {
	FUNCTION_BEGIN;
	if (m_GuildInfo.dwGuildId) {
	}
	return false;
}

bool CPlayerObj::GuildWarCheck(CPlayerObj* pTarget) {
	FUNCTION_BEGIN;
	if (pTarget && pTarget->isGuildWar()) {
		if (isGuildWar()) {
			if (pTarget->m_GuildInfo.dwGuildId != m_GuildInfo.dwGuildId) {
				std::set<DWORD>::iterator it = m_WarGuildSet.find(pTarget->m_GuildInfo.dwGuildId);
				if (it != m_WarGuildSet.end()) {
					return true;
				}
			}
		}
	}
	return false;
}

bool CPlayerObj::GuildAllianceCheck(CPlayerObj* pTarget) {
	FUNCTION_BEGIN;
	if (pTarget && m_AllianceGuildSet.size() && pTarget->m_AllianceGuildSet.size()) {
		if (pTarget->m_GuildInfo.dwGuildId != m_GuildInfo.dwGuildId) {
			std::set<DWORD>::iterator it = m_AllianceGuildSet.find(pTarget->m_GuildInfo.dwGuildId);
			if (it != m_AllianceGuildSet.end()) {
				return true;
			}
		}
	}
	return false;
}

void CPlayerObj::ChangeSex(int nSex) {
	switch (nSex)
	{
	case MAN_SEX:
	{
		m_siFeature.sex = MAN_SEX;
	}break;
	case WOMAN_SEX:
	{
		m_siFeature.sex = WOMAN_SEX;
	}break;
	}
	GetBaseProperty();
	SetAbilityFlag();
	UpdateAppearance(FeatureIndex::sex, m_siFeature.sex);
}

void CPlayerObj::ChangeJob(int nJob) {
	switch (nJob)
	{
	case MAGE_JOB:
	{
		m_siFeature.job = MAGE_JOB;
	}break;
	}
	GetBaseProperty();
	SetAbilityFlag();
	UpdateAppearance(FeatureIndex::job, m_siFeature.job);
}

int CPlayerObj::GetRelationSize(BYTE btRelationType) {
	FUNCTION_BEGIN;
	switch (btRelationType)
	{
	case 	LIST_FRIEND://好友列表
	{
		return m_xFriendList.size();
	}break;
	case	LIST_BLOCK:	//黑名单
	{
		return m_xBlockList.size();
	}break;
	case	LIST_ENEMY:	//仇人列表
	{
		return m_xEnemyList.size();
	}break;
	case	LIST_ALL:	//所有列表
	{
		return m_xFriendList.size() + m_xBlockList.size() + m_xEnemyList.size();
	}break;
	default:
	{
		return 0;
	}break;
	}
}

void CPlayerObj::KillMonster(CMonster* pMonster) {
	FUNCTION_BEGIN;
	if (pMonster) {
		m_curAttTarget = pMonster;
		if (pMonster->GetMonsterDataBase()->MonAiID) {
			TriggerEvent(this, NPCKILL, pMonster->GetMonsterDataBase()->MonAiID);
		}
		m_curAttTarget = NULL;
	}
}



void CPlayerObj::StudySkill(DWORD dwSkillid, BYTE btLevel, bool bCost) {
	FUNCTION_BEGIN;
	stMagic* pmagic = m_cMagic.findskill(dwSkillid);
	if (pmagic && pmagic->savedata.originLv == btLevel) {
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("studyskill", this, dwSkillid, btLevel);
		return;	//已经学会
	}
	auto pmagicdata = sJsonConfig.GetMagicDataBase(dwSkillid, btLevel);
	bool bEnough = false;
	if (pmagicdata) {
		if (m_dwLevel >= pmagicdata->wSkillNeedLv) {
			if (bCost) {
				if (m_res[ResID::game] >= pmagicdata->dwNeedGameCcy) {
					if (m_res[ResID::battleExp] >= pmagicdata->dwNeedBattleExp) {
						bEnough = true;
					}
					else {
						sendTipMsg(this, vformat("`6`战斗经验不足%d", pmagicdata->dwNeedBattleExp));
					}
				}
				else {
					sendTipMsg(this, vformat("`6`决战币不足%d", pmagicdata->dwNeedGameCcy));
				}
			}
			else
				bEnough = true;
		}
		else {
			sendTipMsg(this, vformat("`6`等级不足%d", pmagicdata->wSkillNeedLv));
		}
	}
	if (bEnough) {
		if (bCost)
		{
			ResChange(ResID::game, -(int)pmagicdata->dwNeedGameCcy, vformat("学习技能_%d_%d", dwSkillid, btLevel));
			ResChange(ResID::battleExp, -(int)pmagicdata->dwNeedBattleExp, vformat("学习技能_%d_%d", dwSkillid, btLevel));
		}
		stMagic* pMagic = m_cMagic.addskill(dwSkillid, btLevel);
		if (pMagic) {
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("studyskill", this, dwSkillid, btLevel);
			m_cMagic.SendCretSkill(&pMagic->savedata);
		}
	}
}

bool CPlayerObj::DeleteSkill(DWORD dwSkillid) {
	FUNCTION_BEGIN;
	if (m_cMagic.removeskill(dwSkillid)) {
		stSkillDeleteCmd skilldeletecmd;
		skilldeletecmd.dwMagicId = dwSkillid;
		SendMsgToMe(&skilldeletecmd, sizeof(skilldeletecmd));
		int nMax = 0;
		stShortCuts* pShortCuts = m_ShortCut.FindShortCuts(dwSkillid);
		while (pShortCuts && nMax < 5 * MAX_ROW) {
			stDelShortCuts retcmd;
			retcmd.ErrorCode = SETSHORTCUTS_SUCCESS;
			retcmd.shortcuts = *(pShortCuts);
			if (m_ShortCut.DeleteShortCuts(pShortCuts->btRow, pShortCuts->btCol)) {
				SendMsgToMe(&retcmd, sizeof(retcmd));
				break;
			}
			pShortCuts = m_ShortCut.FindShortCuts(dwSkillid);
			nMax++;
		}
		return true;
	}
	return false;
}


bool CPlayerObj::SaveClientSet(char* dest, DWORD& retlen) {
	FUNCTION_BEGIN;
	int maxsize = retlen;
	retlen = 0;
	if (maxsize < (sizeof(int))) { return false; }

	int count = 0;
	int len = sizeof(count);

	if (m_vClientSet.size()) {
		memcpy(&dest[len], &m_vClientSet[0], m_vClientSet.size());
	}
	len = len + m_vClientSet.size();
	count++;

	*((int*)(dest)) = count;
	retlen = ROUNDNUMALL(len, 3) / 3 * 4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin = ptlsbuf;
	ZeroMemory(pin, retlen);
	base64_encode(dest, len, pin, retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CPlayerObj::LoadClientSet(const char* dest, int retlen, int nver) {
	FUNCTION_BEGIN;
	int maxsize = retlen;
	if (maxsize == 0) { return true; }

	retlen = ROUNDNUMALL(retlen, 4) / 4 * 3;
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin = ptlsbuf;
	ZeroMemory(pin, retlen);
	base64_decode((char*)dest, maxsize, pin, retlen);
	memcpy((char*)dest, pin, retlen);
	maxsize = retlen;

	if (maxsize < (sizeof(int))) { return false; }

	int count = *((int*)(dest));
	int len = sizeof(count);

	while (count > 0) {
		m_vClientSet.clear();
		char szupdatebuffer[2][1024 * 4];
		szupdatebuffer[0][0] = 0;
		szupdatebuffer[1][0] = 0;
		int nowbuffer = 0;
#define		_UPDATENOWBUFFER_		nowbuffer=((nowbuffer==0)?1:0);
		char* poldbuffer = (char*)&dest[len];
		bool bomoveloadpos = false;
		bool boAddabi = true;

		if (!bomoveloadpos) { bomoveloadpos = true; len = len; }

		if (boAddabi) {
			if (retlen - len >= 0) {
				m_vClientSet.resize(retlen - len);
				if (m_vClientSet.size()) {
					memcpy(&m_vClientSet[0], poldbuffer, retlen - len);
				}
			}
		}
		count--;
	}
	return true;
}

void setRelationTable(sol::table& table, int tabidx, int64_t i64value1, const char* value2) {
	FUNCTION_BEGIN;
	if (!table.valid()) { return; }
	table[tabidx] = table.create_with(
		"onlyid", (double)i64value1,
		"name", value2
	);
}

void CPlayerObj::PlayDeathDropped() {
	FUNCTION_BEGIN;
	if (GetEnvir()->isDieNoDropAll()) {
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("chenmo_deadth", this);
		return;
	}
	do
	{
		AILOCKT(CUserEngine::getMe().m_scriptsystem.m_LuaVM->m_luacalllock);
		lua_State* L = CUserEngine::getMe().m_scriptsystem.m_LuaVM->GetHandle(); //danger
		if (!L) return;
		bool bCanDrop = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("PlayDeathCanDrop", true, this);
		if (!bCanDrop) return;
		sol::state_view lua(L);
		sol::table table1 = lua.create_table();
		if (table1.valid()) {
			int size = 0;
			for (int i = 0; i < _MAX_BAG_COUNT; i++) {
				for (auto iter = m_Packet.m_BagPacket[i].m_PackItemList.begin(); iter != m_Packet.m_BagPacket[i].m_PackItemList.end(); ++iter) {
					CItem* pItem = iter->second;
					if (pItem) {
						if (pItem->GetType() == ITEM_TYPE_EQUIP) {
							if (pItem->GetEquipStation() >= EQUIP_WEAPONS && pItem->GetEquipStation() < EQUIP_MOUNT) {
								table1[++size] = pItem;
							}
						}
						else {
							table1[++size] = pItem;
						}
					}
				}
			}
		}
		sol::table table2 = lua.create_table();
		if (table2.valid() && !GetEnvir()->isDieNoDropEquip()) {
			int size = 0;
			for (int i = EQUIP_WEAPONS; i < EQUIP_MOUNT; i++) {
				CItem* pItem = m_Packet.m_stEquip[i];
				if (pItem) {
					table2[++size] = pItem;
				}
			}
		}
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayDeathDroppedItem", this, table1, table2);

	} while (false);



}

void CPlayerObj::LuaCallLuaByFuncName(const char* evtfunc) {
	FUNCTION_BEGIN;
	if (evtfunc && evtfunc[0] != 0 && CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
		stAutoSetScriptParam autoparam(this);
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(evtfunc);
	}
}

bool CPlayerObj::sendTipMsg(CPlayerObj* player, const char* pszMsg) {
	if (!pszMsg) {
		g_logger.error("CPlayerObj::sendTipMsg 传入pszMsg 参数为空 !!!");
		return false;
	}
	if (player) {
		BUFFER_CMD(stTipMsg, sendCmd, stBasePacket::MAX_PACKET_SIZE);
		sendCmd->nPosX = player->quest_vars_get_var_n("client_click_pos_x");
		sendCmd->nPosY = player->quest_vars_get_var_n("client_click_pos_y");
		player->quest_vars_set_var_n("client_click_pos_x", 0, false);
		player->quest_vars_set_var_n("client_click_pos_y", 0, false);
		sendCmd->szTipMsg.push_str(pszMsg);
		player->SendMsgToMe(sendCmd, sizeof(*sendCmd) + sendCmd->szTipMsg.getarraysize());
		return true;
	}
	return false;
}
bool CPlayerObj::LuaSendTipMsg(const char* pszMsg)
{
	if (pszMsg)
		return sendTipMsg(this, UTG(pszMsg));
	return false;
}

bool CPlayerObj::sendTipMsgByXml(CPlayerObj* player, const char* pszPat, ...) {
	FUNCTION_BEGIN;
	if (player) {
		char szSendMsg[1024] = { 0 };
		getMessage(szSendMsg, 1024, pszPat);

		return sendTipMsg(player, szSendMsg);
	}

	return false;
}

bool CPlayerObj::sendTipMsgByXml(int64_t i64OnlyId, const char* pszPat, ...)
{
	FUNCTION_BEGIN;
	CPlayerObj* player = CUserEngine::getMe().m_playerhash.FindByOnlyId(i64OnlyId);
	if (player) {
		char szSendMsg[1024] = { 0 };
		getMessage(szSendMsg, 1024, pszPat);

		return sendTipMsg(player, szSendMsg);
	}
	return false;
}

void CPlayerObj::sendLuaMsg(CPlayerObj* pDstPlayer, DWORD dwDataType, const char* szData) {
	FUNCTION_BEGIN;
	BUFFER_CMD(stQuestScriptData, sendcmd, stBasePacket::MAX_PACKET_SIZE);
	sendcmd->dwDataType = dwDataType;
	if (szData && strcmp(szData, "") != 0) {
		sendcmd->DataArr.push_str(UTG(szData));

		if (pDstPlayer) {
			pDstPlayer->SendMsgToMe(sendcmd, sizeof(*sendcmd) + sendcmd->DataArr.getarraysize());
		}
		else {
			CUserEngine::getMe().SendMsg2AllUser(sendcmd, sizeof(*sendcmd) + sendcmd->DataArr.getarraysize());
		}
	}
}

sol::table CPlayerObj::LuaGetMapOtherGroupMember(sol::this_state ts) {
	sol::state_view lua(ts);
	auto table = lua.create_table();
	if (table && m_GroupInfo.dwGroupId) {
		auto curMap = GetEnvir();
		if (!curMap)
			return table;
		std::unordered_set<CPlayerObj*> playerSet;
		int index = 1;
		if (curMap->GetGroupPlayer(m_GroupInfo.dwGroupId, playerSet)) {
			for (auto player : playerSet)
			{
				if (player && player != this) {
					table[index] = player;
					index++;
				}
			}
		}
	}
	return table;
}

void CPlayerObj::getMsgFromTxServer(const char* pszFunName, bool boForceRefresh) {
	GameService::getMe().Send2TencentSvrs(m_pGateUser->m_szAccount, pszFunName, m_pGateUser->szclientip, boForceRefresh);
}

void CPlayerObj::ChangePetProperty()
{
	FUNCTION_BEGIN;
	if (m_Petm.m_pSkeletonPet) {
		m_Petm.m_pSkeletonPet->SetPetProperty();
	}
	if (m_Petm.m_pTheAnimalPet) {
		m_Petm.m_pTheAnimalPet->SetPetProperty();
	}
	if (m_Petm.m_pTianBingPet) {
		m_Petm.m_pTianBingPet->SetPetProperty();
	}
	if (m_Petm.m_pMagicPet) {
		m_Petm.m_pMagicPet->SetPetProperty();
	}
	if (m_Petm.m_pYueLingPet) {
		m_Petm.m_pYueLingPet->SetPetProperty();
	}
}


DWORD CPlayerObj::getVipType() {
	FUNCTION_BEGIN;
	enum {
		VIP_MAXLEVEL = 0x00000001,	//等级第一
		VIP_WARRIOR = 0x00000002,	//战士第一
		VIP_MAGE = 0x00000004,	//法师第一
		VIP_MONK = 0x00000008,	//道士第一
		VIP_NULL1 = 0x00000010,	//留空
		VIP_NULL2 = 0x00000020,	//留空
		VIP_NULL3 = 0x00000040,	//留空
		VIP_TQ1 = 0x00000080,	//特权1级
		VIP_TQ2 = 0x00000100,
		VIP_TQ3 = 0x00000200,
		VIP_TQ4 = 0x00000400,
		VIP_TQ5 = 0x00000800,
		VIP_NULL12 = 0x00001000,
		VIP_NULL13 = 0x00002000,
		VIP_NULL14 = 0x00004000,
		VIP_NULL15 = 0x00008000,
		VIP_REN_ZHENG = 0x00010000,	//官方认证
		VIP_NULL21 = 0x00020000,
		VIP_NULL22 = 0x00040000,
		VIP_NULL23 = 0x00080000,
		VIP_NULL24 = 0x00100000,
		VIP_NULL25 = 0x00200000,
		VIP_NULL26 = 0x00400000,
		VIP_NULL27 = 0x00800000,
		VIP_NULL28 = 0x01000000,
		VIP_NULL29 = 0x02000000,
		VIP_NULL30 = 0x04000000,
		VIP_NULL31 = 0x08000000,
		VIP_NULL32 = 0x10000000,
		VIP_NULL33 = 0x20000000,
		VIP_NULL34 = 0x40000000,
		VIP_NULL35 = 0x80000000,
	};

	DWORD dwTpye = 0;
	if (CUserEngine::getMe().m_i64LevelMaxPlayer == m_i64UserOnlyID) {
		dwTpye |= VIP_MAXLEVEL;
	}

	static int VIP[] = { 0,VIP_TQ1,VIP_TQ2,VIP_TQ3,VIP_TQ4, VIP_TQ5 };
	if (m_nVipLevel > 0 && m_nVipLevel < count_of(VIP)) {
		dwTpye |= VIP[m_nVipLevel];
	}
	return dwTpye;
}

void CPlayerObj::LoopLimitItemPacket()
{
	FUNCTION_BEGIN;
	DWORD dwNowTime = (DWORD)time(NULL);
	for (std::set<CItem*, compare_LimitItem>::iterator it = set_LimitItemPacket.begin(); it != set_LimitItemPacket.end();)
	{
		CItem* pItem = *it;
		if (pItem->IfLimitedTime())//有到期物品
		{
			it++;
			if (pItem) {
				ItemDeleteInAll(pItem, -1, "m_LoopLimitItemPacket");
			}
		}
		else
		{
			++it;
			break;
		}
	}
}


bool CPlayerObj::AddItemLimitTime(CItem* pItem, DWORD dwLimitTime)//dwLimitTime 秒
{
	FUNCTION_BEGIN;
	if (pItem && dwLimitTime)
	{
		if (this->m_Packet.FindItemInBag(pItem->GetItemID()) || this->m_Packet.FindItemInBody(pItem->GetItemID()) || this->m_Packet.FindItemInStorage(pItem->GetItemID()))
		{
			pItem->m_Item.dwExpireTime = dwLimitTime;

			this->set_LimitItemPacket.erase(pItem);
			this->set_LimitItemPacket.insert(pItem);
			return true;
		}
	}
	return false;
}


void CPlayerObj::LuaSetPetState(emPetState emState) {
	FUNCTION_BEGIN;
	m_Petm.SetPetState(emState);
}

void CPlayerObj::LuaLoadAllPets(sol::table table) {
	FUNCTION_BEGIN;
	if (!table.valid()) { return; }
	m_Petm.LoadAllPets([&table](int idx, CPetObj* pPet) {
		table[idx] = pPet;
		});
}

char* CPlayerObj::GetCrossTradeAccount() {
	FUNCTION_BEGIN;
	return (char*)(vformat("%s+%d", getAccount(), GameService::getMe().m_nTradeid));

}

char* CPlayerObj::GetCrossTradeName() {
	FUNCTION_BEGIN;
	return (char*)(vformat("[%s]+%s", GameService::getMe().m_szTradeName, getName()));
}

BYTE CPlayerObj::getPlatFormType() {
	return 255;
}

int CPlayerObj::CalculatingRestoreHp() {
	FUNCTION_BEGIN;
	if (!IsFighting())
		return m_stAbility[AttrID::HpRestore];
	return 0;
}

int CPlayerObj::CalculatingRestoreMp() {
	FUNCTION_BEGIN;
	if (!IsFighting())
		return m_stAbility[AttrID::MpRestore];
	return 0;
}

int CPlayerObj::CalculatingRestorePp() {
	FUNCTION_BEGIN;
	int64_t curtime = time(nullptr);
	if (!m_isRun && curtime >= m_i64RunSec && !IsFighting())
		return m_stAbility[AttrID::PpRestore];
	m_isRun = false;
	return 0;
}

int CPlayerObj::getVipLevel()
{
	return m_nVipLevel;//CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("vip_get",0,this);
}

void CPlayerObj::addLuaEffid(DWORD dwEffid) {
	FUNCTION_BEGIN;
	if (dwEffid > 0) {
		m_vLuaEffid.push_back(dwEffid);
	}
}
void CPlayerObj::addSpecialLuaEffid(DWORD dwEffid) {
	FUNCTION_BEGIN;
	if (dwEffid > 0) {
		m_vSpecialLuaEffid.push_back(dwEffid);
	}
}
void CPlayerObj::addCofLuaEffid(DWORD dwEffid, float cof) {
	FUNCTION_BEGIN;
	if (dwEffid > 0) {
		stLuaEffCof eff;
		eff.effectid = dwEffid;
		eff.cof = cof;
		m_vCofLuaEffid.push_back(eff);
	}
}


void CPlayerObj::calculateLuaAbility(stARpgAbility* abi) {
	FUNCTION_BEGIN;
	if (abi) {
		abi->Clear();
		CUserEngine::getMe().m_abilityKeys.forEach([&](const std::string& key)
			{
				DWORD effectId = quest_vars_get_var_n(key);
				if (effectId == 0) return;
				auto effect = sJsonConfig.GetEffectDataById(effectId);
				if (!effect) return;
				abi->Add(*effect);
			});
	}
}

void CPlayerObj::calculateLuaTimeAbility(stARpgAbility* abi) {
	FUNCTION_BEGIN;
	if (abi) {
		abi->Clear();
		CUserEngine::getMe().m_abilityTimeKeys.forEach([&](const std::string& key)
			{
				DWORD effectId = quest_vars_get_var_n(key);
				if (effectId == 0) return;
				auto effect = sJsonConfig.GetEffectDataById(effectId);
				if (!effect) return;
				abi->Add(*effect);
			});
	}
}

void CPlayerObj::calculateAttrPointAbility(stARpgAbility* abi)
{
	if (abi) {
		abi->Clear();
		for (size_t i = 1; i <= 5; i++) {
			switch (i)
			{
			case 1:
				abi->attrs[tosizet(AttrID::Strength)] = quest_vars_get_var_n("ap1");
			case 2:
				abi->attrs[tosizet(AttrID::Physique)] = quest_vars_get_var_n("ap2");
			case 3:
				abi->attrs[tosizet(AttrID::Agility)] = quest_vars_get_var_n("ap3");
			case 4:
				abi->attrs[tosizet(AttrID::Wisdom)] = quest_vars_get_var_n("ap4");
			case 5:
				abi->attrs[tosizet(AttrID::Intelligence)] = quest_vars_get_var_n("ap5");
			default:
				break;
			}
		}
	}
}

// 一级属性转化二级属性
void CPlayerObj::FirstConvertAbility(stARpgAbility* abi, stARpgAbility* converabi)
{
	if (abi && converabi) {
		converabi->Clear();
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("AttrConversion", false, this,abi,converabi);
	}
}

void CPlayerObj::ChangePkMode(DWORD dwPkMode, bool boServer/* = false*/)
{
	bool boCanChange = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("CanChangePkMode", true, this);//调用脚本函数返回玩家在地图里面是否能更改PK模式
	if (!boCanChange) {
		return;
	}
	stCretPkModel stDstCmd;
	bool boSend = false;
	if (boServer) {
		stDstCmd.btPkModel = dwPkMode;
		this->m_btPkModel = dwPkMode;
		boSend = true;
	}
	else if (dwPkMode >= PKMODEL_PEACEMODE && dwPkMode <= PKMODEL_ALLTHEMODE) {
		if (dwPkMode == PKMODEL_TEAMMODE && m_GroupInfo.dwGroupId <= 0) {
			sendTipMsg(this, GameService::getMe().GetStrRes(1, "PK"));
			return;
		}
		else if (dwPkMode == PKMODEL_GUILDMODE && m_GuildInfo.dwGuildId <= 0) {
			sendTipMsg(this, GameService::getMe().GetStrRes(2, "PK"));
			return;
		}
		else if (dwPkMode == PKMODEL_GUILDWARMODE && m_GuildInfo.dwGuildId <= 0) {
			sendTipMsg(this, GameService::getMe().GetStrRes(3, "PK"));
			return;
		}
		stDstCmd.btPkModel = dwPkMode;
		this->m_btPkModel = dwPkMode;
		boSend = true;
	}
	if (boSend) {
		int type = (dwPkMode == 0) ? 7 : dwPkMode;
		CChat::sendSystem(this, GameService::getMe().GetStrRes(type, "pkmodel"));
		SendMsgToMe(&stDstCmd, sizeof(stCretPkModel));
	}
}


BYTE CPlayerObj::GetPkMode() {
	return m_btPkModel;
}

void CPlayerObj::UpdatePlayerInfo() {
	FUNCTION_BEGIN;
	stUpdatePlayerInfo retcmd;
	retcmd.btGmLv = m_btGmLvl;
	retcmd.btPlayerZSLvl = 0;
	SendMsgToMe(&retcmd, sizeof(retcmd));
}

void CPlayerObj::SendSpecialRingSkillCd(stMagic* pMagic) {
	FUNCTION_BEGIN;
	if (pMagic) {
		stSkillCDTime cdtimecmd;
		cdtimecmd.dwMagicId = pMagic->savedata.skillid;
		cdtimecmd.dwPublicTick = 0;
		cdtimecmd.dwSelfTick = pMagic->getleftcd(this);
		SendMsgToMe(&cdtimecmd, sizeof(cdtimecmd));
	}
}

int CPlayerObj::CheckPosCollide(int ndx, int ndy, int& count) //检测当前格子是否能被野蛮冲撞推走
{
	int nRet = 1;
	count = 0;
	auto vCret = GetEnvir()->GetCretInRangeAll(ndx, ndy, 0);
	for (auto it = vCret.begin(); it != vCret.end(); ++it) {
		CCreature* pCret = *it;
		int ntype = CheckPlayerCollide(pCret);
		if (ntype == 1)
		{//可穿过
		}
		else if (ntype == 2)
		{//可以被冲撞
			nRet = 2;
			count++;
		}
		else if (ntype == 3)
		{//3：冲撞不动会被反弹
			nRet = 3;
			break;
		}
	}
	return nRet;
}

int  CPlayerObj::CheckPlayerCollide(CCreature* pCret) //检测CCreature* pCret是否能被野蛮冲撞推走,1：可以穿过，2：可以被冲撞，3：冲撞不动会被反弹
{
	if (pCret && !pCret->isNpc() && isEnemy(pCret) && !isSafeZone(pCret) && !pCret->m_boNoByPush)
	{
		bool boSuccess = false;

		if (pCret->isPlayer()) {
			if ( m_dwLevel >= pCret->m_dwLevel ) {
				boSuccess = true;
			}
		}
		else if (pCret->isPet()) {
			if (pCret->toPet()->isPlayerMaster()) {
				if ( m_dwLevel >= pCret->m_dwLevel ) {
					boSuccess = true;
				}
			}
		}
		else {
			if (m_dwLevel >= pCret->m_dwLevel) {
				boSuccess = true;
			}
		}
		if (boSuccess)
		{
			return 2;
		}
		return 3;
	}
	if (pCret && pCret->isMonster() && pCret->m_boNoByPush)
	{
		return 3;
	}
	return 1;
}

stItem* CPlayerObj::GetMailedItem(DWORD id, DWORD num, BYTE binding)
{
	FUNCTION_BEGIN;
	stItem* pItem = NULL;
	auto pItemLoadBase = sJsonConfig.GetItemDataById(id);
	if (pItemLoadBase == NULL)
	{
		return NULL;
	}

	if (num > max(pItemLoadBase->dwMaxCount, pItemLoadBase->nVariableMaxCount))
	{
		return NULL;
	}

	pItem = CUserEngine::getMe().GetMailedItem();
	if (pItem) {
		pItem->dwBaseID = id;
		pItem->dwBinding = binding;
		pItem->btBornFrom = _CREATE_MAIL;
		pItem->dwCount = num;
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

float CPlayerObj::GetExpRate(int nGroupNum)
{//获取玩家经验倍率
	float nExpCof = (
		((m_dwDoubleTime >= (DWORD)time(NULL)) ? m_dwDoubleRate : 0)
		+ m_dwJyRate / 10000.0f
		+ ((nGroupNum > 0) ? (nGroupNum * 0.1f) : 0)
		+ (CUserEngine::getMe().m_scriptsystem.m_LuaVM ? CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<float>("AdditionalExpRate", 0, this) : 0)
		);
	return nExpCof;
}

void CPlayerObj::DropTest(DWORD dwNum, DWORD dwMonsterId) {
	if (m_btGmLvl <= 0) {
		g_logger.warn("DropTest: 权限不足");
		return;
	}
	auto pMonsterData = sJsonConfig.GetDropDataBase(dwMonsterId);
	if (!pMonsterData) {
		g_logger.error("DropTest: 无效的怪物ID %u", dwMonsterId);
		return;
	}
	if (dwNum >= 10000) {
		g_logger.warn("DropTest: 测试数量过大 %u，限制为9999", dwNum);
		dwNum = 9999;
	}
	std::vector<CItem*> vTotalDropItems;
	std::vector<CItem*> vDropItems;
	for (DWORD i = 0; i < dwNum; i++) {
		vDropItems.clear();
		CMonster::SelectDropItem(pMonsterData, this, vDropItems);
		vTotalDropItems.insert(vTotalDropItems.end(),
			vDropItems.begin(),
			vDropItems.end());
	}
	for (auto& pItem:vTotalDropItems)
	{
		if (!m_Packet.AddItemToBag(pItem, true, false, true, "掉落测试")) {
			CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
			g_logger.error("掉落测试添加物品失败");
			return;
		}
	}
}

void CPlayerObj::SendWeChatInfo(int ptid) {
	FUNCTION_BEGIN;
	stOpenApiWeiXinAccountLog retcmd;
	retcmd.channelId = ptid;
	strcpy_s(retcmd.name, _MAX_NAME_LEN_ - 1, getName());
	strcpy_s(retcmd.account, _MAX_NAME_LEN_ - 1, m_pGateUser->m_szAccount);
	strcpy_s(retcmd.wxaccount, _MAX_NAME_LEN_ - 1, quest_vars_get_var_s("WXFL_WXInfo"));
	CUserEngine::getMe().SendMsg2TencentApiSvr(&retcmd, sizeof(stOpenApiWeiXinAccountLog));
}

void CPlayerObj::SendGroupChangeInfo(BYTE ntype, int guildid, const char* guildname) {
	FUNCTION_BEGIN;
	stOpenApiGroupChangeLog retcmd;
	sprintf_s(retcmd.guild_id, sizeof(retcmd.guild_id), "%d", guildid);
	strcpy_s(retcmd.guild_name, _MAX_NAME_LEN_ - 1, guildname);
	retcmd.server_id = GameService::getMe().m_nTrueZoneid;
	sprintf_s(retcmd.server_name, sizeof(retcmd.server_name), "%d区", GameService::getMe().m_nTrueZoneid);
	sprintf_s(retcmd.role_id, sizeof(retcmd.role_id), "%I64d", m_i64UserOnlyID);
	strcpy_s(retcmd.role_name, _MAX_NAME_LEN_ - 1, getName());
	strcpy_s(retcmd.account, _MAX_NAME_LEN_ - 1, m_pGateUser->m_szAccount);
	retcmd.type = ntype;
	CUserEngine::getMe().SendMsg2TencentApiSvr(&retcmd, sizeof(stOpenApiGroupChangeLog));
}

void CPlayerObj::PlayerUnderBattle() {
	m_i64BattleSec = time(NULL) + 10;   //10秒后脱离战斗状态
}

void CPlayerObj::PlayerUnderPk() {
	m_i64PkSec = time(NULL) + 10;
}

void CPlayerObj::LuaSendBuffState2Client(DWORD dwBuffID, BYTE btLevel) {
	stCretBuffState retcmd;
	retcmd.dwMagicID = dwBuffID;
	retcmd.btBuffOrAction = 2;
	retcmd.btLevel = btLevel ? btLevel : 0;
	SendMsgToMe(&retcmd, sizeof(stCretBuffState));
}

void CPlayerObj::LuaSendBuffAllState2Client(DWORD dwBuffID, BYTE btState, int nTime) {
	stCretBuffState retcmd;
	retcmd.dwMagicID = dwBuffID;
	retcmd.btBuffOrAction = btState;
	retcmd.btLevel = 1;
	retcmd.dwTick = nTime / 1000;
	SendMsgToMe(&retcmd, sizeof(stCretBuffState));
}

void CPlayerObj::SetReadyState(ReadyState state, const char* log)
{
	g_logger.debug("设置玩家状态%d->%d,%s", m_btReadyState, state, log);
	m_btReadyState = state;

}

TiShiManager::TiShiManager(CPlayerObj* owner)
{
	m_owner = owner;
}

void TiShiManager::AddCheckList(int id, std::string path, uint32_t delaytime) {
	auto outerIt = m_mapTips.find(id);
	if (outerIt != m_mapTips.end()) {
		// 若外层键存在，再检查内层键是否存在
		auto innerIt = outerIt->second.find(path);
		if (innerIt != outerIt->second.end()) {
			m_mapTips[id][path].updateTime = GetTickCount64() + delaytime;
			return;
		}
	}
	stTiShi tishi;
	tishi.updateTime = GetTickCount64() + delaytime;
	m_mapTips[id][path] = tishi;
}


void TiShiManager::AddMapChangeList(int id, std::string path) {
	auto outerIt = m_mapTips.find(id);
	if (outerIt != m_mapTips.end()) {
		// 若外层键存在，再检查内层键是否存在
		auto innerIt = outerIt->second.find(path);
		if (innerIt != outerIt->second.end()) {
			m_mapTips[id][path].mapchange = 1;
			return;
		}
	}
	stTiShi tishi;
	tishi.mapchange = 1;
	m_mapTips[id][path] = tishi;
}

void TiShiManager::Run() {
	FUNCTION_MONITOR(100, "TiShiManager::Run")
		auto tickCount = GetTickCount64();
	auto outIt_0 = m_mapTips.find(0);
	if (outIt_0 != m_mapTips.end()) {
		std::unordered_map<std::string, stTiShi>& inMap = outIt_0->second;
		auto& tishi = inMap[""];
		if (tishi.updateTime) {
			if (tickCount >= tishi.updateTime) {
				CALL_LUA("CallRefreshTiShi", m_owner, outIt_0->first, "", tishi);
				tishi.updateTime = 0;

				// 清除所有要刷新的页签
				auto it = m_mapTips.begin();
				while (it != m_mapTips.end()) {
					if (it->first != 0) {
						it = m_mapTips.erase(it);
					}
					else {
						++it;
					}
				}
			}
			return;
		}
	}

	for (auto& it : m_mapTips) {
		auto& innerMap = it.second;
		for (auto& iter : innerMap) {
			auto tishi = &iter.second;
			if (tishi->updateTime && tickCount >= tishi->updateTime) {
				CALL_LUA("CallRefreshTiShi", m_owner, it.first, iter.first, tishi);
				tishi->updateTime = 0;
			}
		}
	}

	for (auto it = m_mapCallBack.begin(); it != m_mapCallBack.end();)
	{
		auto& delaytime = it->second;
		if (delaytime >= 0 && tickCount >= delaytime)
		{
			stAutoSetScriptParam autoparam(m_owner);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(it->first.c_str());
			it = m_mapCallBack.erase(it);
		}
		else {
			it++;
		}
	}
}

void TiShiManager::MapChange() {
	for (auto& it : m_mapTips) {
		auto& innerMap = it.second;
		for (auto& iter : innerMap) {
			auto tishi = &iter.second;
			if (tishi->mapchange) {
				CALL_LUA("CallRefreshTiShi", m_owner, it.first, iter.first, tishi);
				tishi->mapchange = 0;
			}
		}
	}
}

void TiShiManager::AddLuaCallBack(std::string strFuncStr, int32_t delaytime) {
	//保证到最后一个检测完成后，再执行回调函数，以免红点数据异常（玩家连续点领取的时候，最后一个领取再发送面板红点）
	m_mapCallBack[strFuncStr] = GetTickCount64() + delaytime;
}

TuBiaoManager::TuBiaoManager(CPlayerObj* owner)
{
	m_owner = owner;
}

void TuBiaoManager::AddCheckList(uint16_t tubiao_id, uint32_t delaytime) {
	if (m_mapTuBiao.find(tubiao_id) == m_mapTuBiao.end()) {
		stTuBiao tubiao;
		tubiao.updateTime = GetTickCount64() + delaytime;
		m_mapTuBiao[tubiao_id] = tubiao;
	}
	else {
		m_mapTuBiao[tubiao_id].updateTime = GetTickCount64() + delaytime;
	}
}

void TuBiaoManager::Run() {
	auto tickCount = GetTickCount64();
	for (auto& it : m_mapTuBiao) {
		if (it.second.updateTime && tickCount >= it.second.updateTime) {
			CALL_LUA("RefreshTuBiao", m_owner);
			break;
		}
	}
	for (auto& it : m_mapTuBiao) {
		if (it.second.updateTime && tickCount >= it.second.updateTime) {
			it.second.updateTime = 0;
		}
	}
}

void TuBiaoManager::KeepStorage(uint16_t tubiao_id, uint8_t state) {
	auto iter = m_mapTuBiao.find(tubiao_id);
	if (iter != m_mapTuBiao.end()) {
		iter->second.state = state;
	}
}

uint8_t TuBiaoManager::GetShowState(uint16_t icon_id) {
	auto iter = m_mapTuBiao.find(icon_id);
	if (iter != m_mapTuBiao.end()) {
		return iter->second.state;
	}
	return 0;
}

bool TuBiaoManager::NeedUpdate(uint16_t icon_id) {
	auto tickCount = GetTickCount64();
	auto iter = m_mapTuBiao.find(icon_id);
	if (iter != m_mapTuBiao.end()) {
		return iter->second.updateTime && tickCount >= iter->second.updateTime;
	}
	return false;
}

bool CPlayerObj::GoldChanged(double nChanged, bool boAutiAddicted, const char* szopt)
{
	FUNCTION_BEGIN;
	if (nChanged < 0 && (double)m_dwGold < abs(nChanged)) return false;
	if (nChanged != 0)
	{
		stCretGoldChanged retcmd;
		if (m_dwGold + nChanged <= _MAX_CARRYGOLD_(m_dwLevel)) {
			m_dwGold += nChanged;
			retcmd.dwGold = m_dwGold;
			retcmd.nChanged = nChanged;
			retcmd.boMax = false;
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("GoldChange", this, nChanged, szopt);
		}
		else {
			g_logger.debug("玩家金币到达上限，玩家名【%s】,金币上限【%u】,当前金币【%u】,增加金币【%u】,日志【%s】", getName(), _MAX_CARRYGOLD_(m_dwLevel), m_dwGold, (DWORD)nChanged, szopt);
			retcmd.boMax = true;
		}
		SendMsgToMe(&retcmd, sizeof(retcmd));

		if (abs(nChanged) > 300) {
			GameService::getMe().Send2LogSvr(_SERVERLOG_GETGOLD_, 0, 0, this, "'%s','%s','%s',%I64d,%d,%d,'%s'",
				szopt ? szopt : "other",
				getAccount(),
				getName(),
				m_i64UserOnlyID,
				m_dwGold,
				(int)nChanged,
				GetEnvir()->getMapName());
		}
	}
	return true;
}

static long g_dwtransid = _random(1000) + 1000;
void CPlayerObj::SetRmbGoldLog(int nChanged, int nOld, const char* szEvtName)
{
	DWORD dwtransid = InterlockedIncrement(&g_dwtransid);
	std::string strBillNo = vformat("%d", dwtransid);
	BYTE btPayType = 0;
	if (this->m_btGmLvl > 0) {
		btPayType = 2;
	}
	DWORD dwafter = nOld + nChanged;
	int mainLand = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("GetSvrRMBMainland", 1);
	GameService::getMe().Send2LogSvr(_SERVERLOG_RMB_CHANGE, 0, 0, this,
		"'%s','%s','%s',%I64d,%d,'%s',%d,%d,%d,%d,%d,%d,%d",
		szEvtName,
		getAccount(),
		getName(),
		m_i64UserOnlyID,
		this->m_dwLevel,
		strBillNo.c_str(),
		nOld,
		nChanged,
		dwafter,
		0,
		btPayType,
		m_dwUseRmb,
		mainLand);
}

bool CPlayerObj::UpdateChargeNoChange(int nChanged, const char* szEvtName)					//铸币改变
{
	FUNCTION_BEGIN;
	BYTE btPayType = 1;
	if (nChanged < 0 && m_res[ResID::charge] < abs(nChanged))
		return false;
	if (this->m_btGmLvl > 0) {
		btPayType = 2;
	}
	std::string strBillNo;
	DWORD dwtransid = InterlockedIncrement(&g_dwtransid);
	strBillNo = vformat("%d", dwtransid);
	DWORD dwOldRmbGold = m_res[ResID::charge];
	m_res[ResID::charge] += nChanged;
	if (nChanged < 0) {
		m_dwUseRmb -= nChanged;
	}
	int mainLand = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("GetSvrRMBMainland", 1);

	//更新本地内存中数据
	g_logger.debug("能源晶石(rmb)改变'本地:%s-%s','%s','%s',%I64d,%d,%d,%d,%d", strBillNo.c_str(), szEvtName, getAccount(), getName(), m_i64UserOnlyID, dwOldRmbGold, nChanged, m_res[ResID::charge], btPayType);
	GameService::getMe().Send2LogSvr(_SERVERLOG_RMB_CHANGE, 0, 0, this,
		"'%s','%s','%s',%I64d,%d,'%s',%d,%d,%d,%d,%d,%d,%d",
		vformat("%s-%s", "local", szEvtName),
		getAccount(),
		getName(),
		m_i64UserOnlyID,
		this->m_dwLevel,
		strBillNo.c_str(),
		dwOldRmbGold,
		nChanged,
		m_res[ResID::charge],
		m_res[ResID::hischarge],
		btPayType,
		m_dwUseRmb,
		mainLand);

	stPlayerResChange retcmd;
	retcmd.btIndex = tosizet(ResID::charge);
	retcmd.nNow = m_res[ResID::charge];
	retcmd.nChange = 0;
	SendMsgToMe(&retcmd, sizeof(retcmd));
	return true;
}

bool CPlayerObj::ResChange(ResID rid, int nChange, const char* szEvtName) {							// 资源更改
	FUNCTION_BEGIN;
	if (szEvtName == nullptr || szEvtName[0] == 0 || nChange == 0)
		return false;
	if (!stRes::IsValidID(rid)) {
		return false;
	}
	int old = m_res[rid];
	int newVal = old + nChange;
	DWORD dwtransid = 0;
	switch (rid)
	{
	case ResID::charge:
		if (!UpdateCharge(nChange, szEvtName, dwtransid))
			return false;
		break;
	case ResID::chargebind:
	case ResID::game:
	case ResID::gamebind:
		if (nChange < 0 && newVal < 0)
			return false;
		break;
	case ResID::horus:
	case ResID::shellGold:
	case ResID::superShellGold:
	case ResID::ladderPoint:
	case ResID::daycharge:
	case ResID::hischarge:
	case ResID::buildDegree:
	case ResID::contribution:
	case ResID::battleExp:
	case ResID::citizenExp: 
		{
			m_res[rid] = newVal;
			int nowLv = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("GetNowCitizenLv", 0, this);
			if (m_res[ResID::citizenLv] != nowLv) {
				ResChange(ResID::citizenLv, nowLv - m_res[ResID::citizenLv], "市民等级");
			}
		}
		break;
	case ResID::crime:
	case ResID::energy:
	case ResID::fatigue:
	case ResID::vitality:
		break;
	case ResID::exp:
		newVal = max(0, newVal);
		if (!AddExp(newVal, szEvtName))
			return false;
		break;
	case ResID::pk:
		if (newVal >= 150) {
			ChangePkMode(PKMODEL_PEACEMODE, true);
		}
		break;
	case ResID::citizenLv:
		UpdateAppearance(FeatureIndex::nameColor, newVal);
		break;
	case ResID::attrPoint:
	case ResID::skillPoint:
	case ResID::superSkillPoint:
		//if (nChange < 0 && newVal < 0)
		//	return false;
		break;
	case ResID::neng:
		newVal = max(0, newVal);
		newVal = min(CUserEngine::getMe().m_nNengLimit, newVal);
		break;
	default:
		break;
	}
	if (old != newVal)
	{
		m_res[rid] = newVal;
		stPlayerResChange retcmd;
		retcmd.btIndex = static_cast<uint8_t>(rid);
		retcmd.nChange = newVal - old;
		retcmd.nNow = newVal;
		SendMsgToMe(&retcmd, sizeof(retcmd));
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerResChange", this, static_cast<uint8_t>(rid), retcmd.nChange);
		if (stRes::IsCcy(rid)) {
			SetCcyLog(rid, szEvtName, nChange, old);
		}
	}
	return true;
}

int CPlayerObj::DoPunctureMagic(BYTE damageType) {
	if (damageType != DAMAGETYPE_PHYSICAL || GetJobType() != SWORD_JOB) {
		return 0;
	}
	auto* pMagic = FindSkill(_MAGIC_Puncture_);
	auto pMagicDb = pMagic ? pMagic->GetMagicDataBase() : nullptr;
	if (!pMagicDb) {
		return 0;
	}
	int roll = _random(10000);
	if (roll > pMagicDb->nSuccessCof) {
		return 0;
	}
	int punctureRate = 0;
	if (auto pEffDb = pMagicDb->GetEffectDataBase()) {
		punctureRate = pEffDb->GetAttr(AttrID::PunctureRate);
	}
	if (sConfigMgr.IsAtkDebug()) {
		CChat::sendClient(toPlayer(), "玩家%s, 被动技能穿刺", getName());
		CChat::sendClient(this, "剑师,被动技能穿刺,随机数=%d,成功率=%d,触发穿刺率增加=%d", roll, pMagicDb->nSuccessCof, punctureRate);
	}
	return punctureRate;
}

int CPlayerObj::DoShanBiMagic() { //闪避技能
	if (GetJobType() != SWORD_JOB) {
		return 0;
	}
	auto* pMagic = FindSkill(_MAGIC_SHANBI_);
	auto pMagicDb = (pMagic ? pMagic->GetMagicDataBase() : nullptr);
	if (!pMagicDb || pMagicDb->btlevel == 0) {
		return 0;
	}
	int roll = _random(10000);
	if (roll > pMagicDb->nSuccessCof) {
		return 0;
	}
	int reducePerc = 0;
	if (auto pEffDb = pMagicDb->GetEffectDataBase()) {
		reducePerc = pEffDb->GetAttr(AttrID::DamageDecPer);
	}
	if (sConfigMgr.IsDefDebug()) {
		CChat::sendClient(toPlayer(), "玩家%s, 触发被动技能闪避", getName());
		CChat::sendClient(this, "随机数=%d,成功率=%d,触发减伤率=%d", roll, pMagicDb->nSuccessCof, reducePerc);
	}
	return reducePerc;
}

int CPlayerObj::DoExcitedMagic(int attackValue) {  //兴奋技能
	auto* pMagic = FindSkill(_MAGIC_Excited_);
	auto pDb = (pMagic ? pMagic->GetMagicDataBase() : nullptr);
	if (!pDb) {
		return 0;
	}
	int roll = _random(10000);
	if (roll > pDb->nSuccessCof) {
		return 0;
	}
	int bonusAttack = static_cast<int>(
		(static_cast<int64_t>(attackValue) * pDb->nDamageCof) / Denom
		);
	if (sConfigMgr.IsAtkDebug()) {
		CChat::sendClient(toPlayer(), "玩家%s, 触发被动技能兴奋", getName());
		CChat::sendClient(this, "随机数=%d,成功率=%d,技能伤害系数增加=%d,触发增加物攻=%d", roll, pDb->nSuccessCof, pDb->nDamageCof, bonusAttack);
	}
	return attackValue;
}

int CPlayerObj::DoContinuousFiringMagic(int attackValue) {
	auto* pMagic = FindSkill(_MAGIC_ContinuousFiring_);
	auto pDb = (pMagic ? pMagic->GetMagicDataBase() : nullptr);
	if (!pDb) {
		return 0;
	}
	int roll = _random(10000);
	if (roll > pDb->nSuccessCof) {
		return 0;
	}
	int bonusAttack = static_cast<int>(
		(static_cast<int64_t>(attackValue) * pDb->nDamageCof) / Denom
		);
	if (sConfigMgr.IsAtkDebug()) {
		CChat::sendClient(toPlayer(), "玩家%s, 触发被动连射", getName());
		CChat::sendClient(this, "随机数=%d,成功率=%d,技能伤害系数增加=%d,触发增加物攻=%d", roll, pDb->nSuccessCof, pDb->nDamageCof, bonusAttack);
		CChat::sendClient(this, "枪师,被动技能兴奋,随机数=%d,成功率=%d,技能伤害系数增加=%d,触发增加物攻=%d", roll, pDb->nSuccessCof, pDb->nDamageCof, bonusAttack);
	}
	return bonusAttack;
}

bool CPlayerObj::DoCounterSkill(CCreature* target) {
	if (!target) {
		return false;
	}
	auto* skill = FindSkill(_MAGIC_Counterattack_);
	auto db = (skill ? skill->GetMagicDataBase() : nullptr);
	if (!db) {
		return true;
	}
	int roll = _random(10000);
	if (roll > db->nSuccessCof) {
		return false;
	}
	stCretAttack attackcmd;
	attackcmd.dwTmpId = GetObjectId();
	attackcmd.stTarget.dwtmpid = target->GetObjectId();
	attackcmd.stTarget.xDes = target->m_nCurrX;
	attackcmd.stTarget.yDes = target->m_nCurrY;
	attackcmd.dwMagicID = _MAGIC_Counterattack_;
	skill->doSkill(this, &attackcmd);

	if (sConfigMgr.IsDefDebug()) {
		CChat::sendClient(this, "玩家%s, 触发被动反击", getName());
		CChat::sendClient(this, "反击技能触发,随机数=%d,成功率=%d", roll, db->nSuccessCof);
	}
	return true;
}


bool CPlayerObj::IsEnoughAttackCostNeng(stCretAttack* pcmd) {
	auto job = GetJobType();
	if (job == emJobType::SWORD_JOB || job == emJobType::GUN_JOB) {
		CCreature* pTatget = GetEnvir()->FindCretByTmpId(pcmd->stTarget.dwtmpid, pcmd->stTarget.xDes, pcmd->stTarget.yDes);
		if (pTatget && pTatget->toMonster() && pTatget->toMonster()->GetMonType() == emMonsterType::ORE) //攻击目标为矿石不检查普攻能量值
			return true;
		return m_res[ResID::neng] > 0;

	}
	return true;
}

bool CPlayerObj::DoAttackCostNeng() {
	if (GetEnvir() && GetEnvir()->isNoResNeng()) return true; //特殊地图不消耗能量
	auto job = GetJobType();
	if (job == emJobType::SWORD_JOB || job == emJobType::GUN_JOB) {
		if (m_Packet.FindItemInBodyByPos(EQUIP_WEAPONS)) {
			if (m_res[ResID::neng] > 0)
				return ResChange(ResID::neng, -1, __FUNCTION__);
			else {
				return false;
			}
		}
	}
	return true;
}

int CPlayerObj::CalcMonDamage(CMonster* pMon) {
	if (pMon->GetMonType() == emMonsterType::ORE) {
		int todayEnergy = getMiningNeng();
		if (todayEnergy <= 0) {
			sendTipMsg(this, vformat("`6`挖矿能量不足，请明日再来"));
			return 0;
		}
		if (todayEnergy >= pMon->GetOreCostEnergy()) {
			if (GetFreeCellCount() > 0) {
				return 1;
			}
			else {
				sendTipMsg(this, vformat("`6`背包已满无法挖矿"));
				return 0;
			}
		}
		else {
			sendTipMsg(this, vformat("`6`剩余能量不足无法挖矿"));
			return 0;
		}
	}
	return 0;
}

bool CPlayerObj::CanGetMonReward(CMonster* Monster) {
	if (Monster == NULL) return false;
	int monLv = Monster->GetMonsterDataBase()->nLevel;
	if (monLv > m_nKillMaxMonLv || monLv < m_nKillMinMonLv) {
		CChat::sendClient(toPlayer(), "玩家%s, 击杀怪物无法获取奖励， 前往击杀%d-%d怪物", getName(), m_nKillMinMonLv, m_nKillMaxMonLv);
		return false;
	}
	bool candrop = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("MonCanDrop", true, this, Monster);
	return candrop;
}

int CPlayerObj::GetGuardFeature()
{
	CItem* pItem = m_Packet.m_stEquip[MECHANICAL_GUARD];
	if (pItem) {
		BYTE guardLv = pItem->m_Item.btGuardLv;
		if (pItem->m_Item.nDura > 0) {
			return pItem->GetFaceId() + guardLv;
		}
		else {
			return pItem->GetFaceId() + guardLv + 5; //守护能量消失
		}
	}
	return 0;
}

void CPlayerObj::LuaChangeProperty(BYTE emType) {
	SetAbilityFlag((emAbilityFlag)emType);
	DoChangeProperty(m_stAbility, true, __FILE_FUNC__);

}

int CPlayerObj::getMiningNeng() {
	int todayCostEnergy = quest_vars_get_var_n("energycnt");
	return CUserEngine::getMe().m_nMiningLimit - todayCostEnergy;
}

bool CPlayerObj::UpdateCharge(int nChange, const char* szEvtName, DWORD& dwtransid) {
	if (CUserEngine::getMe().RMBUnusual) {
		g_logger.error("rmb消耗异常");
		return false;
	}
	if (nChange < 0 && m_res[ResID::charge] < abs(nChange))
		return false;
	if (nChange < 0) {
		if (GetTickCount64() < m_ChangeRmbTick)
			return false;
		m_ChangeRmbTick = GetTickCount64() + 50;
	}
	dwtransid = InterlockedIncrement(&g_dwtransid);
	stUpdateAccountRmbCmd updatecmd;
	updatecmd.nChangeRMB = nChange;
	updatecmd.nCurRMB = m_res[ResID::charge];
	updatecmd.dwtransid = dwtransid;
	updatecmd.i64onlyid = m_i64UserOnlyID;
	strcpy_s(updatecmd.szAccount, sizeof(updatecmd.szAccount) - 1, m_pGateUser->m_szAccount);
	if (CUserEngine::getMe().isCrossSvr()) {
		if (m_dwSrcZoneId > 0 && !CUserEngine::getMe().isCrossSvr(m_dwSrcZoneId)) {
			// 更新账号数据库
			return CUserEngine::getMe().BroadcastGameSvr(&updatecmd, sizeof(updatecmd), 0/*pMap->getSvrIdType()*/, true, m_dwSrcZoneId, m_wSrcTrade);
		}
		else {
			g_logger.error("rmb消耗异常%d", m_dwSrcZoneId);
			return false;
		}
	}
	else {
		if (m_pGateUser && m_pGateUser->m_OwnerLoginSvr && m_pGateUser->m_OwnerLoginSvr->IsConnected())
			return m_pGateUser->m_OwnerLoginSvr->sendcmd(&updatecmd, sizeof(updatecmd));
	}
	return false;
}

void CPlayerObj::SetCcyLog(ResID rid, const char* szEventName, int nChange, int nOld, DWORD dwtransid) {
	switch (rid)
	{
	case ResID::none:
		break;
	case ResID::charge:
	{
		BYTE btPayType = 0;
		if (m_btGmLvl > 0) {
			btPayType = 2;
		}
		if (nChange < 0) {
			m_dwUseRmb -= nChange;
		}
		std::string strBillNo = vformat("%d", dwtransid);
		g_logger.debug("能源晶石(rmb)改变'账号:%d-%s','%s','%s',%I64d,%d,%d,%d,%d", dwtransid, szEventName, getAccount(), getName(), m_i64UserOnlyID, nOld, nChange, m_res[rid], btPayType);
		int mainLand = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("GetSvrRMBMainland", 1);
		GameService::getMe().Send2LogSvr(_SERVERLOG_RMB_CHANGE, 0, 0, this,
			"'%s','%s','%s',%I64d,%d,'%s',%d,%d,%d,%d,%d,%d,%d",
			vformat("%s-%s", "datebase", szEventName),
			getAccount(),
			getName(),
			m_i64UserOnlyID,
			this->m_dwLevel,
			strBillNo.c_str(),
			nOld,
			nChange,
			m_res[rid],
			0,
			btPayType,
			m_dwUseRmb,
			mainLand);
		break;
	}
	case ResID::chargebind:
		if (nChange != 0) {
			GameService::getMe().Send2LogSvr(_SERVERLOG_BINDRMBCHANGE_, 0, 0, this, "'%s','%s','%s',%I64d,%d,%d,%d,%d",
				szEventName,
				getAccount(),
				getName(),
				m_i64UserOnlyID,
				m_dwLevel,
				nOld,
				nChange,
				m_res[rid]);
		}
		break;
	case ResID::game:
		if (abs(nChange) >= 10000) {
			GameService::getMe().Send2LogSvr(_SERVERLOG_BINDRMBGOLDCHANGE_, 0, 0, this, "'%s','%s','%s',%I64d,%I64d,%d,'%s'",
				szEventName,
				getAccount(),
				getName(),
				m_i64UserOnlyID,
				m_res[rid],
				nChange,
				GetEnvir()->getMapName());
		}
		break;
	case ResID::gamebind:
		FUNCTION_BEGIN;
		if (nChange != 0 && abs(nChange) > 10000) {
			GameService::getMe().Send2LogSvr(_SERVERLOG_LIJIN_, 0, 0, this, "'%s','%s','%s',%I64d,%d,%I64d,%d,%I64d",
				szEventName,
				getAccount(),
				getName(),
				m_i64UserOnlyID,
				m_dwLevel,
				nOld,
				nChange,
				m_res[rid]);
		}
		break;
	default:
		break;
	}
}

int CPlayerObj::GetDrugShareCd()
{
	int nRecoverySpeed = m_stAbility[AttrID::DrugRestoreSpeed] + m_stAbility[AttrID::DrugCdPhase] * sConfigMgr.GetDrugStageChange();
	int nShareCd = sConfigMgr.GetShareDrugBaseCd() * (1 - nRecoverySpeed / 10000.0f);
	nShareCd = nShareCd < sConfigMgr.GetShareDrugMinCd() ? sConfigMgr.GetShareDrugMinCd() : nShareCd;
	return nShareCd;
}