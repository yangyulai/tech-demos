#pragma once

#include "LocalDB.h"
#include "cmd\Magic_cmd.h"
#include "lua_base.h"
#include "event/EventDispatcher.h"

class CCreature;
class CMagicManage;
struct stARpgAbility;
struct stCretAttack;
struct stProcessMessage;
class CGameMap;
class CPlayerObj;

//ħ��ʧ��ԭ��
enum
{
	CRET_MAGICFAIL_CASTNOTSUCCESS= 1,//ʩ�Ų��ɹ�
	CRET_MAGICFAIL_MAGICLACKING= 2,	//ħ������
	CRET_MAGICFAIL_NOTINSCENE= 3,	//���ڳ�����
	CRET_MAGICFAIL_BOUND= 4,		//������
	CRET_MAGICFAIL_PETRIFACTION= 5,	//��ʯ��
	CRET_MAGICFAIL_IMPRISONMENT= 6, //������
	CRET_MAGICFAIL_SLEEP= 7,		//����˯
	CRET_MAGICFAIL_PARALYSIS= 8,	//�����
	CRET_MAGICFAIL_FROZEN= 9,		//������
	CRET_MAGICFAIL_DEAD= 10,		//����
	CRET_MAGICFAIL_TARGET =11,		//Ŀ����Ч
	CRET_MAGICFAIL_LACKITEM =12,	//ȱ����Ʒ
	CRET_MAGICFAIL_DELETEITEMFAIL =13,	//ɾ����Ʒʧ��
	CRET_MAGICFAIL_NOCOOLING= 14,	//û����ȴ
	CRET_MAGICFAIL_NORANGE= 15,		//������Χ
	CRET_MAGICFAIL_NEEDLIFENUM = 16,	//û���㹻������ֵ
	CRET_MAGICFAIL_CANNOTHIT = 17,		//���ܹ���
	CRET_MAGICFAIL_UNKNOWNMAGIC = 18,	//δ���岻��ʶ��ļ���
	CRET_MAGICFAIL_NOASSIST = 19,		//����ʹ�ø���ħ��
	CRET_MAGICFAIL_NOEQUIP = 20,		//û���������
	CRET_MAGICFAIL_NOHP = 21,			//��������
	CRET_MAGICFAIL_NOPP = 22,			//��������
	CRET_MAGICFAIL_NOENERGY = 23,		//������������
	CRET_MAGICFAIL_NONENG = 24,			//��������
	CRET_MAGICFAIL_SAFEZONE = 25,		//��ȫ���ͷż���
};

// �����б�
enum emMagicId_t{
	_MAGIC_PUTONGGONGJI_			= 1000,				//��ͨ����
	_MAGIC_FARPUTONGGONGJI_			= 2000,				//Զ����ͨ����
	_MAGIC_Excited_					= 1002,				//�˷�
	_MAGIC_Counterattack_			= 1003,				//����
	_MAGIC_Puncture_				= 2003,				//����
	_MAGIC_ContinuousFiring_		= 2502,				//����
	_MAGIC_RageHit_					= 3001,				//ŭ��
	_MAGIC_REPLY_					= 10100,			//�ظ�
	_MAGIC_ZHILIAO_					= 20050,			//����
	_MAGIC_MOVEMAGIC_				= 20400,			//����
	_MAGIC__SOULSTORM_				= 20800,			//����籩  
	_MAGIC_SHANBI_					= 30100,			//����
	_MAGIC_BAOYAN_					= 10500,			//����

};

#define SKILL_CD_TIME		520

