#ifndef _QUEST_BASE_H_SADSD65748_
#define _QUEST_BASE_H_SADSD65748_

#include "define.h"
#include <sstream>
#include <map>
#include <hash_map>
#include <array>
#include "HashManage.h"
#include "db/DBConnPool.h"
#include "qglobal.h"
#include "cmd/Quest_cmd.h"
#include "cmd/Item_cmd.h"
#include "Item.h"
#include <unordered_map>
#define DB_QUEST_MAX 80000
#define MAX_TRACKING_LEN 256
#define QUEST_BIN_VER		2
#define QUEST_REWARD_COUNT  4			//此定义涉及存档...改动小心
#define QUEST_BIN_CHECK_NUM	0x5678
#define DB_QUESTDATA_TBL "mydb_questdata_tbl"
#define DB_NPCSELLDATA_TBL "mydb_npcsell_tbl"

enum EVTTYPE{//事件类型
	NEEDLV=0,	//函数样式needlv_%d 需要的等级数
	NPCKILL=1,	//函数样式npckill_%d npc的id
	NPCVISIT=2,	//函数样式npcvisit_%d npc的id
	ITEMUSE=3,	//函数样式itemuse_%d 物品id
	ITEMGET=4,	//函数样式itemget_%d 物品id 标记player questmark
	ITEMDROP=5,	//函数样式itemdrop_%d 物品id
	MAPAREA=6,	//函数样式maparea_%d_%d_%d_%d 地图ID 坐标X 坐标Y 范围Z  标记player mapcheck
	LOGIN = 10,	//函数样式login_%d 登陆次数
	OFFLINE = 11,	//函数样式offline_%d 离线次数
	DIE=12,		//函数样式die_%d 死亡次数
	MAPCELL=17,	//函数样式mapcell_%d_%d_%d_%d 地图ID 坐标X 坐标Y 范围Z
	EQUIPITEM=22,//函数样式equipitem_%d 装备物品事件，物品ID
	CHANGESVROFFLINE=25,//函数样式changesvroffline_%d 切换服务器的事件
	KILLPLAYER=28,	//杀人
	ADDFRIEND=29,	//加好友
	INGROUP=30,		//组队
	HADCHANGEDSVRENTER = 31,//切换服务器进入目标服务器后触发
	TESHU_QUEST=43,
};

enum QUESTSTATUS{//任务状态
	QUESTNO=-1,			//任务未接
	QUESTNEW=0,			//新建任务
	QUESTDOING=1,		//任务进行中
	QUESTCOMPLETED=2,	//完成任务(条件达成)
	QUESTFINISH=3,		//结束任务
	QUESTFAIL=4,		//任务失败
};

enum{
	SYSTEM=0,	//剧情/主线
	LIFEEXP=2,	//历练/支线
	LIFEEXP21 = 21,	//历练/支线
	LIFEEXP22 = 22,	//历练/支线
	LIFEEXP23 = 23,	//历练/支线
	LIFEEXP24 = 24,	//历练/支线
};

enum{
	EQUIPNO=0,		//不检查
	EQUIPBODY=1,	//检查身上
	EQUIPPACKAGE=2,	//检查包裹
};

enum emCheckNpc{
	_CHECK_NOT_ = 0,
	_CHECK_START_ = 1,
	_CHECK_END_ = 2,
};

enum{
	TIMEOK=0,	//时间检查OK
	TIMENOT=1,	//时间检查失败
};

struct stQuestConditions{								//任务完成条件
	DWORD nId;
	DWORD nCount;
	int nItemPos;									//物品的持久
	int nDelete;									//是否回收物品
	int nDeleteEquip;								//是否回收身上物品,如果只判断不回收,身上和包裹都会检测,如果是回收默认不查身上,但是如果此条=1,就会收身上装备

	DWORD nItemLvl;									//装备的穿戴等级
	stNonpareil stNpProperty[_MAX_NP_ALL_COUNT];		//极品属性
	stQuestConditions(){
		ZEROSELF;
	}
};

