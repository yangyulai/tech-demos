#include "BaseCreature.h"
#include "AStar.h"
#include "../gamesvr.h"
#include "../dbsvrGameConnecter.h"
#include "GameMap.h"
#include "UsrEngn.h"
#include "Chat.h"
#include "Config.h"
#include "MonsterObj.h"
#include "MagicRange.h"
#include "timeMonitor.h"

CCreature::CCreature(std::string_view name, uint8_t type, PosType x, PosType y, DWORD dwTmpId)
	: MapObject(name, type, x, y)
	  , m_pswitchsvrmoveinfo(NULL)
	  , m_switchsvrtime(0)
	  , m_nSpaceMoveMovetype(0)
	  , m_dwLevel(0)
	  , m_cMagic(this)
	  , m_changeMapPos(0, 0)
	  , m_cBuff(this)
	  , m_moveType(0)
	  , m_moveStep(1)
{
	FUNCTION_BEGIN;
	m_LifeState = NOTDIE;
	m_btTargetState = 0;
	m_dwDieTime = 0;
	m_nAgePoint = 0;
	m_pEnvir = NULL; //地图
	m_nNowHP = 0;
	m_nNowMP = 0;
	m_btDirection = DRI_DOWN; //方向 DR_DOWN
	m_wHomeMapID = 0;
	m_wHomeCloneMapId = 0;
	m_nHomeX = 0;
	m_nHomeY = 0;
	m_nViewRangeX = 7; //视野
	m_nViewRangeY = 7;
	m_boGmHide = false;
	m_dwNextRunTick = ::GetTickCount64() + _random(200); //该对象上次被处理的时间
	m_nRunIntervalTick = 30; //处理该对象的间隔时间
	m_btWudi = 0; //无敌
	m_btMiaosha = 0; //秒杀
	m_dwCurAttTargetTmpId = 0;
	m_curAttTarget = NULL;
	m_dwLoseTargetTick = 0;
	m_btDamageType = 0; //伤害类型
	m_wDamageLoss = 0; //伤害损耗
	m_dwLastHiterTmpId = 0;
	m_pLastHiter = NULL;
	m_dwNextMoveTick = GetTickCount64();
	m_dwMoveIntervalTime = 600;
	m_dwNextHitTick = 0; //下次可攻击时间
	m_dwHitIntervalTime = 1100; //攻击时间间隔
	m_dwNextCastTick = 0;
	m_dwCastIntervalTime = 1000;
	m_dwPkRunTime = 0;
	m_boHpToZero = false;
	m_boImmunePoisoning = false;
	m_boNoByPush = false;
	m_btBattleCamp = emBattle_Null;
	m_boBattleWuDi = false;
	m_boAllowSpace = false;
	m_dwScriptId = 0;
	m_dwLastLevelUpTime = 0;
	m_dwNoviceGuideId = 0;
	m_Timer = CLD_DEBUG_NEW CScriptTimerManager(&m_QuestList, this);
	m_boCheckViewRange = false;
	m_boNoParalysis = false;
	m_NoCdMode = false;
	m_vars.needThreadSafe = true;
	isTransMove = false;
}

CCreature::~CCreature(){
	ClearMsg();
	SAFE_DELETE(m_Timer);
}

void CCreature::InitEvent()
{
	if (m_stAbility[AttrID::HpRestoreFiveSec] || m_stAbility[AttrID::MpRestoreFiveSec]){
		timer_.AddTimer(5000, [this]()
			{
				StatusValueChange(stCretStatusValueChange::hp, m_stAbility[AttrID::HpRestoreFiveSec], __FUNC_LINE__);
				StatusValueChange(stCretStatusValueChange::mp, m_stAbility[AttrID::MpRestoreFiveSec], __FUNC_LINE__);
			});
	}
}
void CCreature::MakeGhost(bool delaydispose,const char* ff){
	FUNCTION_BEGIN;
	g_logger.debug("CCreature %.8x  %s ID=%d标记为离开游戏! %d : %s",this,getName(), GetObjectId(),(BYTE)delaydispose,ff);
	m_LifeState=ISDIE;
	Disappear();
	if (GetEnvir()){
		if(isPlayer()){
			toPlayer()->m_Petm.clear();
		}
		GetEnvir()->DoLeaveMap(this);
	}
	Delete();
	CUserEngine::getMe().RemoveCreature(this);
}

void CCreature::Disappear()
{
	FUNCTION_BEGIN;
	stMapRemoveCret remove;
	remove.dwTmpId= GetObjectId();
	remove.removetype=isTransMove ? transTypeOutView:transTypeNone;
	remove.btCretType= GetType();
	SendRefMsg(&remove,sizeof(remove),true);
	isTransMove = false;
	static zLogger::zLevel tmploglvl(zLogger::zDEBUG.name,zLogger::zDEBUG.writelevel,6,zLogger::zDEBUG.realtimewrite,zLogger::zDEBUG.showcolor);
	g_logger.log(tmploglvl,"CCreature %.8x  %s 从场景 %.8x 中删除!ID=%d(xy:%d:%d)",this,getName(),GetEnvir(), GetObjectId(),m_nCurrX,m_nCurrY);
}

void CCreature::ClearMsg() {
	AILOCKT(m_MsgList);
	for (auto& msg:m_MsgList)
	{
		msg.clear();
	}
	m_MsgList.clear();
}

bool CCreature::GetGameMessage(stProcessMessage& ProcessMsg)
{
	FUNCTION_MONITOR(48, "CCreature::GetGameMessage()");
	AILOCKT(m_MsgList);
	auto it = std::find_if(m_MsgList.begin(), m_MsgList.end(),
		[currentTick = GetTickCount64()](const stProcessMessage& msg) {
			return msg.dwDelayTick == 0 || currentTick > msg.dwDelayTick;
		});
	if (it != m_MsgList.end())
	{
		ProcessMsg = *it;
		m_MsgList.erase(it);
		return true;
	}
	return false;
}
void CCreature::ProcessMsgList()
{
	stProcessMessage ProcessMsg;
	while (GetGameMessage(ProcessMsg)) {
		if (ProcessMsg.pMsg) {
			ProcessMsg.bofreebuffer = true;
			Operate(ProcessMsg.pMsg->pluscmd(), ProcessMsg.pMsg->pluscmdsize(), &ProcessMsg);
			ProcessMsg.clear();
		}
	}
}

void CCreature::pushDelayMsg(CCreature* BaseObject, stQueueMsg* sMsg, int nDelay) {
	FUNCTION_BEGIN;
	if (!sMsg) { return; }
	stProcessMessage stMsg;
	if (nDelay == 0) {
		stMsg.dwDelayTick = 0;
	}
	else {
		stMsg.dwDelayTick = GetTickCount64() + nDelay;
	}
	stMsg.pCret = BaseObject;
	stMsg.pMsg = sMsg;

	AILOCKT(m_MsgList);
	m_MsgList.push_back(stMsg);
}

void CCreature::pushMsg(CCreature* BaseObject, stQueueMsg* sMsg) {
	FUNCTION_BEGIN;
	if (!sMsg) { return; }
	pushDelayMsg(BaseObject, sMsg, 0);
}

void CCreature::pushDelayMsg(CCreature* BaseObject, void* pcmd, int ncmdlen, int nDelay) {
	stQueueMsg* pmsg = CopyQueueMsg(pcmd, ncmdlen);
	pushDelayMsg(BaseObject, pmsg, nDelay);
}

void CCreature::pushMsg(CCreature* BaseObject, void* pcmd, int ncmdlen) {
	stQueueMsg* pmsg = CopyQueueMsg(pcmd, ncmdlen);
	pushMsg(BaseObject, pmsg);
}

void CCreature::Update(){
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true,"");
	FUNCTION_MONITOR(48, "");
	ProcessMsgList();
	timer_.Update();
	m_cBuff.Run();
	if (!isDie() && m_nNowHP <= 0) {
		Die();
	}
	FeatureChanged();
	SetLastHitter(NULL);
}

bool  CCreature::Operate(stBaseCmd* pcmd,int ncmdlen,stProcessMessage* pMsg)
{
	FUNCTION_BEGIN;
	char dis[MAX_PATH] = { 0 };
	sprintf_s(dis, sizeof(dis), "包号(%d,%d)", pcmd->cmd, pcmd->subcmd);
	FUNCTION_WRAPPER(true, dis);
	FUNCTION_MONITOR(48, dis);
	bool boRet=false;
	switch (pcmd->value)
	{
	case 0:
		{

		}
		break;
	default:
		{
			g_logger.error("%s 未定义的消息 %d %d (%d)",getName(),pcmd->cmd,pcmd->subcmd,ncmdlen);
			if (isPlayer()) {
				toPlayer()->KickPlayer(pcmd->cmd, pcmd->subcmd);
			}
		}
		break;
	}
	return boRet;
}

void CCreature::GetBaseProperty(){
	FUNCTION_BEGIN;
	return;
}

void CCreature::ChangeProperty(bool bosend,const char* ff){
	FUNCTION_BEGIN;
	DoChangeProperty(m_stAbility,bosend,ff);
}

void CCreature::ChangePropertyCheckPlayer(emAbilityFlag type) {
	FUNCTION_BEGIN;
	if (isPlayer()) {
		toPlayer()->SetAbilityFlag(type);
	}
	else {
		ChangeProperty();
	}
}

void CCreature::DoChangeProperty(stARpgAbility& abi,bool boNotif,const char* ff){
	FUNCTION_BEGIN;
	return;
}

void CCreature::SendRefMsg(void* pcmd,int ncmdlen,bool exceptme){
	auto curMap = GetEnvir();
	if (!curMap)
		return;
	curMap->ForeachVisualObjects(this, [&](MapObject* object)
		{
			if (object->GetType() != CRET_PLAYER)
				return;
			if (auto player = dynamic_cast<CPlayerObj*>(object))
			{
				if (exceptme && player == this) { return; }
				player->SendMsgToMe(pcmd, ncmdlen);
			}
		});
}

bool CCreature::SetEnvir(CGameMap* pEnvir){
	FUNCTION_BEGIN;
	static zLogger::zLevel tmploglvl(zLogger::zDEBUG.name,zLogger::zDEBUG.writelevel,6,zLogger::zDEBUG.realtimewrite,zLogger::zDEBUG.showcolor);
	g_logger.log(tmploglvl,"CCreature %.8x  %s 从场景 %.8x 中移动到 %.8x (%s) !",this,getName(),m_pEnvir,pEnvir,(pEnvir!=NULL)?pEnvir->getMapName():"" );
	if(pEnvir && m_pEnvir!=pEnvir){
		m_pEnvir=pEnvir;
	}
	return true;
}
void CCreature::AfterSpaceMove(MapObject* obj)
{
	stCretAfterSpaceMove retCmd;
	retCmd.dwTmpId = GetObjectId();
	retCmd.dir = m_btDirection;
	retCmd.ncurx = m_nCurrX;
	retCmd.ncury = m_nCurrY;
	if (isPlayer())
	{
		retCmd.spacemovetype = toPlayer()->m_btChangeSvrSpaceMoveType;
	}
	obj->SendMsgToMe(&retCmd, sizeof(retCmd));
}

void CCreature::EnterMapNotify(MapObject* obj)
{

}

void CCreature::OnLeaveTargetCheck(MapObject* obj)
{
	if (auto player = dynamic_cast<CPlayerObj*>(obj))
	{
		if (m_curAttTarget==player)
		{
			LoseTarget();
		}
	}
}

