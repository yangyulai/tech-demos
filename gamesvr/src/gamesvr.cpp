#include "cfgparser.h"
#include "cmd/TencentApi_cmd.h"
#include "cmdparse.h"
#include "dbsvrGameConnecter.h"
#include "gamelogic/Chat.h"
#include "gamelogic/UsrEngn.h"
#include "gamesvr.h"
#include "HardWare.h"
#include "qglobal.h"
#include "res/resource.h"
#include "server_cmd.h"
#include "timeMonitor.h"
#include "winapiini.h"
#include "WndBase.h"
#include "zLogger.h"
#include <cstdio>
#include <ctime>
#include <dbghelp.h>
#include <fstream>
#include <iostream>
#include <Ole2.h>
#include <windows.h>
#include <network/WinHttpClient.h>

zLogger chatlogger(std::string(""));
zLogger LeaderLogger(std::string(""));
fnSaveGMOrder g_SaveGMOrder=NULL;

#pragma warning(disable:4239)		//使用了非标准扩展 : “参数” : 从“GameService::RecycleThreadCallBack::stcheckSession”转换到“zHashManagerBase<valueT,e1>::removeValue_Pred_Base &”
#define _GATEWAY_CONN_				0

#define _LOGIN_SVR_CONNETER_		0
#define _DB_SVR_CONNETER_			1
#define _SUPERGAME_SVR_CONNETER_	2
#define _CHECKNAME_SVR_CONNETER_	3
#define _LOG_SVR_CONNETER_			4
#define _SCRIPTGAME_SVR_CONNETER_	5
#define _GLOBAL_SVR_CONNECTER		6
#define _TENCENT_SVR_CONNECTER		7
#define _GAMEPROXY_SVR_CONNECTER		8

CLD_AsyncClientSocket* GameTcpServices::CreateAsyncClient(CMAsyncTcpAccepters::CSubAccepter* pAccepter, SOCKET s)
{
	FUNCTION_BEGIN;
	GameService* login=GameService::instance();
	if (login->m_boshutdown || CUserEngine::getMe().m_boIsShutDown|| !login->m_boStartService){
		return NULL;
	}
	switch (pAccepter->gettype())
	{
	case _GATEWAY_CONN_:
		{
			svrinfomapiter it;
			bool bofind = false;	
			char szremoterip[32];
			CLD_Socket::GetRemoteAddress(s,szremoterip,sizeof(szremoterip));		//获得远程连接的地址
			do{
				AILOCKT(login->cfg_lock);											
				for (it = login->m_svrlistmap.begin(); it != login->m_svrlistmap.end(); it++) {
					stServerInfo* psvr = &it->second;
					if (stricmp(psvr->szIp, szremoterip) == 0 && psvr->svr_type==_GAMESVR_GATEWAY_TYPE) {
						//只允许网关连接该端口
						bofind = true;
						break;
					}
				}
			}while (false);
			if (bofind) {
				CGameGatewaySvrSession* pqpsocket = CLD_DEBUG_NEW CGameGatewaySvrSession(pAccepter, s);
				login->m_gatewaysession.s_insert(pqpsocket);
				return (CLD_AsyncClientSocket *) pqpsocket;
			} else {
				g_logger.warn("%s : %d 非法的服务器连接...", szremoterip, CLD_Socket::GetRemotePort(s));
			}
		}
		break;
	}
	return NULL;
}

void GameTcpServices::OnAsyncClientConnect(CLD_Socket* Socket)
{
	FUNCTION_BEGIN;
	CMAsyncTcpAccepters::CSubAccepter* paccept = (CMAsyncTcpAccepters::CSubAccepter*) ((CLD_AsyncClientSocket*) Socket)->GetAccepter();
	if (paccept){
		g_logger.debug("%s:%d(%s) 连接成功...", Socket->GetRemoteAddress(), Socket->GetRemotePort(), paccept->getdis());
	}else{
		g_logger.debug("%s:%d 连接成功...", Socket->GetRemoteAddress(), Socket->GetRemotePort());
	}
}

void GameTcpServices::OnClientDisconnect(CLD_Socket* Socket)
{
	FUNCTION_BEGIN;
	CMAsyncTcpAccepters::CSubAccepter* pAccepter = (CMAsyncTcpAccepters::CSubAccepter*) ((CLD_AsyncClientSocket*) Socket)->GetAccepter();
	switch (pAccepter->gettype())
	{
	case _GATEWAY_CONN_:
		{
			//网关断开连接
			GameService* gamesvr=GameService::instance();

			CGameGatewaySvrSession* pGateway=(CGameGatewaySvrSession*)Socket;
			pGateway->SetAllUserSocketClose();

			//通知所有服务器该网关关闭
			BUFFER_CMD(stGateTcpStateChangeCmd, tmpGateTcpStateChangeCmd, 1024 * 8);//支持10个网关
			tmpGateTcpStateChangeCmd->ipinfostates.clear();
			do{
				std::vector<stIpInfoState>& gateipinfos=gamesvr->m_gateipinfos ;
				AILOCKT(gamesvr->cfg_lock);
				gamesvr->m_GateTcpStateChangeCmd->ipinfostates.clear();
				for (size_t j=0;j<gateipinfos.size();j++){
					if ( gateipinfos[j].svr_id==pGateway->svr_id ){
						gateipinfos[j].state=(BYTE)-1;
						gateipinfos[j].ncount=0;
					}
					gamesvr->m_GateTcpStateChangeCmd->ipinfostates.push_back(gateipinfos[j],__FUNC_LINE__);
					tmpGateTcpStateChangeCmd->ipinfostates.push_back(gateipinfos[j],__FUNC_LINE__);
				}
				gamesvr->m_GateTcpStateChangeCmdSize=sizeof(*gamesvr->m_GateTcpStateChangeCmd)+gamesvr->m_GateTcpStateChangeCmd->ipinfostates.getarraysize();
			}while(false);
			gamesvr->Send2LoginSvrs(tmpGateTcpStateChangeCmd,sizeof(*tmpGateTcpStateChangeCmd)+tmpGateTcpStateChangeCmd->ipinfostates.getarraysize());
			gamesvr->Send2DBSvrs(tmpGateTcpStateChangeCmd,sizeof(*tmpGateTcpStateChangeCmd)+tmpGateTcpStateChangeCmd->ipinfostates.getarraysize());
			gamesvr->m_gatewaysession.s_erase(pGateway);

		}break;
	}
	g_logger.debug("%s:%d(%s) 连接断开...", Socket->GetRemoteAddress(), Socket->GetRemotePort(), pAccepter->getdis());
}

void GameTcpServices::OnClientError(CLD_Socket* Socket, CLD_TErrorEvent ErrEvent, OUT int& nErrCode, char* sErrMsg)
{
	FUNCTION_BEGIN;
	g_logger.debug("%s:%d 连接异常(%d->%s)...", Socket->GetRemoteAddress(), Socket->GetRemotePort(), nErrCode, sErrMsg);
	CMAsyncTcpAccepters::CSubAccepter* pAccepter = (CMAsyncTcpAccepters::CSubAccepter*) ((CLD_AsyncClientSocket*) Socket)->GetAccepter();
	if (nErrCode!=0){
		switch (pAccepter->gettype())
		{
		case _GATEWAY_CONN_:
			{
				//网关断开连接
				((CGameGatewaySvrSession*)Socket)->Terminate(__FF_LINE__);
			}
			break;
		}
		nErrCode=0;
	}
}

void init_loadplayer_dbcol(stSetDbcolCmd* m_pSetDbcolCmd) {
	dbColProxy tmpdbcol;
	auto size = std::size(stLoadPlayerData_define);
	for (size_t i = 0;i<size;++i){
		const dbCol* p = &stLoadPlayerData_define[i];
		tmpdbcol.getdbCol()->clone(p);
		if (p && p->name &&
			(stricmp(p->name, "createtime") == 0
				|| stricmp(p->name, "GM等级") == 0
				|| stricmp(p->name, "useronlyid") == 0)
			) {
			tmpdbcol.getdbCol()->state = (tmpdbcol.getdbCol()->state | _DBCOL_NOT_WRITE_);
		}
		m_pSetDbcolCmd->dbcols.push_back(tmpdbcol, __FUNC_LINE__);
	}
}

