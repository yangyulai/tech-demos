#include "loginsvrgameConnecter.h"
#include "server_cmd.h"
#include "gamesvr.h"
#include "gamelogic/UsrEngn.h"
#include "gamelogic/Chat.h"
#include "gamelogic/NpcTrade.h"

//sssssvvvvvnnnn

DWORD g_dwSaveAccountDataCheckSum=0;

void CLoginSvrGameConnecter::OnAsyncConnect(){
	FUNCTION_BEGIN;
	__super::OnAsyncConnect();
	GameService* gamesvr=GameService::instance();
	//连接上服务器后立即发送验证信息
	//连接上服务器后立即发送验证信息
	gamesvr->m_Svr2SvrLoginCmd.m_now=time(NULL);
	sendcmd(&gamesvr->m_Svr2SvrLoginCmd,sizeof(gamesvr->m_Svr2SvrLoginCmd));
}
void CLoginSvrGameConnecter::OnDisconnect(){
	FUNCTION_BEGIN;
	__super::OnDisconnect();
	m_enc.setEncMethod(CEncrypt::ENCDEC_NONE);
	m_bovalid = false;
}
bool CLoginSvrGameConnecter::isvalid(){
	FUNCTION_BEGIN;
	return m_bovalid;
}
time_t CLoginSvrGameConnecter::valid_timeout(){
	FUNCTION_BEGIN;
	return 30;
}

// bool CLoginSvrGameConnecter::decodetoken(stLoginToken* pSrcToken,stLoginToken* pDstToken){
// 	FUNCTION_BEGIN;
// 	if (pSrcToken){
// 		if (isvalid()) {
// 			stLoginToken tmp = *pSrcToken;
// 			if (tmp.decode(&m_enc)){
// 				if (!pDstToken){ pDstToken=pSrcToken; }
// 				CopyMemory(pDstToken, &tmp, sizeof(*pDstToken));
// 				return true;
// 			}
// 		}
// 	}
// 	return false;
// }

