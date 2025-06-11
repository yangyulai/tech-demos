#include "Script.h"
#include <register_lua.hpp>
#include "BaseCreature.h"
#include "UsrEngn.h"
#include "Chat.h"
#include "MonsterObj.h"
#include <winapiini.h>
#include "Config.h"
#include "JsonConfig.h"
#include "timeMonitor.h"
#include "../gamesvr.h"
#include <EnumExporter.h>
#include "MapMagicEvent.h"
#include "MapItemEvent.h"

stAutoSetScriptParam::stAutoSetScriptParam(CCreature* player,CNpcObj* pnpc){
	m_poldplayer=scriptcurrentuser;
	m_poldsetplayer=lua_setscriptcurrentuser;
	scriptcurrentuser=player;
	lua_setscriptcurrentuser=player;
}

stAutoSetScriptParam::~stAutoSetScriptParam(){
	scriptcurrentuser=m_poldplayer;
	lua_setscriptcurrentuser=m_poldsetplayer;
}


CScriptSystem::CScriptSystem() : m_enumExporter(std::make_unique<EnumExporter>())
{
	FUNCTION_BEGIN;
	m_ScriptTick = 0;
	m_min = -1;
	m_halfhour = -1;
	m_hour = -1;
	m_day = -1;
	m_tensecond = -1;
	m_LuaVM = NULL;
}

CScriptSystem::~CScriptSystem()
{
	FUNCTION_BEGIN;
	SAFE_DELETE(m_LuaVM);
}

bool CScriptSystem::InitScript(char* pszScriptFileName,DWORD initstate)
{
	FUNCTION_BEGIN;
	if(initstate==eScript_uninit){
		SAFE_DELETE(m_LuaVM);
		return true;
	}else{
		if(m_LuaVM==NULL && initstate!=eScript_init){ initstate=eScript_init; }
		CLuaVM* tmpLuaVM=m_LuaVM;
		m_LuaVM=CLD_DEBUG_NEW CLuaVM;
		if (m_LuaVM){
			Bind(m_LuaVM);
			BindStruct(m_LuaVM);
			BindStructBase(m_LuaVM);
			BindClass(m_LuaVM);
			BindQuest(m_LuaVM);
			BindOther(m_LuaVM);
			BindCreature(m_LuaVM);
			BindActivity(m_LuaVM);
			ExportEnum();
			if (initstate == eScript_init) {
				lua_file_dump();
			}
			if ( !m_LuaVM->DoFile(pszScriptFileName) ){
				g_logger.error("lua 脚本 %s 加载失败!",pszScriptFileName);
				SAFE_DELETE(m_LuaVM);
				m_LuaVM=tmpLuaVM;
				return false;
			}
			SAFE_DELETE(tmpLuaVM);
		}else{
			g_logger.error("lua 脚本 %s 加载失败!",pszScriptFileName);
			m_LuaVM=tmpLuaVM;
			return false;
		}
		return true;
	}
}

void CScriptSystem::run(){
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true,"");
	FUNCTION_MONITOR(64, "");

	if (GetTickCount64()>m_ScriptTick)
	{
		CUserEngine::getMe().m_globaltimer.run();
		if (m_LuaVM)
		{
			time_t scripttime;
			struct tm tm1;
			time(&scripttime);
			localtime_s(&tm1,&scripttime);
			if ((m_min==-1) || (tm1.tm_min!=m_min && tm1.tm_min>m_min) || (tm1.tm_min==0 && m_min==59))
			{
				if (m_min!=-1){m_LuaVM->VCall_LuaStr("MinCall");}
				m_min=tm1.tm_min;	
			}
			if ((m_halfhour==-1) || (tm1.tm_min!=m_halfhour && tm1.tm_min==0) || (tm1.tm_min!=m_halfhour && tm1.tm_min==30))
			{
				if (m_halfhour!=-1){m_LuaVM->VCall_LuaStr("HalfHourCall");}
				m_halfhour=tm1.tm_min;
			}
			if ((m_hour==-1) || (tm1.tm_hour!=m_hour && tm1.tm_hour>m_hour) || (tm1.tm_hour==0 && m_hour==23))
			{
				if (m_hour!=-1){m_LuaVM->VCall_LuaStr("HourCall");}
				m_hour=tm1.tm_hour;
			}
			if ((m_day==-1) || (tm1.tm_yday!=m_day && tm1.tm_yday>m_day) || (tm1.tm_yday==0 && m_day>tm1.tm_yday))
			{
				if (m_day!=-1){m_LuaVM->VCall_LuaStr("DayCall");}
				m_day=tm1.tm_yday;
			}
			if(m_tensecond == -1 || m_ScriptTick >= m_tensecond){
				if (m_tensecond!=-1){m_LuaVM->VCall_LuaStr("TenSecondCall");}
				m_tensecond = m_ScriptTick + 10000;
			}
		}
		m_ScriptTick = GetTickCount64()+500;
	}
}

CScriptTimer::CScriptTimer(bool bPlay)
:m_tTimeBegin(0)
,m_tTimeEnd(0)
,m_TimerStatus(tsStop){
	FUNCTION_BEGIN;
	ZeroMemory(&m_func[0],QUEST_FUNC_LEN);
	m_boDel = false;
	m_checktime = 0;
	m_checknum = 0;
	m_checktype = NOCHECK;
	m_runcount = 0;
	m_maxruncount = 1;
	m_owner=NULL;
	m_quest=NULL;
	if (bPlay){
		Play();
	}
}

CScriptTimer::~CScriptTimer(){
	FUNCTION_BEGIN;
	if (m_TimerStatus != tsStop){
		Stop();
	}
}

time_t CScriptTimer::GetCurrentSeconds(){
	FUNCTION_BEGIN;
	return time(NULL);
}

void CScriptTimer::Play(){
	FUNCTION_BEGIN;
	if (m_TimerStatus == tsStop){
		m_tTimeBegin = GetCurrentSeconds();
	}
	m_TimerStatus = tsRun;
}

void CScriptTimer::Stop(){
	FUNCTION_BEGIN;
	m_tTimeEnd = GetCurrentSeconds();
	m_TimerStatus = tsStop;
}

void CScriptTimer::Pause(){
	FUNCTION_BEGIN;
	m_tTimeEnd = GetCurrentSeconds();
	m_TimerStatus = tsPause;
}

void CScriptTimer::Reset(){
	FUNCTION_BEGIN;
	m_tTimeBegin = GetCurrentSeconds();
	if (m_checktype==CHECKALL){m_checktime = m_tTimeBegin;}
	m_TimerStatus = tsRun;
}

CScriptTimer::TimerStatus CScriptTimer::GetStatus(){
	return m_TimerStatus;
}

DWORD CScriptTimer::GetTime(){
	FUNCTION_BEGIN;
	if (m_TimerStatus != tsRun)
	{
		if (m_tTimeEnd < m_tTimeBegin)
		{
			m_tTimeEnd = m_tTimeBegin;
		}
		return (DWORD)(m_tTimeEnd-m_tTimeBegin);
	}
	else
	{
		return (DWORD)(GetCurrentSeconds()-m_tTimeBegin);
	}
}

void CScriptTimer::SetTime(DWORD sec){
	FUNCTION_BEGIN;
	m_tTimeBegin=m_tTimeBegin-sec;
}

