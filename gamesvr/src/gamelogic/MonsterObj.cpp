#include "MonsterObj.h"
#include "UsrEngn.h"
#include "Chat.h"
#include "Config.h"
#include "timeMonitor.h"
#include "MapMagicEvent.h"
#include "MapItemEvent.h"
#include <utils/Random.hpp>
bool  CMonster::Operate(stBaseCmd* pcmd,int ncmdlen,stProcessMessage* pMsg){
	FUNCTION_BEGIN;
	bool boRet=false;
	switch (pcmd->value)
	{
	case 0:
		{

		}
		break;
	case stCretOtherMove::_value:
		{
			_CHECK_PACKAGE_LEN(stCretOtherMove,ncmdlen);
			stCretOtherMove* pDstCmd=(stCretOtherMove*)pcmd;
			if (pDstCmd->btMoveType!=emMove && pDstCmd->btMoveType!=emMove_Run && pDstCmd->btmovesetp>0){
				stCretOtherMove targetcmd;
				targetcmd=*pDstCmd;
				stCretMoveRet retcmd;
				m_moveStep = pDstCmd->btmovesetp;
				fullMoveRet(&retcmd,0);
				retcmd.dir=pDstCmd->dir;
				retcmd.btMoveType=pDstCmd->btMoveType;
				int nsetp=retcmd.btmovesetp;
				int ncux=m_nCurrX;
				int ncuy=m_nCurrY;
				int ndx=0,ndy=0;
				bool boSendDelay=true;
				if (nsetp>0){
					CCreature* pCretOne=NULL;
					if(GetEnvir()->GetNextPosition(ncux,ncuy,retcmd.dir,1,ndx,ndy) && GetEnvir()->CanWalk( this,ndx,ndy,m_nCurrZ,true)){
						pCretOne=GetEnvir()->GetCretInXY(ndx,ndy);
					}else{
						boSendDelay=false;
						retcmd.moveerrorcode=-2;
					}
					if(pDstCmd->btMoveType==emCollision ){
						if(pCretOne){
							if(GetEnvir()->GetNextPosition(ncux,ncuy,retcmd.dir,2,ndx,ndy) && GetEnvir()->CanWalk( this,ndx,ndy,m_nCurrZ,true)){
								CCreature* pCret=GetEnvir()->GetCretInXY(ndx,ndy);
								if(pCret){
									if(m_dwLevel>pCret->m_dwLevel && !pCret->m_boNoByPush && isEnemy(pCret)){
										stCretOtherMove tcmd;      
										tcmd=*pDstCmd;
										tcmd.btmovesetp=1;
										tcmd.btMoveType=emReverse;
										if(GetEnvir()->GetNextPosition(ncux,ncuy,retcmd.dir,3,ndx,ndy) && GetEnvir()->CanWalk( this,ndx,ndy,m_nCurrZ,true)){
											CCreature* pCretThree=GetEnvir()->GetCretInXY(ndx,ndy);
											if(pCretThree){
												boSendDelay=false;
												retcmd.moveerrorcode=-2;
											}else{        
												pCret->pushMsg(this,&tcmd,sizeof(stCretOtherMove));
											}
										}else{
											boSendDelay=false;
											retcmd.moveerrorcode=-2;
										}
									}else{
										boSendDelay=false;
										retcmd.moveerrorcode=-2;
									}
								}
							}else{
								boSendDelay=false;
								retcmd.moveerrorcode=-2;
							}
						}
						if(pCretOne){
							if(m_dwLevel>pCretOne->m_dwLevel && !pCretOne->m_boNoByPush && boSendDelay && isEnemy(pCretOne)){
								stCretOtherMove tcmd;
								tcmd=*pDstCmd;
								tcmd.btmovesetp=1;
								tcmd.btMoveType=emReverse;
								pCretOne->pushMsg(this,&tcmd,sizeof(stCretOtherMove));
								ndx=pCretOne->m_nCurrX;
								ndy=pCretOne->m_nCurrY;
							}else{
								boSendDelay=false;
								retcmd.moveerrorcode=-2;
							}
						}
					}
					ncux=ndx;
					ncuy=ndy;
					nsetp--;
				}
				if (boSendDelay){
					int i = 1;
					int moveStep = 0;
					for (i = 1; i <= retcmd.btmovesetp; i++)
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
					if (GetEnvir()->MoveCretTo(this,ndx,ndy,m_nCurrZ,true)){
						targetcmd.btmovesetp= moveStep;
						int delaysec = CALL_LUARET<int>("movestepsec", 360 * moveStep, moveStep);
						m_cBuff.AddBuff(1002, 1, delaysec);
						retcmd.location.ncurx=m_nCurrX;
						retcmd.location.ncury=m_nCurrY;
						retcmd.location.ncurz=m_nCurrZ;
						retcmd.dir=(targetcmd.btMoveType==emReverse)?CGameMap::GetReverseDirection(retcmd.dir):retcmd.dir;
						retcmd.btmovesetp= moveStep;
						retcmd.moveerrorcode=0;
					}else{
						boSendDelay=false;
						retcmd.moveerrorcode=(BYTE)-3;
					}
				}
				if(boSendDelay)SendRefMsg(&retcmd,sizeof(retcmd));
			}
		}break;
	case stCretActionRet::_value:
		{
			boRet=OnCretAction((stCretActionRet*)pcmd,ncmdlen,pMsg);
		}break;
	case stCretStruckFull::_value:
		{
			boRet=OnCretStruck((stCretStruckFull*)pcmd,ncmdlen);
		}break;
	case stCretBuffFeature::_value:
		{
			_CHECK_PACKAGE_LEN(stCretBuffFeature,ncmdlen);
			FeatureChanged();
		}break;
	default:
		{
			boRet=CCreature::Operate(pcmd,ncmdlen,pMsg);
		}
		break;
	}
	return boRet;
}