void CCreature::LeaveMapNotify(MapObject* obj)
{
	if (obj->GetType() != CRET_PLAYER) return;
	OnLeaveTargetCheck(obj);
	stMapRemoveCret cmd;
	cmd.dwTmpId = GetObjectId();
	cmd.btCretType = GetType();
	obj->SendMsgToMe(&cmd, sizeof(cmd));
	//if (isNpc())g_logger.debug("%s 【%d,%d %d,%d】 离开视野", getName(), GetX(), GetY(), GetGridX(), GetGridY());
}

void CCreature::MapMoveNotify(MapObject* obj)
{
	stCretMoveRet retcmd;
	fullMoveRet(&retcmd, 0);
	obj->SendMsgToMe(&retcmd, sizeof(retcmd));
}
void CCreature::LoseTarget()
{
	m_curAttTarget=NULL;
	m_dwCurAttTargetTmpId=0;
}


void CCreature::SetAttackTarget(CCreature* pTarget,bool ignoreTarget)
{
	if (!pTarget)
		return;
	if (pTarget == m_curAttTarget)
		return;
	if (m_curAttTarget && !ignoreTarget)
		return;
	if (pTarget == this)
		return;
	if (!CanHit())
		return;
	if (!isEnemy(pTarget))
		return;
	m_curAttTarget = pTarget;
	m_dwCurAttTargetTmpId = pTarget->GetObjectId();
	m_dwLoseTargetTick = GetTickCount64() + LOST_TARGET_INTERVAL;
	m_dwNextMoveTick = GetTickCount64();
}

bool CCreature::MoveTo(int nDir, int nmovesetp, int ncx, int ncy, bool boNotCheckObj)
{
	auto curMap = GetEnvir();
	if (!curMap)
		return false;
	if (!CanWalk() || isDie())
	{
		return false;
	}
	int nsetp=nmovesetp;
	int ncux=m_nCurrX;
	int ncuy=m_nCurrY;
	int ndx=0,ndy=0;
	while(nsetp>0){
		if (!(curMap->GetNextPosition(ncux,ncuy,nDir,1,ndx,ndy) && curMap->CanWalk( this,ndx,ndy,m_nCurrZ,boNotCheckObj))){
			break;
		}
		ncux=ndx;
		ncuy=ndy;
		nsetp--;
	}
	if (nsetp>0){
		//中间有障碍
	}else{
		m_moveStep = nmovesetp;
		m_btDirection = nDir;
		if (nmovesetp<=0 || curMap->DoMapMove(this,ndx,ndy) ){
			return true;
		}
	}
	return false;
}

//尝试行走~8个方向
bool CCreature::TryWalk(int wantdir, bool boNotCheckObj)
{
	int oldx=m_nCurrX;
	int oldy=m_nCurrY;
	MoveTo(wantdir,1,0,0,boNotCheckObj);
	int rand=_random(7);
	for (int i=DRI_UP;i<DRI_NUM;i++){
		if (oldx==m_nCurrX&& oldy==m_nCurrY){

			if (!num_in(rand,0,7)){
				rand=DR_UP;
			}
			if (rand!=wantdir){
				MoveTo(rand,1,0,0,boNotCheckObj);
				rand++;
			}
		}else{
			m_dwNextHitTick=max(m_dwNextHitTick,GetTickCount64())+700 ;  //移动后 限定下次攻击时间 以保证前端可以将移动的动作播放完毕
			m_dwNextCastTick=max(m_dwNextCastTick,GetTickCount64())+700;
			return true;
		}
	}
	return false;
}

bool CCreature::CanHit() {
	if (m_cBuff.GetBuffState(MAGICSTATE_PETRIFACTION))
	{
		return false;
	}
	return true;
}
bool CCreature::CanWalk(){
	if (m_cBuff.GetBuffState(MAGICSTATE_PETRIFACTION) || m_cBuff.GetBuffState(MAGICSTATE_DIZZY))
	{
		return false;
	}
	return true;
}
bool CCreature::CanRun(){
	if (m_cBuff.GetBuffState(MAGICSTATE_PETRIFACTION) || m_cBuff.GetBuffState(MAGICSTATE_DIZZY))
	{
		return false;
	}
	return true;
}
bool CCreature::CanMagic(){
	if (m_cBuff.GetBuffState(MAGICSTATE_PETRIFACTION))
	{
		return false;
	}
	return true;
}

bool CCreature::isInViewRange(CCreature* pCret,bool boPrintFalselog){
	if (pCret){
		if (pCret==this){ return true; }
		if ( (pCret	&& isInViewRangeXY(pCret->m_nCurrX,pCret->m_nCurrY)
			&& ((pCret->isPlayer()) || isBoss() || isPlayer() || isPet())
			&&pCret->isValidObj()  
			&& !pCret->m_boGmHide
			&& pCret->GetEnvir()==GetEnvir() ) ){
				if ( pCret->m_boCheckViewRange && isPlayer() ){
					if (pCret->isNpc()){
						if (m_boCheckViewRange){
							return false;
							//return CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("lua_isInViewRange",false,(CCreature*)this,pCret);
						}else{
							return false;
						}
					}else{
						return true;
					}
				}else{
					return true;
				}
		}
		if (boPrintFalselog)
		{
			if (pCret	&&  pCret->GetEnvir() != GetEnvir()) {
				g_logger.error("isInViewRange pCret地图不正确!");
				return false;
			}
		}
		
	}
	if (boPrintFalselog)
	{
		g_logger.error("isInViewRange pCret不存在!");
	}
	return false;
}

bool CCreature::isInViewRangeXY(WORD x,WORD y){
	//FUNCTION_BEGIN;
	BYTE btX = m_nViewRangeX;
	BYTE btY = m_nViewRangeY;
	if (GetEnvir()) {
		btX = btX + GetEnvir()->getViewChangeX();
		btY = btY + GetEnvir()->getViewChangeY();
	}
	return (((x >= (m_nCurrX - btX)) && (x <= (m_nCurrX + btX)))
		&& ((y >= (m_nCurrY - btY)) && (y <= (m_nCurrY + btY))));
}

void CCreature::fullMoveRet(stCretMoveRet* pcmd, BYTE errorcode){
	pcmd->location.mapid=GetEnvir()->getMapId();
	pcmd->location.ncurx=m_nCurrX;
	pcmd->location.ncury=m_nCurrY;
	pcmd->location.ncurz=m_nCurrZ;
	pcmd->dwTmpId= GetObjectId();
	pcmd->dir=(BYTE)m_btDirection;
	pcmd->moveerrorcode=(BYTE)errorcode;
	pcmd->btmovesetp= m_moveStep;
	pcmd->btMoveType= m_moveType;
}

void CCreature::MapChanged(){
 
	
}

bool CCreature::DoSpaceMove(stSpaveMoveInfo* pSpaceMoveInfo){
	FUNCTION_BEGIN;
	if (!pSpaceMoveInfo){ 
		return false; 
	}
	// 初始化区域 ID 和平台 ID
	pSpaceMoveInfo->dwZoneid = pSpaceMoveInfo->dwZoneid == 0xFFFFFFFF ? GameService::getMe().m_nZoneid : pSpaceMoveInfo->dwZoneid;
	pSpaceMoveInfo->wTradeid = pSpaceMoveInfo->wTradeid == 0 ? GameService::getMe().m_nTradeid : pSpaceMoveInfo->wTradeid;

	// 检查是否合法
	if (!CheckMapSpace(pSpaceMoveInfo)) {
		pSpaceMoveInfo->btMoveState = stSpaveMoveInfo::_SPACEMOVE_FINISH_;
		return false;
	}

	// 处理跨服务器移动
	if (pSpaceMoveInfo->dwZoneid != GameService::getMe().m_nZoneid || pSpaceMoveInfo->wTradeid != GameService::getMe().m_nTradeid) {
		return HandleCrossServerMove(pSpaceMoveInfo);
	}

	// 处理地图内移动
	auto pMap = pSpaceMoveInfo->DstMap;
	if (pMap) {
		// 本服
		if (pSpaceMoveInfo->dwSvrIdx == GameService::getMe().m_svridx)
		{
			if (pMap == GetEnvir() && pMap->getMapCloneId() == pSpaceMoveInfo->dwCloneMapId) { 
				return HandleMoveLocalSameMap(pSpaceMoveInfo);
			}
			return HandleMoveLocalDiffMap(pSpaceMoveInfo);
		}
		// 跨服
		if (m_boAllowSpace || !isMonster()) {
			if (isPlayer() && !getSwitchSvrInfo() && pSpaceMoveInfo->btMoveState == stSpaveMoveInfo::_SPACEMOVE_NONE_) {
				//return HandleMoveCross(pSpaceMoveInfo);
			}
		}
	}
	
	// 移动失败处理
	pSpaceMoveInfo->btMoveState = stSpaveMoveInfo::_SPACEMOVE_FINISH_;
	return false;
}

bool CCreature::CheckMapSpace(stSpaveMoveInfo* pSpaceMoveInfo){
	FUNCTION_BEGIN;
	if (!pSpaceMoveInfo || !pSpaceMoveInfo->DstMap)return false;
	if ((isPlayer() || isPet()) && CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
		stAutoSetScriptParam autoparam(this);
		bool boCheckScript = true;
		if (isPlayer()) {
			boCheckScript = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("CheckMapSpace", true, GetEnvir(), pSpaceMoveInfo->DstMap, pSpaceMoveInfo->btMoveType, this);
		}
		if (toPet()) {
			boCheckScript = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("CheckMapSpace", true, GetEnvir(), pSpaceMoveInfo->DstMap, pSpaceMoveInfo->btMoveType, toPet()->getMaster());
		}
		if (!boCheckScript){return boCheckScript;}
	}
	if (pSpaceMoveInfo->btMoveType==CUserEngine::_MOVE_TYPE_MAPGATE_)return true;
	bool boRet=false;
	CGameMap* pMap=NULL;
	if (GetEnvir()){
		pMap=GetEnvir();
		if (pSpaceMoveInfo->btMoveType!=CUserEngine::_MOVE_TYPE_GOHOME && GetEnvir()->isNoSpaceExceptHome() ){
			boRet=false;
		}else boRet=true;
		if (boRet){
			if ( GetEnvir()->isNoSpaceAll() ){
				boRet=false;
			}
		}
	}
	if (boRet){
		if (pSpaceMoveInfo->DstMap){
			pMap=pSpaceMoveInfo->DstMap;
			if (pSpaceMoveInfo->btMoveType!=CUserEngine::_MOVE_TYPE_GOHOME && pSpaceMoveInfo->DstMap->isNoSpaceExceptHome() ){
				boRet=false;
			}else boRet=true;
			if (boRet){
				if ( pSpaceMoveInfo->DstMap->isNoSpaceAll() ){
					boRet=false;
				}
			}
		}else boRet=false;
	}
	if (!boRet && isPlayer() && pMap){
		CChat::sendClient(this->toPlayer(),GameService::getMe().GetStrRes(RES_LANG_MAPCHANGE),pMap->getMapName());
	}
	return boRet;
}

bool CCreature::HandleCrossServerMove(stSpaveMoveInfo* pSpaceMoveInfo) {
	pSpaceMoveInfo->btMoveState = stSpaveMoveInfo::_SPACE_WAIT_SWITCHSVR_CHECK_;
	stPreChangeGameSvrCmd checkcmd;
	FillPreChangeGameSvrCmd(&checkcmd, pSpaceMoveInfo);
	if (CUserEngine::getMe().BroadcastGameSvr(&checkcmd, sizeof(checkcmd), MAKELONG(pSpaceMoveInfo->dwSvrIdx, _GAMESVR_TYPE), true, checkcmd.dwDestZoneId, checkcmd.wDestTradeid)) {
		setSwitchSvrInfo(pSpaceMoveInfo);
	}
	else {
		pSpaceMoveInfo->btMoveState = stSpaveMoveInfo::_SPACEMOVE_FINISH_;
	}
	return true;
}