struct stQuestDB
{
	__int64	i64qid;								//任务ID
	char	szquestname[_MAX_QUEST_LEN_];		//任务名称
	BYTE	btquestsection;						//任务章节
	BYTE	btquesttype;						//任务类型
	BYTE    btquestsubtype;						//任务子类型
	DWORD	beginnpcid;							//开始NPC
	DWORD	endnpcid;							//结束NPC
	char	szquestdescnot[_MAX_DESC_LEN_];		//可接而未接描述
	char	szquestdescing[_MAX_DESC_LEN_];		//进行中描述
	char	szquestdescfin[_MAX_DESC_LEN_];		//完成描述
	char	szbegintargetdesc[_MAX_DESC_LEN_];	//开始目标描述
	char	szquesttargetdesc[_MAX_DESC_LEN_];	//任务目标描述
	char	szendtargetdesc[_MAX_DESC_LEN_];	//结束目标描述
	BYTE	btquesttargettype;					//任务目标类型(事件)
	char	szquestconditions[_MAX_DESC_LEN_];	//任务达成条件
	char    szquestconadd[_MAX_DESC_LEN_];		//任务达成条件说明
	char	szScreenShowMsg[_MAX_DESC_LEN_];
	int		nminlv;								//最小等级
	int		nmaxlv;								//最大等级
	DWORD     nloopcount;							//第几环
	__int64	i64frontqid;						//前置任务id
	__int64	i64Nextqid;						//前置任务id
	char	szmapcheck[_MAX_QUEST_LEN_];		//地图检查
	int		nmapposx;							//坐标X
	int		nmapposy;							//坐标Y
	int		ntimecheck;							//计时检查
	char	szquestreward[_MAX_DESC_LEN_*2];		//任务奖励
	BYTE	btitemchoose;						//物品可选择
	char	announcement[_MAX_DESC_LEN_];		//公告
	std::vector<stQuestConditions> vconditions;  //任务完成条件
	char  szeventfunc[QUEST_FUNC_LEN];
	std::vector<stItem> VQuestreward;				//任务奖励的物品
	BYTE btCheckNpc;						//是否需要ｎｐｃ检测
	void init();
	static void setItemReward(stQuestDB* pQuest,char* pszreward, std::vector<stItem> &vi);
	void setConditions(char* pszconditions);

	static char* setEventLuaFunName(int nEventType);
	stQuestDB(){
		ZEROSELF;
	}
	~stQuestDB(){
		if(VQuestreward.size()>0)
			VQuestreward.clear();
	}
};

