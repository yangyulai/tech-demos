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
//������
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
	GameTcpServices m_TcpServices;					//tcp������������  �̳���CMIocpTcpAccepter
	CMAsyncTcpConnters m_TcpConnters;				//���������ߵĹ���

	static bool m_boStartService;					//�Ƿ�ʼ����

#ifdef _GLOBAL_GAMESVR_
	static const WORD m_svrtype=_GLOBAL_GAMESVR_TYPE;		//��Ϸ����������
#else
	static const WORD m_svrtype=_GAMESVR_TYPE;		//��Ϸ����������
#endif // _GLOBAL_GAMESVR_

	WORD m_svridx;									//������ID
	uint8_t m_lineId;
	char m_szsvrcfgdburl[MAX_PATH];					//��Ϸ������cfg���ݵ�ַ
	char m_szgamesvrparamdburl[MAX_PATH];			//��Ϸ���������ݿ��ַ
	char m_szgamecfgdburl[MAX_PATH];				//��Ϸ���ݵ�ַ
	char m_szremotetestsvrurl[MAX_PATH];
	char m_szSvrName[MAX_PATH];						//����������
	bool m_boConnectManage;							//�Ƿ�������Ϸ���������
	bool m_boAutoStart;								//�Ƿ��Զ���ʼ
	int m_niocpworkthreadcount;						//IOCP���߳���
	stServerConfig m_svrcfg;
	std::atomic<uint32_t> m_itemTmpId;				//��Ʒ��ʱID
	CIntLock m_tmplock;								//��ʱID��������
	char m_szQuestScriptPath[_MAX_PATH];
	char m_szTureIp[_MAX_PATH];						//ת���õ�����ip��������
	char m_szLocalIp[_MAX_IP_LEN_];					//����IP
	char m_szZoneName[_MAX_NAME_LEN_];
	char m_szMapTableName[_MAX_PATH];
	int  m_nZoneid;
	WORD m_gatewayport;								//���ض˿�
	WORD m_dbport;									//���ݿ�˿�
	WORD m_gameport;								//��Ϸ�˿�

	char m_szAuthIp[_MAX_IP_LEN_];
	WORD m_nAuthPort;

	bool m_boshutdown;								//�ж��Ƿ��Ѿ��ر�
	time_t m_timeshutdown;							//�ر�ʱ��
	bool m_forceclose;								//�Ƿ�ǿ�ƹر�
	zXMLParser::ResStringHash m_xStrResList;                  //��Դ�б�
	zXMLParser::ResStringHash  m_xScriptStrResList;           //�ű���Դ�б�
	BYTE m_boForceCloseGetWay;
	bool m_boAllToOne;			//����һ�����ģʽ
	//ʹ�� cfg_lock ����
	CIntLock cfg_lock;								//��Ŀ�о�����Ҫ����������
	svrinfomap m_svrlistmap;						//�����������ӵķ������б�
	std::vector<stIpInfoState>  m_gateipinfos;					//typedef std::map< DWORD,stIpInfoState > ipinfostatemap;
	svrpublickeymap m_svrpublickeys;				//typedef std::map<DWORD,stSvrPublicKey>  svrpublickeymap;

	stGateTcpStateChangeCmd* m_GateTcpStateChangeCmd;	//���ؿͻ������������ı�ʱ���͵�loginsvr(ֻ�������ϵ�ʱ��һ��)
	unsigned int m_GateTcpStateChangeCmdSize;

	stNotifMapListChangeCmd* m_NotifMapListChangeCmd;	//�÷��������صĵ�ͼ�ı�ʱ ����loginsvr �� gatesvr 
	unsigned int m_NotifMapListChangeCmdSize;

	stNotifLoginsvrPublickeyCmd* m_NotifLoginsvrPublickeyCmd;		//������loginsvr��������ת������������
	unsigned int m_NotifLoginsvrPublickeyCmdSize;	

	stSvr2SvrLoginCmd m_Svr2SvrLoginCmd;		//��½�ʺŷ�����

	CWaitLoginHashManager m_waitloginhash;		//�ȴ���½������б�
	CWaitPlayerChangeSvrHashManager m_waitchangesvrhash;		//�ȴ��л������������

	CLD_DBConnPool	m_svrdatadb;				//��Ϸ�����浵���ݿ�(��ƷID ����ʱ��) + ��Ϸ�������ݿ�(��ͼ ��Ʒ ˢ��)

	int  m_nTradeid;
	char m_szTradeName[_MAX_NAME_LEN_];
	BYTE m_btAllisGm;
	DWORD m_dwmaxsvrid;
	DWORD m_dwminsvrid;
	BYTE m_btGmOnlineWDYS;
	int m_nTrueZoneid;
	
	std::CSyncSet<CGameGatewaySvrSession*> m_gatewaysession;						//��Ϸ��������Map

	CLoginSvrGameConnecter* m_loginsvrconnter;				//��½����������Map

	CDBSvrGameConnecter* m_dbsvrconnecter;					//���ݿ����������Map

	//CIntLock m_toplock;							//��Ҫ������������
	CSuperGameSvrConnecter*	m_supergamesvrconnecter;							//���������������
