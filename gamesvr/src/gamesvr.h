#pragma once

#include "zSingleton.h"
#include "lookaside_alloc.h"
#include "network/iocp.h"
#include "network/tcptask.h"
#include "network/packet.h"
#include "endec/EncDec.h"
#include "WndBase.h"
#include "gamegatewaySession.h"
#include "loginsvrGameConnecter.h"
#include "superGameSvrConnecter.h"
#include "server_cmd.h"
#include "gamelogic/LocalDB.h"
#include "checknamesvrConnecter.h"
#include "GmServerManageConnecter.h"
#include "LogSvrConnecter.h"
#include <utils/IDGenerator.h>
#include "globalSvrConnecter.h"
#include "TencentApiGameSvrConnecter.h"
#ifndef _SKIP_GAME_PROXY_
#include "GameSvrProxyConnecter.h"
#else
#include "globalproxyConnecter.h"
#endif
extern zLogger chatlogger;
extern zLogger LeaderLogger;
//管理器
class GameTcpServices : public CMAsyncTcpAccepters {
public:
	GameTcpServices()
		: CMAsyncTcpAccepters("")
	{
	};
	virtual CLD_AsyncClientSocket* CreateAsyncClient(CMAsyncTcpAccepters::CSubAccepter* pAccepter, SOCKET s);
	virtual void OnAsyncClientConnect(CLD_Socket* Socket);
	virtual void OnClientDisconnect(CLD_Socket* Socket);
	virtual void OnClientError(CLD_Socket* Socket, CLD_TErrorEvent ErrEvent, OUT int& nErrCode, char* sErrMsg);
	//virtual void OnClientRead(CLD_Socket* Socket);
};

#define _SVR_PARAM_DBHASHCODE_			0
#define _GM_LOCALDB_HASHCODE_			2

#define _BASECCREATURE_32_TMPID_		0xFFFFFFFE


class GameService : public Singleton< GameService>,public CWndBase {
public:
	//////////////////////////////////////////////////////////////////////////
	void EnableCtrl( bool bEnableStart );
	bool CreateToolbar();

	virtual void OnStartService();
	virtual void OnStopService();
	virtual void OnConfiguration();

	virtual void OnQueryClose(bool& boClose);
	virtual bool OnInit();
	virtual void OnUninit();
	virtual long OnTimer( int nTimerID );
	virtual long OnCommand( int nCmdID );
	virtual bool OnCommand( char* szCmd );
	virtual bool Init( HINSTANCE hInstance );
	virtual void OnClearLog();
	virtual void OnIdle();
	virtual void OnSafeClose();
	virtual void OnDolua();
	//////////////////////////////////////////////////////////////////////////
public:
	GameTcpServices m_TcpServices;					//tcp服务器管理器  继承与CMIocpTcpAccepter
	CMAsyncTcpConnters m_TcpConnters;				//对于连接者的管理

	static bool m_boStartService;					//是否开始服务

#ifdef _GLOBAL_GAMESVR_
	static const WORD m_svrtype=_GLOBAL_GAMESVR_TYPE;		//游戏服务器类型
#else
	static const WORD m_svrtype=_GAMESVR_TYPE;		//游戏服务器类型
#endif // _GLOBAL_GAMESVR_

	WORD m_svridx;									//服务器ID
	uint8_t m_lineId;
	char m_szsvrcfgdburl[MAX_PATH];					//游戏服务器cfg数据地址
	char m_szgamesvrparamdburl[MAX_PATH];			//游戏服务器数据库地址
	char m_szgamecfgdburl[MAX_PATH];				//游戏数据地址
	char m_szremotetestsvrurl[MAX_PATH];
	char m_szSvrName[MAX_PATH];						//服务器名字
	bool m_boConnectManage;							//是否连接游戏管理服务器
	bool m_boAutoStart;								//是否自动开始
	int m_niocpworkthreadcount;						//IOCP的线程数
	stServerConfig m_svrcfg;
	std::atomic<uint32_t> m_itemTmpId;				//物品临时ID
	CIntLock m_tmplock;								//临时ID分配锁定
	char m_szQuestScriptPath[_MAX_PATH];
	char m_szTureIp[_MAX_PATH];						//转发用的外网ip或者域名
	char m_szLocalIp[_MAX_IP_LEN_];					//本地IP
	char m_szZoneName[_MAX_NAME_LEN_];
	char m_szMapTableName[_MAX_PATH];
	int  m_nZoneid;
	WORD m_gatewayport;								//网关端口
	WORD m_dbport;									//数据库端口
	WORD m_gameport;								//游戏端口

	char m_szAuthIp[_MAX_IP_LEN_];
	WORD m_nAuthPort;

	bool m_boshutdown;								//判断是否已经关闭
	time_t m_timeshutdown;							//关闭时间
	bool m_forceclose;								//是否被强制关闭
	zXMLParser::ResStringHash m_xStrResList;                  //资源列表
	zXMLParser::ResStringHash  m_xScriptStrResList;           //脚本资源列表
	BYTE m_boForceCloseGetWay;
	bool m_boAllToOne;			//开启一区多服模式
	//使用 cfg_lock 保护
	CIntLock cfg_lock;								//项目中尽量不要包含其他锁
	svrinfomap m_svrlistmap;						//所有允许连接的服务器列表
	std::vector<stIpInfoState>  m_gateipinfos;					//typedef std::map< DWORD,stIpInfoState > ipinfostatemap;
	svrpublickeymap m_svrpublickeys;				//typedef std::map<DWORD,stSvrPublicKey>  svrpublickeymap;

