#include "GmServerManageConnecter.h"
#include "server_cmd.h"
#include "gamesvr.h"
#include "gamelogic/UsrEngn.h" 

void CGmServerManageConnecter::OnAsyncConnect(){
	FUNCTION_BEGIN;
	__super::OnAsyncConnect();
	GameService* service=GameService::instance();

	//连接上服务器后立即发送验证信息
	service->m_Svr2SvrLoginCmd.m_now=time(NULL);
	sendcmd(&service->m_Svr2SvrLoginCmd,sizeof(service->m_Svr2SvrLoginCmd));
}
void CGmServerManageConnecter::OnDisconnect(){
	FUNCTION_BEGIN;
	__super::OnDisconnect();

	m_bovalid = false;
}
bool CGmServerManageConnecter::isvalid(){
	FUNCTION_BEGIN;
	return m_bovalid;
}
time_t CGmServerManageConnecter::valid_timeout(){
	FUNCTION_BEGIN;
	return 30;
}

bool CGmServerManageConnecter::msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case stSvr2SvrLoginCmd::_value:
		{
			return OnstSvr2SvrLoginCmd((stSvr2SvrLoginCmd*)pcmd,ncmdlen,bufferparam);
		}
		break;
	case stSvrSendGMCmd::_value:
		{
			_CHECK_PACKAGE_LEN(stSvrSendGMCmd,ncmdlen);
			stSvrSendGMCmd* sendcmd=(stSvrSendGMCmd*)pcmd;
			stGmCmd curcmd;
			curcmd.btGmLv=sendcmd->lv;
			strcpy_s(curcmd.szName,_MAX_NAME_LEN_-1,sendcmd->szPlayerName);
			strncpy_s(curcmd.szCmdStr,_MAX_RETCMD_LEN_-1,sendcmd->stSendStr.getptr(),sendcmd->stSendStr.getarraysize());
			do {
				AILOCKT(CUserEngine::getMe().m_cmdstrlist);
				CUserEngine::getMe().m_cmdstrlist.push_back(curcmd);
			} while (false);
		}break;
	case stSvrRefreshPlayerCmd::_value:
		{
			_CHECK_PACKAGE_LEN(stSvrRefreshPlayerCmd,ncmdlen);
			stSvrRefreshPlayerCmd* pSrcCmd=(stSvrRefreshPlayerCmd*)pcmd;
			BUFFER_CMD(stSvrRefreshPlayerRetCmd,retcmd,stBasePacket::MAX_PACKET_SIZE);
			retcmd->svr_marking=pSrcCmd->svr_marking;
			strcpy_s(retcmd->szName,_MAX_NAME_LEN_,pSrcCmd->szName);
			CPlayersHashManager::iter it;
			int idx=0;
			do {
				AILOCKT(CUserEngine::getMe().m_loadokplayers);
				for (it=CUserEngine::getMe().m_playerhash.begin();it!=CUserEngine::getMe().m_playerhash.end();it++)
				{
					CPlayerObj* player=it->second;
					if (player && idx<300){
						char buffer[_MAX_NAME_LEN_];
						sprintf_s(buffer,_MAX_NAME_LEN_-1,"<p n=\"%s\"/>",player->getName());
						retcmd->szNameList.push_back(buffer,_MAX_NAME_LEN_,__FUNC_LINE__);
						idx++;
					}
					else {break;}
				}
			}while(false);
			sendcmd(retcmd,sizeof(stSvrRefreshPlayerRetCmd)+retcmd->szNameList.getarraysize());
		}break;
	case stCheckServerRunCmd::_value:
		{
			_CHECK_PACKAGE_LEN(stCheckServerRunCmd,ncmdlen);
			stCheckServerRunCmd* pSrcCmd = (stCheckServerRunCmd*)pcmd;
			stCheckServerRunCmdRet retcmd;
			retcmd.svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
			retcmd.btErrorCode=CUserEngine::getMe().m_boStartup?0:-1;
			CUserEngine::getMe().SendMsg2GmManageSvr(&retcmd,sizeof(retcmd));
		}break;
	}
	return true;
}

bool CGmServerManageConnecter::OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
	if (svr_id_type_value != pcmd->svr_id_type_value) {
		Terminate(__FF_LINE__);
		g_logger.error("连接的服务器(%u:%u <> %u:%u)与配置文件记录不一致...", svr_id, svr_type, pcmd->svr_id, pcmd->svr_type);
		m_bovalid = false;
	} else {
		time_t difftime=time(NULL)-pcmd->m_now;
		if ( difftime>(60*15) ){
			g_logger.forceLog(zLogger::zERROR ,"服务器 %d(%d) [%s:%d] 系统时间异常( %s <> %s ),请校正所有服务器时间后重新启动服务!",
				svr_id,svr_type,GetRemoteAddress(),GetRemotePort(),timetostr(time(NULL)) ,timetostr(pcmd->m_now));
		}else if(difftime>10){
			g_logger.forceLog(zLogger::zDEBUG ,"服务器 %d(%d) [%s:%d] 系统时间差为 %d 秒( %s <> %s )!",svr_id,svr_type,GetRemoteAddress(),GetRemotePort(),difftime,timetostr(time(NULL)) ,timetostr(pcmd->m_now));
		}
		m_bovalid = true;
		g_logger.debug("游戏管理服务器 %d(%d) 登陆校验成功!",svr_id,svr_type);
		GameService* service=GameService::instance();
		stSvrProcessIdCmd ProcCmd;
		ProcCmd.svr_id=service->m_svridx;
		ProcCmd.svr_type=service->m_svrtype;
		sendcmd(&ProcCmd,sizeof(stSvrProcessIdCmd));
	}
	return true;
}
