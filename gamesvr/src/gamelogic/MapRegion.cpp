#include "MapRegion.h"
#include <zLogger.h>

#include "MapBase.h"

MapRegion::MapRegion(MapBase* map, uint16_t rx, uint16_t ry):
	m_map(map), m_pPrev(nullptr), m_pNext(nullptr),
	m_headNode(nullptr),
	m_refCount(0),
	m_regionX(rx),
	m_regionY(ry),
	m_monCount(0)
{
}

MapRegion::~MapRegion()
{
}

void MapRegion::ActiveMe()
{
	//g_logger.error("RegionX=%d,RegionY=%d Active", m_regionX, m_regionY);
}

void MapRegion::DeactivateMe()
{
	//g_logger.error("RegionX=%d,RegionY=%d Deactivate", m_regionX, m_regionY);
}

bool MapRegion::AddObject(MapObject* obj)
{
	//g_logger.debug("RegionX=%d,RegionY=%d AddObject¡¾%.8x->(%.8x,%s) %s:%s:%d¡¿", m_regionX, m_regionY, obj,m_map,m_map->m_name.c_str(),obj->GetTypeName(),
	//               obj->getName(), obj->GetObjectId());

	assert(m_objects.insert(obj).second);
	obj->m_prev = nullptr;
	if (m_headNode == nullptr)
	{
		obj->m_next = nullptr;
		m_headNode = obj;
	}
	else
	{
		obj->m_next = m_headNode;
		m_headNode->m_prev = obj;
		m_headNode = obj;
	}
	if (obj->isMonster()) {
		m_monCount++;
	}
	return true;
}

bool MapRegion::RemoveObject(MapObject* obj)
{
	//g_logger.debug("RegionX=%d,RegionY=%d RemoveObject¡¾%.8x->(%.8x,%s) %s:%s:%d¡¿", m_regionX, m_regionY, obj,m_map, m_map->m_name.c_str(),obj->GetTypeName(),
	//               obj->getName(), obj->GetObjectId());
	assert(m_objects.erase(obj) > 0);
	MapObject* pPrevObj = obj->m_prev;
	MapObject* pNextObj = obj->m_next;

	obj->m_prev = nullptr;
	obj->m_next = nullptr;

	if (pPrevObj)
	{
		pPrevObj->m_next = pNextObj;
	}
	else
	{
		m_headNode = pNextObj;
	}
	if (pNextObj)
	{
		pNextObj->m_prev = pPrevObj;
	}
	if (obj->isMonster()) {
		m_monCount--;
	}
	return true;
}

bool MapRegion::RemoveAllObjects()
{
	MapObject* pObj = m_headNode;
	while (pObj)
	{
		MapObject* p = pObj;
		pObj = p->m_next;
		p->m_prev = nullptr;
		p->m_next = nullptr;
	}
	m_headNode = nullptr;
	m_monCount = 0;
	return true;
}

void MapRegion::Run() const
{
	MapObject* pObj = m_headNode;
	while (pObj)
	{
		auto& nextObj = pObj->m_next;
		pObj->Update();
		pObj = nextObj;
	}
}