//////////////////////////////////////////////////////////////////////////
GameService::GameService()
	:CWndBase(WND_WIDTH,WND_HEIGHT*2,"GameService_Wnd")
{
	m_boForceCloseGetWay = 0;
	m_timeshutdown=0;
	m_forceclose=false;

	m_niocpworkthreadcount=0;
	m_boStartService = false;
	m_boshutdown=false;
	m_svridx = 0;
	m_szsvrcfgdburl[0]=0;
	m_szgamesvrparamdburl[0]=0;
	m_szgamecfgdburl[0]=0;
	m_szremotetestsvrurl[0]=0;
	m_szQuestScriptPath[0]=0;
	m_szSvrName[0]=0;
	m_szZoneName[0]=0;
	m_szTureIp[0] = 0;
	m_boConnectManage=false;
	m_nZoneid=0;
	m_nTradeid=0;
	m_dwmaxsvrid=0;
	m_dwminsvrid=0;
	m_szAuthIp[0] = 0;
	m_nAuthPort = 18888;
	m_szTradeName[0]=0;
	m_gamesvrStateMask = 0;
	m_supergamesvrconnecter=NULL;
#ifndef _SKIP_GAME_PROXY_
	m_pGameSvrProxyConnecter=NULL;
#else
	m_globalProxyConnecter = NULL;
#endif
	m_checknamesvrconnecter=NULL;
	m_globalsvrconnecter=NULL;
	m_tencentapigamesvrconnecter=NULL;

	m_logsvrconnecters.clear();
	m_gmservermanageconnecter=NULL;
	m_loginsvrconnter = NULL;
	m_dbsvrconnecter = NULL;

	m_szMapTableName[0]=0;
	m_msgProcessThread=NULL;
	m_LogicProcessThread=NULL;
	//////////////////////////////////////////////////////////////////////////
	m_Svr2SvrLoginCmd.svr_id = m_svridx;
	m_Svr2SvrLoginCmd.svr_type = m_svrtype;

	static char SvrMapsBuf[1024 * 256];
	m_NotifMapListChangeCmd = (stNotifMapListChangeCmd *) &SvrMapsBuf;
	constructInPlace(m_NotifMapListChangeCmd);
	m_NotifMapListChangeCmd->svrmaps.clear();
	m_NotifMapListChangeCmdSize=sizeof(*m_NotifMapListChangeCmd);


	static char GateTcpStateBuf[1024 * 8];
	m_GateTcpStateChangeCmd = (stGateTcpStateChangeCmd *) &GateTcpStateBuf;
	constructInPlace(m_GateTcpStateChangeCmd);
	m_GateTcpStateChangeCmd->ipinfostates.clear();
	m_GateTcpStateChangeCmdSize=sizeof(*m_GateTcpStateChangeCmd);

	static char NotifLoginsvrPublickesCmd[1024 * 16];
	m_NotifLoginsvrPublickeyCmd = (stNotifLoginsvrPublickeyCmd *) &NotifLoginsvrPublickesCmd;
	constructInPlace(m_NotifLoginsvrPublickeyCmd);
	m_NotifLoginsvrPublickeyCmd->svrpublickeys.clear();
	m_NotifLoginsvrPublickeyCmdSize=sizeof(*m_NotifLoginsvrPublickeyCmd);

	static char SetDbcolCmd[1024 * 64];
	m_pSetDbcolCmd= (stSetDbcolCmd *) &SetDbcolCmd;
	constructInPlace(m_pSetDbcolCmd);
	init_loadplayer_dbcol(m_pSetDbcolCmd);
	m_pSetDbcolCmdLen=sizeof(*m_pSetDbcolCmd)+m_pSetDbcolCmd->dbcols.getarraysize();
	stLoadPlayerData::initsize();
	m_pSetDbcolCmd->nbindataoffset=stLoadPlayerData::s_bindataoffset;
	m_pSetDbcolCmd->ndatasize=stLoadPlayerData::s_datasize;
	//////////////////////////////////////////////////////////////////////////
	LoadLocalConfig();
	char szTemp[MAX_PATH];
	char szruntime[20];
	timetostr(time(NULL),szruntime,20);
	sprintf_s(szTemp,MAX_PATH,"[%s : GameSvr](BuildTime: %s)-(RunTime: %s)",m_szSvrName,__BUILD_DATE_TIME__,szruntime);
	SetTitle(szTemp);
	m_boAllToOne = false;
}

HMODULE g_AutoCompleteh=0;
GameService::~GameService(){
	FUNCTION_BEGIN;
	if (g_AutoCompleteh){
		FreeLibrary(g_AutoCompleteh);
		g_AutoCompleteh=0;
	}
	m_xStrResList.clear();
}

static bool s_bosavesvrparam=false;

void GameService::RecycleThreadCallBack()
{
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true,NULL);
	//只在回收线程中执行
	time_t curtime = time(NULL);
	if (m_boStartService) {
		static time_t s_timeAction=time(NULL)+2;
		if (curtime > s_timeAction) {
			//尝试激活所有已经断开的客户端
			m_TcpConnters.timeAction();
			s_timeAction = time(NULL) + 4;
		}
		//////////////////////////////////////////////////////////////////////////
		static time_t s_timeSaveSvrParam=time(NULL)+60*1;
		curtime = time(NULL);
		if ( curtime > s_timeSaveSvrParam && !m_boshutdown && !s_bosavesvrparam ){
			//如果是异常关闭 时间往后移动5分钟 物品ID 增加 3000(5分钟出3000个物品)
			m_svrcfg.dwsavesvrparaminterval=60*1;
			SaveServerParam( (m_svrcfg.dwsavesvrparaminterval+120) ,100*(m_svrcfg.dwsavesvrparaminterval+120) );
			s_timeSaveSvrParam=time(NULL)+m_svrcfg.dwsavesvrparaminterval;
		}
		//////////////////////////////////////////////////////////////////////////
		static time_t s_timeNotifySvrState=time(NULL)+60*2;
		static time_t s_timeadchannelstime=time(NULL)+60*5;
		static int s_lastOnlineUserCount=0;
		curtime = time(NULL);
		if (curtime > s_timeNotifySvrState){
			stNotifyGameSvrState Notify;
			Notify.nServerId = m_nZoneid;
			Notify.nServerIndex = m_svridx;
			Notify.nUserCount=CUserEngine::getMe().m_playerhash.size();
			if(s_lastOnlineUserCount!=Notify.nUserCount){
				Send2LoginSvrs(&Notify,sizeof(Notify));
				if (CUserEngine::getMe().isCrossSvr()) {
					CUserEngine::getMe().SendMsg2GlobalSvr(&Notify,sizeof(Notify));
				}
				s_lastOnlineUserCount=Notify.nUserCount;
			}
			if(CUserEngine::getMe().isCrossSvr()){
				s_timeNotifySvrState=time(NULL)+5;
			}else{
				s_timeNotifySvrState=time(NULL)+60*2;
			}
		}
	}
}

DWORD GameService::grouptmpid()
{
	FUNCTION_BEGIN;
	AILOCKT(m_tmplock);
	m_svrcfg.grouptmpid++;
	if (m_svrcfg.grouptmpid > 0x000ffffe)
	{
		m_svrcfg.grouptmpid = 1;
	}
	return ((m_svridx << 20) | m_svrcfg.grouptmpid);
}
uint64_t GameService::ItemIdGenerate()
{
	return m_itemIdGen.generate(m_itemTmpId);
}

bool ipinfousercount_cmp(const stIpInfoState& m1,const stIpInfoState& m2){
	return m1.ncount<m2.ncount;	//升序
}

bool GameService::GetALoginIpInfo(in_addr& ip,WORD& port,BYTE ip_type,char* pszTGWip){
	//DBService* login=DBService::instance();
	do{
		std::vector<stIpInfoState>& gateipinfos=m_gateipinfos ;
		AILOCKT(cfg_lock);
		//保证人数最少的端口始终在最前面
		if (gateipinfos.size()>=2){
			sort(gateipinfos.begin(),gateipinfos.end(),ipinfousercount_cmp);
		}
		int nmin=-1;
		int ntype_min=-1;
		for (size_t i=0;i<gateipinfos.size();i++){
			if ( gateipinfos[i].state==0 ){
				if(GameService::getMe().m_boForceCloseGetWay == 0 ||
					(GameService::getMe().m_boForceCloseGetWay == 2 && 
					stricmp(inet_ntoa(gateipinfos[i].ip),GameService::getMe().m_szLocalIp) != 0) || 
					(GameService::getMe().m_boForceCloseGetWay == 1 && 
					stricmp(inet_ntoa(gateipinfos[i].ip),GameService::getMe().m_szLocalIp) == 0)){
						if (nmin<0){ nmin=i;};
						if (gateipinfos[i].type==ip_type){
							ntype_min=i;
							break;
						}
				}
			}
		}
		if (ntype_min<0){ ntype_min=nmin;	}
		if (ntype_min>=0 && (size_t)ntype_min<gateipinfos.size()){
			g_logger.debug("重新分配网关 %s",inet_ntoa(gateipinfos[ntype_min].ip));
			strcpy_s(pszTGWip,_MAX_PATH-1,gateipinfos[ntype_min].szTGWip);
			ip=gateipinfos[ntype_min].ip;
			port=gateipinfos[ntype_min].port;
			gateipinfos[ntype_min].ncount++;
			return true;
		}
	} while (false);
	return false;
}

bool GameService::m_boStartService=false;

DWORD WINAPI OnExceptionBeginCallBack(LPEXCEPTION_POINTERS ExceptionInfo){
	FUNCTION_BEGIN;
	GameService* psvr=GameService::instance_readonly();
	if (psvr && GameService::m_boStartService==true){ psvr->SaveServerParam(10,100*10); }
	return 0;
}

