#include "Chat.h"
#include <fstream>
#include "server_cmd.h"
#include "NpcTrade.h"
#include "UsrEngn.h"
#include "../gamesvr.h"
#include "MagicRange.h"
#include <WinInet.h>
#include "JsonConfig.h"
#pragma comment(lib, "Wininet.lib") 
/*
表1 汉字编码范围

名称     |       第一字节             |           第二字节
--------|-------------------------|------------------------
GB2312  |   0xB0-0xF7(176-247)    |    0xA0-0xFE（160-254）
--------|-------------------------|-------------------------
GBK0    |  x81-0xFE（129-254）     |   0x40-0xFE（64-254）
--------|-------------------------|-------------------------
Big5    |   0x81-0xFE（129-255）   |   0x40-0x7E（64-126）
|                         |    0xA1－0xFE（161-254）
--------|-------------------------|------------------------
//*/

const UINT GBK_MIN_CHAR1 = 0x81;
const UINT GBK_MAX_CHAR1 = 0xfe;       // 决定列  第一字节

const UINT GBK_MIN_CHAR2 = 0x40;
const UINT GBK_MAX_CHAR2 = 0xfe;       // 决定行  第二字节

const UINT GB2312_MIN_CHAR1 = 0xa1;
const UINT GB2312_MAX_CHAR1 = 0xf7;       // 决定列  第一字节

const UINT GB2312_MIN_CHAR2 = 0xa0;
const UINT GB2312_MAX_CHAR2 = 0xfe;       // 决定行  第二字节

const UINT EN_MIN_CHAR = 33;
const UINT EN_MAX_CHAR = 0x7f;

DWORD CChat::m_dwSendGmManage=0;

inline bool isGB2312Code( WORD ch )
{
	int c1 = (ch & 0xff);
	int c2 = ((ch & 0xff00) >> 8);
	return GB2312_MIN_CHAR1 <= c1 && c1 <= GB2312_MAX_CHAR1 &&
		GB2312_MIN_CHAR2 <= c2 && c2 <= GB2312_MAX_CHAR2;
}

void CChat::RandomStr(char *pszStr)
{
	FUNCTION_BEGIN;
	char szChange[10] = {'~','!','@','#','$','%','^','&','*','-'};
	for (DWORD i=0;i<strlen(pszStr);i++){
		pszStr[i] = szChange[_random(9)];
	}
}

bool CChat::sendPrivate(CPlayerObj *pPlayer,const char *psztoName,const char *pattern, ...)
{
	FUNCTION_BEGIN;
	if(!pPlayer || !psztoName) return false;
	if(stricmp(pPlayer->getName(),psztoName) == 0) return false;	//自己不能和自己说话
	
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_PRIVATE,szChatMsg,pPlayer->getName(),psztoName,0);
	
	CPlayerObj *pToPlayer = CUserEngine::getMe().m_playerhash.FindByName(psztoName);
	if(!pToPlayer){
		if(pPlayer->m_dwSrcZoneId && pPlayer->m_dwSrcZoneId!=GameService::getMe().m_nZoneid && CUserEngine::getMe().isCrossSvr()){
			retcmd->dwZoneId=MAKELONG(GameService::getMe().m_nTradeid,GameService::getMe().m_nZoneid);
			retcmd->dwVip = pPlayer->getVipType();
			retcmd->i64SrcOnlyId=MAKELONGLONG(GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value,retcmd->dwZoneId);
			retcmd->i64DestOnlyId=0;
			retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
			retcmd->btPlatForm=pPlayer->getPlatFormType();
			retcmd->dwSendTime = pPlayer->quest_vars_get_var_n("ChatBubbleId");
			CUserEngine::getMe().BroadcastGameSvr(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize(),GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value,true,pPlayer->m_dwSrcZoneId,pPlayer->m_wSrcTrade);
		}else{
			SENDTOSUPER(psztoName,pPlayer->getName(),0,CHAT_TYPE_PRIVATE,retcmd);
		}
	}
	else{
		if(!(pToPlayer->m_dwUserConfig & USERCONFIG_CANPRIVITECHAT)){
			CChat::sendSystem(pPlayer->getName(), GameService::getMe().GetStrRes(RES_LANG_USERCONFIGNOTPRIVITE),pToPlayer->getName());
			return false;
		}
		retcmd->dwVip = pToPlayer->getVipType();
		retcmd->dwGuildId = pToPlayer->m_GuildInfo.dwGuildId;
		retcmd->btPlatForm=pPlayer->getPlatFormType();
		retcmd->dwSendTime = pPlayer->quest_vars_get_var_n("ChatBubbleId");
		pPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());

		retcmd->dwVip = pPlayer->getVipType();
		retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
		pToPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());

		std::string msgstr = std::string(pattern);
		replace_all(msgstr, "'", "''");
		GameService::getMe().Send2LogSvr(_SERVERLOG_GAMECHAT_,0,0,pPlayer, "%d, %d, \'%s\', \'%s\', %I64d, %d, \'%s\', \'%s\', \'%s\', %I64d, %d, \'%s\', \'%s\'", 
			CHAT_TYPE_PRIVATE,
			pPlayer->m_btGmLvl,
			pPlayer->getAccount(),
			pPlayer->getName(),
			pPlayer->m_i64UserOnlyID,
			pPlayer->m_dwLevel,
			inet_ntoa(pPlayer->clientip),
			pToPlayer->getAccount(),
			pToPlayer->getName(),
			pToPlayer->m_i64UserOnlyID,
			pToPlayer->m_dwLevel,
			inet_ntoa(pPlayer->clientip),
			msgstr.c_str());
	}
	return true;
}

bool CChat::sendQQPrivate(CPlayerObj *pPlayer,const char *psztoName,const char *pattern, ...)
{
	FUNCTION_BEGIN;
	if(!pPlayer || !psztoName) return false;
	if(stricmp(pPlayer->getName(),psztoName) == 0) return false;	//自己不能和自己说话

	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_QQ,szChatMsg,pPlayer->getName(),psztoName,0);
	retcmd->dwVip = pPlayer->getVipType();
	retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
	retcmd->btPlatForm=pPlayer->getPlatFormType();
	retcmd->dwSendTime = pPlayer->quest_vars_get_var_n("ChatBubbleId");
	CPlayerObj *pToPlayer = CUserEngine::getMe().m_playerhash.FindByName(psztoName);
	if(!pToPlayer){
		SENDTOSUPER(psztoName,pPlayer->getName(),0,CHAT_TYPE_QQ,retcmd);
	}
	else{
		pPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
		pToPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	}
	return true;
}

bool CChat::sendSpeaker(CPlayerObj *pPlayer,const char *pattern, ...)
{
	FUNCTION_BEGIN;
	if (!pPlayer) return false;
	GETPATTERN(szChatMsg, pattern);
	BUFFER_CMD(stCretChat, retcmd, stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd, CHAT_TYPE_SPEAKER, szChatMsg,pPlayer->getName(), NULL, 0);
	retcmd->dwVip = pPlayer->getVipType();
	retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
	retcmd->dwSendTime = pPlayer->quest_vars_get_var_n("ChatBubbleId");
	CUserEngine::getMe().SendMsg2AllUser(retcmd, sizeof(*retcmd) + retcmd->szZeroChatMsg.getarraysize());
	SENDTOSUPER(NULL, pPlayer->getName(), 0, CHAT_TYPE_SPEAKER, retcmd);
	
	return true;
}

bool CChat::sendRefMsg(CPlayerObj *pPlayer,const char *pattern, ...)
{
	FUNCTION_BEGIN;
	if(!pPlayer ) return false;
 	GETPATTERN(szChatMsg,pattern);
 
 	char szShowName[512];
 	pPlayer->getShowName(szShowName,sizeof(szShowName)-1);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_REFMSG,szChatMsg,szShowName,NULL,0);
	retcmd->dwVip = pPlayer->getVipType();
	retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
	retcmd->btPlatForm=pPlayer->getPlatFormType();
	retcmd->dwSendTime = pPlayer->quest_vars_get_var_n("ChatBubbleId");
	retcmd->dwTemId = pPlayer->GetObjectId();
 	pPlayer->SendRefMsg(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize(),false);
	return true;
}

bool CChat::sendSystemToAll(bool btoAllServer,bool boBanner,const char *pattern, ...)	
{
	FUNCTION_BEGIN;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_SYSTEMNOTICE,szChatMsg,NULL,NULL,0);
	retcmd->boBanner=boBanner;

	if(btoAllServer){
		SENDTOSUPER(NULL,NULL,0,CHAT_TYPE_SYSTEMNOTICE,retcmd);
	}
	else{
		CUserEngine::getMe().SendMsg2AllUser(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	}

	return true;
}


bool CChat::sendSimpleMsg2ToAll(bool btoAllServer,const char *pattern, ...)	
{
	FUNCTION_BEGIN;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_SIMPLE2,szChatMsg,NULL,NULL,0);

	if(btoAllServer){
		SENDTOSUPER(NULL,NULL,0,CHAT_TYPE_SYSTEMNOTICE,retcmd);
	}
	else{
		CUserEngine::getMe().SendMsg2AllUser(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	}

	return true;
}

bool CChat::sendGmToUser(CPlayerObj *pPlayer,const char *pattern, ...)
{	
	FUNCTION_BEGIN;
	if(pPlayer){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_GM,szChatMsg,NULL,NULL,0);
		pPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	}	
	return true;
}


bool CChat::sendGmToAll(bool btoAllServer,bool boBanner,const char* name,int gmlvl,const char *pattern, ...)
{	
	FUNCTION_BEGIN;
	if (gmlvl>=1){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_GM,szChatMsg,NULL,NULL,0);
		retcmd->boBanner=boBanner;

		g_logger.debug("%s %d级GM喊话: %s",name,gmlvl,szChatMsg);
		if(btoAllServer){
			SENDTOSUPER(NULL,NULL,0,CHAT_TYPE_GM,retcmd);
		}
		else{
			CUserEngine::getMe().SendMsg2AllUser(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
		}
	}
	return true;
}

bool CChat::sendSystem(const char* szName,const char *pattern, ...)
{
	FUNCTION_BEGIN;
	CPlayerObj *pToPlayer = CUserEngine::getMe().m_playerhash.FindByName(szName);
	if (pToPlayer){
		GETPATTERN(szChatMsg,pattern);
		return sendSystem(pToPlayer,szChatMsg);
	}else if(szName && szName[0]!=0){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_SYSTEM,szChatMsg,NULL,NULL,0);
		SENDTOSUPER(NULL,szName,0,CHAT_TYPE_SYSTEM,retcmd);
	}
	return true;
	
}

bool CChat::sendSystemByType( const char* szName, emChatType ChatType, const char *pattern, ... )
{
	FUNCTION_BEGIN;
	CPlayerObj *pToPlayer = CUserEngine::getMe().m_playerhash.FindByName(szName);
	if (pToPlayer){
		GETPATTERN(szChatMsg,pattern);
		return sendSystemByType(pToPlayer, ChatType, szChatMsg);
	}else if(szName && szName[0]!=0){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,ChatType,szChatMsg,NULL,NULL,0);
		SENDTOSUPER(NULL,szName,0,ChatType,retcmd);
	}
	return true;
}

bool CChat::sendSystemByType( CPlayerObj *pToPlayer, emChatType ChatType, const char *pattern, ... )
{
	FUNCTION_BEGIN;
	if(!pToPlayer) return false;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,ChatType,szChatMsg,NULL,NULL,0);
	pToPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	return true;
}

bool CChat::sendSystem(CPlayerObj *pToPlayer,const char *pattern, ...)
{
	FUNCTION_BEGIN;

	if(!pToPlayer) return false;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_SYSTEM,szChatMsg,NULL,NULL,0);
	pToPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	return true;

}

bool CChat::sendGroupMsg(char* szName,char* szToName,DWORD dwGroupId,const char* pattern,...)
{
	FUNCTION_BEGIN;
	CPlayerObj *pToPlayer = CUserEngine::getMe().m_playerhash.FindByName(szToName);
	if (pToPlayer)
	{
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_GROUP,szChatMsg,szName,NULL,0);
		CPlayerObj *pPlayer = CUserEngine::getMe().m_playerhash.FindByName(szName);
		if (pPlayer){
			retcmd->dwVip = pPlayer->getVipType();
			retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
			retcmd->btPlatForm=pPlayer->getPlatFormType();
			retcmd->dwSendTime = pPlayer->quest_vars_get_var_n("ChatBubbleId");
		}
		pToPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	}
	else
	{
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_GROUP,szChatMsg,szName,NULL,0);
		CPlayerObj *pPlayer = CUserEngine::getMe().m_playerhash.FindByName(szName);
		if (pPlayer){
			retcmd->dwVip = pPlayer->getVipType();
			retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
			retcmd->btPlatForm=pPlayer->getPlatFormType();
			retcmd->dwSendTime = pPlayer->quest_vars_get_var_n("ChatBubbleId");
		}
		SENDTOGLOBAL(NULL,szName,dwGroupId,CHAT_TYPE_GROUP,retcmd);
	}
	return true;
}
bool CChat::sendFightMapChat(CCreature *pCret,const char *pattern, ...)
{
	FUNCTION_BEGIN;

	return true;
}

bool CChat::sendClient(CPlayerObj* pToPlayer,const char* pattern, ...){
	FUNCTION_BEGIN;
	if (pToPlayer){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_CLIENT,szChatMsg,NULL,pToPlayer->getName(),0);
		pToPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
		return true;
	}
	return false;
}

bool CChat::sendToGmClient(CPlayerObj* pToPlayer,const char* pattern, ...){
	FUNCTION_BEGIN;
	if (pToPlayer && pToPlayer->m_btGmLvl){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_CLIENT,szChatMsg,NULL,pToPlayer->getName(),0);
		pToPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
		return true;
	}
	return false;
}

bool CChat::sendMapChat(CGameMap* pMap,bool boBanner,const char* pattern, ...){
	FUNCTION_BEGIN;
	if (pMap){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_PRINCES,szChatMsg,NULL,NULL,0);
		retcmd->boBanner=boBanner;

		pMap->SendMsgToMapAllUser(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
		return true;
	}
	return false;
}

bool CChat::sendNoticeToUser(CPlayerObj* pPlayer, bool boBanner,const char* pattern, ...){
	FUNCTION_BEGIN;
	if (pPlayer){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_NOTICE,szChatMsg,NULL,pPlayer->getName(),0);
		retcmd->boBanner = boBanner;
		pPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
		return true;
	}
	return false;
}