bool CScriptTimer::CheckTime(BYTE checktype){
	FUNCTION_BEGIN;
	time_t tt;
	tm tm1;
	switch (checktype)
	{
	case CHECKALL:
		{
			if (GetTime()>=m_checknum){return true;}
		}break;
	case WEEK:
		{
			if (m_checktime==0 && m_checktype==WEEK)
			{
				tt = GetCurrentSeconds();
				localtime_s(&tm1,&tt);
				if (CheckInterval(m_stTimer.m_intervaltype,tt) && tm1.tm_wday==m_stTimer.m_week && tm1.tm_hour==m_stTimer.m_hours && tm1.tm_min==m_stTimer.m_min && tm1.tm_sec>=m_stTimer.m_sec)
				{
					return true;
				}
			}
		}break;
	case DAY:
		{
			if (m_checktime==0 && m_checktype==DAY)
			{
				tt = GetCurrentSeconds();
				localtime_s(&tm1,&tt);
				if (CheckInterval(m_stTimer.m_intervaltype,tt) && tm1.tm_hour==m_stTimer.m_hours && tm1.tm_min==m_stTimer.m_min && tm1.tm_sec>=m_stTimer.m_sec)
				{
					return true;
				}
			}
		}break;
	case HOURS:
		{
			if (m_checktime==0 && m_checktype==HOURS)
			{
				tt = GetCurrentSeconds();
				localtime_s(&tm1,&tt);
				if (CheckInterval(m_stTimer.m_intervaltype,tt) && tm1.tm_min==m_stTimer.m_min && tm1.tm_sec>=m_stTimer.m_sec)
				{
					return true;
				}
			}
		}break;
	case MINUTES:
		{
			if (m_checktime==0 && m_checktype==MINUTES)
			{
				tt = GetCurrentSeconds();
				localtime_s(&tm1,&tt);
				if (CheckInterval(m_stTimer.m_intervaltype,tt) && tm1.tm_sec>=m_stTimer.m_sec)
				{
					return true;
				}
			}
		}break;
	case SECONDS:
		{
			if (m_checktime==0 && m_checktype==SECONDS)
			{
				tt = GetCurrentSeconds();
				localtime_s(&tm1,&tt);
				if (CheckInterval(m_stTimer.m_intervaltype,tt))
				{
					return true;
				}
			}
		}break;
	}
	return false;
}

bool CScriptTimer::CheckInterval(BYTE intervaltype,time_t nowtime){
	FUNCTION_BEGIN;
	switch (intervaltype)
	{
	case WEEK:
		{
			if ((DWORD)(nowtime)>=(DWORD)(m_stTimer.m_time+m_stTimer.m_interval*m_runcount*7*24*3600)){return true;};
		}break;
	case DAY:
		{
			if ((DWORD)(nowtime)>=(DWORD)(m_stTimer.m_time+m_stTimer.m_interval*m_runcount*24*3600)){return true;};
		}break;
	case HOURS:
		{
			if ((DWORD)(nowtime)>=(DWORD)(m_stTimer.m_time+m_stTimer.m_interval*m_runcount*3600)){return true;};
		}break;
	case MINUTES:
		{
			if ((DWORD)(nowtime)>=(DWORD)(m_stTimer.m_time+m_stTimer.m_interval*m_runcount*60)){return true;};
		}break;
	case SECONDS:
		{
			if ((DWORD)(nowtime)>=(DWORD)(m_stTimer.m_time+m_stTimer.m_interval*m_runcount)){return true;};
		}break; 
	}
	return false;
}

int CScriptTimer::CheckKeep(BYTE keeptype){
	FUNCTION_BEGIN;
	switch (keeptype)
	{
	case WEEK:
		{
			return m_stTimer.m_keeptime*7*24*3600+m_tTimeEnd-GetCurrentSeconds();
		}break;
	case DAY:
		{
			return m_stTimer.m_keeptime*24*3600+m_tTimeEnd-GetCurrentSeconds();
		}break;
	case HOURS:
		{
			return m_stTimer.m_keeptime*3600+m_tTimeEnd-GetCurrentSeconds();
		}break;
	case MINUTES:
		{
			return m_stTimer.m_keeptime*60+m_tTimeEnd-GetCurrentSeconds();
		}break;
	case SECONDS:
		{
			return m_stTimer.m_keeptime+m_tTimeEnd-GetCurrentSeconds();
		}break; 
	}
	return -1;
} 

bool CScriptTimer::CheckEvent(CQuestList* qlist){
	FUNCTION_BEGIN;
	if (CheckTime(m_checktype))
	{
		return true;
	}
	return false;
}

void CScriptTimer::CallEvt(CQuestList* qlist){
	FUNCTION_BEGIN;
	if (m_TimerStatus==tsStop)
	{
		return;
	}
	else if (m_TimerStatus==tsRun)
	{
		if (CheckEvent(qlist) && (m_maxruncount==0 || m_runcount<m_maxruncount))
		{
	 
				CPlayerObj* pTmpPlayer=(CPlayerObj*)m_owner;
				stAutoSetScriptParam autoparam(pTmpPlayer);
				if (m_quest && pTmpPlayer){
 
					if (GetTime()>=m_quest->m_evtvar.timercheck){
						m_quest->m_evtvar.timestatus=TIMENOT;
					}
				}
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(m_func);
			m_runcount++;
			if (m_checktype!=CHECKALL)
			{
				Pause();
			}
			else
			{
				if (m_maxruncount!=0 && m_runcount>=m_maxruncount)
				{
					Stop();
				}
				else
				{
					Reset();
				}
			}
		}
	}
	else if (m_TimerStatus==tsPause)
	{
		if (CheckKeep(m_stTimer.m_keeptype)<=0)
		{
			if (m_maxruncount!=0 && m_runcount>=m_maxruncount)
			{
				Stop();
			}
			else
			{
				Reset();
			}
		}
	}
}

CScriptTimerManager::CScriptTimerManager(CQuestList* mainlist,void* mainer)
:m_mainlist(mainlist),m_owner(mainer){
	FUNCTION_BEGIN;
}

CScriptTimerManager::~CScriptTimerManager(){
	FUNCTION_BEGIN;
	m_owner=NULL;
	m_mainlist=NULL;
	clear();
}