static dbCol QuestDB_define[] = {
	{_DBC_SO_("QuestId", DB_QWORD, stQuestDB, i64qid)}, 
	{_DBC_SO_("任务名称", DB_STR, stQuestDB, szquestname)}, 
	{_DBC_SO_("任务章节", DB_BYTE, stQuestDB, btquestsection)},
	{_DBC_SO_("任务类型", DB_BYTE, stQuestDB, btquesttype)}, 
	{_DBC_SO_("任务子类型", DB_BYTE, stQuestDB, btquestsubtype)}, 
	{_DBC_SO_("开始NPC", DB_DWORD, stQuestDB, beginnpcid)},
	{_DBC_SO_("结束NPC", DB_DWORD, stQuestDB, endnpcid)},
	{_DBC_SO_("可接而未接描述", DB_STR, stQuestDB, szquestdescnot)},
	{_DBC_SO_("进行中描述", DB_STR, stQuestDB, szquestdescing)},
	{_DBC_SO_("完成描述", DB_STR, stQuestDB, szquestdescfin)},
	{_DBC_SO_("开始目标描述", DB_STR, stQuestDB, szbegintargetdesc)},
	{_DBC_SO_("任务目标描述", DB_STR, stQuestDB, szquesttargetdesc)},
	{_DBC_SO_("结束目标描述", DB_STR, stQuestDB, szendtargetdesc)},
	{_DBC_SO_("任务目标类型", DB_BYTE, stQuestDB, btquesttargettype)},
	{_DBC_SO_("任务达成条件", DB_STR, stQuestDB, szquestconditions)}, 
	{_DBC_SO_("任务达成条件特殊说明", DB_STR, stQuestDB, szquestconadd)}, 
	{_DBC_SO_("最小等级", DB_DWORD, stQuestDB, nminlv)}, 
	{_DBC_SO_("最大等级", DB_DWORD, stQuestDB, nmaxlv)}, 
	{_DBC_SO_("任务环数", DB_DWORD, stQuestDB, nloopcount)}, 
	{_DBC_SO_("前置任务id", DB_QWORD, stQuestDB, i64frontqid)},
	{_DBC_SO_("后置任务id", DB_QWORD, stQuestDB, i64Nextqid)},
	{_DBC_SO_("地图检查", DB_STR, stQuestDB, szmapcheck)}, 
	{_DBC_SO_("坐标X", DB_DWORD, stQuestDB, nmapposx)}, 
	{_DBC_SO_("坐标Y", DB_DWORD, stQuestDB, nmapposy)}, 
	{_DBC_SO_("计时检查", DB_DWORD, stQuestDB, ntimecheck)}, 
	{_DBC_SO_("奖励物品", DB_STR, stQuestDB, szquestreward)}, 
	{_DBC_SO_("物品可选择", DB_BYTE, stQuestDB, btitemchoose)}, 
	{_DBC_SO_("screenshowmsg", DB_STR, stQuestDB, szScreenShowMsg)},
	{_DBC_SO_("checknpc", DB_BYTE, stQuestDB, btCheckNpc)},
	{_DBC_MO_NULL_(stQuestDB)}, 
};

struct CVar{//变量结构
	enum{
		_DONT_CHANGE_LIFE_TYPE_	=			-1,		
		_NEED_SAVE_				=			0,		
		_DONT_SAVE_				=			1,	
	};
	BYTE _life_type;		
	std::string _value;
};
#define  _MAX_DATA_BUFSIZE_				1024*32

class CVars//任务变量
{
private:
	typedef stdext::hash_map< std::string , CVar > VARMAPS;
	typedef VARMAPS::iterator var_iterator;
	typedef VARMAPS::const_iterator const_var_iterator;	

	VARMAPS _vars;	

	bool _save(char* dest,DWORD& retlen );//保存变量
	bool _load(const char* dest,int retlen,int nver );//读取变量

	void _show(std::string& showstr);//显示变量
public:
	bool needThreadSafe;
	CIntLock omplock;

	void set_threadSafe(bool threadsafe){
		if(needThreadSafe != threadsafe){
			needThreadSafe = threadsafe;
		}
	}

	template<typename value_type>
	bool get_var(const std::string& name, value_type& value,BYTE &lifetype)//得到变量
	{
		if(needThreadSafe){
			do 
			{
				AILOCKT(omplock);
				var_iterator it = _vars.find(name);
				if (it != _vars.end()) {
					std::stringstream os(it->second._value);
					os >> value;
					lifetype=it->second._life_type;
					return true;
				}
				return false;
			} while (false);
		}else{
			var_iterator it = _vars.find(name);
			if (it != _vars.end()) {
				std::stringstream os(it->second._value);
				os >> value;
				lifetype=it->second._life_type;
				return true;
			}
			return false;
		}
	}

	template<typename value_type>
	bool get_var(const std::string& name, value_type& value)
	{
		if(needThreadSafe){
			do 
			{
				AILOCKT(omplock);
				var_iterator it = _vars.find(name);
				if (it != _vars.end()) {
					std::stringstream os(it->second._value);
					os >> value;
					return true;
				}
				return false;

			}while(false);
		}else{
			var_iterator it = _vars.find(name);
			if (it != _vars.end()) {
				std::stringstream os(it->second._value);
				os >> value;
				return true;
			}
			return false;
		}
		
	}

