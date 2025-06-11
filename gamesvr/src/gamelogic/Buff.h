#pragma once

#include "LocalDB.h"
#include "event/EventDispatcher.h"

class CCreature;
class CMagicManage;
struct stARpgAbility;
struct stCretAttack;
struct stProcessMessage;
class CGameMap;

class stBuff;

using BuffFunction = std::function<BYTE(CCreature*, stBuff*, int&)>;
constexpr size_t MAX_BUFF_FUNC = 255;

//状态类型，如果有状态显示对应FeatureStatus
enum emMagicStateType:uint8_t
{
	MAGICSTATE_NULL = 0,				//没有状态
	MAGICSTATE_HP = 1,					//血相关,包括蓝
	MAGICSTATE_MP = 2,					//蓝相关
	MAGICSTATE_RELIVE = 3,				//复活
	MAGICSTATE_SPEEDSLOW = 4,			//减速
	MAGICSTATE_DIZZY = 5,				//晕眩
	MAGICSTATE_PETRIFACTION = 6,		//石化
	MAGICSTATE_ABI = 7,					//属性相关
	MAGICSTATE_ABI_PERCENT = 8,			//(属性百分比)
	MAGICSTATE_MAGICSHIELD = 9,			//法师的防护
	MAGICSTATE_SWORDSHIELD = 10,		//剑士的抗魔护盾
	MAGICSTATE_MULTEXP = 11,			//多倍经验

	MAGICSTATE_MAXCOUNT,				//最大状态值
};

enum emBuffTime{
	TimeNo		=	0,	//没有时间类型
	TimeOut		=	1,	//毫秒为单位的持续时间类型 ,从现在开始，保持到多少秒后,可以修改_keep_tick延长时间
	TimeRun		=	2,	//毫秒为单位的间隔时间类型,从现在开始到结束，每间隔多少秒执行一次，可以修改_last_tick延长时间
	SecondOut	=	3,	//秒为单位的持续时间类型,从现在开始，保持到多少秒后,可以修改_keep_tick延长时间
	SecondRun	=	4,	//秒为单位的间隔时间类型,从现在开始到结束，每间隔多少秒执行一次，可以修改_last_tick延长时间
	TimeInfinite =  5,	//永久buff
};

enum emBuffRunType :uint8_t
{
	BUFF_NEW,		//新BUFF
	BUFF_RUN,		//BUFF已经加入，运行中
	BUFF_DEL,		//BUFF失效
};

class stBuff{
public:
	stBuff(const std::shared_ptr<stBuffDataBase>& buffDataBase);
	uint32_t GetBuffId() const;
	BYTE	GetLevel() const;;	//获得BUFF等级
	BYTE	GetTimeType() const;
	const char* GetBuffName() const;
	int64_t GetTimeLeft() const;

	int64_t m_keepTime;
	int64_t m_addTime;
	int64_t m_eventId;

	uint32_t dwBuffID;
	int nDura;
	DWORD m_AttackCretTmpId;

	uint8_t m_AttakCretType;
	uint8_t m_buffLevel;
	bool bInEffect;		//是否生效中，如果存在互斥技能，buff失效
	bool boNotShow;
	emBuffRunType m_emRunType;
	emMagicStateType m_emStateType;
	mutable std::weak_ptr<stBuffDataBase> m_buffDataBase;
	std::shared_ptr<stBuffDataBase> GetBuffDataBase() const;

};

class CBUFFManager{
public:
	using BuffPtr = std::unique_ptr<stBuff>;
protected:
	enum {
		MAX_NUM = 60,
	};
	std::unordered_map<uint32_t, BuffPtr> idMap_;
	std::unordered_multimap<emMagicStateType, stBuff*> stateMap_;

	ULONGLONG m_dwBuffTimeTick;
	CCreature* m_Owner;
	EventDispatcher m_event;
public:
	std::atomic<bool> m_BuffStateList[MAGICSTATE_MAXCOUNT];
	inline static std::array<BuffFunction, MAX_BUFF_FUNC + 1> g_buff_registry;

public:
	CBUFFManager(CCreature* Owner){ 
		m_dwBuffTimeTick=0;
		m_Owner=Owner;
		for (int i=0;i<MAGICSTATE_MAXCOUNT;i++)
		{
			m_BuffStateList[i] = false;
		}	
	}
	~CBUFFManager(){ clear(); }
	void Run() { m_event.run(); }
	bool AddBuff(DWORD dwBuffID, BYTE btLevel, int64_t dwKpTime=0, int nDura = 0, CCreature *pA=nullptr);
	bool AddBuff(std::shared_ptr<stBuffDataBase>& buffDataBase, int64_t dwKpTime=0, int nDura = 0);
	stBuff* FindBuff(DWORD dwBuffID);
	bool RemoveBuff(DWORD dwBuffID);
	bool RemoveBuff(stBuff* buff);
	bool RemoveBuff(emMagicStateType StateType);
	bool LuaRemoveBuff(DWORD dwBuffID) {return RemoveBuff(dwBuffID);}
	void clear();
	BYTE GetBuffLevel(emMagicStateType StateType);
	bool GetBuffState(emMagicStateType StateType);
	int64_t BuffFeature();
	int GetBuffCof(emMagicStateType StateType);
	void OfflineRemoveBuff();
	static void initBuffFun();//在UsrEngn中调用
	bool savetmpbuff(char* dest,DWORD& retlen);
	bool savebuff(char* dest,DWORD& retlen);
	bool loadbuff(const char* dest,int retlen,int nver);
	stBuff* FindByStateType(emMagicStateType StateType);
	stBuff* FindByBuffID(DWORD dwBuffid);
	bool IsControlBuff(BYTE StateType);
	void calculateBuffAbility();
	static void Exec_BuffLogicFunc(CCreature* owner, stBuff* buff, int& nNum);

};