void CScriptTimerManager::settimedate(BYTE settype,DWORD id, DWORD timenum,const char* timefunc,DWORD timedate,DWORD runcount,DWORD maxcount){
	FUNCTION_BEGIN;
	CScriptTimer tmptimer;
	bool btCreate=false;
	time_iter it=_timers.find(id);
	if (it!=_timers.end())
	{
		CScriptTimer* ptmptimer=&it->second;
		strcpy_s(ptmptimer->m_func,QUEST_FUNC_LEN-1,timefunc);
		if (settype == ptmptimer->CHECKALL)
		{
			ptmptimer->m_checktype = settype;
			ptmptimer->m_checktime = timedate;
			ptmptimer->m_checknum = timenum;
		}
		else
		{
			ptmptimer->m_checktype = settype;
			ptmptimer->m_checktime = timedate;
			ptmptimer->m_stTimer.m_intervaltype = settype;
			ptmptimer->m_stTimer.m_interval = timenum;
		}
		//ptmptimer->m_runcount = runcount;
		ptmptimer->m_maxruncount = maxcount;
		ptmptimer->m_owner=m_owner;
		if (m_owner){
			ptmptimer->m_quest=((CPlayerObj*)m_owner)->m_QuestList.find_quest(id);
		}
		ptmptimer->Play();
	}
	else
	{
		strcpy_s(tmptimer.m_func,QUEST_FUNC_LEN-1,timefunc);
		if (settype == tmptimer.CHECKALL)
		{
			tmptimer.m_checktype = settype;
			tmptimer.m_checktime = timedate;
			tmptimer.m_checknum = timenum;
		}
		else
		{
			tmptimer.m_checktype = settype;
			tmptimer.m_checktime = timedate;
			tmptimer.m_stTimer.m_intervaltype = settype;
			tmptimer.m_stTimer.m_interval = timenum;
		}
		tmptimer.m_runcount = runcount;
		tmptimer.m_maxruncount = maxcount;
		tmptimer.m_owner=m_owner;
		if (m_owner){
			tmptimer.m_quest=((CPlayerObj*)m_owner)->m_QuestList.find_quest(id);
		}
		tmptimer.Play();
		btCreate=true;
	}
	if (btCreate){_timers.insert(TIMERMAPS::value_type(id,tmptimer));}
}

void CScriptTimerManager::settime(BYTE settype,DWORD id,stTimer tmpstTimer,const char* timefunc,DWORD runcount,DWORD maxcount){
	FUNCTION_BEGIN;
	CScriptTimer tmptimer;
	bool btCreate=false;
	time_iter it=_timers.find(id);
	tmpstTimer.set();
	if (it!=_timers.end())
	{
		CScriptTimer* ptmptimer=&it->second;
		strcpy_s(ptmptimer->m_func,QUEST_FUNC_LEN-1,timefunc);
		ptmptimer->m_stTimer = tmpstTimer;
		ptmptimer->m_checktype = settype;
		//ptmptimer->m_runcount = runcount;
		ptmptimer->m_maxruncount = maxcount;
		ptmptimer->m_owner=m_owner;
		if (m_owner){
			ptmptimer->m_quest=((CPlayerObj*)m_owner)->m_QuestList.find_quest(id);
		}
		ptmptimer->Play();
	}
	else
	{
		strcpy_s(tmptimer.m_func,QUEST_FUNC_LEN-1,timefunc);
		tmptimer.m_stTimer = tmpstTimer;
		tmptimer.m_checktype = settype;
		tmptimer.m_runcount = runcount;
		tmptimer.m_maxruncount = maxcount;
		tmptimer.m_owner=m_owner;
		if (m_owner){
			tmptimer.m_quest=((CPlayerObj*)m_owner)->m_QuestList.find_quest(id);
		}
		tmptimer.Play();
		btCreate=true;
	}
	if (btCreate){_timers.insert(TIMERMAPS::value_type(id, tmptimer));}
}

void CScriptTimerManager::clear(){
	FUNCTION_BEGIN;
	if (_timers.size()!=0)
	{
		_timers.clear();
	}
}

void CScriptTimerManager::run(){
	FUNCTION_BEGIN;
	CScriptTimer* tmptimer=NULL;
	time_iter it=_timers.begin();
	time_iter ite=_timers.end();
	time_iter prev=it;
	while (it!=ite)
	{
		++it;
		tmptimer=&prev->second;
		tmptimer->CallEvt(m_mainlist);
		if ((tmptimer->m_boDel) || (tmptimer->m_maxruncount!=0 && tmptimer->m_runcount>=tmptimer->m_maxruncount && tmptimer->GetStatus()==CScriptTimer::tsStop))
		{
			_timers.erase(prev);
		}
		prev=it;
	}
}

CScriptTimer* CScriptTimerManager::find_timer(DWORD id){
	FUNCTION_BEGIN;
	time_iter it=_timers.find(id);
	if (it!=_timers.end())
	{
		return &it->second;
	}
	return NULL;
}

bool CScriptTimerManager::remove_timer(DWORD id){
	FUNCTION_BEGIN;
	time_iter it=_timers.find(id);
	if (it!=_timers.end())
	{
		CScriptTimer* tmptimer=&it->second;
		tmptimer->m_boDel=true;
		return true;
	}
	return false;
}

CScriptSql* globalscriptsql(){//脚本数据库
	return &(CUserEngine::getMe().m_scriptsql);
}

CUserEngine& GetEngine() {//获取引擎
	return CUserEngine::getMe();
}

CPlayerObj* me(){//返回当前用户
	if (lua_setscriptcurrentuser && lua_setscriptcurrentuser->isPlayer()){
		return lua_setscriptcurrentuser->toPlayer();
	}else return NULL;
}

void setme(CCreature* player){
	if (player){
		if (player->isPlayer()){
			lua_setscriptcurrentuser=player;
			if (scriptcurrentuser && scriptcurrentuser->isPlayer() && scriptcurrentuser->toPlayer()->m_pVisitNPC){
				lua_setscriptcurrentuser->toPlayer()->m_pVisitNPC=scriptcurrentuser->toPlayer()->m_pVisitNPC;
			}
		}else{
			lua_setscriptcurrentuser=player;
		}
	}else{
		lua_setscriptcurrentuser=scriptcurrentuser;
	}
}

CCreature* cret(){
	return lua_setscriptcurrentuser;
}

CScriptTimerManager* globaltimer(){//全局计时器
	return &CUserEngine::getMe().m_globaltimer;
}

void LuaLoggerDebug(const char* msg) {
	FUNCTION_BEGIN;
	if (!msg || msg[0] == 0) { return; }
	g_logger.forceLog(zLogger::zDEBUG, UTG(msg));
}

void LuaLoggerInfo(const char* msg){
	FUNCTION_BEGIN;
	if (!msg || msg[0]==0){return;}
	g_logger.forceLog(zLogger::zINFO, UTG(msg));
}

void LuaLoggerWarn(const char* msg) {
	FUNCTION_BEGIN;
	if (!msg || msg[0] == 0) { return; }
	g_logger.forceLog(zLogger::zWARN, UTG(msg));
}

void LuaLoggerError(const char* msg) {
	FUNCTION_BEGIN;
	if (!msg || msg[0] == 0) { return; }
	g_logger.forceLog(zLogger::zERROR, UTG(msg));
}

void LuaLoggerFatal(const char* msg) {
	FUNCTION_BEGIN;
	if (!msg || msg[0] == 0) { return; }
	g_logger.forceLog(zLogger::zFATAL, UTG(msg));
}

void LuaLoggerHidDebug(const char* msg) {
	FUNCTION_BEGIN;
	if (!msg || msg[0] == 0) { return; }
	g_logger.debug(UTG(msg));
}

void LuaLoggerHidInfo(const char* msg) {
	FUNCTION_BEGIN;
	if (!msg || msg[0] == 0) { return; }
	g_logger.info(UTG(msg));
}

void LuaLoggerHidWarn(const char* msg) {
	FUNCTION_BEGIN;
	if (!msg || msg[0] == 0) { return; }
	g_logger.warn(UTG(msg));
}

void LuaLoggerHidError(const char* msg) {
	FUNCTION_BEGIN;
	if (!msg || msg[0] == 0) { return; }
	g_logger.error(UTG(msg));
}