void GameService::NotifyWeiXin(std::string content) const
{
	if (m_btAllisGm == 0 || m_svridx != 301 || m_nZoneid != 1001)
	{
		return;
	}
	std::string str = getcurworkpath();
	if (str != R"(D:\ARPG7Cool)")
	{
		return;
	}
	WinHttpClient client(L"qyapi.weixin.qq.com");
	const std::wstring resource =
		L"/cgi-bin/webhook/send?key=29a7d86c-721b-46b8-bbbc-e56f40bf21ac";
	const std::string request_body = vformat(R"({
        "msgtype": "text",
        "text": {
            "content": "%s"
        }
    })", content.c_str());
	const std::wstring content_type = L"application/json; charset=utf-8";
	std::string response_body;
	long status_code = 0;
	bool ok = client.Post(resource, request_body.c_str(), content_type, response_body, status_code);
	if (!ok) {
		g_logger.error("[Error] POST 请求失败: %s \n", client.GetLastErrorMessage().c_str());
	}
}
void GameService::StartService()
{
	FUNCTION_BEGIN;
	if (m_boStartService || !LoadLocalConfig()) {
		return;
	}
	g_logger.forceLog(zLogger::zINFO,"开始启动服务...");
	stUrlInfo cfgui(0, m_szsvrcfgdburl/*"msmdb://game:k3434yiuasdh848@127.0.0.1:0/\".\\db\\mydb_svrcfg.mdb\""*/, true);
	CSqlClientHandle* cfgsqlc = cfgui.newsqlclienthandle();
	if (cfgsqlc){
		if (cfgsqlc->setHandle()){
			if ( LoadSvrConfig(cfgsqlc) && LoadServerParam()) {
				m_boStartService = false;
				m_msgProcessThread=CThreadFactory::CreateBindClass(this,&GameService::SimpleMsgProcessThread,(void*)NULL);
				if (m_msgProcessThread){
					m_LogicProcessThread=CThreadFactory::CreateBindClass(this,&GameService::LogicProcessThread,(void*)NULL);
					if (m_LogicProcessThread){
						m_boStartService = true;
					}
				}
				if (!m_boStartService){
					m_boStartService = true;
					StopService();
					m_boStartService = false;
				}

			} else {
				m_boStartService = true;
				StopService();
				m_boStartService = false;
			}
		}
		cfgsqlc->unsetHandle();
		cfgsqlc->finalHandle();
		SAFE_DELETE(cfgsqlc);
	}
	if (m_boStartService){
		g_logger.forceLog(zLogger::zINFO,"启动服务成功...");
		if (CUserEngine::getMe().Startup())
		{
			m_TcpConnters.timeAction(true);

			m_msgProcessThread->Start(false);
			m_LogicProcessThread->Start(false);
			Send2LogSvr(0,0,0,NULL,"\'服务器启动服务!\'");

			if (CUserEngine::getMe().m_shareData.IsValid()) {
				auto data = CUserEngine::getMe().m_shareData.data();
				data->maxLineId = data->maxLineId < m_svridx ? m_svridx : data->maxLineId;
			}
			NotifyWeiXin((char*)u8"服务器启动成功");
		}else g_logger.forceLog(zLogger::zINFO,"启动服务失败...");
	} else {
		g_logger.forceLog(zLogger::zINFO,"启动服务失败...");
	}
}

void GameService::RefSvrRunInfo(){
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER_RESET(true);
	if (!m_boshutdown && !CUserEngine::getMe().m_boIsShutDown ){
		CThreadMonitor::getme().DebugPrintAllThreadDebufInfo();
	}

	DWORD dwgetwaitretsavecount=0,dwgetsavecount=0;
	do{
		if (m_dbsvrconnecter){
			dwgetwaitretsavecount+=m_dbsvrconnecter->getwaitretsavecount();
			dwgetsavecount+=m_dbsvrconnecter->getsavecount();
		}
	}while(false);

	extern size_t g_savedatamaxsize;
	extern size_t g_savedatazlibmaxsize;
	g_logger.forceLog(zLogger::zINFO,"UserCount : %u getwaitretsavecount : %u  dwgetsavecount : %u  SavedataMaxsize : %uk/%uk  "
		,CUserEngine::getMe().m_playerhash.size(),dwgetwaitretsavecount,dwgetsavecount,((g_savedatamaxsize/1024) +1),((g_savedatazlibmaxsize/1024) +1)  );
}

void GameService::ShutDown()
{
	FUNCTION_BEGIN;
	if (m_boshutdown){return;}
	if (!m_boStartService){return;}
	m_boshutdown=true;
	StopService();
	SaveServerParam(0,0);
	//RefSvrRunInfo();
	this->Processmsg();
	Sleep(2000);
}

void GameService::StopService()
{
	FUNCTION_BEGIN;
	if (!m_boStartService) {
		return;
	}
	g_logger.forceLog(zLogger::zINFO,"开始停止服务...");
	Send2LogSvr(0,0,0,NULL,"\'服务器停止服务!\'");

	do {
		//AILOCKT(m_toplock);
		if (m_LogicProcessThread){	m_LogicProcessThread->Suspend();}

		m_supergamesvrconnecter=NULL;
#ifndef _SKIP_GAME_PROXY_
		m_pGameSvrProxyConnecter=NULL;
#else
		m_globalProxyConnecter = NULL;
#endif
		m_checknamesvrconnecter=NULL;
		m_globalsvrconnecter=NULL;
		m_tencentapigamesvrconnecter=NULL;

		m_logsvrconnecters.clear();
		m_gmservermanageconnecter=NULL;
		if (m_LogicProcessThread){	m_LogicProcessThread->Resume();}
	} while (false);
	m_gatewaysession.s_clear();
	m_loginsvrconnter = NULL;
	m_TcpServices.closeall();
	m_TcpConnters.closeall();

	if (m_msgProcessThread){
		m_msgProcessThread->Terminate();
		m_msgProcessThread->Waitfor();
		SAFE_DELETE(m_msgProcessThread);
	}

	if (m_LogicProcessThread){
		m_LogicProcessThread->Terminate();
		m_LogicProcessThread->Waitfor();
		SAFE_DELETE(m_LogicProcessThread);
	}

	m_TcpConnters.clear();
	m_TcpServices.clear();

	clearsyncplist(m_waitdelgateuser);
	CUserEngine::getMe().Cleanup();

	m_boStartService = false;
	g_logger.forceLog(zLogger::zINFO,"停止服务成功...");
}

bool GameService::LoadLocalConfig()
{
	FUNCTION_BEGIN;
	CXMLConfigParse config;
	config.InitConfig();
	if (m_szsvrcfgdburl[0]==0){
		config.readstr("svrcfg",m_szsvrcfgdburl,sizeof(m_szsvrcfgdburl)-1,"msmdb://game:k3434yiuasdh848@127.0.0.1:0/\".\\db\\mydb_svrcfg.mdb\"");
		if (m_szsvrcfgdburl[0]==0){
			return false;
		}
	}
	if (m_szgamesvrparamdburl[0]==0){
		config.readstr("svrparam",m_szgamesvrparamdburl,sizeof(m_szgamesvrparamdburl)-1,"msmdb://game:k3434yiuasdh848@127.0.0.1:0/\".\\db\\mydb_svrparam.mdb\"");
		if (m_szgamesvrparamdburl[0]==0){
			return false;
		}
	}

	if (m_szremotetestsvrurl[0]==0){
		config.readstr("rts",m_szremotetestsvrurl,sizeof(m_szremotetestsvrurl)-1,"192.168.1.197:50002");
	}

	if (m_szgamecfgdburl[0]==0){
		config.readstr("gamecfg",m_szgamecfgdburl,sizeof(m_szgamecfgdburl)-1,(m_szremotetestsvrurl[0]==0)?"msmdb://game:k3434yiuasdh848@127.0.0.1:0/\".\\db\\mydb_gamecfg.mdb\"":"");
		if (m_szgamecfgdburl[0]==0 && m_szremotetestsvrurl[0]==0){
			return false;
		}
	}
	if (m_szMapTableName[0]==0){
		config.readstr("maptbl",m_szMapTableName,sizeof(m_szMapTableName)-1,DB_MAPINFO_TBL);
		if (m_szMapTableName[0]==0){
			strcpy_s(m_szMapTableName,sizeof(m_szMapTableName),DB_MAPINFO_TBL);
		}
	}

	if (m_szSvrName[0]==0){
		config.readstr("svrname",m_szSvrName,sizeof(m_szSvrName)-1,"测试区");
	}
	if(m_szQuestScriptPath[0]==0){
		config.readstr("quest",m_szQuestScriptPath,sizeof(m_szQuestScriptPath)-1,".\\luaScript\\GameSvr\\main.lua");
	}
	m_btAllisGm=config.readvalue("allgm",(BYTE)0);
	m_btGmOnlineWDYS=config.readvalue("gmwdys",(BYTE)1);
	m_boConnectManage=(config.readvalue("openmanage",(BYTE)1)==1);
	m_boAutoStart=(config.readvalue("autostart",(BYTE)0)==1);
	m_niocpworkthreadcount=safe_min(config.readvalue("iocpworks",(int)0),36);
	if (m_svridx==0){
		m_svridx = config.readvalue("svrid",(WORD)0,true) & 0x7fff;
		if (m_svridx==0){
			return false;
		}
	}
	m_lineId = m_svridx - _GAMESVR_TYPE;
	m_nAuthPort=config.readvalue("authport",(WORD)18888);
	config.readstr("authip",m_szAuthIp,sizeof(m_szAuthIp)-1,"127.0.0.1");

	//更新服务器ID
	m_Svr2SvrLoginCmd.svr_id = m_svridx;


	zXMLParser::getInstance()->initStringResFile(".\\StrRes.xml",&m_xStrResList);
	zXMLParser::getInstance()->initStringResFile(".\\LuaScriptRes.xml",&m_xScriptStrResList);
	return true;
}

