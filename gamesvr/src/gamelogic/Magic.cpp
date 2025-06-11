#include "magic.h"
#include "MagicExtend.h"
#include "BaseCreature.h"
#include "Buff.h"
#include "UsrEngn.h"
#include "Chat.h"
#include "timeMonitor.h"

//////////////////////////////////////////////////////////////////////////

DWORD __skillmaxlevel(stMagic* pmagic)
{
	FUNCTION_BEGIN;
	int nMaxLevel = 10;//TODO
	//g_logger.debug("技能ID%d 名字%s 最大等级%d",nMaxLevel,pmagic->getShowName(),nMaxLevel);
	return nMaxLevel;
}

std::shared_ptr<stBuffDataBase> stBuff::GetBuffDataBase() const
{
	if (auto ptr = m_buffDataBase.lock()) {
		return ptr;
	}
	auto freshData = sJsonConfig.GetBuffDataBase({ dwBuffID ,m_buffLevel });
	if (!freshData)
	{
		g_logger.error("获取BUFF数据失败, BUFF_ID: %d,Level:%d", dwBuffID, m_buffLevel);
		return nullptr;
	}
	m_buffDataBase = freshData;
	return freshData;
}

stBuff::stBuff(const std::shared_ptr<stBuffDataBase>& buffDataBase): nDura(0), m_buffDataBase(buffDataBase)
{
	dwBuffID = buffDataBase->dwID;
	m_buffLevel = buffDataBase->btLevel;
	bInEffect = true;
	m_addTime = GetTickCount64();
	m_keepTime = 0;
	m_emRunType = BUFF_NEW;
	m_emStateType = static_cast<emMagicStateType>(buffDataBase->btStateType);
	boNotShow = buffDataBase->boShow;
	m_AttakCretType = 0;
	m_AttackCretTmpId = 0;
	m_eventId = 0;
}

uint32_t stBuff::GetBuffId() const
{
	return dwBuffID;
}

BYTE stBuff::GetLevel() const
{
	return m_buffLevel;
}

BYTE stBuff::GetTimeType() const
{
	if (auto buffDataBase = GetBuffDataBase())
	{
		return buffDataBase->btTimeType;
	}
	return 0;
}

const char* stBuff::GetBuffName() const
{
	if (auto buffDataBase = GetBuffDataBase())
	{
		return buffDataBase->szName.c_str();
	}
	return nullptr;
}

long long stBuff::GetTimeLeft() const
{
	int64_t nowtick = GetTickCount64();
	return  nowtick < (m_addTime + m_keepTime) ? (m_addTime + m_keepTime - nowtick) : 0;
}

bool CBUFFManager::AddBuff(DWORD dwBuffID, BYTE btLevel, int64_t dwKpTime, int dura, CCreature* pA){
	FUNCTION_BEGIN;
	if (auto pBuffData = sJsonConfig.GetBuffDataBase({ dwBuffID,btLevel })){
		if (AddBuff(pBuffData, dwKpTime, dura)) {
			if (pBuffData->dwBuffCancel) {
				m_Owner->m_cBuff.RemoveBuff(pBuffData->dwBuffCancel);
			}
			if (pA){
				stBuff* pBuff = m_Owner->m_cBuff.FindBuff(pBuffData->dwID);
				if (pBuff) {
					pBuff->m_AttakCretType = pA->isPlayer() ? OWNER_PLAYER : OWNER_MONSTER;
					pBuff->m_AttackCretTmpId = pA->GetObjectId();
				}
			}
			return true;
		}
	}
	return false;
}

bool CBUFFManager::AddBuff(std::shared_ptr<stBuffDataBase>& buffDataBase, int64_t dwKpTime, int dura){
	FUNCTION_BEGIN;
	if((idMap_.size() >= MAX_NUM) || (m_Owner->isBoss() && buffDataBase->btStateType == MAGICSTATE_PETRIFACTION && m_Owner->toMonster()->m_nLastPetrifactionTime > time(NULL))){return false;}
	for (DWORD i = 0; i < buffDataBase->vMutexBuff.size(); i++) { //互斥buff
		int nBuffID = buffDataBase->vMutexBuff[i];
		auto pBuff = m_Owner->m_cBuff.FindBuff(nBuffID);	
		if (pBuff) {
			return false;
		}
	}
	if (buffDataBase->dwBuffRepel){
		auto pBuff = m_Owner->m_cBuff.FindBuff(buffDataBase->dwBuffRepel);
		if (pBuff)
			return false;
	}
	if (buffDataBase)
	{
		auto buffid = buffDataBase->dwID;
		auto buffLv = buffDataBase->btLevel;
		stBuff* pBuff=FindByBuffID(buffid);
		if (pBuff && buffDataBase->boCover){//覆盖
			pBuff->nDura = dura ? dura : buffDataBase->nDura;
			pBuff->m_addTime = GetTickCount64();
			pBuff->m_keepTime = dwKpTime ? dwKpTime : buffDataBase->i64KeepTime;
			if (!pBuff->boNotShow){
				stCretBuffState retcmd;
				retcmd.dwMagicID=pBuff->GetBuffId();
				retcmd.btBuffOrAction=1;
				retcmd.dwTick= pBuff->m_keepTime/1000;
				retcmd.btLevel = buffLv;
				retcmd.nValue = pBuff->nDura;
				m_Owner->SendMsgToMe(&retcmd,sizeof(stCretBuffState));
			}
			return true;
		}
		else if (pBuff && buffDataBase->boOverlap){//叠加
			pBuff->nDura += dura ? dura : buffDataBase->nDura;
			pBuff->m_keepTime += dwKpTime ? dwKpTime : buffDataBase->i64KeepTime;
			if (!pBuff->boNotShow){
				stCretBuffState retcmd;
				retcmd.dwMagicID=pBuff->GetBuffId();
				retcmd.btBuffOrAction=1;
				retcmd.dwTick= pBuff->m_keepTime / 1000;
				retcmd.btLevel = buffLv;
				retcmd.nValue = pBuff->nDura;
				m_Owner->SendMsgToMe(&retcmd,sizeof(stCretBuffState));
			}
			return true;
		}
		else if (pBuff == nullptr) {//新建
			auto newBuff = std::make_unique<stBuff>(buffDataBase);
			newBuff->m_keepTime = dwKpTime ? dwKpTime : buffDataBase->i64KeepTime;
			newBuff->nDura = dura ? dura : buffDataBase->nDura;
			auto time_type = buffDataBase->btTimeType;
			auto runInterval = [&]()->int64_t {
				if (time_type == TimeOut || time_type == SecondOut) {
					return 200;
				}
				else if (time_type == TimeRun) {
					return buffDataBase->dwTimeInterval;
				}
				else if (time_type == TimeInfinite) {
					return 0;
				}
				}();
			auto* ptr = newBuff.get();
			assert(idMap_.emplace(buffid, std::move(newBuff)).second);
			assert(stateMap_.emplace(ptr->m_emStateType, ptr)->second);
			if (buffDataBase->wFormulaNum){
				char szfunc[QUEST_FUNC_LEN];
				sprintf_s(szfunc,QUEST_FUNC_LEN-1,"%s_%d",BUFF_FORMULA,buffDataBase->wFormulaNum);
				do 
				{
					stAutoSetScriptParam autoparam(m_Owner);
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall(szfunc, "add", m_Owner, buffDataBase, (double)ptr->GetTimeLeft());
				} while (false);
			}

			if (!ptr->boNotShow){
				stCretBuffState retcmd;
				retcmd.dwMagicID=ptr->GetBuffId();
				retcmd.btBuffOrAction=1;
				retcmd.dwTick= ptr->m_keepTime /1000;
				retcmd.btLevel = buffLv;
				retcmd.nValue = ptr->nDura;
				m_Owner->SendMsgToMe(&retcmd,sizeof(stCretBuffState));
			}
			int nNum = 0;
			Exec_BuffLogicFunc(m_Owner, ptr, nNum);
			if (ptr->m_keepTime > 0 || runInterval > 0) {
				ptr->m_eventId = m_event.AddTimerEvent(stTimerEvent::_INTERVALTIMER_NOMAL,
					[buffid, this, buffDataBase](stEventBase* eventbase) {
						auto run_buff = this->FindBuff(buffid);
						if (run_buff) {
							if (!m_Owner->isDie()) {
								auto leftTime = run_buff->GetTimeLeft();
								if (leftTime > 0 || run_buff->m_keepTime == 0) {
									int nNum = 0;
									Exec_BuffLogicFunc(m_Owner, run_buff, nNum);
								}
								else {
									RemoveBuff(run_buff);
								}
							}
							else {
								auto buffDataBase = run_buff->GetBuffDataBase();
								if (buffDataBase)
								{
									if ((m_Owner->isDie() && buffDataBase->boDieRemove) || (buffDataBase->boOfflineRemove)) {
										RemoveBuff(run_buff);
									}
								}
					
							}
						}
					}, &m_event, runInterval, 0xffffffff);
			}
			return true;
		}
	}
	return false;
}

