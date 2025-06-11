#include "server_cmd.h"
#include "superGameSvrConnecter.h"
#include "gamesvr.h"
#include "gamelogic/UsrEngn.h"

void CSuperGameSvrConnecter::OnAsyncConnect(){
	FUNCTION_BEGIN;
	__super::OnAsyncConnect();
	GameService* gamesvr=GameService::instance();
	//连接上服务器后立即发送验证信息
	gamesvr->m_Svr2SvrLoginCmd.m_now=time(NULL);
	sendcmd(&gamesvr->m_Svr2SvrLoginCmd,sizeof(gamesvr->m_Svr2SvrLoginCmd));
}
void CSuperGameSvrConnecter::OnDisconnect(){
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
bool CSuperGameSvrConnecter::isvalid(){
	FUNCTION_BEGIN;
	return m_bovalid;
}
time_t CSuperGameSvrConnecter::valid_timeout(){
	FUNCTION_BEGIN;
	return 30;
}

bool CSuperGameSvrConnecter::msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;

// 	if(pcmd->cmd ==CMD_GAMESVR2SUPERSVR) {
// 		//从superserver接收到的由别的服务器转来的消息
// 		CUserEngine::getMe().doSuperSvrCmd(pcmd,ncmdlen);
// 	}else if(pcmd->cmd ==CMD_GAMESVRSWITCHMSG){
// 		CUserEngine::getMe().doSuperSvrCmd(pcmd,ncmdlen);
// 	}else{
	switch (pcmd->value)   
	{
	case stSvr2SvrLoginCmd::_value:
		{
			if (OnstSvr2SvrLoginCmd((stSvr2SvrLoginCmd*)pcmd,ncmdlen,bufferparam))
			{
				//CUserEngine::getMe().m_grouphash.SendAllGroupInfo();
				do {
					AILOCKT(CUserEngine::getMe().m_loadokplayers);
					for (CPlayersHashManager::iter it=  CUserEngine::getMe().m_playerhash.begin();it !=CUserEngine::getMe().m_playerhash.end();it ++)
					{
						CPlayerObj * player = it->second;
						player->UpdateToSuperSvr();
					}
					
					do 
					{
						BUFFER_CMD(stSendRankMsgSuperSrv,AllRankCmd,stBasePacket::MAX_PACKET_SIZE);
						stGameSvrGetRankTopTen ranktopcmd;
						ranktopcmd.nRankType=Rank_Max_Count;
						AllRankCmd->msg.push_back((char*)&ranktopcmd,sizeof(stGameSvrGetRankTopTen),__FUNC_LINE__);
						CUserEngine::getMe().SendMsg2SuperSvr(AllRankCmd,sizeof(stSendRankMsgSuperSrv)+AllRankCmd->msg.getarraysize());
					} while (0);

					do 
					{
						BUFFER_CMD(stSendRankMsgSuperSrv,AllRankCmd,stBasePacket::MAX_PACKET_SIZE);
						stGetCrossSvrRankTopTen ranktopcmd;
						ranktopcmd.nRankType=0;
						AllRankCmd->msg.push_back((char*)&ranktopcmd,sizeof(stGetCrossSvrRankTopTen),__FUNC_LINE__);
						CUserEngine::getMe().SendMsg2SuperSvr(AllRankCmd,sizeof(stGetCrossSvrRankTopTen)+AllRankCmd->msg.getarraysize());
					} while (0);
					
				} while (false);
				return true;
			}else return false;
		}
		break;
	default:
		{
			CUserEngine::getMe().doSuperSvrCmd(pcmd,ncmdlen);
		}break;
	}
//	}
	return true;
}

bool CSuperGameSvrConnecter::OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
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
		g_logger.debug("全局游戏服务器 %d(%d) 登陆校验成功!",svr_id,svr_type);
	}
	return true;
}

