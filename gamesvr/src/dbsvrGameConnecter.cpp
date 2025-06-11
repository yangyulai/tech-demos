#include "dbsvrGameConnecter.h"
#include "server_cmd.h"
#include "gamesvr.h"
#include "gamelogic/UsrEngn.h"
#include "gamelogic/PlayerObj.h"
#include "gamelogic/Chat.h"
//sssssvvvvvnnnn
static DWORD g_dwSaveCheckSum=0;
void CDBSvrGameConnecter::OnAsyncConnect(){
	FUNCTION_BEGIN;
	__super::OnAsyncConnect();
	GameService* gamesvr=GameService::instance();
	//连接上服务器后立即发送验证信息
	gamesvr->m_Svr2SvrLoginCmd.m_now=time(NULL);
	sendcmd(&gamesvr->m_Svr2SvrLoginCmd,sizeof(gamesvr->m_Svr2SvrLoginCmd));
	g_dwSaveCheckSum=_random(1000)+100;
}
void CDBSvrGameConnecter::OnDisconnect(){
	FUNCTION_BEGIN;
	__super::OnDisconnect();
	m_bovalid = false;
}
bool CDBSvrGameConnecter::isvalid(){
	FUNCTION_BEGIN;
	return m_bovalid;
}
time_t CDBSvrGameConnecter::valid_timeout(){
	FUNCTION_BEGIN;
	return 30;
}

bool CDBSvrGameConnecter::msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
	switch (pcmd->value)
	{
	case stSvr2SvrLoginCmd::_value:
		{
			return OnstSvr2SvrLoginCmd((stSvr2SvrLoginCmd*)pcmd,ncmdlen,bufferparam);
		}
		break;
	case stNotifDBHashCodeChangeCmd::_value:
		{
		}
		break;
	case stPlayerChangeNameRetCmd::_value:
		{
			stPlayerChangeNameRetCmd* pdstcmd=(stPlayerChangeNameRetCmd*)pcmd;
 			do {
				AILOCKT(CUserEngine::getMe().m_syncmsglist);
				CUserEngine::getMe().m_syncmsglist.push_back(bufferparam->pQueueMsgBuffer);
				bufferparam->bofreebuffer=false;
			} while (false);
		}break;
	case stLoadPlayerDataRetCmd::_value:
		{
			stLoadPlayerDataRetCmd* pdstcmd=(stLoadPlayerDataRetCmd*)pcmd;
			GameService* gamesvr=GameService::instance();
			stWaitLoginPlayer* pwait=NULL;
			CGameGateWayUser* pPlayerUser=NULL;
			bool isselok=false;
			do {
				AILOCKT(gamesvr->m_waitloginhash);
				pwait=gamesvr->m_waitloginhash.FindByTmpid(pdstcmd->tmpid.tmpid_value);
				if ( pwait && pwait->CheckSelByName(pdstcmd->gamedata.szName) ){
					gamesvr->m_waitloginhash.removeValue(pwait);
					if (pdstcmd->nLoadErrorCode==0 
						&& pwait->pUser)
					{
						pPlayerUser=pwait->pUser;
						isselok=pwait->isSelOk();
						g_logger.debug("玩家 %s %s 游戏数据加载成功!",pwait->szAccount,pdstcmd->gamedata.szName);
					}else{
						g_logger.error("玩家 %s %s 游戏数据加载失败(%d)!",pwait->szAccount,pdstcmd->gamedata.szName,pdstcmd->nLoadErrorCode);
					}
					SAFE_DELETE(pwait);
				}
				if (pPlayerUser==NULL){
					g_logger.error("角色 %s 游戏数据加载失败(%d)!",pdstcmd->gamedata.szName,pdstcmd->nLoadErrorCode);
				}else{
					if ( !(pdstcmd->getSize()<=(int)ncmdlen && CUserEngine::getMe().AddLoadOkPlayer( CUserEngine::getMe().MakeNewHuman(pPlayerUser,&pdstcmd->gamedata),isselok  )!=NULL) ){
						g_logger.debug("%s 创建角色失败!",pdstcmd->gamedata.szName);
						GameService::getMe().Add2Delete(pPlayerUser);
					}
				}
			} while (false);
		}
		break;
	case stSavePlayerDataRetCmd::_value:
		{
			stSavePlayerDataRetCmd* pdstcmd=(stSavePlayerDataRetCmd*)pcmd;
			GameService* gamesvr=GameService::instance();
			if (pdstcmd->nSaveErrorCode==0){
				//存档成功 删除该存档请求
				removewaitretsave(pdstcmd->szAccount,pdstcmd);
				g_logger.debug("玩家 %s 角色 %s 游戏数据存档成功! num:%d",pdstcmd->szAccount,pdstcmd->szName,pdstcmd->checknum);
			}else{
				if (pdstcmd->nSaveErrorCode==-10){
					removewaitretsave(pdstcmd->szAccount,pdstcmd);
					g_logger.error("玩家 %s 角色 %s 游戏数据存档顺序出现异常,更新的数据已经保存! ErrorCode(%d) num:%d",pdstcmd->szAccount,pdstcmd->szName,pdstcmd->nSaveErrorCode,pdstcmd->checknum);
				}else{
					repush2save(pdstcmd->szAccount,pdstcmd->checknum);
					g_logger.error("玩家 %s 角色 %s 游戏数据存档失败,重新加入存档队列(%d / %d) ErrorCode(%d) num:%d",pdstcmd->szAccount,pdstcmd->szName,getwaitretsavecount(),getsavecount(),pdstcmd->nSaveErrorCode,pdstcmd->checknum);
				}
			}
		}
		break;	
	case stLoadDataToRobotRet::_value:
		{
			stLoadDataToRobotRet* pdstcmd = (stLoadDataToRobotRet*)pcmd;
			CUserEngine::getMe().m_SkyLadder.doLoadRobotData(pdstcmd);			
		}
		break;
	}
	return true;
}