void CCreature::FillPreChangeGameSvrCmd(stPreChangeGameSvrCmd* checkcmd, stSpaveMoveInfo* pSpaceMoveInfo) {
	checkcmd->notcheckobj = pSpaceMoveInfo->boNotCheckObj;
	checkcmd->oldPlayerOnlyId = isPlayer() ? toPlayer()->m_i64UserOnlyID : 0;
	checkcmd->dx = pSpaceMoveInfo->X;
	checkcmd->dy = pSpaceMoveInfo->Y;
	checkcmd->dwSrcZoneId = GameService::getMe().m_nZoneid;
	checkcmd->dwDestZoneId = pSpaceMoveInfo->dwZoneid;
	checkcmd->dwGameSvrVersion = CUserEngine::getMe().m_dwThisGameSvrVersion.load();
	checkcmd->wSrcTradeid = GameService::getMe().m_nTradeid;
	checkcmd->wDestTradeid = pSpaceMoveInfo->wTradeid;
	checkcmd->full_mapid = stSvrMapId(pSpaceMoveInfo->DstMap->getMapId(), pSpaceMoveInfo->dwSvrIdx - _GAMESVR_TYPE, pSpaceMoveInfo->dwCloneMapId).all;
	checkcmd->space_move_tick = pSpaceMoveInfo->space_move_tick;
	checkcmd->old_gamesvr_id_type = GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value;
	checkcmd->btrandxy = pSpaceMoveInfo->btGetRandXY;
}

bool CCreature::HandleMoveLocalSameMap(stSpaveMoveInfo* pSpaceMoveInfo) {
	pSpaceMoveInfo->btMoveState = stSpaveMoveInfo::_SPACEMOVE_FINISH_;
	if (pSpaceMoveInfo->DstMap->CanWalk(this, pSpaceMoveInfo->X, pSpaceMoveInfo->Y, 0, pSpaceMoveInfo->boNotCheckObj)) {
		if (!isPlayer()) {
			LoseTarget();
		}
		if (pSpaceMoveInfo->DstMap->MoveCretTo(this, pSpaceMoveInfo->X, pSpaceMoveInfo->Y, m_nCurrZ, pSpaceMoveInfo->boNotCheckObj)) {
			//AfterSpaceMove(this);
			EnterMapNotify(this);
			m_dwSpaceMoveTime = time(NULL);
			isTransMove = false;
		}
	}
	return true;
}

bool CCreature::HandleMoveLocalDiffMap(stSpaveMoveInfo* pSpaceMoveInfo) {
	auto pMap = pSpaceMoveInfo->DstMap;
	if (pMap->getMapCloneId() != pSpaceMoveInfo->dwCloneMapId && pSpaceMoveInfo->dwCloneMapId != 0) {
		auto* pNewMap = CUserEngine::getMe().m_maphash.FindById(pMap->getMapId(), 0, pSpaceMoveInfo->dwCloneMapId);
		if (!pNewMap) {
			return false;
		}
		pMap = pNewMap;
	}
	pSpaceMoveInfo->btMoveState = stSpaveMoveInfo::_SPACEMOVE_FINISH_;
	if (pMap->CanWalk(this, pSpaceMoveInfo->X, pSpaceMoveInfo->Y, 0, pSpaceMoveInfo->boNotCheckObj)) {
		if (!isPlayer()) {
			LoseTarget();
		}
		m_nSpaceMoveMovetype = pSpaceMoveInfo->btMoveType;
		m_dwSpaceMoveTime = time(NULL);

	}
	return true;
}


void CCreature::FillFastTransferPlayerCmd(stFastTransferPlayer* cmd, CPlayerObj* player) {
	player->fullPlayerChangeSvrData(&cmd->playerBinData, getsafepacketbuflen() - sizeof(stFastTransferPlayer) - 1024, _SAVE_TYPE_CHANGESVR_);
	cmd->playerBinData.gamedata.dwmapid = player->m_changeMapId.part.mapid;
	cmd->playerBinData.gamedata.wclonemapid = player->m_changeMapId.part.cloneId;
	cmd->playerBinData.gamedata.x = m_changeMapPos.x;
	cmd->playerBinData.gamedata.y = m_changeMapPos.y;

	cmd->ip_type = player->m_pGateUser->m_iptype;
	cmd->fclientver = player->m_pGateUser->m_fclientver;
	cmd->gamesvr_id_type = MAKELONG(player->m_changeMapId.part.line+300, _GAMESVR_TYPE);
	strcpy_s(cmd->playername, sizeof(cmd->playername) - 1, player->getName());
	strcpy_s(cmd->account, sizeof(cmd->account) - 1, player->getAccount());
	cmd->useronlyid = player->m_i64UserOnlyID;
	cmd->loginsvr_tmpid = player->m_pGateUser->m_tmpid;
	strcpy_s(cmd->szBundleId, sizeof(cmd->szBundleId) - 1, player->m_pGateUser->m_szClientBundleId);
	strcpy_s(cmd->szPlatform, sizeof(cmd->szPlatform) - 1, player->m_pGateUser->m_szClientPlatform);
	strcpy_s(cmd->szVersion, sizeof(cmd->szVersion) - 1, player->m_pGateUser->m_szClientVersion);
}

void CCreature::FillNotifyPlayerChangeGameSvrCmd(stNotifyPlayerChangeGameSvrCmd* ncc, CPlayerObj* player) {
	ncc->newsvr = MAKELONG(player->m_changeMapId.part.line + 300, _GAMESVR_TYPE);
	ncc->oldsvr = MAKELONG(GameService::getMe().m_svridx, _GAMESVR_TYPE);
	ncc->tmpid = player->m_pGateUser->m_tmpid;
	strcpy_s(ncc->szName, sizeof(ncc->szName) - 1, player->getName());
	strcpy_s(ncc->szAccount, sizeof(ncc->szAccount), player->m_pGateUser->m_szAccount);
}

void CCreature::FeatureChanged(){
	if (m_featureStream.Length() == 0)
		return;
	STACK_ALLOCA_BUFFER(stCretChangeFeature, cmd, m_featureStream.Length() + 10);
	cmd->dwTmpId = GetObjectId(); //eCretType
	cmd->crettype = GetType();
	cmd->feature.push_back((char*)m_featureStream.GetMem(), m_featureStream.Length());
	SendRefMsg(cmd, sizeof(*cmd) + cmd->feature.getarraysize(), false);
	m_featureStream.Clear();
}

bool CCreature::NameChanged(){
	FUNCTION_BEGIN;
	stCretChangeName changename;
	changename.dwTmpId= GetObjectId();
	getShowName(changename.szShowName,sizeof(changename.szShowName));
	SetDisplayName(m_name.c_str());
	if(isPet()){
		if(toPet()->isPlayerMaster()){
			strcpy_s(changename.szMasterName,sizeof(changename.szMasterName),toPet()->getPlayerMaster()->getName());
			changename.btType = 2;
		}
	}
	else if(isMonster() && toMonster()->GetMonsterDataBase()->boOwner && toMonster()->getOwnerId()){
		CPlayerObj* pPlayer = GetEnvir()->GetPlayer(toMonster()->getOwnerId());
		if(pPlayer){
			strcpy_s(changename.szMasterName,sizeof(changename.szMasterName),pPlayer->getName());
			if(pPlayer->m_GroupInfo.dwGroupId > 0){
				changename.btType = 5;
			}else{
				changename.btType = 4;
			}
		}
	}

	SendRefMsg(&changename,sizeof(changename));	
	return true;
}


bool CCreature::LevelChanged(BYTE btShow){
	FUNCTION_BEGIN;
	stCretLevelUp levelcmd;
	levelcmd.dwTempId= GetObjectId();
	levelcmd.dwLevel=m_dwLevel;
	levelcmd.i64LeftExp=0;
	levelcmd.i64MaxExp=0;
	levelcmd.dwLeveUpTime = time(NULL);
	levelcmd.btShow=btShow;
	SendRefMsg(&levelcmd,sizeof(levelcmd));
	return true;
}

const char* CCreature::getShowName(char* szbuffer,int nmaxlen){
	FUNCTION_BEGIN;
	static char s_szShowNameBuffer[512];
	if(szbuffer==NULL){
		szbuffer=s_szShowNameBuffer;
		nmaxlen=count_of(s_szShowNameBuffer);
	}
	if (szbuffer){	
		strcpy_s(szbuffer,nmaxlen-1,getName()); 
		filtershowname(szbuffer,strlen(szbuffer));
		return szbuffer;
	}
	return getName();
}

bool CCreature::CanBeSee(CCreature* pCret)
{
	if (m_boGmHide) {
		return false;
	}
	if (pCret->isBoss()) {
		return true;
	}

	return true;
}

bool CCreature::isLittleMon() {
	return (isMonster() && toMonster()->GetMonType() == emMonsterType::NORMAL) ? true : false;
}

bool CCreature::isEliteMon(){
	return (isMonster() && toMonster()->GetMonType() == emMonsterType::ELITEMON) ? true : false;
}

bool CCreature::isBoss()
{
	return (isMonster() && toMonster()->GetMonType() == emMonsterType::BOSS)?true:false;
}

bool CCreature::Die(){
	FUNCTION_BEGIN;
	auto curMap = GetEnvir();
	if (!curMap) return false;
	if (isDie())
		return false;
	m_LifeState = ISDIE;
	m_dwDieTime = time(NULL);
	if (m_nNowHP!=0){ m_nNowHP=0;m_boHpToZero=true;}
	SendLiftStateChange();
	if (!m_pLastHiter && m_dwLastHiterTmpId) {
		SetLastHitter(curMap->GetCreature(m_dwLastHiterTmpId));
	}
	return true;
}

void CCreature::SendReliveMsg() {//特效用
	//FUNCTION_BEGIN;
	stCretReliveEffect lifestatecmd;
	lifestatecmd.dwTmpId = GetObjectId();
	SendRefMsg(&lifestatecmd, sizeof(lifestatecmd));
}
bool CCreature::LuaRelive(double nHp, double nMp) {
	FUNCTION_BEGIN;
	return Relive((int64_t)nHp, (int64_t)nMp);
}

void CCreature::SendLiftStateChange()
{
	stCretLifeStateChange lifestatecmd;
	lifestatecmd.dwTmpId = GetObjectId();
	lifestatecmd.curLifeState = m_LifeState;
	lifestatecmd.posX = m_nCurrX;
	lifestatecmd.posY = m_nCurrY;
	SendRefMsg(&lifestatecmd, sizeof(lifestatecmd));
}

bool CCreature::Relive(int64_t nHp, int64_t nMp){
	FUNCTION_BEGIN;
	if ( isDie()){
		if (isPlayer()) {
			toPlayer()->m_dwSaveMoveTimeForNext = GetTickCount64();
		}
		m_LifeState = NOTDIE;
		int64_t hp = nHp > 0 ? min(nHp, m_stAbility[AttrID::MaxHP]) : m_stAbility[AttrID::MaxHP];
		int64_t mp = nMp > 0 ? min(nMp, m_stAbility[AttrID::MaxMP]) : m_stAbility[AttrID::MaxMP];
		StatusValueChange(stCretStatusValueChange::hp, hp, __FUNC_LINE__);
		StatusValueChange(stCretStatusValueChange::mp, mp, __FUNC_LINE__);
		m_boHpToZero=false;
		SendLiftStateChange();
	}
	return true;
}

