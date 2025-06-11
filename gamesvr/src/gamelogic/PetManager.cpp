#include "PetManager.h"
#include "UsrEngn.h"

CPetManage::CPetManage(CCreature* player) :m_Owner(player) {
	FUNCTION_BEGIN;
	if (player->isPlayer()) {
		if (player->toPlayer()->m_siFeature.job == MAGE_JOB) {
			m_btMaxCount = 1;
		}
	}
	m_pets.clear();
	m_pSkeletonPet = NULL;
	m_pTheAnimalPet = NULL;
	m_pYueLingPet = NULL;
	m_pTianBingPet = NULL;
	m_pMagicPet = NULL;
};

CPetManage::~CPetManage() {
	FUNCTION_BEGIN;
	clear();
}

void CPetManage::Init() {
	FUNCTION_BEGIN;
	if (m_Owner->isPlayer()) {
		m_btMaxCount = 1;
	}
	m_pSkeletonPet = NULL;
	m_pTheAnimalPet = NULL;
}

CPetObj* CPetManage::addPetObj(int dwMonId, int nlevel, bool bload) {
	FUNCTION_BEGIN;
	if (!getOwner()) return nullptr;
	if (m_pets.size() >= m_btMaxCount) { g_logger.error("[刷PET(主人名%s:%d)异常]超过可允许携带宠物数 %s : %d : %d", getOwner()->getName(), dwMonId, getOwner()->GetEnvir()->getMapName(), -1, -1); return nullptr; }
	if (getPetObjByMonID(dwMonId)) return nullptr;
	CPetObj* pPet = nullptr;
	auto pmoninfo = sJsonConfig.GetMonsterDataBase(dwMonId);
	if (pmoninfo) {
		int nx = 0;
		int ny = 0;
		if (!getOwner()->GetEnvir()->GetNearNoCretXY(getOwner()->m_nCurrX, getOwner()->m_nCurrY, nx, ny, 2, 24)) {
			nx = getOwner()->m_nCurrX;
			ny = getOwner()->m_nCurrY;
		}
		pPet = getOwner()->GetEnvir()->RegenPet(dwMonId, nlevel, nx, ny);
		if (pPet) {
			if (pPet->ChangeMaster(getOwner())) {
				pPet->sendCharBase();
			}
			else {
				DelFromPetObj(pPet);
				g_logger.error("[刷PET(主人名%s:%d)异常]不能设置主人 %s : %d : %d", getOwner()->getName(), dwMonId, m_Owner->GetEnvir()->getMapName(), nx, ny);
			}
		}
	}
	if (pPet) {
		pPet->m_btBattleCamp = getOwner()->m_btBattleCamp;
		//g_logger.debug("PET %.8x 怪物ID=%d  主人%s 召唤成功! 地图 %s 坐标 %d : %d",pPet,dwMonId,getOwner()->getName(),pPet->GetEnvir()->getMapName(),pPet->m_nCurrX,pPet->m_nCurrY);
	}
	if (pPet) {
		pPet->SetPetProperty();
		if (isPlayerOwner()) {
			stCretPetDie PetCmd;
			PetCmd.btDieType = 1;
			PetCmd.dwPetBaseId = dwMonId;
			toPlayerOwner()->SendMsgToMe(&PetCmd, sizeof(PetCmd));
		}
	}
	return pPet;
}