bool CDBSvrGameConnecter::OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam){
	FUNCTION_BEGIN;
	if (svr_id_type_value != pcmd->svr_id_type_value) {
		Terminate(__FF_LINE__);
		g_logger.error("连接的服务器(%u:%u <> %u:%u)与配置文件记录不一致...", svr_id, svr_type, pcmd->svr_id, pcmd->svr_type);
		m_bovalid = false;
	} else {
		DWORD difftime=(DWORD)time(NULL)-pcmd->m_now;
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
			sendcmd((char *) gamesvr->m_pSetDbcolCmd,gamesvr->m_pSetDbcolCmdLen);
		} while (false);
		m_bovalid = true;
		g_logger.debug("数据库服务器 %d(%d) 登陆校验成功!",svr_id,svr_type);
	}
	return true;
}


//////////////////////////////////////////////////////////////////////////

CDBSvrGameConnecter::savedatalist::iterator findbyaccount(CDBSvrGameConnecter::savedatalist& list,const char* szaccount){
	for (CDBSvrGameConnecter::savedatalist::iterator it=list.begin();it!=list.end();it++){
		stWaitSaveInfo* pinfo=(*it);
		if (stricmp(pinfo->savedatacmd.szAccount,szaccount)==0){ return it;	}
	}
	return list.end();
}

bool CDBSvrGameConnecter::savedataisdiff(stWaitSaveInfo* before,stWaitSaveInfo* after){
	FUNCTION_BEGIN;
	return true;
	//if (sizeof(before->savedatacmd)==sizeof(after->savedatacmd)){
	//	return (memcmp(&before->savedatacmd,&after->savedatacmd,sizeof(before->savedatacmd) )!=0);
	//}
}

