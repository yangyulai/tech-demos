#ifndef __GAME_CHAT_H__ASDFWEfewfjk29084231k23kl
#define __GAME_CHAT_H__ASDFWEfewfjk29084231k23kl
#include "zsingleton.h"
#include "UsrEngn.h"
#include "cmdparse.h"
#include "HashManage.h"

#define GETPATTERN(str,pattern) char str[_MAX_CHAT_LEN_*4];ZeroMemory(str,_MAX_CHAT_LEN_*4);getMessage(str,_MAX_CHAT_LEN_*4,pattern);


#define DB_CHATCONFIG_TBL "Config_Chat"

#pragma pack(push,1)

struct stChatConfig{
	DWORD dwId;					//����id
	char szChatType[20];		//��������
	bool boCountryLimit;		//��������
	DWORD dwLevel;				//�ȼ�����
	DWORD dwZsLevel;			//ת������
	DWORD dwGold;				//���
	DWORD dwRumorsExposureRate;	//�����ع���
	DWORD dwTime;				//���ʱ��
	DWORD dwCount;				//���ʱ���ڵĴ���
	DWORD dwItemId;				//�������ID
	DWORD dwMaxChatLength;      //�������ݳ���
	int nVipLevel;
	stChatConfig(){
		ZEROSELF;
	}
};

static dbCol ChatConfig_define[] = { 
	{_DBC_SO_("����id", DB_DWORD, stChatConfig, dwId)}, 
	{_DBC_SO_("��������", DB_STR, stChatConfig, szChatType)}, 
	{_DBC_SO_("�����������", DB_BYTE, stChatConfig, boCountryLimit)}, 
	{_DBC_SO_("������Ҫ�ȼ�", DB_DWORD, stChatConfig, dwLevel)}, 
	{_DBC_SO_("������Ҫת��", DB_DWORD, stChatConfig, dwZsLevel)},
	{_DBC_SO_("������Ҫ��Ǯ", DB_DWORD, stChatConfig, dwGold)},
	{_DBC_SO_("�����ع���", DB_DWORD, stChatConfig, dwRumorsExposureRate)},
	{_DBC_SO_("������ʱ��", DB_DWORD, stChatConfig, dwTime)},
	{_DBC_SO_("���췢�Դ���", DB_DWORD, stChatConfig, dwCount)},
	{_DBC_SO_("������Ҫ����", DB_DWORD, stChatConfig, dwItemId)},
	{_DBC_SO_("������󳤶�", DB_DWORD, stChatConfig, dwMaxChatLength)},
	{_DBC_SO_("����VIP�ȼ�", DB_DWORD, stChatConfig, nVipLevel)},
	{_DBC_MO_NULL_(stChatConfig)}, 
};