	bool get_var_c(const std::string& name, char*& value,BYTE &lifetype)//得到变量
	{
		if(needThreadSafe){
			do 
			{
				AILOCKT(omplock);
				var_iterator it = _vars.find(name);
				if (it != _vars.end()) {
					//std::stringstream os(it->second._value);
					//os >> value;
					value=(char*)it->second._value.c_str();
					lifetype=it->second._life_type;
					return true;
				}
				return false;

			}while(false);
		}else{
			var_iterator it = _vars.find(name);
			if (it != _vars.end()) {
				//std::stringstream os(it->second._value);
				//os >> value;
				value=(char*)it->second._value.c_str();
				lifetype=it->second._life_type;
				return true;
			}
			return false;
		}


	
	}
	std::string get_var_cstr(const std::string& name)
	{
		if (needThreadSafe) {
			do
			{
				AILOCKT(omplock);
				var_iterator it = _vars.find(name);
				if (it != _vars.end()) {
					return it->second._value;
				}
				return "";

			} while (false);
		}
		else {
			var_iterator it = _vars.find(name);
			if (it != _vars.end()) {
				return it->second._value;
			}
			return "";
		}
	}
	bool get_var_c(const std::string& name, char*& value)
	{
		if(needThreadSafe){
			do 
			{
				AILOCKT(omplock);
				var_iterator it = _vars.find(name);
				if (it != _vars.end()) {
					//std::stringstream os(it->second._value);
					//os >> value;
					value=(char*)it->second._value.c_str();
					return true;
				}
				return false;

			}while(false);
		}else{
			var_iterator it = _vars.find(name);
			if (it != _vars.end()) {
				//std::stringstream os(it->second._value);
				//os >> value;
				value=(char*)it->second._value.c_str();
				return true;
			}
			return false;
		}

	
	}

	template <typename value_type>
	bool set_var(const std::string& name,value_type const& value,BYTE lifetype=CVar::_DONT_CHANGE_LIFE_TYPE_)//设置变量
	{

		if(needThreadSafe){
			do 
			{
				AILOCKT(omplock);
				std::stringstream os;
				os << value;
				var_iterator it = _vars.find(name);
				if (it != _vars.end()) {
					it->second._value=os.str().c_str();
					//os >> it->second._value;
					if (lifetype!=CVar::_DONT_CHANGE_LIFE_TYPE_){
						it->second._life_type=lifetype;
					}
					return true;
				}else {
					CVar* pvar=&_vars[name];
					pvar->_value=os.str().c_str();
					//os >> pvar->_value;
					if (lifetype!=CVar::_DONT_CHANGE_LIFE_TYPE_){
						pvar->_life_type=lifetype;
					}else{
						pvar->_life_type=CVar::_NEED_SAVE_;
					}
					return true;
				}

			}while(false);
		}else{
			std::stringstream os;
			os << value;
			var_iterator it = _vars.find(name);
			if (it != _vars.end()) {
				it->second._value=os.str().c_str();
				//os >> it->second._value;
				if (lifetype!=CVar::_DONT_CHANGE_LIFE_TYPE_){
					it->second._life_type=lifetype;
				}
				return true;
			}else {
				CVar* pvar=&_vars[name];
				pvar->_value=os.str().c_str();
				//os >> pvar->_value;
				if (lifetype!=CVar::_DONT_CHANGE_LIFE_TYPE_){
					pvar->_life_type=lifetype;
				}else{
					pvar->_life_type=CVar::_NEED_SAVE_;
				}
				return true;
			}
		}

		

	}

	template <typename value_type>
	bool set_tmpvar(const std::string& name, value_type const& value){//设置临时变量
		return set_var(name,value,CVar::_DONT_SAVE_);
	}