bool CCreature::isSwitchSvrTimeOut(){
	FUNCTION_BEGIN;
	return (m_pswitchsvrmoveinfo && ((time(NULL)-m_switchsvrtime)>30) ); 
}

bool CCreature::isSwitchSvr(){
	//FUNCTION_BEGIN;
	return  ( m_pswitchsvrmoveinfo && m_pswitchsvrmoveinfo->btMoveState>=stSpaveMoveInfo::_SPACE_WAIT_SWITCHSVR_ ); 
}

bool CCreature::isValidObj(){
	return isClientReady();
}

bool CCreature::isSafeZone(CCreature* pTarget){
	FUNCTION_BEGIN;
	//没有安全区
	if(GetEnvir()->isNoSafeZone()) return false;
	if (isMonster())return false; //怪物保留安全区打人

	CCreature* pSrc = this;
	CCreature* pDst = pTarget;
	if(isPet()){
		if(toPet()->isPlayerMaster()){
			pSrc = toPet()->getPlayerMaster();
		}
	}
	if(pTarget && pTarget->isPet()){
		if(pTarget->toPet()->isPlayerMaster()){
			pDst = pTarget->toPet()->getPlayerMaster();
		}
	}
	if (pDst) {
		if (pDst->isMonster()) 
			return false;
		else
			return pDst->GetEnvir()->getSafeType(pDst->m_nCurrX, pDst->m_nCurrY) != 0;
	}

	if(pSrc){
		return (pSrc->GetEnvir()->getSafeType(pSrc->m_nCurrX, pSrc->m_nCurrY) != 0);
	}else{
		return (GetEnvir()->getSafeType(pSrc->m_nCurrX, pSrc->m_nCurrY) != 0);
	}
	return false;
}

bool CCreature::GetFrontPosition(int &nX,int &nY)
{
	if (GetEnvir()){
		nX=m_nCurrX;
		nY=m_nCurrY;
		return GetEnvir()->GetNextPosition(m_nCurrX,m_nCurrY,m_btDirection,1,nX,nY);
	}
	return false;
}

CCreature* CCreature::GetPoseCreate(){
	int nX,nY;
	if(GetFrontPosition(nX,nY)){
		return	GetEnvir()->GetCretInXY(nX,nY,false);
	}
	return NULL;
}

bool CCreature::testHomeXY(){
	FUNCTION_BEGIN;
	if ( GetEnvir()){
		DWORD dwHomeMapId=GetEnvir()->getHomeMapId();
		if (dwHomeMapId && dwHomeMapId!=m_wHomeMapID){
			m_wHomeMapID=dwHomeMapId;

			m_wHomeCloneMapId=0;
			m_nHomeX=GetEnvir()->getHomeX();
			m_nHomeY=GetEnvir()->getHomeY();
			return true;
		}
	}
	return false;
}

enum{
	ZONENULL=0,			//普通区域
	SAFEZONE=1,			//安全区
	PKNOSWORD=2,		//不加PK值
	DIENODROPEQUIP=4,	//死亡不掉身上装备
	DIENODROPALL=8,		//死亡不掉全部装备
};

void CCreature::StatusValueChange(BYTE btIndex, int nChange, const char* szLog, bool isForce) {
	FUNCTION_BEGIN;
	int old = 0;
	int newVal = 0;
	int nMax = 0;
	switch (btIndex)
	{
	case stCretStatusValueChange::hp:
		nMax = m_stAbility[AttrID::MaxHP];
		if (isWudi()) 
			nChange = nMax;
		old = m_nNowHP;
		newVal = old + nChange;
		newVal = max(newVal, 0);
		newVal = min(newVal, nMax);
		m_nNowHP = newVal;
		if (m_nNowHP > 0 && !isDie()) {
			m_boHpToZero = false;
		}
		break;
	case stCretStatusValueChange::mp:
		nMax = m_stAbility[AttrID::MaxMP];
		old = m_nNowMP;
		newVal = old + nChange;
		newVal = max(newVal, 0);
		newVal = min(newVal, nMax);
		m_nNowMP = newVal;
		break;
	case stCretStatusValueChange::pp:
		nMax = m_stAbility[AttrID::MaxPP];
		old = m_nNowPP;
		newVal = old + nChange;
		newVal = max(newVal, 0);
		newVal = min(newVal, nMax);
		m_nNowPP = newVal;
		break;
	default:
		break;
	}
	if (old != newVal || isForce)
	{
		stCretStatusValueChange retcmd;
		retcmd.onlyid = isPlayer() ? toPlayer()->m_i64UserOnlyID : 0;
		retcmd.dwtmpid = GetObjectId();
		retcmd.btIndex = btIndex;
		retcmd.nNow = newVal;
		retcmd.nMax = nMax;
		retcmd.nChange = newVal - old;
		SendRefMsg(&retcmd, sizeof(retcmd));
		if (btIndex == stCretStatusValueChange::hp && isPlayer()){
			auto* p = toPlayer();
			if (p->m_GroupInfo.dwGroupId > 0){
				std::unordered_set<CPlayerObj*> playerSet;
				for (auto it = p->m_cGroupMemberHash.begin(); it != p->m_cGroupMemberHash.end(); it ++){
					if (auto player = CUserEngine::getMe().m_playerhash.FindByOnlyId(it->first))
					{
						if (player != p)
							player->SendMsgToMe(&retcmd, sizeof(retcmd));
					}
				}
			}

		}
	}
}

void CCreature::StruckDamage(int64_t nDamage, CCreature *pSrcCret, Byte btDamageSrc)
{
	if (CUserEngine::getMe().m_scriptsystem.m_LuaVM){
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("CretStruckDamage", pSrcCret, this, (double)nDamage, (int)btDamageSrc);
	}
	if (isMonster())
	{
		if (toMonster()->GetMonType() == emMonsterType::FORTRESSDOOR)
		{
			CPlayerObj* pAttackPlayer = nullptr;
			if (pSrcCret->isPet()) pAttackPlayer = pSrcCret->toPet()->getMaster()->toPlayer();
			if (pSrcCret->isPlayer()) pAttackPlayer = pSrcCret->toPlayer();
			if (pAttackPlayer && pAttackPlayer->m_GuildInfo.dwGuildId > 0)
			{
				CUserEngine::getMe().m_GuildFortress.UpdateDoorDam(pAttackPlayer->GetEnvir(), pAttackPlayer->m_GuildInfo.dwGuildId, nDamage);
			}
			stCretStatusValueChange retcmd;
			retcmd.onlyid = isPlayer() ? toPlayer()->m_i64UserOnlyID : 0;
			retcmd.dwtmpid = GetObjectId();
			retcmd.btIndex = stCretStatusValueChange::hp;
			retcmd.nNow = m_nNowHP;
			retcmd.nMax = m_stAbility[AttrID::MaxHP];
			retcmd.nChange = -nDamage;
			SendRefMsg(&retcmd, sizeof(retcmd));
			return;
		}				
	}
	StatusValueChange(stCretStatusValueChange::hp, -nDamage, __FUNC_LINE__);
}

bool CCreature::DamageSpell(int64_t nDamage)
{ 
	//减兰
	FUNCTION_BEGIN;
	if (isWudi()){nDamage=m_stAbility[AttrID::MaxMP];}
	int64_t nOldMp = m_nNowMP;
	nOldMp+= nDamage;
	if (nOldMp<0){
		/*g_logger.debug("MP不足,为:%d,需要:%d",m_nNowMP,nDamage);*/ 
		return false;
	}
	else {
		//g_logger.debug("当前MP:%d,当前MP改变%d,改变后MP:%d",m_nNowMP,nDamage,nOldMp);
		m_nNowMP = nOldMp;
		m_nNowMP = min(m_stAbility[AttrID::MaxMP],m_nNowMP);
		return true;
	}
	return false;
}

bool CCreature::SpellCanUse(int64_t nDamage){
	//判断蓝是否足够
	FUNCTION_BEGIN;
	if (m_nNowMP>=abs(nDamage)){
		return true;
	}else return false;
}

bool CCreature::CalculatingPk(CCreature* pCret,emPkStatus PkStatus){
	FUNCTION_BEGIN;
	switch (PkStatus)
	{
	case PK_STATUS_RUNTIME:
		{
			if (pCret){
				CPlayerObj* pPlayer=pCret->getPlayer();
				auto pAttacker=getPlayer();
				if (pAttacker && pPlayer && pPlayer!=this && pPlayer->m_res[ResID::pk] ==0 && pPlayer->m_dwPkRunTime==0){
					bool boAddPkRunTime=true;
					boAddPkRunTime=!(pPlayer->GuildWarCheck(pAttacker->toPlayer()));
				}
				return true;
			}
		}break;
	case PK_STATUS_PKSWORD:
		{
			if (pCret && isPlayer() && pCret->isPlayer() 
				&& pCret!=this && !pCret->GetEnvir()->isPkNoSword() && !GetEnvir()->isPkNoSword()){
					bool boAddpksword=true;
					if(toPlayer()->GuildWarCheck(pCret->toPlayer()) || (toPlayer()->m_res[ResID::citizenLv] < 0)) {
						boAddpksword=false;
					}
					if (boAddpksword && m_dwPkRunTime==0 && pCret->toPlayer()->GetPkMode() != PKMODEL_GOODANDEVILMODE ){
						pCret->quest_vars_set_var_n("PkValBeforeDie", pCret->toPlayer()->m_res[ResID::pk], false);
						pCret->toPlayer()->ResChange(ResID::pk, 1, "击杀pk");
						pCret->toPlayer()->sendTipMsg(pCret->toPlayer(), vformat("`1`您击败了玩家%s", getName()));
					}
					if (!pCret->m_boGmHide){
						CChat::sendClient(this->toPlayer(),GameService::getMe().GetStrRes(RES_LANG_YOUDIE),pCret->getName());
					}
					CChat::sendClient(pCret->toPlayer(),GameService::getMe().GetStrRes(RES_LANG_YOUKILL),getName());
					g_logger.debug("PK模式记录 %s 用模式 %d 杀死了 %s",pCret->getName(),pCret->toPlayer()->GetPkMode(),this->getName());

					if (this->toPlayer()->m_xEnemyList.FindByOnlyId(pCret->toPlayer()->m_i64UserOnlyID) == NULL){ 
						stRelationAdd Relationaddcmd;
						Relationaddcmd.btType=LIST_ENEMY;
						strcpy_s(Relationaddcmd.szName,_MAX_NAME_LEN_-1,pCret->getName());
						SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv,this->toPlayer()->m_i64UserOnlyID,&Relationaddcmd,sizeof(Relationaddcmd));
					}
					stRelationInnerChangeDegree innerchangedegreecmd;
					innerchangedegreecmd.btType=LIST_ENEMY;
					innerchangedegreecmd.i64OnlyId=pCret->toPlayer()->m_i64UserOnlyID;
					innerchangedegreecmd.nDegree=-1;
					SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv,this->toPlayer()->m_i64UserOnlyID,&innerchangedegreecmd,sizeof(innerchangedegreecmd));

					stRelation* pRelation=pCret->toPlayer()->m_xEnemyList.FindByOnlyId(this->toPlayer()->m_i64UserOnlyID);
					if (pRelation){
						stRelationInnerChangeDegree innerchangedegreecmd;
						innerchangedegreecmd.btType=LIST_ENEMY;
						innerchangedegreecmd.i64OnlyId=this->toPlayer()->m_i64UserOnlyID;
						innerchangedegreecmd.nDegree=1;
						SENDMSG2BLOBALBYONLYID(stSendFriendMsgSuperSrv,pCret->toPlayer()->m_i64UserOnlyID,&innerchangedegreecmd,sizeof(innerchangedegreecmd));
					}
					if (!boAddpksword){
						this->toPlayer()->GuildWarKill(pCret->toPlayer()->m_GuildInfo.dwGuildId,0,1);
						pCret->toPlayer()->GuildWarKill(this->toPlayer()->m_GuildInfo.dwGuildId,1,0);
					}
					m_dwPkRunTime = 0;
					toPlayer()->UpdateAppearance(FeatureIndex::nameColor, 0);
					return true;
			}
		}break;
	}
	return false;
}

