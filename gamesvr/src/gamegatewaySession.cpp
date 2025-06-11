#include "gamegatewaySession.h"
#include "gamesvr.h"
#include "gamelogic/PlayerObj.h"
#include "gamelogic/UsrEngn.h"
#include "cmd/login_cmd.h"
#include "dbsvrGameConnecter.h"
IMP_OP_NEW(CGameGateWayUser);

CGameGateWayUser::CGameGateWayUser(CGateWaySession* pgate): stGateWayUser(pgate), m_boLoginOk(false), m_tmpid(),
                                                            m_OwnerDbSvr(NULL), m_OwnerLoginSvr(NULL), m_Player(NULL),
                                                            m_iptype(0)
{
	m_szAccount[0] = 0;
	m_szTxSubPlatformName[0] = 0;
	svr_id_type_value = 0;
	m_szMeshineid[0] = 0;
	m_szClientBundleId[0] = 0;
	m_szClientPlatform[0] = 0;
	m_szClientVersion[0] = 0;
	m_szLoginChannel[0] = 0;
};

CGameGateWayUser::~CGameGateWayUser(){
	FUNCTION_BEGIN;
	g_logger.debug("CGameGateWayUser对象 %.8x 删除,%s %s",this,m_szAccount,m_Player?m_Player->getName():"");
}

//客户端重登陆后第一个消息开始处理的地方

bool CGameGateWayUser::clientmsgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
// 	extern bool g_boshowproxylog;
// 	if (g_boshowproxylog){
// 		g_logger.debug("网关会话(%d-%d)转发(%d:%d)到服务器会话(%d)",Gateidx(),ClientSocketIdx(),pcmd->cmd,pcmd->subcmd,Svridx());
// 	}
	switch (pcmd->value)
	{
	case stUserReLoginGameSvr::_value:
		{
			if (ncmdlen>=sizeof(stUserReLoginGameSvr) && (!m_boLoginOk || (m_Player && m_Player->m_pGateUser==this))){
				m_iptype=((stUserReLoginGameSvr*)pcmd)->ip_type;
				m_fclientver=((stUserReLoginGameSvr*)pcmd)->fclientver;
				return OnstUserReLoginGameSvr((stUserReLoginGameSvr*)pcmd,ncmdlen,bufferparam);
			}else{

			}
		}
		break;
	case stClientBundlePlatform::_value:
		{
			stClientBundlePlatform* pdstcmd = (stClientBundlePlatform*)pcmd;
			strcpy_s(m_szClientBundleId, sizeof(m_szClientBundleId)-1, pdstcmd->szBundleId);
			strcpy_s(m_szClientPlatform, sizeof(m_szClientPlatform)-1, pdstcmd->szPlatform);
			strcpy_s(m_szClientVersion, sizeof(m_szClientVersion)-1, pdstcmd->szVersion);
		}break;
	case stFastTransferPlayer::_value:
		{
			if (ncmdlen >= sizeof(stFastTransferPlayer) && (!m_boLoginOk || (m_Player && m_Player->m_pGateUser == this))) {
				m_iptype = ((stFastTransferPlayer*)pcmd)->ip_type;
				m_fclientver = ((stFastTransferPlayer*)pcmd)->fclientver;
				return OnstFastInnerChangeGameSvr((stFastTransferPlayer*)pcmd, ncmdlen, bufferparam);
			}
		}
		break;
	default:
		{
			if (m_boLoginOk && m_Player && m_Player->m_btReadyState>=_READYSTATE_SVR_READY_){
				if (pcmd->value==stCheckSpeedCmd::_value){
					stCheckSpeedCmd* pdstcmd=(stCheckSpeedCmd*)pcmd;
					pdstcmd->setLog(GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value);
					g_logger.forceLog(zLogger::zFORCE, "(%d)测速信号_proxy_recv_process %d:%s : r:%d(l:%i64d)",pdstcmd->dwProxyCount,pdstcmd->dwCheckIndex,pdstcmd->szCheckStr,pdstcmd->dwLocalTick,::GetTickCount64() );
				}else if (pcmd->value == stClientIsReady::_value)
				{
					if (m_Player->isDataLoaded() && !m_Player->isClientReady())
					{
						m_Player->m_btReadyState = _READYSTATE_ISALL_READY_;
						g_logger.debug("%s 客户端准备完毕", m_Player->getName());
						m_Player->NotifyMap();
					}
					else if (m_Player->ClientIsReady() && m_Player->m_changeMapId.all) {
						g_logger.debug("%s 切换地图 客户端准备完毕", m_Player->getName());
						m_Player->NotifyMap();
					}
					else {
						//m_Player->NotifyClose(CLOSEUSER_STATEERROR);
						g_logger.error("%s 状态错误 %d", m_Player->getName(), m_Player->m_btReadyState);
					}
				}else
				{
					//DWORD dwCurTime = GetTickCount();
					//g_logger.error("begin:收到网关 包号:%d %d 时间:%d", pcmd->cmd, pcmd->subcmd, dwCurTime);
					m_Player->ProcessUserMessage(pcmd,ncmdlen,bufferparam);
				}
			}else if (pcmd->value==stCheckSpeedCmd::_value){
				if (m_Player && m_Player->m_pGateUser && !m_Player->m_pGateUser->isSocketClose()){
					m_Player->m_pGateUser->SendProxyCmd(pcmd,ncmdlen);
				}
				stCheckSpeedCmd* pdstcmd=(stCheckSpeedCmd*)pcmd;
				pdstcmd->setLog(GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value);
				g_logger.forceLog(zLogger::zFORCE, "(%d)测速信号_proxy_recv %d:%s : r:%d(l:%i64d)",pdstcmd->dwProxyCount, pdstcmd->dwCheckIndex,pdstcmd->szCheckStr,pdstcmd->dwLocalTick,::GetTickCount64() );
			}
		}
		break;
	}
	return true;
}