// struct stBoard  
// {
// 	int id;
// 	DWORD beginTime;
// 	DWORD endTime;
// 	char szNotice[512];
// 
// 	typedef vector<stBoard> BoardList;
// 
// 	//��ӹ���
// 	static bool add(stBoard& board)
// 	{
// 		result = 1;
// 		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("addincrease",
// 			board.beginTime, board.endTime, board.szNotice);
// 		return result == 0 ? true : false;
// 	}
// 
// 	//ɾ������
// 	static bool del(int id)
// 	{
// 		result = 1;
// 		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("delincrease", id);
// 		return result == 0 ? true : false;
// 	}
// 
// 	//��ȡ���й����б�
// 	static void getList()
// 	{
// 		boardNum = 0;
// 		memset(boards, 0, sizeof(boards));
// 		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("readincrease");
// 	}
// 
// 	//��ȡ�����б�
// 	static void getList(BoardList& boardList)
// 	{
// 		boardList.clear();
// 		for(int i = 0; i < boardNum; ++i)
// 		{
// 			boardList.push_back(boards[i]);
// 		}
// 	}
// 
// 	//���ù�����ʱ��
// 	static bool updateInterval(DWORD itime)
// 	{
// 		result = 1;
// 		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("setnoticeinterval", itime);
// 		return result == 0 ? true : false;
// 	}
// 
// 	//��ȡ������ʱ��
// 	static void readInterval(DWORD& itime)
// 	{
//  		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("getnoticeinterval", &itime);
// // 		CWinApiIni ini(iniPath);
// // 		itime = ini.ReadInt("ʱ����", "����");
// 	}
// 
// 	static bool readRecord(char* str, stBoard& board)
// 	{
// 		if(str == NULL)
// 		{
// 			return false;
// 		}
// 		char* ptr = NULL;
// 		char* newPtr = str;
// 
// 		//��ȡID
// 		ptr = newPtr;
// 		newPtr=strchr(str, '#');
// 		if(NULL == newPtr)
// 		{
// 			return false;
// 		}
// 		*newPtr = 0;
// 		board.id = atoi(ptr);
// 
// 		//��ȡ��ʼʱ��
// 		*newPtr = '#';
// 		newPtr++;
// 		ptr = newPtr;
// 		newPtr = strchr(ptr, '#');
// 		if(NULL == newPtr)
// 		{
// 			return false;
// 		}
// 		*newPtr = 0;
// 		board.beginTime = atoi(ptr);
// 
// 		//��ȡ����ʱ��
// 		*newPtr = '#';
// 		newPtr++;
// 		ptr = newPtr;
// 		newPtr = strchr(ptr, '#');
// 		if(NULL == newPtr)
// 		{
// 			return false;
// 		}
// 		board.endTime = atoi(ptr);
// 
// 		//��ȡ��������
// 		*newPtr = '#';
// 		newPtr++;
// 		ptr = newPtr;
// 		strcpy_s(board.szNotice, sizeof(board.szNotice)-1, ptr);
// 
// 		return true;
// 	}
// 
// 	static char * iniPath;
// 	static int result;
// 	static int boardNum;
// 	static const int MaxBoardNum;
// 	static stBoard boards[20];
// };

#pragma  pack(pop)

enum{
	_GMCMD_INPUT_GMINGAME_ = 0,
	_GMCMD_INPUT_OTHER_ = 1,
};
inline CPlayerObj* s_psystem_virtual_palyer = NULL;
inline bool isSystemVirtualPlayer(CCreature* cret) {
	return (cret == s_psystem_virtual_palyer);
}
class CChat:public Singleton<CChat>
{
public:
	static DWORD m_dwSendGmManage;
public:
	CChat() {m_dwSendGmManage=0;};
	~CChat() {;};
	static void RandomStr(char *pszStr);
	static bool sendPrivate(CPlayerObj *pPlayer,const char *psztoName,const char *pattern, ...);	//����������Ϣ������һ����ɫ
	static bool sendQQPrivate(CPlayerObj *pPlayer,const char *psztoName,const char *pattern, ...);	//����QQ������Ϣ������һ����ɫ
	//static bool sendChatMsg2User(CPlayerObj *pPlayer,emChatType etype,const char *psztoName,const char *pattern, ...);