stBuff* CBUFFManager::FindBuff(DWORD dwBuffID){
	return FindByBuffID(dwBuffID);
}

bool CBUFFManager::RemoveBuff(DWORD dwBuffID){
	FUNCTION_BEGIN;
	stBuff* pBuff = FindByBuffID(dwBuffID);
	if (pBuff){
		RemoveBuff(pBuff);
		return true;
	}
	return false;
}

bool CBUFFManager::RemoveBuff(stBuff* pBuff) {
	FUNCTION_BEGIN;
	if (pBuff) {
		auto buffDataBase = pBuff->GetBuffDataBase();
		if (!buffDataBase) return false;
		pBuff->m_emRunType = BUFF_DEL;
		int nNum = 0;
		Exec_BuffLogicFunc(m_Owner, pBuff, nNum);
		if (pBuff->m_eventId > 0) {
			m_event.RemoveEvent(pBuff->m_eventId);
		}
		if (!pBuff->boNotShow) {
			stCretBuffState retcmd;
			retcmd.dwMagicID = pBuff->GetBuffId();
			retcmd.btBuffOrAction = 2;
			retcmd.btLevel = buffDataBase->btLevel;
			m_Owner->SendMsgToMe(&retcmd, sizeof(stCretBuffState));
		}
		if (buffDataBase->wFormulaNum)
		{
			char szfunc[QUEST_FUNC_LEN];
			sprintf_s(szfunc, QUEST_FUNC_LEN - 1, "%s_%d", BUFF_FORMULA, buffDataBase->wFormulaNum);
			stAutoSetScriptParam autoparam(m_Owner);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall(szfunc, "del", m_Owner, buffDataBase);
		}
		auto range = stateMap_.equal_range(pBuff->m_emStateType);
		for (auto rit = range.first; rit != range.second; ++rit) {
			if (rit->second->GetBuffId() == pBuff->GetBuffId()) {
				stateMap_.erase(rit);
				break;
			}
		}
		idMap_.erase(pBuff->GetBuffId());
		return true;
	}
	return false;
}

bool CBUFFManager::RemoveBuff(emMagicStateType StateType){
	FUNCTION_BEGIN;
	std::vector<stBuff*> tmpV;
	auto range = stateMap_.equal_range(StateType);
	for (auto it = range.first; it != range.second; ++it) {
		if (auto buff = it->second) {
			tmpV.push_back(buff);
		}
	}
	for (auto it : tmpV) {
		RemoveBuff(it);
	}
	return true;
}

void CBUFFManager::clear(){
	FUNCTION_BEGIN;
	idMap_.clear();
	for (int i=0;i<MAGICSTATE_MAXCOUNT;i++)
	{
		m_BuffStateList[i] = false;
	}
}

BYTE CBUFFManager::GetBuffLevel(emMagicStateType StateType){
	FUNCTION_BEGIN;
	stBuff* pBuff= FindByStateType(StateType);
	if (pBuff){
		return pBuff->GetLevel();
	}
	return 0;
}

bool CBUFFManager::GetBuffState(emMagicStateType StateType){
	//FUNCTION_BEGIN;
	if (StateType>MAGICSTATE_NULL && StateType<MAGICSTATE_MAXCOUNT){
		return m_BuffStateList[StateType];
	}
	return false;
}
int64_t CBUFFManager::BuffFeature()
{
	int64_t feature = 0;
	if (GetBuffState(MAGICSTATE_SPEEDSLOW)) {
		feature |= stCretFeature::STATE_SpeedSlow;
	}
	if (GetBuffState(MAGICSTATE_PETRIFACTION)) {
		feature |= stCretFeature::STATE_Petrifaction;
	}
	if (GetBuffState(MAGICSTATE_DIZZY)) {
		feature |= stCretFeature::STATE_JinGu;
	}
	if (GetBuffState(MAGICSTATE_MAGICSHIELD)) {
		feature |= stCretFeature::STATE_MagicShield;
	}
	if (GetBuffState(MAGICSTATE_SWORDSHIELD)) {
		feature |= stCretFeature::STATE_SwordShield;
	}
	return feature;
}

