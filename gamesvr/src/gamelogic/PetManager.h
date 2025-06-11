#pragma once
#include "Pet.h"
#include "cmd\pet_cmd.h"

class CPetManage {
public:
	//enum{
	//	PET_MAX_COUNT=20,			//
	//};
	typedef std::map< __int64, CPetObj* > petmaps;
	typedef petmaps::iterator petmapit;

	petmaps m_pets;			//捕获的战将 可以单独交易

	int GetMaxCount() { return m_btMaxCount; }
protected:
	CCreature* m_Owner;
	BYTE m_btMaxCount;

	bool putPet(CPetObj* ppet);
	bool removePet(CPetObj* ppet);

	friend class CPetObj;
public:
	CPetObj* m_pSkeletonPet;	//骷髅
	CPetObj* m_pTheAnimalPet;	//狗
	CPetObj* m_pTianBingPet;	//天兵宝宝
	CPetObj* m_pYueLingPet;		//月灵宝宝
	CPetObj* m_pMagicPet;		//魔宠，跟随系，拍马屁系

	//CPetObj* m_pTheYueLinPet;

	CPetManage(CCreature* player);
	~CPetManage();

	void Init();
	bool isPlayerOwner() { return (m_Owner && m_Owner->isPlayer()); }
	CPlayerObj* toPlayerOwner() { return (m_Owner && m_Owner->isPlayer()) ? (CPlayerObj*)m_Owner : NULL; }
	CCreature* getOwner() { return m_Owner; }
	CPetObj* addPetObj(int dwMonId, int nlevel = 1, bool bload = false);
	CPetObj* addPetObj(int dwMonId, CPlayerObj* p, int nlevel = 1, bool bload = false);				// 人物分身(宠物)
	bool delPetObj(__int64 i64tmpid);
	bool delPetObj(double i64tmpid);
	CPetObj* getPetObjByMonID(int monid);
	CPetObj* getPetObj(double i64tmpid);
	CPetObj* findAllPet(__int64 i64tmpid);	//包括 坐骑 出战 捕获
	size_t size() { return m_pets.size(); }
	bool clear();
	bool DieClear();
	bool SetPetState(emPetState emState);
	void LoadAllPets(std::function<void(int idx, CPetObj*)> addPetHdl);
	void DisappearPet();
	bool save(char* dest, DWORD& retlen);
	bool load(const char* dest, int retlen, int nver);
	DWORD getpetcount();
	bool DelFromPetObj(CPetObj* pPet);
	void AllSpaceMove(CGameMap* pMap, DWORD dwCloneID, int nx, int ny);
	void AllSetBattleCamp(BYTE btCamp);
	void AllSetSafeModel(BYTE btSafeModel);
};