	static bool sendSpeaker(CPlayerObj *pPlayer,const char *pattern, ...);	//С���ȷ���
	static bool SendWorld(CPlayerObj *pPlayer,const char *pattern, ...);												//����������Ϣ
	static bool SendWorldTeam(CPlayerObj *pPlayer,const char *pattern, ...);						//�������
	static bool sendRefMsg (CPlayerObj *pPlayer,const char *pattern, ...);							//����ͬ��Ļ���
	static bool sendSystem(CPlayerObj *pToPlayer,const char *pattern, ...);		                // ϵͳ��Ϣ
	static bool sendSystem(const char* szName,const char *pattern, ...);								//������������Ϣ��������� ֧�������з���������
	static bool sendSystemToAll(bool btoAllServer,bool boBanner,const char *pattern, ...);											//������Ϣ�����з��������������
	static bool sendSystemByType(const char* szName, emChatType ChatType, const char *pattern, ...);
	static bool sendSystemByType(CPlayerObj *pToPlayer, emChatType ChatType, const char *pattern, ...);
	static bool sendSimpleMsg2ToAll(bool btoAllServer,const char *pattern, ...);
	static bool sendGmToAll(bool btoAllServer,bool boBanner,const char* name,int gmlvl,const char *pattern, ...);											//����gm��ʱ��Ϣ���ͻ���
	static bool sendGmToUser(CPlayerObj *pPlayer,const char *pattern, ...);
	static bool sendGroupMsg(char* szName,char* szToName,DWORD dwGroupId,const char* pattern,...);                        //���͸�������ɫ�������Ϣ
	static bool sendCenterMsg(const char* szName,const char *pattern, ...);
	static bool sendClanMsg(CPlayerObj* pPlayer,const char* pattern,...);                        //���͸�������ɫ�������Ϣ
	static bool sendPrincesMsg(CPlayerObj* pPlayer,const char* pattern,...);					//���ڵ�ͼ�������˵õ���Ϣ
	static bool sendchatcmd(CPlayerObj* pToPlayer,void* pbuf,int ncmdlen);
	static bool sendRumorsMsg(CPlayerObj* pPlayer,const char* pattern,...);					//����
	static bool sendNpcChat(CCreature* pNpc,BYTE btType,const char* pattern,...);			//����˵��
	static bool sendBohouChat(CPlayerObj *pToPlayer,const char *pattern, ...);			//��������
	static bool sendTradeChat(CPlayerObj *pToPlayer,const char *pattern, ...);
	static bool sendFightMapChat(CCreature *pCret,const char *pattern, ...);
	static bool sendClient(CPlayerObj* pToPlayer,const char* pattern, ...);				//�ͻ�������
	static bool sendToGmClient(CPlayerObj* pToPlayer,const char* pattern, ...);			//ֻ��GM�ſ��Կ����Ŀͻ�������
	static bool sendMapChat(CGameMap* pMap,bool boBanner,const char* pattern, ...);					//ϵͳ����ͼ����
	static bool sendNoticeToUser(CPlayerObj* pPlayer, bool boBanner,const char* pattern, ...);
	static bool sendSimpleMsg(CPlayerObj* pPlayer,const char* pattern, ...);
	static bool sendSimpleMsg2(CPlayerObj* pPlayer,const char* pattern,...);	//���½���ʾ
	static bool sendOperatorMsg(bool boBanner,const char* pattern, ...);
	static bool sendNoticeToMap(CGameMap* pMap,const char* pattern, ...);
	static bool sendNoticeToAll(bool btoAllServer,const char* pattern, ...);
	static bool sendGuildToAll(DWORD dwGuildId,const char* pattern, ...);
	static bool sendClassMsg(CPlayerObj* pPlayer,const char* pattern, ...);
	static void SETSENDCMD(stCretChat* retcmd,emChatType ChatType,const char* szMsg,const char* szPlayName,const char* szToName,BYTE CountryId);
	static void SENDTOSUPER(const char* szToName,const char* szInName,DWORD TempId,emChatType ChatType,stCretChat* RetCmd);
	static void SENDTOGLOBAL(const char* szToName,const char* szInName,DWORD TempId,emChatType ChatType,stCretChat* RetCmd);
	//////////////////////////////////////////////////////////////////////////
	typedef bool (* gm_cmdfunc)(CPlayerObj*,const char* ,CCmdlineParse& ,const char* ,int,BYTE);
	struct stGmCmdInfo{
		gm_cmdfunc pfunc;
		char szDis[1024];
		int needauthority;
		int needtrueplayer;

		stGmCmdInfo(){
			pfunc=NULL;szDis[0]=0;needauthority=0;needtrueplayer=0;
		}
		stGmCmdInfo(gm_cmdfunc p,int na=0,int np=0,const char* dis=""){
			pfunc=p;strcpy_s(szDis,sizeof(szDis)-1,dis);needauthority=na;needtrueplayer=np;
		}
	};
	typedef stdext::hash_map< std::string, stGmCmdInfo > gmcmdfuncmap;
	static gmcmdfuncmap m_gmcmdfunmaps;
	static bool initgmcmdfunc();
	static bool sendGmCmd(CPlayerObj* pSender,bool toAllSvr,const char* name,int gmlvl,BYTE inputType,const char* pattern,...);
};

BOOL GetSvrKey(CHAR*);
#endif
