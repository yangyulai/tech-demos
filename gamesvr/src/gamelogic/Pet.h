#pragma once

#include "MonsterObj.h"

class CPlayerObj;
class CMonster;
#pragma pack(push,1)
struct stPetSvrData{
	DWORD dwMonBaseID;						// �������ID
	DWORD dwLevel;							// ��ǰ�ȼ�
	int nNowExp;							// ��ǰ��������
	int nMaxExp;							// ������Ҫ��������
	int nNowHp;
	int nNowMp;
	stPetSvrData(){
		ZEROSELF;
	}
};
#pragma pack(pop)

enum emPetState{
	emPet_Follow,	//����״̬
	emPet_Attack,	//����״̬
	emPet_Stop,		//ֹͣ״̬
	emPet_Max,		//���״̬
};

#define MAX_PET_LEVEL 7
class CItem;
class CPetObj:public CMonster
{
protected:
	CCreature* m_pMaster;				//����
public:
	static __int64 s_i64PetId;
	__int64 m_i64PetId;						//����ΨһID
	stPetSvrData m_stPetData;				//����浵����(��ʱû�õ�)
	emPetState m_emPetState;				//����״̬
	DWORD m_dwCurMagicId;

	CPetObj(CMonster* pmon, std::shared_ptr<stMonsterDataBase>& monsterDataBase);
	~CPetObj() override;
	void Update() final;
	bool Die() final;				//����
	void RunMonsterAI() final;
	bool ChangeMaster(CCreature* pNewMaster);
	bool isPlayerMaster(){ return (m_pMaster && m_pMaster->isPlayer()); }
	__inline CPlayerObj* getPlayerMaster(){ return (m_pMaster && m_pMaster->isPlayer()) ? (CPlayerObj*)m_pMaster : NULL; };
	__inline CCreature* getMaster() { return m_pMaster; }
	CCreature* getRootMaster();
	stPetSvrData& getPetData(){ return m_stPetData; }
	DWORD GetLevel();
	double GetPetId() { return m_i64PetId; }
	int GetPetBaseId();
	bool CanLevelUp(int nLevel,DWORD dwMonid);
	void sendCharBase();
	bool SetPetState(emPetState emState);
	void SetPetProperty();
	void SetPetProperty(CPlayerObj* p);
	void SetCurPetMagic(DWORD dwMagicId) { m_dwCurMagicId = dwMagicId; }
	DWORD GetCurPetMagic() const { return m_dwCurMagicId; }
	void StudySkill(DWORD dwSkillid,BYTE btLevel);
	bool DeleteSkill(DWORD dwSkillid);
	void ClearSkill();
	//void OpenMagicPetEff(std::vector<CCreature*> vTarget);
	void ChangeTarget(CCreature* pTarget);
	int GetZhaoHuanChangeLv(int nMagicID);//��ȡ�ó����Ӧ����ǿ�ȼ����ٻ����ܵģ�
};

typedef LimitHash<__int64,CPetObj*>		CGlobalPetsManage;