int CBUFFManager::GetBuffCof(emMagicStateType StateType){
	FUNCTION_BEGIN;
	stBuff* pBuff=FindByStateType(StateType);
	if (!pBuff) return 0;
	auto buffDataBase = pBuff->GetBuffDataBase();
	if (!buffDataBase) return 0;
	return buffDataBase->wBuffCof;
}
void CBUFFManager::OfflineRemoveBuff() {
	std::vector<stBuff*> delBuff;
	for (auto& [id, buff] : idMap_)
	{
		if (!buff) continue;
		auto buffDataBase = buff->GetBuffDataBase();
		if (!buffDataBase) continue;
		if (buffDataBase->btSave == 0) {
			delBuff.push_back(buff.get());
		}
	}
	for (auto& it : delBuff) {
		this->RemoveBuff(it);
	}
}
bool CBUFFManager::savebuff(char* dest,DWORD& retlen){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	retlen=0;
	if ( maxsize< (sizeof(int)+sizeof(stSaveBuff)) ){ return false;}

	int count=0;
	int len = sizeof(count);

	stSaveBuff TmpSaveBuff;
	for (auto& [id, buff] : idMap_)
	{
		if (!buff) continue;
		auto buffDataBase = buff->GetBuffDataBase();
		if (!buffDataBase) continue;
		if (buffDataBase->btSave == 0) {
			TmpSaveBuff.dwBuffID = buffDataBase->dwID;
			TmpSaveBuff.btLevel = buffDataBase->btLevel;
			TmpSaveBuff.dwCurTime = (buffDataBase->btSave == 2) ? time(NULL) : 0;
			TmpSaveBuff.dwRemainTime = buff->GetTimeLeft();
			if (TmpSaveBuff.dwRemainTime > 0 || buffDataBase->btTimeType == TimeInfinite) {
				memcpy((void*)(dest + len), &TmpSaveBuff, sizeof(stSaveBuff));
				len += sizeof(stSaveBuff);
				count++;
			}
		}
	}
	if (m_Owner->isPlayer() && m_Owner->toPlayer()->m_loadBuffData.empty() == false) {
		for (auto& it : m_Owner->toPlayer()->m_loadBuffData) {
			stLoadBuffData& loadbuffdata = it;
			auto pBuffData = sJsonConfig.GetBuffDataBase({ loadbuffdata.buffid, loadbuffdata.level });
			if (pBuffData && pBuffData->btSave) {
				TmpSaveBuff.dwBuffID = loadbuffdata.buffid;
				TmpSaveBuff.btLevel = pBuffData->btLevel;
				TmpSaveBuff.dwCurTime = (pBuffData->btSave == 2) ? time(NULL) : 0;
				TmpSaveBuff.dwRemainTime = loadbuffdata.timeLeft;
				if (TmpSaveBuff.dwRemainTime > 0 || pBuffData->btTimeType == TimeInfinite) {
					memcpy(&TmpSaveBuff.dwSaveData[0], &loadbuffdata.nDura, sizeof(DWORD));
					memcpy((void*)(dest + len), &TmpSaveBuff, sizeof(stSaveBuff));
					len += sizeof(stSaveBuff);
					count++;
				}
			}
		}
	}

	*((int*)(dest))=count;
	retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen);
	base64_encode(dest,len,pin,retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CBUFFManager::savetmpbuff(char* dest,DWORD& retlen)
{
	FUNCTION_BEGIN;
	int maxsize=retlen;
	retlen=0;
	if ( maxsize< (sizeof(int)+sizeof(stSaveBuff)) ){ return false;}

	int count=0;
	int len = sizeof(count);

	stSaveBuff TmpSaveBuff;
	for (auto&[id,buff]: idMap_)
	{
		if (!buff) continue;
		auto buffDataBase = buff->GetBuffDataBase();
		if (!buffDataBase) continue;
		if (buffDataBase->btSave == 0) {
			TmpSaveBuff.dwBuffID = buffDataBase->dwID;
			TmpSaveBuff.btLevel = buffDataBase->btLevel;
			TmpSaveBuff.dwCurTime = 0;
			TmpSaveBuff.dwRemainTime = buff->GetTimeLeft();
			if (TmpSaveBuff.dwRemainTime > 0 || buffDataBase->btTimeType == TimeInfinite) {
				memcpy((void*)(dest + len), &TmpSaveBuff, sizeof(stSaveBuff));
				len += sizeof(stSaveBuff);
				count++;
			}
		}
	}
	if (m_Owner->isPlayer() && m_Owner->toPlayer()->m_loadBuffData.empty() == false) {
		for (auto& it : m_Owner->toPlayer()->m_loadBuffData) {
			stLoadBuffData& loadbuffdata = it;
			auto pBuffData = sJsonConfig.GetBuffDataBase({ loadbuffdata.buffid, loadbuffdata.level });
			if (pBuffData && pBuffData->btSave == 0) {
				TmpSaveBuff.dwBuffID = loadbuffdata.buffid;
				TmpSaveBuff.btLevel = pBuffData->btLevel;
				TmpSaveBuff.dwCurTime = 0;
				TmpSaveBuff.dwRemainTime = loadbuffdata.timeLeft;
				if (TmpSaveBuff.dwRemainTime > 0 || pBuffData->btTimeType == TimeInfinite) {
					memcpy(&TmpSaveBuff.dwSaveData[0], &loadbuffdata.nDura, sizeof(DWORD));
					memcpy((void*)(dest + len), &TmpSaveBuff, sizeof(stSaveBuff));
					len += sizeof(stSaveBuff);
					count++;
				}
			}
		}
	}

	*((int*)(dest))=count;
	retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen);
	base64_decode((char*)dest,maxsize,pin,retlen);
	memcpy((char*)dest,pin,retlen);
	return true;
}

bool CBUFFManager::loadbuff(const char* dest,int retlen,int nver){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	if (maxsize==0) return true;

	retlen=ROUNDNUMALL(retlen,4)/4*3;
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= CLD_DEBUG_NEW char[retlen];
	ZeroMemory(pin,retlen);
	base64_decode((char*)dest,maxsize,pin,retlen);
	memcpy((char*)dest,pin,retlen);
	maxsize=retlen;

	if (maxsize< (sizeof(int))){ return false;}

	int count= *((int*)(dest));
	int len=sizeof(count);

	int nsavebuffsize=0;

	nsavebuffsize=sizeof(stSaveBuff);

	while (count>0){
		int datalen=safe_max(maxsize-len,0);
		if (datalen>=nsavebuffsize*count){
			stSaveBuff TmpSaveBuff;
			memcpy(&TmpSaveBuff,(void *)(dest+len),nsavebuffsize);
			len += nsavebuffsize;
			int64_t i64Time=0;
			auto pBuffData = sJsonConfig.GetBuffDataBase({ TmpSaveBuff.dwBuffID, TmpSaveBuff.btLevel });
			if (pBuffData){
				if (TmpSaveBuff.dwCurTime){
					if (pBuffData->btTimeType==TimeOut || pBuffData->btTimeType==TimeRun){
						i64Time=(TmpSaveBuff.dwRemainTime)-((time(NULL)-TmpSaveBuff.dwCurTime)*1000);
					}else{
						i64Time=((TmpSaveBuff.dwRemainTime)-((time(NULL)-TmpSaveBuff.dwCurTime)));
						i64Time=i64Time*1000;
					}
					if (!pBuffData->boTimeOutRemove && i64Time<=0){i64Time=1;}
					if (m_Owner && m_Owner->isPlayer() && i64Time > 0) {
						m_Owner->toPlayer()->m_loadBuffData.emplace_back(TmpSaveBuff.dwBuffID, TmpSaveBuff.btLevel, i64Time, 0);
					}
				}else{
					if (pBuffData->btTimeType==TimeOut || pBuffData->btTimeType==TimeRun){
						i64Time=TmpSaveBuff.dwRemainTime;
					}else{
						i64Time=(TmpSaveBuff.dwRemainTime)*1000;
					}
					int nDura = pBuffData->nDura>0?TmpSaveBuff.dwSaveData[0]:0;
					if (m_Owner && m_Owner->isPlayer()) {
						m_Owner->toPlayer()->m_loadBuffData.emplace_back(TmpSaveBuff.dwBuffID, TmpSaveBuff.btLevel, i64Time, nDura);
					}
				}
			}
		}
		else if (datalen!=0){
			return false;
		}
		count--;
	}
	return true;
}