bool CMonster::Die()
{
	auto curMap = GetEnvir();
	if (!curMap) return false;
	if (isDie())
		return false;
	if (CCreature::Die())
	{
		if (m_pLastHiter) {
			if (m_pLastHiter->isPet()) {
				if (m_pLastHiter->toPet()->getMaster()->isPlayer()) {
					SetLastHitter(m_pLastHiter->toPet()->getPlayerMaster());
				}
			}
		}
		if (m_pLastHiter && m_pLastHiter->isPlayer()) {
			if (m_pLastHiter->toPlayer()->CanGetMonReward(toMonster())) { // 击杀怪物是否能获得掉落物品
				randomDropItem(m_pLastHiter->toPlayer());
			}
			MonGetReward(m_pLastHiter->toPlayer());
		}
		else {
			randomDropItem();
		}
		TriggerEvent(this, DIE, 1);
		curMap->AddDelayedMonsterSpawn(m_monGenId, 0);
		if (GetMonType() == emMonsterType::FORTRESSBALL){
			MakeGhost(true, __FUNC_LINE__);
		}
		else {
			timer_.AddTimer(2000, [this]() {
				MakeGhost(true, __FUNC_LINE__);
			}, false);
		}
		return true;
	}
	return false;
}

void CMonster::EnterMapNotify(MapObject* obj)
{
	if (obj->GetType() != CRET_PLAYER) return;
	auto dataBase = GetMonsterDataBase();
	stMapCreateCret retcmd;
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
	getShowName(retcmd.szShowName, sizeof(retcmd.szShowName));
	retcmd.dress = getFeatureId(obj);
	retcmd.btCretType = CRET_MONSTER;
	retcmd.n_bo_AllFeature = m_cBuff.BuffFeature();
	retcmd.crettypeid = m_deCretTypeId;
	retcmd.dwMonsterAudioId = dataBase->AudioId;
	if (isBoss() && GetMonsterDataBase()->boOwner && getOwnerId()) {
		if (CPlayerObj* pPlayer = GetEnvir()->GetPlayer(getOwnerId())) {
			CopyString(retcmd.szMasterName, pPlayer->getName());
			retcmd.btType = (pPlayer->m_GroupInfo.dwGroupId > 0) ? 5 : 4;
		}
	}
	CPlayerObj* player = dynamic_cast<CPlayerObj*>(obj);
	if (GetMonType() == emMonsterType::FORTRESSBALL)
	{
		retcmd.btType = 6;
		if (player) {
			std::string colorstr = CALL_LUARET<std::string>("GuildFortressNameColor", "<color=#ff0000>%s</color>", m_guildId > 0 && player->m_GuildInfo.dwGuildId == m_guildId);
			CopyString(retcmd.szShowName, vformat(colorstr.c_str(), retcmd.szShowName));
		}
	}
	obj->SendMsgToMe(&retcmd, sizeof(retcmd));
	AfterSpaceMove(obj);
	if (player && GetNoAutoAtt() == 0)
	{
		SetAttackTarget(player);
	}
}

void CMonster::Update(){
	ULONGLONG thisTick=GetTickCount64();
	if (thisTick>m_dwNextRunTick){
		FUNCTION_BEGIN;
		CCreature::Update();
		if (!isDie()){
			if (GetMonsterDataBase()->BossAiID){
				m_Timer->run();
			}
		}
		thisTick=GetTickCount64();
		if (thisTick>=m_dwNextMoveTick && thisTick>=m_dwNextHitTick){
			RunMonsterAI();
		}
		m_dwNextRunTick = GetTickCount64() + m_nRunIntervalTick + 200;

	}
}

CMonster::CMonster(uint8_t type, PosType x, PosType y, const std::shared_ptr<stMonsterDataBase>& monInfo, uint32_t mon_gen_id,DWORD dwTmpId)
	: CCreature(monInfo->name, type, x, y), m_monsterId(monInfo->nID)
{
	FUNCTION_BEGIN;
	m_monGenId = mon_gen_id;
	auto dataBase = GetMonsterDataBase();
	m_szMonsterShowName[0] = 0;
	m_dwLevel = dataBase->nLevel;
	m_nViewRangeX = dataBase->ViewRange;
	m_nViewRangeY = dataBase->ViewRange;
	m_btDirection = (dataBase->MonDir == DR_MID) ? _random(7) : dataBase->MonDir;
	m_nRunIntervalTick = 50;
	m_deCretTypeId = dataBase->nID;
	m_dwScriptId = 0;
	m_boImmunePoisoning = dataBase->ImmunePoisoning;
	m_boNoByPush = dataBase->boNoByPush;
	m_boNoParalysis = dataBase->boNoParalysis;
	m_curAttTarget = nullptr;
	m_dwOwnerShipId = 0;
	m_dwNextOwnerShipTime = 0;
	m_dwOwnerShipIntervalTick = 10 * 1000;
	m_dwDifficultyCof = 0;
	m_btType = (emMonsterType)dataBase->monster_type;
	m_dwLastCheckAggroTick = 0;
	m_nLastPetrifactionTime = 0;
	m_btZhongJiLvl = 0;
	m_guildId = 0;
	InitAiConfig();
	InitEvent();
}

