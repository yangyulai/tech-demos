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
#define QUEST_REWARD_COUNT  4			//�˶����漰�浵...�Ķ�С��
#define QUEST_BIN_CHECK_NUM	0x5678
#define DB_QUESTDATA_TBL "mydb_questdata_tbl"
#define DB_NPCSELLDATA_TBL "mydb_npcsell_tbl"

enum EVTTYPE{//�¼�����
	NEEDLV=0,	//������ʽneedlv_%d ��Ҫ�ĵȼ���
	NPCKILL=1,	//������ʽnpckill_%d npc��id
	NPCVISIT=2,	//������ʽnpcvisit_%d npc��id
	ITEMUSE=3,	//������ʽitemuse_%d ��Ʒid
	ITEMGET=4,	//������ʽitemget_%d ��Ʒid ���player questmark
	ITEMDROP=5,	//������ʽitemdrop_%d ��Ʒid
	MAPAREA=6,	//������ʽmaparea_%d_%d_%d_%d ��ͼID ����X ����Y ��ΧZ  ���player mapcheck
	LOGIN = 10,	//������ʽlogin_%d ��½����
	OFFLINE = 11,	//������ʽoffline_%d ���ߴ���
	DIE=12,		//������ʽdie_%d ��������
	MAPCELL=17,	//������ʽmapcell_%d_%d_%d_%d ��ͼID ����X ����Y ��ΧZ
	EQUIPITEM=22,//������ʽequipitem_%d װ����Ʒ�¼�����ƷID
	CHANGESVROFFLINE=25,//������ʽchangesvroffline_%d �л����������¼�
	KILLPLAYER=28,	//ɱ��
	ADDFRIEND=29,	//�Ӻ���
	INGROUP=30,		//���
	HADCHANGEDSVRENTER = 31,//�л�����������Ŀ��������󴥷�
	TESHU_QUEST=43,
};

enum QUESTSTATUS{//����״̬
	QUESTNO=-1,			//����δ��
	QUESTNEW=0,			//�½�����
	QUESTDOING=1,		//���������
	QUESTCOMPLETED=2,	//�������(�������)
	QUESTFINISH=3,		//��������
	QUESTFAIL=4,		//����ʧ��
};

enum{
	SYSTEM=0,	//����/����
	LIFEEXP=2,	//����/֧��
	LIFEEXP21 = 21,	//����/֧��
	LIFEEXP22 = 22,	//����/֧��
	LIFEEXP23 = 23,	//����/֧��
	LIFEEXP24 = 24,	//����/֧��
};

enum{
	EQUIPNO=0,		//�����
	EQUIPBODY=1,	//�������
	EQUIPPACKAGE=2,	//������
};

enum emCheckNpc{
	_CHECK_NOT_ = 0,
	_CHECK_START_ = 1,
	_CHECK_END_ = 2,
};

enum{
	TIMEOK=0,	//ʱ����OK
	TIMENOT=1,	//ʱ����ʧ��
};

struct stQuestConditions{								//�����������
	DWORD nId;
	DWORD nCount;
	int nItemPos;									//��Ʒ�ĳ־�
	int nDelete;									//�Ƿ������Ʒ
	int nDeleteEquip;								//�Ƿ����������Ʒ,���ֻ�жϲ�����,���ϺͰ���������,����ǻ���Ĭ�ϲ�������,�����������=1,�ͻ�������װ��

	DWORD nItemLvl;									//װ���Ĵ����ȼ�
	stNonpareil stNpProperty[_MAX_NP_ALL_COUNT];		//��Ʒ����
	stQuestConditions(){
		ZEROSELF;
	}
};