bool CCreature::CalculatingSpeed() {
	FUNCTION_BEGIN;
	DWORD oldhit = m_dwHitIntervalTime;
	DWORD oldcast = m_dwCastIntervalTime;
	DWORD oldmove = m_dwMoveIntervalTime;
	m_dwHitIntervalTime = 800;
	if (m_dwHitIntervalTime < 600)
	{
		m_dwHitIntervalTime = 600;
	}
	m_dwCastIntervalTime = 900;
	m_dwMoveIntervalTime = sConfigMgr.m_gMoveIntervalTime;
	double dwMovePer = GetSpeedPer();
	m_dwMoveIntervalTime = m_dwMoveIntervalTime / (1 + dwMovePer / Denom);
	m_dwWalkIntervalTime = m_dwWalkIntervalTime / (1 + dwMovePer / Denom);
	return oldhit != m_dwHitIntervalTime || oldcast != m_dwCastIntervalTime || oldmove != m_dwMoveIntervalTime;
}

int CCreature::CalculatingRestoreHp(){
	return m_stAbility[AttrID::HpRestore];
}

int CCreature::CalculatingRestoreMp(){
	FUNCTION_BEGIN;
	return m_stAbility[AttrID::MaxMP];
}

int CCreature::CalculatingRestorePp() {
	FUNCTION_BEGIN;
	return 0;
}

int64_t CCreature::CalculatingDamage(CCreature* pTarget, const std::shared_ptr<stMagicDataBase>& pMagicData, stInt& TmpDamageType,int& nAggro,int nTargetNum,int nPlayerTargetNum,int nTotalTargetNum){ //计算攻击伤害
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true,"");
	FUNCTION_MONITOR(48,"");
	if(!pTarget){ return 0; }
	if (isMiaosha()>0){return pTarget->m_stAbility[AttrID::MaxHP] * (min(isMiaosha(),100) / 100.0f);}
	int64_t nDamage=0;
	//检查PK,检查队友等等
	if (pMagicData){
		if (pMagicData->nDamageType){
			if (isPlayer()) {
				quest_vars_set_var_n("AttackTargetID", pTarget->GetObjectId(), false);
				if (pTarget->isPlayer())
					quest_vars_set_var_n("AttackTargetType", 1, false);
				else if (pTarget->isMonster())
					quest_vars_set_var_n("AttackTargetType", 2, false);
				else
					quest_vars_set_var_n("AttackTargetType", 0, false);
			}
			nDamage = CalculatingBaseDamage(pTarget, pMagicData, TmpDamageType, nTargetNum, nPlayerTargetNum, nTotalTargetNum);
			//if (pMagicData->nDamageType == DAMAGETYPE_PHYSICAL && pTarget->isPlayer() && pTarget->m_cBuff.FindBuff()) //魔法师防护技能效果
			//	nDamage = nDamage * 30 / 100;
			nAggro = TmpDamageType.nAggro;
			CalculatingPk(pTarget,PK_STATUS_RUNTIME);
		}
	}
	return nDamage;
}

int64_t CCreature::CalculatingBaseDamage(CCreature* pTarget, const std::shared_ptr<stMagicDataBase>& pMagicData, stInt& TmpDamageType, int nTargetNum, int nPlayerTargetNum, int nTotalTargetNum) {
	int64_t damage = 0;
	bool debug_atk = sConfigMgr.IsAtkDebug();
	bool debug_def = sConfigMgr.IsDefDebug();
	bool debug = debug_atk || debug_def;
	if (pTarget) {
		if (pTarget->isMonster())
		{
			switch (pTarget->toMonster()->GetMonType())
			{
				case emMonsterType::ORE:
				{
					if(isPlayer()) return toPlayer()->CalcMonDamage(pTarget->toMonster());
				}break;
				case emMonsterType::FORTRESSBALL:
				{
					if (isPlayer() || isPet()) {
						DWORD dwGuildId = isPlayer() ? toPlayer()->m_GuildInfo.dwGuildId : toPet()->getMaster()->toPlayer()->m_GuildInfo.dwGuildId;
						if (pTarget->toMonster()->m_guildId == dwGuildId) return -1; //军团id 相等就是友方能量球
						return 1;
					}
					return 0;
				}break;
			}
		}
		//第一个环节:
		//		判断命中
		//		是否暴击
		//		攻击防御运算,
		//		伤害计算 (反击技能 被攻击者拥有反击技能被攻击触发)
		//		第一个环节获得基础伤害
		if (debug) {
			if (isPlayer() && debug_atk) {
				if (pMagicData->nDamageType == DAMAGETYPE_PHYSICAL)
					CChat::sendClient(toPlayer(), "==========您攻击:物理技能:%d，技能必中状态:%d==========", pMagicData->nID, pMagicData->btSkillHit);
				else if (pMagicData->nDamageType == DAMAGETYPE_MAGIC)
					CChat::sendClient(toPlayer(), "==========您攻击:魔法技能:%d，技能必中状态:%d==========", pMagicData->nID, pMagicData->btSkillHit);
			}
			else if (pTarget->isPlayer() && debug_def) {
				if (pMagicData->nDamageType == DAMAGETYPE_PHYSICAL)
					CChat::sendClient(pTarget->toPlayer(), "==========您被攻击:物理技能:%d，技能必中状态:%d==========", pMagicData->nID, pMagicData->btSkillHit);
				else if (pMagicData->nDamageType == DAMAGETYPE_MAGIC)
					CChat::sendClient(pTarget->toPlayer(), "==========您被攻击:魔法技能:%d，技能必中状态:%d==========", pMagicData->nID, pMagicData->btSkillHit);
			}
		}
		//是否命中
		if (!IsHit(pTarget, pMagicData)){
			TmpDamageType.nInt = Miss;
			return 0;
		}
		// 是否暴击
		bool isCrit = IsCrit(pTarget); 		
		if (isCrit) {
			TmpDamageType.nInt = PCritical;
		}

		//伤害计算阶段
		//物理攻击力/魔法攻击力
		int randAtk = GetAtk(pMagicData);
		//物理/魔法防御
		int randDef = GetDef(pTarget, pMagicData->nDamageType);
		// 反击攻击力
		int nCounterAttack = GetCounterAttack(pMagicData, pTarget, randAtk,TmpDamageType);
		// temp对抗攻击力
		int tempConfrontAttack = nCounterAttack > 0 ? nCounterAttack : randAtk;
		//最小伤害值
		int nMinDamage = ceil(tempConfrontAttack * 0.03);
		//获取被动增加的物攻
		int nExtraAttack = PassiveMagicAddAttack(pMagicData, tempConfrontAttack);
		nExtraAttack = nExtraAttack > 0 ? nExtraAttack : 0;
		// 默认暴击增伤
		int nCritAddDamge = 10000; // 100% =1
		int nPVEMul = pTarget->isPlayer() ? 10000 : m_stAbility[AttrID::PveMul];	 // PVE倍率 (10000  = 100%)
		int nPVPMul = pTarget->isPlayer() ? m_stAbility[AttrID::PvpMul] : 10000 ;	 // PVP倍率
		int nPVEMulResult = 0;			 // PVE乘区结果
		int nPVPMulResult = nPVPMul;	 // PVP乘区结果
		int BossDamageVal = 0;	 //   Boss克制值计算
		int BossDamagePer = 0;	 //  Boss克制率计算

		int nTempDamageDec = 0; //获取增加减伤%
		int nTempPuncture = 0;	//获取增加物理穿刺%
		int nMagicDamage = pMagicData->nDamage;	 //加入计算的技能伤害值
		// 攻击者
		if (isPlayer()) { 
			nTempPuncture = toPlayer()->DoPunctureMagic(pMagicData->nDamageType); 	// 穿刺技能 获取物理穿刺%
			nMagicDamage = 0;
		}
		//被攻击者
		if (pTarget->isPlayer()) { 
			// 闪避技能 获取减伤%
			nTempDamageDec = pTarget->toPlayer()->DoShanBiMagic();
		} else {
			nPVEMulResult = nPVEMul;
			if (pTarget->isBoss()) {
				BossDamageVal = m_stAbility[AttrID::BossDamageInc] - pTarget->m_stAbility[AttrID::BossDamageDec];
				BossDamagePer = m_stAbility[AttrID::BossDamageIncPer] - pTarget->m_stAbility[AttrID::BossDamageDecPer];
				nPVEMulResult = nPVEMul + 10000 + BossDamagePer;
			}
		}

		// 有效减伤计算 (后续会加个物理/魔法伤害加成/减免，后续做功能再加入计算)
		int nEffectDamageDec = 10000 + m_stAbility[AttrID::DamageAddPer] - pTarget->m_stAbility[AttrID::DamageDecPer] + nTempDamageDec;
		nEffectDamageDec = nEffectDamageDec > 500 ? nEffectDamageDec : 500; // 500 = 5%

		// 穿刺率计算
		int nPuncture = m_stAbility[AttrID::PunctureRate] - pTarget->m_stAbility[AttrID::PunctureDecRate] + nTempPuncture;
		nPuncture = nPuncture > 0 ? nPuncture : 0;

		// 最终计算得对抗攻击力
		int FinalAttack = tempConfrontAttack + m_stAbility[AttrID::AttackAdd] - pTarget->m_stAbility[AttrID::AttackDec] - randDef * (1 - nPuncture / Denom) + BossDamageVal + nExtraAttack;
		FinalAttack = FinalAttack > nMinDamage ? FinalAttack : nMinDamage;

		// 暴击伤害
		if (isCrit) {
			nCritAddDamge = 12500 + m_stAbility[AttrID::CritMul] - pTarget->m_stAbility[AttrID::CritDec];
			nCritAddDamge = nCritAddDamge > 10000 ? nCritAddDamge : 10000;
		}
		// 乘区结果
		int nMulResult = (nPVEMulResult / Denom * nPVPMulResult / Denom * pMagicData->nDamageCof / Denom * nEffectDamageDec / Denom * nCritAddDamge / Denom) * Denom;
		nMulResult = nMulResult > 500 ? nMulResult : 500; // 500 = 5%

		// 和值结果
		int nAddResult = nMagicDamage + pMagicData->nSkillAddition + m_stAbility[AttrID::FixAddDamage] - pTarget->m_stAbility[AttrID::FixDecDamage];

		// 最终算第一阶段伤害值
		damage = ceil(FinalAttack * nMulResult / Denom) + nAddResult;
		damage = damage > nMinDamage ? damage : nMinDamage;

		// 反击技能触发
		if (pTarget->isPlayer() && pMagicData->nID != _MAGIC_Counterattack_) {
			pTarget->toPlayer()->DoCounterSkill(this);
		}

		if (debug) {
			char msg_1[_MAX_CHAT_LEN_];
			sprintf_s(msg_1, _MAX_CHAT_LEN_ - 1,
				"公式取攻击值=%d,公式取防御值=%d,反击攻击力=%d,技能系数=%d,技能攻击伤害=%d,最小伤害值=%d --- boss类数值 :克制值=%d,抵御值=%d,克制率=%d,抵御率=%d,",
				randAtk, randDef, nCounterAttack, pMagicData->nDamageCof, pMagicData->nDamage, nMinDamage, m_stAbility[AttrID::BossDamageInc], pTarget->m_stAbility[AttrID::BossDamageDec], m_stAbility[AttrID::BossDamageIncPer], m_stAbility[AttrID::BossDamageDecPer]);
			char msg_2[_MAX_CHAT_LEN_];
			sprintf_s(msg_2, _MAX_CHAT_LEN_ - 1,
				"穿刺: 穿刺技能增加穿刺率=%d,闪避技能获取减伤=%d,穿刺率=%d，穿刺减免率=%d, 穿刺率计算 =%d,",
				nTempPuncture, nTempDamageDec, m_stAbility[AttrID::PunctureRate], pTarget->m_stAbility[AttrID::PunctureDecRate], nPuncture);
			char msg_3[_MAX_CHAT_LEN_];
			sprintf_s(msg_3, _MAX_CHAT_LEN_ - 1,
				"对抗结果: 对抗攻击力=%d,攻击固定增加=%d,攻击固定减少=%d，公式取防御值 =%d,穿刺率=%d,BOSS克制值抵御值计算=%d,被动增加物攻=%d, 最终攻击力计算得=%d ",
				tempConfrontAttack, m_stAbility[AttrID::AttackAdd], pTarget->m_stAbility[AttrID::AttackDec], randDef, nPuncture, BossDamageVal, nExtraAttack,FinalAttack);
			char msg_4[_MAX_CHAT_LEN_];
			sprintf_s(msg_4, _MAX_CHAT_LEN_ - 1,
				"有效减伤: 伤害增加=%d,伤害减免=%d,闪避技能获得减伤率=%d, 计算得减伤=%d;  暴击增伤= %d ",
				m_stAbility[AttrID::DamageAddPer], pTarget->m_stAbility[AttrID::DamageDecPer], nTempDamageDec, nEffectDamageDec, nCritAddDamge);
			char msg_5[_MAX_CHAT_LEN_];
			sprintf_s(msg_5, _MAX_CHAT_LEN_ - 1,
				"PVE/PVP:  PVP倍率=%d,PVE倍率=%d, 技能追加=%d,固定增伤=%d,固定减伤=%d,乘区结果=%d,和值结果=%d ",
				nPVEMul, nPVPMul, pMagicData->nSkillAddition, m_stAbility[AttrID::FixAddDamage], pTarget->m_stAbility[AttrID::FixDecDamage], nMulResult, nAddResult);
			char msg[_MAX_CHAT_LEN_];
			sprintf_s(msg, _MAX_CHAT_LEN_ - 1,
				"第一阶段最终计算伤害=%I64d", damage);
			if (isPlayer() && debug_atk) {
				CChat::sendClient(toPlayer(), "%s", msg_1);
				CChat::sendClient(toPlayer(), "%s", msg_2);
				CChat::sendClient(toPlayer(), "%s", msg_3);
				CChat::sendClient(toPlayer(), "%s", msg_4);
				CChat::sendClient(toPlayer(), "%s", msg_5);
				CChat::sendClient(toPlayer(), "%s", msg);
			}
			if (pTarget->isPlayer() && debug_def) {
				CChat::sendClient(pTarget->toPlayer(), "%s", msg_1);
				CChat::sendClient(pTarget->toPlayer(), "%s", msg_2);
				CChat::sendClient(pTarget->toPlayer(), "%s", msg_3);
				CChat::sendClient(pTarget->toPlayer(), "%s", msg_4);
				CChat::sendClient(pTarget->toPlayer(), "%s", msg_5);
				CChat::sendClient(pTarget->toPlayer(), "%s", msg);
			}
		}
		
		if (isPlayer() && pTarget->isMonster() && damage >= pTarget->m_nNowHP) {
			if (debug_atk) CChat::sendClient(toPlayer(), "怪物死亡，伤害值%d, 怪物血量%d", damage, pTarget->m_nNowHP);
			return damage;
		}
		if (damage <= 0) return 0; //最小伤害为0，返回
		char szfunc[QUEST_FUNC_LEN];
		sprintf_s(szfunc, QUEST_FUNC_LEN - 1, "%s", SKILL_FORMULA);
		do {
			stAutoSetScriptParam autoparam(this);
			damage = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<double>(szfunc, 0, (CCreature*)this, pTarget, pMagicData, &TmpDamageType, pTarget->m_wDamageLoss, nTargetNum, nPlayerTargetNum, nTotalTargetNum, (double)damage);
		} while (false);
	}
	return damage;
}