bool CChat::sendSimpleMsg(CPlayerObj* pPlayer,const char* pattern, ...){
	FUNCTION_BEGIN;
	if (pPlayer){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_SIMPLE,szChatMsg,NULL,pPlayer->getName(),0);
		pPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
		return true;
	}
	return false;
}
bool CChat::sendSimpleMsg2(CPlayerObj* pPlayer,const char* pattern, ...){
	FUNCTION_BEGIN;
	if (pPlayer){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_SIMPLE2,szChatMsg,NULL,pPlayer->getName(),0);
		pPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
		return true;
	}
	return false;
}

bool CChat::sendOperatorMsg(bool boBanner,const char* pattern, ...){
	FUNCTION_BEGIN;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_OPERATOR,szChatMsg,NULL,NULL,0);
	retcmd->boBanner=boBanner;
	SENDTOSUPER(NULL,NULL,0,CHAT_TYPE_OPERATOR,retcmd);
	return true;
}

bool CChat::sendNoticeToMap(CGameMap* pMap,const char* pattern, ...){
	FUNCTION_BEGIN;
	if (pMap){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_NOTICE,szChatMsg,NULL,NULL,0);
		pMap->SendMsgToMapAllUser(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
		return true;
	}
	return false;
}

bool CChat::sendNoticeToAll(bool btoAllServer,const char* pattern, ...){
	FUNCTION_BEGIN;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_NOTICE,szChatMsg,NULL,NULL,0);

	if(btoAllServer){
		SENDTOSUPER(NULL,NULL,0,CHAT_TYPE_NOTICE,retcmd);
	}
	else{
		CUserEngine::getMe().SendMsg2AllUser(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	}
	return true;
}

bool CChat::sendCenterMsg(const char* szName,const char *pattern, ...)
{
	FUNCTION_BEGIN;
	CPlayerObj *pToPlayer = CUserEngine::getMe().m_playerhash.FindByName(szName);
	if (pToPlayer)
	{
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_CENTER,szChatMsg,NULL,szName,0);
		pToPlayer->SendMsgToMe(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	}
	else
	{
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_CENTER,szChatMsg,NULL,NULL,0);
		SENDTOSUPER(szName,NULL,0,CHAT_TYPE_CENTER,retcmd);
	}
	return true;
}

bool CChat::sendClanMsg(CPlayerObj* pPlayer,const char* pattern,...)                        //发送给单独角色的氏族信息
{
	FUNCTION_BEGIN;
	if(!pPlayer || pPlayer->m_GuildInfo.dwGuildId == 0) return false;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_CLAN,szChatMsg,pPlayer->getName(),NULL,0 );
	retcmd->dwVip = pPlayer->getVipType();
	retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
	retcmd->btPlatForm=pPlayer->getPlatFormType();
	retcmd->dwSendTime = pPlayer->quest_vars_get_var_n("ChatBubbleId");
	if(pPlayer->m_dwSrcZoneId && pPlayer->m_dwSrcZoneId!=GameService::getMe().m_nZoneid && CUserEngine::getMe().isCrossSvr()){
		retcmd->dwZoneId=MAKELONG(GameService::getMe().m_nTradeid,GameService::getMe().m_nZoneid);
		CUserEngine::getMe().BroadcastGameSvr(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize(),GameService::getMe().m_Svr2SvrLoginCmd.svr_id_type_value,true,pPlayer->m_dwSrcZoneId,pPlayer->m_wSrcTrade);
	}else{
		SENDTOGLOBAL(vformat("%d",pPlayer->m_GuildInfo.dwGuildId),pPlayer->getName(),0,CHAT_TYPE_CLAN,retcmd );
	}

	return true;

}

bool CChat::sendGuildToAll(DWORD dwGuildId,const char* pattern, ...)	//公会公告
{
	FUNCTION_BEGIN;
	if (!dwGuildId) return false;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_CLAN,szChatMsg,NULL,NULL,0);
	retcmd->dwGuildId = dwGuildId;
	SENDTOGLOBAL(vformat("%d",dwGuildId),NULL,0,CHAT_TYPE_CLAN,retcmd );
	return true;
}

bool CChat::sendClassMsg(CPlayerObj* pPlayer,const char* pattern, ...)
{
	FUNCTION_BEGIN;
	if(pPlayer){
		GETPATTERN(szChatMsg,pattern);
		BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
		SETSENDCMD(retcmd,CHAT_TYPE_CLASS,szChatMsg,pPlayer->getName(),NULL,0);
		SENDTOGLOBAL(NULL,pPlayer->getName(),0,CHAT_TYPE_CLASS,retcmd);
		return true;
	}
	return false;
}

void CChat::SETSENDCMD(stCretChat* retcmd,emChatType ChatType,const char* szMsg,const char* szPlayName,const char* szToName,BYTE CountryId){
	if(retcmd){
		retcmd->btChatType = ChatType;
		if (szMsg) retcmd->szZeroChatMsg.push_str(szMsg);
		if(szPlayName) s_strncpy_s(retcmd->szName,szPlayName,_MAX_NAME_LEN_-1);
		if(szToName) s_strncpy_s(retcmd->szTargetName,szToName,_MAX_NAME_LEN_-1);
		retcmd->btCountryInfoId = CountryId;
	}
}

void CChat::SENDTOSUPER(const char* szToName,const char* szInName,DWORD TempId,emChatType ChatType,stCretChat* RetCmd){
	if(RetCmd){
		BUFFER_CMD(stSendChatMsgSuperSrv,sendCmd,stBasePacket::MAX_PACKET_SIZE);
		if(szToName) s_strncpy_s(sendCmd->szName,szToName,_MAX_NAME_LEN_-1);
		if(szInName) s_strncpy_s(sendCmd->szMyName,szInName,_MAX_NAME_LEN_-1);
		sendCmd->btChatType = ChatType;
		sendCmd->msg.push_back((char*)RetCmd,sizeof(*RetCmd)+RetCmd->szZeroChatMsg.getarraysize(),__FUNC_LINE__);
		CUserEngine::getMe().SendMsg2SuperSvr(sendCmd,sizeof(*sendCmd)+sendCmd->msg.getarraysize());
	}
}

void CChat::SENDTOGLOBAL(const char* szToName,const char* szInName,DWORD TempId,emChatType ChatType,stCretChat* RetCmd){
	if(RetCmd){
		BUFFER_CMD(stSendChatMsgGlobalSrv,sendCmd,stBasePacket::MAX_PACKET_SIZE);
		if(szToName) s_strncpy_s(sendCmd->szName,szToName,_MAX_NAME_LEN_-1);
		if(szInName) s_strncpy_s(sendCmd->szMyName,szInName,_MAX_NAME_LEN_-1);
		sendCmd->btChatType = ChatType;
		sendCmd->msg.push_back((char*)RetCmd,sizeof(*RetCmd)+RetCmd->szZeroChatMsg.getarraysize(),__FUNC_LINE__);
		CUserEngine::getMe().SendMsg2GlobalSvr(sendCmd,sizeof(*sendCmd)+sendCmd->msg.getarraysize());
	}
}

bool CChat::sendPrincesMsg(CPlayerObj* pPlayer,const char* pattern,...)//所在地图的所有人得到信息
{
	FUNCTION_BEGIN;
	if(!pPlayer/* || pPlayer->m_Member.dwPrincesId == 0*/) return false;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_PRINCES,szChatMsg,pPlayer->getName(),NULL,0);
	retcmd->dwVip = pPlayer->getVipType();
	retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
	retcmd->btPlatForm=pPlayer->getPlatFormType();
	retcmd->dwSendTime = pPlayer->quest_vars_get_var_n("ChatBubbleId");
	//SENDTOSUPER(pPlayer->m_Member.szPrincesName,pPlayer->getName(),0,CHAT_TYPE_PRINCES,retcmd);
	if (pPlayer->GetEnvir()){
		pPlayer->GetEnvir()->SendMsgToMapAllUser(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	}
	return true;
}

bool CChat::sendRumorsMsg(CPlayerObj* pPlayer,const char* pattern,...)
{
	FUNCTION_BEGIN;
	return true;
}

bool CChat::SendWorld(CPlayerObj *pPlayer,const char *pattern, ...)
{
	FUNCTION_BEGIN;
	if (!pPlayer) return false;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd, CHAT_TYPE_WORLD,szChatMsg,pPlayer->getName(),NULL,0);
	retcmd->dwVip = pPlayer->getVipType();
	if (CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("CheckNameGm", false, pPlayer->getName())) {
		retcmd->dwVip = 0x00000800;
	}
	retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
	retcmd->btPlatForm=pPlayer->getPlatFormType();
	retcmd->dwSendTime =pPlayer->quest_vars_get_var_n("ChatBubbleId");
	CUserEngine::getMe().SendMsg2AllUser(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	SENDTOSUPER(NULL,pPlayer->getName(),0,CHAT_TYPE_WORLD,retcmd);
	return true;
}

bool CChat::SendWorldTeam(CPlayerObj *pPlayer,const char *pattern, ...)
{
	FUNCTION_BEGIN;
	if (!pPlayer) return false;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_WORLD_TEAM,szChatMsg,pPlayer->getName(),NULL,0);
	retcmd->dwVip = pPlayer->getVipType();
	retcmd->dwGuildId = pPlayer->m_GuildInfo.dwGuildId;
	retcmd->dwSendTime = pPlayer->quest_vars_get_var_n("ChatBubbleId");
	CUserEngine::getMe().SendMsg2AllUser(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	SENDTOSUPER(NULL,pPlayer->getName(),0,CHAT_TYPE_WORLD,retcmd);
	return true;
}

bool  CChat::sendBohouChat(CPlayerObj *pToPlayer,const char *pattern, ...)
{
	FUNCTION_BEGIN;
// 	GETPATTERN(szChatMsg,pattern);
// 	SETSENDCMD(retcmd,CHAT_TYPE_BOHOU,szChatMsg,pToPlayer->getName(),NULL,pToPlayer->m_nFactionId);
// 	CUserEngine::getMe().SendMsg2AllBoHouUser(&retcmd,sizeof(retcmd),pToPlayer->m_nFactionId);
// 	SENDTOSUPER(NULL,pToPlayer->getName(),0,CHAT_TYPE_BOHOU,retcmd);
	return true;
}

bool CChat::sendTradeChat(CPlayerObj *pToPlayer,const char *pattern, ...)
{
	FUNCTION_BEGIN;
	if (!pToPlayer) return false;
	GETPATTERN(szChatMsg,pattern);
	BUFFER_CMD(stCretChat,retcmd,stBasePacket::MAX_PACKET_SIZE);
	SETSENDCMD(retcmd,CHAT_TYPE_DEAL,szChatMsg,pToPlayer->getName(),NULL,0);
	retcmd->dwVip = pToPlayer->getVipType();
	retcmd->dwGuildId = pToPlayer->m_GuildInfo.dwGuildId;
	retcmd->btPlatForm=pToPlayer->getPlatFormType();
	retcmd->dwSendTime = pToPlayer->quest_vars_get_var_n("ChatBubbleId");
	CUserEngine::getMe().SendMsg2AllUser(retcmd,sizeof(*retcmd)+retcmd->szZeroChatMsg.getarraysize());
	SENDTOSUPER(NULL,pToPlayer->getName(),0,CHAT_TYPE_DEAL,retcmd);
	return true;
}
bool CChat::sendchatcmd(CPlayerObj* pToPlayer,void* pbuf,int ncmdlen)
{
	FUNCTION_BEGIN;
	pToPlayer->SendMsgToMe(pbuf,ncmdlen);
	return true;
}

bool CChat::sendNpcChat(CCreature* pNpc,BYTE btType,const char* pattern,...)
{
	FUNCTION_BEGIN;
	if(!pNpc ) return false;
	GETPATTERN(szChatMsg,pattern);
	stCretNpcChat retcmd;
	retcmd.btChatType = btType;
	retcmd.dwTmpId = pNpc->GetObjectId();
	s_strncpy_s(retcmd.szChatMsg,szChatMsg,_MAX_CHAT_LEN_-1);
	pNpc->SendRefMsg(&retcmd,sizeof(retcmd));
	return true;
}

//////////////////////////////////////////////////////////////////////////
#define _GMCMDSENDERQZ_				"exer"

bool gm_cmdfunc_help(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	CChat::sendSystem( pSender,"命令帮助语法规则: [!]@cmd  {x [m]}|{y [n]}\n  //! : 标记为在全区所有服务器执行;\n  //@cmd :命令;\n  //{} :参数分组;\n  // | :或者;\n  //[] :括号内表示可选参数 ");
	const char* szCmd=NULL;
	szCmd =parse["cmd"].c_str();
	if (szCmd==NULL || szCmd[0]==0){ szCmd=parse["1"].c_str(); }
	if (szCmd && szCmd[0]!=0){
		CChat::gmcmdfuncmap::iterator it=CChat::m_gmcmdfunmaps.find(szCmd);
		if (it!=CChat::m_gmcmdfunmaps.end()){
			CChat::stGmCmdInfo* pcmdinfo=&it->second;
			if (pcmdinfo && pcmdinfo->pfunc){
				if (gmlvl>=pcmdinfo->needauthority && it->first.length()>0 && it->first.c_str()[0]!='_'){
					CChat::sendSystem(pSender,"@%s %s   //需要权限:%d",it->first.c_str(),pcmdinfo->szDis,pcmdinfo->needauthority);
				}
				return true;
			}
		}else{
			bool bofindcmd=false;
			for (CChat::gmcmdfuncmap::iterator it=CChat::m_gmcmdfunmaps.begin();it!=CChat::m_gmcmdfunmaps.end();it++){
				CChat::stGmCmdInfo* pcmdinfo=&it->second;
				if ( pcmdinfo && pcmdinfo->pfunc && strnicmp( it->first.c_str(),szCmd,strlen(szCmd) )==0 ){
					if (gmlvl>=pcmdinfo->needauthority && it->first.length()>0 && it->first.c_str()[0]!='_'){
						CChat::sendSystem(pSender,"@%s %s   //需要权限:%d",it->first.c_str(),pcmdinfo->szDis,pcmdinfo->needauthority);
						bofindcmd=true;
					}
				}
			}
			if (bofindcmd){ return true; }
		}
		CChat::sendSystem(pSender,"Commond %s not found!",szCmd);
	}else{
		for (CChat::gmcmdfuncmap::iterator it=CChat::m_gmcmdfunmaps.begin();it!=CChat::m_gmcmdfunmaps.end();it++){
			CChat::stGmCmdInfo* pcmdinfo=&it->second;
			if (pcmdinfo && pcmdinfo->pfunc){
				if (gmlvl>=pcmdinfo->needauthority && it->first.length()>0 && it->first.c_str()[0]!='_'){
					CChat::sendSystem(pSender,"@%s %s   //需要权限:%d",it->first.c_str(),pcmdinfo->szDis,pcmdinfo->needauthority);
				}
			}
		}
		return true;
	}
	return true;
}

//gm_cmdfunc_sdone