bool CLoginSvrGameConnecter::msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case stSvr2SvrLoginCmd::_value:				//预登录命令											0x0101
		{
			return OnstSvr2SvrLoginCmd((stSvr2SvrLoginCmd*)pcmd,ncmdlen,bufferparam);
		}
		break;
	case stNotifLoginsvrPublickeyCmd::_value:	//游戏服务器将登陆服务器的密钥转发给所有网关           0x0102
		{
			stNotifLoginsvrPublickeyCmd* pdstcmd=(stNotifLoginsvrPublickeyCmd*)pcmd;
			if (pdstcmd->svrpublickeys.size==1&& pdstcmd->svrpublickeys[0].svr_id_type_value==svr_id_type_value ){
				m_enc.setEncMethod(CEncrypt::encMethod(pdstcmd->svrpublickeys[0].encodetype));
				m_enc.set_key_rc5((unsigned char*)&pdstcmd->svrpublickeys[0].rc5key,sizeof(pdstcmd->svrpublickeys[0].rc5key),RC5_8_ROUNDS);
				GameService* gamesvr=GameService::instance();
				do{
					AILOCKT(gamesvr->cfg_lock);
					for (int i=0;i<pdstcmd->svrpublickeys.size;i++){
						gamesvr->m_svrpublickeys[pdstcmd->svrpublickeys[i].svr_id_type_value]=pdstcmd->svrpublickeys[i];
					}
					gamesvr->m_NotifLoginsvrPublickeyCmd->svrpublickeys.clear();
					svrpublickeymap::iterator it;
					for (it=gamesvr->m_svrpublickeys.begin();it!=gamesvr->m_svrpublickeys.end();it++){
						gamesvr->m_NotifLoginsvrPublickeyCmd->svrpublickeys.push_back(it->second,__FUNC_LINE__);
					}
					gamesvr->m_NotifLoginsvrPublickeyCmdSize=sizeof(*gamesvr->m_NotifLoginsvrPublickeyCmd)+gamesvr->m_NotifLoginsvrPublickeyCmd->svrpublickeys.getarraysize();
				}while(false);
				gamesvr->SendSvrCmd2GatewaySvrs(pdstcmd,ncmdlen);

				char szpass[1024*2]={0};
				Mem2Hex((char*)&pdstcmd->svrpublickeys[0].rc5key,sizeof(pdstcmd->svrpublickeys[0].rc5key),szpass,sizeof(szpass));
				g_logger.forceLog(zLogger::zINFO,"获得帐号服务器 %d(%d) 公共密钥 %s",svr_id,svr_type,szpass);
			}else{
				Terminate(__FF_LINE__);
				char szpass[1024]={0};
				g_logger.debug("获得帐号服务器 %d(%d) 公共密钥错误!",svr_id,svr_type);
			}
		}
		break;
	case stNotifDBHashCodeChangeCmd::_value:		//客户端重登陆发送的包，发送服务器ID，登陆令牌返回的校验值等，IP类型，客户端版本号 0x0105
		{
		}
		break;
	case stNotifyUserLoginOKCmd::_value:			//客户端重登陆命令的返回包，返回是否成功，服务器版本号，加密算法标示，加密算法密钥 0x0106
		{
			//踢掉该帐号下所有玩家
			stNotifyUserLoginOKCmd* pdstcmd=(stNotifyUserLoginOKCmd*)pcmd;
			GameService* gamesvr=GameService::instance();
			do 	{
				AILOCKT(gamesvr->m_waitloginhash);
				stWaitLoginPlayer* pwait=gamesvr->m_waitloginhash.FindByAccount(pdstcmd->szAccount);
				if (pwait){	
					GameService::getMe().Add2Delete(pwait->pUser);
					gamesvr->m_waitloginhash.removeValue(pwait);
					constructInPlace(pwait);
				}else{
					pwait=CLD_DEBUG_NEW stWaitLoginPlayer;
				}
				if (pwait){
					pwait->btPlayerCount=0;
					pwait->pUser=NULL;
					//pwait->mapid=pdstcmd->mapid;
					pwait->tmpid=pdstcmd->tmpid;
					//pwait->gamesvr_id_type=pdstcmd->gamesvr_id_type;
					pwait->ip_type=pdstcmd->ip_type;
					pwait->fclientver=pdstcmd->fclient;
					strcpy_s(pwait->szAccount,sizeof(pwait->szAccount),pdstcmd->szAccount);
					//strcpy_s(pwait->szName,sizeof(pwait->szName),pdstcmd->szName);
					pwait->btPlayerCount=pdstcmd->players.size;
					for(int i=0;i<pwait->btPlayerCount && i<max_palyer_count;i++){
						pwait->PlayerInfos[i]=pdstcmd->players[i];
					}
					pwait->Activatetime=time(NULL);
					if (!gamesvr->m_waitloginhash.addValue(pwait)){
						SAFE_DELETE(pwait);
					}else{
						g_logger.debug("%s 用户登录成功,游戏服务器初始化有效登陆时间为%d分钟!%d个角色",pdstcmd->szAccount,_WAIT_INTOGAMESVR_TIME_/60,pwait->btPlayerCount);
					}
				}
			} while (false);
			//删除切换服务器列表


			//删除正在游戏列表
			if (pdstcmd->force_login!=0 /*是强制登陆*/){
				do {
					AILOCKT(CUserEngine::getMe().m_loadokplayers);
					CPlayerObj* player=CUserEngine::getMe().m_playerhash.FindByAccount(pdstcmd->szAccount);
					if (player && player->m_pGateUser && !player->m_pGateUser->isSocketClose()){
						BUFFER_CMD(stForceOffLine, offlineCmd, 512);
						offlineCmd->btErrorCode = 1;
						char reasonDesc[256];
						ZeroMemory(reasonDesc, 256);
						sprintf_s(reasonDesc, 255, "强制断线,您的账号在 %s 重新登陆!", inet_ntoa(pdstcmd->clientip));
						offlineCmd->reasonDesc.push_str(reasonDesc);
						player->SendMsgToMe(offlineCmd, sizeof(*offlineCmd) + offlineCmd->reasonDesc.getarraysize());
						
						
						//CChat::sendCenterMsg(player->getName(),"强制断线,您的账号在 %s 重新登陆!",inet_ntoa(pdstcmd->clientip));
						player->m_boIsWaitChangeSvr = false;
						g_logger.warn("%s踢人状态设置%d" __FUNCTION__, player->getName(), KICK_RELOGIN);
						player->m_btKickState=KICK_RELOGIN;
						player->m_dwDoKickTick=GetTickCount64()+500;
						if(player->m_dwCrossKickZoneId){
							stCrossKickPlayer retcmd;
							if(player->m_wCrossKickTradeid && player->m_wCrossKickTradeid!=GameService::getMe().m_nTradeid){
								strcpy_s(retcmd.szAccount,sizeof(retcmd.szAccount)-1,player->GetCrossTradeAccount());
							}else{
								strcpy_s(retcmd.szAccount,sizeof(retcmd.szAccount)-1,player->getAccount());
							}
							retcmd.nCrossZoneId=player->m_dwCrossKickZoneId;
							retcmd.wCrossTradeid=player->m_wCrossKickTradeid;
							retcmd.btKickType=1;
							CUserEngine::getMe().BroadcastGameSvr(&retcmd,sizeof(stCrossKickPlayer),0,true,player->m_dwCrossKickZoneId,player->m_wCrossKickTradeid);
						}
					}
				} while (false);
			}
		}
		break;
	case stNotifyUserSelPlayerCmd::_value:					//客户端选定人物后，发送此包
		{
			//用户选择了角色
			//踢掉该帐号下所有玩家
			stNotifyUserSelPlayerCmd* pdstcmd=(stNotifyUserSelPlayerCmd*)pcmd;
			GameService* gamesvr=GameService::instance();
			if (pdstcmd->gamesvr_id_type==gamesvr->m_Svr2SvrLoginCmd.svr_id_type_value){
				stWaitLoginPlayer* pwait=NULL;
				do {
					AILOCKT(gamesvr->m_waitloginhash);
					pwait=gamesvr->m_waitloginhash.FindByAccount(pdstcmd->szAccount);
					if (pwait){	
						pwait->Activatetime=time(NULL);	
						g_logger.debug("%s 角色选择成功,游戏服务器重置有效登陆时间为%d分钟!%d个角色",pdstcmd->szAccount,_WAIT_INTOGAMESVR_TIME_/60,pwait->btPlayerCount);
						pwait->btSelIdx=pdstcmd->nselectidx;
						pwait->btmapsublineid=pdstcmd->btmapsubline;
					}else{
						AILOCKT(CUserEngine::getMe().m_loadokplayers);
						CPlayerObj* player=CUserEngine::getMe().m_loadokplayers.FindByAccount(pdstcmd->szAccount);
						if (player){
							if (!player->m_boIsSelOk 
								&& player->m_pGateUser 
								&& player->m_pGateUser->m_tmpid.tmpid_value==pdstcmd->tmpid.tmpid_value 
								&& strcmp(player->m_pGateUser->m_szAccount,pdstcmd->szAccount)==0 ){
									player->m_boIsSelOk=true;
							}
						}
					}
				} while (false);
			}else{
				do {
					AILOCKT(CUserEngine::getMe().m_loadokplayers);
					CPlayerObj* player=CUserEngine::getMe().m_playerhash.FindByAccount(pdstcmd->szAccount);
					if (player && player->m_pGateUser && !player->m_pGateUser->isSocketClose()){

						BUFFER_CMD(stForceOffLine, offlineCmd, 512);
						offlineCmd->btErrorCode = 2;
						char reasonDesc[256];
						ZeroMemory(reasonDesc, 256);
						sprintf_s(reasonDesc, 256, "强制断线,您的账号在 %s 重新登陆!", inet_ntoa(pdstcmd->clientip));
						offlineCmd->reasonDesc.push_str(reasonDesc);
						player->SendMsgToMe(offlineCmd, sizeof(*offlineCmd) + offlineCmd->reasonDesc.getarraysize());
						player->m_boIsWaitChangeSvr = false;
						g_logger.warn("%s踢人状态设置%d" __FUNCTION__, player->getName(), KICK_RELOGIN);
						player->m_btKickState=KICK_RELOGIN;
						player->m_dwDoKickTick=GetTickCount64()+500;
						if(player->m_dwCrossKickZoneId){
							stCrossKickPlayer retcmd;
							if(player->m_wCrossKickTradeid && player->m_wCrossKickTradeid!=GameService::getMe().m_nTradeid){
								strcpy_s(retcmd.szAccount,sizeof(retcmd.szAccount)-1,player->GetCrossTradeAccount());
							}else{
								strcpy_s(retcmd.szAccount,sizeof(retcmd.szAccount)-1,player->getAccount());
							}
							retcmd.nCrossZoneId=player->m_dwCrossKickZoneId;
							retcmd.wCrossTradeid=player->m_wCrossKickTradeid;
							retcmd.btKickType=1;
							CUserEngine::getMe().BroadcastGameSvr(&retcmd,sizeof(stCrossKickPlayer),0,true,player->m_dwCrossKickZoneId,player->m_wCrossKickTradeid);
						}
					}
				} while (false);
			}
		}break;
	case stPlayerChangeNameRetCmd::_value:
	case stLoadAccountDataRetCmd::_value:
		{
			do {
				AILOCKT(CUserEngine::getMe().m_syncmsglist);
				CUserEngine::getMe().m_syncmsglist.push_back(bufferparam->pQueueMsgBuffer);
				bufferparam->bofreebuffer=false;
			} while (false);
		}break;
	case stUpdateAccountRmbCmd::_value:
		{
			stUpdateAccountRmbCmd* pdstcmd=(stUpdateAccountRmbCmd*)pcmd;
			GameService* gamesvr=GameService::instance();
			do {
			AILOCKT(CUserEngine::getMe().m_loadokplayers);
				CPlayerObj* player=CUserEngine::getMe().m_playerhash.FindByAccount(pdstcmd->szAccount);
				if (!player){ player=CUserEngine::getMe().m_loadokplayers.FindByAccount(pdstcmd->szAccount);	}
				if ( player && player->m_pGateUser && strcmp(player->m_pGateUser->m_szAccount,pdstcmd->szAccount)==0 ){
					player->m_res[ResID::charge] =0;
					player->UpdateChargeNoChange(pdstcmd->nCurRMB, "rmbupdate");		//更新当前元宝数量 不需要更新账号数据库
					player->m_res[ResID::hischarge] =pdstcmd->nRMBHistory;
					player->m_boAccountDataLoadOk=true;
				}
			} while (false);
		}break;
	case stSendRmb2GameSvrCmd::_value:
		{
			stSendRmb2GameSvrCmd* pdstcmd=(stSendRmb2GameSvrCmd*)pcmd;
			GameService* gamesvr=GameService::instance();
			do {
				AILOCKT(CUserEngine::getMe().m_loadokplayers);
				CPlayerObj* player=CUserEngine::getMe().m_playerhash.FindByName(pdstcmd->szName);
				if (!player){ player=CUserEngine::getMe().m_loadokplayers.FindByName(pdstcmd->szName);	}
				if (!player) { player = CUserEngine::getMe().m_playerhash.FindByAccount(pdstcmd->szAccount); }
				if (!player) { player = CUserEngine::getMe().m_loadokplayers.FindByAccount(pdstcmd->szAccount); }
				if ( player && player->m_pGateUser && strcmp(player->m_pGateUser->m_szAccount,pdstcmd->szAccount)==0 /*&& player->m_boAccountDataLoadOk*/ ){
					g_logger.debug("玩家 %s %s 账号充值 RMB:%d 流水号:%s! giftid:%d",pdstcmd->szAccount,player->getName(),pdstcmd->Point,pdstcmd->Order_No, pdstcmd->game_type);
					if (!pdstcmd->game_type) {
						player->UpdateChargeNoChange(pdstcmd->truePoint, "cashgold");		//更新当前元宝数量 不需要更新账号数据库
						player->m_res[ResID::hischarge] = pdstcmd->nRMBHistory;
						player->m_res[ResID::daycharge] = pdstcmd->nDayRMBHistory;
						player->m_dwLastChargeCcyTime = pdstcmd->lastAddRMBTime;
					}
					//if (player->m_pGateUser && player->m_pGateUser->m_OwnerLoginSvr){
					//	player->m_pGateUser->m_OwnerLoginSvr->push_back2save(player,_SAVE_TYPE_LOGIN_OUT_);
					//}
					GameService::getMe().Send2LogSvr(_SERVERLOG_CHARGE_DETAIL_,0,0,player,
					"%d,'%s','%s',%d,'%s','%s','%s',%d,%d,%d,%d,%d",
					pdstcmd->zoneid,
					pdstcmd->Order_No,
					player->getAccount(),
					pdstcmd->Point,
					timetostr(time(NULL)),
					player->getName(),
					player->GetEnvir() != NULL ? player->GetEnvir()->getMapName():"未知",
					player->GetX(),
					player->GetY(),
					player->m_dwLevel,
					0,
					player->m_dwPlayerOnlineTime
					);
				}
			} while (false);

			
			do {
				stRmbAdd rmbadd;
				strcpy_s(rmbadd.szAccount, _MAX_ACCOUT_LEN_ - 1, pdstcmd->szAccount);
				strcpy_s(rmbadd.Order_No, _MAX_RMB_ORDER_NO_LEN_ - 1, pdstcmd->Order_No);
				strcpy_s(rmbadd.szName, _MAX_NAME_LEN_ - 1, pdstcmd->szName);
				rmbadd.Point = pdstcmd->Point;
				rmbadd.GameType = pdstcmd->game_type;
				rmbadd.rechargetype = pdstcmd->rechargetype;
				AILOCKT(CUserEngine::getMe().m_rmbaddlist);
				CUserEngine::getMe().m_rmbaddlist.push_back(rmbadd);
			} while (false);
		}break;
	case stSaveAccountDataRetCmd::_value:
		{
			return true;
			stSaveAccountDataRetCmd* pdstcmd=(stSaveAccountDataRetCmd*)pcmd;
			GameService* gamesvr=GameService::instance();

			if (pdstcmd->nSaveErrorCode==0){
				//存档成功 删除该存档请求
				removewaitretsave(pdstcmd->szAccount,pdstcmd);
				g_logger.debug("玩家 %s 角色 %s 账号存档数据 存档成功!",pdstcmd->szAccount,pdstcmd->szName);
			}else{
				if (pdstcmd->nSaveErrorCode==-10){
					removewaitretsave(pdstcmd->szAccount,pdstcmd);
					g_logger.error("玩家 %s 角色 %s 账号存档数据 存档顺序出现异常,更新的数据已经保存! ErrorCode(%d)",pdstcmd->szAccount,pdstcmd->szName,pdstcmd->nSaveErrorCode);
				}else{
					repush2save(pdstcmd->szAccount,pdstcmd->checknum);
					g_logger.error("玩家 %s 角色 %s 账号存档数据 存档失败,重新加入存档队列(%d / %d) ErrorCode(%d)",pdstcmd->szAccount,pdstcmd->szName,getwaitretsavecount(),getsavecount(),pdstcmd->nSaveErrorCode);
				}
			}
		}break;
	case stLoginPlayerOnlineCmd::_value:
		{
			stLoginPlayerOnlineCmd* pdstcmd=(stLoginPlayerOnlineCmd*)pcmd;
			do 
			{
				stOnlineNum onlinenum;
				onlinenum.dwAllOnlineNum=pdstcmd->dwAllOnlineNum;
				onlinenum.vOnlineNumArr.clear();
				for (int i=0;i<pdstcmd->OnlineArr.size;i++)
				{
					if (pdstcmd->OnlineArr[i]._p1){
						stlink2<WORD,WORD> slink;
						slink._p1=pdstcmd->OnlineArr[i]._p1;
						slink._p2=pdstcmd->OnlineArr[i]._p2;
						onlinenum.vOnlineNumArr.push_back(slink);
					}
				}
				AILOCKT(CUserEngine::getMe().m_onlinenumlist);
				CUserEngine::getMe().m_onlinenumlist.clear();
				CUserEngine::getMe().m_onlinenumlist.push_back(onlinenum);
			} while (false);
		}break;
	case stChangePlayerDelStateCmd::_value:
		{
#ifndef _GLOBAL_GAMESVR_
			//转发角色删除
			stChangePlayerDelStateCmd* pdstcmd=(stChangePlayerDelStateCmd*)pcmd;
			GameService* gamesvr=GameService::instance();
			if (gamesvr && gamesvr->m_supergamesvrconnecter){
				gamesvr->m_supergamesvrconnecter->sendcmd(pdstcmd,ncmdlen);
			}
#endif
		}break;
	case stCrossKickPlayer::_value:
		{
			stCrossKickPlayer* pdstcmd=(stCrossKickPlayer*)pcmd;
			if(pdstcmd->wCrossTradeid && pdstcmd->wCrossTradeid!=GameService::getMe().m_nTradeid){
				char* szAccount=(char*)vformat("%s+%d",pdstcmd->szAccount,GameService::getMe().m_nTradeid);
				strcpy_s(pdstcmd->szAccount,sizeof(pdstcmd->szAccount)-1,szAccount);
			}
			CUserEngine::getMe().BroadcastGameSvr(pdstcmd,sizeof(stCrossKickPlayer),0,true,pdstcmd->nCrossZoneId,pdstcmd->wCrossTradeid);
		}break;
	case stCreteRoleYaoQingMaCmd::_value:
	{
		stCreteRoleYaoQingMaCmd* pdstcmd = (stCreteRoleYaoQingMaCmd*)pcmd;
		CUserEngine::getMe().SendMsg2GlobalSvr(pdstcmd, sizeof(stCreteRoleYaoQingMaCmd));
	}break;
	case stUserLoginExternInfo::_value:
		{
		}break;
	}
	return true;
}