CMonster::~CMonster(){
	FUNCTION_BEGIN;
	ClearMsg();
}
void CMonster::InitAiConfig()
{
	auto dataBase = GetMonsterDataBase();
	m_AiCfg.nMonType = 0;
	m_AiCfg.bitNoAutoAtt = dataBase->ActiveAttack ? 0 : 1;
	m_AiCfg.bitNoAttack = dataBase->CanAttack ? 0 : 1;
	m_AiCfg.bitCanNotMove = dataBase->CanMove ? 0 : 1;
	m_AiCfg.bitFlytoTarget = dataBase->CanFly ? 1 : 0;
	m_AiCfg.bitWondering = true;
	m_AiCfg.bitGoHome = true;
}
void CMonster::AddMonsterSkills() {
	auto dataBase = GetMonsterDataBase();
	if (!dataBase->SkillNum.empty())
	{
		for (const auto& [magicId,lv] : dataBase->SkillNum) {
			if (magicId==0) continue;
			m_cMagic.addskill(magicId, lv);
		}
	}
}

void CMonster::SendRefMsg(void* pcmd,int ncmdlen,bool exceptme){
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true,"");
	FUNCTION_MONITOR(48,"");
	CCreature::SendRefMsg(pcmd,ncmdlen,exceptme);
}

void CMonster::GotoHomeXY()
{
	FUNCTION_BEGIN;
	if (GetTickCount64()>m_dwNextMoveTick)
	{
		if ((m_nCurrX!=m_nHomeX)||(m_nCurrY!=m_nHomeY)) 
		{

			// 离开视野后闪现
			if (m_curAttTarget && m_curAttTarget->isPlayer() && ChebyshevDistance(m_curAttTarget) >= max(m_curAttTarget->m_nViewRangeX, m_curAttTarget->m_nViewRangeY) + 5)
			{
				m_isGoingHome = false;
				LocalMapTransfer(m_nHomeX, m_nHomeY, 1, true);
				LoseTarget();
				StatusValueChange(stCretStatusValueChange::hp, m_stAbility[AttrID::MaxHP], __FUNC_LINE__);
				StatusValueChange(stCretStatusValueChange::mp, m_stAbility[AttrID::MaxMP], __FUNC_LINE__);
				return;
			}
			int wantdir = GetEnvir()->GetNextDirection(m_nCurrX, m_nCurrY, m_nHomeX, m_nHomeY);
			bool bowalkok = CCreature::MoveTo(wantdir, 1, m_nHomeX, m_nHomeY, true);
			if (bowalkok && ChebyshevDistance(m_nHomeX, m_nHomeY) <= 1) {
				LoseTarget();
				m_isGoingHome = false;
				StatusValueChange(stCretStatusValueChange::hp, m_stAbility[AttrID::MaxHP], __FUNC_LINE__);
				StatusValueChange(stCretStatusValueChange::mp, m_stAbility[AttrID::MaxMP], __FUNC_LINE__);
			}			
			m_dwNextMoveTick = GetTickCount64() + (bowalkok ? (m_dwMoveIntervalTime) : (m_dwMoveIntervalTime * 2 + _random(5000)));
		}
	}
}

bool CMonster::MonsterTryWalk(int wantdir, int nsetp, int nx, int ny, bool boNotCheckObj){
	FUNCTION_BEGIN;
	if (nsetp==0){return false;}
	int oldx=m_nCurrX;
	int oldy=m_nCurrY;
	MoveTo(wantdir,nsetp,nx,ny,boNotCheckObj);
	int rand=wantdir;
	for (int i=DRI_UP;i<DRI_NUM;i++){
		if ( (oldx==m_nCurrX)&&(oldy==m_nCurrY) ){
			rand++;
			rand = rand % 7;
			if (rand!=wantdir || nsetp > 1){
				MoveTo(rand,1,nx,ny,boNotCheckObj);
			}
		}else{
			m_dwNextHitTick = max(m_dwNextHitTick, GetTickCount64()) + m_dwHitIntervalTime;  //移动后 限定下次攻击时间 以保证前端可以将移动的动作播放完毕
			m_dwNextCastTick = max(m_dwNextCastTick, GetTickCount64()) + m_dwMoveIntervalTime;
			return true;
		}
	}
	return false;
}

void CMonster::RunNoTargetAI(bool boNoActive){
	if (!CanWalk()) return;
	auto dataBase = GetMonsterDataBase();
	if (dataBase->ActivitiesRange >= 1 && m_AiCfg.bitWondering != 0) {
		if (GetTickCount64() > m_dwNextMoveTick) {
			int wantdir = _random(7);
			bool bowalkok = TryWalk(wantdir);
			m_dwNextMoveTick = GetTickCount64() + (bowalkok ? (m_dwMoveIntervalTime * 4) : (m_dwMoveIntervalTime * 8 + _random(5000)));
		}
	}
}