bool CDBSvrGameConnecter::savedatacat(stWaitSaveInfo* dstdata,stWaitSaveInfo* before,stWaitSaveInfo* after){
	FUNCTION_BEGIN;
	dstdata->curstate=stWaitSaveInfo::_STATE_NONE_;
	dstdata->poscount=before->poscount+after->poscount;			//
	dstdata->addtime=safe_min(before->addtime,after->addtime);			//
	dstdata->postime=safe_min(before->postime,after->postime);			//
	dstdata->savedatacmd.tmpid=after->savedatacmd.tmpid;
	if (after->savedatacmd.gamedata.savecount>=before->savedatacmd.gamedata.savecount){
		if (dstdata!=after){ CopyMemory(&dstdata->savedatacmd.gamedata,&after->savedatacmd.gamedata,after->savedatacmd.gamedata.getSaveDataSize()); }
	}else{
		if (dstdata!=before){ CopyMemory(&dstdata->savedatacmd.gamedata,&before->savedatacmd.gamedata,before->savedatacmd.gamedata.getSaveDataSize()); }	
	}
	//
	dstdata->savedatacmd.gamedata.savecount=safe_max(after->savedatacmd.gamedata.savecount,before->savedatacmd.gamedata.savecount);
	dstdata->savedatacmd.gamedata.lastsavetime=safe_max(after->savedatacmd.gamedata.lastsavetime,before->savedatacmd.gamedata.lastsavetime);

	dstdata->savedatacmd.savetype= ((before->savedatacmd.savetype==_SAVE_TYPE_TIMER_)?after->savedatacmd.savetype:before->savedatacmd.savetype);
	return true;
}

bool CDBSvrGameConnecter::push_back2save(CPlayerObj* player,BYTE savetype){
	FUNCTION_BEGIN;
	if(CUserEngine::getMe().isCrossSvr())return true;
	do {
		player->m_dwWaitChangeSvrSaveDataTime=time(NULL);
		bool bochangesvrsave=(savetype==_SAVE_TYPE_CHANGESVR_);
		AILOCKT(m_waitsavelock); 
		savedatalist::iterator it;
		it=findbyaccount(m_waitsavehash,player->m_pGateUser->m_szAccount);
		if (it!=m_waitsavehash.end()){
			//还没有提交的申请中有则合并
			player->m_boWaitChangeSvrSaveData=bochangesvrsave;//true
			PTR_CMD(stWaitSaveInfo,ptmpinfo,getsafepacketbuf());
			stWaitSaveInfo& tmpinfo=*ptmpinfo;
			if (player->fullPlayerSaveData(&tmpinfo.savedatacmd,getsafepacketbuflen()-sizeof(tmpinfo)-1024 ,savetype)>0){
				tmpinfo.savedatacmd.savetype=savetype;
				stWaitSaveInfo* pinfo=(*it);	//before
				savedatacat(pinfo,pinfo,&tmpinfo);
				return true;
			}
		}else{
			it=findbyaccount(m_waitsaverethash,player->m_pGateUser->m_szAccount);
			PTR_CMD(stWaitSaveInfo,ptmpinfo,getsafepacketbuf());
			if (it!=m_waitsaverethash.end()){
				//已经提交的申请中有则检查是否相同 相同则丢弃 不同(isdiff) 则加入未提交申请列表
				if(player->fullPlayerSaveData(&ptmpinfo->savedatacmd,getsafepacketbuflen()-sizeof(*ptmpinfo)-1024 ,savetype)>0){
					ptmpinfo->savedatacmd.savetype=savetype;
					if ( savedataisdiff((*it),ptmpinfo) ){
						stWaitSaveInfo* pwaitsaveinfo=(stWaitSaveInfo*) __mt_char_alloc(ptmpinfo->getSize()+32);
						if (pwaitsaveinfo){
							CopyMemory(pwaitsaveinfo,ptmpinfo,ptmpinfo->getSize());
							m_waitsavehash.push_back(pwaitsaveinfo);
							player->m_boWaitChangeSvrSaveData=bochangesvrsave;//true
							return true;
						}
					}
				}
			}else{
				//2个列表中都没有 则直接加入未提交申请列表
				if(player->fullPlayerSaveData(&ptmpinfo->savedatacmd,getsafepacketbuflen()-sizeof(*ptmpinfo)-1024 ,savetype)>0){
					ptmpinfo->savedatacmd.savetype=savetype;
					stWaitSaveInfo* pwaitsaveinfo=(stWaitSaveInfo*) __mt_char_alloc(ptmpinfo->getSize()+32);
					if (pwaitsaveinfo){
						CopyMemory(pwaitsaveinfo,ptmpinfo,ptmpinfo->getSize());
						m_waitsavehash.push_back(pwaitsaveinfo);
						player->m_boWaitChangeSvrSaveData=bochangesvrsave; //true
						return true;
					}
				}
			}
		}
	} while (false);

	return false;	
}