	stGateTcpStateChangeCmd* m_GateTcpStateChangeCmd;	//网关客户端连接数量改变时发送到loginsvr(只在连接上的时候发一次)
	unsigned int m_GateTcpStateChangeCmdSize;

	stNotifMapListChangeCmd* m_NotifMapListChangeCmd;	//该服务器加载的地图改变时 发给loginsvr 和 gatesvr 
	unsigned int m_NotifMapListChangeCmdSize;

	stNotifLoginsvrPublickeyCmd* m_NotifLoginsvrPublickeyCmd;		//将所有loginsvr加密密码转发到所有网关
	unsigned int m_NotifLoginsvrPublickeyCmdSize;	

	stSvr2SvrLoginCmd m_Svr2SvrLoginCmd;		//登陆帐号服务器

	CWaitLoginHashManager m_waitloginhash;		//等待登陆的玩家列表
	CWaitPlayerChangeSvrHashManager m_waitchangesvrhash;		//等待切换服务器的玩家

	CLD_DBConnPool	m_svrdatadb;				//游戏参数存档数据库(物品ID 运行时间) + 游戏配置数据库(地图 物品 刷怪)

	int  m_nTradeid;
	char m_szTradeName[_MAX_NAME_LEN_];
	BYTE m_btAllisGm;
	DWORD m_dwmaxsvrid;
	DWORD m_dwminsvrid;
	BYTE m_btGmOnlineWDYS;
	int m_nTrueZoneid;
	
	std::CSyncSet<CGameGatewaySvrSession*> m_gatewaysession;						//游戏网关连接Map

	CLoginSvrGameConnecter* m_loginsvrconnter;				//登陆服务器连接Map

	CDBSvrGameConnecter* m_dbsvrconnecter;					//数据库服务器连接Map

	//CIntLock m_toplock;							//不要被其他锁包含
	CSuperGameSvrConnecter*	m_supergamesvrconnecter;							//区管理服务器连接
#ifndef _SKIP_GAME_PROXY_
	CGameSvrProxyConnecter* m_pGameSvrProxyConnecter;
#else
	CGlobalProxyConnecter* m_globalProxyConnecter;								//一机多服模式下直接连globalproxy
#endif
	CCheckNameSvrConnecter* m_checknamesvrconnecter;							//验证服务器
	CGlobalSvrConnecter* m_globalsvrconnecter;									//global社交服务器
	CTencentApiGameSvrConnecter* m_tencentapigamesvrconnecter;					//腾讯API游戏服务器连接

	std::vector<CLogSvrConnecter*>	m_logsvrconnecters;										//日志服务器连接
	CGmServerManageConnecter* m_gmservermanageconnecter;						//游戏管理服务器连接

	std::CSyncSet<CGameGateWayUser*> m_waitdelgateuser;							//等待删除的网关user
	stSetDbcolCmd* m_pSetDbcolCmd;
	size_t m_pSetDbcolCmdLen;
	DWORD m_gamesvrStateMask;		//其他游戏服务器的状态,是否处于可用中

	GameService();
	~GameService();

	void NotifyWeiXin(std::string content) const;
	void StartService();
	void RefSvrRunInfo();
	void ShutDown();
	void StopService();
	bool ReloadSvrConfig();
	bool exec_input_cmd(void* currobj,char* szCmd,stGmRetExecCmd* retcmd,int nretmax);
	bool GetALoginIpInfo(in_addr& ip,WORD& port,BYTE ip_type,char* pszTGWip);
	bool Send2LoginSvrs(void* pcmd,int ncmdlen);								//回复消息给LoginServer
	bool Send2DBSvrs(void* pcmd,int ncmdlen);									//
	bool SendSvrCmd2GatewaySvrs(void* pcmd,int ncmdlen);						//回复消息GateWayServer
	bool Send2TencentSvrs(const char* pszOpenId,const char* pszFunName,const char* szClientIp,bool boForceRefresh);
	CDBSvrGameConnecter* GetDBSvrByName(const char* szName);					//获得数据库服务器的连接指针
	bool BatchSendToGate(stProxyBroadcastUserIndex* puserindexs, int nuser, void* pbuf, unsigned int nsize, int zliblevel = Z_DEFAULT_COMPRESSION);
	bool Send2LogSvr(int nLogType,int nSubLogType,int execType,CPlayerObj* pPlayer,const char* fmtlogstr,...);
	void reloadStrRes();
	const char* GetStrRes(int dwIndex, std::string strType="system");
	const char* GetLuaStrRes(int dwIndex, std::string strType="system");
	virtual void RecycleThreadCallBack();

	DWORD grouptmpid();															//获得队伍临时id
	uint64_t ItemIdGenerate();

	bool LoadLocalConfig();
	bool LoadSvrConfig(CSqlClientHandle* sqlc);
	bool LoadServerParam();
	bool SaveServerParam(int naddtime,int nadditemid);

	CLD_ThreadBase* m_msgProcessThread;
	CLD_ThreadBase* m_LogicProcessThread;
	ItemIDGenerator m_itemIdGen;	//物品ID生成器

	unsigned int __stdcall SimpleMsgProcessThread(CLD_ThreadBase* pthread,void* param);
	unsigned int __stdcall LogicProcessThread(CLD_ThreadBase* pthread,void* param);
	unsigned int __stdcall ScriptSqlThread(CLD_ThreadBase* pthread,void* param);

	bool Add2Delete(CGameGateWayUser* pGateUser){
		if (pGateUser){
			pGateUser->notifyGateRemoveUser();
			m_waitdelgateuser.s_insert(pGateUser);
		}
		return true;
	}
private:
	virtual void thRun(){RecycleThreadCallBack();};
};