void CMonster::RunTargetAI(){
	FUNCTION_BEGIN;
	if (m_AiCfg.bitNoStruckAtt != 0) return;
	auto dataBase = GetMonsterDataBase();
	if (m_curAttTarget && GetEnvir()==m_curAttTarget->GetEnvir() && !m_curAttTarget->isDie()){
		int targetX = m_curAttTarget->m_nCurrX, targetY = m_curAttTarget->m_nCurrY;
		if ((abs(m_nCurrX-targetX)>dataBase->CastingRange) || (abs(m_nCurrY-targetY)>dataBase->CastingRange)){
			if (CanWalk()){
				if (GetTickCount64()>m_dwNextMoveTick){
					//g_logger.debug("怪物%s在施法范围外移动",getShowName());
					if (m_AiCfg.bitFlytoTarget==0){
						int dir=GetEnvir()->GetNextDirection(m_nCurrX,m_nCurrY,targetX,targetY);
						bool bowalkok=MonsterTryWalk(dir,1,targetX,targetY,false);				
						m_dwNextMoveTick=GetTickCount64()+(bowalkok?(m_dwMoveIntervalTime):(m_dwMoveIntervalTime*2+_random(2000)));
					}else{
						bool bowalkok=LocalMapTransfer(targetX,targetY,dataBase->ViewRange,false);
						if (!bowalkok){
							int dir=GetEnvir()->GetNextDirection(m_nCurrX,m_nCurrY,targetX,targetY);
							bowalkok=MonsterTryWalk(dir,1,targetX,targetY,false);		
						}
						m_dwNextMoveTick=GetTickCount64()+(bowalkok?(m_dwMoveIntervalTime):(m_dwMoveIntervalTime*2+_random(2000)));
					}
				}
			}else{
				LoseTarget();
			}
		}
		if ((abs(m_nCurrX-targetX)<=dataBase->CastingRange) && (abs(m_nCurrY-targetY)<=dataBase->CastingRange)){
			if (CanHit()){
				stMagic* Magic = m_cMagic.randomattackskill();
				if (Magic){
					if (Magic->GetMagicDataBase()->btAttackType==NEARLY_ATTACK){
						if ((abs(m_nCurrX-targetX)>dataBase->AttackRange) || (abs(m_nCurrY-targetY)>dataBase->AttackRange)){
							if (CanWalk()){
								if (GetTickCount64()>m_dwNextMoveTick){
									//g_logger.debug("怪物%s在攻击范围外移动",getShowName());
									if (m_AiCfg.bitFlytoTarget==0){
										int dir=GetEnvir()->GetNextDirection(m_nCurrX,m_nCurrY,targetX,targetY);
										bool bowalkok=MonsterTryWalk(dir,1,targetX,targetY,false);
										m_dwNextMoveTick=GetTickCount64()+(bowalkok?(m_dwMoveIntervalTime):(m_dwMoveIntervalTime*2+_random(2000)));
									}else{
										bool bowalkok=LocalMapTransfer(targetX,targetY,dataBase->ViewRange,false);
										if (!bowalkok){
											int dir=GetEnvir()->GetNextDirection(m_nCurrX,m_nCurrY,targetX,targetY);
											bowalkok=MonsterTryWalk(dir,1,targetX,targetY,false);		
										}
										m_dwNextMoveTick=GetTickCount64()+(bowalkok?(m_dwMoveIntervalTime):(m_dwMoveIntervalTime*2+_random(2000)));
									}
								}
							}
						}
						else if (m_nCurrX == targetX && m_nCurrY == targetY)
						{
							if (!AttackTarget(Magic, dataBase->CastingRange)) {//同坐标如果施法不成功 移动
								if (CanWalk()) {
									if (GetTickCount64() > m_dwNextMoveTick) {
										if (m_AiCfg.bitFlytoTarget == 0) {
											int wantdir = _random(7);
											TryWalk(wantdir);
										}
									}
								}
							}
						}
						if ((abs(m_nCurrX-targetX)<=dataBase->AttackRange) && (abs(m_nCurrY-targetY)<=dataBase->AttackRange)){
							//g_logger.debug("怪物%s在攻击玩家 自己坐标X:%d 坐标Y:%d 目标坐标X:%d 坐标Y:%d 攻击范围:%d",getShowName(),m_nCurrX,m_nCurrY,targetX,targetY,dataBase->AttackRange);
							AttackTarget(Magic,dataBase->AttackRange);
						}
					}
					else {
						//g_logger.debug("怪物%s在施法攻击玩家",getShowName());
						if (!AttackTarget(Magic, dataBase->CastingRange)) {//如果施法不成功 移动
							int dir = GetEnvir()->GetNextDirection(m_nCurrX, m_nCurrY, targetX, targetY);
							bool bowalkok = MonsterTryWalk(dir, 1, targetX, targetY, false);
							m_dwNextMoveTick = GetTickCount64() + (bowalkok ? (m_dwMoveIntervalTime) : (m_dwMoveIntervalTime * 2 + _random(2000)));
						}
					}
				}
				else {
					//g_logger.debug("怪物%s魔法为NULL",getShowName());
				}
			}
		}
	}
	else {
		//g_logger.debug("名字:%s 有目标AI 时间:%s 丢失目标",getShowName(),timetostr(time(NULL)));
		LoseTarget();
	}
}

void CMonster::SetAttackTarget(CCreature* pTarget, bool ignoreTarget)
{
	if (pTarget && !pTarget->isPlayer() && !pTarget->isPet())
		return;
	CCreature::SetAttackTarget(pTarget, ignoreTarget);
}
void CMonster::MonsterMoveToHome()
{
	m_isGoingHome = true;
}