bool CGameGateWayUser::OnclientCloseUser(){
	FUNCTION_BEGIN;
	g_logger.debug( "%.8x : %s:%s 玩家断开连接!",m_Player,m_szAccount,m_Player?m_Player->getName():"" );
	notifyGateRemoveUser();
	CUserEngine::getMe().RemoveCreature(m_Player);
	return true;
}

_TH_VAR_INIT_ARRAY(char,tls_savedate_charbuffer,1024*512+1,'\0');


bool CGameGateWayUser::OnstUserReLoginGameSvr(stUserReLoginGameSvr* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
	stUserReLoginRet loginret;
	loginret.fsvrver=g_gameSvrVer;
	loginret.nSvrZoneid=GameService::getMe().m_nZoneid;
	loginret.nSvrIndex=0;
	loginret.btReloginType = pcmd->btReloginType;
	loginret.nErrorCode=_LOGIN_RET_TOKEN_ERROR_;//重登陆数据异常 IP改变或则loginsvr重启过 找不到该帐号
	GameService * gamesvr=GameService::instance();
	CGameGateWayUser* puser=this;
	bool isnewuser=(puser->m_Player==NULL);
	bool tmploginok=false;
	m_OwnerLoginSvr=gamesvr->m_loginsvrconnter;
	if (pcmd->btReloginType== _CHANGEZONE_RELOGIN_GAMESVR_ ||(m_OwnerLoginSvr 
		&& pcmd->logintoken.isvalid(pcmd->tokencheck,pcmd->loginsvr_id_type,clientip) 
		&& pcmd->gamesvr_id_type==gamesvr->m_Svr2SvrLoginCmd.svr_id_type_value))
	{
		stLoadPlayerDataCmd loadcmd;
		loginret.nErrorCode=_LOGIN_RET_NOTIFYGATE_ERROR_;	//通知网关失败
		loadcmd.i64UserOnlyId=pcmd->i64UserOnlyId;
		loadcmd.btmapsubline=pcmd->btmapsubline;
		if (notifyGateAddUser(NULL,0)){
			loginret.nErrorCode=_LOGIN_RET_DBSVRASSERT_;		//数据库服务器正在维护
			m_OwnerDbSvr= GameService::getMe().m_dbsvrconnecter;
			if (m_OwnerDbSvr){
				DWORD curmapid=0;
				if (m_Player){
					CPlayerObj* player=m_Player;
					if (player->m_pGateUser==puser
						&& player->m_btSelUserState==_PLAYER_STATE_NETCUT_ 
						&& player==m_Player 
						&& strcmp( pcmd->szPlayerName,player->getName())==0 ){
							do {
								AILOCKT(CUserEngine::getMe().m_loadokplayers);
								CUserEngine::getMe().m_playerhash.removeValue(m_Player);
								if (CUserEngine::getMe().m_loadokplayers.addValue(m_Player)){
									m_Player->m_addloadoktime=time(NULL);
									loginret.nErrorCode=_LOGIN_RET_SUCCESS_; 
									tmploginok=true;
								}else{
									CUserEngine::getMe().m_playerhash.addValue(m_Player);
									loginret.nErrorCode=_LOGIN_RET_PLAYER_ISLOGIN_; //该用户帐号已经登陆
								}
							} while (false);
					}else{
						loginret.nErrorCode=_LOGIN_RET_PLAYER_ISLOGIN_; //该用户帐号已经登陆
					}
				}else if (pcmd->btReloginType==_SELPLAYER_RELOGIN_GAMESVR_){
					loginret.nErrorCode=_LOGIN_RET_PLAYER_NOTLOGIN_; //该用户帐号没有登陆
					stWaitLoginPlayer* pwait=NULL;
					do{
						AILOCKT(gamesvr->m_waitloginhash);
						pwait=gamesvr->m_waitloginhash.FindByTmpid(pcmd->logintoken.loginsvr_tmpid.tmpid_value);
						if (pwait){
							stSelectPlayerInfo* pselplayerinfo=pwait->FindByName(pcmd->szPlayerName);
							if ( pselplayerinfo && !pwait->isSelFailed() ){
								loginret.nErrorCode=_LOGIN_RET_PLAYER_ISLOGIN_; //该用户帐号已经登陆
								if (pwait->pUser==NULL){
									pwait->fclientver=pcmd->fclientver;
									pwait->ip_type=pcmd->ip_type;
									pwait->pUser=this;
									m_boLoginOk=true;
									tmploginok=true;
									curmapid=pselplayerinfo->dwmapid;
									pwait->Activatetime=time(NULL);
									loginret.nErrorCode=_LOGIN_RET_SUCCESS_;
									//=========
									strcpy_s(m_szAccount,sizeof(m_szAccount),pwait->szAccount);
									strcpy_s(m_szTxSubPlatformName,sizeof(m_szTxSubPlatformName),pcmd->szTxSubPlatformName);
									strcpy_s(m_szMeshineid,sizeof(m_szMeshineid),pcmd->szMechineId);
									m_tmpid=pcmd->logintoken.loginsvr_tmpid;
									m_iptype=pcmd->ip_type;
									m_fclientver=pcmd->fclientver;
								}else{
									pwait=NULL;
								}
							}else{
								g_logger.debug("%s 选择角色 %s (%d:%d:%d)",pcmd->szAccount,pcmd->szPlayerName,(size_t)pselplayerinfo,pwait->btLoginIdx,pwait->btSelIdx );
								pwait=NULL;
							}
						}
					} while (false);
					if (m_boLoginOk && m_OwnerDbSvr){
						bool iswaitret=false;
						//_MAX_SEND_PACKET_SIZE_
						stSavePlayerDataCmd* szzlibbuf=(stSavePlayerDataCmd* )_TH_VAR_PTR(tls_savedate_charbuffer);
						unsigned int nmaxsize=_TH_VAR_SIZEOF(tls_savedate_charbuffer);
						//=========
						strcpy_s(m_szAccount,sizeof(m_szAccount),pcmd->szAccount);
						strcpy_s(m_szTxSubPlatformName,sizeof(m_szTxSubPlatformName),pcmd->szTxSubPlatformName);
						strcpy_s(m_szMeshineid,sizeof(m_szMeshineid),pcmd->szMechineId);
						m_tmpid=pcmd->logintoken.loginsvr_tmpid;
						m_iptype=pcmd->ip_type;
						m_fclientver=pcmd->fclientver;
						if ( m_OwnerDbSvr->findinsavelist(pcmd->szAccount,iswaitret,szzlibbuf,nmaxsize) ){
							loginret.nErrorCode=_LOGIN_RET_PLAYER_ISSAVEING_;
						}else{
							//发送消息向DB请求数据
							loadcmd.mapid=curmapid;
							//DWORD dwtmp=InterlockedIncrement(&puser->m_dwDataIdx);
							loadcmd.checknum=0;//(dwtmp+1);
							//checknum 可以为0 其他tmpid都不允许为0
							loadcmd.tmpid=pwait->tmpid;
							strcpy_s(loadcmd.szAccount,sizeof(loadcmd.szAccount),pcmd->szAccount);
							strcpy_s(loadcmd.szName,sizeof(loadcmd.szName),pcmd->szPlayerName);
							m_OwnerDbSvr->sendcmdex(loadcmd);
						}
					}
				}else if (pcmd->btReloginType==_CHANGESVR_RELOGIN_GAMESVR_ || pcmd->btReloginType==_CHANGEZONE_RELOGIN_GAMESVR_){
					loginret.nErrorCode=_LOGIN_RET_PLAYER_NOTLOGIN_; //该用户帐号没有登陆
					stSvrChangeGameSvrCmd* pChangeSvrData=NULL;
					do{
						AILOCKT(gamesvr->m_waitchangesvrhash);
						if(pcmd->btReloginType==_CHANGEZONE_RELOGIN_GAMESVR_){
							pChangeSvrData=gamesvr->m_waitchangesvrhash.FindByAccount(pcmd->szAccount);
						}else{
							pChangeSvrData=gamesvr->m_waitchangesvrhash.FindByTmpid(pcmd->logintoken.loginsvr_tmpid.tmpid_value);
						}

						if (pChangeSvrData && strcmp(pChangeSvrData->data.gamedata.szName,pcmd->szPlayerName)==0 && strcmp(pChangeSvrData->data.szAccount,pcmd->szAccount)==0){
							gamesvr->m_waitchangesvrhash.removeValue(pChangeSvrData);
						}else{
							pChangeSvrData=NULL;
						}
					} while (false);
					/*
					do{
						if (pChangeSvrData){
							stWaitLoginPlayer* pwait=gamesvr->m_waitloginhash.FindByAccount(pcmd->szAccount);
						}
					}while(false);
					*/
					if (pChangeSvrData){
						//=========
						strcpy_s(m_szAccount,sizeof(m_szAccount)-1,pcmd->szAccount);
						strcpy_s(m_szTxSubPlatformName,sizeof(m_szTxSubPlatformName)-1,pChangeSvrData->data.gamedata.szTxSubPlatformName);
						strcpy_s(m_szMeshineid,sizeof(m_szMeshineid),pcmd->szMechineId);
						m_tmpid=pcmd->logintoken.loginsvr_tmpid;
						m_iptype=pcmd->ip_type;
						m_fclientver=pcmd->fclientver;
						//pChangeSvrData->data.gamedata.btmapsublineid=pcmd->btmapsubline;
						CPlayerObj* player=CUserEngine::getMe().MakeNewHuman(puser,&pChangeSvrData->data.gamedata,true);
						if (player){
							player->m_boIsChangeSvr=true;
							player->m_btChangeSvrSpaceMoveType=pChangeSvrData->spacemovetype;
							if(pcmd->btReloginType==_CHANGEZONE_RELOGIN_GAMESVR_){
								player->m_dwSrcGameSvrIdType = pChangeSvrData->old_gamesvr_id_type;
								player->m_dwSrcZoneId=pChangeSvrData->dwSrcZoneId;
								player->m_dwSrcTrueZoneId = pChangeSvrData->dwSrcTrueZoneId;
								player->m_wSrcTrade=pChangeSvrData->wSrcTradeid;
								if(CUserEngine::getMe().isCrossSvr(pChangeSvrData->dwSrcZoneId) && !CUserEngine::getMe().isCrossSvr()){
									stCrossZoneToLogin retcmd;
									strcpy_s(retcmd.szAccount,sizeof(retcmd.szAccount)-1,player->getAccount());
									retcmd.nCrossZoneId=0;
									retcmd.wCrossTradeid=0;
									gamesvr->Send2LoginSvrs(&retcmd,sizeof(stCrossZoneToLogin));
								}
							}
							stMapInfo tempid;
							tempid.dwmapid=pChangeSvrData->data.gamedata.dwmapid;
							curmapid=tempid.dwmapid;

							player=CUserEngine::getMe().AddLoadOkPlayer(player,true);
							if (player){
								m_boLoginOk=true;
								tmploginok=true;
								loginret.nErrorCode=_LOGIN_RET_SUCCESS_;
							}
						}
						if (player==NULL){
							g_logger.debug("%s 切换服务器,创建角色失败!",pChangeSvrData->data.gamedata.szName);
						}
						__mt_char_dealloc(pChangeSvrData);
						pChangeSvrData=NULL;
					}else{
						g_logger.debug("%s 切换服务器,角色数据未找到!",pcmd->szPlayerName);
					}
				}
			}
		}
	}
	g_logger.debug("玩家%s 选择角色 %s 登陆游戏完成 %d %d !",pcmd->szAccount,pcmd->szPlayerName,loginret.nErrorCode,(BYTE)tmploginok);
	SendProxyCmdEx(loginret);
	if (!tmploginok && isnewuser){
		GameService::getMe().Add2Delete(puser);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CGameGatewaySvrSession::pushMsgQueue(stBasePacket* ppacket,stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	__super::pushMsgQueue(ppacket,pcmd, ncmdlen,bufferparam);
	switch (pcmd->value)
	{
	case stProxyDataCmd::_value:
		{
			stBaseCmd* pluscmd=(stBaseCmd*)((stProxyDataCmd*)pcmd)->pluscmd.getptr();
			int npluscmdlen=((stProxyDataCmd*)pcmd)->pluscmd.size;
			if (npluscmdlen>=sizeof(stBaseCmd)){
				if (pluscmd->value==stCheckSpeedCmd::_value){
					stCheckSpeedCmd* pdstcmd=(stCheckSpeedCmd*)pluscmd;
					pdstcmd->setLog(GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value);
					g_logger.forceLog(zLogger::zFORCE, "(%d)测速信号_recv %d:%s : r:%d(l:%i64d)",pdstcmd->dwProxyCount,pdstcmd->dwCheckIndex,pdstcmd->szCheckStr,pdstcmd->dwLocalTick,::GetTickCount64() );
				}	
			}
		}
	}
}

bool CGameGatewaySvrSession::gatewaymsgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case stSvr2SvrLoginCmd::_value:
		{
			//函数里检查这个包是否合法
			return OnstSvr2SvrLoginCmd((stSvr2SvrLoginCmd*)pcmd,ncmdlen,bufferparam);
		}
		break;
	case stGateTcpStateChangeCmd::_value:
		{
			GameService * gamesvr=GameService::instance();
			stGateTcpStateChangeCmd* pdstcmd=(stGateTcpStateChangeCmd*)pcmd;
			do{
				std::vector<stIpInfoState>& gateipinfos=gamesvr->m_gateipinfos ;
				AILOCKT(gamesvr->cfg_lock);
				for (int i=0;i<pdstcmd->ipinfostates.size;i++){
					bool bofind=false;
					for (size_t j=0;j<gateipinfos.size();j++){
						if ( gateipinfos[j].ipinfo_id==pdstcmd->ipinfostates[i].ipinfo_id 
							&& gateipinfos[j].type==pdstcmd->ipinfostates[i].type
							&& gateipinfos[j].ip.s_addr==pdstcmd->ipinfostates[i].ip.s_addr ){
							gateipinfos[j]=pdstcmd->ipinfostates[i];
							bofind=true;
							break;
						}
					}
					if (!bofind){
						gateipinfos.push_back(pdstcmd->ipinfostates[i]);
					}
				}
				gamesvr->m_GateTcpStateChangeCmd->ipinfostates.clear();
				std::vector<stIpInfoState>::iterator it;
				for (it=gateipinfos.begin();it!=gateipinfos.end();it++){
					gamesvr->m_GateTcpStateChangeCmd->ipinfostates.push_back(*it,__FUNC_LINE__);
				}
				gamesvr->m_GateTcpStateChangeCmdSize=sizeof(*gamesvr->m_GateTcpStateChangeCmd)+gamesvr->m_GateTcpStateChangeCmd->ipinfostates.getarraysize();
			}while(false);
			gamesvr->Send2LoginSvrs(pdstcmd,ncmdlen);
			gamesvr->Send2DBSvrs(pdstcmd,ncmdlen);

			g_logger.debug("获得 游戏网关 %d(%d) 变更连接数据 %d 个",svr_id,svr_type,pdstcmd->ipinfostates.size);
		}
		break;
	case stRefreshGamesvrState::_value:
		{
			GameService * gamesvr = GameService::instance();
			gamesvr->m_gamesvrStateMask = ((stRefreshGamesvrState*)pcmd)->gamestateMask;
		}
		break;
	}
	return true;
}

