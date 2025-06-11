#pragma once
#include <LuaScript.h>
#include "lua_base.h"
#include "zsingleton.h"
#include "quest.h"
#include <memory>
inline static thread_local CCreature* scriptcurrentuser = NULL;
inline static thread_local CCreature* lua_setscriptcurrentuser = NULL;

class CNpcObj;
class CCreature;
struct stAutoSetScriptParam{
	CCreature* m_poldplayer;
	CCreature* m_poldsetplayer;
	stAutoSetScriptParam(CCreature* player=NULL,CNpcObj* pnpc=NULL);
	~stAutoSetScriptParam();
};

struct stInt{
	int nInt;
	int nAggro;
	int nReduceDamage;				//内功抵伤
	int nSealDamage;				//官印增伤
	Byte btAddType;					//默认0：无效果  1:会心一击	
	Byte btAddTypeTwo;					//默认0：无效果  1:强攻
	stInt(){
		ZEROSELF;
	}
};

struct stVMInfo{
	int luavmIdx;
	stVMInfo(){
		ZEROSELF;
	}
};
class EnumExporter;

class CScriptSystem
{
public:
	CLuaVM *m_LuaVM;	//lua虚拟机指针
	CScriptSystem();
	~CScriptSystem();
	bool InitScript(char* pszScriptFileName,DWORD initstate=eScript_reload);	//初始化脚本管理
	void Bind(CLuaVM* luavm);
	void BindStruct(CLuaVM* luavm);
	void BindStructBase(CLuaVM* luavm);
	void BindClass(CLuaVM* luavm);
	void BindQuest(CLuaVM* luavm);
	void BindOther(CLuaVM* luavm);
	void BindActivity(CLuaVM* luavm);
	void BindCreature(CLuaVM* luavm);
	void ExportEnum() const;
	void run();


private:
	ULONGLONG m_ScriptTick;
	int   m_min;		//每分钟
	int   m_halfhour;	//每半小时
	int   m_hour;		//每小时
	int   m_day;		//每天0点0分
	ULONGLONG   m_tensecond;	//每10秒
	std::unique_ptr<EnumExporter> m_enumExporter;

};

struct stTimer{
	DWORD m_time;
	BYTE m_week;
	BYTE m_day;
	BYTE m_hours;
	BYTE m_min;
	BYTE m_sec;
	WORD m_interval;		//间隔时间
	BYTE m_intervaltype;	//间隔类型
	WORD m_keeptime;		//保持时间
	BYTE m_keeptype;		//保持类型
	void set(){
		tm settm;
		time_t time;
		localtime_s(&settm,&time);
		m_time = (DWORD)time;
		m_week=settm.tm_wday;
		m_day=settm.tm_mday;
		m_hours=settm.tm_hour;
		m_min=settm.tm_min;
		m_sec=settm.tm_sec;
	}
	stTimer(){
		ZEROSELF;
	}
};

class CScriptTimer{
public:
	enum TimerStatus { tsRun, tsStop, tsPause };
private:	
	DWORD m_tTimeBegin;			
	DWORD m_tTimeEnd;			
	TimerStatus m_TimerStatus;	
	time_t GetCurrentSeconds();//当前时间，秒数
public:
	enum{//间隔类型,保持类型,检查类型
		WEEK=0,
		DAY=1,
		HOURS=2, 
		MINUTES=3, 
		SECONDS=4,
		CHECKALL=5,
		NOCHECK=6,
	};
	char m_func[QUEST_FUNC_LEN];//事件函数
	stTimer m_stTimer;		//系统可以用的时间结构，个人有时也可以用
	DWORD  m_checktime;	//任务开始的时间，个人用
	DWORD   m_checknum;		//要限制的时间
	BYTE    m_checktype;	//检查类型
	DWORD   m_runcount;		//执行次数
	DWORD   m_maxruncount;	//最大执行次数
	bool    m_boDel;
	void*	m_owner;
	CQuest* m_quest;
	CScriptTimer(bool bPlay = false);
	~CScriptTimer();
	void Play();
	void Stop();
	void Pause();
	void Reset();
	TimerStatus GetStatus();//得到状态
	DWORD GetTime();//返回多少秒
	void SetTime(DWORD sec);
	bool CheckTime(BYTE checktype);//检查时间是否可以执行
	bool CheckInterval(BYTE intervaltype,time_t nowtime);//检查间隔时间是否满足
	int CheckKeep(BYTE keeptype);//检查保持时间还有多少
	bool CheckEvent(CQuestList* qlist);//任务列表中可以激活的事件
	void CallEvt(CQuestList* qlist);//调用定时器事件
};

class CScriptTimerManager{
public:
	CQuestList* m_mainlist;
	void settimedate(BYTE settype,DWORD id, DWORD timenum,const char* timefunc,DWORD timedate=time(NULL),DWORD runcount=0,DWORD maxcount=1);	//设置定时器
	void settime(BYTE settype,DWORD id,stTimer tmpstTimer,const char* timefunc,DWORD runcount=0,DWORD maxcount=1);
	void clear();														//清理定时器MAP，销毁时用
	void run();
	CScriptTimer* find_timer(DWORD id);	//找定时器
	bool remove_timer(DWORD id);			//移除定时器
	CScriptTimerManager(CQuestList* mainlist=NULL,void* mainer=NULL);
	~CScriptTimerManager();
private:	
	typedef std::map<DWORD,CScriptTimer> TIMERMAPS;//定时器管理
	typedef TIMERMAPS::iterator time_iter;
	typedef TIMERMAPS::const_iterator const_time_iter;

	TIMERMAPS _timers;
	void* m_owner;
};