void GameService::reloadStrRes(){
	FUNCTION_BEGIN;
	zXMLParser::getInstance()->initStringResFile(".\\StrRes.xml",&GameService::getMe().m_xStrResList);
	zXMLParser::getInstance()->initStringResFile(".\\LuaScriptRes.xml",&GameService::getMe().m_xScriptStrResList);
}

const char* GameService::GetStrRes(int dwIndex, std::string strType)
{
	FUNCTION_BEGIN;
	return zXMLParser::getInstance()->GetStrRes(&GameService::getMe().m_xStrResList,dwIndex,strType);
}

const char* GameService::GetLuaStrRes(int dwIndex, std::string strType)
{
	FUNCTION_BEGIN;
	return zXMLParser::getInstance()->GetStrRes(&GameService::getMe().m_xScriptStrResList,dwIndex,strType);
}

bool GameService::ReloadSvrConfig(){
	FUNCTION_BEGIN;
	stUrlInfo cfgui(0, m_szsvrcfgdburl, true);
	CSqlClientHandle* cfgsqlc = cfgui.newsqlclienthandle();
	bool boret = false;
	if (cfgsqlc){
		if (cfgsqlc->setHandle()){
			char extip_port[2048] = { 0 };
			char sztradename[64] = { 0 };
			WORD wtradeid=(WORD)-1;
			dbCol serverinfo_define[] = { 
				{_DBC_SO_("gametype", DB_WORD, stServerInfo, wgame_type)}, 
				{_DBC_SO_("zoneid", DB_WORD, stServerInfo, wzoneid)}, 
				{_DBC_SO_("zonename", DB_STR, stServerInfo, szZoneName)}, 
				{_DBC_SO_("id", DB_WORD, stServerInfo, svr_id)}, 
				{_DBC_SO_("type", DB_WORD, stServerInfo, svr_type)}, 
				{_DBC_SO_("name", DB_STR, stServerInfo, szName)}, 
				{_DBC_SO_("ip", DB_STR, stServerInfo, szIp)}, 
				{_DBC_SO_("gatewayport", DB_WORD, stServerInfo, gatewayport)}, 
				{_DBC_SO_("dbport", DB_WORD, stServerInfo, dbport)}, 
				{_DBC_SO_("gameport", DB_WORD, stServerInfo, gameport)}, 
				{_DBC_SO_("subsvrport", DB_WORD, stServerInfo, subsvrport)}, 
				{_DBC_SA_("extip_port", DB_STR, extip_port)},	//对客户端开放的 IP 端口
				{_DBC_SO_("dburl", DB_STR, stServerInfo,szUrl)}, 

				{_DBC_SA_("tradeid", DB_WORD,wtradeid )}, 
				{_DBC_SA_("tradename", DB_STR,sztradename )},
				{_DBC_SA_("truezoneid", DB_DWORD,m_nTrueZoneid )},
				{_DBC_MO_NULL_(stServerInfo)}, 
			};
			dbCol check_svrid_define[] = { 
				{_DBC_SA_("maxsvrid", DB_DWORD,m_dwmaxsvrid)}, 
				{_DBC_SA_("minsvrid", DB_DWORD,m_dwminsvrid)}, 
				{_DBC_NULL_}
			};
			int tmpbuffer=0;
			int ncur = cfgsqlc->execSelectSql(vformat("SELECT  MAX(id) AS maxsvrid, MIN(id) AS minsvrid	FROM " DB_CONFIG_TBL" WHERE (type = %u) ", m_svrtype), check_svrid_define, (unsigned char*) &tmpbuffer);
			if (ncur != 1) {
				cfgsqlc->unsetHandle();
				cfgsqlc->finalHandle();
				SAFE_DELETE(cfgsqlc);
				return false; 
			}
			DWORD dwMaxZoneid = 0;
			DWORD dwMinZoneid = 0;
			dbCol check_alltoone_define[] = { 
				{_DBC_SA_("maxzoneid", DB_DWORD,dwMaxZoneid)}, 
				{_DBC_SA_("minzoneid", DB_DWORD,dwMinZoneid)}, 
				{_DBC_NULL_}
			};

			tmpbuffer = 0;
			ncur = cfgsqlc->execSelectSql(vformat("SELECT  MAX(truezoneid) AS maxzoneid, MIN(truezoneid) AS minzoneid	FROM " DB_CONFIG_TBL" WHERE (type = %u and deleted = 0) ", m_svrtype), check_alltoone_define, (unsigned char*) &tmpbuffer);
			if (ncur != 1) { 
				cfgsqlc->unsetHandle();
				cfgsqlc->finalHandle();
				SAFE_DELETE(cfgsqlc);
				return false; 
			}
			if (dwMaxZoneid != dwMinZoneid){
				g_logger.forceLog(zLogger::zFORCE,"数据配置为一区多服模式");
				m_boAllToOne = true;
			}



			
			int ncount = cfgsqlc->getCount(DB_CONFIG_TBL," deleted=0 ");
			if (ncount > 0) {
				ZSTACK_ALLOCA(stServerInfo *, info,(ncount + 1));
				int ncur = cfgsqlc->execSelectSql(vformat("select top 1 * FROM " DB_CONFIG_TBL" where id=%u and type=%u and deleted=0 ", m_svridx, m_svrtype), serverinfo_define, (unsigned char*) info);
				if (ncur == 1) {
					strcpy_s(m_Svr2SvrLoginCmd.szZoneList,sizeof(m_Svr2SvrLoginCmd.szZoneList)-1,info[0].szUrl);
					if(m_supergamesvrconnecter){
						m_supergamesvrconnecter->sendcmd((void *)&m_Svr2SvrLoginCmd,sizeof(m_Svr2SvrLoginCmd));
					}

					if(m_globalsvrconnecter){
						m_globalsvrconnecter->sendcmd((void *)&m_Svr2SvrLoginCmd,sizeof(m_Svr2SvrLoginCmd));
					}

					if(m_tencentapigamesvrconnecter){
						m_tencentapigamesvrconnecter->sendcmd((void *)&m_Svr2SvrLoginCmd,sizeof(m_Svr2SvrLoginCmd));
					}

					if(m_loginsvrconnter){
						m_loginsvrconnter->sendcmd((void *)&m_Svr2SvrLoginCmd,sizeof(m_Svr2SvrLoginCmd));
					}

					if(m_dbsvrconnecter){
						m_dbsvrconnecter->sendcmd((void *)&m_Svr2SvrLoginCmd,sizeof(m_Svr2SvrLoginCmd));
					}
					g_logger.forceLog(zLogger::zFORCE, "重新加载合区区数据成功，%d区包含了%s",m_Svr2SvrLoginCmd.dwTrueZoneid,m_Svr2SvrLoginCmd.szZoneList);

					boret = true;
				}
			}
		}
		cfgsqlc->unsetHandle();
		cfgsqlc->finalHandle();
		SAFE_DELETE(cfgsqlc);
	}

	return true;
}