stBuff* CBUFFManager::FindByStateType(emMagicStateType StateType)
{
	auto range = stateMap_.equal_range(StateType);
	for (auto rit = range.first; rit != range.second; ++rit) {
		if (rit->second->m_emStateType == StateType) {
			return rit->second;
		}
	}
	return nullptr;
}

stBuff* CBUFFManager::FindByBuffID(DWORD dwBuffid)
{
	if (auto it = idMap_.find((dwBuffid));it!=idMap_.end())
	{
		return it->second.get();
	}
	return nullptr;
}

bool CBUFFManager::IsControlBuff(BYTE StateType) {
	//FUNCTION_BEGIN;
	if (StateType > MAGICSTATE_NULL && StateType < MAGICSTATE_MAXCOUNT) {
		if (StateType == MAGICSTATE_PETRIFACTION || StateType == MAGICSTATE_DIZZY) {
			return true;
		}
	}
	return false;
}

void CBUFFManager::calculateBuffAbility()
{
	auto& abi = m_Owner->m_stBuffAbility;
	abi.Clear();
	for (auto& [id, buff] : idMap_)
	{
		if (!buff) continue;
		auto buffDataBase = buff->GetBuffDataBase();
		if (!buffDataBase) continue;
		if (auto effect = buffDataBase->GetEffectDataBase()) {
			abi.Add(*effect);
		}
	}
}

std::shared_ptr<stMagicDataBase> stMagic::GetMagicDataBase() const
{
	if (auto ptr = m_magicDataBase.lock()) {
		return ptr;
	}
	auto fresh = sJsonConfig.GetMagicDataBase(savedata.skillid, savedata.level);
	if (!fresh)
	{
		g_logger.error("获取技能数据失败, 技能ID: %d，技能等级：%d", savedata.skillid, savedata.level);
		return nullptr;
	}
	m_magicDataBase = fresh;
	return fresh;
}

std::unique_ptr<stMagic> stMagic::createSkill(DWORD skillid, BYTE level){
	FUNCTION_BEGIN;
	auto pmagicdata=sJsonConfig.GetMagicDataBase(skillid,level);
	if (pmagicdata){ 
		auto pmagic = MagicExtend::CreateSkill(skillid, level, pmagicdata->btMagicBattleType);
		if (pmagic) {
			pmagic->savedata.originLv = level;
			pmagic->savedata.level = level;
			pmagic->savedata.skillid = pmagicdata->nID;
			pmagic->savedata.dwexp = 0;
			pmagic->savedata.boLocked = pmagicdata->boLocked;
			pmagic->savedata.boLockChange = pmagicdata->boLockChange;
			pmagic->savedata.boContinuousCasting = pmagicdata->boContinuousCasting;
			pmagic->savedata.boContinuousCastChange = pmagicdata->boContinuousCastChange;
			pmagic->istmpmagic = false;
		}
		return pmagic;
	}
	return nullptr;
}

bool stMagic::checkCanUse(CCreature* pCret,stCretAttack* pcmd)
{
	int errorCode = 0;
	if (GetMagicDataBase())
	{
		//技能是否可用检测
		if (!errorCode && GetMagicDataBase()->nID < _MIN_MAGICID_) { errorCode = CRET_MAGICFAIL_UNKNOWNMAGIC; }
		if (!errorCode && !isCooling(pCret)) { errorCode = CRET_MAGICFAIL_NOCOOLING; }

		if (!errorCode && pCret->m_nNowMP < GetNeedMp(pCret)) { errorCode = CRET_MAGICFAIL_MAGICLACKING; }
		if (!errorCode && pCret->m_nNowHP < (int64_t)GetMagicDataBase()->dwNeedHp) { errorCode = CRET_MAGICFAIL_NOHP; }
		if (!errorCode && pCret->m_nNowPP < (int64_t)GetMagicDataBase()->dwNeedPp) { errorCode = CRET_MAGICFAIL_NOPP; }
		if (!errorCode && pCret->isDie()) { errorCode = CRET_MAGICFAIL_DEAD; }
		if (!errorCode && !pCret->CanHit()) { errorCode = CRET_MAGICFAIL_CANNOTHIT; }
		if (!errorCode && pcmd && GetMagicDataBase()->btEnemyType == ENEMYTYPE_ENEMY && pCret->isSafeZone(dynamic_cast<CCreature*>(pCret->GetEnvir()->GetObjById(pcmd->stTarget.dwtmpid)))) { errorCode = CRET_MAGICFAIL_SAFEZONE; }
		if (!errorCode && GetMagicDataBase()->btMagicFuncType == emMagicFuncType::MAGICFUNC_ASSIST) {
			if (auto pBuff = pCret->LuaFindBuff(_MAGIC__SOULSTORM_)) //心灵风暴不能使用辅助魔法
				errorCode = CRET_MAGICFAIL_NOASSIST; 
		}
		if (!errorCode && GetMagicDataBase()->nID == _MAGIC_FARPUTONGGONGJI_) { // 远程普攻
			if (pCret->isPlayer()){
				if(pCret->toPlayer()->GetJobType() != emJobType::GUN_JOB)
					errorCode = CRET_MAGICFAIL_CASTNOTSUCCESS;
				else {
					if(!pCret->toPlayer()->m_Packet.FindItemInBodyByPos(EQUIP_WEAPONS))
						errorCode = CRET_MAGICFAIL_CASTNOTSUCCESS;
				}
			}
		}
		if (!errorCode && (GetMagicDataBase()->nID == 1000 || GetMagicDataBase()->nID == 2000))
		{
			if (pCret->isPlayer() && pcmd) {
				if (!pCret->toPlayer()->IsEnoughAttackCostNeng(pcmd))
					errorCode = CRET_MAGICFAIL_NONENG;
			}

		}
	}

	//给前端发送技能攻击回包
	stCretAttackRet retcmd;
	retcmd.btErrorCode = errorCode;
	retcmd.dwtmpid = pCret->GetObjectId();
	retcmd.ncurx = pCret->m_nCurrX;
	retcmd.ncury = pCret->m_nCurrY;
	retcmd.ncurz = pCret->m_nCurrZ;
	retcmd.btDirect = pCret->m_btDirection;
	retcmd.dwMagicID = GetMagicDataBase()->nID;
	retcmd.btMagicLevel = GetMagicDataBase()->btlevel;
	bool canUse = (errorCode == 0) ? true : false;
	if (canUse)
	{
		//技能可用时，通知技能释放者、同场景相关玩家播放技能攻击/施法动画
		pCret->SendRefMsg(&retcmd, sizeof(retcmd));
		//g_logger.warn("stMagic::checkCanUse() -> SendRefMsg(stCretAttackRet* pcmd)：玩家名=%s 技能ID=%d ", pCret->m_szCretName, retcmd.dwMagicID);
	}
	else
	{
		//技能不可用时，仅通知技能释放者显示不可用原因
		pCret->SendMsgToMe(&retcmd, sizeof(retcmd));
		//g_logger.warn("stMagic:checkCanUse()：技能使用条件检测未通过，错误码=%d!", errorCode);
	}

	return canUse;
}