void CMonster::MakeGhost(bool delaydispose,const char* ff){
	FUNCTION_BEGIN;
	m_curAttTarget=NULL;
	CCreature::MakeGhost(delaydispose,ff);
}

const char* CMonster::getShowName(char* szbuffer,int nmaxlen){
	FUNCTION_BEGIN;
	if (GetMonType() == emMonsterType::FORTRESSBALL) {
		if (m_guildId == 0){
			strcpy_s(szbuffer, nmaxlen - 1, vformat("占领军团：无"));
			return szbuffer;
		}
		auto guildit = CUserEngine::getMe().m_mGuilds.find(m_guildId);
		if (guildit != CUserEngine::getMe().m_mGuilds.end()) {
			strcpy_s(szbuffer, nmaxlen - 1, ("占领军团：" + std::string(guildit->second.szGuildName)).c_str());
			return szbuffer;
		}
	}
	if (m_szMonsterShowName[0]==0){
		return CCreature::getShowName(szbuffer,nmaxlen);
	}else{
		strcpy_s(szbuffer,nmaxlen-1,m_szMonsterShowName);
		return szbuffer;
	}
}

int CMonster::getFeatureId(MapObject* obj) {
	if (obj && GetMonType() == emMonsterType::FORTRESSBALL){
		if (CPlayerObj * pPlayer = dynamic_cast<CPlayerObj*>(obj)) {
			if (m_guildId > 0 && pPlayer->m_GuildInfo.dwGuildId == m_guildId) return 500004; //友方
		}
		return 500003;	//敌方
	}
	return GetMonsterDataBase()->feature_id;
}

void CMonster::GetBaseProperty(){
	FUNCTION_BEGIN;
	m_stBaseAbility.Clear();
	m_stBaseAbility += *GetMonsterDataBase();
}

bool CMonster::MoveTo(int nDir, int nmovesetp, int ncx, int ncy, bool boNotCheckObj)
{
	if (ChebyshevDistance(m_nHomeX, m_nHomeY) > GetMonsterDataBase()->ActivitiesRange)
	{
		MonsterMoveToHome();
		return false;
	}
	return CCreature::MoveTo(nDir, nmovesetp, ncx, ncy, boNotCheckObj);
}

void CMonster::DoChangeProperty(stARpgAbility& abi,bool boNotif,const char* ff){
	FUNCTION_BEGIN;
	abi = m_stBaseAbility;
	abi+=m_stBuffAbility;
	CalculatingSpeed();
}

void CMonster::SelectDropItem(std::shared_ptr<stDropItemDataBase>& pDropItemData, CPlayerObj* pPlayer, std::vector<CItem*>& tmpV) {
	FUNCTION_BEGIN;
	if(pDropItemData){
		uint32_t nDropNum = 0;		// 记录掉落物品数量
		for (auto it = pDropItemData->dropItemSet.begin(); it != pDropItemData->dropItemSet.end(); it++) {
			if (pDropItemData->dwMaxDropCount > 0 && nDropNum >= pDropItemData->dwMaxDropCount) break; // 超过最大掉落数量,0不限制
			if (auto& drop = *it; RandomFloatNorm() <= drop.ra)  // 判断掉落概率
			{
				if (drop.id > 0) {
					if (auto item = CItem::CreateItem(drop.id, _CREATE_MON_DROP, drop.co, 0, __FUNC_LINE__, 0, "", ""))
					{
						tmpV.push_back(item);
					}
					nDropNum++;	
				}

				// 检查子掉落数据，和上面的掉落不影响，上面掉落，子掉落表也会去掉落
				auto subDropData = drop.GetSubDropData();
				if (subDropData && pPlayer && (drop.job == 0 || drop.job == pPlayer->GetJobType())) {
					if (!subDropData->szSubDropItems.empty()) {
						DWORD totalWeight = subDropData->weight;
						if (totalWeight == 0 || subDropData->dwSubMaxDropCount == 0) continue;
						DWORD dwDropCount = _random_d(subDropData->dwSubMaxDropCount)+1;
						for (size_t i = 1; i <= dwDropCount; i++)
						{
							DWORD dwRndDropWeight = _random_d(totalWeight); // 随机掉落权重
							DWORD dwDropWeight = 0;
							for (size_t i = 0; i < subDropData->szSubDropItems.size(); ++i)
							{
								auto itemDataBase = subDropData->m_itemDataBases[i].GetItemDataBase();
								if (!itemDataBase) continue;
								dwDropWeight += subDropData->szSubDropItems[i][1];
								/*if (itemDataBase->nRare == RARE_TYPE_BLUE) {
									dwDropWeight = dwDropWeight * (1 + pPlayer->m_stAbility[AttrID::BlueEquipRate] / 10000.0f);
								}
								if (itemDataBase->nRare == RARE_TYPE_GOLD) {
									dwDropWeight = dwDropWeight * (1 + pPlayer->m_stAbility[AttrID::GoldEquipRate] / 10000.0f);
								}*/
								if (dwRndDropWeight > dwDropWeight) continue; // 随机掉落权重超出当前权重区间，跳出继续
								if (auto item = CItem::CreateItem(itemDataBase->nID, _CREATE_MON_DROP, 1, 0, __FUNC_LINE__, 0, "", ""))
								{
									tmpV.push_back(item);
								}
								break;
							}
						}
					}
				}
				
			}
		}
	}
}

