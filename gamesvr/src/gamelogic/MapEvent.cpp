#include "MapEvent.h"
#include <zLogger.h>

CMapEvent::CMapEvent(std::string_view name, uint8_t type, PosType x, PosType y)
: MapObject(name,type,x,y)
{
	m_dwNextRunTick = 0;
	m_dwIntervalTick = 500;
	m_emEventType = MAPEVENT_NULL;
	m_emOwnerType = OWNER_NULL;
	m_emDisappearType = DISAPPEAR_TIME;
	m_tBeginTime = 0;
	m_tContinueTime = 0;
	m_OwnerMap = NULL;
}

CMapEvent::~CMapEvent(){
	static zLogger::zLevel tmploglvl(zLogger::zDEBUG.name,zLogger::zDEBUG.writelevel,6,zLogger::zDEBUG.realtimewrite,zLogger::zDEBUG.showcolor);
	g_logger.log(tmploglvl,"MapItem¶ÔÏó[%d,%d]  %.8x(%d,%d,%d) ±»É¾³ý",GetX(),GetY(),this,m_emEventType,m_emOwnerType,m_emDisappearType);
	m_OwnerMap=NULL;
}

void CMapEvent::Update(){
}

bool CMapEvent::isCanContinue() const
{
	return (time(NULL)<=(m_tBeginTime+m_tContinueTime));
}

bool CMapEvent::isCanBegin() const
{return (time(NULL)>=m_tBeginTime);}

int CMapEvent::GetContinueTime() const
{return safe_max((int)((m_tBeginTime+m_tContinueTime)-time(NULL)),0);}