bool CDBSvrGameConnecter::push_back2save(stSavePlayerDataCmd* pSaveData,BYTE savetype){
	FUNCTION_BEGIN;
	do {
		bool bochangesvrsave=(savetype==_SAVE_TYPE_CHANGESVR_);
		AILOCKT(m_waitsavelock); 
		savedatalist::iterator it;
		it=findbyaccount(m_waitsavehash,pSaveData->szAccount);
		if (it!=m_waitsavehash.end()){
			//还没有提交的申请中有则合并
			PTR_CMD(stWaitSaveInfo,ptmpinfo,getsafepacketbuf());
			stWaitSaveInfo& tmpinfo=*ptmpinfo;
			CopyMemory(&tmpinfo.savedatacmd,pSaveData,pSaveData->getSize());
			tmpinfo.savedatacmd.savetype=savetype;
			stWaitSaveInfo* pinfo=(*it);	//before
			savedatacat(pinfo,pinfo,&tmpinfo);
			return true;
		}else{
			it=findbyaccount(m_waitsaverethash,pSaveData->szAccount);
			PTR_CMD(stWaitSaveInfo,ptmpinfo,getsafepacketbuf());
			if (it!=m_waitsaverethash.end()){
				//已经提交的申请中有则检查是否相同 相同则丢弃 不同(isdiff) 则加入未提交申请列表
				CopyMemory(&ptmpinfo->savedatacmd,pSaveData,pSaveData->getSize());
				ptmpinfo->savedatacmd.savetype=savetype;
				if ( savedataisdiff((*it),ptmpinfo) ){
					stWaitSaveInfo* pwaitsaveinfo=(stWaitSaveInfo*) __mt_char_alloc(ptmpinfo->getSize()+32);
					if (pwaitsaveinfo){
						CopyMemory(pwaitsaveinfo,ptmpinfo,ptmpinfo->getSize());
						m_waitsavehash.push_back(pwaitsaveinfo);
						return true;
					}
				}
			}else{
				//2个列表中都没有 则直接加入未提交申请列表
				CopyMemory(&ptmpinfo->savedatacmd,pSaveData,pSaveData->getSize());
				ptmpinfo->savedatacmd.savetype=savetype;
				stWaitSaveInfo* pwaitsaveinfo=(stWaitSaveInfo*) __mt_char_alloc(ptmpinfo->getSize()+32);
				if (pwaitsaveinfo){
					CopyMemory(pwaitsaveinfo,ptmpinfo,ptmpinfo->getSize());
					m_waitsavehash.push_back(pwaitsaveinfo);
					return true;
				}
			}
		}
	} while (false);

	return false;	
}