struct stQuestDB
{
	__int64	i64qid;								//����ID
	char	szquestname[_MAX_QUEST_LEN_];		//��������
	BYTE	btquestsection;						//�����½�
	BYTE	btquesttype;						//��������
	BYTE    btquestsubtype;						//����������
	DWORD	beginnpcid;							//��ʼNPC
	DWORD	endnpcid;							//����NPC
	char	szquestdescnot[_MAX_DESC_LEN_];		//�ɽӶ�δ������
	char	szquestdescing[_MAX_DESC_LEN_];		//����������
	char	szquestdescfin[_MAX_DESC_LEN_];		//�������
	char	szbegintargetdesc[_MAX_DESC_LEN_];	//��ʼĿ������
	char	szquesttargetdesc[_MAX_DESC_LEN_];	//����Ŀ������
	char	szendtargetdesc[_MAX_DESC_LEN_];	//����Ŀ������
	BYTE	btquesttargettype;					//����Ŀ������(�¼�)
	char	szquestconditions[_MAX_DESC_LEN_];	//����������
	char    szquestconadd[_MAX_DESC_LEN_];		//����������˵��
	char	szScreenShowMsg[_MAX_DESC_LEN_];
	int		nminlv;								//��С�ȼ�
	int		nmaxlv;								//���ȼ�
	DWORD     nloopcount;							//�ڼ���
	__int64	i64frontqid;						//ǰ������id
	__int64	i64Nextqid;						//ǰ������id
	char	szmapcheck[_MAX_QUEST_LEN_];		//��ͼ���
	int		nmapposx;							//����X
	int		nmapposy;							//����Y
	int		ntimecheck;							//��ʱ���
	char	szquestreward[_MAX_DESC_LEN_*2];		//������
	BYTE	btitemchoose;						//��Ʒ��ѡ��
	char	announcement[_MAX_DESC_LEN_];		//����
	std::vector<stQuestConditions> vconditions;  //�����������
	char  szeventfunc[QUEST_FUNC_LEN];
	std::vector<stItem> VQuestreward;				//����������Ʒ
	BYTE btCheckNpc;						//�Ƿ���Ҫ������
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
	{_DBC_SO_("��������", DB_STR, stQuestDB, szquestname)}, 
	{_DBC_SO_("�����½�", DB_BYTE, stQuestDB, btquestsection)},
	{_DBC_SO_("��������", DB_BYTE, stQuestDB, btquesttype)}, 
	{_DBC_SO_("����������", DB_BYTE, stQuestDB, btquestsubtype)}, 
	{_DBC_SO_("��ʼNPC", DB_DWORD, stQuestDB, beginnpcid)},
	{_DBC_SO_("����NPC", DB_DWORD, stQuestDB, endnpcid)},
	{_DBC_SO_("�ɽӶ�δ������", DB_STR, stQuestDB, szquestdescnot)},
	{_DBC_SO_("����������", DB_STR, stQuestDB, szquestdescing)},
	{_DBC_SO_("�������", DB_STR, stQuestDB, szquestdescfin)},
	{_DBC_SO_("��ʼĿ������", DB_STR, stQuestDB, szbegintargetdesc)},
	{_DBC_SO_("����Ŀ������", DB_STR, stQuestDB, szquesttargetdesc)},
	{_DBC_SO_("����Ŀ������", DB_STR, stQuestDB, szendtargetdesc)},
	{_DBC_SO_("����Ŀ������", DB_BYTE, stQuestDB, btquesttargettype)},
	{_DBC_SO_("����������", DB_STR, stQuestDB, szquestconditions)}, 
	{_DBC_SO_("��������������˵��", DB_STR, stQuestDB, szquestconadd)}, 
	{_DBC_SO_("��С�ȼ�", DB_DWORD, stQuestDB, nminlv)}, 
	{_DBC_SO_("���ȼ�", DB_DWORD, stQuestDB, nmaxlv)}, 
	{_DBC_SO_("������", DB_DWORD, stQuestDB, nloopcount)}, 
	{_DBC_SO_("ǰ������id", DB_QWORD, stQuestDB, i64frontqid)},
	{_DBC_SO_("��������id", DB_QWORD, stQuestDB, i64Nextqid)},
	{_DBC_SO_("��ͼ���", DB_STR, stQuestDB, szmapcheck)}, 
	{_DBC_SO_("����X", DB_DWORD, stQuestDB, nmapposx)}, 
	{_DBC_SO_("����Y", DB_DWORD, stQuestDB, nmapposy)}, 
	{_DBC_SO_("��ʱ���", DB_DWORD, stQuestDB, ntimecheck)}, 
	{_DBC_SO_("������Ʒ", DB_STR, stQuestDB, szquestreward)}, 
	{_DBC_SO_("��Ʒ��ѡ��", DB_BYTE, stQuestDB, btitemchoose)}, 
	{_DBC_SO_("screenshowmsg", DB_STR, stQuestDB, szScreenShowMsg)},
	{_DBC_SO_("checknpc", DB_BYTE, stQuestDB, btCheckNpc)},
	{_DBC_MO_NULL_(stQuestDB)}, 
};