bool CCreature::CalculatingTarget(std::vector<CCreature*>& vTarget,stMagic* pMagic,DWORD dwTmpId,int nX,int nY){
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true,"");
	FUNCTION_MONITOR(48,"");
	CCreature* Target=NULL;
	if (pMagic){
		auto magicDataBase = pMagic->GetMagicDataBase();
		if (magicDataBase->btShape==SHAPE_SELF && pMagic->isTarget(this,this)){//对自身施放
			vTarget.push_back(this);
			return true;
		}else{
			if (magicDataBase->btAttackType==NEARLY_ATTACK){
				if (GetEnvir()->GetCretsInMagicRange(m_nCurrX,m_nCurrY,m_btDirection,nX,nY,vTarget,dwTmpId,magicDataBase,this,magicDataBase->wAttackNum,(magicDataBase->boSelf?0: GetObjectId()),magicDataBase->boDie)!=0){
					CCreature* pNewTarget=GetPoseCreate();
					if(pNewTarget)m_curAttTarget=pNewTarget;
					return true;
				}
			}
			else if (magicDataBase->btAttackType==FAR_ATTACK){
				if (GetEnvir()->GetCretsInMagicRange(nX,nY,DRI_NUM,-1,-1,vTarget,dwTmpId,magicDataBase,this,magicDataBase->wAttackNum,(magicDataBase->boSelf?0: GetObjectId()),magicDataBase->boDie)!=0)
				{
					const int missRange = 2;	//允许的误差范围（从客户端发送到服务器收到这段时间，攻击对象可能已经移动了一点距离）
					int maxRange = magicDataBase->btMaxRange + missRange;
					for (auto it = vTarget.begin(); it != vTarget.end(); )
					{
						CCreature* pTarget = *it;
						if (!pTarget || abs(m_nCurrX - pTarget->m_nCurrX) > maxRange || abs(m_nCurrY - pTarget->m_nCurrY) > maxRange)
						{
							it = vTarget.erase(it);

// 							if (pTarget)
// 							{
// 								g_logger.warn("施法距离验证失败：技能名=%s 最大施法距离=%d 技能释放者=%s(%d, %d) 技能作用目标=%s(%d, %d)", pMagic->getShowName(), maxRange,
// 									this->m_szCretName, m_nCurrX, m_nCurrY, pTarget->m_szCretName, pTarget->m_nCurrX, pTarget->m_nCurrY);
// 							}
						}
						else
						{
							it++;
						}
					}
					return true;
				}
			}
		}
	}
	return false;
}

bool CCreature::DoAttack(std::vector<CCreature*> vTarget,stCretAttack* pAttackCmd, stMagic* pMagic, int nDir, DWORD dwPlayTime,bool boSendAttack){
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true,"");
	FUNCTION_MONITOR(48,"");
	m_btDirection=nDir;
	bool boCDTime=false;
	stCretAttackRet retcmd;
	retcmd.dwtmpid= GetObjectId();
	retcmd.ncurx=m_nCurrX;
	retcmd.ncury=m_nCurrY;
	retcmd.ncurz=m_nCurrZ;
	retcmd.btDirect=m_btDirection;
	auto magicDataBase = pMagic->GetMagicDataBase();
	retcmd.dwMagicID=magicDataBase->nID;
	retcmd.btMagicLevel=magicDataBase->btlevel;
	retcmd.btErrorCode=0;
	CCreature* Target=NULL;
	std::vector<CCreature*>::iterator it;	

	if (DamageSpell(-(int64_t)(magicDataBase->dwNeedMP+m_stBaseAbility[AttrID::MaxMP]*(magicDataBase->dwNeedMPCof/10000.0f)))){
		if (isMonster() || isPlayer() || isPet()){
			if (magicDataBase->btAttackType==NEARLY_ATTACK  || isSkillRange(pAttackCmd->stTarget.xDes,pAttackCmd->stTarget.yDes,magicDataBase->btMaxRange)){
				DWORD dwCof = (DWORD)magicDataBase->nSuccessCof;
				if ( _random(10000)<= dwCof || isRobot()){

					if (magicDataBase->btMagicBattleType == MAGICTYPE_COMMON){

						if (vTarget.size()==0){
							switch (magicDataBase->nID)
							{
							default:
								{
									stCretActionRet Actionretcmd;
									Actionretcmd.dwTempId= GetObjectId();
									Actionretcmd.dwTargetId=0;
									if (magicDataBase->boDir){
										Actionretcmd.btDir=nDir;
									}else{
										Actionretcmd.btDir=(BYTE)-1;
									}
									Actionretcmd.nX=pAttackCmd->stTarget.xDes;
									Actionretcmd.nY=pAttackCmd->stTarget.yDes;
									Actionretcmd.dwMagicId=magicDataBase->nSkillActionId;
									Actionretcmd.dwActionTick=dwPlayTime;
									pushDelayMsg(this,&Actionretcmd,sizeof(Actionretcmd),100);
								}break;
							}
						}


						int nPlayerTargetNum = 0;
						if (this->isPlayer()) {
							nPlayerTargetNum = this->toPlayer()->getTotalTargetPlayerNum(vTarget);
						}
						bool boNormalRange = false;
						int nTargetNum = 0;
						for (it=vTarget.begin();it!=vTarget.end();it++){
							Target=(*it);
							nTargetNum++;
							if (Target){
								switch (magicDataBase->nID)
								{
								default:
									{
										if (!boNormalRange){
											stCretActionRet Actionretcmd;
											Actionretcmd.dwTempId= GetObjectId();
											Actionretcmd.dwTargetId=Target->GetObjectId();
											if (magicDataBase->boDir){
												Actionretcmd.btDir=nDir;
											}else{
												Actionretcmd.btDir=(BYTE)-1;
											}
											Actionretcmd.nX=pAttackCmd->stTarget.xDes;
											Actionretcmd.nY=pAttackCmd->stTarget.yDes;
											Actionretcmd.dwMagicId=magicDataBase->nSkillActionId;
											Actionretcmd.dwActionTick=dwPlayTime;
											//if (magicDataBase->btMagicBattleType==MAGICTYPE_COMMONRANGE){
											//	Actionretcmd.dwTargetId=0;
											//	boNormalRange = true;
											//}
											pushDelayMsg(this, &Actionretcmd, sizeof(Actionretcmd), 350);
										}
									}break;
								}

								ReleaseCretStruckFull(Target, magicDataBase, nTargetNum, nPlayerTargetNum, vTarget.size());
								bool bEnemy = isEnemy(Target);

								if ((magicDataBase->btEnemyType == ENEMYTYPE_NONE || (!bEnemy && magicDataBase->btEnemyType == ENEMYTYPE_FRIEND) || (bEnemy && magicDataBase->btEnemyType == ENEMYTYPE_ENEMY))){
									switch (magicDataBase->nID)
									{
									default:
										{
											for (DWORD i=0;i<magicDataBase->szSelfBuffID.size();i++){
												int nBuffID=magicDataBase->szSelfBuffID[i];
												m_cBuff.AddBuff(nBuffID,magicDataBase->btlevel);
											}
											for (DWORD i=0;i<magicDataBase->szBuffID.size();i++){
												int nBuffID=magicDataBase->szBuffID[i];
												Target->m_cBuff.AddBuff(nBuffID, magicDataBase->btlevel, 0, 0, this);												
											}
										}break;
									}
								}
							}
						}
					}else{ //地面魔法
						//
					}
					//设置CD
					switch (magicDataBase->nID)
					{
					default:
						{
							if (pMagic->isCooling(this)) {
								if (isPlayer() && magicDataBase->nResetCdCof > 0 && _random_d(10000)< (DWORD)magicDataBase->nResetCdCof && time(NULL)>quest_vars_get_var_n(vformat("lastResetCdSec_%d", magicDataBase->nID))) {
									CALL_LUA("ResetSkillCd",(CPlayerObj*)this,magicDataBase->nID);
								}
								else {
									pMagic->resetUseTime(this); 
								}
								boCDTime = true;
							}
						}break;
					}
					if (isPet()){
						AddExp(1);
					}
				}else{ //施放技能失败
					retcmd.btErrorCode=CRET_MAGICFAIL_CASTNOTSUCCESS;
					pMagic->resetUseTime(this);
					boCDTime=true;
				}
			}
			else retcmd.btErrorCode=CRET_MAGICFAIL_NORANGE;
		}
		else retcmd.btErrorCode=CRET_MAGICFAIL_LACKITEM;
	}
	else retcmd.btErrorCode=CRET_MAGICFAIL_MAGICLACKING;
	if (boSendAttack){
		SendRefMsg(&retcmd,sizeof(retcmd));
	}	
	if (!boCDTime){
		if (pMagic && magicDataBase){
			stSkillCDTime cdtimecmd;
			cdtimecmd.dwMagicId=pMagic->savedata.skillid;
			cdtimecmd.dwPublicTick=magicDataBase->boPublicCD?GetPublicCDTime():0;
			cdtimecmd.dwSelfTick=GetPublicCDTime();
			SendMsgToMe(&cdtimecmd,sizeof(cdtimecmd));
		}
	}
	return retcmd.btErrorCode == 0;
}