bool CGameGatewaySvrSession::isvalid(){
	FUNCTION_BEGIN;
	return m_bovalid;
}
time_t CGameGatewaySvrSession::valid_timeout(){
	FUNCTION_BEGIN;
	return 30;
}

bool CGameGatewaySvrSession::OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
	GameService * gamesvr=GameService::instance();
	if (pcmd->dwCheckCode == _PRE_CHECK_CODE_ && pcmd->svr_type==_GAMESVR_GATEWAY_TYPE && ncmdlen>=sizeof(stSvr2SvrLoginCmd)) {
		do{
			bool boAllowServer = false; 
			do{
				AILOCKT(gamesvr->cfg_lock);
				svrinfomapiter it=gamesvr->m_svrlistmap.find(pcmd->svr_marking);
				if(it != gamesvr->m_svrlistmap.end()){
					boAllowServer=true;
					m_svrinfo=it->second;
				}
			}while (false);
			if (boAllowServer) {
				bool bofindthis=false;
				{
					svr_id_type_value = pcmd->svr_id_type_value;
					m_svrinfo.wgame_type=pcmd->wgame_type;
					m_svrinfo.wzoneid=pcmd->wzoneid;
					do{
						AILOCKT(*GetAccepter()->GetClientList());
						DEF1_FUNCTOR_BEGIN(find_id, CLD_ClientSocket* , bool)
						{
							//CLD_ClientSocket * pclient=*_p1;
							if (m_this != _p1 && (m_this->svr_id_type_value == ((CGameGatewaySvrSession *) (_p1))->svr_id_type_value)){
								return true;
							}else if (m_this==_p1 && ((CGameGatewaySvrSession *) (_p1))->m_bovalid){
								m_bofindthis=true;
							}
							return false;
						}
						CGameGatewaySvrSession *	m_this;
						bool m_bofindthis;
						DEF_FUNCTOR_END;

						find_id::_IF find_byidtype;
						find_byidtype.m_this=this;
						find_byidtype.m_bofindthis=false;
						CLD_Accepter::iterator it = find_if(GetAccepter()->begin(), GetAccepter()->end(), find_byidtype);
						if (it != GetAccepter()->end()) {
							g_logger.error("发现重复的服务器连接 id=%d  type=%d", pcmd->svr_id, pcmd->svr_type);
							svr_id_type_value = 0;
							Terminate(__FF_LINE__);
							return true;
						}
						bofindthis=find_byidtype.m_bofindthis;
					}while(false);
				}
				if (!bofindthis){
					//只有第一次收到该连接的这个包才返回加密公钥
					DWORD difftime=(DWORD)time(NULL)-pcmd->m_now;
					if ( difftime>(60*15) ){
						g_logger.forceLog(zLogger::zERROR ,"服务器 %d(%d) [%s:%d] 系统时间异常( %s <> %s ),请校正所有服务器时间后重新启动服务!",
							svr_id,svr_type,GetRemoteAddress(),GetRemotePort(),timetostr(time(NULL)) ,timetostr(pcmd->m_now));
						Terminate(__FF_LINE__);
						return true;
					}else if(difftime>10){
						g_logger.forceLog(zLogger::zDEBUG ,"服务器 %d(%d) [%s:%d] 系统时间差为 %d 秒( %s <> %s )!",svr_id,svr_type,GetRemoteAddress(),GetRemotePort(),difftime,timetostr(time(NULL)) ,timetostr(pcmd->m_now));
					}
					gamesvr->m_Svr2SvrLoginCmd.m_now=time(NULL);
					SendGateCmd((char *) &gamesvr->m_Svr2SvrLoginCmd,sizeof(gamesvr->m_Svr2SvrLoginCmd));
					do {
						AILOCKT(gamesvr->cfg_lock);
						SendGateCmd((char *) gamesvr->m_NotifLoginsvrPublickeyCmd,gamesvr->m_NotifLoginsvrPublickeyCmdSize);
						SendGateCmd((char *) gamesvr->m_NotifMapListChangeCmd,gamesvr->m_NotifMapListChangeCmdSize);
					} while (false);
					g_logger.debug("网关 %d(%d) 连接校验成功!",svr_id,svr_type);
				}
				m_bovalid = true;
			} else {
				Terminate(__FF_LINE__);
			}
		}while(false);
	} else {
		Terminate(__FF_LINE__);
	}
	return true;
}