struct CVar{//�����ṹ
	enum{
		_DONT_CHANGE_LIFE_TYPE_	=			-1,		
		_NEED_SAVE_				=			0,		
		_DONT_SAVE_				=			1,	
	};
	BYTE _life_type;		
	std::string _value;
};
#define  _MAX_DATA_BUFSIZE_				1024*32

class CVars//�������
{
private:
	typedef stdext::hash_map< std::string , CVar > VARMAPS;
	typedef VARMAPS::iterator var_iterator;
	typedef VARMAPS::const_iterator const_var_iterator;	

	VARMAPS _vars;	

	bool _save(char* dest,DWORD& retlen );//�������
	bool _load(const char* dest,int retlen,int nver );//��ȡ����

	void _show(std::string& showstr);//��ʾ����
public:
	bool needThreadSafe;
	CIntLock omplock;

	void set_threadSafe(bool threadsafe){
		if(needThreadSafe != threadsafe){
			needThreadSafe = threadsafe;
		}
	}

	template<typename value_type>
	bool get_var(const std::string& name, value_type& value,BYTE &lifetype)//�õ�����
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

	bool get_var_c(const std::string& name, char*& value,BYTE &lifetype)//�õ�����
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
	bool set_var(const std::string& name,value_type const& value,BYTE lifetype=CVar::_DONT_CHANGE_LIFE_TYPE_)//���ñ���
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
	bool set_tmpvar(const std::string& name, value_type const& value){//������ʱ����
		return set_var(name,value,CVar::_DONT_SAVE_);
	}

	bool set_var_c(const std::string& name,const char* value,BYTE lifetype=CVar::_DONT_CHANGE_LIFE_TYPE_, int len = 0)//���ñ���
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

	bool set_tmpvar_c(const std::string& name, const char* value){//������ʱ����
		return set_var_c(name,value,CVar::_DONT_SAVE_);
	}