bool GameService::LoadSvrConfig(CSqlClientHandle* sqlc)
{
	FUNCTION_BEGIN;
	if (!sqlc) {
		return false;
	}
	char extip_port[2048] = { 0 };
	//char dburl[2048] = { 0 };
	char sztradename[64] = { 0 };
	WORD wtradeid=(WORD)-1;
	dbCol serverinfo_define[] = { 
		{_DBC_SO_("gametype", DB_WORD, stServerInfo, wgame_type)}, 
		{_DBC_SO_("zoneid", DB_WORD, stServerInfo, wzoneid)}, 
		{_DBC_SO_("zonename", DB_STR, stServerInfo, szZoneName)}, 
		{_DBC_SO_("id", DB_WORD, stServerInfo, svr_id)}, 
		{_DBC_SO_("type", DB_WORD, stServerInfo, svr_type)}, 
		{_DBC_SO_("name", DB_STR, stServerInfo, szName)}, 
		{_DBC_SO_("ip", DB_STR, stServerInfo, szIp)}, 
		{_DBC_SO_("gatewayport", DB_WORD, stServerInfo, gatewayport)}, 
		{_DBC_SO_("dbport", DB_WORD, stServerInfo, dbport)}, 
		{_DBC_SO_("gameport", DB_WORD, stServerInfo, gameport)}, 
		{_DBC_SO_("subsvrport", DB_WORD, stServerInfo, subsvrport)}, 
		{_DBC_SA_("extip_port", DB_STR, extip_port)},	//对客户端开放的 IP 端口
		{_DBC_SO_("dburl", DB_STR, stServerInfo,szUrl)},

		{_DBC_SA_("tradeid", DB_WORD,wtradeid )}, 
		{_DBC_SA_("tradename", DB_STR,sztradename )},
		{_DBC_SA_("truezoneid", DB_DWORD,m_nTrueZoneid )},
		{_DBC_MO_NULL_(stServerInfo)}, 
	};
	//数据库服务器参数设置说明

	//extip_port    网络类型(0=电信 1=网通):外网IP(192.168.9.143):端口1(7000):端口2(7001)
	//游戏服务器最多只支持 _MAX_SERVICES_LISTEN_ 个IP端口  帐号服务器 没有限制

	//dburl	  数据库编号(0..数据库总个数   账号hash后与数据库总个数取余,获得在那个编号对应的数据库中)
	//			=是否开始事务(暂时不只次开启,全部写0)="数据库类型(mssql)://账号:密码@数据库IP:端口(0 表示不指定端口,
	//			使用程序提供的默认端口)/数据库名"
	dbCol check_svrid_define[] = { 
		{_DBC_SA_("maxsvrid", DB_DWORD,m_dwmaxsvrid)}, 
		{_DBC_SA_("minsvrid", DB_DWORD,m_dwminsvrid)}, 
		{_DBC_NULL_}
	};
	int tmpbuffer=0;
	int ncur = sqlc->execSelectSql(vformat("SELECT  MAX(id) AS maxsvrid, MIN(id) AS minsvrid	FROM " DB_CONFIG_TBL" WHERE (type = %u) ", m_svrtype), check_svrid_define, (unsigned char*) &tmpbuffer);
	if (ncur != 1) { return false; }
	DWORD dwMaxZoneid = 0;
	DWORD dwMinZoneid = 0;
	dbCol check_alltoone_define[] = { 
		{_DBC_SA_("maxzoneid", DB_DWORD,dwMaxZoneid)}, 
		{_DBC_SA_("minzoneid", DB_DWORD,dwMinZoneid)}, 
		{_DBC_NULL_}
	};

	tmpbuffer = 0;
	ncur = sqlc->execSelectSql(vformat("SELECT  MAX(truezoneid) AS maxzoneid, MIN(truezoneid) AS minzoneid	FROM " DB_CONFIG_TBL" WHERE (type = %u and deleted = 0) ", m_svrtype), check_alltoone_define, (unsigned char*) &tmpbuffer);
	if (ncur != 1) { return false; }
	if (dwMaxZoneid != dwMinZoneid){
		g_logger.forceLog(zLogger::zFORCE,"数据配置为一区多服模式");
		m_boAllToOne = true;
	}
	bool boret = false;
	int ncount = sqlc->getCount(DB_CONFIG_TBL," deleted=0 ");
	if (ncount > 0) {
		ZSTACK_ALLOCA(stServerInfo *, info,(ncount + 1));
		int ncur = sqlc->execSelectSql(vformat("select top 1 * FROM " DB_CONFIG_TBL" where id=%u and type=%u and deleted=0 ", m_svridx, m_svrtype), serverinfo_define, (unsigned char*) info);
		if (ncur == 1) {
			if (wtradeid<0 || wtradeid>0xffff){
				g_logger.error("运营平台ID超过范围(%d<=%d<=%d)",0,wtradeid,0xffff);
				return false;
			}

			m_nZoneid = info[0].wzoneid;
			strcpy_s(m_szZoneName,sizeof(m_szZoneName)-1,info[0].szZoneName);
			char szTemp[MAX_PATH];
			char szruntime[20];
			timetostr(time(NULL),szruntime,20);
#ifdef _WIN64
			sprintf_s(szTemp,MAX_PATH,"GS x64[%s : GameSvr](BuildTime: %s)-(RunTime: %s)",m_szZoneName,__ZDATE__,szruntime);
#else
			sprintf_s(szTemp,MAX_PATH,"GS x86[%s : GameSvr](BuildTime: %s)-(RunTime: %s)",m_szZoneName,__ZDATE__,szruntime);
#endif
			SetTitle(szTemp);

			m_Svr2SvrLoginCmd.dwTrueZoneid=m_nTrueZoneid;
			m_Svr2SvrLoginCmd.wgame_type=0;
			m_Svr2SvrLoginCmd.wzoneid=m_nZoneid;
			strcpy_s(m_Svr2SvrLoginCmd.szZoneName,sizeof(m_Svr2SvrLoginCmd.szZoneName)-1,m_szZoneName);
			strcpy_s(m_Svr2SvrLoginCmd.szZoneList,sizeof(m_Svr2SvrLoginCmd.szZoneList)-1,info[0].szUrl);

			m_nTradeid=wtradeid;
			strcpy_s(m_szTradeName,sizeof(m_szTradeName)-1,sztradename);
			g_logger.forceLog(zLogger::zINFO,"运营平台[ %s ] ID=%d",sztradename,m_nTradeid);

			strcpy_s(m_szLocalIp, sizeof(m_szLocalIp) - 1, info[0].szIp);
			strcpy_s(m_szTureIp,sizeof(m_szTureIp) -1,extip_port);
			m_gatewayport= info[0].gatewayport;
			m_dbport = info[0].dbport;
			m_gameport = info[0].gameport;
			//=====================================
			//监听网关端口
			if (m_TcpServices.put(m_gatewayport, "游戏网关 数据交换端口", _GATEWAY_CONN_, 0, m_szLocalIp) != 0) {
				g_logger.error("游戏网关 数据交换端口 ( %s : %d ) 监听失败...", m_szLocalIp, m_gatewayport);
				return false;
			}
			//////////////////////////////////////////////////////////////////////////
			boret = true;
		}
		if (boret) {
			int idx=0;
			ncount = sqlc->execSelectSql(vformat("select top %d * FROM " DB_CONFIG_TBL" where id>0 and id<>%u and deleted=0 ", ncount, m_svridx), serverinfo_define, (unsigned char *) info);
			if (ncount > 0) {
				do{
					AILOCKT(cfg_lock);
					m_svrlistmap.clear();
				} while (false);

				if (boret) {
					for (int i = 0; i < ncount; i++) {
						if ( info[i].svr_type == _DBSVR_TYPE || info[i].svr_type == _LOGINSVR_TYPE || info[i].svr_type == _LOG_SVR_CONNETER_ || info[i].svr_type ==_GAMESVR_GATEWAY_TYPE  || info[i].svr_type ==_SUPERGAMESVR_TYPE  || info[i].svr_type ==_SCRIPTGAMESVR_TYPE || info[i].svr_type ==_GLOBALSERVER_TYPE) {
							do{
								AILOCKT(cfg_lock);
								m_svrlistmap[info[i].svr_marking] = info[i];
							} while (false);
						}
						if ( info[i].svr_type == _LOGSVR_TYPE && info[i].subsvrport > 0 ){
							CLogSvrConnecter* plogsvrconnecter = CLD_DEBUG_NEW CLogSvrConnecter(&info[i]);
							if (plogsvrconnecter) {
								m_logsvrconnecters.push_back(plogsvrconnecter);
								m_TcpConnters.put(plogsvrconnecter,info[i].szIp,info[i].subsvrport, "日志服务器连接", _LOG_SVR_CONNETER_, idx);
								idx++;
							}else{
								return false;
							}
						}

						if (info[i].svr_type == _LOGINSVR_TYPE && info[i].gameport > 0) {
							//连接帐号服务器
							m_loginsvrconnter = CLD_DEBUG_NEW CLoginSvrGameConnecter(&info[i]);
							if (m_loginsvrconnter) {
								m_TcpConnters.put(m_loginsvrconnter,info[i].szIp,info[i].gameport, "帐号服务器连接", _LOGIN_SVR_CONNETER_, idx);
								idx++;
							}else{
								return false;
							}
						}else if (info[i].svr_type == _DBSVR_TYPE && info[i].gameport > 0){
							//连接数据库服务器
							m_dbsvrconnecter = CLD_DEBUG_NEW CDBSvrGameConnecter(&info[i]);
							if (m_dbsvrconnecter) {
								m_TcpConnters.put(m_dbsvrconnecter,info[i].szIp,info[i].gameport, "数据库服务器连接", _DB_SVR_CONNETER_, idx);
								idx++;
							}else{
								return false;
							}
						}else if ( info[i].svr_type == _SUPERGAMESVR_TYPE && info[i].gameport > 0 && m_supergamesvrconnecter==NULL){
							m_supergamesvrconnecter = CLD_DEBUG_NEW CSuperGameSvrConnecter(&info[i]);
							if (m_supergamesvrconnecter) {
								m_TcpConnters.put(m_supergamesvrconnecter,info[i].szIp,info[i].gameport, "全局游戏服务器连接", _SUPERGAME_SVR_CONNETER_, idx);
								idx++;
							}else{
								return false;
							}
						}
#ifndef _SKIP_GAME_PROXY_
						else if ( info[i].svr_type == _GAMESVRPROXY_TYPE && info[i].subsvrport > 0 && m_pGameSvrProxyConnecter==NULL){
							m_pGameSvrProxyConnecter = CLD_DEBUG_NEW CGameSvrProxyConnecter(&info[i]);
							if (m_pGameSvrProxyConnecter) {
								m_TcpConnters.put(m_pGameSvrProxyConnecter,info[i].szIp,info[i].subsvrport, "游戏服务器代理连接", _GAMEPROXY_SVR_CONNECTER, idx);
								idx++;
							}else{
								return false;
							}
						}
#else
						else if(info[i].svr_type == _GLOBALPROXYSVR_TYPE && info[i].subsvrport > 0 && m_globalProxyConnecter == NULL){
							m_globalProxyConnecter = CLD_DEBUG_NEW CGlobalProxyConnecter(&info[i]);
							if(m_globalProxyConnecter){
								m_TcpConnters.put(m_globalProxyConnecter, info[i].szIp, info[i].subsvrport, "globalproxy连接",_GLOBALPROXYSVR_TYPE, idx);
								idx ++;
							}else{
								return false;
							}
						}
#endif
						else if ( info[i].svr_type == _CHECKNAMESVR_TYPE && info[i].subsvrport > 0 && m_checknamesvrconnecter==NULL){
							m_checknamesvrconnecter = CLD_DEBUG_NEW CCheckNameSvrConnecter(&info[i]);
							if (m_checknamesvrconnecter) {
								m_TcpConnters.put(m_checknamesvrconnecter,info[i].szIp,info[i].subsvrport, "全局名字重复性检查服务器连接", _CHECKNAME_SVR_CONNETER_, idx);
								idx++;
							}else{
								return false;
							}
						}else if (info[i].svr_type == _GLOBALSERVER_TYPE && info[i].gameport > 0 && m_globalsvrconnecter==NULL){
							m_globalsvrconnecter = CLD_DEBUG_NEW CGlobalSvrConnecter(&info[i]);
							if(m_globalsvrconnecter){
								m_TcpConnters.put(m_globalsvrconnecter,info[i].szIp,info[i].gameport, "Global服务器连接", _GLOBAL_SVR_CONNECTER, idx);
								idx++;
							}else{
								return false;
							}
						}else if(info[i].svr_type==_TENCENTGAMESVR_TYPE && info[i].gameport > 0 && m_tencentapigamesvrconnecter==NULL){
							m_tencentapigamesvrconnecter = CLD_DEBUG_NEW CTencentApiGameSvrConnecter(&info[i]);
							if(m_tencentapigamesvrconnecter){
								m_TcpConnters.put(m_tencentapigamesvrconnecter,info[i].szIp,info[i].gameport, "Tencent游戏服务器连接", _TENCENT_SVR_CONNECTER, idx);
								idx++;
							}else{
								return false;
							}
						}else if(info[i].svr_type==_GMSERVERMANAGE_TYPE && info[i].gameport>0 && m_gmservermanageconnecter==NULL){
							m_gmservermanageconnecter=CLD_DEBUG_NEW CGmServerManageConnecter(&info[i]);
							if(m_gmservermanageconnecter){
								m_TcpConnters.put(m_gmservermanageconnecter,info[i].szIp,info[i].gameport,"游戏管理服务器连接",_GMSERVERMANAGE_TYPE,idx);
								idx++;
							}else{
								return false;
							}
					}
					}
					Authorize();
				}
			}
		}
	}
	m_nTrueZoneid = m_Svr2SvrLoginCmd.dwTrueZoneid;
	if (!m_itemIdGen.init(m_svridx - m_dwminsvrid, m_nTradeid, m_nZoneid))
	{
		g_logger.error("物品ID生成器初始化失败");
		return false;
	}
	return boret;
}
bool GameService::SaveServerParam(int naddtime,int nadditemid){
	FUNCTION_BEGIN;
	g_logger.forceLog(zLogger::zINFO,"itemtmpid: %u  write_itemtmpid: %u", m_itemTmpId.load(), m_itemTmpId +nadditemid);
	GETAUTOSQL(CSqlClientHandle *, sqlc, m_svrdatadb, _SVR_PARAM_DBHASHCODE_);
	if (sqlc){
		DWORD shutdowntime=time(NULL)+naddtime;
		DWORD tmpid= m_itemTmpId +nadditemid;
		DWORD tmpgrouptmpid=m_svrcfg.grouptmpid+1;
		stServerConfig tmp;
		dbCol svrparam_define[] = { 
			{_DBC_SA_("id", DB_WORD, m_svridx)}, 
			{_DBC_SA_("type", DB_WORD, m_svrtype)}, 
			{_DBC_SA_("grouptmpid", DB_DWORD, tmpgrouptmpid)}, 
			{_DBC_SA_("itemtmpid", DB_DWORD, tmpid)}, 
			{_DBC_NULL_},  
		};
		int nret=sqlc->execUpdate(DB_SERVERPARAM_TBL, svrparam_define, (unsigned char *) &tmp, vformat(" id=%u and type=%u ", m_svridx, m_svrtype));
		if (nret > 0) {
			return true;
		}else{
			nret=sqlc->execInsert(DB_SERVERPARAM_TBL, svrparam_define, (unsigned char *) &tmp);
			if (nret > 0) {
				return true;
			}
		}
	}
	return false;
}