void LuaLoggerHidFatal(const char* msg) {
	FUNCTION_BEGIN;
	if (!msg || msg[0] == 0) { return; }
	g_logger.fatal(UTG(msg));
}
void getpatterninfo(char* str, const char* message) {
	strncpy(str, message, _MAX_CHAT_LEN_ * 4);
	str[_MAX_CHAT_LEN_ * 4 - 1] = '\0';
}
bool LuaSendClientInfo(const char* szName, emChatType ChatType, const char* message)
{
	if (szName)
	{
		szName = UTG(szName);
	}
	if (message)
	{
		message = UTG(message);
	}
	CPlayerObj* pToPlayer = CUserEngine::getMe().m_playerhash.FindByName(szName);
	if (pToPlayer) {
		char szChatMsg[_MAX_CHAT_LEN_ * 4];
		getpatterninfo(szChatMsg, message);
		return CChat::sendSystemByType(pToPlayer, ChatType, szChatMsg);
	}
	else if (szName && szName[0] != 0) {
		char szChatMsg[_MAX_CHAT_LEN_ * 4];
		getpatterninfo(szChatMsg, message);
		BUFFER_CMD(stCretChat, retcmd, stBasePacket::MAX_PACKET_SIZE);
		CChat::SETSENDCMD(retcmd, ChatType, szChatMsg, NULL, NULL, 0);
		CChat::SENDTOSUPER(NULL, szName, 0, ChatType, retcmd);
	}
	return true;
}

bool scriptsys(CPlayerObj* target,CGameMap* pMap,DWORD dwGuildId,BYTE btTags,bool boBanner,const char* msg){//脚本对用户发送消息
	if (!msg || msg[0]==0){return false;}
	msg = UTG(msg);
	if (target && target->m_pGateUser){
		switch (btTags)
		{
			case 0:{return CChat::sendClient(target,msg);}break;//客户端提醒 
			case 1:{return CChat::sendCenterMsg(target->getName(),msg);}break;//发送消息在客户端中间
			case 2:{return CChat::sendSystem(target->getName(),msg);}break;//给用户发送消息，支持所有服务器
			case 3:{return CChat::sendNoticeToUser(target, boBanner,msg);}break;//公告消息
			case 6:{return CChat::sendSimpleMsg(target,msg);}break;//客户端右下角的简单消息
			case 7:{return CChat::sendSimpleMsg2(target,msg);}break;//客户端左下角的简单消息
			case 8:{return CChat::sendClanMsg(target,msg);}break;//公会公告
			case 9:{return CChat::sendSpeaker(target,msg);}break;//喇叭消息
		}
	}else if (pMap){
		switch (btTags)
		{
			case 0:{return CChat::sendMapChat(pMap,boBanner,msg);}break;//本地图聊天
			case 1:{return CChat::sendNoticeToMap(pMap,msg);}break;//公告地图消息
		}
	}else if (dwGuildId){
		switch (btTags)
		{
			case 0:{return CChat::sendGuildToAll(dwGuildId,msg);}break;//公会公告
		}
	}else{
		switch (btTags)
		{
			case 0:{return CChat::sendSystemToAll(false,boBanner,msg);}break;//发送信息给所有服务器的所有玩家,单服
			case 1:{return CChat::sendGmToAll(false,boBanner,target?target->getName():"script",0x7F,msg);}break;//发送gm及时消息给客户端,单服
			case 2:{return CChat::sendSystemToAll(true,boBanner,msg);}break;//发送信息给所有服务器的所有玩家,全服
			case 3:{return CChat::sendGmToAll(true,boBanner,target?target->getName():"script",0x7F,msg);}break;//发送gm及时消息给客户端,全服
			case 4:{return CChat::sendNoticeToAll(false,msg);}break;//公告消息,单服
			case 5:{return CChat::sendNoticeToAll(true,msg);}break;//公告消息,全服
			case 20:{return CChat::sendOperatorMsg(boBanner,msg);}break;
			case 30:{return CChat::sendSimpleMsg2ToAll(false,msg);}break;//公告消息,全服
		}
	}
	return false;
}

CPlayerObj* FindUserByOnlyId(double dOnlyid){//根据玩家唯一ID查找玩家数据的指针
	if (dOnlyid){
		return CUserEngine::getMe().m_playerhash.FindByOnlyId((__int64)dOnlyid);
	}
	return NULL;
}

void FindSuperUserByName(const char* name){
	if (!name || name[0]==0){return;}
	stGameFindPlayerByName retcmd;
	strcpy_s(retcmd.szName, sizeof(retcmd.szName)-1, name);
	CUserEngine::getMe().SendMsg2SuperSvr(&retcmd,sizeof(stGameFindPlayerByName));
}

CPlayerObj* FindUserByAccount(const char* szAccount){
	if (!szAccount || szAccount[0]==0){return NULL;}
	return CUserEngine::getMe().m_playerhash.FindByAccount(szAccount);
}


CPlayerObj* FindUserByName(const char* name){//根据玩家名查找玩家数据的指针
	if (!name || name[0]==0){return NULL;}
	return CUserEngine::getMe().m_playerhash.FindByName(UTG(name));
}

CGameMap* LuaGetMapById(DWORD mapid,DWORD lineid, DWORD cloneId){
	if (!mapid){return NULL;}
	return CUserEngine::getMe().m_maphash.FindById(mapid,lineid, cloneId);
}

CGameMap* GetUserMap(CPlayerObj* target){//得到目标玩家的当前地图指针
	if (!target) return NULL;
	return target->GetEnvir();
}

DWORD GetUserMapId(CPlayerObj* target){//得到目标玩家的当前地图ID
	if (!target) return 0;
	return target->GetEnvir()->getMapId();
}

const char* LuaGetUserMapName(CPlayerObj* target){//得到目标玩家的当前地图名字
	FUNCTION_BEGIN;
	if (!target) return NULL;
	return GTU(target->GetEnvir()->getMapName());
}

bool SetUserCloneMapToOtherSvr(CPlayerObj* target,DWORD mapid,WORD wCloneId,DWORD countryid,DWORD lineid, PosType X=-1, PosType Y=-1,DWORD dwZoneId=0xFFFFFFFF,WORD wTradeid=0,DWORD dwDestSvrId=0){//设置目标玩家所在地图和坐标
	if (!target) return false;
	if (!mapid){return false;}
	if(!target->isPlayer() || target->isRobot())return false;
	CGameMap* tmpmap = CUserEngine::getMe().m_maphash.FindById(mapid,lineid, 0);
	if (tmpmap)
	{
		if(CUserEngine::getMe().isCrossSvr() && GameService::getMe().m_boAllToOne && (dwZoneId==0xFFFFFFFF || dwZoneId==GameService::getMe().m_nZoneid))
		{
			return false;
		}
		if(CUserEngine::getMe().m_shutdowntime>0 && CUserEngine::getMe().isCrossSvr(dwZoneId))
		{
			CChat::sendGmToUser(target,"当前服务器即将维护，暂时无法跨服。");
			return false;
		}
		if (X<0 && Y<0)
		{
			if (!tmpmap->GetRandXYInMap(X,Y))
			{
				return false;
			}
		}
		return target->MoveToMap(mapid, wCloneId, X, Y, dwDestSvrId);
	}
	return false;
}

void timeevttrigger(CQuestList questlist,CQuest* quest){//触发事件
	stQuestEvent* tmpevt=NULL;
	tmpevt = questlist.find_evtbyquest(quest);
	if (tmpevt)
	{
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(tmpevt->szeventfunc);
	}
}

