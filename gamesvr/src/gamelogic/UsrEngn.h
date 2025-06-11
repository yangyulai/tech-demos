#pragma once

#include "zSingleton.h"
#include "GameMap.h"
#include "BaseCreature.h"
#include "PlayerObj.h"
#include "Script.h"
#include "ScriptSql.h"
#include "cmd/ARPG_Mail_cmd.h"
#include "cmd/consignment_cmd.h"
#include "cmd\Rank_cmd.h"
#include "cmd/Guild_cmd.h"
#include "json/json.h"
#include <boost/pool/object_pool.hpp>
#include <memory/sharedmemory.h>
#include "Item.h"
#include "SkyLadder.h"
#include "GuildFortress.h"
#include "DynamicMap.h"
#pragma pack(push,1)

struct stGmCmd{
	char szName[_MAX_NAME_LEN_];
	BYTE btGmLv;
	char szCmdStr[_MAX_RETCMD_LEN_];
	stGmCmd(){
		ZEROSELF;
	}
};

struct stDelayChatCMD{
	std::string szCMD;
	std::string playerName;
	bool toallsvr;
	stDelayChatCMD(){
		  toallsvr = false;
		 szCMD = "";
		 playerName = "";
	}
};

struct stRmbAdd{
	char szAccount[_MAX_ACCOUT_LEN_];	//账号
	char szName[_MAX_NAME_LEN_];
	char Order_No[_MAX_RMB_ORDER_NO_LEN_];		//订单号
	int  Point;							//充值点数
	int GameType;                  //充值类型
	int rechargetype;				//充值平台类型
	stRmbAdd(){
		ZEROSELF;
	}
};

struct stOnlineNum{
	DWORD dwAllOnlineNum;	//全服总在线人数
	std::vector<stlink2<WORD,WORD>> vOnlineNumArr;	//分线在线人数
	stOnlineNum(){
		dwAllOnlineNum=0;
		vOnlineNumArr.clear();
	}
};

struct stNoticeItem{	//公告物品
	stItem NoticeItem;
	time_t AddTime;
	stNoticeItem(){
		ZEROSELF;
	}
};

#pragma pack(pop)

#define _MAX_MAPCLONEID_		0xfff0
#define CALL_LUA CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall
#define CALL_LUARET CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call


struct stNewHumHomeMapInfo{
	WORD wmapid;
	BYTE btmapcountryid;
	BYTE btmapsublineid;
	int64_t i64UserOnlyId;
	int nlevel;
	//char szName[_MAX_NAME_LEN_];
	BYTE job;
	BYTE sex;
	BYTE btCountryID;
	int homex;
	int homey;
};

struct stHumMapInfo{
	DWORD dwmapid;
	WORD wmapcloneid;
	BYTE btmapcountryid;
	BYTE btmapsublineid;
	int64_t i64UserOnlyId;
	int nlevel;
	//char szName[_MAX_NAME_LEN_];
	BYTE job;
	BYTE sex;
	BYTE btCountryID;
	int curx;
	int cury;
};

struct stVarToSave
{
	std::string name;
	std::string value;
	time_t sendtick;
	int saveidx;
};

enum emDelayLuaCall{
	none = 0,
	reloadRes = 1,
	setserverexp = 2,
	excutecommand = 3,
 
};


struct stDelayCallFunction{
	emDelayLuaCall type;
	std::vector<std::string> params;
	stDelayCallFunction(){
		type = none;
	}
	~stDelayCallFunction(){
		params.clear();
	}
};

enum emPooledObjectType
{
	poolTypeCretAttackMsg = 3,
	poolTypeItem = 4,

};

struct stPointxy
{
	int x;
	int y;
	stPointxy()
	{
		ZEROSELF;
	}
};

struct stMagicPoint
{
	int nMagicID;
	int nRound;
	std::list<stPointxy> Pointxy;
	stMagicPoint()
	{
		nMagicID = 0;
		nRound = 0;
		Pointxy.clear();
	}
	void clear()
	{
		nMagicID = 0;
		nRound = 0;
		Pointxy.clear();
	}
};


struct stMagicPointRand
{
	int nRandNum;
	std::map<int, stPointxy> Pointxy;
	stMagicPointRand()
	{
		nRandNum = 0;
		Pointxy.clear();
	}
	void clear()
	{
		nRandNum = 0;
		Pointxy.clear();
	}
};
class CUserEngine : public Singleton<CUserEngine>
{
	std::CSyncList<stItem*> m_mailedstItems;
public:
	boost::object_pool<stCretMove> m_cretMovePool;
	boost::object_pool<stCretAttack> m_cretAttackPool;
	boost::object_pool<CItem> m_itemPool;
	std::atomic<uint32_t> m_dwThisGameSvrVersion;
	bool m_boStartup;
	CPlayersHashManager m_loadokplayers;
	ULONGLONG	m_processloadokplayertick;
	CPlayersHashManager m_playerhash;			//游戏中玩家列表
	CMapHashManager m_maphash;					//地图列表
	CScriptSystem m_scriptsystem;               //脚本
	SharedMemory<SharedData> m_shareData;
	CQuestInfo m_globalquestinfo;				//全局任务信息表
	CQuestList m_globalquestlist;				//全局任务表，可以负载一些活动，全服任务
	CScriptTimerManager m_globaltimer;			//全局计时器
	CScriptSql m_scriptsql;						//脚本执行数据库