bool CLoginSvrGameConnecter::OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
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
		do {
			GameService* gamesvr=GameService::instance();
			AILOCKT(gamesvr->cfg_lock);
			sendcmd((char *) gamesvr->m_NotifMapListChangeCmd,gamesvr->m_NotifMapListChangeCmdSize);
			sendcmd((char *) gamesvr->m_GateTcpStateChangeCmd,gamesvr->m_GateTcpStateChangeCmdSize);
		} while (false);
		m_bovalid = true;
		g_logger.debug("帐号服务器 %d(%d) 登陆校验成功!",svr_id,svr_type);
	}
	return true;
}



//////////////////////////////////////////////////////////////////////////

CLoginSvrGameConnecter::savedatalist::iterator findbyaccount(CLoginSvrGameConnecter::savedatalist& list,const char* szaccount){
	for (CLoginSvrGameConnecter::savedatalist::iterator it=list.begin();it!=list.end();it++){
		stWaitSaveAccountDataInfo* pinfo=(*it);
		if (stricmp(pinfo->savedatacmd.szAccount,szaccount)==0){ return it;	}
	}
	return list.end();
}

bool CLoginSvrGameConnecter::savedataisdiff(stWaitSaveAccountDataInfo* before,stWaitSaveAccountDataInfo* after){
	FUNCTION_BEGIN;
	return (before->savedatacmd.gamedata.nRMB!=after->savedatacmd.gamedata.nRMB);
	//if (sizeof(before->savedatacmd)==sizeof(after->savedatacmd)){
	//	return (memcmp(&before->savedatacmd,&after->savedatacmd,sizeof(before->savedatacmd) )!=0);
	//}
}