void TestCall(const char* evtfunc){
	if (evtfunc && scriptcurrentuser)
	{
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(evtfunc);
	}
}

int getweekday(){
	time_t day;
	struct tm tm1;
	time(&day);
	localtime_s(&tm1,&day);
	return tm1.tm_wday;
}

char* getweekdaystr(){
	const char* weekdayarr[]={"星期日","星期一","星期二","星期三","星期四","星期五","星期六"}; 
	time_t day;
	struct tm tm1;
	time(&day);
	localtime_s(&tm1,&day);
	return (char*)weekdayarr[tm1.tm_wday];
}

bool LuaDeleteMap(){
	return false;
}

CCreature* LuaFindBoss(CGameMap* pMap,DWORD dwTmpid){
	FUNCTION_BEGIN;
	if (!pMap){return NULL;}
	return pMap->GetCreature(dwTmpid);
}

int LuaDeleteBoss(CCreature* pMon){
	FUNCTION_BEGIN;
	if (pMon){
		pMon->MakeGhost(true,__FUNC_LINE__);
		return 1;
	}
	return 0;
}

int LuaDeleteBossById(CGameMap* pMap,DWORD dwTmpid){
	FUNCTION_BEGIN;
	if (pMap){
		CCreature* pMon=pMap->GetCreature(dwTmpid);
		if (pMon){
			pMon->MakeGhost(true,__FUNC_LINE__);
			return 1;
		}
	}
	return 0;
}

BYTE GetDir(int nSx,int nSy,int nDx,int nDy){
	return CGameMap::GetNextDirection(nSx,nSy,nDx,nDy);
}

const std::string GetPackStr() {
	return CUserEngine::getMe().packstr;
}

const char* GetItemName(DWORD itemid){//得到物品基本名字
	if (auto itembase = sJsonConfig.GetItemDataById(itemid)){
		return itembase->szName.c_str();
	}
	return "";
}

DWORD GetItemMaxCount(DWORD itemid){//得到物品最大重叠数
	return CItem::GetMaxDbCount(itemid);
}

const char* GetMonsterShowName(DWORD monid){
	auto pmondata= sJsonConfig.GetMonsterDataBase(monid);
	if (pmondata){
		_GET_TH_LOOPCHARBUF(1024,true);
		strcpy_s(ptlsbuf,ntlslen-1,pmondata->szName.c_str());
		return filtershowname(ptlsbuf,_MAX_NAME_LEN_-1);
	}
	return "";
}

WORD LuaGetServerId(){
	return GameService::getMe().m_svridx;
}

const char* LuaGetServerZoneName(){
	return GameService::getMe().m_szZoneName;
}

int LuaGetServerZoneId(){
	return GameService::getMe().m_nZoneid;
}

int LuaGetServerTradeId(){
	return GameService::getMe().m_nTradeid;
}

int LuaGetServerTrueZoneId(){
	return GameService::getMe().m_nTrueZoneid;
}
int LuaGetPlayerCnt() {
	return CUserEngine::getMe().m_playerhash.size();
}

sol::table LuaGetPlayerTab(sol::this_state ts) {
	sol::state_view lua(ts);
	auto table = lua.create_table();
	int idx = 1;
	for (auto it = CUserEngine::getMe().m_playerhash.begin(); it != CUserEngine::getMe().m_playerhash.end(); it++) {
		CPlayerObj* player = it->second;
		if (player) {
			table[idx] = player;
			idx++;
		}
	}
	return table;
}

enum emCheckType{
	LV_TYPE,
	EQUIP_TYPE,
	PACKET_TYPE,
};

CCreature* LuaGetNpc(DWORD npcid) { //without map info
	FUNCTION_BEGIN;
	if (npcid) {
//TODO
	}
	return NULL;
}

void LuaAddRmbHistroy(CPlayerObj *player,int num){
	if(player){
		player->m_res[ResID::hischarge] += num;
	}
}

sol::table GetMapAllPlayer(CGameMap* pmap,sol::this_state ts){
	sol::state_view lua(ts);
	auto table = lua.create_table();
	if (pmap){
		int idx=1;
		pmap->ForeachObjectsByType(CRET_PLAYER, [&](MapObject* obj)
			{
				if (CPlayerObj* player = dynamic_cast<CPlayerObj*>(obj)) {
					table[idx] = player;
					idx++;
				}
			});
		return table;
	}
	return table;
}

stARpgAbility g_ARpgAbility;
stARpgAbility* getCreateARpgAbi(){
	return &g_ARpgAbility;
}

bool LuaMonChat(CCreature* pCret,BYTE btType,const char* szMsg){
	FUNCTION_BEGIN;
	if (!szMsg || szMsg[0]==0){return false;}
	//if (btType<0 || btType>1){return false;}
	btType = CHAT_TYPE_MONSTER;
	if (!pCret){return false;}
	return CChat::sendNpcChat(pCret,btType,szMsg);
}

void LuaSendScriptDataToClientAllMap(CGameMap* pEnvir,DWORD dwDataType,const char* szData){
	BUFFER_CMD(stQuestScriptData,sendcmd,stBasePacket::MAX_PACKET_SIZE);
	sendcmd->dwDataType=dwDataType;
	if(szData&&strcmp(szData,"")!=0){
		sendcmd->DataArr.push_str(UTG(szData));
		if (pEnvir){
			pEnvir->SendMsgToMapAllUser(sendcmd,sizeof(*sendcmd)+sendcmd->DataArr.getarraysize());
		}else{
			CUserEngine::getMe().SendMsg2AllUser(sendcmd,sizeof(*sendcmd)+sendcmd->DataArr.getarraysize());
		}
	}

}


bool LuaSendMsgPackDataToClient(CPlayerObj* player, const char* data, int len, uint8_t flag)
{
	if (player && data && data[0] != 0) {
		BUFFER_CMD(stLuaMsgPack, sendcmd, stBasePacket::MAX_PACKET_SIZE);
		sendcmd->flag = flag;
		uint16_t encstart = 2;
		uint16_t encend = 128 > len ? len : 128;
		uint8_t key = (encstart + encend) % 120;

		char* pdata = (char*)data;

		for (int i = encstart; i < encend; i++) {
			pdata[i] = pdata[i] ^ key;
		}
		sendcmd->encstart = encstart;
		sendcmd->encend = encend;
		sendcmd->data.push_back(pdata, len);
		player->SendMsgToMe(sendcmd, sizeof(*sendcmd) + sendcmd->data.getarraysize());
		//DWORD dwCurTime = GetTickCount();
		//g_logger.error("转发命令到网关 包号:%d:%d 时间:%d", sendcmd->cmd, sendcmd->subcmd, dwCurTime);
		return true;
	}
	return false;
}

bool LuaSendMsgPackDataToMap(CGameMap* pEnvir, const char* data, int len, uint8_t flag)
{
	if (pEnvir && data && data[0] != 0) {
		BUFFER_CMD(stLuaMsgPack, sendcmd, stBasePacket::MAX_PACKET_SIZE);
		sendcmd->flag = flag;
		uint16_t encstart = 2;
		uint16_t encend = 128 > len ? len : 128;
		uint8_t key = (encstart + encend) % 120;

		char* pdata = (char*)data;

		for (int i = encstart; i < encend; i++) {
			pdata[i] = pdata[i] ^ key;
		}
		sendcmd->encstart = encstart;
		sendcmd->encend = encend;
		sendcmd->data.push_back(pdata, len);
		pEnvir->SendMsgToMapAllUser(sendcmd, sizeof(*sendcmd) + sendcmd->data.getarraysize());
		return true;
	}
	return false;
}

