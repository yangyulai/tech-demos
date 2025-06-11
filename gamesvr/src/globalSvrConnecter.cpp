#include "server_cmd.h"
#include "globalSvrConnecter.h"
#include "gamesvr.h"
#include "gamelogic/UsrEngn.h"

void CGlobalSvrConnecter::OnAsyncConnect(){
	FUNCTION_BEGIN;
	__super::OnAsyncConnect();
	GameService* gamesvr=GameService::instance();
	//�����Ϸ�����������������֤��Ϣ
	gamesvr->m_Svr2SvrLoginCmd.m_now=time(NULL);
	sendcmd(&gamesvr->m_Svr2SvrLoginCmd,sizeof(gamesvr->m_Svr2SvrLoginCmd));
}
void CGlobalSvrConnecter::OnDisconnect(){
	FUNCTION_BEGIN;
	__super::OnDisconnect();
	m_bovalid = false;

	do {
		AILOCKT(CUserEngine::getMe().m_loadokplayers);
		stGlobalOnServerClosed retcmd;
		for (CPlayersHashManager::iter it=  CUserEngine::getMe().m_playerhash.begin();it !=CUserEngine::getMe().m_playerhash.end();it ++)
		{
			CPlayerObj * player = it->second;
			if(player)player->SendMsgToMe(&retcmd,sizeof(stGlobalOnServerClosed));
		}
	} while (0);
}
bool CGlobalSvrConnecter::isvalid(){
	FUNCTION_BEGIN;
	return m_bovalid;
}
time_t CGlobalSvrConnecter::valid_timeout(){
	FUNCTION_BEGIN;
	return 30;
}

bool CGlobalSvrConnecter::msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
	switch(pcmd->value)
	{
	case stSvr2SvrLoginCmd::_value:
		{
			if (OnstSvr2SvrLoginCmd((stSvr2SvrLoginCmd*)pcmd,ncmdlen,bufferparam))
			{
				do {
					AILOCKT(CUserEngine::getMe().m_loadokplayers);
					for (CPlayersHashManager::iter it=  CUserEngine::getMe().m_playerhash.begin();it !=CUserEngine::getMe().m_playerhash.end();it ++)
					{
						CPlayerObj * player = it->second;
						player->UpdateToGlobalSvr();
					}
				} while (false);
				do
				{
					stGlobalGuildGameSvrGetGuild retcmd;
					CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd,sizeof(stGlobalGuildGameSvrGetGuild));
				}while (false);
				//do 
				//{
				//	stGlobalSuperVarLoad loadcmd;
				//	CUserEngine::getMe().SendMsg2GlobalSvr(&loadcmd,sizeof(stGlobalSuperVarLoad));
				//} while (false);
				return true;
			}else return false;
		}break;
	default:
		{
			CUserEngine::getMe().doGlobalSvrCmd(pcmd,ncmdlen);
		}
	}
	return true;
}

bool CGlobalSvrConnecter::OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
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
		g_logger.debug("Global��Ϸ������ %d(%d) ��½У��ɹ�!",svr_id,svr_type);
		if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("OnGlobalSvrConnect");
		}
	}
	return true;
}

