#pragma once
#include "MapEvent.h"
class CMapItemEvent final : public CMapEvent
{
public:
	CMapItemEvent(const CMapItemEvent& other) = delete;
	CMapItemEvent(CMapItemEvent&& other) noexcept = delete;
	CMapItemEvent& operator=(const CMapItemEvent& other) = delete;
	CMapItemEvent& operator=(CMapItemEvent&& other) noexcept = delete;

	CMapItemEvent(CItem* pItem,PosType x, PosType y);
	~CMapItemEvent() override;
	void EnterMapNotify(MapObject* obj) override;
	void LeaveMapNotify(MapObject* obj) override;
	void Update() override;
	void Init(CGameMap* OwnerMap, int64_t i64OfOnlyId, int64_t i64DropOnlyId, emOwnerType OwnerType);
	BYTE GetLastAllPickSec();
	bool CanPick(CPlayerObj* player);

	int64_t m_i64ItemId;			//物品唯一ID
	CItem* OwnerItem;				//物品结构
	int64_t i64OfBaseOnlyId;				//掉给谁的
	int64_t i64DropBaseOnlyId;			//谁掉落的
	BYTE btAllPickInterval;         //所有人都能拾取的时间间隔(单位秒)
	DWORD dwGroupId;                //掉落时的队伍ID
	int64_t i64DisCardOnlyId;		//丢弃者玩家ID	
	bool boIsPlayerDrop; //玩家被爆的
};