	bool set_var_c(const std::string& name,const char* value,BYTE lifetype=CVar::_DONT_CHANGE_LIFE_TYPE_, int len = 0)//设置变量
	{
		if(needThreadSafe){
			do 
			{
				AILOCKT(omplock);
				var_iterator it = _vars.find(name);
				if (it != _vars.end()) {
					if (len > 0) {
						it->second._value.assign(value, len);
					}
					else {
						it->second._value = value;
					}
					//os >> it->second._value;
					if (lifetype!=CVar::_DONT_CHANGE_LIFE_TYPE_){
						it->second._life_type=lifetype;
					}
					return true;
				}else {
					CVar* pvar=&_vars[name];
					if (len > 0) {
						pvar->_value.assign(value, len);
					}
					else {
						pvar->_value = value;
					}
					//os >> pvar->_value;
					if (lifetype!=CVar::_DONT_CHANGE_LIFE_TYPE_){
						pvar->_life_type=lifetype;
					}else{
						pvar->_life_type=CVar::_NEED_SAVE_;
					}
					return true;
				}

			}while(false);
		}else{
			var_iterator it = _vars.find(name);
			if (it != _vars.end()) {
				if (len > 0) {
					it->second._value.assign(value, len);
				}
				else {
					it->second._value = value;
				}
				//os >> it->second._value;
				if (lifetype!=CVar::_DONT_CHANGE_LIFE_TYPE_){
					it->second._life_type=lifetype;
				}
				return true;
			}else {
				CVar* pvar=&_vars[name];
				if (len > 0) {
					pvar->_value.assign(value, len);
				}
				else {
					pvar->_value = value;
				}
				//os >> pvar->_value;
				if (lifetype!=CVar::_DONT_CHANGE_LIFE_TYPE_){
					pvar->_life_type=lifetype;
				}else{
					pvar->_life_type=CVar::_NEED_SAVE_;
				}
				return true;
			}
		}


	

	}

	bool set_tmpvar_c(const std::string& name, const char* value){//设置临时变量
		return set_var_c(name,value,CVar::_DONT_SAVE_);
	}

	void remove_var(const std::string& name){//移除变量

		if(needThreadSafe){
			do 
			{
				AILOCKT(omplock);
				_vars.erase(name);

			}while(false);
		}else{
			_vars.erase(name);
		}
		
	}
	bool find_var(const std::string& name){//找到变量
		if(needThreadSafe){
			do 
			{
				AILOCKT(omplock);
				return (_vars.find(name)!=_vars.end());

			}while(false);
		}else{
			return (_vars.find(name)!=_vars.end());
		}
		
	}

	void clear(){//清除所有变量
		if(needThreadSafe){
			do 
			{
				AILOCKT(omplock);
				_vars.clear();

			}while(false);
		}else{
			_vars.clear();
		}
		
	}

	bool save(char* dest,DWORD& retlen );//保存变量
	bool load(const char* dest,int retlen,int nver );//读取变量

	void show(std::string& showstr);//显示变量

	CVars();
	~CVars();
};




class CQuestList;
class CQuest;

enum{
	_BEGIN_ = 0,
	_END_ = 1,
	_ALL_ = 2,
};

typedef __int64 stEventMapID;//任务事件管理ID

typedef stunion3< stEventMapID ,WORD,WORD,DWORD> stRawEventMapIDMaker1;//建造事件管理ID的结构
typedef stunion4< stEventMapID ,WORD,WORD,WORD,WORD> stRawEventMapIDMaker2;

struct stQuestEvent{//任务事件
	union{
		CQuest* quest;	//任务
		int questid;    //任务ID
	};
	WORD mapx;
	WORD mapy;
	BYTE maprange;
	char szeventfunc[QUEST_FUNC_LEN];	//要调用的LUA函数
};