bool CDBSvrGameConnecter::insert_head2save(CPlayerObj* player,BYTE savetype){
	FUNCTION_BEGIN;
	do {
		player->m_dwWaitChangeSvrSaveDataTime=time(NULL);
		bool bochangesvrsave=(savetype==_SAVE_TYPE_CHANGESVR_);
		AILOCKT(m_waitsavelock); 
		savedatalist::iterator it;
		it=findbyaccount(m_waitsavehash,player->m_pGateUser->m_szAccount);
		if (it!=m_waitsavehash.end()){
			//还没有提交的申请中有则合并
			player->m_boWaitChangeSvrSaveData=bochangesvrsave; //true
			PTR_CMD(stWaitSaveInfo,ptmpinfo,getsafepacketbuf());
			stWaitSaveInfo& tmpinfo=*ptmpinfo;
			if (player->fullPlayerSaveData(&tmpinfo.savedatacmd,getsafepacketbuflen()-sizeof(tmpinfo)-1024 ,savetype)>0){
				tmpinfo.savedatacmd.savetype=savetype;
				stWaitSaveInfo* pinfo=(*it);	//before
				m_waitsavehash.erase(it);
				savedatacat(pinfo,pinfo,&tmpinfo);
				m_waitsavehash.insert(m_waitsavehash.begin(),pinfo);
				return true;
			}
		}else{
			it=findbyaccount(m_waitsaverethash,player->m_pGateUser->m_szAccount);
			PTR_CMD(stWaitSaveInfo,ptmpinfo,getsafepacketbuf());
			if (it!=m_waitsaverethash.end()){
				//已经提交的申请中有则检查是否相同 相同则丢弃 不同(isdiff) 则加入未提交申请列表
				if(player->fullPlayerSaveData(&ptmpinfo->savedatacmd,getsafepacketbuflen()-sizeof(*ptmpinfo)-1024 ,savetype)>0){
					ptmpinfo->savedatacmd.savetype=savetype;
					if ( savedataisdiff((*it),ptmpinfo) ){
						stWaitSaveInfo* pwaitsaveinfo=(stWaitSaveInfo*) __mt_char_alloc(ptmpinfo->getSize()+32);
						if (pwaitsaveinfo){
							CopyMemory(pwaitsaveinfo,ptmpinfo,ptmpinfo->getSize());
							m_waitsavehash.insert(m_waitsavehash.begin(),pwaitsaveinfo);
							player->m_boWaitChangeSvrSaveData=bochangesvrsave; //true
							return true;
						}
					}
				}
			}else{
				//2个列表中都没有 则直接加入未提交申请列表
				if(player->fullPlayerSaveData(&ptmpinfo->savedatacmd,getsafepacketbuflen()-sizeof(*ptmpinfo)-1024 ,savetype)>0){
					ptmpinfo->savedatacmd.savetype=savetype;
					stWaitSaveInfo* pwaitsaveinfo=(stWaitSaveInfo*) __mt_char_alloc(ptmpinfo->getSize()+32);
					if (pwaitsaveinfo){
						CopyMemory(pwaitsaveinfo,ptmpinfo,ptmpinfo->getSize());
						m_waitsavehash.insert(m_waitsavehash.begin(),pwaitsaveinfo);
						player->m_boWaitChangeSvrSaveData=bochangesvrsave; //true
						return true;
					}
				}
			}
		}
	} while (false);

	return false;	
}

bool CDBSvrGameConnecter::findinsavelist(const char* szaccount,bool& iswaitret,stSavePlayerDataCmd* pcmdbuf,int nmaxlen){
	FUNCTION_BEGIN;
	do {
		iswaitret=false;
		AILOCKT(m_waitsavelock);
		savedatalist::iterator it;
		it=findbyaccount(m_waitsavehash,szaccount);
		if (it!=m_waitsavehash.end()){
			if (pcmdbuf!=NULL && nmaxlen>0){
				stWaitSaveInfo* pinfo=(*it);
				CopyMemory(pcmdbuf,&pinfo->savedatacmd,pinfo->savedatacmd.getSize());
			}
			return true;
		}else{
			it=findbyaccount(m_waitsaverethash,szaccount);
			if (it!=m_waitsaverethash.end()){
				if (pcmdbuf!=NULL && nmaxlen>0){
					stWaitSaveInfo* pinfo=(*it);
					CopyMemory(pcmdbuf,&pinfo->savedatacmd,pinfo->savedatacmd.getSize());
				}
				iswaitret=true;
				return true;
			}
		}
	}while(false);
	return false;
}

