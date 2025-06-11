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

	petmaps m_pets;			//�����ս�� ���Ե�������

	int GetMaxCount() { return m_btMaxCount; }
protected:
	CCreature* m_Owner;
	BYTE m_btMaxCount;

	bool putPet(CPetObj* ppet);
	bool removePet(CPetObj* ppet);

	friend class CPetObj;
public:
	CPetObj* m_pSkeletonPet;	//����
	CPetObj* m_pTheAnimalPet;	//��
	CPetObj* m_pTianBingPet;	//�������
	CPetObj* m_pYueLingPet;		//���鱦��
	CPetObj* m_pMagicPet;		//ħ�裬����ϵ������ƨϵ

	//CPetObj* m_pTheYueLinPet;

	CPetManage(CCreature* player);
	~CPetManage();

	void Init();
	bool isPlayerOwner() { return (m_Owner && m_Owner->isPlayer()); }
	CPlayerObj* toPlayerOwner() { return (m_Owner && m_Owner->isPlayer()) ? (CPlayerObj*)m_Owner : NULL; }
	CCreature* getOwner() { return m_Owner; }
	CPetObj* addPetObj(int dwMonId, int nlevel = 1, bool bload = false);
	CPetObj* addPetObj(int dwMonId, CPlayerObj* p, int nlevel = 1, bool bload = false);				// �������(����)
	bool delPetObj(__int64 i64tmpid);
	bool delPetObj(double i64tmpid);
	CPetObj* getPetObjByMonID(int monid);
	CPetObj* getPetObj(double i64tmpid);
	CPetObj* findAllPet(__int64 i64tmpid);	//���� ���� ��ս ����
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