	std::CSyncList<stGmCmd>	m_cmdstrlist; 
	std::CSyncList<stRmbAdd> m_rmbaddlist;
	std::CSyncList<stQueueMsg*> m_syncmsglist;
	std::CSyncList<stQueueMsg*> m_runsyscmsglist;
	std::CSyncList<stOnlineNum> m_onlinenumlist;	//统计在线人数
	std::CSyncQueue<stDelayChatCMD*> m_delaychatcmdlist;//延迟处理的GM命令队列
	bool m_boIsShutDown;
	time_t m_shutdowntime;
	time_t m_lasthintshutdowntime;
	std::string m_shutdowndis;
	std::SyncHashMap<int64_t,DWORD> m_hCrossShutDownKick;
	std::CSyncList<DWORD> m_FBmapid;
	DWORD m_gLogLevel;				//日志要记录的等级
	std::map<DWORD,DWORD> m_mKeepHpSet;	//回血物品
	std::map<DWORD,stGSALLGuild> m_mGuilds;
	std::vector<std::vector<stHumanRankInfo>> m_cListV;		//排行榜
	std::vector<std::vector<stHumanRankInfo>> m_cGGListV;	//跨服排行榜
	std::vector<stUserKillLog> m_UserKillV;
	int64_t m_i64LevelMaxPlayer;		//等级最高的玩家
	std::CSyncVector<int64_t> m_vGetLuaErrorPlayer;				//需要返回给前端lua错误的玩家列表
	SetWrapper<std::string> m_abilityKeys; // Lua属性效果Key配置
	SetWrapper<std::string> m_abilityTimeKeys; // Lua属性限时效果Key配置
	SetWrapper<std::string> m_specialAbilityKeys; // Lua特殊属性效果Key配置
	CSkyLadder m_SkyLadder;					//天梯系统
	CGuildFortress m_GuildFortress;			//军团要塞
	std::map<BYTE, std::set<__int64>> m_aucPlayer;  // 可参与分红的玩家id
	DynamicMap<int, int> m_bossRefreshTime; //当前线的boss刷新时间戳，bossid,time

	int m_openDay = 0;
	int m_crossVersion = 0;
	bool m_isRebooting = false;
	bool m_updateWorld = false;
	bool m_worldDayCall = false;
	int m_world = 0;
	DynamicMap<int,bool> m_actFirstOpen;
	DynamicMap<int,bool> m_actFirstClose;
	DynamicMap<std::string, int> m_globalVars;
	#pragma region lua配置
	int m_nNengLimit;					// 能量上限
	int m_nMiningLimit;					// 挖矿能量每日上限
	std::map<int, int> m_OreEnergy;				   // 矿石消耗能量
	#pragma endregion
	DWORD m_dwFPS;

