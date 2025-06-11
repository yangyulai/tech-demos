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

	int64_t m_i64ItemId;			//��ƷΨһID
	CItem* OwnerItem;				//��Ʒ�ṹ
	int64_t i64OfBaseOnlyId;				//����˭��
	int64_t i64DropBaseOnlyId;			//˭�����
	BYTE btAllPickInterval;         //�����˶���ʰȡ��ʱ����(��λ��)
	DWORD dwGroupId;                //����ʱ�Ķ���ID
	int64_t i64DisCardOnlyId;		//���������ID	
	bool boIsPlayerDrop; //��ұ�����
};
