#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <zlib/zlib.h>
#include <event/TimerSystem.h>

#include "Point.h"
#include "MapDef.h"
class CPlayerObj;
class CNpcObj;
class CPetObj;
class CMonster;
class CRobot;
class MapObject
{
	friend class MapRegion;
	friend class MapBase;
public:
	MapObject(const MapObject& other) = delete;
	MapObject(MapObject&& other) noexcept = delete;
	MapObject& operator=(const MapObject& other) = delete;
	MapObject& operator=(MapObject&& other) noexcept = delete;
	MapObject(std::string_view name, uint8_t type, PosType x, PosType y);
	virtual ~MapObject();
	virtual void OnLeaveMap();
	virtual bool ShouldNotify(const MapObject* observer) const;
	virtual void Update();
	virtual bool CanEnterMap();
	virtual bool HasTriggerRegions() const;
	virtual void SendMsgToMe(void* cmd, int cmd_len, int zliblvl = Z_DEFAULT_COMPRESSION) {}
	virtual void EnterMapNotify(MapObject* obj) {}
	virtual void LeaveMapNotify(MapObject* obj) {}
	virtual void MapMoveNotify(MapObject* obj) {}
	std::string_view GetName() const;
	std::string& Name();
	const char* getName() const;
	uint32_t GetObjectId() const;
	void SetObjectId(uint32_t id);
	const char* GetTypeName() const;
	uint8_t GetType() const;
	void SetType(uint8_t type);
	PosType GetX() const { return m_nCurrX; }
	PosType GetY() const { return m_nCurrY; }
	PosType GetGridX() const;
	PosType GetGridY() const;
	void SetX(PosType x) { m_nCurrX = x; }
	void SetY(PosType y) { m_nCurrY = y; }
	void SetPoint(PosType x, PosType y);
	bool isPlayer() const { return m_type == CRET_PLAYER; }							// �Ƿ������
	CPlayerObj* toPlayer();
	bool isNpc() const { return m_type == CRET_NPC; }									// �Ƿ�NPC	
	CNpcObj* toNpc();
	bool isPet() const { return m_type == CRET_PET; }									// �Ƿ��ǳ���
	CPetObj* toPet();
	bool isMonster() const { return  m_type == CRET_MONSTER; }						// �Ƿ��ǹ��� ���Ҳ��ǳ���	
	CMonster* toMonster();
	bool isItem() const { return m_type == CRET_ITEM; }								// �Ƿ�����Ʒ
	bool isMagic() const { return m_type == CRET_MAGIC; }								// �Ƿ���ħ��
	bool isRobot() const { return m_type == CRET_ROBOT; }								// �Ƿ��ǻ���
	CRobot* toRobot();
	int ChebyshevDistance(PosType x, PosType y) const;
	int ManhattanDistance(PosType x, PosType y) const;
	int ChebyshevDistance(MapObject* other) const;// �б�ѩ�����: ����֮��ľ���������������ֵ������ֵ
	int ManhattanDistance(MapObject* other) const;// �����پ���: ����֮��ľ���������������ֵ��ľ���ֵ֮��
	bool IsSamePoint(PosType x, PosType y) const;
	void SetName(const std::string& name);
	void SetName(const char* name);
	void SetDisplayName(const char* name);
	void SetName(std::string_view name);
	template<size_t N>
	void SetName(const char(&name)[N]);
	void Delete() { delete_ = true; }
	bool IsDelete() const { return delete_; }
	//MapBase* m_map;
	std::string m_name;
	std::string m_displayName;
	MapObject* m_prev;
	MapObject* m_next;
	uint32_t m_objectId;
	PosType m_nCurrX;				//����X
	PosType m_nCurrY;				//����Y
	PosType m_nCurrZ;				//����Z
	MapObject* cell_prev = nullptr;
	MapObject* cell_next = nullptr;
	MapRegion* m_region;
private:
	bool delete_;
protected:
	TimerSystem timer_;
	uint8_t m_type;
};

template <size_t N>
void MapObject::SetName(const char(& name)[N])
{
	size_t len = strnlen(name, N - 1);
	m_name.assign(name, len);
}