	std::CSyncQueue<stDelayCallFunction*> m_delaycallfunction;
	std::string packstr;
	int m_CheckMinSec;
	std::map<int, std::map<int, stMagicPoint> > m_mapMagicPoint;
	std::map<int, stMagicPointRand> m_mapMagicPointRand;
	bool RMBUnusual;
	CUserEngine();
	~CUserEngine();
	CPlayerObj* AddLoadOkPlayer(CPlayerObj* PlayObject,bool isselok);
	CPlayerObj* MakeNewHuman(CGameGateWayUser* pGateUser,stLoadPlayerData* pgamedata,bool boIsChangeSvr=false);
	void LoadGM(CPlayerObj* PlayObject, stLoadPlayerData* pgamedata);
	bool InitNewHuman(CPlayerObj* PlayObject);
	bool RemoveCreature(CCreature* pCret);
	enum{
		_MOVE_TYPE_NONE_=0,		//有动画
		_MOVE_TYPE_SKILL_=1,	//技能,有动画
		_MOVE_TYPE_GOHOME=2,	//回城,有动画
		_MOVE_TYPE_MAPGATE_=3,	//没有动画
	};
	bool SendMsg2SuperSvr(void *pbuf,int nLen);					//发送消息给superserver
#ifndef _SKIP_GAME_PROXY_
	bool SendMsg2GameSvrProxy(void *pbuf,int nLen);
#endif
	bool BroadcastGameSvr(void *pbuf,int nLen,DWORD dwSvrIdType=0,bool exceptme=true,DWORD dwZoneid = 0xFFFFFFFF,WORD wTradeId=0);
	bool SendMsg2GlobalProxy(void *pbuf, int nLen);
	bool SendMsg2GmManageSvr(void* pbuf, int nLen);				//发送消息给GM管理服务器
	void SendMsg2AllUser(void *pbuf,int nLen);					//发送消息给本服务器所有人
	bool SendProxyMsg2Gamesvr(void *pbuf,int nLen,DWORD dwSvrIdType=0,bool exceptme=true);					//发送其他游戏服务器
	bool SendProxyMsg2User(const char* szName,void *pbuf,int nLen);
	bool SendProxyMsg2User(int64_t i64OnlyId,void *pbuf,int nLen);
	bool SendMsg2GlobalSvr(void *pbuf,int nLen);				//发送消息给Globalsvr
	bool SendWrapperMsg2GGS(void *pbuf,int nLen, int64_t i64Onlyid = 0);				//发送消息给ggs
	bool SendMailMsg2Super(void *pbuf, int nLen, int64_t i64Onlyid = 0);				//发送消息给super
	bool SendMsg2TencentApiSvr(void *pbuf,int nLen);			//发送消息给TencentApiSvr
	bool SendProxyMsg2GlobalSvrUser(const char* szName,void *pbuf,int nLen);
	bool SendCrossMsg(WORD trueZoneID, WORD svrID, void *pBuf, int nLen);	//发送跨服消息
	void addLuaErrorPlayer(CPlayerObj* pPlayer);
	CPlayerObj* IsPlaying(const char* szName);				
	bool Startup();
	bool Cleanup();
	DWORD getMapCloneId();
	void putMapCloneid(DWORD id);
	bool ProcessSyncMsgs();
	bool ProcessExecCmd();
	bool ProcessLoadOkPlayers();
	bool ProcessSpaceMovePlayers();								//飞行的人物的进程
	void ProgressShutDown();
	void run();
	void doSuperSvrCmd(stBaseCmd* pcmd, unsigned int ncmdlen);	//处理从supergame过来的消息
	void doGameSvrProxyCmd(stBaseCmd* pcmd, unsigned int ncmdlen);
	void doProxyMsgRet(stBaseCmd* pcmd, unsigned int ncmdlen,stBaseCmd* ppluscmd,int npluscmdlen);
	void doGlobalSvrCmd(stBaseCmd* pcmd, unsigned int ncmdlen); //处理从globalsvr过来的消息
	void doTencentApiSvrCmd(stBaseCmd* pcmd,unsigned int ncmdlen);//处理腾讯api服务器消息
	bool isCrossSvr(DWORD dwZoneid=0);
	void SendSystemMail(int64_t i64Onlyid, const char* szName, const char* szTitle, const char* szNotice, CItem* pItem = NULL);
	void SendSystemMail(int64_t i64Onlyid, const char* szName, const char* szTitle, const char* szNotice, stItem* item);
	void SendSystemMailByIDAndCount(int64_t i64Onlyid, const char* szName, const char* szTitle, const char* szNotice, DWORD dwBaseid,DWORD dwCount, int frommapid, const char* bornfrom, const char* szmaker);
	DWORD GetDiffDayNow(DWORD dwTime);		//获取参数与当前时间相距的天数
	void sendLuaError();
	void ReleasePItem(CItem *pItem,const char* szReleaseFrom);			//释放物品加入缓冲池
	void doDelayLuaCall();
	void addDelayLuaCall(emDelayLuaCall delaycall,int paramsCount, ...);
	void doChatCmdList();
	stItem* GetMailedItem();
	stItem* GetMailedItem(DWORD dwBaseid, DWORD dwCount, BYTE binding, int frommapid, const char* bornfrom, const char* szmaker);
	void PushMailedItem(stItem* item);
	void ClearMailedItem();
	void ClearMagicPoint();
	void SetMagicPoint(int nMagicID, const char* pSrc);
	void AddOneMagicPoint(int nMagicID, int nRound, int nxMin, int nxMax, int nyMin, int nYmax);
	stMagicPoint* GetMagicPoint(int nMagicID, int nRound);
	void SetMagicPointRand(int nMagicID, int nRandNum, const char* pSrc);
	void AddOneMagicPointRand(int nMagicID, int nRandNum, int nxMin, int nxMax, int nyMin, int nYmax);
	bool GetMagicPointRand(int nMagicID, std::list<stPointxy>& Result);
	DWORD GetSavedSharedData(SavedSharedData type);
	void SetSavedSharedData(SavedSharedData type,uint32_t value);
	void GetRandom(std::vector<int> RandSorce, int nRandNum, std::list<int>& Result);
	stAuction& GetAuc(BYTE index);
};