#include "MagicExtend.h"
#include "BaseCreature.h"
#include "UsrEngn.h"
#include "MapMagicEvent.h"

std::unique_ptr<stMagic> MagicExtend::CreateSkill(DWORD skillid, BYTE level, BYTE magictype)
{
	switch (skillid)
	{
	case _MAGIC_ZHILIAO_: //治愈术
		return std::make_unique<Magic_Healing>();
	case _MAGIC_REPLY_: //回复
		return std::make_unique<Magic_Reply>();
	default:
		switch (magictype)
		{
		case MAGICTYPE_COMMON:
			return std::make_unique<Magic_Common>();
		case MAGICTYPE_GROUND:
			return std::make_unique<Magic_Map>();
		case MAGICTYPE_MOVE:
			return std::make_unique<Magic_Move>();
		case MAGICTYPE_SHIELD:
			return std::make_unique<Magic_Shield>();
		case MAGICTYPE_BUFF:
			return std::make_unique<Magic_Buff>();
		default:
			return std::make_unique<stMagic>();
		}
	}
}

bool Magic_Common::doSkill(CCreature* pCret, stCretAttack* pcmd)
{
	bool skillCanUse = checkCanUse(pCret, pcmd);
	if (skillCanUse)
	{
		stMagic::doSkill(pCret, pcmd);

		std::vector<CCreature*> vCret;
		getTargets(pCret, pcmd, vCret);

		if (vCret.size()){
			int nTargetNum = 0;
			int nPlayerTargetNum = 0;
			CPlayerObj* pA = nullptr;
			if (pCret->isPlayer()) {
				pA = pCret->toPlayer();
				nPlayerTargetNum = pCret->toPlayer()->getTotalTargetPlayerNum(vCret);
			}
			for (auto it = vCret.begin(); it != vCret.end(); it++) {
				auto pTarget = (*it);
				nTargetNum++;
				if (pTarget)
				{
					pCret->ReleaseCretStruckFull(pTarget, GetMagicDataBase(), nTargetNum, nPlayerTargetNum, vCret.size(), 1);
				}
			}
		}

	}
	return skillCanUse;
}


void Magic_Map::AddMapMagic(CCreature* pCret, stMagic* pMagic, Magic_Map* pMapMagic, PosType nDx, PosType nDy)
{
	CMapMagicEvent* pMagicEvent = CLD_DEBUG_NEW CMapMagicEvent(pCret, pMapMagic,nDx, nDy);
	if (!pCret->GetEnvir()->AddMagicToMap(nDx, nDy, pMagicEvent))
	{
		g_logger.error("地面魔法添加失败 %s (%d,%d)", pMagic->GetMagicDataBase()->szName, nDx, nDy);
		SAFE_DELETE(pMagicEvent);
	}

}


bool Magic_Map::doSkill(CCreature* pCret, stCretAttack* pcmd)
{
	bool skillCanUse = checkCanUse(pCret, pcmd);
	if (skillCanUse)
	{
		stMagic::doSkill(pCret, pcmd);

		//将技能加到地图中
		stMagic* pMagic = pCret->m_cMagic.findskill(pcmd->dwMagicID);
		int nDx = (pMagic->GetMagicDataBase()->btAttackType == FAR_ATTACK) ? pcmd->stTarget.xDes : pCret->m_nCurrX;
		int nDy = (pMagic->GetMagicDataBase()->btAttackType == FAR_ATTACK) ? pcmd->stTarget.yDes : pCret->m_nCurrY;
		AddMapMagic(pCret, pMagic, this, nDx, nDy);
	}

	return skillCanUse;
}

