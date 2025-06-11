#ifndef _CLD_GAMEGAMESVRPROXYCONNETER_4SDJFA0ERS_H_
#define _CLD_GAMEGAMESVRPROXYCONNETER_4SDJFA0ERS_H_
#pragma once

#include "network/sockettask.h"
#include "server_cmd.h"

class CGameSvrProxyConnecter:public CLoopbufAsyncConnecterTask{
public:
	union {
		struct {
			WORD svr_id;
			WORD svr_type;
		};
		DWORD svr_id_type_value;
	};
	stServerInfo m_svrinfo;
	bool m_bovalid;
	//数据库服务器 管理数据库列表
	//////////////////////////////////////////////////////////////////////////
	virtual void OnAsyncConnect();
	virtual void OnDisconnect();
	bool isvalid();
	virtual time_t valid_timeout();
	//////////////////////////////////////////////////////////////////////////
	CGameSvrProxyConnecter(stServerInfo* psvrinfo):CLoopbufAsyncConnecterTask(),svr_id_type_value(0),m_bovalid(false){
		if (psvrinfo){
			svr_id_type_value=psvrinfo->svr_id_type_value;
			m_svrinfo=*psvrinfo;
		}
		m_dwMaxRecvPacketLen=_MAX_SEND_PACKET_SIZE_;
	};

	virtual bool msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);
	bool OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);
};


#endif		//_CLD_SUPERGAMESVRCONNETER_4SDJFA0ERS_H_