bool gm_cmdfunc_sdone(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	//stSvrSendGMCmdRet retcmd;
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	time_t shutdowntime=strtotime(parse["time"].c_str());
	time_t aftertime=atoi(parse["after"].c_str());
	if (!CUserEngine::getMe().m_boIsShutDown){
		if (shutdowntime==0 && aftertime==0){
			CUserEngine::getMe().m_shutdowntime=0;
			CUserEngine::getMe().m_lasthintshutdowntime=0;
			CUserEngine::getMe().m_shutdowndis= "";
			CUserEngine::getMe().m_boIsShutDown=false;
			CChat::sendSystem(pSender,"服务器停止维护计时!");
			retcmd->nErrorcode=0;
			retcmd->btRetType=NORMALSTR;
		}else{
			if (shutdowntime>time(NULL)){	
			}else if(aftertime>0){
				shutdowntime=time(NULL)+aftertime;
			}
			if (shutdowntime>time(NULL)){
				CUserEngine::getMe().m_shutdowntime=shutdowntime;
				CUserEngine::getMe().m_lasthintshutdowntime=0;
				CUserEngine::getMe().m_shutdowndis = parse["dis"].c_str();				
				CUserEngine::getMe().m_boIsShutDown=false;
				CChat::sendSystem(pSender,"设置服务器维护倒计时成功,服务器将于 %s 进行维护!",timetostr(shutdowntime));
				g_logger.forceLog(zLogger::zINFO,"%s 设置服务器维护倒计时成功,服务器将于 %s 进行维护!",pSender->getName(),timetostr(shutdowntime));
				retcmd->nErrorcode=0;
				retcmd->btRetType=NORMALSTR;
			}else{
				CUserEngine::getMe().m_shutdowntime=0;
				CUserEngine::getMe().m_lasthintshutdowntime=0;
				CUserEngine::getMe().m_shutdowndis = "";
				CUserEngine::getMe().m_boIsShutDown=false;
				CChat::sendSystem(pSender,"服务器停止维护计时!");
				retcmd->nErrorcode=0;
				retcmd->btRetType=NORMALSTR;
			}
		}
	}
	//CUserEngine::getMe().SendMsg2GmManageSvr(&retcmd,sizeof(retcmd));
	//CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	return true;
}


bool gm_cmdfunc_shutdown(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	//stSvrSendGMCmdRet retcmd;
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	time_t shutdowntime=strtotime(parse["time"].c_str());
	time_t aftertime=atoi(parse["after"].c_str());
	if (!CUserEngine::getMe().m_boIsShutDown){
		DWORD dwCrossLeftKickTime=0;
		if (shutdowntime==0 && aftertime==0){
			CUserEngine::getMe().m_shutdowntime=0;
			CUserEngine::getMe().m_lasthintshutdowntime=0;
			CUserEngine::getMe().m_shutdowndis ="";
			CUserEngine::getMe().m_boIsShutDown=false;
			g_logger.forceLog(zLogger::zINFO,"服务器停止维护计时!");
			CChat::sendSystem(pSender,"服务器停止维护计时!");
			retcmd->nErrorcode=0;
			retcmd->btRetType=NORMALSTR;
		}else{
			if (shutdowntime>time(NULL)){	
			}else if(aftertime>0){
				shutdowntime=time(NULL)+aftertime;
			}
			if (shutdowntime>time(NULL)){
				CUserEngine::getMe().m_shutdowntime=shutdowntime;
				CUserEngine::getMe().m_lasthintshutdowntime=0;
				CUserEngine::getMe().m_shutdowndis = parse["dis"].c_str();
				CUserEngine::getMe().m_boIsShutDown=false;
				CChat::sendSystem(pSender,"设置服务器维护倒计时成功,服务器将于 %s 进行维护!",timetostr(shutdowntime));
				g_logger.forceLog(zLogger::zINFO,"%s 设置服务器维护倒计时成功,服务器将于 %s 进行维护!",pSender->getName(),timetostr(shutdowntime));
				retcmd->nErrorcode=0;
				retcmd->btRetType=NORMALSTR;
				dwCrossLeftKickTime=(shutdowntime-time(NULL))/2;
			}else{
				CUserEngine::getMe().m_shutdowntime=0;
				CUserEngine::getMe().m_lasthintshutdowntime=0;
				CUserEngine::getMe().m_shutdowndis="";
				CUserEngine::getMe().m_boIsShutDown=false;
				CChat::sendSystem(pSender,"服务器停止维护计时!");
				g_logger.forceLog(zLogger::zINFO,"服务器停止维护计时!");
				retcmd->nErrorcode=0;
				retcmd->btRetType=NORMALSTR;
			}
		}

		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("KickCrossPlayer", dwCrossLeftKickTime);
	}
	//CUserEngine::getMe().SendMsg2GmManageSvr(&retcmd,sizeof(retcmd));
	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	return true;
}

//@wd state = 1无敌0正常
bool gm_cmdfunc_wudi(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	pSender->m_btWudi=(pSender->m_btWudi==0?1:0);
	if(pSender->m_btWudi!=0){
		CChat::sendSystem(pSender,"Invincible Open!");
		g_logger.debug("[%s]开启GM命令无敌",pSender->getName());
	}else{	
		CChat::sendSystem(pSender,"Invincible Closed!");
		g_logger.debug("[%s]关闭GM命令无敌",pSender->getName());
	}
	return true;		
}

//@ms state = 1必杀0正常
bool gm_cmdfunc_miaosha(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	BYTE btrate = atoi(parse["rate"].c_str());
	btrate = min(100,btrate);
	btrate = max(0,btrate);

	pSender->m_btMiaosha = btrate ;
	if(pSender->m_btMiaosha!=0){
		CChat::sendSystem(pSender,"Onekilled Open!");
		g_logger.debug("[%s]开启GM命令一击必杀",pSender->getName());
	}else{
		CChat::sendSystem(pSender,"Onekilled Closed!");
		g_logger.debug("[%s]关闭GM命令一击必杀",pSender->getName());
	}

	return true;		
}

bool gm_cmdfunc_dolua(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	retcmd->btRetType=DO_LUA;
	retcmd->nErrorcode=0;
	stAutoSetScriptParam aparam(pSender);
	if (parse["single"]!="") {
		CUserEngine::getMe().m_scriptsystem.InitScript(GameService::getMe().m_szQuestScriptPath, eScript_reload);
		return true;
	}
	else if (parse["file"]!=""){
		if (!CUserEngine::getMe().m_scriptsystem.m_LuaVM->DoFile(parse["file"].c_str())){
			CChat::sendSystem(pSender,vformat("Do script [%s] failed",parse["file"].c_str()));
			retcmd->nErrorcode=-1;
			CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
			return true;
		}
	}else if (parse["call"]!=""){
		if (!CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call_LuaStr<bool>(parse["call"].c_str(),false)){
			retcmd->nErrorcode=-1;
			CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
			return true;
		}
	}else if (parse["code"]!=""){
		if (!CUserEngine::getMe().m_scriptsystem.m_LuaVM->DoString(parse["code"].c_str())){
			retcmd->nErrorcode=-1;
			CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
			return true;
		}
	}else{
		if(!CUserEngine::getMe().m_scriptsystem.InitScript(GameService::getMe().m_szQuestScriptPath,eScript_reload)){
			CChat::sendSystem(pSender,"Reload Lua Script Failed");
			retcmd->nErrorcode=-1;
			CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
			return true;
		}
	}
	CChat::sendSystem(pSender,"Reload Lua Script Succeed");

	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	return true;
}

bool gm_cmdfunc_makeitem(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	stItem item;
	item.dwBaseID = atoi(parse["baseid"].c_str());
	auto pstBase = sJsonConfig.GetItemDataById(item.dwBaseID);

	if (!pstBase)
	{
		CChat::sendSystem(pSender,"@item baseid=0");
		return false;
	}
	DWORD dwCount=max(min(atoi(parse["count"].c_str()),9999),1);
	DWORD dwNpCof=atoi(parse["npcof"].c_str());
	DWORD dwHoleCof=atoi(parse["hlcof"].c_str());
	DWORD dwStrengCount=min(atoi(parse["sc"].c_str()),255);
	DWORD dwStrengLevel=min(atoi(parse["sl"].c_str()),255);
	DWORD dwNtd=min(atoi(parse["ntd"].c_str()),255);
	DWORD dwBinding=max(min(atoi(parse["binding"].c_str()),4),0);
	DWORD dwStarCof=max(atoi(parse["starcof"].c_str()),0);
	DWORD dwStarLv=max(min(atoi(parse["starlv"].c_str()),20),0);
	int nOldCount = 0;
	int nNowCount=0;
	DWORD dwMaxCount=pstBase->dwMaxCount;
	if (pstBase->dwMaxCount <= 0 ){
		CChat::sendSystem(pSender,"@item baseid=%d  最大叠加%d 出错!!!",pstBase->nID,pstBase->dwMaxCount);
		return false;
	}
	DWORD old = atoi(parse["old"].c_str());
	if (old == 0 && stRes::IsValidID(item.dwBaseID))
	{
		int nCount = min(atoi(parse["count"].c_str()), 9999999);
		pSender->ResChange(static_cast<ResID>(item.dwBaseID), nCount, "gmmake");
		return true;
	}
	int mapid=pSender?pSender->GetEnvir()->getMapId():1000;
	const char *pname=pSender?pSender->getName():"未知玩家";
	while (dwCount>0 && nNowCount<(int)dwCount){
		if (dwCount>dwMaxCount){
			nNowCount+=dwMaxCount;
			nNowCount=min((int)dwCount,nNowCount);
		}else{
			nNowCount=dwCount;
		}
		CItem* pItem =CItem::CreateItem(item.dwBaseID,_CREATE_MON_DROP,nNowCount-nOldCount,0,__FUNC_LINE__,mapid,"gm制造",pname);
		if (pItem)
		{
			pItem->m_Item.setdwBinding(dwBinding);
			pSender->m_Packet.AddItemToBag(pItem,false,false,true,"gmmake");
		}
		nOldCount = nNowCount;
	}
	return true;
}

bool gm_cmdfunc_Gold(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	retcmd->nErrorcode=1;

	CPlayerObj* pSenderEx = NULL;
	const char* szDstUser = parse["name"].c_str();
	if (szDstUser && szDstUser[0]!=0){
		pSenderEx= CUserEngine::getMe().m_playerhash.FindByName(szDstUser);
	}
	if(!pSenderEx){
		pSenderEx=pSender;
	}
	int nType=atoi(parse["type"].c_str());
	int nNum = atoi(parse["gold"].c_str());
	if (nNum > 2100000000) {
		CChat::sendSystem(pSender, "输入金额过大!");
		return false;
	}
	if(pSenderEx && !isSystemVirtualPlayer(pSenderEx)){
		switch (nType){
		case 0:	{
				pSenderEx->GoldChanged(atoi(parse["gold"].c_str()),false,"gmmake");
				CChat::sendSystem(pSender,"Add Gold OK!");
				retcmd->retStr.push_str(vformat("%s服务器已成功对%s增加了%s金币",GameService::getMe().m_szZoneName,parse["name"].c_str(),parse["gold"].c_str()));
				retcmd->nErrorcode=0;
			}break;
		case 1:	{
			}break;
		case 2:	{
				int dwAddRmb = atoi(parse["gold"].c_str());
				if (dwAddRmb > 1000000) { CChat::sendSystem(pSender, "Add RmbGold too much!"); return true; }
				pSenderEx->ResChange(ResID::charge, atoi(parse["gold"].c_str()), "GMMakeRmb");
				CChat::sendSystem(pSender,"Add RmbGold OK!");
				retcmd->retStr.push_str(vformat("%s服务器已成功对%s增加了%s大米",GameService::getMe().m_szZoneName,parse["name"].c_str(),parse["gold"].c_str()));
				retcmd->nErrorcode=0;
			}break;
		case 3:	{
				pSenderEx->ResChange(ResID::gamebind, atoi(parse["gold"].c_str()),"GMMakeLijin");
				CChat::sendSystem(pSender,"Add GiftsGold OK!");
				retcmd->retStr.push_str(vformat("%s服务器已成功对%s增加了%s绑定元宝",GameService::getMe().m_szZoneName,parse["name"].c_str(),parse["gold"].c_str()));
				retcmd->nErrorcode=0;
			}break;
		case 5:{
				pSenderEx->ResChange(ResID::game, atoi(parse["gold"].c_str()), "GMMakeYuanBao");
				CChat::sendSystem(pSender,"Add YuanBao OK!");
				retcmd->retStr.push_str(vformat("%s服务器已成功对%s增加了%s元宝",GameService::getMe().m_szZoneName,parse["name"].c_str(),parse["gold"].c_str()));
				retcmd->nErrorcode=0;
			}break;
		}
		CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	}else{//离线玩家
		BUFFER_CMD(stSvrSendGMCmd,proxycmd,stBasePacket::MAX_PACKET_SIZE);
		proxycmd->lv = 5;
		proxycmd->wgame_type = 0;
		proxycmd->wzoneid = 0;
		proxycmd->svr_id = 100;
		proxycmd->svr_type = _LOGINSVR_TYPE;
		proxycmd->chat_type = 0;
		proxycmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
		strcpy_s(proxycmd->szPlayerName,sizeof(proxycmd->szPlayerName)-1,name);
		proxycmd->stSendStr.push_str(vformat("@gold name=%s type=%s gold=%s",parse["name"].c_str(),parse["type"].c_str(),parse["gold"].c_str()));
		strcpy_s(proxycmd->szReasonStr,sizeof(proxycmd->szReasonStr)-1,"增加货币");
		if(nType==2){//rmb发给loginserver处理
			GameService::getMe().Send2LoginSvrs(proxycmd,sizeof(stSvrSendGMCmd) + proxycmd->stSendStr.getarraysize());
		}else{//其他直接发给DB超作
			GameService::getMe().Send2DBSvrs(proxycmd,sizeof(stSvrSendGMCmd)+proxycmd->stSendStr.getarraysize());
		}
	}
	return true;
}

bool gm_cmdfunc_spirit(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	CPlayerObj* pSenderEx = NULL;
	const char* szDstUser = parse["name"].c_str();
	if (szDstUser && szDstUser[0]!=0)
	{
		pSenderEx= CUserEngine::getMe().m_playerhash.FindByName(szDstUser);
	}
	if(pSenderEx== NULL)
	{	
		pSenderEx= CUserEngine::getMe().m_playerhash.FindByName(parse["user"].c_str());
		if(!pSenderEx)
		{
			pSenderEx = pSender;
		}		
	}
	int nType=atoi(parse["type"].c_str());
	return true;
}

