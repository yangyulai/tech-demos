/**
*2009-6-12 23:29:20
*dbsvrGameConnecter.h
**/
#ifndef _CLD_DBSVRGAMECONNETER_FGEXDFYJ_H_
#define _CLD_DBSVRGAMECONNETER_FGEXDFYJ_H_
#pragma once

#include "network/sockettask.h"
#include "server_cmd.h"
#include "HashManage.h"


class CPlayerObj;

struct stWaitSaveInfo{
	enum{
		_STATE_NONE_,		//�ռӽ��� ��û�ύ������
		_STATE_WAIT_,		//�Ѿ��ύ�����ڵȴ�����
	};
	BYTE curstate;
	BYTE poscount;			//
	DWORD addtime;			//
	DWORD postime;			//

	stSavePlayerDataCmd savedatacmd;

	stWaitSaveInfo(){
		curstate=_STATE_NONE_;
		addtime=(DWORD)time(NULL);
		poscount=0;
		postime=0;
	}

	int getSize(){
		//return (sizeof(*this)+savedatacmd.gamedata.getBinData().getarraysize());
		return (sizeof(*this)-sizeof(savedatacmd.gamedata)+savedatacmd.gamedata.getSaveDataSize());
	}
};


class CDBSvrGameConnecter:public CLoopbufAsyncConnecterTask{
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
	//���ݿ������ �������ݿ��б�
	//////////////////////////////////////////////////////////////////////////
	virtual void OnAsyncConnect();
	virtual void OnDisconnect();
	bool isvalid();
	virtual time_t valid_timeout();
	//////////////////////////////////////////////////////////////////////////
	CDBSvrGameConnecter(stServerInfo* psvrinfo):CLoopbufAsyncConnecterTask(),svr_id_type_value(0),m_bovalid(false),m_waitsaverethash_size(0){
		if (psvrinfo){
			svr_id_type_value=psvrinfo->svr_id_type_value;
			m_svrinfo=*psvrinfo;
			m_waitsaverethash_size=0;
			m_dwMaxRecvPacketLen=_MAX_SEND_PACKET_SIZE_;
		}
	};

	virtual bool msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);
	bool OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);

	//////////////////////////////////////////////////////////////////////////
	//����浵�����ݹ���
	CIntLock m_waitsavelock;
	typedef std::list< stWaitSaveInfo* > savedatalist;
	savedatalist m_waitsavehash;
	savedatalist m_waitsaverethash;
	int m_waitsaverethash_size;

	//�ϲ�2���ύ�ı�����������
	bool savedatacat(stWaitSaveInfo* dstdata,stWaitSaveInfo* before,stWaitSaveInfo* after);
	bool savedataisdiff(stWaitSaveInfo* before,stWaitSaveInfo* after);

	//��Ҫ�õ���������ݲ��Һʹ浵��������޹� ���Բ��ܷ���һ���ɴ浵��������ָ�� 
	bool findinsavelist(const char* szaccount,bool& iswaitret,stSavePlayerDataCmd* pcmdbuf,int nmaxlen);
	
	//
	bool push_back2save(CPlayerObj* player,BYTE savetype);
	bool push_back2save(stSavePlayerDataCmd* pSaveData,BYTE savetype);
	bool insert_head2save(CPlayerObj* player,BYTE savetype);

	//=======
	//���д浵������� 
	//�浵ʧ�ܴ��¼���ȴ��浵�б�����
	bool repush2save(const char* szaccount,DWORD dwchecknum);
	//�浵�ɹ�ɾ���ô浵
	bool removewaitretsave(const char* szaccount,stSavePlayerDataRetCmd* pdstcmd);

	bool haswaitsave();
	int getwaitretsavecount();
	int getsavecount();

	virtual void run();
	//////////////////////////////////////////////////////////////////////////
};

typedef std::map< DWORD,CDBSvrGameConnecter* > dbsvrhashcodemap;


#endif		//_CLD_DBSVRGAMECONNETER_FGEXDFYJ_H_