bool GameService::LoadServerParam(){
	FUNCTION_BEGIN;
	//return true;
	CLD_dbColMaker maker;
	m_svrdatadb.putURL(_SVR_PARAM_DBHASHCODE_,vformat(m_szgamesvrparamdburl,m_nTrueZoneid),false,4);
	do {
		GETAUTOSQL(CSqlClientHandle*, sqlc, m_svrdatadb, _SVR_PARAM_DBHASHCODE_);
		if (!sqlc) { return false; }

		//////////////////////////////////////////////////////////////////////////
		stServerConfig tmp;
		auto str = vformat("select top 1 * from mydb_svrparam_tbl where id=%u and type=%u", m_svridx, m_svrtype);
		auto ncur = sqlc->execSelectSql(str, stServerConfigDefine, (unsigned char*)&tmp);
		if (ncur != 1) { return false; }
		if (tmp.itemtmpid == 0 || tmp.shutdowntime == 0 || (DWORD)time(NULL) <= tmp.shutdowntime) {
			g_logger.error("服务器保存参数异常! shutdowntime=%s tmpid=%u ", timetostr(tmp.shutdowntime), tmp.itemtmpid);
			return false;
		}
		else {
			m_svrcfg = tmp;
		}
	} while (false);
	m_itemTmpId = m_svrcfg.itemtmpid;
	g_logger.forceLog(zLogger::zINFO, "itemtmpid: %u", m_svrcfg.itemtmpid);
	return true;
}

bool GameService::Send2LoginSvrs(void* pcmd,int ncmdlen){
	FUNCTION_BEGIN;
	if (m_loginsvrconnter && !m_loginsvrconnter->isTerminate() && m_loginsvrconnter->IsConnected()){
		m_loginsvrconnter->sendcmd(pcmd,ncmdlen);
	}
	return true;
}

bool GameService::Send2TencentSvrs(const char* pszOpenId,const char* pszFunName,const char* szClientIp,bool boForceRefresh){
	stOpenApiGetFResult sendcmd;
	sendcmd.nTradeID = _SERVERTRADE_TENCENT_;
	strcpy_s(sendcmd.szOpenId,sizeof(sendcmd.szOpenId)-1,pszOpenId);
	strcpy_s(sendcmd.szScriptNAme,sizeof(sendcmd.szScriptNAme)-1,pszFunName);
	sendcmd.boForceRefresh = boForceRefresh;
	sendcmd.nTrueZoneId=m_nTrueZoneid;
	strcpy_s(sendcmd.szClientIp,sizeof(sendcmd.szClientIp)-1,szClientIp);
	strcpy_s(sendcmd.szLocalIp,sizeof(sendcmd.szLocalIp)-1,m_szLocalIp);
	if(m_tencentapigamesvrconnecter){
		m_tencentapigamesvrconnecter->sendcmd(&sendcmd,sizeof(sendcmd));
		return true;
	}
	return false;
}

bool GameService::Send2DBSvrs(void* pcmd,int ncmdlen){
	FUNCTION_BEGIN;
	CDBSvrGameConnecter* dbsvr=m_dbsvrconnecter;
	do{
		if (dbsvr && dbsvr->IsConnected() && !dbsvr->isTerminate()){
			dbsvr->sendcmd(pcmd,ncmdlen);
		}
	} while (false);
	return true;
}

bool GameService::BatchSendToGate(stProxyBroadcastUserIndex* puserindexs, int nuser, void* pbuf, unsigned int nsize, int zliblevel)
{
	m_gatewaysession.s_foreach([&](CGameGatewaySvrSession* session)
	{
		if (session && !session->isTerminate() && session->IsConnected())
		{
			session->SendProxyCmd2Users(puserindexs, nuser, pbuf, nsize, zliblevel);
		}
	});
	return true;
}

bool GameService::Send2LogSvr(int nLogType,int nSubLogType,int execType,CPlayerObj* pPlayer,const char* fmtlogstr,...){
	if (!m_logsvrconnecters.empty()){
		for (DWORD i=0;i<m_logsvrconnecters.size();i++){
			CLogSvrConnecter* plogsvrconnecter=m_logsvrconnecters[i];
			if (plogsvrconnecter){
				BUFFER_CMD(stLogStrCmd,logcmd,1024*128);
				int nTrueZoneid = m_nTrueZoneid;
				if (CUserEngine::getMe().isCrossSvr() && pPlayer) {
					nTrueZoneid = pPlayer->m_dwSrcZoneId - 1000;
				}
				logcmd->dwTrueZoneid = nTrueZoneid;
				logcmd->nLogType=nLogType;
				logcmd->nExecType=execType;
				logcmd->nSubLogType = nSubLogType;
				logcmd->locTime=time(NULL);
				va_list ap;	
				va_start(ap, fmtlogstr);		
				_safe_vsnprintf(&logcmd->logstr[0], (1024*127) - 32, fmtlogstr, ap);	
				va_end(ap);	
				logcmd->logstr.size=strlen(&logcmd->logstr[0]);
				if (nLogType != 2) {
					if (pPlayer) {
						logcmd->logstr.push_str(vformat(",'%s','%s',%d,'%s','%s','%s'", pPlayer->getSubPlatform(), pPlayer->getMeshineid(), pPlayer->m_dwOriginalZoneid,
							pPlayer->getClientBundleId(), pPlayer->getClientPlatform(), pPlayer->getClientVersion()));
					}
					else {
						logcmd->logstr.push_str(vformat(",'%s','%s',%d,'%s','%s','%s'", " ", " ", 0, " ", " ", " "));
					}
				}
				logcmd->logstr.size=strlen(&logcmd->logstr[0])+1;
				logcmd->logstr[logcmd->logstr.size]=0;
				plogsvrconnecter->sendcmd(logcmd,sizeof(*logcmd)+logcmd->logstr.getarraysize());
			}
		}
	}

	return true;
}

