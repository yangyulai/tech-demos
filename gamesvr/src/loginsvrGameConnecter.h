/**
*2009-6-12 23:29:20
*loginsvrgameConnecter.h
**/
#ifndef _CLD_LOGINSVRGAMECONNETER_FGEXDFYJ_H_
#define _CLD_LOGINSVRGAMECONNETER_FGEXDFYJ_H_
#pragma once

#include "network/sockettask.h"
#include "server_cmd.h"
#include "HashManage.h"


class CPlayerObj;

struct stWaitSaveAccountDataInfo{
	enum{
		_STATE_NONE_,		//�ռӽ��� ��û�ύ������
		_STATE_WAIT_,		//�Ѿ��ύ�����ڵȴ�����
	};
	BYTE curstate;
	BYTE poscount;			//
	DWORD addtime;			//
	DWORD postime;			//

	stSaveAccountDataCmd savedatacmd;

	stWaitSaveAccountDataInfo(){
		curstate=_STATE_NONE_;
		addtime=(DWORD)time_t(NULL);
		poscount=0;
		postime=0;
	}

	int getSize(){
		return (sizeof(*this));
	}
};

class CLoginSvrGameConnecter:public CLoopbufAsyncConnecterTask{
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
	//���ƽ���
	CEncrypt m_enc;
	//�ʺŷ����� �������ݿ��б�
	//////////////////////////////////////////////////////////////////////////
	virtual void OnAsyncConnect();
	virtual void OnDisconnect();
	bool isvalid();
	virtual time_t valid_timeout();
	//bool decodetoken(stLoginToken* pSrcToken,stLoginToken* pDstToken);

	//////////////////////////////////////////////////////////////////////////
	CLoginSvrGameConnecter(stServerInfo* psvrinfo):CLoopbufAsyncConnecterTask(),svr_id_type_value(0),m_bovalid(false),m_waitsaverethash_size(0){
		if (psvrinfo){
			svr_id_type_value=psvrinfo->svr_id_type_value;
			m_svrinfo=*psvrinfo;
			m_waitsaverethash_size=0;
		}
		m_dwMaxRecvPacketLen=_MAX_SEND_PACKET_SIZE_;
	};

	virtual bool msgParse(stBaseCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);
	bool OnstSvr2SvrLoginCmd(stSvr2SvrLoginCmd* pcmd, unsigned int ncmdlen,stQueueMsgParam* bufferparam);


	//////////////////////////////////////////////////////////////////////////
	//����浵�����ݹ���
	CIntLock m_waitsavelock;
	typedef std::list< stWaitSaveAccountDataInfo* > savedatalist;
	savedatalist m_waitsavehash;
	savedatalist m_waitsaverethash;
	int m_waitsaverethash_size;

	//�ϲ�2���ύ�ı�����������
	bool savedatacat(stWaitSaveAccountDataInfo* dstdata,stWaitSaveAccountDataInfo* before,stWaitSaveAccountDataInfo* after);
	bool savedataisdiff(stWaitSaveAccountDataInfo* before,stWaitSaveAccountDataInfo* after);

	//��Ҫ�õ���������ݲ��Һʹ浵��������޹� ���Բ��ܷ���һ���ɴ浵��������ָ�� 
	bool findinsavelist(const char* szaccount,bool& iswaitret,stSaveAccountDataCmd* pcmdbuf,int nmaxlen);

	//
	bool push_back2save(CPlayerObj* player,BYTE savetype);
	bool insert_head2save(CPlayerObj* player,BYTE savetype);

	//=======
	//���д浵������� 
	//�浵ʧ�ܴ��¼���ȴ��浵�б�����
	bool repush2save(const char* szaccount,DWORD dwchecknum);
	//�浵�ɹ�ɾ���ô浵
	bool removewaitretsave(const char* szaccount,stSaveAccountDataRetCmd* pdstcmd);

	bool haswaitsave();
	int getwaitretsavecount();
	int getsavecount();

	virtual void run();
	//////////////////////////////////////////////////////////////////////////
};

typedef std::map< DWORD,CLoginSvrGameConnecter* > loginsvrhashcodemap;

class CGameGateWayUser;

struct stWaitLoginPlayer{
	stSvrTmpId	 tmpid;
	BYTE ip_type;
	stVerNum fclientver;
	char szAccount[_MAX_ACCOUT_LEN_];
	CGameGateWayUser* pUser;	//�Ƿ��Ѿ���½ �Ѿ���½�����ڵȴ�������Ϸ����
	BYTE btLoginIdx;
	BYTE btPlayerCount;
	BYTE btSelIdx;
	BYTE btmapsublineid;
	stSelectPlayerInfo PlayerInfos[max_palyer_count];
	DWORD Activatetime;

