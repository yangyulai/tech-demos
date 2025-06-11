#include "MapItemEvent.h"
#include "BaseCreature.h"
#include "Item.h"
#include "UsrEngn.h"

CMapItemEvent::CMapItemEvent(CItem* pItem, PosType x, PosType y)
: CMapEvent(pItem->GetItemName(), CRET_ITEM, x, y)
, dwGroupId(0)
, OwnerItem(pItem)
{
	m_OwnerMap = NULL;
	m_emEventType = MAPEVENT_ITEM;
	m_emOwnerType = OWNER_NULL;
	i64OfBaseOnlyId = 0;
	i64DropBaseOnlyId = 0;
	btAllPickInterval = 60;
	i64DisCardOnlyId = 0;
	boIsPlayerDrop = false;
	m_i64ItemId = OwnerItem->m_Item.i64ItemID;

}

CMapItemEvent::~CMapItemEvent(){
	m_OwnerMap=NULL;
	if (m_emDisappearType==DISAPPEAR_TIME){
		if (OwnerItem){
			CUserEngine::getMe().ReleasePItem(OwnerItem,__FUNC_LINE__);
		}
	}else if (m_emDisappearType==DISAPPEAR_ADD){
		OwnerItem=NULL;
	}
}

void CMapItemEvent::EnterMapNotify(MapObject* obj)
{
	stMapItemEventAdd_NP retcmd;
	retcmd.wX = GetX();
	retcmd.wY = GetY();
	retcmd.i64ItemID = OwnerItem->m_Item.i64ItemID;
	retcmd.dwBaseID = OwnerItem->m_Item.dwBaseID;
	retcmd.dwCount = OwnerItem->m_Item.dwCount;
	retcmd.dwEffId = OwnerItem->m_Item.dwEffId;
	retcmd.btQuality = OwnerItem->m_Item.btQuality;
	retcmd.i64OwnerId = i64OfBaseOnlyId;
	retcmd.btDroperType = boIsPlayerDrop ? 1 : 0;
	//if (!CanPick(this))  retcmd.btLastAllPickSec = GetLastAllPickSec();
	if (retcmd.dwBaseID >= 154 && retcmd.dwBaseID <= 156)	 // 灵符礼券特殊处理拾取cd为0
	{
		retcmd.btLastAllPickSec = 0;
	}
	obj->SendMsgToMe(&retcmd, sizeof(retcmd));
}
void CMapItemEvent::LeaveMapNotify(MapObject* obj)
{
	stMapItemEventDel retCmd;
	retCmd.i64Id = m_i64ItemId;
	retCmd.wX = GetX();
	retCmd.wY = GetY();
	obj->SendMsgToMe(&retCmd, sizeof(retCmd));
}
void CMapItemEvent::Update()
{
	if (GetTickCount64() >= next_check_tick)
	{
		if (GetDisappearType() != DISAPPEAR_TIME)
			return;
		if (!isCanContinue()) {
			m_OwnerMap->DelItemFromMap(GetX(), GetY(), this);
			return;
		}
		next_check_tick = GetTickCount64() + 6000;
	}
}

void CMapItemEvent::Init(CGameMap *OwnerMap, int64_t i64OfOnlyId, int64_t i64DropOnlyId, emOwnerType OwnerType){
	m_OwnerMap=OwnerMap;
	m_emEventType=MAPEVENT_ITEM;
	m_emOwnerType=OwnerType;
	i64OfBaseOnlyId=i64OfOnlyId;
	i64DropBaseOnlyId=i64DropOnlyId;
	if (i64OfBaseOnlyId) {
		CPlayerObj* pPlayer = CUserEngine::getMe().m_playerhash.FindByOnlyId(i64OfBaseOnlyId);
		if (pPlayer && pPlayer->m_GroupInfo.dwGroupId) {
			dwGroupId = pPlayer->m_GroupInfo.dwGroupId;
		}
	}
	i64DisCardOnlyId = 0;
}

BYTE CMapItemEvent::GetLastAllPickSec()
{ DWORD dwNowTime = time(NULL); return (dwNowTime >= m_tBeginTime + btAllPickInterval) ? 0 : m_tBeginTime + btAllPickInterval - dwNowTime; }

bool CMapItemEvent::CanPick(CPlayerObj* player) { //是否能拾取
	if (OwnerItem && OwnerItem->m_Item.dwBaseID >= 154 && OwnerItem->m_Item.dwBaseID <= 156) {
		if (player && player->m_GuildInfo.dwPowerLevel >= _GUILDMEMBER_POWERLVL_MASTER) {
			return false;
		}
	}
	if (!i64OfBaseOnlyId) return true;
	if (i64OfBaseOnlyId == 0xFFFFFFFFFFFFFFFF) {
		return GetLastAllPickSec() ? false : true;
	}
	if (player->m_i64UserOnlyID == i64OfBaseOnlyId) //拾取者就是指定的拾取者
		return true;
	else {
		if (m_emOwnerType == OWNER_PLAYER || m_emOwnerType == OWNER_NULL) return true;
		if (GetLastAllPickSec() == 0) return true;
		if (i64OfBaseOnlyId && player->m_GroupInfo.dwGroupId && dwGroupId == player->m_GroupInfo.dwGroupId) {
			stGMember tmp;
			if (player->m_cGroupMemberHash.find(i64OfBaseOnlyId, tmp)) {
				return true;
			}
		}
	}
	return false;
}