bool stMagic::doSkill(CCreature* pCret, stCretAttack* pcmd)
{
	if (!GetMagicDataBase()){
		return false;
	}
	//更新人物朝向、下次可移动时间、下次可攻击时间
	pCret->m_btDirection = pCret->GetEnvir()->GetNextDirection(pCret->m_nCurrX, pCret->m_nCurrY, pcmd->stTarget.xDes, pcmd->stTarget.yDes);
	if (pCret->isPlayer())
	{
		CPlayerObj* pPlayer = (CPlayerObj*)pCret;
		uint64_t currentTime = GetTickCount64();
		DWORD intervalTime = (GetMagicDataBase()->nDamageType == 1) ? pPlayer->m_dwHitIntervalTime : pPlayer->m_dwCastIntervalTime;
		uint64_t nextTime = currentTime + intervalTime;

		// 根据伤害类型更新对应的保存时间
		if (GetMagicDataBase()->nDamageType == 1) {
			pPlayer->m_dwSaveAttackTimeForNext = nextTime;
		}
		else if (GetMagicDataBase()->nDamageType == 2) {
			pPlayer->m_dwSaveReleaseTimeForNext = nextTime;
		}

		// 更新移动保存时间
		if (!pPlayer->m_cDelayMoveList.empty()) {
			pPlayer->m_dwSaveMoveTimeForNext += intervalTime;
		}
		else {
			pPlayer->m_dwSaveMoveTimeForNext = nextTime;
		}
	}
	//设置CD
	bool boCDTime = false;
	if (isCooling(pCret)) {
		resetUseTime(pCret);
		boCDTime = true;	
	}
	if (pCret->isPlayer() && GetMagicDataBase()->nResetCdCof > 0 && _random_d(10000) < (DWORD)GetMagicDataBase()->nResetCdCof && time(NULL) > pCret->quest_vars_get_var_n(vformat("lastResetCdSec_%d", GetMagicDataBase()->nID))) {
		CALL_LUA("ResetSkillCd", (CPlayerObj*)pCret, GetMagicDataBase()->nID);
		clearUseTime(pCret);
		boCDTime = true;
	}

	auto magicNeedMp = GetNeedMp(pCret);
	pCret->StatusValueChange(stCretStatusValueChange::hp, -GetMagicDataBase()->dwNeedHp, "技能耗蓝");
	pCret->StatusValueChange(stCretStatusValueChange::mp, -magicNeedMp, "技能耗蓝");
	if (pCret->isPlayer()){
		pCret->StatusValueChange(stCretStatusValueChange::pp, -GetMagicDataBase()->dwNeedPp, __FUNC_LINE__);
		if (GetMagicDataBase()->nID == 1000 || GetMagicDataBase()->nID == 2000){
			pCret->toPlayer()->DoAttackCostNeng();
		}
	}

	//通知前端播放技能命中动画
	stCretActionRet Actionretcmd;
	Actionretcmd.dwTempId = pCret->GetObjectId();
	Actionretcmd.dwTargetId = pcmd->stTarget.dwtmpid;
	Actionretcmd.btDir = GetMagicDataBase()->boDir ? GetMagicDataBase()->boDir : (BYTE)-1;
	Actionretcmd.nX = pcmd->stTarget.xDes;
	Actionretcmd.nY = pcmd->stTarget.yDes;
	Actionretcmd.dwMagicId = GetMagicDataBase()->nSkillActionId;
	Actionretcmd.dwActionTick = 0;
	pCret->pushDelayMsg(pCret, &Actionretcmd, sizeof(Actionretcmd), 0);

	//通知前端技能CD
	if (!boCDTime) {
		stSkillCDTime cdtimecmd;
		cdtimecmd.dwMagicId = savedata.skillid;
		cdtimecmd.dwPublicTick = GetMagicDataBase()->boPublicCD ? pCret->GetPublicCDTime() : 0;
		cdtimecmd.dwSelfTick = pCret->GetPublicCDTime();
		pCret->SendMsgToMe(&cdtimecmd, sizeof(cdtimecmd));
	}

	return true;
}

bool stMagic::isCooling(CCreature* pCret){
	FUNCTION_BEGIN;
	if (GetMagicDataBase()){
		DWORD cdTime = GetMagicDataBase()->dwCDbyTime;
		if(pCret && pCret->isPlayer()){
			if (cdTime > 2000) {
				lastUseTimeSec = pCret->quest_vars_get_var_n(vformat("lastUseTimeSec_%d", savedata.skillid));
				if(time(NULL) < lastUseTimeSec){ //调了系统时间,重置一下.
					lastUseTimeSec = 0;
				}
			}
		}
		if (GetTickCount64()>=(lastUseTime+cdTime) && time(NULL)>=(lastUseTimeSec+cdTime/1000)){
			return true;
		}else {
			//	g_logger.error("isCooling %d",GetTickCount64()- (lastUseTime+cdTime));
			return false;
		}
	}
	return false;
}
int stMagic::getleftcd(CCreature* pCret){
	FUNCTION_BEGIN;
	if (GetMagicDataBase()){
		DWORD cdTime = GetMagicDataBase()->dwCDbyTime;
		if(pCret && pCret->isPlayer()){
			if (cdTime > 2000) {
				lastUseTimeSec = pCret->quest_vars_get_var_n(vformat("lastUseTimeSec_%d", savedata.skillid));
			}
		}
		if (time(NULL) >= (lastUseTimeSec + cdTime / 1000))
		{//CD结束了
			return 0;
		}
		int leftcdtime=(lastUseTimeSec+cdTime/1000-time(NULL))*1000;
		leftcdtime=max(0,leftcdtime);
		return leftcdtime;
	}
	return 0;
}