	__inline bool isSelOk(){
		return ((btLoginIdx==btSelIdx) && (btSelIdx!=0xff));
	}
	__inline bool isSelFailed(){
		return ( (btLoginIdx!=btSelIdx) && (btSelIdx!=0xff) && (btLoginIdx!=0xff) );
	}
	__inline stWaitLoginPlayer(){
		btLoginIdx=0xff;
		btSelIdx=0xff;
		btPlayerCount=0;
		btmapsublineid=0;
	}

	__inline stSelectPlayerInfo* FindByName(const char* name){
		if (btLoginIdx==0xff){
			for (int i=0;i<btPlayerCount && i<max_palyer_count;i++){
				if (strcmp(name,PlayerInfos[i].szName)==0){
					btLoginIdx=i;
					return &PlayerInfos[i];
				}
			}
		}
		return NULL;
	}

	__inline bool CheckSelByName(const char* name){
		if ( btLoginIdx>=0 && btLoginIdx<max_palyer_count && btLoginIdx<btPlayerCount ){
			if (strcmp(name,PlayerInfos[btLoginIdx].szName)==0){
				return true;
			}
		}
		return false;
	}
};


struct stLimitWaitLoginPlayerHashtmpid:LimitHash< unsigned __int64,stWaitLoginPlayer*> {
	static __inline const unsigned __int64 mhkey(stWaitLoginPlayer*& e){
		return e->tmpid.tmpid_value;
	}
};

struct stLimitWaitLoginPlayerHashAccount:LimitStrCaseHash< stWaitLoginPlayer*> {
	static __inline const char* mhkey(stWaitLoginPlayer*& e){
		return e->szAccount;
	}
};

// struct stLimitWaitLoginPlayerHashName:LimitStrCaseHash< stWaitLoginPlayer*> {
// 	static __inline const char* mhkey(stWaitLoginPlayer*& e){
// 		return e->szName;
// 	}
// };

class CWaitLoginHashManager : public zLHashManager3< 
	stWaitLoginPlayer*,
	stLimitWaitLoginPlayerHashtmpid,
	stLimitWaitLoginPlayerHashAccount
	//stLimitWaitLoginPlayerHashName
> {
public:
	stWaitLoginPlayer* FindByTmpid(unsigned __int64 tmpid){
		AILOCKT(*this);
		stWaitLoginPlayer* value=NULL;
		if (m_e1.find(tmpid,value)){
			return value;
		}
		return NULL;
	}
	stWaitLoginPlayer* FindByAccount(const char* account){
		AILOCKT(*this);
		stWaitLoginPlayer* value=NULL;
		if (m_e2.find(account,value)){
			return value;
		}
		return NULL;
	}
// 	stWaitLoginPlayer* FindByName(const char* szname){
// 		AILOCKT(*this);
// 		stWaitLoginPlayer* value=NULL;
// 		if (m_e3.find(szname,value)){
// 			return value;
// 		}
// 		return NULL;
// 	}
};


struct stLimitWaitPlayerChangeSvrHashtmpid:LimitHash< unsigned __int64,stSvrChangeGameSvrCmd*> {
	static __inline const unsigned __int64 mhkey(stSvrChangeGameSvrCmd*& e){
		return e->data.tmpid.tmpid_value;
	}
};

struct stLimitWaitPlayerChangeSvrHashAccount:LimitStrCaseHash< stSvrChangeGameSvrCmd*> {
	static __inline const char* mhkey(stSvrChangeGameSvrCmd*& e){
		return e->data.szAccount;
	}
};

// struct stLimitWaitPlayerChangeSvrHashName:LimitStrCaseHash< stSvrChangeGameSvrCmd*> {
// 	static __inline const char* mhkey(stSvrChangeGameSvrCmd*& e){
// 		return e->data.gamedata.szName;
// 	}
// };

class CWaitPlayerChangeSvrHashManager : public zLHashManager3< 
	stSvrChangeGameSvrCmd*,
	stLimitWaitPlayerChangeSvrHashtmpid,
	stLimitWaitPlayerChangeSvrHashAccount
	//stLimitWaitPlayerChangeSvrHashName
> {
public:
	stSvrChangeGameSvrCmd* FindByTmpid(unsigned __int64 tmpid){
		AILOCKT(*this);
		stSvrChangeGameSvrCmd* value=NULL;
		if (m_e1.find(tmpid,value)){
			return value;
		}
		return NULL;
	}
	stSvrChangeGameSvrCmd* FindByAccount(const char* account){
		AILOCKT(*this);
		stSvrChangeGameSvrCmd* value=NULL;
		if (m_e2.find(account,value)){
			return value;
		}
		return NULL;
	}
	// 	stSvrChangeGameSvrCmd* FindByName(const char* szname){
	// 		AILOCKT(*this);
	// 		stSvrChangeGameSvrCmd* value=NULL;
	// 		if (m_e3.find(szname,value)){
	// 			return value;
	// 		}
	// 		return NULL;
	// 	}
};

#endif		//_CLD_LOGINSVRGAMECONNETER_FGEXDFYJ_H_