bool CMonster::randomDropItem(CPlayerObj* player){
	FUNCTION_BEGIN;
	auto curMap = GetEnvir();
	if (!curMap) {
		return false;
	}
	auto pMoninfo = GetMonsterDataBase();
	auto pDropItemData = sJsonConfig.GetDropDataBase(m_monsterId);
	if (pDropItemData){
		std::vector<CItem*> vDropItems;
		SelectDropItem(pDropItemData, player, vDropItems);
		if (!vDropItems.empty()) {
			std::vector<Point> dropPoints;
			curMap->GetSpiralDropPosition(m_nCurrX, m_nCurrY, vDropItems.size(), dropPoints);

			for (size_t i = 0; i < dropPoints.size(); ++i)
			{
				CItem* pItem = vDropItems[i];
				if (!pItem) {
					continue;
				}
				int nx = dropPoints[i].x, ny = dropPoints[i].y;
				CMapItemEvent* pItemEvent = CLD_DEBUG_NEW CMapItemEvent(pItem, nx, ny);
				__int64 i64OnlyId = player ? player->m_i64UserOnlyID : 0;
				pItemEvent->Init(GetEnvir(), pMoninfo->boOwner ? i64OnlyId : 0, GetObjectId(), OWNER_MONSTER);
				pItemEvent->SetTime(time(NULL), pItem->GetDisappearTime());
				if (!GetEnvir()->AddItemToMap(nx, ny, pItemEvent)) {
					g_logger.error("地面物品添加失败 %s (%d,%d)", pItem->GetItemName(), nx, ny);
					pItem->SetItemLog("mondropaddfail", this, NULL, false, pItem->GetItemCount());
					SAFE_DELETE(pItemEvent);
				}
			}
		}
	}
	return true;
}

int CMonster::GetPetCastingRange(){
	return GetMonsterDataBase()->CastingRange;
}

int CMonster::GetPetAttackRange() {
	return GetMonsterDataBase()->AttackRange;
}

bool CMonster::MonGetReward(CPlayerObj* player){
	FUNCTION_BEGIN;
	auto pMonInfo = GetMonsterDataBase();
	if (pMonInfo && player){
		int AverageExp=0;
		bool boHiterGet = player->CanGetMonReward(this); //是否能获得怪物奖励
		if (player->m_GroupInfo.dwGroupId){
			player->GroupMonGetReward(this, m_dwLevel, GetEnvir(), m_nCurrX, m_nCurrY, boHiterGet);
			
		}else {
			if (GetEnvir()==player->GetEnvir()){
				if (boHiterGet) {
					std::string str = "kill-";
					str += this->getName();
					AverageExp = pMonInfo->CanHaveExp;
					AverageExp = AverageExp * (1 + player->m_stAbility[AttrID::ExpMul] / 10000.0f);
					player->ResChange(ResID::exp, AverageExp, str.c_str());
				}
				player->KillMonster(this);
			}
		}
		if (boHiterGet) {
			// 战斗经验
			auto nBattleExp = pMonInfo->nBattleExp;
			if (player->m_stAbility[AttrID::BattleExpMul]) {
				nBattleExp = nBattleExp * player->m_stAbility[AttrID::BattleExpMul] / 100.0f;
			}
			player->ResChange(ResID::battleExp, nBattleExp, "mondie");
			// 市民经验
			player->ResChange(ResID::citizenExp, pMonInfo->nCitizenValue, "mondie");
			int nNum = _random(pMonInfo->nMaxGame, pMonInfo->nMinGame);
			if (nNum > 0)
				player->ResChange(ResID::game, nNum, "monsterdrop");
		}
		if(m_vAtkPlayer.size()){
			for(std::vector<DWORD>::iterator it=m_vAtkPlayer.begin();it!=m_vAtkPlayer.end();it++){
				DWORD dwPlayerTempId=*it;
				if(dwPlayerTempId && dwPlayerTempId != player->GetObjectId()){
					CPlayerObj* pAtkPlayer= GetEnvir()->GetPlayer(dwPlayerTempId);
					if(pAtkPlayer)
					{
						if(pAtkPlayer->m_GroupInfo.dwGroupId && pAtkPlayer->m_GroupInfo.dwGroupId==player->m_GroupInfo.dwGroupId){
							continue;
						}else{
							std::string str = "kill-" ;
							str += this->getName();
							if (GetEnvir()==pAtkPlayer->GetEnvir()){
								if (pAtkPlayer->CanGetMonReward(this)) {
									AverageExp = pMonInfo->CanHaveExp;
									AverageExp = AverageExp * (1 + player->m_stAbility[AttrID::ExpMul] / 10000.0f);
									pAtkPlayer->ResChange(ResID::exp, AverageExp, str.c_str());
								}
								pAtkPlayer->KillMonster(this);
							}
						}
					}
				}
			}
		}
	}
	return true;
}

bool CMonster::CanHit(){
	return m_AiCfg.bitNoAttack==0 && CCreature::CanHit();
}
bool CMonster::CanWalk(){
	return m_AiCfg.bitCanNotMove==0 && CCreature::CanWalk();
}
bool CMonster::CanRun(){
	return CCreature::CanRun();
}
bool CMonster::CanMagic(){
	return CCreature::CanMagic();
}