bool CLoginSvrGameConnecter::savedatacat(stWaitSaveAccountDataInfo* dstdata,stWaitSaveAccountDataInfo* before,stWaitSaveAccountDataInfo* after){
	FUNCTION_BEGIN;
	dstdata->curstate=stWaitSaveAccountDataInfo::_STATE_NONE_;
	dstdata->poscount=before->poscount+after->poscount;			//
	dstdata->addtime=safe_min(before->addtime,after->addtime);			//
	dstdata->postime=safe_min(before->postime,after->postime);			//
	dstdata->savedatacmd.tmpid=after->savedatacmd.tmpid;
	if (after->savedatacmd.gamedata.savecount>=before->savedatacmd.gamedata.savecount){
		if (dstdata!=after){ CopyMemory(&dstdata->savedatacmd.gamedata,&after->savedatacmd.gamedata,after->savedatacmd.gamedata.getSize()); }
	}else{
		if (dstdata!=before){ CopyMemory(&dstdata->savedatacmd.gamedata,&before->savedatacmd.gamedata,before->savedatacmd.gamedata.getSize()); }
	}
	//
	dstdata->savedatacmd.gamedata.savecount=safe_max(after->savedatacmd.gamedata.savecount,before->savedatacmd.gamedata.savecount);
	dstdata->savedatacmd.gamedata.lastsavetime=safe_max(after->savedatacmd.gamedata.lastsavetime,before->savedatacmd.gamedata.lastsavetime);

	dstdata->savedatacmd.savetype= ((before->savedatacmd.savetype==_SAVE_TYPE_TIMER_)?after->savedatacmd.savetype:before->savedatacmd.savetype);
	return true;
}