bool gm_cmdfunc_skill(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	CPlayerObj* pSenderEx = NULL;
	const char* szDstUser = parse["name"].c_str();
	if (szDstUser && szDstUser[0]!=0)
	{
		pSenderEx= CUserEngine::getMe().m_playerhash.FindByName(szDstUser);
	}
	if(pSenderEx== NULL)
	{	
		pSenderEx= CUserEngine::getMe().m_playerhash.FindByName(parse["user"].c_str());
		if(!pSenderEx)
		{
			pSenderEx = pSender;
		}		
	}
	int all=atoi(parse["all"].c_str());
	int lv=max(atoi(parse["lv"].c_str()),1);
	int job=atoi(parse["job"].c_str());
	int exp=atoi(parse["exp"].c_str());
	if (all==0){
		int id=atoi(parse["id"].c_str());
		stMagic* pMagic=pSenderEx->m_cMagic.findskill(id);
		if (pMagic){
			pSenderEx->m_cMagic.removeskill(pMagic);
			pMagic = pSenderEx->m_cMagic.addskill(id, lv);
		}else{
			pMagic = pSenderEx->m_cMagic.addskill(id, lv);
		}
	}
	pSenderEx->m_cMagic.SendAllCretSkill();
	CChat::sendSystem(pSender,"技能加载成功");
	return true;
}

bool gm_cmdfunc_reloadstrres(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	GameService::getMe().reloadStrRes();
	if(pSender)
		CChat::sendSystem(pSender,"服务器字符串资源刷新成功");
	return true;
}

bool gm_cmdfunc_makemon(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	if (pSender){
		uint32_t monsterId = atoi(parse["id"].c_str());
		pSender->GetEnvir()->AddMonsters(monsterId,pSender->m_nCurrX, pSender->m_nCurrY, 1 ,1 ,0);
	}
	return true;
}

bool gm_ChangeStallStation(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	if(pSender)
	{
		int nType=atoi(parse["type"].c_str());
		if (nType==1)//关闭
		{
			stStallChangeStation stStallStation;
			stStallStation.bStation = false;
			CUserEngine::getMe().SendMsg2SuperSvr(&stStallStation,sizeof(stStallStation));
		} 
		else if(nType==2) //打开
		{
			stStallChangeStation stStallStation;
			stStallStation.bStation = true;
			CUserEngine::getMe().SendMsg2SuperSvr(&stStallStation,sizeof(stStallStation));
		}
	}
	return true;
}

bool gm_cmdfunc_level(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	retcmd->nErrorcode = 1;
	const char* szDstUser=parse["user"].c_str();
	CPlayerObj* pLevelUp=NULL;
	if (szDstUser && szDstUser[0]!=0){
		pLevelUp=CUserEngine::getMe().m_playerhash.FindByName(szDstUser);
	}else{
		pLevelUp=pSender;
	}
	if (pLevelUp){
		auto LimitLv = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("LimitLv", 100, pLevelUp);// +m_stAbility.LevelAdd;
		int nlevel=min(max(atoi(parse["lvl"].c_str()),1), LimitLv);
		pLevelUp->LuaLevelUp(nlevel);
		//
		if (pLevelUp!=pSender){
			//
		}
		retcmd->nErrorcode = 0;
		
	}else{

	}

	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	return true;
}

bool gm_cmdfunc_move(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	FUNCTION_BEGIN;
	int mapid = atoi(parse["mapid"].c_str());
	int nZoneid = atoi(parse["zoneid"].c_str());
	int nTradeid = atoi(parse["tradeid"].c_str());
	nZoneid = nZoneid == 0 ? 0xFFFFFFFF : nZoneid;
	int clonemapid = atoi(parse["cloneid"].c_str());
	DWORD destsvrid = atoi(parse["destsvrid"].c_str());
	CGameMap* pMap = CUserEngine::getMe().m_maphash.FindById(mapid, clonemapid);
	const char* szName = NULL;
	eCretType btNameType = CRET_NONE;
	if (!pMap) {
		btNameType = CRET_PLAYER;
		szName = parse["user"].c_str();
		if (szName == NULL || szName[0] == 0) {
			btNameType = CRET_NPC;
			szName = parse["npc"].c_str();
			if (szName == NULL || szName[0] == 0) {
				int npcid = atoi(parse["npcid"].c_str());
				if (npcid > 0) {
					for (auto it:sJsonConfig.npc_data)
					{
						if (it.id == npcid)
						{
							szName = it.name.c_str();
							break;
						}
					}
				}
			}
		}
	}
	const char* destName = NULL;
	destName = parse["destuser"].c_str();
	CPlayerObj* pmovedst = NULL;
	if (destName == NULL || destName[0] == 0) {
		pmovedst = pSender;
	}
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	//stSvrSendGMCmdRet retcmd;
	retcmd->svr_marking = GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId = GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName, _MAX_NAME_LEN_ - 1, name);
	retcmd->btRetType = TRANSMIT_PLAYER;
	if (pmovedst && isSystemVirtualPlayer(pmovedst)) {
		retcmd->nErrorcode = -1;
		//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"move命令输入错误，不能移动系统玩家");
		retcmd->retStr.push_str("move命令输入错误，不能移动系统玩家");
		//CUserEngine::getMe().SendMsg2GmManageSvr(&retcmd,sizeof(stSvrSendGMCmdRet));
		CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet) + retcmd->retStr.getarraysize());
		g_logger.error("move命令输入错误，不能移动系统玩家");
		return true;
	}
	pMap = pMap ? pMap : pmovedst->GetEnvir();
	PosType nx = atoi(parse["x"].c_str());
	PosType ny = atoi(parse["y"].c_str());
	if (pMap) {
		if (!pMap->CheckCanWalkWithOtherSvr(nx, ny, 0, true)) {
			//寻找一个可以站立的点
			if (!pMap->GetNearXY(nx, ny, nx, ny, 0, 0)) {
				CChat::sendSystem(pSender, "can not find a suitable moving point!");
				retcmd->nErrorcode = -1;
				//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"%s服务器 %s 传送失败",GameService::getMe().m_szZoneName,pmovedst->getName());
				retcmd->retStr.push_str(vformat("%s服务器 %s 传送失败", GameService::getMe().m_szZoneName, pmovedst->getName()));
				//CUserEngine::getMe().SendMsg2GmManageSvr(&retcmd,sizeof(stSvrSendGMCmdRet));
				CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet) + retcmd->retStr.getarraysize());
				return true;
			}
		}
		if (pmovedst->MoveToMap(pMap->getMapId(), 0, nx, ny, destsvrid)) {
			retcmd->retStr.push_str(vformat("%s服务器 %s 传送成功", GameService::getMe().m_szZoneName, pmovedst->getName()));
		}
		else {
			retcmd->nErrorcode = -1;
			//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"%s服务器 %s 传送失败",GameService::getMe().m_szZoneName,pmovedst->getName());
			retcmd->retStr.push_str(vformat("%s服务器 %s 传送失败", GameService::getMe().m_szZoneName, pmovedst->getName()));
		}
	}
	else {
		retcmd->nErrorcode = -1;
		//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"%s服务器 %s 传送失败",GameService::getMe().m_szZoneName,pmovedst->getName());
		retcmd->retStr.push_str(vformat("%s服务器 %s 传送失败", GameService::getMe().m_szZoneName, pmovedst->getName()));
	}
	//CUserEngine::getMe().SendMsg2GmManageSvr(&retcmd,sizeof(stSvrSendGMCmdRet));
	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet) + retcmd->retStr.getarraysize());
	return true;
}

bool gm_cmdfunc_clearitem(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	if (pSender){
		for (int i=0;i<_MAX_BAG_COUNT;i++)
		{
			for (auto it =  pSender->m_Packet.m_BagPacket[i].m_PackItemList.begin(),itnext = it;it != pSender->m_Packet.m_BagPacket[i].m_PackItemList.end();it = itnext){
				itnext++;
				CItem* item= it->second;
				if (!item->GetOutLock())
				{
					if (pSender->m_btGmLvl>0){
						pSender->m_Packet.DeleteItemInBag(item,0xffffffff,true);
					}
				}
			}
		}
	}
	return true;
}

bool gm_cmdfunc_abiadd(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	const char* szDstUser=parse["name"].c_str();
	CPlayerObj* pAbiAdd=NULL;
	if (szDstUser && szDstUser[0]!=0){
		pAbiAdd=CUserEngine::getMe().m_playerhash.FindByName(szDstUser);
	}else{
		pAbiAdd=pSender;
	}
	if (pAbiAdd){
		int nall=atoi(parse["all"].c_str());
		int nNum=atoi(parse["num"].c_str());
		int nadd=atoi(parse["add"].c_str());
		if (nall==1){
			for (int i=0;i<sizeof(stARpgAbility);i+=sizeof(int))
			{
				if (nadd==1){
					*(int*)(&((char*)(&(pAbiAdd->m_stLuaAbility)))[i])+=nNum;
				}else *(int*)(&((char*)(&(pAbiAdd->m_stLuaAbility)))[i])=nNum;
			}
		}else{
			int nidx=atoi(parse["idx"].c_str());
			if (nidx>=0 && nidx<sizeof(stARpgAbility)/sizeof(int)){
				if (nadd==1){
					*(int*)(&((char*)(&(pAbiAdd->m_stLuaAbility)))[nidx*sizeof(int)])+=nNum;
				}else *(int*)(&((char*)(&(pAbiAdd->m_stLuaAbility)))[nidx*sizeof(int)])=nNum;
			}
		}
		//
		if (pAbiAdd!=pSender){
			//
		}
		pAbiAdd->ChangeProperty(true,__FUNC_LINE__);
		CChat::sendSystem(pSender,"属性点增加成功！");
	}else{

	}
	return true;
}

bool gm_cmdfunc_reloaddb(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	//mon|mongen|hidemon|mondropitem|notice|npcgen|gm|quest|randomdrop
	if (pSender){
		BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
		retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
		retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
		strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
		retcmd->nErrorcode=-1;
		retcmd->btRetType=RELOADDB;
		const char* reloaddb=parse["load"].c_str();
		if (stricmp(reloaddb,"rank")==0){
			int nRankType=atoi(parse["ranktype"].c_str());
			stReloadRankByType ReloadRankCmd;
			ReloadRankCmd.nRankType=nRankType?nRankType:Rank_Max_Count;
			BUFFER_CMD(stSendRankMsgSuperSrv,rankcmd,stBasePacket::MAX_PACKET_SIZE);
			rankcmd->msg.push_back((char*)&ReloadRankCmd,sizeof(ReloadRankCmd),__FUNC_LINE__);
			CUserEngine::getMe().SendMsg2SuperSvr(rankcmd,sizeof(*rankcmd)+rankcmd->msg.getarraysize());
			g_logger.warn("排行榜 数据库加载成功!");
			CChat::sendSystem(pSender,"排行榜 数据库加载成功!");
			retcmd->nErrorcode=0;
			retcmd->retStr.push_str(vformat("排行榜 数据库加载成功!"));
		}else if (stricmp(reloaddb,"magicrange")==0){
			if(CMagicRangeDefine::getMe().init()){
				g_logger.warn("MagicRange.xml加载成功!");
				retcmd->nErrorcode=0;
				retcmd->retStr.push_str(vformat("MagicRange.xml加载成功!"));
			}
		}else if(stricmp(reloaddb, "chatfilter") == 0){
			CFilter::getMe().init("FilterChat.txt");
			g_logger.warn("FilterChat加载成功!");
		}
		else if (stricmp(reloaddb, "all") == 0) {
			if (!sJsonConfig.LoadAllData()) {
				g_logger.forceLog(zLogger::zINFO, "重载配置失败");
				return false;
			}
		}
		else
		{
			if (!sJsonConfig.Reload(reloaddb)) {
				g_logger.forceLog(zLogger::zINFO, "重载配置失败");
				return false;
			}
		}
		g_logger.warn("%s 执行完成!",szcmd);
		CChat::sendSystem(pSender,"%s 执行完成!",szcmd);
		CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	}
	return true;
}

bool gm_cmdfunc_who(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	//stSvrSendGMCmdRet retcmd;
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	if (parse[_GMCMDSENDERQZ_]!="" ){
		CChat::sendSystem( parse[_GMCMDSENDERQZ_].c_str(),"服务器 %d 在线人数:%d",GameService::getMe().m_Svr2SvrLoginCmd.svr_id,CUserEngine::getMe().m_playerhash.size() );
	}else if(pSender){
		retcmd->nErrorcode=0;
		retcmd->btRetType=ONLINE_PEOPLE_QUERY;
		//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"%d",CUserEngine::getMe().m_playerhash.size() );
		retcmd->retStr.push_str(vformat("%d",CUserEngine::getMe().m_playerhash.size()));
		CChat::sendSystem( pSender,"区:%d GS:%d 在线人数:%d",GameService::getMe().m_nTrueZoneid,GameService::getMe().m_Svr2SvrLoginCmd.svr_id,CUserEngine::getMe().m_playerhash.size());
		//CUserEngine::getMe().SendMsg2GmManageSvr(&retcmd,sizeof(retcmd));
		CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	}
	return true;
}

bool gm_cmdfunc_ys(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	return true;
}

bool gm_cmdfunc_setgm(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	CPlayerObj* player = CUserEngine::getMe().m_playerhash.FindByName(parse["name"].c_str());
	if (player)
	{
		g_logger.warn("%s 修改 %s 的GM权限 由 %d 至 %d",pSender?pSender->getName():"控制台",player->getName(),player->m_btGmLvl,atoi(parse["gmlvl"].c_str()));
		player->m_btGmLvl = min(atoi(parse["gmlvl"].c_str()),gmlvl);
		stPlayerInfo2GatewayCmd playerset;
		playerset.btGmLv=player->m_btGmLvl;
		player->SendMsgToMe(&playerset,sizeof(playerset));
		DWORD dwtime=atoi(parse["time"].c_str());
		if (dwtime){
			player->m_boTmpGm=true;
			player->m_dwTmpGmTime=time(NULL)+dwtime;
			CChat::sendClient(pSender,"您将 %s 设置为 %d 级GM,有效时间 %d 秒,下线失效.",player->getName(),player->m_btGmLvl,dwtime);
			CChat::sendClient(player,"您被设置为 %d 级GM,有效时间 %d 秒,下线失效.",player->m_btGmLvl,dwtime);
		}else{
			CChat::sendClient(pSender,"您将 %s 设置为 %d 级GM,下线失效.",player->getName(),player->m_btGmLvl);
			CChat::sendClient(player,"您被设置为 %d 级GM,下线失效.",player->m_btGmLvl);
		}
	}
	return true;
}