bool CDBSvrGameConnecter::repush2save(const char* szaccount,DWORD dwchecknum){
	FUNCTION_BEGIN;
	do {
		AILOCKT(m_waitsavelock); 
		savedatalist::iterator it;
		it=findbyaccount(m_waitsaverethash,szaccount);
		if (it!=m_waitsaverethash.end()){
			stWaitSaveInfo* pbeforsave=(*it);
			if (pbeforsave && pbeforsave->savedatacmd.checknum==dwchecknum){
				m_waitsaverethash.erase(it);
				m_waitsaverethash_size--;
				m_waitsaverethash_size=safe_max(m_waitsaverethash_size,0);

				it=findbyaccount(m_waitsavehash,szaccount);
				if (it!=m_waitsavehash.end()){
					stWaitSaveInfo* paftersave=(*it);
					savedatacat(paftersave,pbeforsave,paftersave);
					__mt_char_dealloc(pbeforsave);
				}else{
					m_waitsavehash.push_back(pbeforsave);
				}
				return true;
			}
		}
	}while(false);
	return false;
}

bool CDBSvrGameConnecter::removewaitretsave(const char* szaccount,stSavePlayerDataRetCmd* pdstcmd){
	FUNCTION_BEGIN;
	CPlayerObj* PlayObject=NULL;
	int nerrorcode=0;
	do {
		AILOCKT(m_waitsavelock); 
		savedatalist::iterator it;
		it=findbyaccount(m_waitsaverethash,szaccount);
		if (it!=m_waitsaverethash.end()){
			nerrorcode++;
			stWaitSaveInfo* pbeforsave=(*it);
			if (pbeforsave && pbeforsave->savedatacmd.checknum==pdstcmd->checknum){
				nerrorcode++;
				do {
					AILOCKT( CUserEngine::getMe().m_loadokplayers );
					CPlayerObj* PlayObject=CUserEngine::getMe().m_playerhash.FindByName(pbeforsave->savedatacmd.gamedata.szName);
					if (PlayObject){
						nerrorcode++;
						PlayObject->m_boWaitChangeSvrSaveData=false;
						g_logger.debug("玩家 %s 角色 %s 游戏数据存档成功,取消等待存档!",pdstcmd->szAccount,pdstcmd->szName);
					}
				} while (false);

				m_waitsaverethash.erase(it);
				m_waitsaverethash_size--;
				m_waitsaverethash_size=safe_max(m_waitsaverethash_size,0);

				static int max_wait_save_time=2;
				static int max_wait_save_count=0;
				if ( max_wait_save_time<(time(NULL)-pbeforsave->postime) || max_wait_save_count<pbeforsave->poscount ){
					max_wait_save_time=(time(NULL)-pbeforsave->postime);
					max_wait_save_count=pbeforsave->poscount;
					g_logger.debug("游戏数据存档成功最长延迟 %d/%d (%d / %d)!",max_wait_save_time,max_wait_save_count,getwaitretsavecount(),getsavecount());
				}

				__mt_char_dealloc(pbeforsave);
				return true;
			}else if (pbeforsave){
				g_logger.debug("玩家 %s 角色 %s 游戏数据存档成功,未找到对应请求存档数据! %d(%d<>%d)",pdstcmd->szAccount,pdstcmd->szName,nerrorcode,pbeforsave->savedatacmd.checknum,pdstcmd->checknum);
				return false;
			}
		}
		// 		it=findbyaccount(m_waitsavehash,szaccount);
		// 		if (it!=m_waitsavehash.end()){
		// 			stWaitSaveInfo* paftersave=(*it);
		// 		}
	}while(false);
	g_logger.debug("玩家 %s 角色 %s 游戏数据存档成功,未找到对应请求存档数据! %d",pdstcmd->szAccount,pdstcmd->szName,nerrorcode);

// 	if (!PlayObject){
// 		do {
// 			AILOCKT( CUserEngine::getMe().m_loadokplayers );
// 			CPlayerObj* PlayObject=CUserEngine::getMe().m_playerhash.FindByName(pdstcmd->szName);
// 			if (PlayObject){
// 				PlayObject->m_boWaitChangeSvrSaveData=false;
// 				g_logger.debug("玩家 %s 角色 %s 游戏数据存档成功,取消等待存档!",pdstcmd->szAccount,pdstcmd->szName);
// 			}
// 		} while (false);
// 	}
	return false;
}