#define _MAX_SAVEACCOUNTDATAINF_		1024*32

bool CLoginSvrGameConnecter::push_back2save(CPlayerObj* player,BYTE savetype){
	return true;

	//FUNCTION_BEGIN;
	//do {
	//	AILOCKT(m_waitsavelock); 
	//	savedatalist::iterator it;
	//	it=findbyaccount(m_waitsavehash,player->m_pGateUser->m_szAccount);
	//	if (it!=m_waitsavehash.end()){
	//		//还没有提交的申请中有则合并
	//		BUFFER_CMD(stWaitSaveAccountDataInfo,ptmpinfo,_MAX_SAVEACCOUNTDATAINF_);//PTR_CMD(stWaitSaveAccountDataInfo,ptmpinfo,getsafepacketbuf());
	//		stWaitSaveAccountDataInfo& tmpinfo=*ptmpinfo;
	//		if (player->fullAccountSaveData(&tmpinfo.savedatacmd,_MAX_SAVEACCOUNTDATAINF_-sizeof(tmpinfo)-1024 )>0){
	//			tmpinfo.savedatacmd.savetype=savetype;
	//			stWaitSaveAccountDataInfo* pinfo=(*it);	//before
	//			savedatacat(pinfo,pinfo,&tmpinfo);
	//			return true;
	//		}
	//	}else{
	//		it=findbyaccount(m_waitsaverethash,player->m_pGateUser->m_szAccount);
	//		BUFFER_CMD(stWaitSaveAccountDataInfo,ptmpinfo,_MAX_SAVEACCOUNTDATAINF_);//PTR_CMD(stWaitSaveAccountDataInfo,ptmpinfo,getsafepacketbuf());
	//		if (it!=m_waitsaverethash.end()){
	//			//已经提交的申请中有则检查是否相同 相同则丢弃 不同(isdiff) 则加入未提交申请列表
	//			if(player->fullAccountSaveData(&ptmpinfo->savedatacmd,_MAX_SAVEACCOUNTDATAINF_-sizeof(*ptmpinfo)-1024 )>0){
	//				ptmpinfo->savedatacmd.savetype=savetype;
	//				if ( savedataisdiff((*it),ptmpinfo) ){
	//					stWaitSaveAccountDataInfo* pwaitsaveinfo=(stWaitSaveAccountDataInfo*) __mt_char_alloc(ptmpinfo->getSize()+32);
	//					if (pwaitsaveinfo){
	//						CopyMemory(pwaitsaveinfo,ptmpinfo,ptmpinfo->getSize());
	//						m_waitsavehash.push_back(pwaitsaveinfo);
	//						return true;
	//					}
	//				}
	//			}
	//		}else{
	//			//2个列表中都没有 则直接加入未提交申请列表
	//			if(player->fullAccountSaveData(&ptmpinfo->savedatacmd,_MAX_SAVEACCOUNTDATAINF_-sizeof(*ptmpinfo)-1024 )>0){
	//				ptmpinfo->savedatacmd.savetype=savetype;
	//				stWaitSaveAccountDataInfo* pwaitsaveinfo=(stWaitSaveAccountDataInfo*) __mt_char_alloc(ptmpinfo->getSize()+32);
	//				if (pwaitsaveinfo){
	//					CopyMemory(pwaitsaveinfo,ptmpinfo,ptmpinfo->getSize());
	//					m_waitsavehash.push_back(pwaitsaveinfo);
	//					return true;
	//				}
	//			}
	//		}
	//	}
	//} while (false);

	//return false;	
}

