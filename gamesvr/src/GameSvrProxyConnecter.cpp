#include "server_cmd.h"
#include "GameSvrProxyConnecter.h"
#include "gamesvr.h"
#include "gamelogic/UsrEngn.h"

void CGameSvrProxyConnecter::OnAsyncConnect(){
	FUNCTION_BEGIN;
	__super::OnAsyncConnect();
	GameService* gamesvr=GameService::instance();
	//�����Ϸ�����������������֤��Ϣ
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
		g_logger.error("���ӵķ�����(%u:%u <> %u:%u)�������ļ���¼��һ��...", svr_id, svr_type, pcmd->svr_id, pcmd->svr_type);
		m_bovalid = false;
	} else {
		time_t difftime=time(NULL)-pcmd->m_now;
		if ( difftime>(60*15) ){
			g_logger.forceLog(zLogger::zERROR ,"������ %d(%d) [%s:%d] ϵͳʱ���쳣( %s <> %s ),��У�����з�����ʱ���������������!",
				svr_id,svr_type,GetRemoteAddress(),GetRemotePort(),timetostr(time(NULL)) ,timetostr(pcmd->m_now));
		}else if(difftime>10){
			g_logger.forceLog(zLogger::zDEBUG ,"������ %d(%d) [%s:%d] ϵͳʱ���Ϊ %d ��( %s <> %s )!",svr_id,svr_type,GetRemoteAddress(),GetRemotePort(),difftime,timetostr(time(NULL)) ,timetostr(pcmd->m_now));
		}
		m_bovalid = true;
		g_logger.debug("��Ϸ���������� %d(%d) ��½У��ɹ�!",svr_id,svr_type);
	}
	return true;
}