bool CCreature::OnCretAction(stCretActionRet* pcmd, unsigned int ncmdlen, stProcessMessage* param){
	FUNCTION_BEGIN;
	return true;
}

void CCreature::ReleaseCretStruckFull(CCreature* Target, const std::shared_ptr<stMagicDataBase>& pMagicData, int nTargetNum, int nPlayerTargetNum, int nTotalTargetNum ,int32_t delay)
{
	bool boEnemy = isEnemy(Target);
	if (!boEnemy)
	{
		return;
	}
	stInt TmpDamageType;
	int32_t nAggro = 0;
	int64_t nPower = CalculatingDamage(Target, pMagicData, TmpDamageType, nAggro, nTargetNum, nPlayerTargetNum, nTotalTargetNum);
	stCretStruckFull delaycmd;
	delaycmd.dwTempId = Target->GetObjectId();
	delaycmd.npower = nPower;
	delaycmd.nDamageType = TmpDamageType.nInt;
	delaycmd.nHp = Target->m_nNowHP;
	delaycmd.nMaxhp = Target->m_stAbility[AttrID::MaxHP];
	delaycmd.dwMagicID = pMagicData->nID;
	delaycmd.dwSrcTmpId = GetObjectId();
	delaycmd.nAggro = nAggro;
	
	delaycmd.dwAcTmpID = GetObjectId();
	delaycmd.btAddType = TmpDamageType.btAddType;
	delaycmd.btAddTypeTwo = TmpDamageType.btAddTypeTwo;
	delaycmd.btDamageSrc = 0;
	if (!isPlayer()) delaycmd.btDamageSrc = 1;
	if ((Target->GetType() == CRET_PET) && Target->getPlayer())
	{
		delaycmd.dwAcTmpID = Target->GetObjectId();
	}
	if (!delay)
	{
		int nDistance = max(abs(m_nCurrX - Target->m_nCurrX), abs(m_nCurrY - Target->m_nCurrY));
		if (isPlayer() || nDistance > 2)
		{
			Target->pushDelayMsg(this, &delaycmd, sizeof(delaycmd), 20);
		}
		else
		{
			Target->pushDelayMsg(this, &delaycmd, sizeof(delaycmd), 450);
		}
	}
	else
	{
		Target->pushDelayMsg(this, &delaycmd, sizeof(delaycmd), delay);
	}
	if (isPlayer())
	{
		toPlayer()->m_dwSelLineTime = time(NULL);
	}
	if (Target->isPlayer())
	{
		Target->toPlayer()->m_dwSelLineTime = time(NULL);
	}
}


bool CCreature::isSkillRange(int nX,int nY,BYTE btRange){
	FUNCTION_BEGIN;
	return ( ( (nX>=(m_nCurrX-btRange)) && (nX<=(m_nCurrX+btRange)) ) 
		&& ( (nY>=(m_nCurrY-btRange)) && (nY<=(m_nCurrY+btRange)) ) );
}
bool CCreature::isEnemy(CCreature* pTarget){
	FUNCTION_BEGIN;
	return true;
}
void CCreature::TriggerEvent(CCreature* currentuser,EVTTYPE evtid, DWORD evtnum,WORD wTags,WORD mapx,WORD mapy){
	if(currentuser){
		if(currentuser->isPlayer()){
			toPlayer()->m_QuestList.TriggerEvent(toPlayer(),evtid, evtnum,wTags,mapx,mapy);
		}else{
			stAutoSetScriptParam autoparam(currentuser);
			toremd_1_3(evtmapid,evtid,evtnum);
			stEventMapID userevtid = evtmapid._value;
			stQuestEvent* tmpevt= findallevt(CUserEngine::getMe().m_globalquestinfo._events,userevtid);
			if (tmpevt)
			{
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(tmpevt->szeventfunc);
			}
		}
	}
}

bool CCreature::AddExp(double nAdd, const char* szopt){
	FUNCTION_BEGIN;
	return true;
}

bool CCreature::LevelUp(int nAdd, bool bsend){
	FUNCTION_BEGIN;
	return true;
}

stMagic* CCreature::FindSkill(DWORD dwSkillid){
	FUNCTION_BEGIN;
	return m_cMagic.findskill(dwSkillid);
}

bool CCreature::LuaSelfAddBuff(DWORD dwBuffID,BYTE btLevel,double dwKpTime,int dura){
	FUNCTION_BEGIN;
	return m_cBuff.AddBuff(dwBuffID,btLevel,dwKpTime,dura);
}

bool CCreature::LuaSelfAddBuff2(DWORD dwBuffID, BYTE btLevel, double dwKpTime, int dura, CCreature* pA) {
	FUNCTION_BEGIN;
	return m_cBuff.AddBuff(dwBuffID, btLevel, dwKpTime, dura, pA);
}

stBuff* CCreature::LuaFindBuff(DWORD dwBuffID){
	FUNCTION_BEGIN;
	return m_cBuff.FindBuff(dwBuffID);
}

stBuff* CCreature::LuaGetBuffByType(int nState) {
	FUNCTION_BEGIN;
	return m_cBuff.FindByStateType((emMagicStateType)nState);
}

bool CCreature::LuaRemoveBuff(DWORD dwBuffID){
	FUNCTION_BEGIN;
	return m_cBuff.LuaRemoveBuff(dwBuffID);
}

bool CCreature::LuaRemoveBuffState(int nState){
	FUNCTION_BEGIN;
	return m_cBuff.RemoveBuff((emMagicStateType)nState);
}

void CCreature::LuaBuffClear(){
	FUNCTION_BEGIN;
	m_cBuff.clear();
}

bool CCreature::LocalMapTransfer(int nx,int ny,int nrange,bool boNotCheckObj){
	FUNCTION_BEGIN;
	CGameMap* pMap = this->GetEnvir();
	int nX = 0;
	int nY = 0;
	uint8_t oldmovetype = this->m_moveType;
	if (pMap->GetNearNoCretXY(nx,ny,nX,nY,nrange,(nrange*2+1)*(nrange*2+1)-1)) {
		this->m_moveType = empushbak;
		if (pMap->MoveCretTo(this, nX, nY, m_nCurrZ, boNotCheckObj)) {
			m_moveType = oldmovetype;
			if (isPlayer()) {
				toPlayer()->ClearDelayMsg();
			}
		}
		EnterMapNotify(this);
		return true;
	}
	return false;
}

bool CCreature::LuaGetBuffState(int nState){
	FUNCTION_BEGIN;
	return m_cBuff.GetBuffState((emMagicStateType)nState);
}

void  CCreature::LuaFeatureChanged()
{
	FUNCTION_BEGIN;
	FeatureChanged();
}

void  CCreature::LuaUpdateAppearance(BYTE index, int id)
{
	FUNCTION_BEGIN;
	if (index >= 0 && id >= 0) UpdateAppearance(FeatureIndex(index),id);
}

void CCreature::SetLastHitter(CCreature* pLastHitter){
	FUNCTION_BEGIN;
	m_pLastHiter = pLastHitter;
}

double CCreature::quest_vars_get_var_n(const std::string& name){
	FUNCTION_BEGIN;
	double dbret=0.0;
	BYTE lifetype=0;
	char* pgetvalue=nullptr;
	((CVars*)&m_vars)->get_var_c(name,pgetvalue,lifetype);
	if (pgetvalue){
		dbret=strtod(pgetvalue,NULL);
	}
	return dbret;
}

char* CCreature::quest_vars_get_var_s(const std::string& name) {
	FUNCTION_BEGIN;
	BYTE lifetype = 0;
	char* pgetvalue = nullptr;
	((CVars*)&m_vars)->get_var_c(name, pgetvalue, lifetype);
	return pgetvalue;
}

bool CCreature::quest_vars_set_var_n(const std::string& name, double value,bool needsave){
	FUNCTION_BEGIN;
	char setvalue[_MAX_QUEST_LEN_];
	sprintf(setvalue,"%lf",value);
	return ((CVars*)&m_vars)->set_var_c(name,setvalue,(needsave?CVar::_NEED_SAVE_:CVar::_DONT_SAVE_) );
}

void CCreature::SelfCretAction(DWORD dwMagicId,BYTE btlevel){
	FUNCTION_BEGIN;
	auto magic=sJsonConfig.GetMagicDataBase(dwMagicId,btlevel);
	if(magic){
		stCretActionRet Actionretcmd;
		Actionretcmd.dwTempId= GetObjectId();
		Actionretcmd.dwTargetId= GetObjectId();
		if (magic->boDir){
			Actionretcmd.btDir=m_btDirection;
		}else{
			Actionretcmd.btDir=(BYTE)-1;
		}
		Actionretcmd.nX=m_nCurrX;
		Actionretcmd.nY=m_nCurrY;
		Actionretcmd.dwMagicId=magic->nSkillActionId;
		Actionretcmd.dwActionTick=0;
		pushDelayMsg(this,&Actionretcmd,sizeof(Actionretcmd),300);
	}
}

void  CCreature::LuaSendRefMsg(char *szMsg,bool boexceptme){
	BUFFER_CMD(stQuestScriptData,sendcmd,stBasePacket::MAX_PACKET_SIZE);
	sendcmd->dwDataType=0;
	sendcmd->DataArr.push_str(UTG(szMsg));
	if(szMsg && strcmp(szMsg,"")!=0){
		SendRefMsg(sendcmd,sizeof(*sendcmd)+sendcmd->DataArr.getarraysize(),boexceptme);
	}
}