bool GameService::SendSvrCmd2GatewaySvrs(void* pcmd, int ncmdlen)
{
	m_gatewaysession.s_foreach([&](CGameGatewaySvrSession* session)
	{
		if (session && !session->isTerminate() && session->IsConnected())
		{
			session->SendGateCmd(pcmd, ncmdlen);
		}
	});
	return true;
}

void getextstr(char* pbase,char* pout,char* pext){
	if (pext[0]!=0 && ((pext[0]>='a' && pext[0]<='z') 
		|| (pext[0]>='A' && pext[0]<='Z') 
		|| (pext[0]>='0' && pext[0]<='9') 
		|| pext[0]=='_') ){
			sprintf_s(pout,64,"%s%s",pbase,pext);
	}else{
		strcpy_s(pout,64,pbase);
	}
}

bool GameService::OnInit()
{
	FUNCTION_BEGIN;
	CFilter::getMe().init("FilterChat.txt");
	LoadLocalConfig();
	if (m_boAutoStart){
		OnStartService();
	}
	return true;
}

void GameService::OnIdle()
{
	FUNCTION_BEGIN;
	static time_t lastshow=time(NULL);
	if (time(NULL)>lastshow){
		SetStatus(0,"type: %d(id: %d) %d:%s ",m_svrtype,m_svridx,m_nZoneid,m_szZoneName);
		//SetStatus(1,"%u/%u:%u:%u(%u:%u:%u)",0,0,0,0,0,0,0);
		SetStatus(1, "FPS:%d playercount:%d", CUserEngine::getMe().m_dwFPS, (DWORD)CUserEngine::getMe().m_playerhash.size());
		SetStatus(2,"%u: %s",::GetCurrentProcessId(),CThreadMonitor::getme().m_szThreadFlag);
		static time_t static_clear_tick=time(NULL)+60*5;
		if (time(NULL)>static_clear_tick){
			static_clear_tick=time(NULL)+60*5;

			DWORD dwgetwaitretsavecount=0,dwgetsavecount=0;
			if (m_dbsvrconnecter){
				dwgetwaitretsavecount+=m_dbsvrconnecter->getwaitretsavecount();
				dwgetsavecount+=m_dbsvrconnecter->getsavecount();
			}
			extern size_t g_savedatamaxsize;
			extern size_t g_savedatazlibmaxsize;
			g_logger.forceLog(zLogger::zINFO,"UserCount : %u getwaitretsavecount : %u  dwgetsavecount : %u  SavedataMaxsize : %uk/%uk  "
				,CUserEngine::getMe().m_playerhash.size(),dwgetwaitretsavecount,dwgetsavecount,((g_savedatamaxsize/1024) +1),((g_savedatazlibmaxsize/1024) +1) );
		}

		if (CUserEngine::getMe().m_boIsShutDown){
			if (!s_bosavesvrparam && CUserEngine::getMe().m_playerhash.size()==0 ){
				DWORD dwgetwaitretsavecount=0,dwgetsavecount=0;
				if (m_dbsvrconnecter){
					dwgetwaitretsavecount+=m_dbsvrconnecter->getwaitretsavecount();
					dwgetsavecount+=m_dbsvrconnecter->getsavecount();
				}
				if (dwgetwaitretsavecount==0 && dwgetsavecount==0){
					SaveServerParam(0,0);
					s_bosavesvrparam=true;
					g_logger.forceLog(zLogger::zINFO,"服务器运行时参数保存成功,可以关闭服务器!!!");
					extern size_t g_savedatamaxsize;
					extern size_t g_savedatazlibmaxsize;
					g_logger.forceLog(zLogger::zINFO,"UserCount : %u getwaitretsavecount : %u  dwgetsavecount : %u SavedataMaxsize : %uk/%uk  "
						,CUserEngine::getMe().m_playerhash.size(),dwgetwaitretsavecount,dwgetsavecount,((g_savedatamaxsize/1024) +1),((g_savedatazlibmaxsize/1024) +1) );
				}
			}
		}else{
			s_bosavesvrparam=false;
		}

		if (time(NULL)>m_timeshutdown && m_timeshutdown!=0){
			g_logger.forceLog(zLogger::zINFO,"服务器执行定时关闭任务： %s",timetostr(m_timeshutdown));
			m_timeshutdown=0;
			m_forceclose=true;
			Close();
		}
		lastshow=lastshow+1;
	}
	Sleep(50);
}

void GameService::OnQueryClose(bool& boClose)
{
	FUNCTION_BEGIN;
	if (m_boStartService){boClose=false;}
	else {return;}
	if (m_boshutdown){return;}
	this->OnStopService();
}

void GameService::OnUninit()
{
	FUNCTION_BEGIN;

}

void GameService::OnStartService(){
	FUNCTION_BEGIN;
	StartService();
	if (m_boStartService){EnableCtrl( false );}
}

void GameService::OnStopService(){
	FUNCTION_BEGIN;
	if (m_forceclose || (MessageBox(0,"你确定要关闭 游戏服务器 么?",this->GetTitle(),MB_OKCANCEL | MB_DEFBUTTON2)==IDOK)){
		if (!m_forceclose){g_logger.forceLog(zLogger::zINFO,"服务器执行手动关闭任务");}
		ShutDown();
		if (!m_boStartService){
			//EnableCtrl( true );
		}
		EnableCtrl( false );


		//_CrtDumpMemoryLeaks();
		//this->Close();
	}
}


void GameService::OnSafeClose()
{
	FUNCTION_BEGIN;
	if (!m_forceclose){g_logger.forceLog(zLogger::zINFO,"服务器管理器执行运维关闭命令");}
	ShutDown();
	if (!m_boStartService){EnableCtrl( true );}
	this->Close();
}

void GameService::OnConfiguration()
{
	FUNCTION_BEGIN;
	CFilter::getMe().init("FilterChat.txt");
	SetLog(0,"重新加载配置文件...");
}