bool CLoginSvrGameConnecter::insert_head2save(CPlayerObj* player,BYTE savetype){
	return true;

	//FUNCTION_BEGIN;
	//do {
	//	AILOCKT(m_waitsavelock); 
	//	savedatalist::iterator it;
	//	it=findbyaccount(m_waitsavehash,player->m_pGateUser->m_szAccount);
	//	if (it!=m_waitsavehash.end()){
	//		//还没有提交的申请中有则合并
	//		BUFFER_CMD(stWaitSaveAccountDataInfo,ptmpinfo,_MAX_SAVEACCOUNTDATAINF_);//PTR_CMD(stWaitSaveAccountDataInfo,ptmpinfo,getsafepacketbuf());
	//		stWaitSaveAccountDataInfo& tmpinfo=*ptmpinfo;
	//		if (player->fullAccountSaveData(&tmpinfo.savedatacmd,_MAX_SAVEACCOUNTDATAINF_-sizeof(tmpinfo)-1024 )>0){
	//			tmpinfo.savedatacmd.savetype=savetype;
	//			stWaitSaveAccountDataInfo* pinfo=(*it);	//before
	//			m_waitsavehash.erase(it);
	//			savedatacat(pinfo,pinfo,&tmpinfo);
	//			m_waitsavehash.insert(m_waitsavehash.begin(),pinfo);
	//			return true;
	//		}
	//	}else{
	//		it=findbyaccount(m_waitsaverethash,player->m_pGateUser->m_szAccount);
	//		BUFFER_CMD(stWaitSaveAccountDataInfo,ptmpinfo,_MAX_SAVEACCOUNTDATAINF_);//PTR_CMD(stWaitSaveAccountDataInfo,ptmpinfo,getsafepacketbuf());
	//		if (it!=m_waitsaverethash.end()){
	//			//已经提交的申请中有则检查是否相同 相同则丢弃 不同(isdiff) 则加入未提交申请列表
	//			if(player->fullAccountSaveData(&ptmpinfo->savedatacmd,_MAX_SAVEACCOUNTDATAINF_-sizeof(*ptmpinfo)-1024 )>0){
	//				ptmpinfo->savedatacmd.savetype=savetype;
	//				if ( savedataisdiff((*it),ptmpinfo) ){
	//					stWaitSaveAccountDataInfo* pwaitsaveinfo=(stWaitSaveAccountDataInfo*) __mt_char_alloc(ptmpinfo->getSize()+32);
	//					if (pwaitsaveinfo){
	//						CopyMemory(pwaitsaveinfo,ptmpinfo,ptmpinfo->getSize());
	//						m_waitsavehash.insert(m_waitsavehash.begin(),pwaitsaveinfo);
	//						return true;
	//					}
	//				}
	//			}
	//		}else{
	//			//2个列表中都没有 则直接加入未提交申请列表
	//			if(player->fullAccountSaveData(&ptmpinfo->savedatacmd,_MAX_SAVEACCOUNTDATAINF_-sizeof(*ptmpinfo)-1024 )>0){
	//				ptmpinfo->savedatacmd.savetype=savetype;
	//				stWaitSaveAccountDataInfo* pwaitsaveinfo=(stWaitSaveAccountDataInfo*) __mt_char_alloc(ptmpinfo->getSize()+32);
	//				if (pwaitsaveinfo){
	//					CopyMemory(pwaitsaveinfo,ptmpinfo,ptmpinfo->getSize());
	//					m_waitsavehash.insert(m_waitsavehash.begin(),pwaitsaveinfo);
	//					return true;
	//				}
	//			}
	//		}
	//	}
	//} while (false);

	//return false;	
}