bool gm_cmdfunc_setleader(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	CPlayerObj* player = CUserEngine::getMe().m_playerhash.FindByName(parse["name"].c_str());
	if (player)
	{
		g_logger.warn("%s 修改 %s 的引导员权限 由 %d 至 %d",pSender?pSender->getName():"控制台",player->getName(),player->m_btLeader,atoi(parse["leaderlvl"].c_str()));
		player->m_btLeader = atoi(parse["leaderlvl"].c_str());
		DWORD dwtime=atoi(parse["time"].c_str());
		if (player->m_btLeader){
			if (dwtime){
				player->m_tLeaderPeriodTime=time(NULL)+dwtime;
				if (CUserEngine::getMe().m_scriptsystem.m_LuaVM){
					stAutoSetScriptParam autoparam(player);
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("SetLeader");
				}
				CChat::sendClient(pSender,"您将 %s 设置为 %d 级引导员,有效时间 %d 秒.",player->getName(),player->m_btLeader,dwtime);
				CChat::sendClient(player,"您被设置为 %d 级引导员,有效时间 %d 秒.",player->m_btLeader,dwtime);
			}else{
				player->m_btLeader=0;
				player->m_tLeaderPeriodTime=0;
				CChat::sendClient(pSender,"您将 %s 取消引导员权限.",player->getName());
				CChat::sendClient(player,"您被取消引导员权限.");
			}
		}else{
			player->m_btLeader=0;
			player->m_tLeaderPeriodTime=0;
			CChat::sendClient(pSender,"您将 %s 取消引导员权限.",player->getName());
			CChat::sendClient(player,"您被取消引导员权限.");
		}
	}
	return true;
}

bool gm_cmdfunc_job(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	CPlayerObj* player = CUserEngine::getMe().m_playerhash.FindByName(parse["name"].c_str());
	if (!player)
	{
		player=pSender;
	}
	if (player){
		int nJob=atoi(parse["job"].c_str());
		switch (nJob)
		{
		case MAGE_JOB:
			{
				player->m_siFeature.job = MAGE_JOB;
				CChat::sendClient(pSender,"您将 %s 设置为 法师",player->getName());
				CChat::sendClient(player,"您被设置为 法师");
			}break;
		}
		player->UpdateAppearance(FeatureIndex::job,player->GetJobType());
		player->GetBaseProperty();
		player->ChangeProperty(true,__FUNC_LINE__);
	}
	return true;
}

bool gm_cmdfunc_sex(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	CPlayerObj* player = CUserEngine::getMe().m_playerhash.FindByName(parse["name"].c_str());
	if (!player)
	{
		player=pSender;
	}
	if (player)
	{
		int nSex=atoi(parse["sex"].c_str());
		switch (nSex)
		{
		case MAN_SEX:
			{
				player->m_siFeature.sex = MAN_SEX;
				CChat::sendClient(pSender,"您将 %s 设置为 男性",player->getName());
				CChat::sendClient(player,"您被设置为 男性");
			}break;
		case WOMAN_SEX:
			{
				player->m_siFeature.sex = WOMAN_SEX;
				CChat::sendClient(pSender,"您将 %s 设置为 女性",player->getName());
				CChat::sendClient(player,"您被设置为 女性");
			}break;
		}
		player->UpdateAppearance(FeatureIndex::sex,player->GetSexType());
		player->GetBaseProperty();
		player->ChangeProperty(true,__FUNC_LINE__);
	}
	return true;
}

bool gm_cmdfunc_sandbox(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	return true;
}
bool gm_cmdfunc_clearmon(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	if (isSystemVirtualPlayer(pSender)){
		return true;
	}
	CGameMap* pClearMap=NULL;
	int nMapid=atoi(parse["mapid"].c_str());
	pClearMap=pSender->GetEnvir();

	if (pClearMap)
	{
		pClearMap->m_boClearMon=true;
	}
	return true;
}

bool gm_cmdfunc_searchplayer(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	CPlayerObj* searchplayer=CUserEngine::getMe().m_playerhash.FindByName(parse["name"].c_str());
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	//stSvrSendGMCmdRet retcmd;
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	if (searchplayer){
		retcmd->nErrorcode=0;
		retcmd->btRetType=SEARCH_PLAYER;
		char szRetStr[_MAX_RETCMD_LEN_];

		sprintf_s(szRetStr,_MAX_RETCMD_LEN_-1,"<node play=\"%s\" id=\"%d\" ip=\"%s\" ltime=\"%d\" otime=\"%d\" sex=\"%d\" job=\"%d\" exp=\"%I64d\" lv=\"%d\" guildid=\"%d \" gold=\"%d\" rmb=\"%d\" yuanbao=\"%I64d\" bangyuan=\"%I64d\" map=\"%s\" x=\"%d\" y=\"%d\" isoffline=\"%d\" userstate=\"%d\" account=\"%s\" gateip=\"%s\" gateport=\"%d\" />",
			searchplayer->getName(),searchplayer->GetObjectId(),searchplayer->m_pGateUser->szclientip,searchplayer->m_dwLoginTime,searchplayer->m_dwPlayerOnlineTime,searchplayer->m_siFeature.sex,searchplayer->m_siFeature.job,searchplayer->m_res[ResID::exp],searchplayer->m_dwLevel,searchplayer->m_GuildInfo.dwGuildId,searchplayer->m_dwGold,searchplayer->m_res[ResID::charge],
			searchplayer->m_res[ResID::game],searchplayer->m_res[ResID::gamebind],searchplayer->GetEnvir()->getMapName(),searchplayer->m_nCurrX,searchplayer->m_nCurrY,((BYTE)searchplayer->m_pGateUser->isSocketClose()),searchplayer->m_btSelUserState,searchplayer->m_pGateUser->m_szAccount,searchplayer->m_pGateUser->szgateip,searchplayer->m_pGateUser->wgateport);

	    CChat::sendSystem(pSender,szRetStr);
		retcmd->retStr.push_str(szRetStr);
		CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	}
	else {
		BUFFER_CMD(stSvrSendGMCmd,proxycmd,stBasePacket::MAX_PACKET_SIZE);
		proxycmd->lv = 5;
		proxycmd->wgame_type = 0;
		proxycmd->wzoneid = 0;
		proxycmd->svr_id = 200;
		proxycmd->svr_type = 200;
		proxycmd->chat_type = 0;
		proxycmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
		strcpy_s(proxycmd->szPlayerName,sizeof(proxycmd->szPlayerName)-1,name);
		proxycmd->stSendStr.push_str(vformat("@findplayer name=%s",parse["name"].c_str()));
		strcpy_s(proxycmd->szReasonStr,sizeof(proxycmd->szReasonStr)-1,"查询玩家信息");
		GameService::getMe().Send2DBSvrs(proxycmd,sizeof(stSvrSendGMCmd)+proxycmd->stSendStr.getarraysize());
	}
	return true;
}

bool gm_cmdfunc_manageguild(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	BUFFER_CMD(stDeleteGameGuild, pcmd, stBasePacket::MAX_PACKET_SIZE);
	pcmd->dwGuildId = atoi(parse["guildid"].c_str());
	strcpy_s(pcmd->szPlayerName, _MAX_NAME_LEN_ - 1, name);
	CUserEngine::getMe().SendMsg2GlobalSvr(pcmd, sizeof(stDeleteGameGuild));
	return true;
}

bool gm_cmdfunc_nocd(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	pSender->m_NoCdMode=(!pSender->m_NoCdMode);
	if(pSender->m_NoCdMode!=0){
		CChat::sendSystem(pSender,"开启GM命令 技能无消耗无CD模式!");
		g_logger.debug("[%s]开启GM命令 技能无消耗无CD模式",pSender->getName());
	}else{	
		CChat::sendSystem(pSender,"关闭GM命令 技能无消耗无CD模式!");
		g_logger.debug("[%s]关闭GM命令 技能无消耗无CD模式",pSender->getName());
	}
	return true;
}

static unsigned char g_szSaveFindItemBuffer[1024*1024];		//缓存数据
static unsigned char g_szComFindItemBuffer[1024*1024];			//压缩数据
bool gm_cmdfunc_findplayeritem(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	CPlayerObj* searchplayer=CUserEngine::getMe().m_playerhash.FindByName(parse["name"].c_str());
	int nSearchType=atoi(parse["type"].c_str());
	PTR_CMD(stSvrSendGMCmdRet,retcmd,getsafepacketbuf());
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	retcmd->nErrorcode=-1;
	if(searchplayer){
		unsigned char* pbindata=&g_szSaveFindItemBuffer[0];
		unsigned long bindatasize=sizeof(g_szSaveFindItemBuffer);

		if(nSearchType==1){//装备
			if(searchplayer->m_Packet.saveEquip((char*)pbindata,bindatasize)){
				retcmd->nErrorcode=0;
			}
		}else if(nSearchType==2){//包裹
			if(searchplayer->m_Packet.saveBagPacket((char*)pbindata,bindatasize)){
				retcmd->nErrorcode=0;
			}
		}else if(nSearchType==3){//仓库
			if(searchplayer->m_Packet.saveStoragePacket((char*)pbindata,bindatasize)){
				retcmd->nErrorcode=0;
			}
		}
		if(retcmd->nErrorcode==0){
			retcmd->nErrorcode=-1;
			pbindata[bindatasize]=0;
			unsigned long nOutLen=sizeof(g_szComFindItemBuffer);
			if (compresszlib((unsigned char*)pbindata,bindatasize,&g_szComFindItemBuffer[0],nOutLen)== Z_OK) {
				*((int*)(&g_szSaveFindItemBuffer[0]))=bindatasize;
				memcpy(&g_szSaveFindItemBuffer[sizeof(int)],&g_szComFindItemBuffer[0],nOutLen);

				nOutLen+=sizeof(int);
				int retlen=ROUNDNUMALL(nOutLen,3)/3*4;	//当前长度，偏移用
				if(retlen<20*1024){
					retcmd->nErrorcode=0;
					base64_encode((char*)&g_szSaveFindItemBuffer[0],nOutLen,(char*)&g_szComFindItemBuffer[0],retlen);
					g_szComFindItemBuffer[retlen]=0;
					retcmd->retStr.push_str((char*)&g_szComFindItemBuffer[0]);
				}
			}
		}
		CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	}else{
		BUFFER_CMD(stSvrSendGMCmd,proxycmd,stBasePacket::MAX_PACKET_SIZE);
		proxycmd->lv = 5;
		proxycmd->wgame_type = 0;
		proxycmd->wzoneid = 0;
		proxycmd->svr_id = 200;
		proxycmd->svr_type = _DBSVR_TYPE;
		proxycmd->chat_type = 0;
		proxycmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
		strcpy_s(proxycmd->szPlayerName,sizeof(proxycmd->szPlayerName)-1,name);
		proxycmd->stSendStr.push_str(vformat("@findplayeritem name=%s type=%s",parse["name"].c_str(),parse["type"].c_str()));
		strcpy_s(proxycmd->szReasonStr,sizeof(proxycmd->szReasonStr)-1,"查询玩家信息");
		GameService::getMe().Send2DBSvrs(&proxycmd,sizeof(stSvrSendGMCmd)+proxycmd->stSendStr.getarraysize());
	}
	return true;
}

bool gm_cmdfunc_banlist(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	int b=atoi(parse["b"].c_str());
	int e=atoi(parse["e"].c_str());
	int idx=0,num=0;
	CPlayersHashManager::iter it;
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	//stSvrSendGMCmdRet retcmd;
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	char tmpstr[_MAX_RETCMD_LEN_];
	ZeroMemory(tmpstr,_MAX_RETCMD_LEN_);
	const char* menu="<node name=\"%s\" grade=\"%d\" ban=\"%d\" off=\"%d\"/>#re#";
	for (it=CUserEngine::getMe().m_playerhash.begin();it!=CUserEngine::getMe().m_playerhash.end();it++)
	{
		CPlayerObj* banplayer=it->second;
		if (banplayer->m_dwBanChatTime!=0 && banplayer->m_dwBanChatTime>((DWORD)time(NULL))){
			if (idx>=b && idx<=e){
				char printstr[_MAX_ORDER_LEN_];
				if (tmpstr[0]==0){
					sprintf_s(tmpstr,_MAX_RETCMD_LEN_-1,menu,banplayer->getName(),banplayer->m_dwLevel,banplayer->m_dwBanChatTime,0);
				}
				else{
					sprintf_s(printstr,_MAX_ORDER_LEN_-1,menu,banplayer->getName(),banplayer->m_dwLevel,banplayer->m_dwBanChatTime,0);
					str_replace(tmpstr,"#re#",printstr);
				}
				num++;
			}
			idx++;
		}
	}
	if (idx==0){
		retcmd->nErrorcode=-1;
		retcmd->btRetType=BAN_LIST_QUERY;
		//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"查询禁言玩家列表没有成功");
		retcmd->retStr.push_str("查询禁言玩家列表没有成功");
	}
	else {
		retcmd->nErrorcode=0;
		retcmd->btRetType=BAN_LIST_QUERY;
		str_replace(tmpstr,"#re#","");
		//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"cout=\"%d\" %s",num,tmpstr);
		retcmd->retStr.push_str(vformat("cout=\"%d\" %s",num,tmpstr));
	}
	CChat::sendSystem(pSender,tmpstr);
	//CUserEngine::getMe().SendMsg2GmManageSvr(&retcmd,sizeof(retcmd));
	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	return true;
}

bool gm_cmdfunc_banchat(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	CPlayerObj* banplayer=CUserEngine::getMe().m_playerhash.FindByName(parse["name"].c_str());
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	int ban=atoi(parse["ban"].c_str());
	int ret=atoi(parse["ret"].c_str());
	if (banplayer){
		retcmd->nErrorcode=0;
		if (ban==1){
			int ntime=atoi(parse["time"].c_str());
			banplayer->m_dwBanChatTime=time(NULL)+ntime;
			retcmd->btRetType=BAN_PLAYER;
			retcmd->retStr.push_str(vformat("%s服务器已成功对%s进行了禁言操作",GameService::getMe().m_szZoneName,parse["name"].c_str()));
			if (ret==1){
				CChat::sendSystem(banplayer,"您已经被禁言");
			}
		}else if (ban==0){
			banplayer->m_dwBanChatTime=0;
			retcmd->btRetType=UNBAN_PLAYER;
			retcmd->retStr.push_str(vformat("%s服务器已成功对%s进行了解禁操作",GameService::getMe().m_szZoneName,parse["name"].c_str()));
			if (ret==1){
				CChat::sendSystem(banplayer,"您禁言已被解除");
			}
		}
		CChat::sendSystem(pSender,retcmd->retStr.getptr());
		CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	}else {
		BUFFER_CMD(stSvrSendGMCmd,proxycmd,stBasePacket::MAX_PACKET_SIZE);
		proxycmd->lv = 5;
		proxycmd->wgame_type = 0;
		proxycmd->wzoneid = 0;
		proxycmd->svr_id = 200;
		proxycmd->svr_type = _DBSVR_TYPE;
		proxycmd->chat_type = 0;
		proxycmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
		strcpy_s(proxycmd->szPlayerName,sizeof(proxycmd->szPlayerName)-1,name);
		proxycmd->stSendStr.push_str(vformat("@banchat name=%s ban=%s time=%s ret=%s",parse["name"].c_str(),parse["ban"].c_str(),parse["time"].c_str(),parse["ret"].c_str()));
		strcpy_s(proxycmd->szReasonStr,sizeof(proxycmd->szReasonStr)-1,"角色禁言");
		GameService::getMe().Send2DBSvrs(proxycmd,sizeof(stSvrSendGMCmd)+proxycmd->stSendStr.getarraysize());
	}
	return true;
}

