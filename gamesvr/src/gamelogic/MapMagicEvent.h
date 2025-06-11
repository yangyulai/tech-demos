#pragma once
#include "MapEvent.h"
class stMagicDataBase;
class Magic_Map;
class CMapMagicEvent final : public CMapEvent
{
public:
	CMapMagicEvent(const CMapMagicEvent& other) = delete;
	CMapMagicEvent(CMapMagicEvent&& other) noexcept = delete;
	CMapMagicEvent& operator=(const CMapMagicEvent& other) = delete;
	CMapMagicEvent& operator=(CMapMagicEvent&& other) noexcept = delete;

	CMapMagicEvent(CCreature* pCret, Magic_Map* pMagic ,PosType x, PosType y);
	~CMapMagicEvent() override;
	void Init(CCreature* pCret, Magic_Map* pMagic, CGameMap* OwnerMap);
	void EnterMapNotify(MapObject* obj) override;
	void LeaveMapNotify(MapObject* obj) override;
	void Update() override;
	emMagicType GetMagicType() {return m_emMagicType;}	//得到魔法类型
	emMagicType m_emMagicType;	//地面魔法类型(火墙，风墙等)
	inline static DWORD s_dwEvtTmpid = 100;
	DWORD m_dwTmpid;
	DWORD m_dwOwnerTmpID;			//谁放的
	Magic_Map* m_magic;
	CCreature* m_cret;
};