void Magic_Map::run(CCreature* pCret, CGameMap* m_OwnerMap, int targetX, int targetY)
{
	auto m_pMagicData = GetMagicDataBase();
	std::vector<CCreature*> vTarget;
	m_OwnerMap->GetCretsInMagicRange(targetX, targetY, DRI_NUM, -1, -1, vTarget, 0, m_pMagicData, pCret, m_pMagicData->wAttackNum, 0, m_pMagicData->boDie);
	int nPlayerTargetNum = 0;
	if (pCret->isPlayer()) {
		nPlayerTargetNum = pCret->toPlayer()->getTotalTargetPlayerNum(vTarget);
	}
	int nTargetNum = 0;
	for (std::vector<CCreature*>::iterator it = vTarget.begin(); it != vTarget.end(); it++) 
	{
		CCreature* pTargetCret = (*it);
		if (pTargetCret && pCret) 
		{
			nTargetNum++;
			pTargetCret->m_btDamageType = 0;
			pCret->ReleaseCretStruckFull(pTargetCret, GetMagicDataBase(), nTargetNum, nPlayerTargetNum, vTarget.size(), 1);
		}
	}
}

bool Magic_Move::doSkill(CCreature* pCret, stCretAttack* pcmd)
{
	bool skillCanUse = checkCanUse(pCret, pcmd);
	if (skillCanUse)
	{
		stMagic::doSkill(pCret, pcmd);

		CGameMap* pMap = pCret->GetEnvir();
		if (!pMap) return false;
		if (GetMagicDataBase()->nID == _MAGIC_MOVEMAGIC_) {
			
			int ndx = 0, ndy = 0;
			bool bPos = pMap->GetNextPosition(pCret->GetX(), pCret->GetY(), pCret->m_btDirection, GetMagicDataBase()->btMaxRange, ndx, ndy);
			if (bPos)
			{
				pCret->LocalMapTransfer(ndx, ndy, GetMagicDataBase()->btMaxRange, true);
			}
		}
		if (GetMagicDataBase()->nID == _MAGIC_SHANBI_) {
			int nLoop = 100;
			int nRandX = pCret->m_nCurrX;
			int nRandY = pCret->m_nCurrY;
			do
			{
				if (pMap->GetSuiJiJiaoBenFanWei(nRandX, nRandY, 1, GetMagicDataBase()->btMaxRange) && pMap->CanWalk(nRandX, nRandY, 0, false)) {
					break;
				}
			} while (nLoop--);
			if (pCret->LocalMapTransfer(nRandX, nRandY, 1, true))
			{
				for (DWORD i = 0; i < GetMagicDataBase()->szSelfBuffID.size(); i++) {
					int nBuffID = GetMagicDataBase()->szSelfBuffID[i];
					pCret->m_cBuff.AddBuff(nBuffID, GetMagicDataBase()->btlevel);
				}
			}
		}
		if (GetMagicDataBase()->nID ==  _MAGIC_BAOYAN_) {			
			if (CCreature* pSelect = pMap->FindCretByTmpId(pcmd->stTarget.dwtmpid, pcmd->stTarget.xDes, pcmd->stTarget.yDes))
			{
				if (pCret->ChebyshevDistance(pSelect) > 1){
					BYTE dir = pMap->GetNextDirection(pSelect->GetX(), pSelect->GetY(), pCret->m_nCurrX, pCret->m_nCurrY);
					pCret->m_btDirection = pMap->GetReverseDirection(dir);
					bool bPos = pMap->GetNextPosition(pSelect->GetX(), pSelect->GetY(), dir, 1, m_nPosX, m_nPosY); //目标到我的方向少一格
				}
				else {
					m_nPosX = 0;
					m_nPosY = 0;
				}
				pCret->ReleaseCretStruckFull(pSelect, GetMagicDataBase(), 1, 1, 1, 300);
			}
		}
	}

	return skillCanUse;
}


bool Magic_Move::OnCretStruck(CCreature* pCret)
{
	if (!pCret) return false;
	if (GetMagicDataBase()->nID == _MAGIC_BAOYAN_)
	{
		if (m_nPosX && m_nPosY)
			pCret->LocalMapTransfer(m_nPosX, m_nPosY, GetMagicDataBase()->btMaxRange, true);
	}
	return true;
}