bool LuaSendMsgPackDataToServer(const char* data, int len, uint8_t flag)
{
	if (data && data[0] != 0) {
		BUFFER_CMD(stLuaMsgPack, sendcmd, stBasePacket::MAX_PACKET_SIZE);
		sendcmd->flag = flag;
		uint16_t encstart = 2;
		uint16_t encend = 128 > len ? len : 128;
		uint8_t key = (encstart + encend) % 120;

		char* pdata = (char*)data;

		for (int i = encstart; i < encend; i++) {
			pdata[i] = pdata[i] ^ key;
		}
		sendcmd->encstart = encstart;
		sendcmd->encend = encend;
		sendcmd->data.push_back(pdata, len);
		CUserEngine::getMe().SendMsg2AllUser(sendcmd, sizeof(*sendcmd) + sendcmd->data.getarraysize());
		return true;
	}
	return false;
}



void LuaSendScriptDataToClient(CPlayerObj* pDstPlayer,DWORD dwDataType,const char* szData){
	BUFFER_CMD(stQuestScriptData,sendcmd,stBasePacket::MAX_PACKET_SIZE);
	sendcmd->dwDataType=dwDataType;
	if(szData&&strcmp(szData,"")!=0){
		sendcmd->DataArr.push_str(UTG(szData));
		if (pDstPlayer){
			pDstPlayer->SendMsgToMe(sendcmd,sizeof(*sendcmd)+sendcmd->DataArr.getarraysize());
		}else{
			CUserEngine::getMe().SendMsg2AllUser(sendcmd,sizeof(*sendcmd)+sendcmd->DataArr.getarraysize());
		}
	}

}

const char* Md5To32Char(const char* md5msg){
	if (md5msg && md5msg[0]){
		MD5_DIGEST md5dst;
		if (MD5String(md5msg,&md5dst)){
			_GET_TH_LOOPCHARBUF(64,true);
			Mem2Hex((char*)&md5dst,sizeof(md5dst),ptlsbuf,ntlslen-1);
			return ptlsbuf;
		}
	}
	return "";
}

bool LuaCreateMapItem(CGameMap* pMap,int nX,int nY,double dOfOnlyId,CItem* pItem,DWORD dwTime){
	FUNCTION_BEGIN;
	if (pMap && pItem){
		CMapItemEvent* pItemEvent=CLD_DEBUG_NEW CMapItemEvent(pItem ,nX, nY);
		pItemEvent->Init(pMap,static_cast<__int64>(dOfOnlyId),0,OWNER_MONSTER);
		if (!pMap->AddItemToMap(nX,nY,pItemEvent)){
			g_logger.error("地面物品添加失败 %s (%d,%d)",pItem->GetItemName(),nX,nY);
			SAFE_DELETE(pItemEvent);
		}
	}
	return false;
}

void lua_reloadstrres(){
 
	CUserEngine::getMe().addDelayLuaCall(reloadRes,0);
}

const char* lua_getstring(int dwIndex, std::string strType){
	return GameService::getMe().GetLuaStrRes(dwIndex,strType);
}

void lua_supertextlog(const char *FileDir,const char *FilePath,const char *InfoLog,const char *Mode){
	if(FileDir && FileDir[0]!=0 && FilePath && FilePath[0]!=0 && InfoLog && InfoLog[0]!=0 && Mode && Mode[0]!=0){
		stSuperTextLog retcmd;
		strcpy_s(retcmd.szFileDir,sizeof(retcmd.szFileDir)-1,FileDir);
		strcpy_s(retcmd.szFilePath,sizeof(retcmd.szFilePath)-1,FilePath);
		strcpy_s(retcmd.szInfoLog,sizeof(retcmd.szInfoLog)-1,InfoLog);
		strcpy_s(retcmd.szMode,sizeof(retcmd.szMode)-1,Mode);
		CUserEngine::getMe().SendMsg2SuperSvr(&retcmd,sizeof(stSuperTextLog));
	}
}

//文件夹删除
void LuaDeleteDirFile(const char* szPath)
{
	if(szPath==NULL || szPath[0]=='\\'){return;} 
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(vformat("%s*.*",szPath),&fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		while (FindNextFile(hFind,&fd))
		{
			if (fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if (strcmp(fd.cFileName,".")!=0 && strcmp(fd.cFileName,"..")!=0)
				{
					LuaDeleteDirFile(vformat("%s%s\\",szPath,fd.cFileName));
				}
			}
			else
			{
				DeleteFile(vformat("%s%s",szPath,fd.cFileName));
			}
		}
		FindClose(hFind);
	}
	RemoveDirectory(szPath);
}

bool LuaAddLog(int nLogType,int nSubLogType,int execType,CPlayerObj* pPlayer,const char* fmtlogstr)//脚本写入日志
{
	return GameService::getMe().Send2LogSvr(nLogType,nSubLogType,execType,pPlayer,fmtlogstr);
}

int getTrueZoneid(){
	return GameService::getMe().m_nTrueZoneid;
}

int getMonCount(CGameMap* map,DWORD dwMonid)
{
	if (map)
	{
		return map->GetMonCountById(dwMonid);
	}
	return 0;
}

stItemDataBase* LuaGetItemDB(int dwBaseId){
	if (auto itemDataBase = sJsonConfig.GetItemDataById(dwBaseId))
	{
		return itemDataBase.get();
	}
	return nullptr;
}

bool isAllToOne()
{
	return GameService::getMe().m_boAllToOne;
}

DWORD getGameSvrVersion()
{
	return CUserEngine::getMe().m_dwThisGameSvrVersion.load();
}

void setGameSvrVersion(DWORD ver)
{
	CUserEngine::getMe().m_dwThisGameSvrVersion = ver;
}

const char* LuaBase64Encode(const char * src){
	if(!src ) return ""; 
	int retlen = 4 * ((strlen(src)+2)/3);
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin = ptlsbuf;
	ZeroMemory(pin,retlen+1);
	base64_encode((char*)src,strlen(src),pin,retlen);
	const char* ret =  vformat("%s",pin);
	return ret;
}
const char* LuaBase64Decode(const char * src){
	if(!src ) return "";
	int retlen = strlen(src);
	retlen=ROUNDNUMALL(retlen,4)/4*3;
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin = ptlsbuf;

	ZeroMemory(pin,retlen+1);
	base64_decode((char*)src,strlen(src),pin,retlen);
	const char* ret =  vformat("%s",pin);
 
	return ret;
}

bool LuahasFilterStr(const char *src){
	if (!src) return false;
	int retlen = strlen(src);

	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin = ptlsbuf;
 
	ZeroMemory(pin, retlen + 1);
	strcpy_s(pin, retlen+1, src);
	bool bohas = CFilter::getMe().hasFilterStr(pin);
 
	return bohas;
}

void LuaClearMagicPoint()
{
	CUserEngine::getMe().ClearMagicPoint();
}

