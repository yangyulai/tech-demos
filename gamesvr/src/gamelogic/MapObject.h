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
	bool isPlayer() const { return m_type == CRET_PLAYER; }							// 是否是玩家
	CPlayerObj* toPlayer();
	bool isNpc() const { return m_type == CRET_NPC; }									// 是否NPC	
	CNpcObj* toNpc();
	bool isPet() const { return m_type == CRET_PET; }									// 是否是宠物
	CPetObj* toPet();
	bool isMonster() const { return  m_type == CRET_MONSTER; }						// 是否是怪物 并且不是宠物	
	CMonster* toMonster();
	bool isItem() const { return m_type == CRET_ITEM; }								// 是否是物品
	bool isMagic() const { return m_type == CRET_MAGIC; }								// 是否是魔法
	bool isRobot() const { return m_type == CRET_ROBOT; }								// 是否是机器
	CRobot* toRobot();
	int ChebyshevDistance(PosType x, PosType y) const;
	int ManhattanDistance(PosType x, PosType y) const;
	int ChebyshevDistance(MapObject* other) const;// 切比雪夫距离: 两点之间的距离是两点坐标数值差的最大值
	int ManhattanDistance(MapObject* other) const;// 曼哈顿距离: 两点之间的距离是两点坐标数值差的绝对值之和
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
	PosType m_nCurrX;				//坐标X
	PosType m_nCurrY;				//坐标Y
	PosType m_nCurrZ;				//坐标Z
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
