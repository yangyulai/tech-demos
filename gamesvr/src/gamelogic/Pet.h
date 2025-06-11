#pragma once

#include "MonsterObj.h"

class CPlayerObj;
class CMonster;
#pragma pack(push,1)
struct stPetSvrData{
	DWORD dwMonBaseID;						// 怪物基本ID
	DWORD dwLevel;							// 当前等级
	int nNowExp;							// 当前攻击次数
	int nMaxExp;							// 升级需要攻击次数
	int nNowHp;
	int nNowMp;
	stPetSvrData(){
		ZEROSELF;
	}
};
#pragma pack(pop)

enum emPetState{
	emPet_Follow,	//跟随状态
	emPet_Attack,	//攻击状态
	emPet_Stop,		//停止状态
	emPet_Max,		//最大状态
};

#define MAX_PET_LEVEL 7
class CItem;
class CPetObj:public CMonster
{
protected:
	CCreature* m_pMaster;				//主人
public:
	static __int64 s_i64PetId;
	__int64 m_i64PetId;						//宠物唯一ID
	stPetSvrData m_stPetData;				//宠物存档数据(暂时没用到)
	emPetState m_emPetState;				//宠物状态
	DWORD m_dwCurMagicId;

	CPetObj(CMonster* pmon, std::shared_ptr<stMonsterDataBase>& monsterDataBase);
	~CPetObj() override;
	void Update() final;
	bool Die() final;				//死亡
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
	int GetZhaoHuanChangeLv(int nMagicID);//获取该宠物对应的增强等级（召唤技能的）
};

typedef LimitHash<__int64,CPetObj*>		CGlobalPetsManage;