long GameService::OnTimer( int nTimerID ){
	FUNCTION_BEGIN;
	__super::OnTimer(nTimerID);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
bool GameService::exec_input_cmd(void* currobj,char* szCmd,stGmRetExecCmd* retcmd,int nretmax)
{
	char szcmd[1024*8];
	strcpy_s(szcmd,sizeof(szcmd)-1,szCmd);
	GameService* service=GameService::instance();
	char* szparam=strchr(szcmd,' ');
	CCmdlineParse parse;
	if (szparam){ *szparam=0;szparam++;	parse.parseCmdLine(szparam);}
	if(stricmp(szcmd,"closegate")==0){
		GameService::getMe().m_boForceCloseGetWay = atoi(parse["c"].c_str());
		if(GameService::getMe().m_boForceCloseGetWay == 1){
			sprintf_s(&retcmd->szcmdret[0],nretmax,"%s","只能连接本机gate");
		}else if(GameService::getMe().m_boForceCloseGetWay == 2){
			sprintf_s(&retcmd->szcmdret[0],nretmax,"%s","只能连接非本机gate");
		}else{
			sprintf_s(&retcmd->szcmdret[0],nretmax,"%s","任意网关可以连接");
		}

		retcmd->szcmdret.size=strlen(&retcmd->szcmdret[0])+1;
	}else if(stricmp(szcmd,"reloadzonelist") == 0){
		GameService::getMe().ReloadSvrConfig();
	}else if (stricmp(szcmd, "setloglvl")==0 ){
		g_logger.setLevel((parse["write"]=="")?g_logger.writelvl():atoi(parse["write"].c_str()) ,(parse["show"]=="")?g_logger.showlvl():atoi(parse["show"].c_str()));
		chatlogger.setLevel(5 ,(parse["show"]=="")?g_logger.showlvl():atoi(parse["show"].c_str()));
		sprintf_s(&retcmd->szcmdret[0],nretmax,"修改服务器日志等级成功,当前写日志等级为 %d 显示日志等级为 %d",g_logger.writelvl(),g_logger.showlvl());
		retcmd->szcmdret.size=strlen(&retcmd->szcmdret[0])+1;
	}else if (stricmp(szcmd, "getloglvl")==0 ){
		retcmd->ret1=g_logger.writelvl();
		retcmd->ret2=g_logger.showlvl();
		sprintf_s(&retcmd->szcmdret[0],nretmax,"服务器当前写日志等级为 %d 显示日志等级为 %d",retcmd->ret1,retcmd->ret2);
		retcmd->szcmdret.size=strlen(&retcmd->szcmdret[0])+1;
	}else if (stricmp(szcmd, "setlogsvr")==0 ){
		int svrid=atoi(parse["svrid"].c_str());

		const char* dip=parse["dip"].c_str();
		int dport=atoi(parse["dport"].c_str());

		if (dport!=0 && dport<0xffff && !service->m_logsvrconnecters.empty()){
			for (DWORD i=0;i<service->m_logsvrconnecters.size();i++){
				CLogSvrConnecter* plogsvr=service->m_logsvrconnecters[i];
				if ( plogsvr!=NULL && plogsvr->svr_id==svrid ){
					CMAsyncTcpConnters::Connecter2time_iterator it=service->m_TcpConnters.getall().find(plogsvr);
					if (it!=service->m_TcpConnters.end()){
						if (it->first==plogsvr){
							char sip[64]={0};
							strcpy_s(sip,sizeof(sip)-1,it->second.connip.c_str());
							int sport=it->second.nport;

							it->second.connip=dip;
							it->second.nport=dport;

							sprintf_s(&retcmd->szcmdret[0],nretmax,"日志服务器 %d 连接地址 %s:%d 修改为 %s:%d !",svrid,sip,sport,it->second.connip.c_str(),it->second.nport);
							retcmd->szcmdret.size=strlen(&retcmd->szcmdret[0])+1;
							return true;
						}
					}
				}
			}
		}
		sprintf_s(&retcmd->szcmdret[0],nretmax,"日志服务器 %d 连接地址修改失败!");
		retcmd->szcmdret.size=strlen(&retcmd->szcmdret[0])+1;
	}else{
		stGmCmd curcmd;
		curcmd.btGmLv=0xff;
		strcpy_s(curcmd.szName,_MAX_NAME_LEN_-1,s_psystem_virtual_palyer->getName());
		strcpy_s(curcmd.szCmdStr,_MAX_RETCMD_LEN_-1,szCmd);
		do {
			AILOCKT(CUserEngine::getMe().m_cmdstrlist);
			CUserEngine::getMe().m_cmdstrlist.push_back(curcmd);
		} while (false);
		//retcmd->format(1024*6,"不存在的GM命令 %s",szCmd);
		return false;
	}

	return true;
}

bool GameService::OnCommand( char* szCmd )
{
	FUNCTION_BEGIN;

	CUserEngine::getMe().addDelayLuaCall(excutecommand,1,szCmd);
	return true;
}

long GameService::OnCommand( int nCmdID )
{
	FUNCTION_BEGIN;
	switch ( nCmdID )
	{
	case IDM_STARTSERVICE:	OnStartService();	break;
	case IDM_STOPSERVICE:	OnStopService();	break;
	case IDM_CONFIGURATION:	OnConfiguration();	break;
	case IDM_CLEARLOG:		OnClearLog();		break;
	case IDM_DEBUGINFO:		RefSvrRunInfo();	break;
	case IDM_EXIT:			OnExit();			break;
	case IDM_SAFECLOSE:		OnSafeClose();		break;
	case IDM_DOLUA:			OnDolua();		break;
	}
	return 0;
}


bool GameService::CreateToolbar()
{
	FUNCTION_BEGIN;
	TBBUTTON tbBtns[] = 
	{
		{0, IDM_STARTSERVICE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{1, IDM_STOPSERVICE,  TBSTATE_ENABLED, TBSTYLE_BUTTON},
	};
	int nBtnCnt = sizeof( tbBtns ) / sizeof( tbBtns[0] );
	m_hWndToolbar = CreateToolbarEx( m_hWnd, WS_CHILD | WS_VISIBLE | WS_BORDER,
		IDC_TOOLBAR, nBtnCnt, m_hInstance, IDB_TOOLBAR,
		(LPCTBBUTTON) &tbBtns, nBtnCnt, 16, 16, 16, 16, sizeof( TBBUTTON ) );


	m_hCmdEdit = CreateWindow( WC_EDIT, "",WS_CHILD| WS_VISIBLE | ES_WANTRETURN,
		nBtnCnt* (16+8)+16, 3, m_nwidth-(nBtnCnt* (16+8)+16), 19, m_hWndToolbar, 0, m_hInstance, this );

	INIT_AUTOCOMPLETE(m_hCmdEdit,"AutoComplete.dll","InitAutoComplete","SaveGMOrder",(char*)".\\cmdlist\\gamesvrcmd.txt",g_SaveGMOrder,g_AutoCompleteh);
	SetWindowLongPtr( m_hCmdEdit, GWLP_USERDATA,(LONG_PTR)this);
	m_EditMsgProc=(WNDPROC)SetWindowLongPtr (m_hCmdEdit, GWLP_WNDPROC, (LONG_PTR)WinProc);
	return m_hWndToolbar ? true : false;
}

bool GameService::Init( HINSTANCE hInstance )
{
	FUNCTION_BEGIN;
	m_hInstance = hInstance;

	InitCommonControls();	//初始化命令控制

	if ( !CreateWnd() || !CreateToolbar() || !CreateList() || !CreateStatus() )
		return false;

	int   pos[   4   ]={   300,   480,  620,  -1   };     //   100,   200,   300   为间隔   
	::SendMessage(   m_hWndStatus,   SB_SETPARTS, (WPARAM)4, (LPARAM)pos   );   

	EnableCtrl( true );
	ShowWindow( m_hWnd, SW_SHOWDEFAULT );

	if ( !OnInit() )
		return false;

	//SetStatus( 0,"Ready.." );

	return true;
}

void GameService::OnClearLog(){
	FUNCTION_BEGIN;
	__super::OnClearLog();
}

void GameService::OnDolua() {
	FUNCTION_BEGIN;
	CUserEngine::getMe().addDelayLuaCall(excutecommand, 1, "dolua");
}
void GameService::EnableCtrl( bool bEnableStart )
{
	FUNCTION_BEGIN;
	HMENU hMenu = GetSubMenu( GetMenu( m_hWnd ), 0 );

	if ( bEnableStart ){
		EnableMenuItem( hMenu, IDM_STARTSERVICE,	MF_ENABLED | MF_BYCOMMAND );
		EnableMenuItem( hMenu, IDM_STOPSERVICE,		MF_GRAYED  | MF_BYCOMMAND );
		EnableMenuItem( hMenu, IDM_CONFIGURATION,	MF_ENABLED | MF_BYCOMMAND );

		SendMessage( m_hWndToolbar, TB_SETSTATE, IDM_STARTSERVICE, 
			(LPARAM) MAKELONG( TBSTATE_ENABLED, 0 ) );
		SendMessage( m_hWndToolbar, TB_SETSTATE, IDM_STOPSERVICE,
			(LPARAM) MAKELONG( TBSTATE_INDETERMINATE, 0 ) );
	}else{
		EnableMenuItem( hMenu, IDM_STARTSERVICE,	MF_GRAYED  | MF_BYCOMMAND );
		EnableMenuItem( hMenu, IDM_STOPSERVICE,		MF_ENABLED | MF_BYCOMMAND );
		EnableMenuItem( hMenu, IDM_CONFIGURATION,	MF_GRAYED  | MF_BYCOMMAND );

		SendMessage( m_hWndToolbar, TB_SETSTATE, IDM_STARTSERVICE, 
			(LPARAM) MAKELONG( TBSTATE_INDETERMINATE, 0 ) );
		SendMessage( m_hWndToolbar, TB_SETSTATE, IDM_STOPSERVICE,
			(LPARAM) MAKELONG( TBSTATE_ENABLED, 0 ) );
	}
}

static CIntLock msglock;

void __stdcall ShowLogFunc(zLogger::zLevel& level,const char* logtime,const char* msg){			//调试信息
	if (msg) {
		GameService::getMe().SetLog(level.showcolor,msg);
	}
}
#include "frameAllocator.h"

void OnCreateInstance(CWndBase *&pWnd){
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	CRYPT_START
		//Lock_CurrentProcess();
		OleInitialize(NULL);
	pWnd=GameService::instance();

	CXMLConfigParse config;
	config.InitConfig();
	int nshowlvl=config.readvalue("showlvl",(int)5);
	int nloglvl=config.readvalue("writelvl",(int)3);
	char szlogpath[MAX_PATH];
	config.readstr("logpath",szlogpath,sizeof(szlogpath)-1,vformat(".\\log\\gamesvr%d_log\\",GameService::getMe().m_Svr2SvrLoginCmd.svr_id),true);

	g_logger.setLevel(nloglvl, nshowlvl);
	g_logger.SetLocalFileBasePath(szlogpath);
	g_logger.SetShowLogFunc(ShowLogFunc);
	config.readstr("chatlogpath",szlogpath,sizeof(szlogpath)-1,vformat(".\\chatlog\\gamesvr%d_chatlog\\",GameService::getMe().m_Svr2SvrLoginCmd.svr_id),true);

	chatlogger.setLevel(5,nshowlvl);
	chatlogger.SetLocalFileBasePath(szlogpath);
	chatlogger.SetShowLogFunc(ShowLogFunc);

	config.readstr("leaderlogpath",szlogpath,sizeof(szlogpath)-1,vformat(".\\leaderlog\\gamesvr%d_chatlog\\",GameService::getMe().m_Svr2SvrLoginCmd.svr_id),true);
	LeaderLogger.setLevel(5,0);
	LeaderLogger.SetLocalFileBasePath(szlogpath);
	LeaderLogger.SetShowLogFunc(ShowLogFunc);

	CRYPT_END
}

int OnDestroyInstance( CWndBase *pWnd ){
	if ( pWnd ){
		GameService::delMe();
		OleUninitialize();
		pWnd = NULL;
	}
	return 0;
}
