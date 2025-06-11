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
	emMagicType GetMagicType() {return m_emMagicType;}	//�õ�ħ������
	emMagicType m_emMagicType;	//����ħ������(��ǽ����ǽ��)
	inline static DWORD s_dwEvtTmpid = 100;
	DWORD m_dwTmpid;
	DWORD m_dwOwnerTmpID;			//˭�ŵ�
	Magic_Map* m_magic;
	CCreature* m_cret;
};