	void remove_var(const std::string& name){//�Ƴ�����

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
	bool find_var(const std::string& name){//�ҵ�����
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

	void clear(){//������б���
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

	bool save(char* dest,DWORD& retlen );//�������
	bool load(const char* dest,int retlen,int nver );//��ȡ����

	void show(std::string& showstr);//��ʾ����

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

typedef __int64 stEventMapID;//�����¼�����ID

typedef stunion3< stEventMapID ,WORD,WORD,DWORD> stRawEventMapIDMaker1;//�����¼�����ID�Ľṹ
typedef stunion4< stEventMapID ,WORD,WORD,WORD,WORD> stRawEventMapIDMaker2;

struct stQuestEvent{//�����¼�
	union{
		CQuest* quest;	//����
		int questid;    //����ID
	};
	WORD mapx;
	WORD mapy;
	BYTE maprange;
	char szeventfunc[QUEST_FUNC_LEN];	//Ҫ���õ�LUA����
};

#define toremd(name,id)			stRawEventMapIDMaker1 name;name._value=(0);name._p1=(id);
#define toremd_1_2(name,id,idex)			stRawEventMapIDMaker1 name;name._p1=(id);name._p2=(idex);name._p3=(0);
#define toremd_1_3(name,id,idex)			stRawEventMapIDMaker1 name;name._p1=(id);name._p2=(0);name._p3=(idex);
#define toremd_1_2_3(name,id,idex2,idex3)			stRawEventMapIDMaker1 name;name._p1=(id);name._p2=(idex2);name._p3=(idex3);
#define toremd_1_2_3_4(name,id,idex2,idex3,idex4)	stRawEventMapIDMaker2 name;name._p1=(id);name._p2=(idex2);name._p3=(idex3);name._p4=(idex4);

typedef MultiHash< stEventMapID , stQuestEvent* > QUESTEVENTMAPS;//�����¼�������
typedef QUESTEVENTMAPS::iterator qeit;
typedef QUESTEVENTMAPS::const_iterator const_qeit;	
typedef std::pair< qeit,qeit > qeits;
typedef std::pair< const_qeit,const_qeit > const_qeits;

struct CQuestInfo{//������Ϣ
public:
	typedef std::map< DWORD , CQuestInfo* > QUESTINFOMAP;//������Ϣ������
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

	CQuestInfo* add_questinfo(DWORD qid,DWORD qtype,const char* name,const char* dis,BYTE timetype=0,DWORD dwtime=0,BYTE runtype=0,DWORD interval=60*120);//���������Ϣ
	static CQuestInfo* find_questinfo(DWORD qid);//�ҵ�������Ϣ
	static bool disable_questinfo(DWORD qid);//ʹĳ��������Ч
	static bool disable_all_questinfo();//ʹ����������Ч


	static bool clear_questinfo();//�����������

	void seteventid(WORD evtid,DWORD evtnum);//д���¼�ID��ز���
	bool reg_Questevt(CQuest* pQuest,const char* szeventfunc,BYTE range=0,WORD mapx=0,WORD mapy=0);//ע��һ���¼����������
	bool reg_evt(stEventMapID event,const char* szeventfunc,BYTE range=0,WORD mapx=0,WORD mapy=0);//ע��һ���¼����������
	bool unreg_evt(stEventMapID event);//ע����������ĳ���¼�
	bool clear_evt(bool boByLua = true);//������������¼�
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
class CQuest{//����
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
	std::list<stDelConditionsItem> m_lDelConditionsItem;		//������Ҫɾ������Ʒ
	stEvtVar m_evtvar;
	void QuestTerminate();//������ͣ
public:
	CQuestTimeOut _timeout;		
	CQuestTimeRun _timerun;		
	bool _update;			
	CVars _vars;//�����ڲ�����
	DWORD m_nStartTime;

	CQuest(CQuestInfo* info);//����������Ϣ����һ��������
	bool save(char* dest,int& retlen );//��������
	bool load(const char* dest,int retlen,int nver);//��ȡ����
	void show(std::string& showstr);//��ʾ������Ϣ
	bool checktime(time_t cktime);//���ʱ��
	void settime(time_t start,time_t keep,time_t last,time_t interval);//����ʱ��
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

	CQuest* add_quest(DWORD qid);//�������
	void remove_quest(CQuest* quest);//�Ƴ�����
	CQuest* find_quest(DWORD qid);//�ҵ�����

	stQuestEvent* has_evt(stEventMapID event);//�鿴�Ƿ�����������¼�
	qeits get_evts(stEventMapID event);//�õ������¼�

	void clear();//��������б�������Ϣ
	void init(CPlayerObj* pPlayer);
	bool save(char* dest,DWORD& retlen);//���������б�
	bool load(const char* dest,int retlen,int nver);//��ȡ�����б�

	void show(std::string& showstr);//��ʾ����������Ϣ

	stQuestEvent* find_evt(WORD evtid, DWORD evtnum);//�����¼����Լ����ID
	stQuestEvent* find_evtbyquest(CQuest* quest);//������������¼�
	bool loginaddtome(stQuestLogin* retcmd,DWORD pid,bool boSendToMe);
	void createQuestInfo(stQuestInfo* pinfo,const CQuestInfo* pQuestInfo,CPlayerObj* pPlayer);
	void TriggerEvent(CPlayerObj* currentuse,EVTTYPE evtid, DWORD evtnum,WORD wTags=0,WORD mapx=0,WORD mapy=0,bool boFirstCheck = false,int nSelect = 0);
	bool db_reg_evt(CQuest* quest,WORD evtid,DWORD evtnum,WORD mapx=0,WORD mapy=0,BYTE range=0,BYTE countryid=0,BYTE lineid=0);//�������ݿ�ע���¼�(�����ݿ���ص�����û���¼�����Ҫ�Լ����)
	bool regevt(WORD evt, DWORD evtex,const char* szeventfunc);//�ű��Լ�ע���¼�����ͨ������
	bool unregevt(WORD evt, DWORD evtex);//ע���¼�

	~CQuestList();

	qeit evtbegin() {return _events.begin();}
	qeit evtend() {return _events.end();}
	QUESTEVENTMAPS* getEvt() {return &_events;}
private:
	bool add_loaded_quest(CQuest* quest);//��Ӷ�ȡ����
	bool add_quest_events(CQuest* quest);//��Ӹ����������¼�


	bool reg_evt(CQuest* quest,stEventMapID event,const char* szeventfunc,BYTE range=0,WORD mapx=0,WORD mapy=0);//ע���¼�������
	bool unreg_evt(CQuest* quest,stEventMapID event=-1);//ע���¼�
public:
	void setQuestStatus(bool boFirstCheck,CPlayerObj* player,DWORD dwQuestType,DWORD dwCheckId,int nSelect = 0);
	stCheckItemPurity missionCheckItemConditions(CPlayerObj* player,stQuestConditions &questCondition);
	bool doQuestCmd(CPlayerObj* player,stBaseCmd* pcmd,int ncmdlen);		//����������Ϣ
	void questRegEvent(CQuest* pQuest);
	void questCreateQuest(CPlayerObj* player,DWORD dwQuestid,int nChoose=0,bool boChange = false, int nStar = 0);
	bool questFinishQuest(CPlayerObj* player,DWORD dwQuestid,int nMultiReward=1,bool compel=false);	//�������
	void optQuestSubmit(CPlayerObj* player,stQuestSubmit* questsubmit); //��������
	bool CheckQuestShow(CPlayerObj* player,DWORD qid,CCreature* npc);		//���������ʾ
	bool CheckQuestReceive(CPlayerObj* player,DWORD qid);					//���������ȡ����
	void DoingQuest(CPlayerObj* player,DWORD qid,BYTE status,const char* menu,const char* pszExtends);//���������
	void DeleteQuest(CPlayerObj* player,DWORD qid,bool boForce = false);						//��������
	void RefreshQuestStatus(CPlayerObj* player,DWORD qid,BYTE status,CQuest*pQuest = NULL);		//ˢ������״̬
	void CreateQuest(CPlayerObj* player,CQuest* quest,bool boChange=false,int nstar = 0);					//����������
	void FinishQuest(CPlayerObj* player,DWORD qid,bool isLastOne = false);						//�������
	CQuest* FindQuestByType(CPlayerObj* player,BYTE btQuestType);
private:
	typedef std::map< DWORD , CQuest* > QUESTMAPS;//���������
	typedef QUESTMAPS::iterator quest_iterator;
	typedef QUESTMAPS::const_iterator const_quest_iterator;	

	QUESTMAPS _quests;
	QUESTEVENTMAPS _events;
};

stQuestEvent* findallevt(QUESTEVENTMAPS events,stEventMapID event);//��ȫ�������¼�

class CQuestDB{
public:
	CQuestInfo* m_qinfo;
	stQuestDB* finddata(DWORD qid);//�ҵ���������
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
	bool save(char* dest,DWORD& retlen );//����
	bool load(const char* dest,int retlen,int nver);//��ȡ
	bool setdata(DWORD qid,QUESTSTATUS status);//��������
	bool remove(DWORD qid);	//�Ƴ�����
	QUESTSTATUS finddata(DWORD qid);//�ҵ�����
	bool findid(DWORD qid);//�ҵ�����ID
private:
	typedef std::map<DWORD,stQuestCompleteId> QUESTCOMPLETEMARKMAPS;//������ɱ��MAP
	typedef QUESTCOMPLETEMARKMAPS::iterator commark_ite;
	typedef QUESTCOMPLETEMARKMAPS::const_iterator const_commark_ite;

	QUESTCOMPLETEMARKMAPS _commarkmap;
};

#endif