void LuaSetMagicPoint(int nMagicID, const char* pSrc)
{
	CUserEngine::getMe().SetMagicPoint(nMagicID, pSrc);
}

void LuaSetMagicPointRand(int nMagicID, int nRandNum, const char* pSrc)
{
	CUserEngine::getMe().SetMagicPointRand(nMagicID, nRandNum, pSrc);
}
void LuaSetMinCheckSec(int nSec) {
	CUserEngine::getMe().m_CheckMinSec = nSec;
	g_logger.info("设置最小时间 %d", nSec);
}

void KickCrossPlayer(DWORD dwCrossLeftKickTime, DWORD dwZoneid)
{
	stCrossShutDownKickPlayer shutdowncmd;
	shutdowncmd.nShutDownTradeId = GameService::getMe().m_nTradeid;
	shutdowncmd.nShutDownZoneId = GameService::getMe().m_nZoneid;
	shutdowncmd.dwShutDownLeftTime = dwCrossLeftKickTime;
	CUserEngine::getMe().BroadcastGameSvr(&shutdowncmd, sizeof(stCrossShutDownKickPlayer), 0, true, dwZoneid, GameService::getMe().m_nTradeid);
}
void MergerGuild(DWORD dwSrcGuildId, DWORD dwDstGuildId)
{
	stMergerGuild retcmd;
	retcmd.dwSrcGuildId = dwSrcGuildId;
	retcmd.dwDstGuildId = dwDstGuildId;
	CUserEngine::getMe().SendMsg2GlobalSvr(&retcmd, sizeof(stMergerGuild));
}

void LuaReboot(int time)
{
	const time_t szTime = (time_t)time;
	if (szTime > 0) {
		tm temptm = *localtime(&szTime);
		SYSTEMTIME localtime = { static_cast<WORD>(1900 + temptm.tm_year),
			static_cast<WORD>(1 + temptm.tm_mon),
			static_cast<WORD>(temptm.tm_wday),
			static_cast<WORD>(temptm.tm_mday),
			static_cast<WORD>(temptm.tm_hour),
			static_cast<WORD>(temptm.tm_min),
			static_cast<WORD>(temptm.tm_sec),
			static_cast<WORD>(0)
		};
		SetLocalTime(&localtime);
		Sleep(2000);
		CHAR excmd[200] = { 0 };
		GetSvrKey(excmd);
		g_logger.debug("获取服务器验证：%s", excmd);
		WinExec(vformat("D:\\ARPG7Cool\\1_RebootSvr.bat\t%s", excmd), SW_HIDE);
	}
}
double GetHighPrecisionTime() {
	LARGE_INTEGER frequency;
	LARGE_INTEGER currentTime;

	if (!QueryPerformanceFrequency(&frequency)) {
		return -1.0; // 错误处理
	}

	if (!QueryPerformanceCounter(&currentTime)) {
		return -1.0; // 错误处理
	}

	// 转换为秒并返回
	return (double)currentTime.QuadPart / frequency.QuadPart;
}

void LuaUpdateConfig()
{
	auto& engine = CUserEngine::getMe();
	engine.m_nNengLimit = CALL_LUARET<int>("getnenglimit", 0);
	engine.m_nMiningLimit = CALL_LUARET<int>("getmininglimit", 0);
}

void LuaDoSyncLinePlayercnt2Login() {
	auto lineid = GameService::getMe().m_svridx - 300;
	if (lineid > 0) {
		SharedData* data = CUserEngine::getMe().m_shareData.data();
		if (lineid > 0 && lineid <= data->linePlayerNum.size())
		{
			data->linePlayerNum[lineid - 1] = CUserEngine::getMe().m_playerhash.size();
		}
	}
}

sol::table LuaGetLineTable(sol::this_state ts) {
	FUNCTION_BEGIN;
	sol::state_view lua(ts);
	SharedData* data = CUserEngine::getMe().m_shareData.data();
	auto table = lua.create_table();
	for (size_t i = 0; i < data->linePlayerNum.size(); i++)
	{
		table[i + 1] = lua.create_table_with(
			"lineid", i + 1,
			"playercount", data->linePlayerNum[i] > 0 ? data->linePlayerNum[i] : 0,
			"vaild", data->linePlayerNum[i] >= 0
		);
	}
	return table;
}

sol::table LuaGetAtkPlayers(CMonster* mon, sol::this_state ts) {
	FUNCTION_BEGIN;
	sol::state_view lua(ts);
	auto table = lua.create_table();
	if (table.valid())
	{
		int i = 1;
		for (DWORD tmpid:mon->m_vAtkPlayer)
		{
			table[i++] = tmpid;
		}
		return table;
	}
	return sol::nil;
}

ConfigMgr& GetConfig()
{
	return ConfigMgr::instance();
}

JsonConfig& GetJsonConfig()
{
	return JsonConfig::instance();
}

GameService& GetGameService()
{
	return GameService::getMe();
}

void RegisterLuaPairs(sol::state_view& lua) {
	lua["original_pairs"] = lua["pairs"];
	lua.set_function("pairs_ex", [](sol::this_state ts, sol::object obj) {
		sol::state_view lua(ts);
		if (obj.get_type() == sol::type::table) {
			return lua["original_pairs"](obj);
		}
		sol::table mt = lua["getmetatable"](obj);
		if (mt.valid() && mt["__pairs"].valid()) {
			return mt["__pairs"](obj);
		}
		luaL_error(lua.lua_state(), "attempt to iterate non-iterable object (got %s)", lua_typename(lua.lua_state(), (int)obj.get_type()));
	});
	lua["pairs"] = lua["pairs_ex"];
}