void stMagic::resetUseTime(CCreature* pCret){
	FUNCTION_BEGIN;
	lastUseTime=GetTickCount64();
	lastUseTimeSec = time(NULL);
	DWORD reduceTime = 0;
	if (pCret && pCret->isPlayer() && GetMagicDataBase()){
		if (GetMagicDataBase()->dwCDbyTime) {
			pCret->quest_vars_set_var_n(vformat("lastUseTimeSec_%d", savedata.skillid), lastUseTimeSec, true);
		}
		stSkillCDTime cdtimecmd;
		cdtimecmd.dwMagicId=savedata.skillid;
		if (pCret->m_NoCdMode)
		{
			cdtimecmd.dwPublicTick=0;
			cdtimecmd.dwSelfTick=0;
			lastUseTime = 0;
			lastUseTimeSec=0;
		}
		else
		{
			cdtimecmd.dwPublicTick=GetMagicDataBase()->boPublicCD?pCret->GetPublicCDTime():0;
			cdtimecmd.dwSelfTick=GetMagicDataBase()->dwCDbyTime- reduceTime;
		}
		pCret->SendMsgToMe(&cdtimecmd,sizeof(cdtimecmd));
	}
}
void stMagic::clearUseTime(CCreature* pCret){
	FUNCTION_BEGIN;
	lastUseTime=0;
	lastUseTimeSec=0;
	if(pCret){
		pCret->quest_vars_set_var_n(vformat("lastUseTimeSec_%d",savedata.skillid), 0, true);
		stSkillCDTime cdtimecmd;
		cdtimecmd.dwMagicId = savedata.skillid;
		cdtimecmd.dwPublicTick=0;
		cdtimecmd.dwSelfTick=0;
		pCret->SendMsgToMe(&cdtimecmd,sizeof(cdtimecmd));
	}
}

void stMagic::changeUseTime(int nchange, CCreature* pCret) {
	lastUseTime += nchange * 1000;
	lastUseTimeSec += nchange;
	if (pCret) {
		pCret->quest_vars_set_var_n(vformat("lastUseTimeSec_%d", savedata.skillid), lastUseTimeSec, true);
	}
}

const char* stMagic::getShowName(char* szbuffer,int nmaxlen){
	FUNCTION_BEGIN;
	static char s_szShowNameBuffer[512];
	if(szbuffer==NULL){
		szbuffer=s_szShowNameBuffer;
		nmaxlen=count_of(s_szShowNameBuffer);
	}
	if (szbuffer){	
		strcpy_s(szbuffer,nmaxlen-1,GetMagicDataBase()->szName.c_str()); 
		filtershowname(szbuffer,strlen(szbuffer));
		return szbuffer;
	}
	return GetMagicDataBase()->szName.c_str();
}

int stMagic::getmaxlevel(){
	FUNCTION_BEGIN;
	return __skillmaxlevel(this);
}

bool stMagic::skilllevelup(int nLevel,CMagicManage& cMagic){
	FUNCTION_BEGIN;
	if (nLevel != savedata.originLv){
		auto pmagicdata = sJsonConfig.GetMagicDataBase(savedata.skillid, nLevel);
		if (pmagicdata && cMagic.CanLevelUp(pmagicdata->wSkillNeedLv)) {
			savedata.originLv = nLevel;
			DWORD dwLevel = savedata.originLv + savedata.extraLv;
			DWORD dwMaxLv = __skillmaxlevel(this);
			dwLevel = dwLevel > dwMaxLv ? dwMaxLv : dwLevel;
			if (dwLevel != savedata.level){
				pmagicdata = sJsonConfig.GetMagicDataBase(savedata.skillid, savedata.level);
				if (pmagicdata) {
					m_magicDataBase = pmagicdata;
					cMagic.SendCretSkill(&savedata);
				}
			}
			return true;
		}
	}
	return false;
}

bool stMagic::changeLv(int nIndex, int nLv, CMagicManage& cMagic) {
	FUNCTION_BEGIN;
	if (nIndex == 1){ //改变原始等级
		if (savedata.originLv != nLv) {
			auto pmagicdata = sJsonConfig.GetMagicDataBase(savedata.skillid, nLv);
			if (pmagicdata && cMagic.CanLevelUp(pmagicdata->wSkillNeedLv)) {
				savedata.originLv = nLv;
				DWORD dwLevel = savedata.originLv + savedata.extraLv;
				DWORD dwMaxLv = __skillmaxlevel(this);
				dwLevel = dwLevel > dwMaxLv ? dwMaxLv : dwLevel;
				if (savedata.level != dwLevel) {
					savedata.level = dwLevel;
					pmagicdata = sJsonConfig.GetMagicDataBase(savedata.skillid, savedata.level);
					if (pmagicdata) {
						m_magicDataBase = pmagicdata;
						return true;
					}
				}
			}
		}
	}
	else if(nIndex == 2){ //额外等级
		if (savedata.extraLv != nLv) {
			savedata.extraLv = nLv;
			DWORD dwLevel = savedata.originLv + savedata.extraLv;
			DWORD dwMaxLv = __skillmaxlevel(this);
			dwLevel = dwLevel > dwMaxLv ? dwMaxLv : dwLevel;
			if (savedata.level != dwLevel) {
				savedata.level = dwLevel;
				auto pmagicdata = sJsonConfig.GetMagicDataBase(savedata.skillid, savedata.level);
				if (pmagicdata) {
					m_magicDataBase = pmagicdata;
					return true;
				}
			}
		}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
	} 
	return false;
}

void stMagic::calcSkillExtarLv(const stSpecialAbility& specialAbi, CMagicManage& cMagic) {
	FUNCTION_BEGIN;
	DWORD extraLv = 0;
	//被动和百级技能
	if (GetMagicDataBase()->btActiveType == 2)
	{
		extraLv += specialAbi.nFullSkillLv;
	}
	if (specialAbi.nHundSkillLv){		//百级技能等级
		extraLv += specialAbi.nHundSkillLv;
	}
	switch (savedata.skillid) {	// 单一技能等级
	case 1001: { extraLv += specialAbi.nSteelSkinLv; }break;
	case 1002: { extraLv += specialAbi.nThrillLv; }break;
	case 1003: { extraLv += specialAbi.nCounterpunchLv; }break;
	case 1004: { extraLv += specialAbi.nSelfMedicationLv; }break;
	case 1501: { extraLv += specialAbi.nMuseLv; }break;
	case 1502: { extraLv += specialAbi.nSpiritStrengthenLv; }break;
	case 1503: { extraLv += specialAbi.nMagicRefineLv; }break;
	case 1504: { extraLv += specialAbi.nSpellMasterLv; }break;
	case 2001: { extraLv += specialAbi.nConcentrationLv; }break;
	case 2002: { extraLv += specialAbi.nJuckLv; }break;
	case 2003: { extraLv += specialAbi.nPunctureLv; }break;
	case 2004: { extraLv += specialAbi.nRestoreLv; }break;
	case 2501: { extraLv += specialAbi.nFirearmsMasterLv; }break;
	case 2502: { extraLv += specialAbi.nContinuousFiringLv; }break;
	case 2503: { extraLv += specialAbi.nSnipeLv; }break;
	case 2504: { extraLv += specialAbi.nLivelyLv; }break;
	default:
		break;
	}

	if(changeLv(2, extraLv, cMagic))
		cMagic.SendCretSkill(&savedata);
}

