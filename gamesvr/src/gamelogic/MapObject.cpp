#include "MapObject.h"

#include <zLogger.h>

#include "MapRegion.h"
#include "MapBase.h"
#include "PlayerObj.h"
#include "Npc.h"
#include "Robot.h"

std::string_view MapObject::GetName() const
{
	return m_name;
}

std::string& MapObject::Name()
{
	return m_name;
}
const char* MapObject::getName() const
{
	return m_name.c_str();
}

bool MapObject::CanEnterMap()
{ return false; }

bool MapObject::HasTriggerRegions() const
{
	return false;
}

uint32_t MapObject::GetObjectId() const
{
	return m_objectId;
}

void MapObject::SetObjectId(uint32_t objId)
{
	m_objectId = objId;
}

const char* MapObject::GetTypeName() const
{
	return to_string(static_cast<eCretType>(GetType()));
}

uint8_t MapObject::GetType() const
{ return m_type; }

void MapObject::SetType(uint8_t objType)
{m_type= objType; }

PosType MapObject::GetGridX() const
{ return m_region->m_regionX; }

PosType MapObject::GetGridY() const
{ return m_region->m_regionY; }

void MapObject::SetPoint(PosType x, PosType y)
{
	m_nCurrX = x;
	m_nCurrY = y;
}

CPlayerObj* MapObject::toPlayer()
{
	return isPlayer() ? static_cast<CPlayerObj*>(this) : nullptr;
}

CNpcObj* MapObject::toNpc()
{
	return isNpc() ? static_cast<CNpcObj*>(this) : nullptr;
}

CPetObj* MapObject::toPet()
{
	return isPet() ? static_cast<CPetObj*>(this) : nullptr;
}

CMonster* MapObject::toMonster()
{
	return isMonster() ? static_cast<CMonster*>(this) : nullptr;
}

CRobot* MapObject::toRobot()
{
	return isRobot() ? static_cast<CRobot*>(this) : nullptr;
}

int MapObject::ChebyshevDistance(PosType x, PosType y) const
{
	return std::max<int>(std::abs(m_nCurrX - x), std::abs(m_nCurrY - y));
}

int MapObject::ManhattanDistance(PosType x, PosType y) const
{
	return std::abs(m_nCurrX - x) + std::abs(m_nCurrY - y);
}

int MapObject::ChebyshevDistance(MapObject* other) const
{
	if (!other) return 0;
	return std::max<PosType>(std::abs(m_nCurrX - other->GetX()), std::abs(m_nCurrY - other->GetY()));
}

int MapObject::ManhattanDistance(MapObject* other) const
{
	if (!other) return 0;
	return std::abs(m_nCurrX - other->GetX()) + std::abs(m_nCurrY - other->GetY());
}

bool MapObject::IsSamePoint(PosType x, PosType y) const
{
	return m_nCurrX == x && m_nCurrY ==y;
}
void MapObject::SetName(const std::string & name)
{
	m_name = name;
}

void MapObject::SetName(const char* name)
{
	if (name) {
		size_t len = strnlen(name, 96);
		m_name.assign(name, len);
	}
	else {
		m_name.clear();
	}
}

void MapObject::SetDisplayName(const char* name)
{
	if (name) {
		size_t len = strnlen(name, 96);
		m_displayName.assign(name, len);
	}
	else {
		m_displayName.clear();
	}
}

void MapObject::SetName(std::string_view name)
{
	m_name.assign(name.data(), name.size());
}

bool MapObject::ShouldNotify(const MapObject * observer) const
{
	return false;
}

MapObject::MapObject(std::string_view name, uint8_t type, PosType x, PosType y)
	: m_name(name), m_prev(nullptr)
	  , m_next(nullptr)
	  , m_objectId(0)
	  , m_type(type)
	  , m_nCurrX(x)
	  , m_nCurrY(y)
	  , m_nCurrZ(0), m_region(nullptr), delete_(false)
{
	size_t pos = m_name.find('_');
	if (pos != std::string::npos)
		m_displayName = m_name.substr(0, pos);
	else
		m_displayName = m_name;

	g_logger.debug("对象【%s:%s】%.8x 创建,ID=%d", GetTypeName(), m_name.c_str(),this,m_objectId);
}

MapObject::~MapObject()
{
	g_logger.debug("对象【%s:%s】%.8x 被删除,ID=%d", GetTypeName(), m_name.c_str(),this, m_objectId);
}

void MapObject::OnLeaveMap()
{
}

void MapObject::Update()
{

}