#define toremd(name,id)			stRawEventMapIDMaker1 name;name._value=(0);name._p1=(id);
#define toremd_1_2(name,id,idex)			stRawEventMapIDMaker1 name;name._p1=(id);name._p2=(idex);name._p3=(0);
#define toremd_1_3(name,id,idex)			stRawEventMapIDMaker1 name;name._p1=(id);name._p2=(0);name._p3=(idex);
#define toremd_1_2_3(name,id,idex2,idex3)			stRawEventMapIDMaker1 name;name._p1=(id);name._p2=(idex2);name._p3=(idex3);
#define toremd_1_2_3_4(name,id,idex2,idex3,idex4)	stRawEventMapIDMaker2 name;name._p1=(id);name._p2=(idex2);name._p3=(idex3);name._p4=(idex4);

typedef MultiHash< stEventMapID , stQuestEvent* > QUESTEVENTMAPS;//任务事件管理器
typedef QUESTEVENTMAPS::iterator qeit;
typedef QUESTEVENTMAPS::const_iterator const_qeit;	
typedef std::pair< qeit,qeit > qeits;
typedef std::pair< const_qeit,const_qeit > const_qeits;

struct CQuestInfo{//任务信息
public:
	typedef std::map< DWORD , CQuestInfo* > QUESTINFOMAP;//任务信息管理器
	typedef QUESTINFOMAP::iterator qioit;

	static QUESTINFOMAP m_infos;
	enum{
		INVALID_QUESTID=-1,
	};
	DWORD _quest_type;	
	DWORD _id;			

	std::string _name;		
	std::string _dis;		

	bool _isdelete;

	BYTE _time_type;
	BYTE _run_type;

	DWORD _time_param;
	DWORD _interval;

	WORD _evtid;
	DWORD _evtnum;

	stQuestDB* _questdb;	
	QUESTEVENTMAPS _events;

	CQuestInfo* add_questinfo(DWORD qid,DWORD qtype,const char* name,const char* dis,BYTE timetype=0,DWORD dwtime=0,BYTE runtype=0,DWORD interval=60*120);//添加任务信息
	static CQuestInfo* find_questinfo(DWORD qid);//找到任务信息
	static bool disable_questinfo(DWORD qid);//使某个任务无效
	static bool disable_all_questinfo();//使所有任务无效


	static bool clear_questinfo();//清除所有任务

	void seteventid(WORD evtid,DWORD evtnum);//写入事件ID相关参数
	bool reg_Questevt(CQuest* pQuest,const char* szeventfunc,BYTE range=0,WORD mapx=0,WORD mapy=0);//注册一个事件到这个任务
	bool reg_evt(stEventMapID event,const char* szeventfunc,BYTE range=0,WORD mapx=0,WORD mapy=0);//注册一个事件到这个任务
	bool unreg_evt(stEventMapID event);//注销这个任务的某个事件
	bool clear_evt(bool boByLua = true);//清除这个任务的事件
	const char* find_evt(WORD evtid,DWORD evtnum);
	bool timebool();

	CQuestInfo(){
		_quest_type=0;
		_id=0;
		_isdelete=false;
		_time_type=0;
		_run_type=0;
		_time_param=0;
		_interval=0;
		_evtid=0;
		_evtnum=0;
		_questdb=NULL;
	};

	~CQuestInfo();
};

struct stEvtVar{
	DWORD varnum[QUEST_REWARD_COUNT];
	DWORD timercheck;
	BYTE  timestatus;
	QUESTSTATUS status;
	BYTE  btStar;
	union{
		struct {
			WORD wYear;
			WORD wMonth;
			WORD wDay; 
			WORD wWeekDay;
			char  szExtendstracking[MAX_TRACKING_LEN-8];
		};
		char  sztracking[MAX_TRACKING_LEN];
	};
	bool settrack(const char* sztrack){
		if (sztrack){
			s_strncpy_s(sztracking,sztrack,MAX_TRACKING_LEN-1);
			return true;
		}
		return false;
	}
	stEvtVar(){
		ZEROSELF;
	}
};