bool stMagic::isTarget(CCreature* pA,CCreature* pT) {
	if (pA && pT)
	{
		auto attackNum = GetMagicDataBase()->wAttackNum;
		// 范围技能判断矿石是否命中
		if (attackNum > 1 && pA->isPlayer() && pT->isMonster() && pA->toPlayer()->getMiningNeng() < pT->toMonster()->GetOreCostEnergy())
			return false;
		BYTE btSkillModel = GetMagicDataBase()->btSkillModel;
		if (btSkillModel == MODELTYPE_NONE || (btSkillModel == MODELTYPE_HUMAN && pT->isPlayer()) || (btSkillModel == MODELTYPE_MON && pT->isMonster())) //判断人怪
		{
			bool bEnemy = pA->isEnemy(pT);
			BYTE btEnemyModel = GetMagicDataBase()->btEnemyType;
			if (GetMagicDataBase()->btEnemyType == ENEMYTYPE_NONE || (!bEnemy && GetMagicDataBase()->btEnemyType == ENEMYTYPE_FRIEND) || (bEnemy && GetMagicDataBase()->btEnemyType == ENEMYTYPE_ENEMY))//判断敌友
			{
				if(attackNum > 1 || (attackNum <= 1 && pA!=pT))
					return true;
			}
		}
	}
	return false;
}

void stMagic::getTargets(CCreature* pCret, stCretAttack* pcmd, std::vector<CCreature*>& vpTarget) {
	std::vector<CCreature*> vCret;		
	if (GetMagicDataBase()->bSelect) { // 释放技能需要选中目标
		CCreature* pSelect = pCret->GetEnvir()->FindCretByTmpId(pcmd->stTarget.dwtmpid, pcmd->stTarget.xDes, pcmd->stTarget.yDes);
		if (pSelect)
		{
			if (GetMagicDataBase()->wAttackNum == 1) {
				vCret.push_back(pSelect);
			}
			else {
				pCret->CalculatingTarget(vCret, this, pcmd->stTarget.dwtmpid, pcmd->stTarget.xDes, pcmd->stTarget.yDes);
			}
		}
	}
	else {
		pCret->CalculatingTarget(vCret, this, pcmd->stTarget.dwtmpid, pcmd->stTarget.xDes, pcmd->stTarget.yDes);
	}
	for (auto it = vCret.begin(); it != vCret.end(); it++){
		if (isTarget(pCret, *it)){
			vpTarget.push_back(*it);
		}
	}
}

int stMagic::GetNeedMp(CCreature* pCret) {
	if (pCret->isMonster() && !pCret->isRobot())
	{
		return 0;
	}
	//扣除技能耗蓝            // nMpCostPer 正值减少蓝耗 负值增加蓝耗
	int magicNeedMp = GetMagicDataBase()->dwNeedMP - (int)(GetMagicDataBase()->dwNeedMP * pCret->m_stAbility[AttrID::MpCostPer] / 10000.0f) - pCret->m_stAbility[AttrID::MpCost];
	magicNeedMp = magicNeedMp < 0 ? 0 : magicNeedMp;
	return magicNeedMp;
}

//////////////////////////////////////////////////////////////////////////

void CMagicManage::resetAllUseTime(){
	for (auto& [id, magic] : m_skills)
	{
		if (!magic) continue;
		magic->resetUseTime();
	}
}
void CMagicManage::clearAllUseTime(){
	for (auto& [id,magic]:m_skills)
	{
		if (!magic) continue;
		magic->clearUseTime();
	}
}
void CMagicManage::clear(){
	m_skills.clear();
}

stMagic* CMagicManage::addskill(DWORD skillid, BYTE level, bool boStudy){
	stMagic* pmagic=findskill(skillid);
	if (pmagic){ 
		pmagic->changeLv(1, level, *this);
	}else{
		auto newMagic = stMagic::createSkill(skillid,level);
		if (newMagic)
		{
			auto it = m_skills.emplace(newMagic->GetMagicDataBase()->nID, std::move(newMagic)).first;
			pmagic = it->second.get();
		}
	}
	if (pmagic){
		auto magicbase = pmagic->GetMagicDataBase();
		if (magicbase && pmagic->savedata.level > 0 && magicbase->btActiveType == ACTIVETYPE_ACTIVE && magicbase->btEnemyType == ENEMYTYPE_ENEMY)
		{
			m_ActAttackIds.push_back(skillid);
		}
		if (magicbase->btActiveType == ACTIVETYPE_PASSIVE && magicbase->dwEffectid)
			m_owner->ChangePropertyCheckPlayer(ABILITY_FLAG_PASSIVESKILL);
	}
	return pmagic;
}

bool CMagicManage::doremoveskill(stMagic* pmagic){
	if (!pmagic) return false;
	auto magicDataBase = pmagic->GetMagicDataBase();
	FUNCTION_BEGIN;
	if (magicDataBase){
		m_skills.erase(magicDataBase->nID);
		std::remove_if(m_ActAttackIds.begin(), m_ActAttackIds.end(), [pmagic](uint32_t skillid) {
			return skillid == pmagic->GetMagicDataBase()->nID;
		});
	}
	return true;
}

bool CMagicManage::removeskill(DWORD skillid){
	FUNCTION_BEGIN;
	stMagic* pmagic=findskill(skillid);
	if (!pmagic){ return false;	}
	doremoveskill(pmagic);	
	return true;
}

bool CMagicManage::removeskill(stMagic* pmagic){
	FUNCTION_BEGIN;
	if (pmagic && pmagic->GetMagicDataBase()){
		doremoveskill(pmagic);
		return true;
	}
	return false;
}

stMagic* CMagicManage::findskill(DWORD skillid){
	if (auto it = m_skills.find(skillid);it!= m_skills.end())
	{
		return it->second.get();
	}
	return nullptr;
}

