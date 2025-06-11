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
	DWORD dwId;					//聊天id
	char szChatType[20];		//聊天类型
	bool boCountryLimit;		//国家限制
	DWORD dwLevel;				//等级限制
	DWORD dwZsLevel;			//转生限制
	DWORD dwGold;				//金币
	DWORD dwRumorsExposureRate;	//传言曝光率
	DWORD dwTime;				//间隔时间
	DWORD dwCount;				//间隔时间内的次数
	DWORD dwItemId;				//聊天道具ID
	DWORD dwMaxChatLength;      //聊天内容长度
	int nVipLevel;
	stChatConfig(){
		ZEROSELF;
	}
};

static dbCol ChatConfig_define[] = { 
	{_DBC_SO_("聊天id", DB_DWORD, stChatConfig, dwId)}, 
	{_DBC_SO_("聊天类型", DB_STR, stChatConfig, szChatType)}, 
	{_DBC_SO_("聊天国家限制", DB_BYTE, stChatConfig, boCountryLimit)}, 
	{_DBC_SO_("聊天需要等级", DB_DWORD, stChatConfig, dwLevel)}, 
	{_DBC_SO_("聊天需要转生", DB_DWORD, stChatConfig, dwZsLevel)},
	{_DBC_SO_("聊天需要金钱", DB_DWORD, stChatConfig, dwGold)},
	{_DBC_SO_("聊天曝光率", DB_DWORD, stChatConfig, dwRumorsExposureRate)},
	{_DBC_SO_("聊天间隔时间", DB_DWORD, stChatConfig, dwTime)},
	{_DBC_SO_("聊天发言次数", DB_DWORD, stChatConfig, dwCount)},
	{_DBC_SO_("聊天需要道具", DB_DWORD, stChatConfig, dwItemId)},
	{_DBC_SO_("聊天最大长度", DB_DWORD, stChatConfig, dwMaxChatLength)},
	{_DBC_SO_("聊天VIP等级", DB_DWORD, stChatConfig, nVipLevel)},
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
// 	//添加公告
// 	static bool add(stBoard& board)
// 	{
// 		result = 1;
// 		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("addincrease",
// 			board.beginTime, board.endTime, board.szNotice);
// 		return result == 0 ? true : false;
// 	}
// 
// 	//删除公告
// 	static bool del(int id)
// 	{
// 		result = 1;
// 		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("delincrease", id);
// 		return result == 0 ? true : false;
// 	}
// 
// 	//获取所有公告列表
// 	static void getList()
// 	{
// 		boardNum = 0;
// 		memset(boards, 0, sizeof(boards));
// 		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("readincrease");
// 	}
// 
// 	//读取公告列表
// 	static void getList(BoardList& boardList)
// 	{
// 		boardList.clear();
// 		for(int i = 0; i < boardNum; ++i)
// 		{
// 			boardList.push_back(boards[i]);
// 		}
// 	}
// 
// 	//设置公告间隔时间
// 	static bool updateInterval(DWORD itime)
// 	{
// 		result = 1;
// 		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("setnoticeinterval", itime);
// 		return result == 0 ? true : false;
// 	}
// 
// 	//获取公告间隔时间
// 	static void readInterval(DWORD& itime)
// 	{
//  		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("getnoticeinterval", &itime);
// // 		CWinApiIni ini(iniPath);
// // 		itime = ini.ReadInt("时间间隔", "分钟");
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
// 		//读取ID
// 		ptr = newPtr;
// 		newPtr=strchr(str, '#');
// 		if(NULL == newPtr)
// 		{
// 			return false;
// 		}
// 		*newPtr = 0;
// 		board.id = atoi(ptr);
// 
// 		//读取开始时间
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
// 		//读取结束时间
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
// 		//读取公告内容
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
	static bool sendPrivate(CPlayerObj *pPlayer,const char *psztoName,const char *pattern, ...);	//发送聊天消息给单独一个角色
	static bool sendQQPrivate(CPlayerObj *pPlayer,const char *psztoName,const char *pattern, ...);	//发送QQ聊天消息给单独一个角色
	//static bool sendChatMsg2User(CPlayerObj *pPlayer,emChatType etype,const char *psztoName,const char *pattern, ...);

	static bool sendSpeaker(CPlayerObj *pPlayer,const char *pattern, ...);	//小喇叭发言
	static bool SendWorld(CPlayerObj *pPlayer,const char *pattern, ...);												//发送世界消息
	static bool SendWorldTeam(CPlayerObj *pPlayer,const char *pattern, ...);						//世界队伍
	static bool sendRefMsg (CPlayerObj *pPlayer,const char *pattern, ...);							//发给同屏幕玩家
	static bool sendSystem(CPlayerObj *pToPlayer,const char *pattern, ...);		                // 系统消息
	static bool sendSystem(const char* szName,const char *pattern, ...);								//服务器发送消息给单个玩家 支持向所有服务器发送
	static bool sendSystemToAll(bool btoAllServer,bool boBanner,const char *pattern, ...);											//发送信息给所有服务器的所有玩家
	static bool sendSystemByType(const char* szName, emChatType ChatType, const char *pattern, ...);
	static bool sendSystemByType(CPlayerObj *pToPlayer, emChatType ChatType, const char *pattern, ...);
	static bool sendSimpleMsg2ToAll(bool btoAllServer,const char *pattern, ...);
	static bool sendGmToAll(bool btoAllServer,bool boBanner,const char* name,int gmlvl,const char *pattern, ...);											//发送gm及时消息给客户端
	static bool sendGmToUser(CPlayerObj *pPlayer,const char *pattern, ...);
	static bool sendGroupMsg(char* szName,char* szToName,DWORD dwGroupId,const char* pattern,...);                        //发送给单独角色的组队信息
	static bool sendCenterMsg(const char* szName,const char *pattern, ...);
	static bool sendClanMsg(CPlayerObj* pPlayer,const char* pattern,...);                        //发送给单独角色的组队信息
	static bool sendPrincesMsg(CPlayerObj* pPlayer,const char* pattern,...);					//所在地图的所有人得到信息
	static bool sendchatcmd(CPlayerObj* pToPlayer,void* pbuf,int ncmdlen);
	static bool sendRumorsMsg(CPlayerObj* pPlayer,const char* pattern,...);					//传言
	static bool sendNpcChat(CCreature* pNpc,BYTE btType,const char* pattern,...);			//怪物说话
	static bool sendBohouChat(CPlayerObj *pToPlayer,const char *pattern, ...);			//本国聊天
	static bool sendTradeChat(CPlayerObj *pToPlayer,const char *pattern, ...);
	static bool sendFightMapChat(CCreature *pCret,const char *pattern, ...);
	static bool sendClient(CPlayerObj* pToPlayer,const char* pattern, ...);				//客户端提醒
	static bool sendToGmClient(CPlayerObj* pToPlayer,const char* pattern, ...);			//只有GM才可以看见的客户端提醒
	static bool sendMapChat(CGameMap* pMap,bool boBanner,const char* pattern, ...);					//系统本地图聊天
	static bool sendNoticeToUser(CPlayerObj* pPlayer, bool boBanner,const char* pattern, ...);
	static bool sendSimpleMsg(CPlayerObj* pPlayer,const char* pattern, ...);
	static bool sendSimpleMsg2(CPlayerObj* pPlayer,const char* pattern,...);	//左下角显示
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