CPetObj* CPetManage::addPetObj(int dwMonId, CPlayerObj* p, int nlevel, bool bload) {
	FUNCTION_BEGIN;
	if (!getOwner()) return nullptr;
	if (!p) return nullptr;
	if (m_pets.size() >= m_btMaxCount) { g_logger.error("[刷PET(主人名%s:%d)异常]超过可允许携带宠物数 %s : %d : %d", getOwner()->getName(), dwMonId, getOwner()->GetEnvir()->getMapName(), -1, -1); return nullptr; }
	if (getPetObjByMonID(dwMonId)) return nullptr;
	CPetObj* pPet = nullptr;
	auto pmoninfo = sJsonConfig.GetMonsterDataBase(dwMonId);
	if (pmoninfo) {
		int nx = 0;
		int ny = 0;
		if (!getOwner()->GetEnvir()->GetNearNoCretXY(getOwner()->m_nCurrX, getOwner()->m_nCurrY, nx, ny, 2, 24)) {
			nx = getOwner()->m_nCurrX;
			ny = getOwner()->m_nCurrY;
		}
		pPet = getOwner()->GetEnvir()->RegenPet(dwMonId, nlevel, nx, ny, p);
		if (pPet) {
			strcpy_s(pPet->m_szMonsterShowName, sizeof(pPet->m_szMonsterShowName) - 1, p->getName());
			pPet->SetName(pPet->m_szMonsterShowName);
			if (pPet->ChangeMaster(getOwner())) {
				pPet->sendCharBase();
			}
			else {
				DelFromPetObj(pPet);
				g_logger.error("[刷PET(主人名%s:%d)异常]不能设置主人 %s : %d : %d", getOwner()->getName(), dwMonId, m_Owner->GetEnvir()->getMapName(), nx, ny);
			}
		}
	}
	if (pPet) {
		pPet->m_btBattleCamp = getOwner()->m_btBattleCamp;
		//g_logger.debug("PET %.8x 怪物ID=%d  主人%s 召唤成功! 地图 %s 坐标 %d : %d",pPet,dwMonId,getOwner()->getName(),pPet->GetEnvir()->getMapName(),pPet->m_nCurrX,pPet->m_nCurrY);
	}
	if (pPet) {
		pPet->SetPetProperty(p);
	}
	return pPet;
}

bool CPetManage::delPetObj(__int64 i64tmpid) {
	FUNCTION_BEGIN;
	do {
		petmapit it = m_pets.find(i64tmpid);
		if (it != m_pets.end() && it->second) {
			CPetObj* pdelpet = it->second;
			g_logger.debug("CPetManage::delPetObj()中调用 removePet()删除[%s]的宠物[%s]!", pdelpet->getMaster()->getName(), pdelpet->getName());
			removePet(pdelpet);

			DelFromPetObj(pdelpet);
		}
	} while (false);
	return true;
}

bool CPetManage::delPetObj(double i64tmpid) {
	FUNCTION_BEGIN;
	do {
		petmapit it = m_pets.find(i64tmpid);
		if (it != m_pets.end() && it->second) {
			CPetObj* pdelpet = it->second;
			g_logger.debug("CPetManage::delPetObj()中调用 removePet()删除[%s]的宠物[%s]!", pdelpet->getMaster()->getName(), pdelpet->getName());
			removePet(pdelpet);

			DelFromPetObj(pdelpet);
		}
	} while (false);
	return true;
}

CPetObj* CPetManage::getPetObjByMonID(int monid) {
	for (auto it = m_pets.begin(); it != m_pets.end(); it++) {
		if (it->second->GetMonsterDataBase()->nID == monid) {
			return it->second;
		}
	}
	return nullptr;
}

CPetObj* CPetManage::getPetObj(double i64tmpid) {
	petmapit it = m_pets.find(i64tmpid);
	if (it != m_pets.end() && it->second) {
		return it->second;
	}
	return nullptr;
}

bool CPetManage::putPet(CPetObj* ppet) {
	FUNCTION_BEGIN;
	if (m_pets.size() >= m_btMaxCount) { return false; }
	if (ppet) {
		m_pets.insert(petmaps::value_type(ppet->m_i64PetId, ppet));
		return true;
	}
	return false;
}