bool CLoginSvrGameConnecter::findinsavelist(const char* szaccount,bool& iswaitret,stSaveAccountDataCmd* pcmdbuf,int nmaxlen){
	return false;

	/*FUNCTION_BEGIN;
	do {
		iswaitret=false;
		AILOCKT(m_waitsavelock);
		savedatalist::iterator it;
		it=findbyaccount(m_waitsavehash,szaccount);
		if (it!=m_waitsavehash.end()){
			if (pcmdbuf!=NULL && nmaxlen>0){
				stWaitSaveAccountDataInfo* pinfo=(*it);
				CopyMemory(pcmdbuf,&pinfo->savedatacmd,pinfo->savedatacmd.getSize());
			}
			return true;
		}else{
			it=findbyaccount(m_waitsaverethash,szaccount);
			if (it!=m_waitsaverethash.end()){
				if (pcmdbuf!=NULL && nmaxlen>0){
					stWaitSaveAccountDataInfo* pinfo=(*it);
					CopyMemory(pcmdbuf,&pinfo->savedatacmd,pinfo->savedatacmd.getSize());
				}
				iswaitret=true;
				return true;
			}
		}
	}while(false);
	return false;*/
}

bool CLoginSvrGameConnecter::repush2save(const char* szaccount,DWORD dwchecknum){
	return true;

	/*FUNCTION_BEGIN;
	do {
		AILOCKT(m_waitsavelock); 
		savedatalist::iterator it;
		it=findbyaccount(m_waitsaverethash,szaccount);
		if (it!=m_waitsaverethash.end()){
			stWaitSaveAccountDataInfo* pbeforsave=(*it);
			if (pbeforsave && pbeforsave->savedatacmd.checknum==dwchecknum){
				m_waitsaverethash.erase(it);
				m_waitsaverethash_size--;
				m_waitsaverethash_size=safe_max(m_waitsaverethash_size,0);

				it=findbyaccount(m_waitsavehash,szaccount);
				if (it!=m_waitsavehash.end()){
					stWaitSaveAccountDataInfo* paftersave=(*it);
					savedatacat(paftersave,pbeforsave,paftersave);
					__mt_char_dealloc(pbeforsave);
				}else{
					m_waitsavehash.push_back(pbeforsave);
				}
				return true;
			}
		}
	}while(false);
	return false;*/
}

