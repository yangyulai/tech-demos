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
	int nReduceDamage;				//�ڹ�����
	int nSealDamage;				//��ӡ����
	Byte btAddType;					//Ĭ��0����Ч��  1:����һ��	
	Byte btAddTypeTwo;					//Ĭ��0����Ч��  1:ǿ��
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
	CLuaVM *m_LuaVM;	//lua�����ָ��
	CScriptSystem();
	~CScriptSystem();
	bool InitScript(char* pszScriptFileName,DWORD initstate=eScript_reload);	//��ʼ���ű�����
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
	int   m_min;		//ÿ����
	int   m_halfhour;	//ÿ��Сʱ
	int   m_hour;		//ÿСʱ
	int   m_day;		//ÿ��0��0��
	ULONGLONG   m_tensecond;	//ÿ10��
	std::unique_ptr<EnumExporter> m_enumExporter;

};

struct stTimer{
	DWORD m_time;
	BYTE m_week;
	BYTE m_day;
	BYTE m_hours;
	BYTE m_min;
	BYTE m_sec;
	WORD m_interval;		//���ʱ��
	BYTE m_intervaltype;	//�������
	WORD m_keeptime;		//����ʱ��
	BYTE m_keeptype;		//��������
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
	time_t GetCurrentSeconds();//��ǰʱ�䣬����
public:
	enum{//�������,��������,�������
		WEEK=0,
		DAY=1,
		HOURS=2, 
		MINUTES=3, 
		SECONDS=4,
		CHECKALL=5,
		NOCHECK=6,
	};
	char m_func[QUEST_FUNC_LEN];//�¼�����
	stTimer m_stTimer;		//ϵͳ�����õ�ʱ��ṹ��������ʱҲ������
	DWORD  m_checktime;	//����ʼ��ʱ�䣬������
	DWORD   m_checknum;		//Ҫ���Ƶ�ʱ��
	BYTE    m_checktype;	//�������
	DWORD   m_runcount;		//ִ�д���
	DWORD   m_maxruncount;	//���ִ�д���
	bool    m_boDel;
	void*	m_owner;
	CQuest* m_quest;
	CScriptTimer(bool bPlay = false);
	~CScriptTimer();
	void Play();
	void Stop();
	void Pause();
	void Reset();
	TimerStatus GetStatus();//�õ�״̬
	DWORD GetTime();//���ض�����
	void SetTime(DWORD sec);
	bool CheckTime(BYTE checktype);//���ʱ���Ƿ����ִ��
	bool CheckInterval(BYTE intervaltype,time_t nowtime);//�����ʱ���Ƿ�����
	int CheckKeep(BYTE keeptype);//��鱣��ʱ�仹�ж���
	bool CheckEvent(CQuestList* qlist);//�����б��п��Լ�����¼�
	void CallEvt(CQuestList* qlist);//���ö�ʱ���¼�
};

class CScriptTimerManager{
public:
	CQuestList* m_mainlist;
	void settimedate(BYTE settype,DWORD id, DWORD timenum,const char* timefunc,DWORD timedate=time(NULL),DWORD runcount=0,DWORD maxcount=1);	//���ö�ʱ��
	void settime(BYTE settype,DWORD id,stTimer tmpstTimer,const char* timefunc,DWORD runcount=0,DWORD maxcount=1);
	void clear();														//����ʱ��MAP������ʱ��
	void run();
	CScriptTimer* find_timer(DWORD id);	//�Ҷ�ʱ��
	bool remove_timer(DWORD id);			//�Ƴ���ʱ��
	CScriptTimerManager(CQuestList* mainlist=NULL,void* mainer=NULL);
	~CScriptTimerManager();
private:	
	typedef std::map<DWORD,CScriptTimer> TIMERMAPS;//��ʱ������
	typedef TIMERMAPS::iterator time_iter;
	typedef TIMERMAPS::const_iterator const_time_iter;

	TIMERMAPS _timers;
	void* m_owner;
};