bool gm_cmdfunc_banplayer(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	CPlayerObj* banplayer=CUserEngine::getMe().m_playerhash.FindByName(parse["name"].c_str());
	/*BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);*/
	int nban=atoi(parse["ban"].c_str());
	if (banplayer && nban==1){
		//retcmd->nErrorcode=0;
		int ntime=atoi(parse["time"].c_str());
		banplayer->m_dwBanPlayerTime=time(NULL)+ntime;
		banplayer->MakeGhost(true,__FUNC_LINE__);
		//retcmd->btRetType=FORCE_OFFLINE;
		//retcmd->retStr.push_str(vformat("%s服务器已成功对%s进行了强制下线操作并封角色%d秒",GameService::getMe().m_szZoneName,parse["name"].c_str(),ntime));
		//CChat::sendSystem(pSender,retcmd->retStr.getptr());
		//CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	}

	BUFFER_CMD(stSvrSendGMCmd,proxycmd,stBasePacket::MAX_PACKET_SIZE);
	proxycmd->lv = 5;
	proxycmd->wgame_type = 0;
	proxycmd->wzoneid = 0;
	proxycmd->svr_id = 200;
	proxycmd->svr_type = _DBSVR_TYPE;
	proxycmd->chat_type = 0;
	proxycmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(proxycmd->szPlayerName,sizeof(proxycmd->szPlayerName)-1,name);
	proxycmd->stSendStr.push_str(vformat("@banplayer name=%s ban=%s time=%s",parse["name"].c_str(),parse["ban"].c_str(),parse["time"].c_str()));
	strcpy_s(proxycmd->szReasonStr,sizeof(proxycmd->szReasonStr)-1,"角色封号");
	GameService::getMe().Send2DBSvrs(proxycmd,sizeof(stSvrSendGMCmd)+proxycmd->stSendStr.getarraysize());
	return true;
}

bool gm_cmdfunc_querychannel(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	//stSvrSendGMCmdRet retcmd;
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	int channel=atoi(parse["channel"].c_str());
	int open=atoi(parse["open"].c_str());
	if (channel >= CHAT_TYPE_PRIVATE && channel <=CHAT_TYPE_QQ){
		retcmd->nErrorcode=0;
		if (open==1){
			if (CChat::m_dwSendGmManage==0 || (CChat::m_dwSendGmManage & (1 << (channel-1)))==0){
				CChat::m_dwSendGmManage=CChat::m_dwSendGmManage + (0 | (1 << (channel-1)));
			}
			retcmd->btRetType=CHANNEL_START;
			//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"开启频道[%d]聊天",channel);
			retcmd->retStr.push_str(vformat("开启频道[%d]聊天",channel));
		}
		else if (open==0){
			if (CChat::m_dwSendGmManage!=0 || (CChat::m_dwSendGmManage & (1 << (channel-1)))!=0){
				CChat::m_dwSendGmManage=CChat::m_dwSendGmManage - (CChat::m_dwSendGmManage & (1 << (channel-1)));
			}
			retcmd->btRetType=CHANNEL_STOP;
			//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"关闭频道[%d]聊天",channel);
			retcmd->retStr.push_str(vformat("关闭频道[%d]聊天",channel));
		}
		retcmd->chat_type=channel;
	}
	else {
		retcmd->nErrorcode=-1;
		if (open==1){
			retcmd->btRetType=CHANNEL_START;
			//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"开启频道[%d]超出范围",channel);
			retcmd->retStr.push_str(vformat("开启频道[%d]超出范围",channel));
		}
		else if (open==0){
			retcmd->btRetType=CHANNEL_STOP;
			//sprintf_s(retcmd.szRetStr,_MAX_RETCMD_LEN_-1,"关闭频道[%d]超出范围",channel);
			retcmd->retStr.push_str(vformat("关闭频道[%d]超出范围",channel));
		}
		retcmd->chat_type=channel;
	}
	CChat::sendSystem(pSender,retcmd->retStr.getptr());
	//CUserEngine::getMe().SendMsg2GmManageSvr(&retcmd,sizeof(retcmd));
	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	return true;
}

bool gm_cmdfunc_addboard(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	const char* type;		//操作类型
	int idx;				//公告ID
	DWORD stime = 0;		//开始时间
	DWORD etime = 0;		//结束时间
	int per;				//间隔时间
	const char* msg;		//消息内容
	DWORD itime = 0;
	bool state = false;

	type = parse["type"].c_str();
	if(NULL == type){
		return false;
	}

	stime = atoi(parse["stime"].c_str());
	idx = atoi(parse["id"].c_str());
	etime = atoi(parse["etime"].c_str());
	per = atoi(parse["per"].c_str());
	msg = parse["msg"].c_str();
	itime = atoi(parse["time"].c_str());

	/*
	* 返回值:
	*
	* 0 成功
	*/
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	retcmd->dwTrueZoneId=GameService::getMe().m_nTrueZoneid;
	retcmd->nErrorcode = 0;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);

	if(0 == strcmp(type, "del"))
	{
		if(idx < 0)
		{
			return false;
		}

		//删除指定id的公告
//		retcmd->btRetType = BOARD_DEL;
// 		state = stBoard::del(idx);
// 		char buf[_MAX_RETCMD_LEN_];
// 		memset(buf, 0, _MAX_RETCMD_LEN_);
// 		if(false == state){
// 			sprintf_s(buf, _MAX_RETCMD_LEN_-1,
// 				"无法删除服务器[%s], id=[%d]的公告", GameService::getMe().m_szZoneName, idx);
// 		}else{
// 			sprintf_s(buf, _MAX_RETCMD_LEN_-1,
// 				"成功删除服务器[%s], id=[%d]的公告", GameService::getMe().m_szZoneName, idx);
// 		}
// 		retcmd->retStr.push_back(buf, strlen(buf));

		if (CUserEngine::getMe().m_scriptsystem.m_LuaVM){
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("RunBoard",BOARD_DEL,name,szcmd);
		}
	}
	else if(0 == strcmp(type, "add"))
	{
//		stBoard board;
		if(idx < 0 || stime < 0 || etime < 0 || per < 0 || msg == NULL){
			return false;
		}

// 		ZeroMemory(&board, sizeof(stBoard));
// 		board.id = idx;
// 		board.beginTime = stime;
// 		board.endTime = etime;
// 		strncpy(board.szNotice, msg, sizeof(board.szNotice)-1);
// 
// 		//添加公告
// 		retcmd->btRetType = BOARD_ADD;
// 		state = stBoard::add(board);
// 		char buf[_MAX_RETCMD_LEN_];
// 		memset(buf, 0, _MAX_RETCMD_LEN_);
// 		if(false == state){
// 			sprintf_s(buf, _MAX_RETCMD_LEN_-1,
// 				"无法添加服务器[%s], id=[%d]的公告", GameService::getMe().m_szZoneName, idx);
// 		}else{
// 			sprintf_s(buf, _MAX_RETCMD_LEN_-1,
// 				"成功添加服务器[%s], id=[%d]的公告", GameService::getMe().m_szZoneName, idx);
// 		}
// 		retcmd->retStr.push_back(buf, strlen(buf));

		if (CUserEngine::getMe().m_scriptsystem.m_LuaVM){
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("RunBoard",BOARD_ADD,name,szcmd);
		}
	}
	else if(0 == strcmp(type, "get"))
	{

	}
	else if(0 == strcmp(type, "getl"))
	{
// 		//读取服务器中的公告列表
// 		stBoard::BoardList boards;
// 		stBoard::getList();
// 		stBoard::getList(boards);
// 		state = true;
// 
// 		if(boards.size() > 0){
// 			char buf[30000];
// 			const int bufLen = 30000;
// 			char tempBuf[1024];
// 			const int tempBufLen = 1024;
// 			memset(buf, 0, sizeof(buf));
// 
// 			stBoard::BoardList::iterator iter = boards.begin();
// 			stBoard::BoardList::iterator iterEnd = boards.end();
// 			char *ptr = buf;
// 			int useLen = 0;
// 			int nCount = 0;
// 
// 			//<node stime=xxx etime=xxx per=xxx msg=xxx/>
// 			for(;iter != iterEnd; ++iter){
// 				memset(tempBuf, 0, sizeof(tempBuf));
// 				int len = sizeof(iter->szNotice);
// 				iter->szNotice[len-1] = '\0';
// 				sprintf_s(tempBuf, sizeof(tempBuf)-1, "<node id=%d stime=%d etime=%d msg=%s/>",
// 					iter->id, iter->beginTime, iter->endTime, iter->szNotice);
// 				len = strlen(tempBuf);
// 				if((useLen+len) >= bufLen){
// 					break;
// 				}
// 				int freeLen = bufLen-useLen;
// 				strncpy(ptr, tempBuf, len);
// 
// 				useLen += len;
// 				ptr += len;
// 				nCount++;
// 			}
// 
// 			//返回获取到的列表
// 			char retBuf[30000];
// 			memset(retBuf, 0, sizeof(retBuf));
// 			sprintf_s(retBuf,sizeof(retBuf)-1, "count=%d %s", nCount, buf);
// 			retcmd->retStr.push_back(retBuf, strlen(retBuf));
// 		}else
// 		{
// 			state = false;
// 		}
// 		retcmd->btRetType = BOARD_GETL;

		if (CUserEngine::getMe().m_scriptsystem.m_LuaVM){
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("RunBoard",BOARD_GETL,name,szcmd);
		}
	}
	else if(0 == strcmp(type, "update"))
	{
		if(itime < 0)
		{
			return false;
		}

// 		state = stBoard::updateInterval(itime);
// 		retcmd->btRetType = BOARD_UPDATE;

		if (CUserEngine::getMe().m_scriptsystem.m_LuaVM){
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("RunBoard",BOARD_UPDATE,name,szcmd);
		}
	}
	else if(0 == strcmp(type, "read"))
	{
// 		state = true;
// 		DWORD itime = 0;
// 		stBoard::readInterval(itime);
// 		retcmd->btRetType = BOARD_UPDATE;
// 		char buf[_MAX_RETCMD_LEN_];
// 		memset(buf, 0, _MAX_RETCMD_LEN_);
// 		sprintf_s(buf, _MAX_RETCMD_LEN_-1, "num=%d", itime);
// 		retcmd->retStr.push_back(buf, strlen(buf));
// 		retcmd->btRetType = BOARD_READ;

		if (CUserEngine::getMe().m_scriptsystem.m_LuaVM){
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("RunBoard",BOARD_READ,name,szcmd);
		}
	}
	else
	{
		return false;
	}

// 	if(state == false)
// 	{
// 		retcmd->nErrorcode = -1;
// 	}
// 	else
// 	{
// 		retcmd->nErrorcode = 0;
// 	}
// 	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());

	return true;
}

bool gm_cmdfunc_zoneinfo(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	FUNCTION_BEGIN;
	if (!isSystemVirtualPlayer(pSender)){
		CChat::sendToGmClient(pSender,"区名 %s 区编号 %d",GameService::getMe().m_szZoneName,GameService::getMe().m_nZoneid);
	}
	g_logger.warn("区名 %s 区编号 %d",GameService::getMe().m_szZoneName,GameService::getMe().m_nZoneid);
	return true;
}

bool gm_cmdfunc_testcmd(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	if (pSender){
		int nNum=atoi(parse["num"].c_str());
		stGmExecTestCmd retcmd;
		for (int i=0;i<nNum;i++)
		{
			retcmd.nNum=i;
			pSender->SendMsgToMe(&retcmd,sizeof(retcmd));
		}
	}
	return true;
}

bool gm_cmdfunc_setversion(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	DWORD dwVersion=strtotime(parse["ver"].c_str());
	g_logger.error("版本修改成功 当前版本 %s 更改后版本 %s",timetostr(CUserEngine::getMe().m_dwThisGameSvrVersion.load()),parse["ver"].c_str());
	CUserEngine::getMe().m_dwThisGameSvrVersion=dwVersion;

	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	retcmd->btRetType=NORMALSTR;
	retcmd->nErrorcode=0;
	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	return true;
}

bool cmdfunc_closegate(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){

	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	retcmd->btRetType=NORMALSTR;
	retcmd->nErrorcode=1;

	if(parse["c"]!=""){
		GameService::getMe().m_boForceCloseGetWay=atoi(parse["c"].c_str());
		if(GameService::getMe().m_boForceCloseGetWay == 1){
			g_logger.forceLog(zLogger::zINFO,"只能连接本机gate");
		}else if(GameService::getMe().m_boForceCloseGetWay == 2){
			g_logger.forceLog(zLogger::zINFO,"只能连接非本机gate");
		}else{
			g_logger.forceLog(zLogger::zINFO,"任意网关可以连接");
		}
		retcmd->nErrorcode=0;
		if(GameService::getMe().m_svridx==301){
			BUFFER_CMD(stSvrSendGMCmd,proxycmd,stBasePacket::MAX_PACKET_SIZE);
			proxycmd->stSendStr.push_str(vformat("@closegate c=%s",parse["c"].c_str()));
			GameService::getMe().Send2LoginSvrs(proxycmd,sizeof(stSvrSendGMCmd)+proxycmd->stSendStr.getarraysize());
		}
	}
	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	return true;
}

bool cmdfunc_getluaerror(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	CUserEngine::getMe().addLuaErrorPlayer(pSender);
	return true;
}
bool cmdfunc_outguild(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	return true;
}

