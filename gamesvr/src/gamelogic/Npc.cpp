#include "UsrEngn.h"
#include "Npc.h"

CNpcObj::CNpcObj(PosType x, PosType y, mydb_npcgen_tbl* pnpcinfo)
	: CCreature(UTG(pnpcinfo->name.c_str()), CRET_NPC, x, y, 0), m_szNpcFullName{}, m_pNpcInfo(pnpcinfo)
{
	strcpy_s(m_szNpcName, sizeof(m_szNpcName) - 1, pnpcinfo->name.c_str());
	strcpy_s(m_szNpcShowName, sizeof(m_szNpcShowName) - 1, pnpcinfo->name.c_str());
	filtershowname(m_szNpcShowName, sizeof(m_szNpcShowName) - 1);
	m_dwDeleteTime = 0;
	m_dwEmblemId = 0;
}

CNpcObj::~CNpcObj() = default;

void CNpcObj::Update()
{
	FeatureChanged();
}

const char* CNpcObj::getShowName(char* szbuffer, int nmaxlen)
{
	return m_szNpcName;
}

void CNpcObj::EnterMapNotify(MapObject* obj)
{
	if (obj->GetType() != CRET_PLAYER) return;
	stMapCreateCret retcmd;
	retcmd.location.mapid = GetEnvir()->getMapId();
	retcmd.location.ncurx = m_nCurrX;
	retcmd.location.ncury = m_nCurrY;
	retcmd.dwTmpId = GetObjectId();
	retcmd.lifestate = m_LifeState;
	retcmd.dwLevel = m_dwLevel;
	retcmd.nNowHp = 1;
	retcmd.nMaxHp = 1;
	retcmd.btDir = m_btDirection;
	retcmd.btCretType = GetType();
	retcmd.crettypeid = toNpc()->m_pNpcInfo->id;
	retcmd.dress = toNpc()->m_pNpcInfo->monsterId;
	retcmd.dwEmblemId = m_dwEmblemId;
	CopyString(retcmd.szShowName, m_displayName);
	obj->SendMsgToMe(&retcmd, sizeof(retcmd));
	g_logger.debug("%s °æ%d,%d %d,%d°ø Ω¯»Î ”“∞", m_name.c_str(), GetX(), GetY(), GetGridX(), GetGridY());

	AfterSpaceMove(obj);
}

void CNpcObj::run()
{

}

void CNpcObj::SetEmblem(DWORD dwEmblemId) {
	if (m_dwEmblemId != dwEmblemId)
	{
		m_dwEmblemId = dwEmblemId;
		UpdateAppearance(FeatureIndex::emblem, m_dwEmblemId);
	}	
}