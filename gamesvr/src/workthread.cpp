#include "workthread.h"
#include "dbsvrGameConnecter.h"
#include "gamelogic/UsrEngn.h"
#include "timeMonitor.h"
unsigned int __stdcall GameService::SimpleMsgProcessThread(CLD_ThreadBase* pthread,void* param){
	FUNCTION_BEGIN;
	while(!pthread->IsTerminated()){
		ULONGLONG startTick=GetTickCount64();
		do{
			//处理网关发过来的消息
			AILOCKT(m_gatewaysession);
			for (auto it=m_gatewaysession.begin(),next=it;it!=m_gatewaysession.end();it=next){
				++next;
				if (auto socket=*it)
				{
					if (CUserEngine::getMe().m_boIsShutDown){
						socket->Terminate(__FF_LINE__);
					}
					socket->run();
				}
			}	
		} while (false);

		if (m_loginsvrconnter && !m_loginsvrconnter->isTerminate() && m_loginsvrconnter->IsConnected()){
			if (CUserEngine::getMe().m_boIsShutDown && GameService::getMe().m_boAllToOne){
				m_loginsvrconnter->Terminate(__FF_LINE__);
				auto it=m_TcpConnters.getall().find(m_loginsvrconnter);
				if (it!=m_TcpConnters.end()){
					m_TcpConnters.getall().erase(it);
				}
			}
			m_loginsvrconnter->run();
		}

		if (m_dbsvrconnecter){
			m_dbsvrconnecter->run();
		}
		//2分钟内没有成功登陆到游戏服务器的有效角色将被删除
		static std::vector< stWaitLoginPlayer* > g_removed1(64);
		static ULONGLONG nextTick1=GetTickCount64();
		if (GetTickCount64()-nextTick1>500) {
			struct stcheckSession : public CWaitLoginHashManager::removeValue_Pred_Base {
				time_t nowtime;
				stcheckSession(std::vector< stWaitLoginPlayer* >& pv):CWaitLoginHashManager::removeValue_Pred_Base(pv){
					nowtime=time(NULL);
				}
				virtual bool isIt(stWaitLoginPlayer*& value){
					if ( (nowtime-value->Activatetime>_WAIT_INTOGAMESVR_TIME_) ){
						GameService::getMe().Add2Delete(value->pUser);
						return true;
					}
					return false;
				}
				virtual void afterremove(stWaitLoginPlayer*& value){
					SAFE_DELETE(value);
				}
			};
			g_removed1.clear();
			AILOCKT(m_waitloginhash);
			m_waitloginhash.removeOneValue_if(stcheckSession(g_removed1));
			nextTick1=GetTickCount64();
		}

		//2分钟内没有成功登陆到游戏服务器的有效角色将被删除
		static std::vector< stSvrChangeGameSvrCmd* > g_removed(64);
		static ULONGLONG nextTick2 =GetTickCount64();
		if (GetTickCount64()- nextTick2 >500) {
			struct stcheckSession : public CWaitPlayerChangeSvrHashManager::removeValue_Pred_Base {
				time_t nowtime;
				stcheckSession(std::vector< stSvrChangeGameSvrCmd* >& pv):CWaitPlayerChangeSvrHashManager::removeValue_Pred_Base(pv){
					nowtime=time(NULL);
				}
				virtual bool isIt(stSvrChangeGameSvrCmd*& value){
					if ( (nowtime-value->Activatetime>_WAIT_INTOGAMESVR_TIME_) ){
						return true;
					}
					return false;
				}
				virtual void afterremove(stSvrChangeGameSvrCmd*& value){
					__mt_char_dealloc(value);
				}
			};
			g_removed.clear();
			AILOCKT(m_waitchangesvrhash);
			m_waitchangesvrhash.removeOneValue_if(stcheckSession(g_removed));
			nextTick2 =GetTickCount64();
		}

		static ULONGLONG nextTick =GetTickCount64();
		if (GetTickCount64()-nextTick >500) {
			AILOCKT(m_waitdelgateuser);
			if (!m_waitdelgateuser.empty()){
				for (auto gateUser: m_waitdelgateuser)
				{
					SAFE_DELETE(gateUser);
				}
				m_waitdelgateuser.clear();
			}
			nextTick =GetTickCount64();
		}
		if (m_gmservermanageconnecter) { m_gmservermanageconnecter->run(); }
		//50毫秒处理一次
		Sleep(safe_min((DWORD)25,(DWORD)(25-(GetTickCount64()-startTick))));
	}
	return 0;
}

unsigned int __stdcall GameService::LogicProcessThread(CLD_ThreadBase* pthread,void* param){
	FUNCTION_BEGIN;
	while(!pthread->IsTerminated()){
		ULONGLONG startruntick=GetTickCount64();
		try{
			do {
				FUNCTION_WRAPPER(true,"LogicProcessThread::run_connecters");
				//AILOCKT(m_toplock);
#ifndef _GLOBAL_GAMESVR_
				if (m_supergamesvrconnecter){ m_supergamesvrconnecter->run(); }
#ifndef _SKIP_GAME_PROXY_
				if (m_pGameSvrProxyConnecter){ m_pGameSvrProxyConnecter->run(); }
#else
				if(m_globalProxyConnecter){m_globalProxyConnecter->run();}
#endif
				if(m_globalsvrconnecter){m_globalsvrconnecter->run();}
#endif
				if (!m_logsvrconnecters.empty()){
					for (DWORD i=0;i<m_logsvrconnecters.size();i++){
						if (m_logsvrconnecters[i])
						{
							m_logsvrconnecters[i]->run();
						}
					}
				}
			}while(false);
			CUserEngine::getMe().run();
		}catch (std::exception& e){
			g_logger.error("[ %s : PID=%d : TID=%d ] exception: %s",__FUNC_LINE__,::GetCurrentProcessId(),::GetCurrentThreadId(),e.what());
		}
		//50毫秒处理一次
		Sleep(safe_min((DWORD)25,(DWORD)(25-(GetTickCount64()-startruntick))));
	}
	return 0;
}

unsigned int __stdcall GameService::ScriptSqlThread(CLD_ThreadBase* pthread,void* param){
	FUNCTION_BEGIN;
	while(!pthread->IsTerminated()){
		ULONGLONG startruntick=GetTickCount64();
		try{
			CUserEngine::getMe().m_scriptsql.RunSql();
		}catch (std::exception& e){
			g_logger.error("[ %s : PID=%d : TID=%d ] exception: %s",__FUNC_LINE__,::GetCurrentProcessId(),::GetCurrentThreadId(),e.what());
		}
		//50毫秒处理一次
		Sleep(safe_min((DWORD)50,(DWORD)(50-(GetTickCount64()-startruntick))));
	}
	return 0;
}