stMagic* CMagicManage::randomattackskill(){
	FUNCTION_BEGIN;
	int ranid = _random_d(m_skills.size());
	int idx = 0;
	for (auto& [id, magic] : m_skills)
	{
		if (!magic) continue;
		if (idx == ranid) {
			return magic.get();
		}
		idx++;
	}
	return NULL;
}

stMagic* CMagicManage::randomActiveSkill()
{
	FUNCTION_BEGIN;
	int ranid = _random_d(m_ActAttackIds.size());
	int idx = 0;
	for (auto id : m_ActAttackIds)
	{
		if (idx == ranid && m_skills.find(id)!= m_skills.end()) {
			return m_skills[id].get();
		}
		idx++;
	}
	return nullptr;
}

stMagic* CMagicManage::ChooseActSkill(CCreature* pCret, const std::function<bool(stMagic*, stMagic*)>& predicate)
{
	FUNCTION_BEGIN;
	stMagic* pMagic = nullptr;
	for (auto id : m_ActAttackIds)
	{
		auto it = m_skills.find(id);
		if (it != m_skills.end()) {
			auto pTmpMagic = it->second.get();
			if (pTmpMagic->checkCanUse(pCret))
			{
				if (!pMagic || predicate(pTmpMagic, pMagic))
				{
					pMagic = pTmpMagic;
				}
			}
		}
	}
	return pMagic;
}

void CMagicManage::SendCretSkill(stSkillLvl* pSkillLvl){
	FUNCTION_BEGIN;
	if (m_owner){
		stSkillAdd retcmd;
		retcmd.skilllvl=*pSkillLvl;
		m_owner->SendMsgToMe(&retcmd,sizeof(stSkillAdd));
	}
}

void CMagicManage::SendAllCretSkill(){
	FUNCTION_BEGIN;
	if (m_owner){
		BUFFER_CMD(stSendAllSkill,retcmd,stBasePacket::MAX_PACKET_SIZE);
		for (auto& [id,magic]:m_skills)
		{
			if (!magic) continue;
			if (magic->savedata.skillid == _MAGIC_PUTONGGONGJI_) continue;
			retcmd->skill_arr.push_back(magic->savedata, __FUNC_LINE__);
		}
		m_owner->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->skill_arr.getarraysize());
		//CChat::sendSystem(m_owner->getName(),"发送技能%d个",m_skillm.size());
	}
}


void CMagicManage::SetSkillSet(stSkillLvl* curskilldata){
	if (curskilldata && curskilldata->boContinuousCasting){
		stMagic* pMagic=NULL;
		if (pMagic){
			if (pMagic->savedata.boContinuousCastChange && curskilldata->boContinuousCasting){
				pMagic->savedata.boContinuousCasting=!(curskilldata->boContinuousCasting);
				stSkillSetCmd retcmd;
				retcmd.dwMagicId=pMagic->savedata.skillid;
				retcmd.boLocked=pMagic->savedata.boLocked;
				retcmd.boContinuousCasting=pMagic->savedata.boContinuousCasting;
				m_owner->SendMsgToMe(&retcmd,sizeof(retcmd));
			}
		}
	}
}

bool CMagicManage::saveskill(char* dest,DWORD& retlen){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	retlen=0;
	if ( maxsize< (sizeof(int)+sizeof(stSkillLvl)) ){ return false;}

	int count=0;
	int len = sizeof(count);
	for (auto& [id, magic] : m_skills)
	{
		if (!magic) continue;
		memcpy((void*)(dest + len), &magic->savedata, sizeof(stSkillLvl));
		len += sizeof(stSkillLvl);
		count++;
	}
	*((int*)(dest))=count;
	retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
	
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen);
	base64_encode(dest,len,pin,retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CMagicManage::loadskill(const char* dest,int retlen,int nver, emJobType job){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	if (maxsize==0) return true;

	retlen=ROUNDNUMALL(retlen,4)/4*3;
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen);
	base64_decode((char*)dest,maxsize,pin,retlen);
	memcpy((char*)dest,pin,retlen);
	maxsize=retlen;

	int nstlen=sizeof(stSkillLvl);
	if (maxsize< (int)(sizeof(int))){ return false;}

	int count= *((int*)(dest));
	int len=sizeof(count);

	while (count>0){
		DWORD datalen=safe_max(maxsize-len,0);
		if (datalen>=(DWORD)(nstlen*count)){
			stSkillLvl skilllvl;
			memcpy(&skilllvl,(void *)(dest+len),nstlen);
			len += nstlen;
			stMagic* pMagic=addskill(skilllvl.skillid,skilllvl.originLv,skilllvl.boStudy);
			if (pMagic){
				bool lockchange=pMagic->savedata.boLockChange;
				bool castingchange=pMagic->savedata.boContinuousCastChange;
				pMagic->savedata=skilllvl;
				pMagic->savedata.boLockChange=lockchange;
				if(!lockchange){
					pMagic->savedata.boLocked = pMagic->GetMagicDataBase()->boLocked;
				}
				pMagic->savedata.boContinuousCastChange=castingchange;
				if(!castingchange){
					pMagic->savedata.boContinuousCasting = pMagic->GetMagicDataBase()->boContinuousCasting;
				}
			}
		}
		else if (datalen!=0){
			return false;
		}
		count--;
	}
	return true;
}

bool CMagicManage::CanLevelUp(DWORD dwLevel){
	FUNCTION_BEGIN;
	if (m_owner->m_dwLevel>=dwLevel){
		return true;
	}else return false;
}

void CMagicManage::SendAllMagicCD()
{//发送谁有技能CD
	for (auto& [id, magic] : m_skills)
	{
		if (!magic) continue;
		stSkillCDTime cdtimecmd;
		cdtimecmd.dwMagicId = magic->savedata.skillid;
		cdtimecmd.dwPublicTick = 0;
		cdtimecmd.dwSelfTick = magic->getleftcd(m_owner);
		m_owner->SendMsgToMe(&cdtimecmd, sizeof(cdtimecmd));
	}
}

void CMagicManage::calcSkillExtarLv(const stSpecialAbility& specialAbi) {
	for (auto& [id,magic]:m_skills)
	{
		if (!magic) continue;
		magic->calcSkillExtarLv(specialAbi, *this);
	}
}

void CMagicManage::calcPassiveSkillAbility(stARpgAbility* abi, emJobType job) {
	if (!abi) return;
	abi->Clear();
	for (auto&[id,magic]:m_skills)
	{
		if (!magic) continue;
		auto magicDataBase = magic->GetMagicDataBase();
		if (!magicDataBase) continue;
		if (magicDataBase->btActiveType == ACTIVETYPE_PASSIVE && magicDataBase->bBattleTmp == 0) { // 被动技能
			auto effect = magicDataBase->GetEffectDataBase();
			if (!effect)continue;
			abi->Add(*effect);
		}
	}
}