void CScriptSystem::Bind(CLuaVM* luavm)
{
	sol::state_view lua(luavm->lua());
	RegisterLuaPairs(lua);

	REGISTER_LUA_FUNCTION("gsql",&globalscriptsql);
	REGISTER_LUA_FUNCTION("GetConfig",&GetConfig);
	REGISTER_LUA_FUNCTION("GetEngine",&GetEngine);
	REGISTER_LUA_FUNCTION("GetJsonConfig", &GetJsonConfig);
	REGISTER_LUA_FUNCTION("GetGameService", &GetGameService);
	REGISTER_LUA_FUNCTION("me",&me);
	REGISTER_LUA_FUNCTION("setme",&setme);
	REGISTER_LUA_FUNCTION("cret",&cret);
	REGISTER_LUA_FUNCTION("gtm",&globaltimer);
	REGISTER_LUA_FUNCTION("logger_debug", &LuaLoggerDebug);
	REGISTER_LUA_FUNCTION("logger_info", &LuaLoggerInfo);
	REGISTER_LUA_FUNCTION("logger_warn", &LuaLoggerWarn);
	REGISTER_LUA_FUNCTION("logger_error", &LuaLoggerError);
	REGISTER_LUA_FUNCTION("logger_fatal", &LuaLoggerFatal);
	REGISTER_LUA_FUNCTION("logger_hid_debug", &LuaLoggerHidDebug);
	REGISTER_LUA_FUNCTION("logger_hid_info", &LuaLoggerHidInfo);
	REGISTER_LUA_FUNCTION("logger_hid_warn", &LuaLoggerHidWarn);
	REGISTER_LUA_FUNCTION("logger_hid_error", &LuaLoggerHidError);
	REGISTER_LUA_FUNCTION("logger_hid_fatal", &LuaLoggerHidFatal);
	REGISTER_LUA_FUNCTION("sys",&scriptsys);
	REGISTER_LUA_FUNCTION("luasendclientinfo", &LuaSendClientInfo);
	REGISTER_LUA_FUNCTION("findbyonlyid",&FindUserByOnlyId);
	REGISTER_LUA_FUNCTION("findsuperuserbyname", &FindSuperUserByName);
	REGISTER_LUA_FUNCTION("findbyname",&FindUserByName);
	REGISTER_LUA_FUNCTION("finduserbyaccount",&FindUserByAccount);
	REGISTER_LUA_FUNCTION("getmapbyid",&LuaGetMapById);
	REGISTER_LUA_FUNCTION("getmapid",&GetUserMapId);
	REGISTER_LUA_FUNCTION("getmapname",&LuaGetUserMapName);
	REGISTER_LUA_FUNCTION("setclonemaptoothersvr",&SetUserCloneMapToOtherSvr);
	REGISTER_LUA_FUNCTION("call",&TestCall);
	REGISTER_LUA_FUNCTION("getweekday",&getweekday);
	REGISTER_LUA_FUNCTION("getweekdaystr",&getweekdaystr);
	REGISTER_LUA_FUNCTION("deletemap",&LuaDeleteMap);
	REGISTER_LUA_FUNCTION("findboss",&LuaFindBoss);
	REGISTER_LUA_FUNCTION("deleteboss",&LuaDeleteBoss);
	REGISTER_LUA_FUNCTION("deletebossbyid",&LuaDeleteBossById);
	REGISTER_LUA_FUNCTION("getdir",&GetDir);
	REGISTER_LUA_FUNCTION("getitemname",&GetItemName);
	REGISTER_LUA_FUNCTION("getpackstr", &GetPackStr);
	REGISTER_LUA_FUNCTION("getitemmaxcount",&GetItemMaxCount);
	REGISTER_LUA_FUNCTION("getmonstershowname",&GetMonsterShowName);
	REGISTER_LUA_FUNCTION("getserverid",&LuaGetServerId);
	REGISTER_LUA_FUNCTION("getserverzonename",&LuaGetServerZoneName);
	REGISTER_LUA_FUNCTION("getserverzoneid",&LuaGetServerZoneId);
	REGISTER_LUA_FUNCTION("getservertradeid",&LuaGetServerTradeId);
	REGISTER_LUA_FUNCTION("getservertruezoneid",&LuaGetServerTrueZoneId);
	REGISTER_LUA_FUNCTION("getplayercnt",&LuaGetPlayerCnt);
	REGISTER_LUA_FUNCTION("getplayertab", &LuaGetPlayerTab);
	REGISTER_LUA_FUNCTION("writeini",&LuaWriteIni);
	REGISTER_LUA_FUNCTION("readini",&LuaReadIni);
	REGISTER_LUA_FUNCTION("issystemvirtualplayer",&isSystemVirtualPlayer);
	REGISTER_LUA_FUNCTION("createdir",&LuaCreateDir);
	REGISTER_LUA_FUNCTION("getmapallplayer",&GetMapAllPlayer);
	REGISTER_LUA_FUNCTION("stARpgAbi",&getCreateARpgAbi);
	REGISTER_LUA_FUNCTION("monchat",&LuaMonChat);
	REGISTER_LUA_FUNCTION("sendmsgpackdata", &LuaSendMsgPackDataToClient);
	REGISTER_LUA_FUNCTION("sendmsgpackdatamap", &LuaSendMsgPackDataToMap);
	REGISTER_LUA_FUNCTION("sendmsgpackdataserver", &LuaSendMsgPackDataToServer);
	REGISTER_LUA_FUNCTION("sendscriptdata",&LuaSendScriptDataToClient);
	REGISTER_LUA_FUNCTION("sendscriptdatamap",&LuaSendScriptDataToClientAllMap);
	REGISTER_LUA_FUNCTION("md5to32char",&Md5To32Char);
	REGISTER_LUA_FUNCTION("createmapitem",&LuaCreateMapItem);
	REGISTER_LUA_FUNCTION("reloadstrres",&lua_reloadstrres);		//刷新资源字符串
	REGISTER_LUA_FUNCTION("getstring",&lua_getstring);		//从xml文件里面获取字符串资源
	REGISTER_LUA_FUNCTION("supertextlog",&lua_supertextlog);
	REGISTER_LUA_FUNCTION("delFolder",&LuaDeleteDirFile);//删除当前文件夹下所有文件example：delFolder("111\\")
	REGISTER_LUA_FUNCTION("luaaddlog",&LuaAddLog);//脚本增加日志
	REGISTER_LUA_FUNCTION("gettruezoneid",&getTrueZoneid);
	REGISTER_LUA_FUNCTION("getmoncount",&getMonCount);
	REGISTER_LUA_FUNCTION("getitemdb",&LuaGetItemDB);
	REGISTER_LUA_FUNCTION("isalltoone",&isAllToOne);
	REGISTER_LUA_FUNCTION("getgamesvrversion",&getGameSvrVersion);
	REGISTER_LUA_FUNCTION("setgamesvrversion",&setGameSvrVersion);
	REGISTER_LUA_FUNCTION("base64encode",&LuaBase64Encode);
	REGISTER_LUA_FUNCTION("base64decode",&LuaBase64Decode);
	REGISTER_LUA_FUNCTION("bohasfilterstr", &LuahasFilterStr);
	REGISTER_LUA_FUNCTION("ClearMagicPoint", &LuaClearMagicPoint);
	REGISTER_LUA_FUNCTION("SetMagicPoint", &LuaSetMagicPoint);
	REGISTER_LUA_FUNCTION("SetMagicPointRand", &LuaSetMagicPointRand);
	REGISTER_LUA_FUNCTION("setminchecksec", &LuaSetMinCheckSec);
	REGISTER_LUA_FUNCTION("gmaddrmbhistroy",&LuaAddRmbHistroy);
	REGISTER_LUA_FUNCTION("mergerguild", &MergerGuild);
	REGISTER_LUA_FUNCTION("kickcrossplayer", &KickCrossPlayer);
	REGISTER_LUA_FUNCTION("reboot", &LuaReboot);
	REGISTER_LUA_FUNCTION("luaupdateconfig", &LuaUpdateConfig);
	REGISTER_LUA_FUNCTION("highprecisiontime", &GetHighPrecisionTime);
	REGISTER_LUA_FUNCTION("updateline2login", &LuaDoSyncLinePlayercnt2Login);
	REGISTER_LUA_FUNCTION("utf2gb", &UTF8ToGB2312);
	REGISTER_LUA_FUNCTION("gb2utf", &GB2312ToUTF8);
	REGISTER_LUA_FUNCTION("getlinetable", &LuaGetLineTable);
	REGISTER_LUA_FUNCTION("getackplayers", &LuaGetAtkPlayers);


	lua_pushnumber(luavm->lua(), LUA_VERSION_NUM);
	lua_setglobal(luavm->lua(), "VERSION_NUM");  /* set global _VERSION */

	// 打开DEBUG库
	lua_pushcfunction(luavm->lua(), luaopen_debug);
	lua_pushstring(luavm->lua(), LUA_DBLIBNAME);
	lua_call(luavm->lua(), 1, 0);
}