bool CMonster::GetAttackDir(CCreature* Target,int nRange,int &nDir){
	FUNCTION_BEGIN;
	if ( GetEnvir()==Target->GetEnvir() && (abs(m_nCurrX-Target->m_nCurrX)<=nRange)&& (abs(m_nCurrY-Target->m_nCurrY)<=nRange)){
		bool success = false;
		if( m_nCurrY==Target->m_nCurrY && m_nCurrX==Target->m_nCurrX){
			if(TryWalk(DRI_UP , false))
				success = false;
		}else{
			success = true;
		}
		if(success){
			nDir=GetEnvir()->GetNextDirection(m_nCurrX,m_nCurrY,Target->m_nCurrX,Target->m_nCurrY);
			return true;
		}
	}
	return false;
}

bool CMonster::AttackTarget(stMagic* pMagic,int nRange)
{
	FUNCTION_BEGIN;
	bool boRet=false;
	if (m_curAttTarget)
	{
		int nDir=0;
		if (GetTickCount64()>m_dwNextHitTick){
			//能攻击则检查自己与目标距离如果太近  没有近身攻击则后退再攻击 
			if (GetAttackDir(m_curAttTarget,nRange,nDir)|| pMagic->GetMagicDataBase()->btAttackType == FAR_ATTACK){
				//近身攻击
				if (pMagic){
					if (!MonsterAttack(m_curAttTarget, pMagic, nDir)) {
						return false;
					}
				}
				m_dwNextHitTick=GetTickCount64()+m_dwHitIntervalTime;
				m_dwLoseTargetTick=GetTickCount64()+ LOST_TARGET_INTERVAL;
				boRet=true;
			}
		}
	}
	return boRet;
}
void CMonster::RunMonsterAI(){
	FUNCTION_BEGIN;
	FUNCTION_MONITOR(17,vformat("CMonster::RunMonsterAI()--%s",this->getName()));
	if (isDie()) {
		return;
	}

	if (m_isGoingHome)
		GotoHomeXY();
	else {
		if (m_curAttTarget == NULL) {
			RunNoTargetAI();
		}
		else{
			RunTargetAI();
		}
	}

	if (m_dwOwnerShipId && GetTickCount64()>m_dwNextOwnerShipTime){
		m_dwOwnerShipId=0;
		if(GetMonsterDataBase()->boOwner){
			NameChanged();
		}
	}
}

bool CMonster::MonsterAttack(CCreature* pTarget, stMagic* pMagic, int nDir,DWORD dwPlayTime,bool boSendAttack){
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true,"");
	bool boRet=false;
	std::vector<CCreature*> vpCret;
	if (pTarget && pMagic && pMagic->GetMagicDataBase()){
		if(!GetMonsterDataBase()->CanDir)nDir=GetMonsterDataBase()->MonDir;
		m_btDirection=nDir;
		stCretAttack attackcmd;
		attackcmd.stTarget.xDes=pTarget->m_nCurrX;
		attackcmd.stTarget.yDes=pTarget->m_nCurrY;
		attackcmd.dwMagicID = pMagic->GetMagicDataBase()->nID;
		attackcmd.stTarget.dwtmpid = m_curAttTarget->GetObjectId();
		attackcmd.dwTmpId = GetObjectId();
		boRet = pMagic->doSkill(this, &attackcmd);
	}
	return boRet;
}

bool CMonster::DamageSpell(__int64 nDamage){
	return true;
}

void CMonster::Struck(CCreature* pHiter){
	FUNCTION_BEGIN;
	if (isPet() && pHiter && pHiter==toPet()->getMaster()){
		pHiter=NULL;
	}
	if (isPet()){
		if (pHiter && m_curAttTarget==NULL){
			SetAttackTarget(pHiter);
		}
	}else{
		if (pHiter){
			m_isGoingHome = false;
			if(!m_curAttTarget || pHiter != m_curAttTarget)
				SetAttackTarget(pHiter);
		}
	}
}

bool CMonster::CalculatingSpeed(){
	auto dataBase = GetMonsterDataBase();
	m_dwHitIntervalTime=std::max<int>((int)600,(int)(dataBase->AttackTick-(m_dwDifficultyCof*2)));
	m_dwCastIntervalTime= std::max<int>((int)600,(int)(dataBase->CastingTick-(m_dwDifficultyCof*2)));
	m_dwMoveIntervalTime=dataBase->MoveTick;
	if (auto cof = m_cBuff.GetBuffCof(MAGICSTATE_SPEEDSLOW)) {
		m_dwMoveIntervalTime = m_dwMoveIntervalTime / (1 - 1.0 * cof / 10000);
	}
	return true;
}

int CMonster::CalculatingRestoreHp(){
	if (GetMonsterDataBase()->BoRestoreHp == 0) {
		return 0;
	}
	return m_stAbility[AttrID::HpRestore];
}