struct stMagic{
	stSkillLvl savedata;
	mutable std::weak_ptr<stMagicDataBase> m_magicDataBase;
	std::shared_ptr<stMagicDataBase> GetMagicDataBase() const;
	ULONGLONG	lastUseTime;
	DWORD		lastUseTimeSec;
	bool		istmpmagic;				//ħ���Ƿ���Ҫ����
	static std::unique_ptr<stMagic> createSkill(DWORD skillid, BYTE level);
	virtual bool checkCanUse(CCreature* pCret, stCretAttack* pcmd = nullptr);			//��鼼���Ƿ��ʹ�ã������߼���ͬʱ����д�ú�����
	virtual bool doSkill(CCreature* pCret, stCretAttack* pcmd);			//ִ�м����߼��������߼���ͬʱ����д�ú�����
	bool isCooling(CCreature* pCret=NULL);
	void resetUseTime(CCreature* pCret=NULL);
	void clearUseTime(CCreature* pCret=NULL);
	void changeUseTime(int nchange, CCreature* pCret = NULL);
	const char* getShowName(char* szbuffer=NULL,int nmaxlen=0);
	bool skilllevelup(int nLevel,CMagicManage& cMagic);
	bool changeLv(int nIndex, int nLv, CMagicManage& cMagic);
	void calcSkillExtarLv(const stSpecialAbility& specialAbi, CMagicManage& cMagic);
	int getmaxlevel();
	int getleftcd(CCreature* pCret=NULL);
	bool isTarget(CCreature* pA, CCreature* pT);
	void getTargets(CCreature* pCret, stCretAttack* pcmd, std::vector<CCreature*>& vpTarget);
	int GetNeedMp(CCreature* pCret);
	virtual bool OnCretStruck(CCreature* player) { return true; }; //��Ӧ��ҵ�OnCretStruck, ��������˺�ͬʱ��magic ���⹦��

	stMagic(): istmpmagic(false)
	{
		lastUseTime = 0;
		lastUseTimeSec = 0;
	}
};


struct stLimitHashMagicId:LimitHash< DWORD,stMagic*> {
	static __inline const DWORD mhkey(stMagic*& e){
		return e->GetMagicDataBase()->nID;
	}
};

struct stLimitHashMagicName:LimitStrCaseHash< stMagic*> {
	static __inline const char* mhkey(stMagic*& e){
		return e->GetMagicDataBase()->szName.c_str();
	}
};

struct stMagicManage3:public zLHashManager3< 
stMagic*,
stLimitHashMagicId
>{
	stMagic* FindById(DWORD magicid){
		stMagic* value=NULL;
		if (m_e1.find(magicid,value)){
			return value;
		}
		return NULL;
	}
};


class CMagicManage{
protected:
	std::unordered_map<uint32_t, std::unique_ptr<stMagic>> m_skills;//�������ϵļ��ܳ�
	std::list<uint32_t> m_ActAttackIds; //������������id ��,��robot��
	CCreature* m_owner;								
public:
	CCreature* GetOwer(){return m_owner;};
	int GetSkillCount(){return m_skills.size();};
	void resetAllUseTime();
	void clearAllUseTime();
	void clear();
	CMagicManage(CCreature* o):m_owner(o){	return; };
	~CMagicManage(){ clear(); }
	int size(){
		return m_skills.size();
	}
	stMagic* addskill(DWORD skillid, BYTE level, bool boStudy=false);
	bool removeskill(DWORD skillid);
	bool removeskill(stMagic* pmagic);
	stMagic* findskill(DWORD skillid);																					//���Ҽ���
	stMagic* luafindskill(DWORD skillid)	{return findskill(skillid);}
	stMagic* randomattackskill();
	stMagic* randomActiveSkill();
	stMagic* ChooseActSkill(CCreature* pCret, const std::function<bool(stMagic*, stMagic*)>& predicate); //true ѡ��һ��magic
	void SendCretSkill(stSkillLvl* pSkillLvl);
	void SendAllCretSkill();
	void SetSkillSet(stSkillLvl* curskilldata);
	bool saveskill(char* dest,DWORD& retlen);
	bool loadskill(const char* dest,int retlen,int nver, emJobType job);
	bool CanLevelUp(DWORD dwLevel);
	void SendAllMagicCD();
	void calcSkillExtarLv(const stSpecialAbility& specialAbi);
	void calcPassiveSkillAbility(stARpgAbility* abi, emJobType job = emJobType::NO_JOB);
	template<class F>
	void forEach(F&& f) const {
		for (auto& [id, ptr] : m_skills) f(ptr.get());
	}
protected:
	bool doremoveskill(stMagic* pmagic);	
	friend stMagic;
};