void CGameGatewaySvrSession::SetAllUserSocketClose(){
	FUNCTION_BEGIN;
	//关闭该网关上所有用户
	CGatewayConnManage::gwc_iter it;
	do {
		AILOCKT(m_userlist);
		CGameGateWayUser* pUser=NULL;
		for (it=m_userlist.begin();it!=m_userlist.end();it++){
			pUser=(CGameGateWayUser*)it->obj;
			//SOCKET s=it->s;
			if (pUser){
				pUser->notifyGateRemoveUser();
				if (pUser->m_Player){
					g_logger.debug( "%.8x : %s:%s AllUserSocketClose通知网关删除玩家!",pUser->m_Player,pUser->m_Player->m_pGateUser?pUser->m_Player->m_pGateUser->m_szAccount:"",pUser->m_Player?pUser->m_Player->getName():"" );
				}else{
					g_logger.debug( "AllUserSocketClose通知网关删除玩家!");
				}
			}
		}
		m_userlist.removeall();
	} while (false);
}
//正在游戏的玩家
//等待删除的玩家
//等待切换地图成功的玩家
//正在保存数据的玩家

//检查所有玩家列表

//所有列表没有一个统一的锁 可能导致检查出现漏洞??最后可以考虑使用同一个锁 或则涉及到这个几个列表操作全部放到一个统一的线程中处理
//但是 正在游戏的玩家列表 是一个单一的列表 所以最终是不会出现一个玩家2次进游戏 
stGateWayUser* CGameGatewaySvrSession::CreateNewUser(stOpenCmd* popencmd,int nopencmdlen,stBaseCmd* ppluscmd,int npluscmdlen,stQueueMsgParam* bufferparam)
{
	FUNCTION_BEGIN;
    GameService* gamesvr=GameService::instance();
	CUserEngine* pUserEngine = CUserEngine::instance();
	if ( gamesvr->m_boshutdown || !pUserEngine->m_boStartup || pUserEngine->m_boIsShutDown ){	return NULL; }
	CGameGateWayUser* puser=NULL;
	//stUserReLoginRet loginret;
	if ( ppluscmd->value==stUserReLoginGameSvr::_value && npluscmdlen>=sizeof(stUserReLoginGameSvr) ){
		stUserReLoginRet loginret;
		loginret.fsvrver=g_gameSvrVer;
		loginret.nErrorCode=_LOGIN_RET_ISLOGIN_;//帐号已登陆
		stUserReLoginGameSvr* pdstcmd=(stUserReLoginGameSvr*)ppluscmd;
		loginret.btReloginType = pdstcmd->btReloginType;
		g_logger.debug("玩家%s 选择角色 %s 尝试登陆游戏 !",pdstcmd->szAccount,pdstcmd->szPlayerName);
		if (pdstcmd->szAccount[0]==0 || pdstcmd->szPlayerName[0]==0){
			return NULL;
		}
		if (strnicmp(pdstcmd->szPlayerName,_LOCAL_SVR_GMNAME_,sizeof(_LOCAL_SVR_GMNAME_)-1)==0){
			g_logger.debug("角色 %s 使用运维保留用户名前缀 " _LOCAL_SVR_GMNAME_" ,禁止登陆游戏!",pdstcmd->szPlayerName);
			return NULL;
		}
		CPlayerObj* player=NULL;
		stWaitLoginPlayer* pWaitPlayer=NULL;
		do{
			//等待加载数据的玩家
			AILOCKT(gamesvr->m_waitloginhash);
			pWaitPlayer=gamesvr->m_waitloginhash.FindByAccount(pdstcmd->szAccount);
			if (pWaitPlayer && pWaitPlayer->pUser){
				SendProxyCmd(popencmd->sidx,popencmd->wgindex,-1,&loginret,sizeof(loginret));
				g_logger.debug( "%s 用户(%d-%d) stUserReLoginGameSvr::CreateNewUser 失败! -1",pdstcmd->szAccount,popencmd->wgindex,popencmd->sidx );
				return NULL;
			}


			do {
				AILOCKT(CUserEngine::getMe().m_loadokplayers);
				player=CUserEngine::getMe().IsPlaying(pdstcmd->szPlayerName);
				if (player!=NULL){
					SendProxyCmd(popencmd->sidx,popencmd->wgindex,-1,&loginret,sizeof(loginret));
					g_logger.debug( "%s 用户(%d-%d) stUserReLoginGameSvr::CreateNewUser 失败! -2",pdstcmd->szAccount,popencmd->wgindex,popencmd->sidx );
					return NULL;
				}
			} while (false);

		} while (false);

		if (!player && !puser){
			if (pWaitPlayer || pdstcmd->btReloginType==_CHANGESVR_RELOGIN_GAMESVR_ || pdstcmd->btReloginType==_CHANGEZONE_RELOGIN_GAMESVR_){
				//没有重复登陆就返回
				puser=CLD_DEBUG_NEW CGameGateWayUser(this);
				puser->svr_id_type_value  = this->svr_id_type_value;
				puser->m_Player=NULL;
				if (npluscmdlen == sizeof(stUserReloginGameSvrEx)) {
					strcpy_s(puser->m_szLoginChannel, sizeof(puser->m_szLoginChannel), ((stUserReloginGameSvrEx*)ppluscmd)->szLoginChannel);
				}
				return puser;
			}
		}
		g_logger.debug( "%s 用户(%d-%d) stUserReLoginGameSvr::CreateNewUser 失败! -10",pdstcmd->szAccount,popencmd->wgindex,popencmd->sidx );
	}
	else if (ppluscmd->value == stFastTransferPlayer::_value && npluscmdlen >= sizeof(stFastTransferPlayer)) {
		stFastTransferPlayer* pdstcmd = (stFastTransferPlayer*)ppluscmd;
		g_logger.debug("玩家%s 选择角色 %s 尝试快速切服 ! ts:%I64d", pdstcmd->account, pdstcmd->playername, timeGetTime());
		if (pdstcmd->account[0] == 0 || pdstcmd->playername[0] == 0) {
			return NULL;
		}
		if (strnicmp(pdstcmd->playername, _LOCAL_SVR_GMNAME_, sizeof(_LOCAL_SVR_GMNAME_) - 1) == 0) {
			g_logger.debug("角色 %s 使用运维保留用户名前缀 " _LOCAL_SVR_GMNAME_" ,禁止登陆游戏!", pdstcmd->playername);
			return NULL;
		}
		do {
			//等待加载数据的玩家
			AILOCKT(gamesvr->m_waitloginhash);
			auto pWaitPlayer = gamesvr->m_waitloginhash.FindByAccount(pdstcmd->account);
			if (pWaitPlayer && pWaitPlayer->pUser) {
				g_logger.debug("%s 用户(%d-%d) stFastTransferPlayer::CreateNewUser 失败! -1", pdstcmd->account, popencmd->wgindex, popencmd->sidx);
				return NULL;
			}
			do {
				AILOCKT(CUserEngine::getMe().m_loadokplayers);
				auto player = CUserEngine::getMe().IsPlaying(pdstcmd->playername);
				if (player != NULL) {
					g_logger.debug("%s 用户(%d-%d) stFastTransferPlayer::CreateNewUser 失败! -2", pdstcmd->account, popencmd->wgindex, popencmd->sidx);
					return NULL;
				}
			} while (false);
		} while (false);

		g_logger.debug("玩家%s 尝试快速切服! ts:%d" __FF_LINE__, pdstcmd->playername, timeGetTime());
		puser = CLD_DEBUG_NEW CGameGateWayUser(this);
		puser->svr_id_type_value = this->svr_id_type_value;
		puser->m_Player = NULL;
		return puser;
	}
	return NULL;
}
bool CGameGateWayUser::OnstFastInnerChangeGameSvr(stFastTransferPlayer* pcmd, unsigned int ncmdlen, stQueueMsgParam* bufferparam)
{
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true, "");
	FUNCTION_MONITOR(15, "");
	stFastTransferPlayerRet retcmd;
	GameService * gamesvr = GameService::instance();
	m_OwnerLoginSvr = gamesvr->m_loginsvrconnter;
	CGameGateWayUser* puser = this;
	bool isnewuser = (puser->m_Player == NULL);
	bool tmploginok = false;
	do {
		if (notifyGateAddUser(NULL, 0)) {
			m_OwnerDbSvr = GameService::getMe().m_dbsvrconnecter;
			if (m_OwnerDbSvr) {
				strcpy_s(m_szAccount, sizeof(m_szAccount) - 1, pcmd->account);
				strcpy_s(m_szTxSubPlatformName, sizeof(m_szTxSubPlatformName) - 1, pcmd->playerBinData.gamedata.szTxSubPlatformName);

				strcpy_s(m_szClientBundleId, sizeof(m_szClientBundleId) - 1, pcmd->szBundleId);
				strcpy_s(m_szClientPlatform, sizeof(m_szClientPlatform) - 1, pcmd->szPlatform);
				strcpy_s(m_szClientVersion, sizeof(m_szClientVersion) - 1, pcmd->szVersion);
//				strcpy_s(m_szMeshineid, sizeof(m_szMeshineid) - 1, pcmd->playerBinData.gamedata.szMeshineid);
				m_tmpid = pcmd->loginsvr_tmpid;
				m_iptype = pcmd->ip_type;
				m_fclientver = pcmd->fclientver;
				CPlayerObj* player = CUserEngine::getMe().MakeNewHuman(puser, &pcmd->playerBinData.gamedata, true);
				if (player) {
					player->m_boIsChangeSvr = true;
					player->m_btChangeSvrSpaceMoveType = pcmd->spacemovetype;

					player = CUserEngine::getMe().AddLoadOkPlayer(player, true);
					if (player) {
						m_boLoginOk = true;
						tmploginok = true;
					}
				}
				if (player == NULL) {
					g_logger.debug("%s 切换服务器,创建角色失败!", pcmd->playername);
				}
			}
		}
	} while (false);

	g_logger.debug("玩家%s 选择角色 %s 切换游戏服务器完成 %d %d !", pcmd->account, pcmd->playername, (BYTE)m_boLoginOk, (BYTE)tmploginok);
	SendProxyCmdEx(retcmd);
	if (!tmploginok && isnewuser) {
		GameService::getMe().Add2Delete(puser);
	}
	return true;
}