bool CDBSvrGameConnecter::haswaitsave(){
	FUNCTION_BEGIN;
	AILOCKT(m_waitsavelock); 
	return (!m_waitsavehash.empty());
}

int CDBSvrGameConnecter::getwaitretsavecount(){
	FUNCTION_BEGIN;
	return m_waitsaverethash_size;
}

int CDBSvrGameConnecter::getsavecount(){
	FUNCTION_BEGIN;
	AILOCKT(m_waitsavelock); 
	return (m_waitsavehash.size()+m_waitsaverethash.size());
}

#define _MAX_WAITSAVERET_		32

void CDBSvrGameConnecter::run(){
	FUNCTION_BEGIN;
	__super::run();
	int nwait=_MAX_WAITSAVERET_-getwaitretsavecount();
	if ( haswaitsave() /*&& nwait>0*/ && m_bovalid && IsConnected() ){
		while(nwait>0){
			stWaitSaveInfo* psaveinfo=NULL;
			AILOCKT(m_waitsavelock); 
			savedatalist::iterator it;
			it=m_waitsavehash.begin();
			if (it!=m_waitsavehash.end()){
				psaveinfo=(*it);
				if (psaveinfo->savedatacmd.szAccount[0]==0 || psaveinfo->savedatacmd.gamedata.szName[0]==0){
					m_waitsavehash.erase(it);
					__mt_char_dealloc(psaveinfo);
				}else if ( (time(NULL)-psaveinfo->postime)>2 ){
					g_dwSaveCheckSum++;
					psaveinfo->savedatacmd.checknum=g_dwSaveCheckSum;
					if ( sendcmd( &psaveinfo->savedatacmd,psaveinfo->savedatacmd.getSize() ) ){
						g_logger.debug( "发送保存角色 %s 游戏数据请求! num:%d",psaveinfo->savedatacmd.gamedata.szName,psaveinfo->savedatacmd.checknum );

						m_waitsavehash.erase(it);
						m_waitsaverethash.push_back(psaveinfo);
						m_waitsaverethash_size++;
						psaveinfo->poscount++;
						psaveinfo->postime=time(NULL);
						nwait--;
					}else{
						break;
					}
				}else{
					break;
				}
			}else{
				break;
			}
		}
		do{
			stWaitSaveInfo* psaveinfo=NULL;
			AILOCKT(m_waitsavelock); 
			m_waitsaverethash_size=m_waitsaverethash.size();
			savedatalist::iterator it,itnext;
			for(it=m_waitsaverethash.begin(),itnext=it;it!=m_waitsaverethash.end();it=itnext){
				itnext++;
				psaveinfo=(*it);
				if ( psaveinfo && (time(NULL)-psaveinfo->postime)>30 ){
					//repush2save(psaveinfo->savedatacmd.szAccount);
					stWaitSaveInfo* pbeforsave=psaveinfo;
					g_logger.debug("玩家 %s 角色 %s 游戏数据存档超时 30s!",pbeforsave->savedatacmd.szAccount,pbeforsave->savedatacmd.gamedata.szName );
					m_waitsaverethash.erase(it);
					m_waitsaverethash_size--;
					m_waitsaverethash_size=safe_max(m_waitsaverethash_size,0);

					do {
						savedatalist::iterator itws;
						itws=findbyaccount(m_waitsavehash,pbeforsave->savedatacmd.szAccount);
						if (itws!=m_waitsavehash.end()){
							stWaitSaveInfo* paftersave=(*itws);
							savedatacat(paftersave,pbeforsave,paftersave);
							__mt_char_dealloc(pbeforsave);
						}else{
							m_waitsavehash.push_back(pbeforsave);
						}
					} while (false);
				}
			}
		}while(false);
	}
}

//////////////////////////////////////////////////////////////////////////