#ifndef _SKIP_GAME_PROXY_
	CGameSvrProxyConnecter* m_pGameSvrProxyConnecter;
#else
	CGlobalProxyConnecter* m_globalProxyConnecter;								//һ�����ģʽ��ֱ����globalproxy
#endif
	CCheckNameSvrConnecter* m_checknamesvrconnecter;							//��֤������
	CGlobalSvrConnecter* m_globalsvrconnecter;									//global�罻������
	CTencentApiGameSvrConnecter* m_tencentapigamesvrconnecter;					//��ѶAPI��Ϸ����������

	std::vector<CLogSvrConnecter*>	m_logsvrconnecters;										//��־����������
	CGmServerManageConnecter* m_gmservermanageconnecter;						//��Ϸ�������������

	std::CSyncSet<CGameGateWayUser*> m_waitdelgateuser;							//�ȴ�ɾ��������user
	stSetDbcolCmd* m_pSetDbcolCmd;
	size_t m_pSetDbcolCmdLen;
	DWORD m_gamesvrStateMask;		//������Ϸ��������״̬,�Ƿ��ڿ�����

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
	bool Send2LoginSvrs(void* pcmd,int ncmdlen);								//�ظ���Ϣ��LoginServer
	bool Send2DBSvrs(void* pcmd,int ncmdlen);									//
	bool SendSvrCmd2GatewaySvrs(void* pcmd,int ncmdlen);						//�ظ���ϢGateWayServer
	bool Send2TencentSvrs(const char* pszOpenId,const char* pszFunName,const char* szClientIp,bool boForceRefresh);
	CDBSvrGameConnecter* GetDBSvrByName(const char* szName);					//������ݿ������������ָ��
	bool BatchSendToGate(stProxyBroadcastUserIndex* puserindexs, int nuser, void* pbuf, unsigned int nsize, int zliblevel = Z_DEFAULT_COMPRESSION);
	bool Send2LogSvr(int nLogType,int nSubLogType,int execType,CPlayerObj* pPlayer,const char* fmtlogstr,...);
	void reloadStrRes();
	const char* GetStrRes(int dwIndex, std::string strType="system");
	const char* GetLuaStrRes(int dwIndex, std::string strType="system");
	virtual void RecycleThreadCallBack();

	DWORD grouptmpid();															//��ö�����ʱid
	uint64_t ItemIdGenerate();

	bool LoadLocalConfig();
	bool LoadSvrConfig(CSqlClientHandle* sqlc);
	bool LoadServerParam();
	bool SaveServerParam(int naddtime,int nadditemid);

	CLD_ThreadBase* m_msgProcessThread;
	CLD_ThreadBase* m_LogicProcessThread;
	ItemIDGenerator m_itemIdGen;	//��ƷID������

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