bool CPetManage::removePet(CPetObj* ppet) {
	FUNCTION_BEGIN;
	if (!ppet) { return true; }
	bool bochangecur = false;
	if (ppet->getMaster()) {
		g_logger.debug("[%s]的宠物[%s]被解散!", ppet->getMaster()->getName(), ppet->getName());
	}
	m_pets.erase(ppet->m_i64PetId);
	if (ppet == m_pSkeletonPet) { m_pSkeletonPet = NULL; }
	if (ppet == m_pTheAnimalPet) { m_pTheAnimalPet = NULL; }
	if (ppet == m_pYueLingPet) { m_pYueLingPet = NULL; }
	if (ppet == m_pTianBingPet) { m_pTianBingPet = NULL; }
	if (ppet == m_pMagicPet) { m_pMagicPet = NULL; }

	return true;
}

CPetObj* CPetManage::findAllPet(__int64 i64tmpid) {
	FUNCTION_BEGIN;
	do {
		petmapit it = m_pets.find(i64tmpid);
		if (it != m_pets.end()) {
			if (it->second) {
				return it->second;
			}
			else {
				m_pets.erase(it);
			}
		}
	} while (false);
	return NULL;
}

bool CPetManage::clear() {
	FUNCTION_BEGIN;
	for (petmapit it = m_pets.begin(); it != m_pets.end(); it++) {
		CPetObj* ppet = it->second;
		if (ppet) { DelFromPetObj(ppet); }
	}
	m_pets.clear();
	m_pSkeletonPet = NULL;
	m_pTheAnimalPet = NULL;
	m_pYueLingPet = NULL;
	m_pTianBingPet = NULL;
	m_pMagicPet = NULL;
	return true;
}

bool CPetManage::DieClear() {
	FUNCTION_BEGIN;
	for (petmapit it = m_pets.begin(); it != m_pets.end(); it++) {
		CPetObj* ppet = it->second;
		if (ppet) {
			g_logger.debug("CPetManage::DieClear()中调用 DelFromPetObj()删除[%s]的宠物[%s]!", ppet->getMaster()->getName(), ppet->getName());
			if (ppet->getMaster()) {
				g_logger.debug("[%s]的宠物[%s]被解散!", ppet->getMaster()->getName(), ppet->getName());
			}

			if (m_Owner->isPlayer()) {
				stCretPetDie diecmd;
				diecmd.btDieType = 0;
				diecmd.dwPetBaseId = ppet->getPetData().dwMonBaseID;
				m_Owner->SendMsgToMe(&diecmd, sizeof(diecmd));
			}

			DelFromPetObj(ppet);
		}
	}
	m_pets.clear();
	m_pSkeletonPet = NULL;
	m_pTheAnimalPet = NULL;
	m_pYueLingPet = NULL;
	m_pTianBingPet = NULL;
	m_pMagicPet = NULL;
	return true;
}

bool CPetManage::SetPetState(emPetState emState) {
	FUNCTION_BEGIN;
	for (petmapit it = m_pets.begin(); it != m_pets.end(); it++) {
		CPetObj* ppet = it->second;
		if (ppet) {
			ppet->SetPetState(emState);
		}
	}
	return true;
}

void CPetManage::LoadAllPets(std::function<void(int idx, CPetObj*)> addPetHdl) {
	FUNCTION_BEGIN;
	int nIdx = 1;
	for (petmapit it = m_pets.begin(); it != m_pets.end(); it++) {
		CPetObj* pPet = it->second;
		if (pPet) {
			addPetHdl(nIdx++, pPet);
		}
	}
}

void CPetManage::DisappearPet() {
	FUNCTION_BEGIN;
	for (petmapit it = m_pets.begin(); it != m_pets.end(); it++) {
		CPetObj* ppet = it->second;
		if (ppet) {
			ppet->m_boGmHide = true;
			ppet->Disappear();
		}
	}
}