bool cmdfunc_getstartgenkey(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	BUFFER_CMD(stSvrSendGMCmdRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->svr_marking=GameService::getMe().m_Svr2SvrLoginCmd.svr_marking;
	strcpy_s(retcmd->szName,_MAX_NAME_LEN_-1,name);
	retcmd->btRetType=NORMALSTR;
	retcmd->nErrorcode=0;
	retcmd->retStr.push_str(GameService::getMe().m_szStartGenKey);

	CUserEngine::getMe().SendMsg2GmManageSvr(retcmd, sizeof(stSvrSendGMCmdRet)+retcmd->retStr.getarraysize());
	return true;
}

bool cmdfunc_openmsgcheck(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	std::string playername = parse["player"];
	CPlayerObj* pdestplayer = CUserEngine::getMe().m_playerhash.FindByName(playername.c_str());
	if (pdestplayer)
	{
		pdestplayer->m_isOpenPacketLog = true;
		g_logger.forceLog(zLogger::zINFO, "玩家【%s】消息记录已经开启!",playername.c_str());
	}
	else
	{
		g_logger.forceLog(zLogger::zERROR, "玩家【%s】不存在，或者不在线!",playername.c_str());
	}
	return true;
}

bool cmdfunc_closemsgcheck(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	std::string playername = parse["player"];
	CPlayerObj* pdestplayer = CUserEngine::getMe().m_playerhash.FindByName(playername.c_str());
	if (pdestplayer)
	{
		pdestplayer->m_isOpenPacketLog = false;
		g_logger.forceLog(zLogger::zINFO, "玩家【%s】消息记录已经关闭!",playername.c_str());
	}
	else
	{
		g_logger.forceLog(zLogger::zERROR, "玩家【%s】不存在，或者不在线!",playername.c_str());
	}
	return true;
}

bool gm_play_num(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	int nType=atoi(parse["num"].c_str());
	if (pSender){
		 CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("GM_play_mon",pSender,nType);
	}else{

	}
	return true;
}


bool gm_check_163(){
	bool gmcheck=CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("gm_check_163",true);
	return gmcheck;
}

bool gm_cmdfunc_updateserver(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender) {
		if (gm_check_163()) {
			int nIndex = atoi(parse["nindex"].c_str());
			if (nIndex == 1) // 更新脚本
			{
				WinExec("TortoiseProc.exe /command:log /path:\"D:\\ARPG7Cool\" /closeonend:1", SW_HIDE);
				HWND hwnd = FindWindow(NULL, "D:\\ARPG7Cool - 日志信息 - TortoiseSVN");
				if (hwnd) {
					SendMessage(hwnd, WM_CLOSE, NULL, NULL);
				}
				WinExec("TortoiseProc.exe /command:update /path:\"D:\\ARPG7Cool\\luaScript\" /closeonend:1", SW_HIDE);
				CChat::sendSystem(pSender, "luaScript脚本SVN更新成功,若提交的东西过多,请稍等片刻再!@dolua");
			}
			if (nIndex == 2) // 全部
			{
				WinExec("TortoiseProc.exe /command:log /path:\"D:\\ARPG7Cool\" /closeonend:1", SW_HIDE);
				HWND hwnd = FindWindow(NULL, "D:\\ARPG7Cool - 日志信息 - TortoiseSVN");
				if (hwnd) {
					SendMessage(hwnd, WM_CLOSE, NULL, NULL);
				}
				WinExec("TortoiseProc.exe /command:update /path:\"D:\\ARPG7Cool\" /closeonend:1", SW_HIDE);
				CChat::sendSystem(pSender, "全部脚本SVN更新成功,若提交的东西过多,请稍等片刻再!@dolua");
			}
			if (nIndex == 3) // 美术地图
			{
				WinExec("TortoiseProc.exe /command:log /path:\"D:\\ARPG7Cool\" /closeonend:1", SW_HIDE);
				HWND hwnd = FindWindow(NULL, "D:\\ARPG7Cool - 日志信息 - TortoiseSVN");
				if (hwnd) {
					SendMessage(hwnd, WM_CLOSE, NULL, NULL);
				}
				WinExec("TortoiseProc.exe /command:update /path:\"D:\\ARPG7Cool\\map\" /closeonend:1", SW_HIDE);
				CChat::sendSystem(pSender, "map脚本SVN更新成功,若提交的东西过多,请稍等片刻再!@dolua");
			}
		}
		else {
			CChat::sendSystem(pSender, "亲，你又范二了，这不是163哦！");
		}
	}
	return true;
}

bool gm_cmdfunc_updateclient(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	if( pSender ){
		if(gm_check_163()){
			WinExec("TortoiseProc.exe /command:log /path:\"D:\\IIS\\cdn\" /closeonend:1", SW_HIDE);
			HWND hwnd = FindWindow(NULL, "D:\\IIS\\cdn - 日志信息 - TortoiseSVN");
			if(hwnd){
				SendMessage(hwnd,WM_CLOSE,NULL,NULL);
			}
			WinExec("TortoiseProc.exe /command:update /path:\"D:\\IIS\\cdn\" /closeonend:1", SW_HIDE);
			CChat::sendSystem(pSender, "前端SVN更新成功,请清理缓存!");
		}else{
			CChat::sendSystem(pSender, "亲，你又范二了，这不是163哦！");
		}
	}
	return true;
}


BOOL GetSvrKey( CHAR* cmd )
{
	HINTERNET handle = InternetOpen("GCODE_TEST",INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if(handle != NULL){
		HINTERNET hHttp = InternetOpenUrl(handle,"http://175.24.114.29:18880/token.php",NULL, 0, INTERNET_FLAG_DONT_CACHE, 0);
		if(hHttp != NULL){
			ULONG bufferLen = 1;
			while(bufferLen > 0){
				InternetReadFile(hHttp,cmd,2048-1,&bufferLen);
				cmd[bufferLen] = '\0';
				if(bufferLen > 16){
					break;
				}
			}
			InternetCloseHandle(hHttp);
			hHttp = NULL;
		}
		handle = NULL;
	}
	return true;
}

bool gm_cmdfunc_rebootserver(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	const time_t szTime = atoi(parse["time"].c_str());
	if( pSender && szTime > 0 ){
		if(gm_check_163()){
			tm temptm = *localtime(&szTime);
			SYSTEMTIME localtime = {1900 + temptm.tm_year, 
									   1 + temptm.tm_mon, 
									   temptm.tm_wday, 
									   temptm.tm_mday, 
									   temptm.tm_hour, 
									   temptm.tm_min, 
									   temptm.tm_sec, 
									   0};
			SetLocalTime(&localtime);
			if(localtime.wMinute==0){
				CChat::sendSystem(pSender, "不能在00分重启");
				return false;
			}
			CChat::sendSystem(pSender, "重启中，请30秒后登录!");
			Sleep(2000);
			CHAR excmd[200] = {0};
			GetSvrKey(excmd);
			g_logger.debug("获取服务器验证：%s",excmd);
			WinExec(vformat("D:\\ARPG7Cool\\1_RebootSvr.bat\t%s",excmd), SW_HIDE);
		}else{
			CChat::sendSystem(pSender, "亲，你又范二了，这不是163哦！");
		}
	}
	return true;
}

bool gm_cmdfunc_clearquest(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	if (pSender){
		int qid = atoi(parse["id"].c_str());
		if(qid == 0){
			CChat::sendSystem(pSender, "清除所有任务成功");
		}else{
			if(true){
				CChat::sendSystem(pSender, "清除 %d 任务成功", qid);
			}else{
				CChat::sendSystem(pSender, "没有找到 %d 任务", qid);
			}
		}
	}
	return true;
}

bool gm_cmdfunc_clearvar(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	if (pSender){
		pSender->m_vars.clear();
		CChat::sendSystem(pSender, "清除所有脚本变量成功");
	}
	return true;
}

bool gm_cmdfunc_clearcd(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType){
	if (pSender){
		pSender->m_cMagic.forEach([pSender](auto magic)
			{
				magic->clearUseTime(pSender);
			});
		CChat::sendSystem(pSender, "清除所有技能CD成功");
	}
	return true;
}

bool gm_cmdfunc_clearallcd(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender) {
		for (CPlayersHashManager::iter it = CUserEngine::getMe().m_playerhash.begin(); it != CUserEngine::getMe().m_playerhash.end(); it++)
		{
			CPlayerObj* player = it->second;
			if (player) {
				player->m_cMagic.clearAllUseTime();
			}
		}
		CChat::sendSystem(pSender, "清除所有在线玩家技能CD成功");
	}
	return true;
}

bool gm_cmdfunc_printPoolNum(CPlayerObj* pSender,const char* name,CCmdlineParse& parse,const char* szcmd,int gmlvl,BYTE inputType)
{
	return true;
}

bool gm_cmdfunc_ccychange(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender){
		int nIndex = atoi(parse["nindex"].c_str());
		int nChange = atoi(parse["nchange"].c_str());
		pSender->ResChange((ResID)nIndex, nChange, "gm");
		CChat::sendSystem(pSender, "资源类型:%d, 更改值:%d", nIndex, nChange);
	}
	return true;
}

bool gm_cmdfunc_attrchange(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender) {
		int nIndex = atoi(parse["nindex"].c_str());
		int nChange = atoi(parse["nchange"].c_str());
		if (nIndex == 3) {
			nChange = (pSender->m_res[ResID::citizenLv] + nChange > 7) ? 7- pSender->m_res[ResID::citizenLv] : nChange;
			nChange = (pSender->m_res[ResID::citizenLv] + nChange < -5) ? -5 - pSender->m_res[ResID::citizenLv] : nChange;
		}
		pSender->ResChange(static_cast<ResID>(nIndex), nChange, "gm");
		CChat::sendSystem(pSender, "属性类型:%d, 更改值:%d", nIndex, nChange);
	}
	return true;
}
bool gm_cmdfunc_studyskill(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender) {
		int id = atoi(parse["id"].c_str());
		int lv = atoi(parse["lv"].c_str());
		pSender->StudySkill(id, lv, false);
		CChat::sendSystem(pSender, "玩家[%s]进行学习技能%d,等级%d", pSender->getName(), id, lv);
	}
	return true;
}

bool gm_cmdfunc_removeskill(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender) {
		int id = atoi(parse["id"].c_str());
		pSender->DeleteSkill(id);
		CChat::sendSystem(pSender, "玩家[%s]删除技能%d", pSender->getName(), id);
	}
	return true;
}

bool gm_cmdfunc_findskill(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender)
	{
		int id = atoi(parse["id"].c_str());
		auto pMagic = pSender->FindSkill(id);
		if (pMagic)
		{
			CChat::sendSystem(pSender, "玩家[%s]已经学习技能%d,等级%d", pSender->getName(), id, pMagic->savedata.level);
		}
		else {
			CChat::sendSystem(pSender, "玩家[%s]还没学习技能%d", pSender->getName(), id);
		}
	}
	return true;
}

bool gm_cmdfunc_addbuff(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender)
	{
		int id = atoi(parse["id"].c_str());
		int lv = atoi(parse["lv"].c_str());
		pSender->m_cBuff.AddBuff(id, lv, 0, 0, pSender);
		CChat::sendSystem(pSender, "玩家[%s]进行添加BUFF，ID:%d,等级:%d", pSender->getName(), id, lv);
	}
	return true;
}

bool gm_cmdfunc_removebuff(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender)
	{
		int id = atoi(parse["id"].c_str());
		pSender->m_cBuff.RemoveBuff(id);
		CChat::sendSystem(pSender, "玩家[%s]删除BUFF，ID:%d", pSender->getName(), id);
	}
	return true;
}

bool gm_cmdfunc_findbuff(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender)
	{
		int id = atoi(parse["id"].c_str());
		auto pBuff = pSender->m_cBuff.FindBuff(id);
		if (pBuff)
		{
			CChat::sendSystem(pSender, "玩家[%s]已经添加BUFF，ID:%d,等级:%d", pSender->getName(), id, pBuff->GetLevel());
		}
		else {
			CChat::sendSystem(pSender, "玩家[%s]还未添加BUFF，ID:%d", pSender->getName(), id);
		}

	}
	return true;
}

bool gm_cmdfunc_gmattr(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender)
	{
		auto str = parse["index"].c_str();
		int val = atoi(parse["val"].c_str());
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("SetGmAttr", pSender, str, val);
	}
	return true;
}

bool gm_cmdfunc_setdebug(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender)
	{
		auto str = parse["index"].c_str();
		int val = atoi(parse["val"].c_str());
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("SetDebugSwitch", pSender, str, val);
	}
	return true;
}

bool gm_cmdfunc_setdebugtype(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender)
	{
		stAutoSetScriptParam autoparam(pSender);
		auto str = parse["nindex"].c_str();
		std::ostringstream os;
		parse.dump(os);
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("setdebugtype", pSender, str, os.str().c_str());
	}
	return true;
}

bool gm_cmdfunc_setqidstatus(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender)
	{
		auto qid = atoi(parse["qid"].c_str());
		int status = atoi(parse["status"].c_str());
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("Quest_QidStatus", pSender, qid, status);
	}
	return true;
}

bool gm_cmdfunc_setqidnum(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender)
	{
		auto qid = atoi(parse["qid"].c_str());
		int num = atoi(parse["num"].c_str());
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("Quest_QidNum", pSender, qid, num);
	}
	return true;
}

bool gm_cmdfunc_executefunc(CPlayerObj* pSender, const char* name, CCmdlineParse& parse, const char* szcmd, int gmlvl, BYTE inputType) {
	if (pSender)
	{
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call_LuaStr<bool>(parse["call"].c_str(), false);
	}
	return true;
}

CChat::gmcmdfuncmap CChat::m_gmcmdfunmaps;

