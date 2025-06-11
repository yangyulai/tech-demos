/**
*2009-6-12 23:29:20
*logingatewaySession.h
**/
#ifndef _CLD_GAMEGATEWAYSESSION_67DFGDFG6J_H_
#define _CLD_GAMEGATEWAYSESSION_67DFGDFG6J_H_
#pragma once

#include "network/gatewaySvrSession.h"
#include "HashManage.h"
#include "server_cmd.h"
#include "cmd/login_cmd.h"

class CLoginSvrGameConnecter;
class CDBSvrGameConnecter;

//////////////////////////////////////////////////////////////////////////
class CPlayerObj;

class CGameGateWayUser:public stGateWayUser{
protected:
public:
	bool m_boLoginOk;							
	stSvrTmpId m_tmpid;							//服务器的ID
	char m_szAccount[_MAX_ACCOUT_LEN_];
	char m_szTxSubPlatformName[_MAX_NAME_LEN_];	//腾讯子平台信息
	char m_szMeshineid[_MAX_NAME_LEN_];	//机器码
	CLoginSvrGameConnecter* m_OwnerLoginSvr;
	CDBSvrGameConnecter* m_OwnerDbSvr;
	CPlayerObj* m_Player;
	DWORD svr_id_type_value;

	BYTE m_iptype;
	stVerNum m_fclientver;

	char m_szClientBundleId[_MAX_TIPS_LEN_];
	char m_szClientPlatform[_MAX_TIPS_LEN_];
	char m_szClientVersion[_MAX_TIPS_LEN_];
	char m_szLoginChannel[_MAX_NAME_LEN_];//渠道ID

	CGameGateWayUser(CGateWaySession* pgate);

	virtual ~CGameGateWayUser();

	virtual bool clientmsgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);
	virtual bool OnclientCloseUser();

	bool OnstUserReLoginGameSvr(stUserReLoginGameSvr* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);
	bool OnstFastInnerChangeGameSvr(stFastTransferPlayer* pcmd, unsigned int ncmdlen, stQueueMsgParam* bufferparam);
	//////////////////////////////////////////////////////////////////////////
	DEC_OP_NEW(CGameGateWayUser);
};


class CGameGatewaySvrSession:public CGateWaySession{
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

	bool isvalid();
	virtual time_t valid_timeout();

	CGameGatewaySvrSession(CLD_AsyncAccepter* Owner, SOCKET s):CGateWaySession(Owner,s,1024*256),m_bovalid(false){
		m_dwMaxRecvPacketLen=_MAX_SEND_PACKET_SIZE_;
	};

	virtual void pushMsgQueue(stBasePacket* ppacket,stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);
	virtual stGateWayUser* CreateNewUser(stOpenCmd* popencmd,int nopencmdlen,stBaseCmd* ppluscmd,int npluscmdlen,stQueueMsgParam* bufferparam);
	virtual bool gatewaymsgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);

	bool OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);

	void SetAllUserSocketClose();
};

#endif		//_CLD_LOGINGATEWAYSESSION_67DFGDFG6J_H_