bool CLoginSvrGameConnecter::removewaitretsave(const char* szaccount,stSaveAccountDataRetCmd* pdstcmd){
	return true;

	//FUNCTION_BEGIN;
	//do {
	//	AILOCKT(m_waitsavelock); 
	//	savedatalist::iterator it;
	//	it=findbyaccount(m_waitsaverethash,szaccount);
	//	if (it!=m_waitsaverethash.end()){
	//		stWaitSaveAccountDataInfo* pbeforsave=(*it);
	//		if (pbeforsave && pbeforsave->savedatacmd.checknum==pdstcmd->checknum){
	//			m_waitsaverethash.erase(it);
	//			m_waitsaverethash_size--;
	//			m_waitsaverethash_size=safe_max(m_waitsaverethash_size,0);

	//			static int max_wait_save_time=2;
	//			static int max_wait_save_count=0;
	//			if ( max_wait_save_time<(time(NULL)-pbeforsave->postime) || max_wait_save_count<pbeforsave->poscount ){
	//				max_wait_save_time=(time(NULL)-pbeforsave->postime);
	//				max_wait_save_count=pbeforsave->poscount;
	//				g_logger.debug("账号存档数据 存档成功最长延迟 %d/%d (%d / %d)!",max_wait_save_time,max_wait_save_count,getwaitretsavecount(),getsavecount());
	//			}

	//			__mt_char_dealloc(pbeforsave);
	//			return true;
	//		}
	//	}
	//	// 		it=findbyaccount(m_waitsavehash,szaccount);
	//	// 		if (it!=m_waitsavehash.end()){
	//	// 			stWaitSaveAccountDataInfo* paftersave=(*it);
	//	// 		}
	//}while(false);
	//return false;
}

bool CLoginSvrGameConnecter::haswaitsave(){
	FUNCTION_BEGIN;
	AILOCKT(m_waitsavelock); 
	return (!m_waitsavehash.empty());
}

int CLoginSvrGameConnecter::getwaitretsavecount(){
	FUNCTION_BEGIN;
	return m_waitsaverethash_size;
}

int CLoginSvrGameConnecter::getsavecount(){
	FUNCTION_BEGIN;
	AILOCKT(m_waitsavelock); 
	return (m_waitsavehash.size()+m_waitsaverethash.size());
}

#define _MAX_WAITSAVERET_		32

void CLoginSvrGameConnecter::run(){
	FUNCTION_BEGIN;
	__super::run();
	return;

	//int nwait=_MAX_WAITSAVERET_-getwaitretsavecount();
	//if ( haswaitsave() /*&& nwait>0*/ && m_bovalid && IsConnected() ){
	//	while(nwait>0){
	//		stWaitSaveAccountDataInfo* psaveinfo=NULL;
	//		AILOCKT(m_waitsavelock); 
	//		savedatalist::iterator it;
	//		it=m_waitsavehash.begin();
	//		if (it!=m_waitsavehash.end()){
	//			psaveinfo=(*it);
	//			if (psaveinfo->savedatacmd.szAccount[0]==0 || psaveinfo->savedatacmd.szName[0]==0){
	//				m_waitsavehash.erase(it);
	//				__mt_char_dealloc(psaveinfo);
	//			}else if ( (time(NULL)-psaveinfo->postime)>2 ){
	//				g_dwSaveAccountDataCheckSum++;
	//				psaveinfo->savedatacmd.checknum=g_dwSaveAccountDataCheckSum;
	//				if ( sendcmd( &psaveinfo->savedatacmd,psaveinfo->savedatacmd.getSize() ) ){
	//					m_waitsavehash.erase(it);
	//					m_waitsaverethash.push_back(psaveinfo);
	//					m_waitsaverethash_size++;
	//					psaveinfo->poscount++;
	//					psaveinfo->postime=time(NULL);
	//					nwait--;
	//				}else{
	//					break;
	//				}
	//			}else{
	//				break;
	//			}
	//		}else{
	//			break;
	//		}
	//	}
	//	do{
	//		stWaitSaveAccountDataInfo* psaveinfo=NULL;
	//		AILOCKT(m_waitsavelock); 
	//		m_waitsaverethash_size=m_waitsaverethash.size();
	//		savedatalist::iterator it,itnext;
	//		for(it=m_waitsaverethash.begin(),itnext=it;it!=m_waitsaverethash.end();it=itnext){
	//			itnext++;
	//			psaveinfo=(*it);
	//			if ( psaveinfo && (time(NULL)-psaveinfo->postime)>30 ){
	//				//repush2save(psaveinfo->savedatacmd.szAccount);
	//				stWaitSaveAccountDataInfo* pbeforsave=psaveinfo;
	//				g_logger.debug("玩家 %s 角色 %s 账号存档数据 存档超时 30s!",pbeforsave->savedatacmd.szAccount,pbeforsave->savedatacmd.szName );
	//				m_waitsaverethash.erase(it);
	//				m_waitsaverethash_size--;
	//				m_waitsaverethash_size=safe_max(m_waitsaverethash_size,0);

	//				do {
	//					savedatalist::iterator itws;
	//					itws=findbyaccount(m_waitsavehash,pbeforsave->savedatacmd.szAccount);
	//					if (itws!=m_waitsavehash.end()){
	//						stWaitSaveAccountDataInfo* paftersave=(*itws);
	//						savedatacat(paftersave,pbeforsave,paftersave);
	//						__mt_char_dealloc(pbeforsave);
	//					}else{
	//						m_waitsavehash.push_back(pbeforsave);
	//					}
	//				} while (false);
	//			}
	//		}
	//	}while(false);
	//}
}

//////////////////////////////////////////////////////////////////////////

