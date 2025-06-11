/**
*2009-6-12 23:29:20
*logingatewayConnecter.h
**/
#ifndef _CLD_CHECKNAMESVRCONNECTER_RY6UKHSDFSDFH_
#define _CLD_CHECKNAMESVRCONNECTER_RY6UKHSDFSDFH_
#pragma once

#include "network/sockettask.h"
#include "server_cmd.h"

class CCheckNameSvrConnecter:public CLoopbufAsyncConnecterTask{
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
	//令牌解密
	//CEncrypt m_enc;
	//帐号服务器 管理数据库列表
	//////////////////////////////////////////////////////////////////////////
	virtual void OnAsyncConnect();
	virtual void OnDisconnect();
	bool isvalid();
	virtual time_t valid_timeout();
	//////////////////////////////////////////////////////////////////////////
	CCheckNameSvrConnecter(stServerInfo* psvrinfo):CLoopbufAsyncConnecterTask(),svr_id_type_value(0),m_bovalid(false){
		if (psvrinfo){
			svr_id_type_value=psvrinfo->svr_id_type_value;
			m_svrinfo=*psvrinfo;
		}
		m_dwMaxRecvPacketLen=_MAX_SEND_PACKET_SIZE_;
	};

	virtual bool msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);
	bool OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);
};

#endif		//_CLD_CHECKNAMESVRCONNECTER_RY6UKHSDFSDFH_