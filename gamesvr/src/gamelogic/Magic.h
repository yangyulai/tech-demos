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

//魔法失败原因
enum
{
	CRET_MAGICFAIL_CASTNOTSUCCESS= 1,//施放不成功
	CRET_MAGICFAIL_MAGICLACKING= 2,	//魔法不足
	CRET_MAGICFAIL_NOTINSCENE= 3,	//不在场景中
	CRET_MAGICFAIL_BOUND= 4,		//被束缚
	CRET_MAGICFAIL_PETRIFACTION= 5,	//被石化
	CRET_MAGICFAIL_IMPRISONMENT= 6, //被禁锢
	CRET_MAGICFAIL_SLEEP= 7,		//被昏睡
	CRET_MAGICFAIL_PARALYSIS= 8,	//被麻痹
	CRET_MAGICFAIL_FROZEN= 9,		//被冰冻
	CRET_MAGICFAIL_DEAD= 10,		//死亡
	CRET_MAGICFAIL_TARGET =11,		//目标无效
	CRET_MAGICFAIL_LACKITEM =12,	//缺少物品
	CRET_MAGICFAIL_DELETEITEMFAIL =13,	//删除物品失败
	CRET_MAGICFAIL_NOCOOLING= 14,	//没有冷却
	CRET_MAGICFAIL_NORANGE= 15,		//超出范围
	CRET_MAGICFAIL_NEEDLIFENUM = 16,	//没有足够的生命值
	CRET_MAGICFAIL_CANNOTHIT = 17,		//不能攻击
	CRET_MAGICFAIL_UNKNOWNMAGIC = 18,	//未定义不能识别的技能
	CRET_MAGICFAIL_NOASSIST = 19,		//不能使用辅助魔法
	CRET_MAGICFAIL_NOEQUIP = 20,		//没有佩戴武器
	CRET_MAGICFAIL_NOHP = 21,			//生命不足
	CRET_MAGICFAIL_NOPP = 22,			//体力不足
	CRET_MAGICFAIL_NOENERGY = 23,		//特殊能量不足
	CRET_MAGICFAIL_NONENG = 24,			//能量不足
	CRET_MAGICFAIL_SAFEZONE = 25,		//安全区释放技能
};

// 技能列表
enum emMagicId_t{
	_MAGIC_PUTONGGONGJI_			= 1000,				//普通攻击
	_MAGIC_FARPUTONGGONGJI_			= 2000,				//远程普通攻击
	_MAGIC_Excited_					= 1002,				//兴奋
	_MAGIC_Counterattack_			= 1003,				//反击
	_MAGIC_Puncture_				= 2003,				//穿刺
	_MAGIC_ContinuousFiring_		= 2502,				//连射
	_MAGIC_RageHit_					= 3001,				//怒击
	_MAGIC_REPLY_					= 10100,			//回复
	_MAGIC_ZHILIAO_					= 20050,			//治疗
	_MAGIC_MOVEMAGIC_				= 20400,			//传送
	_MAGIC__SOULSTORM_				= 20800,			//心灵风暴  
	_MAGIC_SHANBI_					= 30100,			//闪避
	_MAGIC_BAOYAN_					= 10500,			//暴炎

};

#define SKILL_CD_TIME		520

struct stMagic{
	stSkillLvl savedata;
	mutable std::weak_ptr<stMagicDataBase> m_magicDataBase;
	std::shared_ptr<stMagicDataBase> GetMagicDataBase() const;
	ULONGLONG	lastUseTime;
	DWORD		lastUseTimeSec;
	bool		istmpmagic;				//魔法是否需要保存
	static std::unique_ptr<stMagic> createSkill(DWORD skillid, BYTE level);
	virtual bool checkCanUse(CCreature* pCret, stCretAttack* pcmd = nullptr);			//检查技能是否可使用（子类逻辑不同时，重写该函数）
	virtual bool doSkill(CCreature* pCret, stCretAttack* pcmd);			//执行技能逻辑（子类逻辑不同时，重写该函数）
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
	virtual bool OnCretStruck(CCreature* player) { return true; }; //对应玩家的OnCretStruck, 处理造成伤害同时的magic 额外功能

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
	std::unordered_map<uint32_t, std::unique_ptr<stMagic>> m_skills;//人物身上的技能池
	std::list<uint32_t> m_ActAttackIds; //主动攻击技能id 表,给robot用
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
	stMagic* findskill(DWORD skillid);																					//查找技能
	stMagic* luafindskill(DWORD skillid)	{return findskill(skillid);}
	stMagic* randomattackskill();
	stMagic* randomActiveSkill();
	stMagic* ChooseActSkill(CCreature* pCret, const std::function<bool(stMagic*, stMagic*)>& predicate); //true 选第一个magic
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