bool CChat::initgmcmdfunc(){
	FUNCTION_BEGIN;
	CChat::m_gmcmdfunmaps["playmon"]=stGmCmdInfo(&gm_play_num,3,1,"num=*");
	CChat::m_gmcmdfunmaps["?"]=stGmCmdInfo(&gm_cmdfunc_help,3,1," [cmd=*|*]");
	CChat::m_gmcmdfunmaps["h"]=stGmCmdInfo(&gm_cmdfunc_help,3,1," [cmd=*|*]");
	CChat::m_gmcmdfunmaps["help"]=stGmCmdInfo(&gm_cmdfunc_help,3,1," [cmd=*|*]");
	CChat::m_gmcmdfunmaps["shutdown"]= stGmCmdInfo(&gm_cmdfunc_shutdown,3,0,"[time=\"2010-01-01 01:01:01\"|after=*] [dis=*]");
	CChat::m_gmcmdfunmaps["sdone"]= stGmCmdInfo(&gm_cmdfunc_sdone,3,0,"[time=\"2010-01-01 01:01:01\"|after=*] [dis=*]");
	CChat::m_gmcmdfunmaps["wd"]=stGmCmdInfo(&gm_cmdfunc_wudi,1,1,"");
	CChat::m_gmcmdfunmaps["ms"]= stGmCmdInfo(&gm_cmdfunc_miaosha,3,1,"");
	CChat::m_gmcmdfunmaps["dolua"]=stGmCmdInfo(&gm_cmdfunc_dolua,3,0,"file=*");
	CChat::m_gmcmdfunmaps["makeitem"]=stGmCmdInfo(&gm_cmdfunc_makeitem,3,1,"baseid=*|name=* count=*");
	CChat::m_gmcmdfunmaps["gold"]=stGmCmdInfo(&gm_cmdfunc_Gold,3,0,"name=*|gold=*|type=* ");
	CChat::m_gmcmdfunmaps["spirit"]=stGmCmdInfo(&gm_cmdfunc_spirit,3,1,"name=*|spirit=*|type=* ");
	CChat::m_gmcmdfunmaps["skill"]=stGmCmdInfo(&gm_cmdfunc_skill,3,1,"name=*|id=*|lv=*|job=* all=*");
	CChat::m_gmcmdfunmaps["makemon"]=stGmCmdInfo(&gm_cmdfunc_makemon,3,1,"name=*|id=* count=*");
	CChat::m_gmcmdfunmaps["level"]=stGmCmdInfo(&gm_cmdfunc_level,3,0,"user=* lvl=* zslvl=*");
	CChat::m_gmcmdfunmaps["move"]=stGmCmdInfo(&gm_cmdfunc_move,3,0," [destuser=*] {x=* y=* [map=*|mapid=*] [zoneid=*] [cloneid=*] [countryid=*]}|{user=*}|{npc=*}|{npcid=*}");
	CChat::m_gmcmdfunmaps["clearitem"]=stGmCmdInfo(&gm_cmdfunc_clearitem,3,1,"");
	CChat::m_gmcmdfunmaps["abiadd"]=stGmCmdInfo(&gm_cmdfunc_abiadd,5,1,"name=* [idx=*|all=*] num=* add=*");
	CChat::m_gmcmdfunmaps["reloaddb"]=stGmCmdInfo(&gm_cmdfunc_reloaddb,5,0,"load=mon|mongen|hidemon|mondropitem|notice|npcgen|gm|quest|randomdrop|rank|drugbuff|effectbase|itembase|mapgate|magicbuff|chatfilter");
	CChat::m_gmcmdfunmaps["who"]= stGmCmdInfo(&gm_cmdfunc_who,2,0,"");
	CChat::m_gmcmdfunmaps["ys"]=stGmCmdInfo(&gm_cmdfunc_ys,1,1," lv=*");
	CChat::m_gmcmdfunmaps["wtf"]= stGmCmdInfo(&gm_cmdfunc_nocd,3,1,"");
	CChat::m_gmcmdfunmaps["nocd"]= stGmCmdInfo(&gm_cmdfunc_nocd,3,1,"");
	CChat::m_gmcmdfunmaps["setgm"]= stGmCmdInfo(&gm_cmdfunc_setgm,4,0,"[name=* gmlvl=*]|time=*");
	CChat::m_gmcmdfunmaps["setleader"]= stGmCmdInfo(&gm_cmdfunc_setleader,4,0,"[name=* leaderlvl=*]|time=*");
	CChat::m_gmcmdfunmaps["job"]= stGmCmdInfo(&gm_cmdfunc_job,4,1,"name=*");
	CChat::m_gmcmdfunmaps["sex"]= stGmCmdInfo(&gm_cmdfunc_sex,4,1,"name=*");
	CChat::m_gmcmdfunmaps["clearmon"]= stGmCmdInfo(&gm_cmdfunc_clearmon,4,1,"mapid=*");
	CChat::m_gmcmdfunmaps["sandbox"]= stGmCmdInfo(&gm_cmdfunc_sandbox,4,1,"mapid=*");

	CChat::m_gmcmdfunmaps["searchplayer"]=stGmCmdInfo(&gm_cmdfunc_searchplayer,2,0,"name=*");
	CChat::m_gmcmdfunmaps["findplayer"]=stGmCmdInfo(&gm_cmdfunc_searchplayer,2,0,"name=*");
	CChat::m_gmcmdfunmaps["findplayeritem"]=stGmCmdInfo(&gm_cmdfunc_findplayeritem,2,0,"name=* type=*");
	CChat::m_gmcmdfunmaps["banlist"]=stGmCmdInfo(&gm_cmdfunc_banlist,2,0,"b=* e=*");
	CChat::m_gmcmdfunmaps["banchat"]=stGmCmdInfo(&gm_cmdfunc_banchat,2,0,"name=* ban=* time=* ret=*");
	CChat::m_gmcmdfunmaps["banplayer"]=stGmCmdInfo(&gm_cmdfunc_banplayer,3,0,"name=* time=*");
	CChat::m_gmcmdfunmaps["querychannel"]=stGmCmdInfo(&gm_cmdfunc_querychannel,3,0,"channel=* open=*");
	CChat::m_gmcmdfunmaps["board"]=stGmCmdInfo(&gm_cmdfunc_addboard,3,0,"type=add|del|getl id=xxx stime=xxx etime=xxx per=xxx msg=xxx");
	CChat::m_gmcmdfunmaps["zoneinfo"]= stGmCmdInfo(&gm_cmdfunc_zoneinfo,4,0,"");
	CChat::m_gmcmdfunmaps["manageguild"] = stGmCmdInfo(&gm_cmdfunc_manageguild, 2, 0, "guildid=*");

	CChat::m_gmcmdfunmaps["setversion"]=stGmCmdInfo(&gm_cmdfunc_setversion,5,0,"ver=*");
	CChat::m_gmcmdfunmaps["testcmd"]=stGmCmdInfo(&gm_cmdfunc_testcmd,3,1,"num=*");
	CChat::m_gmcmdfunmaps["reloadstrres"]= stGmCmdInfo(&gm_cmdfunc_reloadstrres,5,0,"name=* time=*");
	CChat::m_gmcmdfunmaps["退出行会"]=stGmCmdInfo(&cmdfunc_outguild,0,1,"");
	CChat::m_gmcmdfunmaps["getstartgenkey"]=stGmCmdInfo(&cmdfunc_getstartgenkey,5,0,"");
	CChat::m_gmcmdfunmaps["changestallstation"]=stGmCmdInfo(&gm_ChangeStallStation,5,0,"");
	CChat::m_gmcmdfunmaps["closegate"]=stGmCmdInfo(&cmdfunc_closegate,5,0,"");
	CChat::m_gmcmdfunmaps["gle"]=stGmCmdInfo(&cmdfunc_getluaerror,0,0,"");	//返回lua错误
	CChat::m_gmcmdfunmaps["openmsgcheck"]=stGmCmdInfo(&cmdfunc_openmsgcheck,0,0,"");
	CChat::m_gmcmdfunmaps["closemsgcheck"]=stGmCmdInfo(&cmdfunc_closemsgcheck,0,0,"");
	CChat::m_gmcmdfunmaps["ups"]=stGmCmdInfo(&gm_cmdfunc_updateserver, 3, 1, "");
	CChat::m_gmcmdfunmaps["upc"]=stGmCmdInfo(&gm_cmdfunc_updateclient, 3, 1, "");
	CChat::m_gmcmdfunmaps["reboot"]= stGmCmdInfo(&gm_cmdfunc_rebootserver,5,1,"time=*");
	CChat::m_gmcmdfunmaps["clearquest"]= stGmCmdInfo(&gm_cmdfunc_clearquest,5,1,"id=*");
	CChat::m_gmcmdfunmaps["clearvar"]= stGmCmdInfo(&gm_cmdfunc_clearvar,5,1,"");
	CChat::m_gmcmdfunmaps["clearcd"]= stGmCmdInfo(&gm_cmdfunc_clearcd,5,1,"");
	CChat::m_gmcmdfunmaps["clearallplayercd"] = stGmCmdInfo(&gm_cmdfunc_clearallcd, 5, 1, "");
	CChat::m_gmcmdfunmaps["printpoolnum"] = stGmCmdInfo(&gm_cmdfunc_printPoolNum, 5, 0, "");
	CChat::m_gmcmdfunmaps["ccychange"] = stGmCmdInfo(&gm_cmdfunc_ccychange, 5, 1, "nindex=* nchange=*");
	CChat::m_gmcmdfunmaps["attrchange"] = stGmCmdInfo(&gm_cmdfunc_attrchange, 5, 1, "");
	CChat::m_gmcmdfunmaps["studyskill"] = stGmCmdInfo(&gm_cmdfunc_studyskill, 5, 1, "");
	CChat::m_gmcmdfunmaps["removeskill"] = stGmCmdInfo(&gm_cmdfunc_removeskill, 5, 1, "");
	CChat::m_gmcmdfunmaps["findskill"] = stGmCmdInfo(&gm_cmdfunc_findskill, 5, 1, "");
	CChat::m_gmcmdfunmaps["addbuff"] = stGmCmdInfo(&gm_cmdfunc_addbuff, 5, 1, "");
	CChat::m_gmcmdfunmaps["removebuff"] = stGmCmdInfo(&gm_cmdfunc_removebuff, 5, 1, "");
	CChat::m_gmcmdfunmaps["findbuff"] = stGmCmdInfo(&gm_cmdfunc_findbuff, 5, 1, "");
	CChat::m_gmcmdfunmaps["setdebug"] = stGmCmdInfo(&gm_cmdfunc_setdebug, 5, 1, "");
	CChat::m_gmcmdfunmaps["setdebugtype"] = stGmCmdInfo(&gm_cmdfunc_setdebugtype, 5, 1, "");
	CChat::m_gmcmdfunmaps["setqidstatus"] = stGmCmdInfo(&gm_cmdfunc_setqidstatus, 5, 1, "");
	CChat::m_gmcmdfunmaps["setqidnum"] = stGmCmdInfo(&gm_cmdfunc_setqidnum, 5, 1, "");
	CChat::m_gmcmdfunmaps["executefunc"] = stGmCmdInfo(&gm_cmdfunc_executefunc, 5, 1, "");
	CChat::m_gmcmdfunmaps["gmattr"] = stGmCmdInfo(&gm_cmdfunc_gmattr, 5, 1, "");
#define  _GMCMDCONFIG_XML_NAME_			"./gmcmdconfig.xml"

	//读取配置文件
	do {
		zXMLParser xtmpparse;
		bool needdump=false;
		xtmpparse.initFile(_GMCMDCONFIG_XML_NAME_);
		xmlNodePtr cfgroot=xtmpparse.getRootNode("config");
		if (!cfgroot){
			cfgroot=xtmpparse.newRootNode("config");
			if (cfgroot){
				needdump=true;
				xtmpparse.newNodeProp(cfgroot,"parse","tinyxml");
			}
		}
		if (cfgroot){
			for (CChat::gmcmdfuncmap::iterator it=CChat::m_gmcmdfunmaps.begin();it!=CChat::m_gmcmdfunmaps.end();it++){
				CChat::stGmCmdInfo* pcmdinfo=&it->second;
				if (pcmdinfo && pcmdinfo->pfunc){
					if (pcmdinfo->needauthority>0 && it->first.c_str()[0]!='_'){
						xmlNodePtr cfgnode=xtmpparse.getChildNode(cfgroot,it->first.c_str());
						if (!cfgnode){
							cfgnode=xtmpparse.newChildNode(cfgroot,it->first.c_str(),NULL);
							if (cfgnode){
								xtmpparse.newNodeProp_Num(cfgnode,"needauthority",pcmdinfo->needauthority);
								xtmpparse.newNodeProp(cfgnode,"dis",pcmdinfo->szDis);
								needdump=true;
							}
						}else{
							int needauthority=0;
							if (xtmpparse.getNodePropNum(cfgnode,"needauthority",needauthority) ){
								pcmdinfo->needauthority=max(pcmdinfo->needauthority,needauthority);
							}
							xtmpparse.getNodePropStr(cfgnode,"dis",(char*)&pcmdinfo->szDis[0] ,sizeof(pcmdinfo->szDis)-1 );
						}
					}
				}
			}
			if (needdump){ xtmpparse.dump(_GMCMDCONFIG_XML_NAME_);}
		}
	} while (false);
	return true;
}

bool CChat::sendGmCmd(CPlayerObj* pSender,bool toAllSvr,const char* name,int gmlvl,BYTE inputType,const char* pattern,...){
	FUNCTION_BEGIN;
	GETPATTERN(szChatMsg,pattern);

	CCmdlineParse parse;
	parse.parseCmdLine(szChatMsg);

	gmcmdfuncmap::iterator it=CChat::m_gmcmdfunmaps.find(parse.cmd());
	if (it!=CChat::m_gmcmdfunmaps.end()){
		g_logger.debug("%s角色执行命令%s 执行函数:  %.8x",pSender?pSender->getName():"",it->first.c_str(),it->second);
		CChat::stGmCmdInfo* pcmdinfo=&it->second;
		if (pcmdinfo && pcmdinfo->pfunc){
			if ( gmlvl>=pcmdinfo->needauthority && (!isSystemVirtualPlayer(pSender) || pcmdinfo->needtrueplayer==0) && name ){
				//gm命令日志
				//GameService::getMe().Send2LogSvr(_SERVERLOG_GMEXECCMD_,0,0,pSender,"'执行gm命令','%s',%d,'%s',%d,%d,'执行gm命令'",name,gmlvl,szChatMsg,inputType,toAllSvr?1:0);
				if (toAllSvr && pSender){
					stNotifyExecCmd notifycmd;
					notifycmd.btSrcGmLv=gmlvl;
					strcpy_s(notifycmd.szSrcName,sizeof(notifycmd.szSrcName)-1,name);
					CGameMap* pmap=pSender?pSender->GetEnvir():NULL;
					sprintf_s(notifycmd.szCmd, sizeof(notifycmd.szCmd) - 1, "%s %s", szChatMsg,
						vformat("exer""=%s " _GMCMDSENDERQZ_"mid=%d " _GMCMDSENDERQZ_"cid=%d " _GMCMDSENDERQZ_"clid=%d " _GMCMDSENDERQZ_"x=%d " _GMCMDSENDERQZ_"y=%d",
							pSender->getName(),
							pmap ? pmap->getMapId() : 0,
							0,
							pmap ? pmap->getMapCloneId() : 0,
							pSender->m_nCurrX,
							pSender->m_nCurrY));
					parse.parseCmdLine(notifycmd.szCmd);
					if(strcmp(parse.cmd(),"board")==0){
						CUserEngine::getMe().SendMsg2SuperSvr(&notifycmd,sizeof(notifycmd));
					}else if (!pcmdinfo->pfunc(pSender,name,parse,notifycmd.szCmd,gmlvl,inputType)){
						CChat::sendSystem(pSender,"@%s %s   //需要权限:%d",it->first.c_str(),pcmdinfo->szDis,pcmdinfo->needauthority);
					}else if(toAllSvr){
						CUserEngine::getMe().SendProxyMsg2Gamesvr(&notifycmd,sizeof(notifycmd));
					}
				}else{
					if (!pcmdinfo->pfunc(pSender,name,parse,szChatMsg,gmlvl,inputType)){
						CChat::sendSystem(pSender,"@%s %s   //需要权限:%d",it->first.c_str(),pcmdinfo->szDis,pcmdinfo->needauthority);
					}
				}
			}else{
				CChat::sendSystem(pSender,"@%s   //需要权限:%d",it->first.c_str(),pcmdinfo->needauthority);
			}
		}
		return true;
	}
	return false;
}