struct stDelConditionsItem{
	CItem* pItem;
	DWORD dwSpecialItemId;
	int nCount;
	stDelConditionsItem(){
		ZEROSELF;
	}
};
class CQuest{//任务
public:
	CQuestInfo* m_info;				

	struct CQuestTimeOut{
		DWORD _start_time;			
		DWORD _keep_time;	
		CQuestTimeOut(){ZEROSELF;};
	};
	struct CQuestTimeRun{
		DWORD _last_time;		
		DWORD _time_interval;	
		CQuestTimeRun(){ZEROSELF;};
	};
	std::list<stDelConditionsItem> m_lDelConditionsItem;		//任务需要删除的物品
	stEvtVar m_evtvar;
	void QuestTerminate();//任务暂停
public:
	CQuestTimeOut _timeout;		
	CQuestTimeRun _timerun;		
	bool _update;			
	CVars _vars;//任务内部变量
	DWORD m_nStartTime;

	CQuest(CQuestInfo* info);//根据任务信息创建一个新任务
	bool save(char* dest,int& retlen );//保存任务
	bool load(const char* dest,int retlen,int nver);//读取任务
	void show(std::string& showstr);//显示任务信息
	bool checktime(time_t cktime);//检查时间
	void settime(time_t start,time_t keep,time_t last,time_t interval);//设置时间
	void savetimer(void* timer);
	DWORD getQuestId() { return m_info->_id; }
	const char* getQuestName() { return m_info->_name.c_str(); }
protected:	
	bool m_boTerminate;

	void* m_timer;
};


struct stCheckItemPurity{
	DWORD dwNum;
	std::list<char*> lItemTmpId;
	stCheckItemPurity(){
		dwNum = 0;
		lItemTmpId.clear();
	}
};


class CQuestList{
public:
	enum {
		MAX_NUM = 30,
	};

	CQuest* add_quest(DWORD qid);//添加任务
	void remove_quest(CQuest* quest);//移除任务
	CQuest* find_quest(DWORD qid);//找到任务

	stQuestEvent* has_evt(stEventMapID event);//查看是否有这个任务事件
	qeits get_evts(stEventMapID event);//得到任务事件

	void clear();//清除任务列表所有信息
	void init(CPlayerObj* pPlayer);
	bool save(char* dest,DWORD& retlen);//保存任务列表
	bool load(const char* dest,int retlen,int nver);//读取任务列表

	void show(std::string& showstr);//显示任务所有信息

	stQuestEvent* find_evt(WORD evtid, DWORD evtnum);//查找事件，自己组合ID
	stQuestEvent* find_evtbyquest(CQuest* quest);//根据任务查找事件
	bool loginaddtome(stQuestLogin* retcmd,DWORD pid,bool boSendToMe);
	void createQuestInfo(stQuestInfo* pinfo,const CQuestInfo* pQuestInfo,CPlayerObj* pPlayer);
	void TriggerEvent(CPlayerObj* currentuse,EVTTYPE evtid, DWORD evtnum,WORD wTags=0,WORD mapx=0,WORD mapy=0,bool boFirstCheck = false,int nSelect = 0);
	bool db_reg_evt(CQuest* quest,WORD evtid,DWORD evtnum,WORD mapx=0,WORD mapy=0,BYTE range=0,BYTE countryid=0,BYTE lineid=0);//任务数据库注册事件(从数据库加载的任务没有事件，需要自己添加)
	bool regevt(WORD evt, DWORD evtex,const char* szeventfunc);//脚本自己注册事件，不通过任务
	bool unregevt(WORD evt, DWORD evtex);//注销事件

	~CQuestList();

	qeit evtbegin() {return _events.begin();}
	qeit evtend() {return _events.end();}
	QUESTEVENTMAPS* getEvt() {return &_events;}
private:
	bool add_loaded_quest(CQuest* quest);//添加读取任务
	bool add_quest_events(CQuest* quest);//添加该任务所有事件


