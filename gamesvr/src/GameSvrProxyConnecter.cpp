#include "server_cmd.h"
#include "GameSvrProxyConnecter.h"
#include "gamesvr.h"
#include "gamelogic/UsrEngn.h"

void CGameSvrProxyConnecter::OnAsyncConnect(){
	FUNCTION_BEGIN;
	__super::OnAsyncConnect();
	GameService* gamesvr=GameService::instance();
	//连接上服务器后立即发送验证信息
	gamesvr->m_Svr2SvrLoginCmd.m_now=time(NULL);
	sendcmd(&gamesvr->m_Svr2SvrLoginCmd,sizeof(gamesvr->m_Svr2SvrLoginCmd));
}
void CGameSvrProxyConnecter::OnDisconnect(){
	FUNCTION_BEGIN;
	__super::OnDisconnect();
// 	do {
// 		GameService* gamesvr=GameService::instance();
// 		AILOCKT(gamesvr->m_dbsvrconnecter);
// 		dbsvrhashcodemap::iterator it,itnext;
// 		for (it=gamesvr->m_dbsvrconnecter_hashcode.begin(),itnext=it;it!=gamesvr->m_dbsvrconnecter_hashcode.end();it=itnext){
// 			itnext++;
// 			if (it->second==this){
// 				gamesvr->m_dbsvrconnecter_hashcode.erase(it);
// 			}
// 		}
// 	} while (false);
	m_bovalid = false;
}
bool CGameSvrProxyConnecter::isvalid(){
	FUNCTION_BEGIN;
	return m_bovalid;
}
time_t CGameSvrProxyConnecter::valid_timeout(){
	FUNCTION_BEGIN;
	return 30;
}

bool CGameSvrProxyConnecter::msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;

	switch (pcmd->value)   
	{
	case stSvr2SvrLoginCmd::_value:
		{
			return OnstSvr2SvrLoginCmd((stSvr2SvrLoginCmd*)pcmd,ncmdlen,bufferparam);
		}
		break;
	default:
		{
			CUserEngine::getMe().doGameSvrProxyCmd(pcmd,ncmdlen);
		}break;
	}
//	}
	return true;
}

bool CGameSvrProxyConnecter::OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
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
		g_logger.debug("游戏服务器代理 %d(%d) 登陆校验成功!",svr_id,svr_type);
	}
	return true;
}