bool CPetManage::save(char* dest, DWORD& retlen) {
	FUNCTION_BEGIN;
	int maxsize = retlen;
	retlen = 0;
	if (maxsize < (sizeof(int))) { return false; }

	int count = 0;
	int len = sizeof(count);

	do {
		for (petmaps::iterator it = m_pets.begin(); it != m_pets.end(); ++it) {
			CPetObj* ppet = it->second;
			if (ppet) {
				stPetSvrData* psavedata = (stPetSvrData*)&dest[len];
				len = len + sizeof(stPetSvrData);
				ppet->getPetData().nNowHp = max(1, ppet->m_nNowHP);
				ppet->getPetData().nNowMp = ppet->m_nNowMP;
				*psavedata = ppet->getPetData();
				count++;
			}
		}
	} while (false);

	*((int*)(dest)) = count;
	retlen = ROUNDNUMALL(len, 3) / 3 * 4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin = ptlsbuf;
	ZeroMemory(pin, retlen);
	base64_encode(dest, len, pin, retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CPetManage::load(const char* dest, int retlen, int nver) {
	FUNCTION_BEGIN;
	int maxsize = retlen;

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

	if (count <= m_btMaxCount) {
		while (count > 0) {
			int datalen = safe_max(maxsize - len, 0);
			if (datalen >= sizeof(stPetSvrData)) {
				stPetSvrData* psavedata = NULL;
				char szupdatebuffer[2][sizeof(stPetSvrData) + 1024 * 4];
				szupdatebuffer[0][0] = 0;
				szupdatebuffer[1][0] = 0;
				int nowbuffer = 0;
#define		_UPDATENOWBUFFER_		nowbuffer=((nowbuffer==0)?1:0);
				char* poldbuffer = (char*)&dest[len];
				bool bomoveloadpos = false;
				bool boAddpet = true;

				if (!bomoveloadpos) { bomoveloadpos = true; len = len + sizeof(stPetSvrData); }

				psavedata = (stPetSvrData*)poldbuffer;
				if (psavedata->dwMonBaseID != 0) {

					if (boAddpet) {
						CPetObj* ppet = addPetObj(psavedata->dwMonBaseID, psavedata->dwLevel, true);
						if (ppet) {
							ppet->getPetData() = *psavedata;
							ppet->StatusValueChange(stCretStatusValueChange::hp, ppet->getPetData().nNowHp, __FUNC_LINE__);
							ppet->StatusValueChange(stCretStatusValueChange::mp, ppet->getPetData().nNowMp, __FUNC_LINE__);
							if (psavedata->dwMonBaseID == 60001) {
								m_pYueLingPet = ppet;
							}
							else if (psavedata->dwMonBaseID == 60002) {
								m_pTheAnimalPet = ppet;
							}
							else if (psavedata->dwMonBaseID == 60003) {
								m_pTianBingPet = ppet;
							}
							else if (psavedata->dwMonBaseID == 60035) {
								m_pYueLingPet = ppet;
							}
						}
					}
				}
				count--;
			}
			else if (datalen != 0) {
				return false;
			}
		}
	}
	return true;
}

DWORD CPetManage::getpetcount() {
	FUNCTION_BEGIN;
	return m_pets.size();
}

bool CPetManage::DelFromPetObj(CPetObj* pPet) {
	FUNCTION_BEGIN;
	if (pPet) {
		SAFE_DELETE(pPet);
		return true;
	}
	return false;
}

void CPetManage::AllSpaceMove(CGameMap* pMap, DWORD dwCloneID, int nx, int ny) {

}

void CPetManage::AllSetBattleCamp(BYTE btCamp) {
	FUNCTION_BEGIN;
	if (m_pets.size()) {
		for (petmaps::iterator it = m_pets.begin(); it != m_pets.end(); ++it) {
			CPetObj* ppet = it->second;
			if (ppet) {
				ppet->m_btBattleCamp = btCamp;
			}
		}
	}
}

void CPetManage::AllSetSafeModel(BYTE btSafeModel)
{
	FUNCTION_BEGIN;

}
