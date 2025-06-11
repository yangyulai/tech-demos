#include "globalproxyConnecter.h"
#include "server_cmd.h"
#include "gamesvr.h"
#include "gamelogic/UsrEngn.h"

void CGlobalProxyConnecter::OnAsyncConnect(){
	FUNCTION_BEGIN;
	__super::OnAsyncConnect();
	GameService* gamesvr=GameService::instance();

	//连接上服务器后立即发送验证信息
	gamesvr->m_Svr2SvrLoginCmd.m_now=time(NULL);
	sendcmd(&gamesvr->m_Svr2SvrLoginCmd,sizeof(gamesvr->m_Svr2SvrLoginCmd));
}
void CGlobalProxyConnecter::OnDisconnect(){
	FUNCTION_BEGIN;
	__super::OnDisconnect();

	m_bovalid = false;
}
bool CGlobalProxyConnecter::isvalid(){
	FUNCTION_BEGIN;
	return m_bovalid;
}
time_t CGlobalProxyConnecter::valid_timeout(){
	FUNCTION_BEGIN;
	return 30;
}

bool CGlobalProxyConnecter::msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case stSvr2SvrLoginCmd::_value:
		{
			return OnstSvr2SvrLoginCmd((stSvr2SvrLoginCmd*)pcmd,ncmdlen,bufferparam);
		}
		break;
	case stCheckServerRunCmd::_value:
		{
			_CHECK_PACKAGE_LEN(stCheckServerRunCmd,ncmdlen);
			stCheckServerRunCmd* pSrcCmd = (stCheckServerRunCmd*)pcmd;
			stCheckServerRunCmdRet retcmd;
			retcmd.svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
			retcmd.btErrorCode=GameService::getMe().m_boStartService?0:-1;
			sendcmd(&retcmd,sizeof(retcmd));
		}break;
	case stProxyMsg2Gamesvr::_value:
		{
			stProxyMsg2Gamesvr* pdstcmd=(stProxyMsg2Gamesvr*)pcmd;
			stBaseCmd* pMsg = (stBaseCmd*)pdstcmd->msg.getptr();
			if (pdstcmd->gamesvr_id_type>0 && pdstcmd->gamesvr_id_type!=GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value)
			{
				return true;
			}
			CUserEngine::getMe().doGameSvrProxyCmd(pMsg,pdstcmd->msg.getarraysize());
		}break;
	}
	return true;
}

bool CGlobalProxyConnecter::OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
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
		g_logger.debug("globalproxy %d(%d) 登陆校验成功!",svr_id,svr_type);
		stSvrProcessIdCmd ProcCmd;
		ProcCmd.svr_id=GameService::getMe().m_svridx;
		ProcCmd.svr_type=GameService::getMe().m_svrtype;
		sendcmd(&ProcCmd,sizeof(stSvrProcessIdCmd));

		std::stringstream strzone;
		strzone << GameService::getMe().m_nTrueZoneid;
		stUpdateZoneList updatezone;
		strcpy_s(updatezone.szZoneList,sizeof(updatezone.szZoneList)-1,strzone.str().c_str());
		sendcmd(&updatezone,sizeof(stUpdateZoneList));
	}
	return true;
}