bool CMonster::OnCretStruck(stCretStruckFull* cmd, unsigned int ncmdlen){
	if (ncmdlen < sizeof(stCretStruckFull))
		return false;
	auto curMap = GetEnvir();
	if (!curMap)
		return false;
	auto attacker = curMap->GetCreature(cmd->dwSrcTmpId);
	if (!attacker)
		return false;
	if (!attacker->isDie()) {
		if (!attacker->isEnemy(this)) {
			return true;
		}
	}

	if (auto pMagic = attacker->FindSkill(cmd->dwMagicID))
	{
		pMagic->OnCretStruck(attacker);
	}

	if (GetMonType() == emMonsterType::ORE && cmd->npower > 1) {
		cmd->npower = 1;
	}
	StruckDamage(cmd->npower, attacker);
	cmd->nHp = m_nNowHP;
	if (!isWudi()) SendRefMsg(cmd, sizeof(stCretStruck));
	if (attacker) {
		cmd->btDamageSrc = (attacker->isPlayer()) ? 0 : 1;
	}
	if (!isDie()) {
		if (!m_curAttTarget || isMonster()) {
			if (attacker && !attacker->isDie()) {
				Struck(attacker);
			}
		}
	}
	if (isMonster() || isPet()) {
		DWORD dwSrcAttackId = cmd->dwSrcTmpId;
		if (attacker && attacker->isPet()) {
			if (attacker->toPet()->getMaster()->isPlayer()) {
				dwSrcAttackId = attacker->toPet()->getPlayerMaster()->GetObjectId();
			}
		}
		if (m_nNowHP <= 0 && !m_boHpToZero) {
			m_boHpToZero = true;
			DWORD dwShipId = (m_dwOwnerShipId) ? m_dwOwnerShipId : dwSrcAttackId;
			if (dwShipId) {
				if (CPlayerObj* player = curMap->GetPlayer(dwShipId); player) {
					m_dwLastHiterTmpId = player->GetObjectId();
					SetLastHitter(player);
				}
				else {
					if (CPlayerObj* player = curMap->GetPlayer(dwSrcAttackId); player) {
						m_dwLastHiterTmpId = player->GetObjectId();
						SetLastHitter(player);
					}
				}
			}
		}
		else {
			if (dwSrcAttackId) {
				ULONGLONG thistick = GetTickCount64();
				if (m_dwOwnerShipId != dwSrcAttackId) {
					if (thistick > m_dwNextOwnerShipTime) {
						m_dwOwnerShipId = dwSrcAttackId;
						CPlayerObj* player = curMap->GetPlayer(m_dwOwnerShipId);
						if (player && player->isPlayer()) {
							player->m_dwBossTmpid = GetObjectId();
						}
						if (GetMonsterDataBase()->boOwner) {
							NameChanged();
						}
						m_dwLastHiterTmpId = dwSrcAttackId;
						m_dwNextOwnerShipTime = thistick + m_dwOwnerShipIntervalTick;
					}
				}
				else {
					m_dwNextOwnerShipTime = thistick + m_dwOwnerShipIntervalTick;
				}
			}
		}
		if (dwSrcAttackId && GetMonsterDataBase()->boShareOut) {
			if (auto result = find(m_vAtkPlayer.begin(), m_vAtkPlayer.end(), dwSrcAttackId); result == m_vAtkPlayer.end()) {
				m_vAtkPlayer.push_back(dwSrcAttackId);
			}
		}
	}
	return true;
}

bool CMonster::OnCretAction(stCretActionRet* pcmd, unsigned int ncmdlen, stProcessMessage* param){
	FUNCTION_BEGIN;
	if (ncmdlen >= sizeof(stCretActionRet)) {
		SendRefMsg(pcmd, ncmdlen);
	}
	return true;
}

bool CMonster::isEnemy(CCreature* pTarget/* =NULL */){
	if(!pTarget || this == pTarget) return false;
	if (isMonster()){
		switch (m_btType)
		{
		case emMonsterType::NORMAL:
			{
				if (pTarget->isPlayer() && !pTarget->toPlayer()->isWudi()){
					return CCreature::isEnemy(pTarget);
				} else return false;
			}break;
		default:
			{
				if (!pTarget->isMonster()){
					return CCreature::isEnemy(pTarget);
				}else return false;
			}break;
		}
	}else if (isPet()){
		CCreature* pMaster=toPet()->getMaster();
		if (pMaster){
			CCreature* pTargetMaster=pTarget->isPet()?pTarget->toPet()->getMaster():NULL;
			if(pMaster==pTargetMaster)return false;
			return pMaster->isEnemy(pTarget);
		}else return true;
	}else return false;
}

bool CMonster::AddExp(double nAdd, const char* szopt){
	FUNCTION_BEGIN;
	if (isPet()){
		toPet()->getPetData().nNowExp+=nAdd;
		if (toPet()->getPetData().nMaxExp > 0 && toPet()->getPetData().nNowExp>=toPet()->getPetData().nMaxExp){
			if (toPet()->CanLevelUp(m_dwLevel+1,GetMonsterDataBase()->nID)){
				LevelUp(1);
			}
		}
	}
	return true;
}

bool CMonster::LevelUp(int nAdd, bool bsend){
	FUNCTION_BEGIN;
	return true;
}

int CMonster::GetOreCostEnergy() {
	auto nID = GetMonsterDataBase()->nID;
	auto it = CUserEngine::getMe().m_OreEnergy.find(nID);
	if (it != CUserEngine::getMe().m_OreEnergy.end()) {
		return it->second;
	}
	return 0;
}

bool CMonster::IsSpecialType() const
{
	return m_btType == emMonsterType::ORE;
}

emMonsterType CMonster::GetMonType(){
	return m_btType;
}

std::shared_ptr<stMonsterDataBase> CMonster::GetMonsterDataBase() const
{
	if (auto ptr = m_monsterDataBase.lock()) {
		return ptr;
	}
	auto freshData = sJsonConfig.GetMonsterDataBase(m_monsterId);
	if (!freshData)
	{
		g_logger.error("获取物品效果数据失败, 怪物ID: %d", m_monsterId);
		return nullptr;
	}
	m_monsterDataBase = freshData;
	return freshData;
}
