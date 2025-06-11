#include "Pet.h"

#include "Config.h"
#include "MonsterObj.h"
#include "PlayerObj.h"
#include "UsrEngn.h"

__int64 CPetObj::s_i64PetId=_random(100)+100;

CPetObj::CPetObj(CMonster* pmon, std::shared_ptr<stMonsterDataBase>& monsterDataBase)
	: CMonster(CRET_PET, pmon->m_nCurrX, pmon->m_nCurrY, monsterDataBase,0,0),
m_pMaster(NULL)
{
	FUNCTION_BEGIN;
	s_i64PetId++;
	m_i64PetId=s_i64PetId;
	m_emPetState=emPet_Attack;
	m_dwCurMagicId = 0;
}

CPetObj::~CPetObj(){
	FUNCTION_BEGIN;
	m_pMaster=NULL;
}

void CPetObj::Update()
{
	CMonster::Update();
}

bool CPetObj::Die()
{
	if (CCreature::Die())  // NOLINT(bugprone-parent-virtual-call)
	{
		if (getMaster()->isPlayer())
		{
			stCretPetDie PetDieCmd;
			PetDieCmd.btDieType = 0;
			PetDieCmd.dwPetBaseId = getPetData().dwMonBaseID;
			getPlayerMaster()->SendMsgToMe(&PetDieCmd, sizeof(PetDieCmd));
			getPlayerMaster()->m_Petm.delPetObj(m_i64PetId);
		}
		return true;
	}
	return false;
}

void CPetObj::RunMonsterAI() {
	FUNCTION_BEGIN;
	if (isDie()) {
		return;
	}

	auto petAttackProcess = [this]() {
		CCreature* pMaster = getMaster();
		if (pMaster) {
			int atid = pMaster->quest_vars_get_var_n("AttackTargetID");
			int attype = pMaster->quest_vars_get_var_n("AttackTargetType");
			if (atid > 0 && attype > 0) {//首先寻找主人攻击对象
				CCreature* curTarget = NULL;
				if (attype == 1)
					curTarget = GetEnvir()->GetCreature(atid);
				else if (attype == 2)
					curTarget = pMaster->GetEnvir()->GetCreature(atid);
				if (curTarget) {
					m_curAttTarget = curTarget;
				}
			}
			if (m_curAttTarget == NULL) {
				atid = pMaster->quest_vars_get_var_n("AttackerID");
				attype = pMaster->quest_vars_get_var_n("AttackerType");
				if (atid > 0 && attype > 0) {//其次寻找攻击主人对象
					CCreature* curTarget = NULL;
					if (attype == 1)
						curTarget = GetEnvir()->GetCreature(atid);
					else if (attype == 2)
						curTarget = pMaster->GetEnvir()->GetCreature(atid);
					if (curTarget) {
						m_curAttTarget = curTarget;
					}
				}
			}
		}
		};
	if (m_emPetState != emPet_Stop)
	{
		if (m_emPetState == emPet_Attack)
		{
			petAttackProcess();
		}
		if (m_curAttTarget == NULL || m_emPetState == emPet_Follow)
		{
			RunNoTargetAI();
		}
		else
		{
			RunTargetAI();
		}
	}
}
bool CPetObj::ChangeMaster(CCreature* pNewMaster){
	FUNCTION_BEGIN;
	if (m_pMaster!=pNewMaster){
		if (pNewMaster){
			//检查新主人能否添加这个宠物 添加成功才从原主人的宠物列表中删除
			if (pNewMaster->isPlayer()){
				if ( pNewMaster->toPlayer()->m_Petm.putPet(this) ){
					if (m_pMaster){ m_pMaster->toPlayer()->m_Petm.removePet(this);	}
					m_pMaster=pNewMaster;
					m_btBattleCamp = pNewMaster->m_btBattleCamp;
					GetBaseProperty();
					ChangeProperty(true, __FUNC_LINE__);
					NameChanged();
					return true;
				}
			}

			return false;
		}else{
			if (m_pMaster){
				if(m_pMaster->isPlayer()){
					m_pMaster->toPlayer()->m_Petm.removePet(this);
				}
			}
			m_pMaster=NULL;
		}
	}
	return true;
}

DWORD CPetObj::GetLevel(){
	return m_dwLevel;
}

int CPetObj::GetPetBaseId() {
	return GetMonsterDataBase()->nID;
}

bool CPetObj::CanLevelUp(int nLevel,DWORD dwMonid){
	return nLevel <= MAX_PET_LEVEL;
}

void CPetObj::sendCharBase(){
	FUNCTION_BEGIN;
	stPetAbilityChangeCmd notifycmd;
	notifycmd.i64TempId=m_i64PetId;
	notifycmd.dwTempId=GetObjectId();
	if (m_pMaster){
		m_pMaster->SendMsgToMe(&notifycmd,sizeof(notifycmd));
	}
}

bool CPetObj::SetPetState(emPetState emState){
	FUNCTION_BEGIN;
	if (emState>=emPet_Follow && emState<emPet_Max){
		m_emPetState=emState;
		return true;
	}else return false;
}

void CPetObj::SetPetProperty(){

}

void CPetObj::SetPetProperty(CPlayerObj *p) {

}

void CPetObj::StudySkill(DWORD dwSkillid, BYTE btLevel) {
	FUNCTION_BEGIN;
	stMagic* pMagic = m_cMagic.addskill(dwSkillid, btLevel);
	if (pMagic && pMagic->savedata.level != btLevel) {
		pMagic->skilllevelup(btLevel, m_cMagic);
	} else {
		if (pMagic) {
			m_cMagic.SendCretSkill(&pMagic->savedata);
		}
	}
}

bool CPetObj::DeleteSkill(DWORD dwSkillid) {
	m_cMagic.removeskill(dwSkillid);
	return false;
}

void CPetObj::ClearSkill() {
	m_cMagic.clear();
}

void CPetObj::ChangeTarget(CCreature* pTarget){
	FUNCTION_BEGIN;
	CCreature* poldtarget = m_curAttTarget;
	CPlayerObj* rootMaster = NULL;
	if(isPlayerMaster()){
		rootMaster = getPlayerMaster();
	}
	
	bool boChange = false;
	if(rootMaster){
		if (poldtarget == NULL || poldtarget == rootMaster || rootMaster->m_curAttTarget == pTarget
			|| (rootMaster->m_curAttTarget == NULL && poldtarget->m_curAttTarget != rootMaster && pTarget->m_curAttTarget == rootMaster)
			){ 
			boChange = true;
		}else{
			boChange = false;
		}
	}

	if(boChange){
		SetAttackTarget(pTarget);
	}
}

CCreature* CPetObj::getRootMaster(){
	FUNCTION_BEGIN;
	if(m_pMaster){
		if(m_pMaster->isPlayer()){
			return m_pMaster;
		}
	}
	return NULL;
}

int CPetObj::GetZhaoHuanChangeLv(int nMagicID)
{	
	CCreature* pMaster1 = getMaster();
	if (!pMaster1 || !pMaster1->toPlayer())
	{
		return 0;
	}
	return 0;
}
