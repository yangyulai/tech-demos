#include "MapMagicEvent.h"
#include "BaseCreature.h"
#include "UsrEngn.h"
#include "MagicExtend.h"

CMapMagicEvent::CMapMagicEvent(CCreature* pCret, Magic_Map* pMagic , PosType x, PosType y)
: CMapEvent(pMagic->GetMagicDataBase()->szName, CRET_MAGIC, x, y)
, m_magic(pMagic)
, m_cret(pCret)
{
	s_dwEvtTmpid++;
	m_dwTmpid = s_dwEvtTmpid;
	m_OwnerMap = NULL;
	m_emOwnerType = OWNER_NULL;
	m_emMagicType = MAGIC_NULL;
	m_OwnerMap = pCret->GetEnvir();
	m_dwIntervalTick = m_magic->GetMagicDataBase()->intervalTime;
	m_dwOwnerTmpID = pCret->GetObjectId();
	m_emEventType = MAPEVENT_MAGIC;

}

CMapMagicEvent::~CMapMagicEvent() {
	m_OwnerMap = NULL;
}

void CMapMagicEvent::Init(CCreature* pCret, Magic_Map* pMagic, CGameMap* OwnerMap){
	this->m_cret = pCret;
}
void CMapMagicEvent::EnterMapNotify(MapObject* obj)
{
	stMapMagicEventAdd retcmd;
	retcmd.dwId = m_magic->GetMagicDataBase()->nSkillActionId;
	retcmd.dwTmpId = m_dwTmpid;
	retcmd.wX = GetX();
	retcmd.wY = GetY();
	retcmd.dwTime = GetContinueTime();
	obj->SendMsgToMe(&retcmd, sizeof(retcmd));
}
void CMapMagicEvent::LeaveMapNotify(MapObject* obj)
{
	stMapMagicEventDel retCmd;
	retCmd.dwId = m_magic->GetMagicDataBase()->nSkillActionId;
	retCmd.dwTmpId = m_dwTmpid;
	retCmd.wX = GetX();
	retCmd.wY = GetY();
	obj->SendMsgToMe(&retCmd, sizeof(retCmd));
}
void CMapMagicEvent::Update()
{
	ULONGLONG thistick = GetTickCount64();
	if (thistick > m_dwNextRunTick)
	{
		if (isCanContinue()) {
			if (m_magic)
			{
				m_magic->run(m_cret, m_OwnerMap, GetX(), GetY());
			}
		}
		else {
			m_OwnerMap->DelMagicFromMap(GetX(), GetY(), this);
		}
		m_dwNextRunTick = thistick + m_dwIntervalTick;
	}
}