bool Magic_Shield::doSkill(CCreature* pCret, stCretAttack* pcmd)
{
	bool skillCanUse = checkCanUse(pCret, pcmd);
	if (skillCanUse)
	{
		stMagic::doSkill(pCret, pcmd);

		stMagic* pMagic = pCret->m_cMagic.findskill(pcmd->dwMagicID);
		if (pCret->m_cBuff.FindBuff(pMagic->GetMagicDataBase()->nID) == NULL)
		{
			for (DWORD i = 0; i < pMagic->GetMagicDataBase()->szBuffID.size(); i++)
			{
				int nBuffID = pMagic->GetMagicDataBase()->szBuffID[i];
				pCret->m_cBuff.AddBuff(nBuffID, pMagic->GetMagicDataBase()->btlevel);

				//g_logger.warn("%s 使用技能 %s 给自己增加了 BUFF %d", pCret->m_szCretName, pMagic->getShowName(), nBuffID);
			}
		}
	}

	return skillCanUse;
}

bool Magic_Buff::doSkill(CCreature* pCret, stCretAttack* pcmd)
{
	bool skillCanUse = checkCanUse(pCret, pcmd);
	if (skillCanUse)
	{
		stMagic::doSkill(pCret, pcmd);

		std::vector<CCreature*> vCret;
		getTargets(pCret,pcmd, vCret);

		if (vCret.size()){
			for (auto it = vCret.begin(); it != vCret.end(); it++){
				auto pTarget = (*it);
				for (DWORD i = 0; i < GetMagicDataBase()->szBuffID.size(); i++)
				{
					int nBuffID = GetMagicDataBase()->szBuffID[i];
					pTarget->m_cBuff.AddBuff(nBuffID, GetMagicDataBase()->btlevel, 0, 0, pCret);
				}
			}
		}
		else{
			//选中的人没用上buff 再 给自己添加buff
			if (GetMagicDataBase()->boSelf) {
				for (DWORD i = 0; i < GetMagicDataBase()->szSelfBuffID.size(); i++) {
					int nBuffID = GetMagicDataBase()->szSelfBuffID[i];
					pCret->m_cBuff.AddBuff(nBuffID, GetMagicDataBase()->btlevel);
				}
			}
		}
	}
	return skillCanUse;
}

// 治疗
bool Magic_Healing::doSkill(CCreature* pCret, stCretAttack* pcmd)
{
	bool skillCanUse = checkCanUse(pCret, pcmd);
	if (skillCanUse)
	{
		stMagic::doSkill(pCret, pcmd);

		//计算技能作用目标
		CPlayerObj* pSelect = pCret->GetEnvir()->GetPlayer(pcmd->stTarget.dwtmpid);
		int RestoreHp = pCret->m_stAbility[AttrID::MaxMAtk] * 0.1;
		if (pSelect && pCret->isEnemy(pSelect)) {
			pSelect->StatusValueChange(stCretStatusValueChange::hp, RestoreHp, __FUNC_LINE__);
		}
		else {
			pCret->StatusValueChange(stCretStatusValueChange::hp, RestoreHp, __FUNC_LINE__);
		}
	}

	return skillCanUse;
}

//回复
bool Magic_Reply::doSkill(CCreature* pCret, stCretAttack* pcmd) {
	bool skillCanUse = checkCanUse(pCret, pcmd);
	if (skillCanUse)
	{
		stMagic::doSkill(pCret, pcmd);
		std::vector<CCreature*> targetCrets;
		getTargets(pCret, pcmd, targetCrets);

		bool bUsed = false;
		for (auto it = targetCrets.begin(); it != targetCrets.end(); it++)
		{
			auto pTarget = (*it);
			if (pTarget->isPlayer())
			{
				pTarget->toPlayer()->StatusValueChange(stCretStatusValueChange::pp, GetMagicDataBase()->nDamage, "回复");
				bUsed = true;
			}
		}
		if (GetMagicDataBase()->boSelf && !bUsed && pCret->isPlayer()) {
			pCret->toPlayer()->StatusValueChange(stCretStatusValueChange::pp, GetMagicDataBase()->nDamage, "回复");
		}
	}
	return skillCanUse;
}