	bool reg_evt(CQuest* quest,stEventMapID event,const char* szeventfunc,BYTE range=0,WORD mapx=0,WORD mapy=0);//注册事件到任务
	bool unreg_evt(CQuest* quest,stEventMapID event=-1);//注销事件
public:
	void setQuestStatus(bool boFirstCheck,CPlayerObj* player,DWORD dwQuestType,DWORD dwCheckId,int nSelect = 0);
	stCheckItemPurity missionCheckItemConditions(CPlayerObj* player,stQuestConditions &questCondition);
	bool doQuestCmd(CPlayerObj* player,stBaseCmd* pcmd,int ncmdlen);		//处理任务消息
	void questRegEvent(CQuest* pQuest);
	void questCreateQuest(CPlayerObj* player,DWORD dwQuestid,int nChoose=0,bool boChange = false, int nStar = 0);
	bool questFinishQuest(CPlayerObj* player,DWORD dwQuestid,int nMultiReward=1,bool compel=false);	//完成任务
	void optQuestSubmit(CPlayerObj* player,stQuestSubmit* questsubmit); //处理任务
	bool CheckQuestShow(CPlayerObj* player,DWORD qid,CCreature* npc);		//检查任务显示
	bool CheckQuestReceive(CPlayerObj* player,DWORD qid);					//检查任务领取条件
	void DoingQuest(CPlayerObj* player,DWORD qid,BYTE status,const char* menu,const char* pszExtends);//任务进行中
	void DeleteQuest(CPlayerObj* player,DWORD qid,bool boForce = false);						//放弃任务
	void RefreshQuestStatus(CPlayerObj* player,DWORD qid,BYTE status,CQuest*pQuest = NULL);		//刷新任务状态
	void CreateQuest(CPlayerObj* player,CQuest* quest,bool boChange=false,int nstar = 0);					//创建新任务
	void FinishQuest(CPlayerObj* player,DWORD qid,bool isLastOne = false);						//完成任务
	CQuest* FindQuestByType(CPlayerObj* player,BYTE btQuestType);
private:
	typedef std::map< DWORD , CQuest* > QUESTMAPS;//任务管理器
	typedef QUESTMAPS::iterator quest_iterator;
	typedef QUESTMAPS::const_iterator const_quest_iterator;	

	QUESTMAPS _quests;
	QUESTEVENTMAPS _events;
};

stQuestEvent* findallevt(QUESTEVENTMAPS events,stEventMapID event);//找全局任务事件

class CQuestDB{
public:
	CQuestInfo* m_qinfo;
	stQuestDB* finddata(DWORD qid);//找到任务数据
	CQuestDB(CQuestInfo* info);
	~CQuestDB(){	
		_questdb.clear();
	};
private:
	std::unordered_map<int64_t,stQuestDB> _questdb;
};

struct stQuestCompleteId{
	DWORD dwCompleteId;
	QUESTSTATUS status;
	stQuestCompleteId(){
		ZEROSELF;
	}
};

class CQuestCompleteMark{
public:
	bool save(char* dest,DWORD& retlen );//保存
	bool load(const char* dest,int retlen,int nver);//读取
	bool setdata(DWORD qid,QUESTSTATUS status);//加入数据
	bool remove(DWORD qid);	//移除数据
	QUESTSTATUS finddata(DWORD qid);//找到数据
	bool findid(DWORD qid);//找到任务ID
private:
	typedef std::map<DWORD,stQuestCompleteId> QUESTCOMPLETEMARKMAPS;//任务完成标记MAP
	typedef QUESTCOMPLETEMARKMAPS::iterator commark_ite;
	typedef QUESTCOMPLETEMARKMAPS::const_iterator const_commark_ite;

	QUESTCOMPLETEMARKMAPS _commarkmap;
};

#endif