void CCreature::SendCretStruck(CCreature* pCret,int npower, int ntype){
	FUNCTION_BEGIN;
	stCretStruck struckcmd;
	struckcmd.dwTempId= GetObjectId();
	struckcmd.npower=npower;
	struckcmd.nDamageType=ntype;
	struckcmd.nHp = m_nNowHP;
	struckcmd.nMaxhp = m_stAbility[AttrID::MaxHP];
	struckcmd.btDamageSrc = (isPlayer()) ? 0 : 1;
	if (pCret) {
		struckcmd.dwAcTmpID = pCret->GetObjectId();
	}
	SendRefMsg(&struckcmd,sizeof(struckcmd));
}

int CCreature::GetPublicCDTime()
{
	int  nCDtime =  SKILL_CD_TIME;// 
	nCDtime = min(max(nCDtime,0),1500);
	return nCDtime;
}


CPlayerObj * CCreature::getPlayer()
{
	FUNCTION_BEGIN;
	CPlayerObj* pOwnerPlayer = nullptr;

	if (isPlayer()) pOwnerPlayer = toPlayer();
	else if (isPet() && toPet()->getMaster()) pOwnerPlayer = toPet()->getMaster()->getPlayer();
	return pOwnerPlayer;
}

void CCreature::learnSkill(DWORD dwSkillid,BYTE btLevel){
	m_cMagic.addskill(dwSkillid,btLevel);
}

int CCreature::GetSpeedPer()
{
	int  dwMovePer = 0;
	if (auto cof = m_cBuff.GetBuffCof(MAGICSTATE_SPEEDSLOW))
	{
		dwMovePer -= cof;
		DWORD dwBuffid = 14014;			
		auto pbuff = m_cBuff.FindByBuffID(dwBuffid);
		if (auto buffDataBase = pbuff->GetBuffDataBase())
		{
			dwMovePer = 0;								// 此id减速生效，其他减速不生效
			dwMovePer -= buffDataBase->wBuffCof;
		}
	}

	int val = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("Mount_GetSpeedAdd", 0, this);
	if (val)
	{
		dwMovePer += val;
	}
	return dwMovePer;
}

bool CCreature::IsHit(CCreature* pTarget, const std::shared_ptr<stMagicDataBase>& pMagicData) {
	if (pTarget && pMagicData) {
		// 技能命中
		if (pMagicData->btSkillHit) return true;

		// 基础命中率判断
		int baseHitVal = 2001; // 基础命中值
		int rndHitVal = _random(10000);
		bool boHit = rndHitVal < baseHitVal;

		int hitPerSign = 0;
		int rndHitSign = 0;

		// 命中率计算
		if (!boHit) {
			int hitPer = m_stAbility[AttrID::Hit] + m_stAbility[AttrID::HitVal] * 0.005 + pMagicData->nSkillHit - pTarget->m_stAbility[AttrID::Juck] + pTarget->m_stAbility[AttrID::HitVal] * 0.0025;
			hitPerSign = hitPer;
			if (hitPer <= 0) {
				boHit = false;
			} else {
				int rndHit = _random(10000);
				rndHitSign = rndHit;
				if (hitPer >= rndHit) boHit = true;
			}
		}
		if (sConfigMgr.IsAtkDebug()) {
			CChat::sendClient(toPlayer(), "%s, 命中计算", getName());
			CChat::sendClient(toPlayer(), "阶段1 >>基础命中值 =%d ,随机命中值=%d", baseHitVal, rndHitVal);
			CChat::sendClient(toPlayer(), "阶段2 >>命中率 =%d ,随机命中值=%d, 命中结果:%s", hitPerSign, rndHitSign, std::to_string(boHit));
		}
		return boHit;

	}
	return false;
}

bool CCreature::IsCrit(CCreature* pTarget) {
	bool boCrit = true;
	int rndValSign = 0;
	int CritPer = m_stAbility[AttrID::CritRate] / Denom - pTarget->m_stAbility[AttrID::CritResist] / Denom;
	if (CritPer <= 0)
		boCrit = false;
	else {
		int rndVal = _random(10000);
		rndValSign = rndVal;
		if (CritPer < (rndVal / Denom)) boCrit = false;
	}
	if (sConfigMgr.IsAtkDebug() && isPlayer()){
		CChat::sendClient(toPlayer(), "%s, 暴击计算", getName());
		CChat::sendClient(toPlayer(), "暴击概率:%d, 暴击随机数=%d, 最终暴击结果 :%s", CritPer , rndValSign, std::to_string(boCrit));
	}
	return boCrit;
}

int CCreature::GetAtk(const std::shared_ptr<stMagicDataBase>& pMagicData) {
	int ret = 0;
	if (pMagicData->nDamageType == DAMAGETYPE_PHYSICAL) {
		int maxAtk = static_cast<int>(ceil(m_stAbility[AttrID::MaxPAtk] + pMagicData->nSkillAddition * (1 + m_stAbility[AttrID::PAtkPer] / Denom)));
		int minAtk = static_cast<int>(floor(m_stAbility[AttrID::MinPAtk] + pMagicData->nSkillAddition * (1 + m_stAbility[AttrID::PAtkPer] / Denom)));
		if (maxAtk <= minAtk) {
			minAtk = maxAtk + minAtk;
			maxAtk = minAtk - maxAtk;
			minAtk = minAtk - maxAtk;
		}
		ret = _random(minAtk, maxAtk);
			
		if (sConfigMgr.IsBaseValDebug() && isPlayer()) {
			CChat::sendClient(toPlayer(), "%s, 攻击值计算", getName());
			CChat::sendClient(toPlayer(), "技能id = %d, 技能名称 = %s, 技能等级 = %d", pMagicData->nID, pMagicData->szName, pMagicData->btlevel);
			CChat::sendClient(toPlayer(), "物攻下限 = %d, 物攻上限 = %d, 技能追加 = %d, 物攻加成 = %d", minAtk, maxAtk, pMagicData->nSkillAddition, m_stAbility[AttrID::PAtkPer]);
			CChat::sendClient(toPlayer(), "最小物攻 = %d, 最大物攻 = %d, 随机后物理攻击 = %d", minAtk, maxAtk, ret);
		}
	}
	else if (pMagicData->nDamageType == DAMAGETYPE_MAGIC) {
		int minAtk = static_cast<int>(floor(m_stAbility[AttrID::MinMAtk] + pMagicData->nSkillAddition * (1 + m_stAbility[AttrID::MAtkPer] / Denom)));
		int maxAtk = static_cast<int>(ceil(m_stAbility[AttrID::MaxMAtk] + pMagicData->nSkillAddition * (1 + m_stAbility[AttrID::MAtkPer] / Denom)));
		if (maxAtk <= minAtk) {
			minAtk = maxAtk + minAtk;
			maxAtk = minAtk - maxAtk;
			minAtk = minAtk - maxAtk;
		}
		ret = _random(minAtk, maxAtk);
		if (sConfigMgr.IsBaseValDebug() && isPlayer()) {
			CChat::sendClient(toPlayer(), "%s, 攻击值计算", getName());
			CChat::sendClient(toPlayer(), "技能id = %d, 技能名称 = %s, 技能等级 = %d", pMagicData->nID, pMagicData->szName, pMagicData->btlevel);
			CChat::sendClient(toPlayer(), "魔攻下限 = %d, 魔攻上限 = %d, 技能追加 = %d, 魔攻加成 = %d", minAtk, maxAtk, pMagicData->nSkillAddition, m_stAbility[AttrID::MAtkPer]);
			CChat::sendClient(toPlayer(), "最小魔攻 = %d, 最大魔攻 = %d, 随机后魔法攻击 = %d", minAtk, maxAtk, ret);
		}
	}
	return ret;
}

int CCreature::GetDef(CCreature* pTarget,BYTE btDamageType) {
	int randDef = 0;
	if (pTarget) {
		if (btDamageType == DAMAGETYPE_PHYSICAL) {
			randDef = pTarget->m_stAbility[AttrID::PDef]; // 物防
		}
		else if (btDamageType == DAMAGETYPE_MAGIC) {
			randDef = pTarget->m_stAbility[AttrID::MDef]; // 魔防
		}
	}
	return randDef;
}

int CCreature::GetCounterAttack(const std::shared_ptr<stMagicDataBase>& pMagicData, CCreature* pTarget,int nAttackVal, stInt& TmpDamageType) {
	if (!isPlayer() || !pTarget) return 0;
	if (pMagicData && pMagicData->nDamageType == DAMAGETYPE_PHYSICAL && pMagicData->nID == _MAGIC_Counterattack_) {
		int CounterAtt = 0;
		TmpDamageType.nInt = StrikeBack;
		if (sConfigMgr.IsCounterDebug()) {
			CChat::sendClient(toPlayer(), "%s, 反击攻击计算", getName());
			CChat::sendClient(toPlayer(), "反击攻击力系数 = %d ", m_stAbility[AttrID::CounterCoeff]);
		}
		if (m_stAbility[AttrID::CounterCoeff] > 0) {
			int nDistance = max(abs(m_nCurrX - pTarget->m_nCurrX), abs(m_nCurrX - pTarget->m_nCurrX));	//与被攻击者距离
			int nDistanceAddition = (10 - nDistance) * 0.05;  //距离加成

			int CounterMulPer = 0;  //反击乘区
			int CounterAdd = 0;		//反击和值
			int nTempCounterAtt = 0;// 最终比较值

			if (nDistanceAddition > 0 ) {
				CounterMulPer = nDistanceAddition;
				CounterAdd = m_stAbility[AttrID::CounterInc] - pTarget->m_stAbility[AttrID::CounterInc];
				CounterAtt = static_cast<int>(ceil(CounterMulPer * nAttackVal + CounterAdd));
				nTempCounterAtt = static_cast<int>(ceil(0.03 * nAttackVal));

				if (CounterAtt < nTempCounterAtt) {
					CounterAtt = nTempCounterAtt;
				}
			}
			if (sConfigMgr.IsCounterDebug()) {
				CChat::sendClient(toPlayer(), "物理攻击值 = %d, 距离 = %d,距离加成 = %d ", nAttackVal, nDistance, nDistanceAddition);
				CChat::sendClient(toPlayer(), "反击乘区 = %d,反击和值 = %d，计算反击攻击力 = %d 比较值 =%d", CounterMulPer, CounterAdd, static_cast<int>(ceil(CounterMulPer * nAttackVal + CounterAdd)), nTempCounterAtt);
				CChat::sendClient(toPlayer(), "final 反击攻击力 = %d", CounterAtt);
			}
		}
		return CounterAtt;
	}
	return 0;
}

int CCreature::PassiveMagicAddAttack(const std::shared_ptr<stMagicDataBase>& pMagicData,int nAttackVal) {
	if (pMagicData && nAttackVal && pMagicData->nDamageType == DAMAGETYPE_PHYSICAL) {
		if (isPlayer()) {
			int tempAttack = 0;
			if (toPlayer()->GetJobType() == GUN_JOB) {
				tempAttack = toPlayer()->DoContinuousFiringMagic(nAttackVal);
			}
			if (toPlayer()->GetJobType() == BOXER_JOB) {
				tempAttack = toPlayer()->DoExcitedMagic(nAttackVal);
			}
			return tempAttack;
		}
	}
	return 0;
}

void CCreature::calculateLuaSpecialAbility(stSpecialAbility* abi) {
	FUNCTION_BEGIN;
	if (abi) {
		abi->Clear();
		CUserEngine::getMe().m_specialAbilityKeys.forEach([&](const std::string& key)
			{
				DWORD effectId = quest_vars_get_var_n(key);
				if (effectId > 0) {
					if (auto pEffect = sJsonConfig.GetSpecialEffectDataById(effectId)) {
						abi->Add(*pEffect);
					}
				}
			});
	}
}

void CCreature::calculateBuffAbility()
{
	m_cBuff.calculateBuffAbility();
}
