#pragma once
#include <SyncHashMap.h>
#include <unordered_set>
#include "MapObject.h"

#define REGION_SIZE 12 //区域大小
class CCreature;

class MapRegion {
public:
	MapRegion(const MapRegion& other) = delete;
	MapRegion(MapRegion&& other) noexcept = delete;
	MapRegion& operator=(const MapRegion& other) = delete;
	MapRegion& operator=(MapRegion&& other) noexcept = delete;
	MapRegion(MapBase* map, uint16_t rx, uint16_t ry);
	~MapRegion();
	void ActiveMe();
	void DeactivateMe();
	bool AddObject(MapObject* obj);
	bool RemoveObject(MapObject* obj);
	bool RemoveAllObjects();
	void Run() const;
	template<typename Func>
	void ExecuteAllObjects(Func&& func) const;
	template<typename Predicate>
	MapObject* FindIf(Predicate&& pred);

	MapBase* m_map;
	MapRegion* m_pPrev;
	MapRegion* m_pNext;
	MapObject* m_headNode;      //对象链表
	int16_t m_refCount;        //引用计数，判断区域是否被激活
	uint16_t m_regionX;         //regionX下标x 预留
	uint16_t m_regionY;         //regionY下标y 预留
	std::unordered_set<MapObject*> m_objects; //区域内的对象集合
	uint32_t m_monCount;
};

template <typename Func>
void MapRegion::ExecuteAllObjects(Func&& func) const
{
	MapObject* current = m_headNode;
	while (current) {
		std::invoke(func, current);
		current = current->m_next;
	}
}


template <typename Predicate>
MapObject* MapRegion::FindIf(Predicate&& pred)
{
	MapObject* current = m_headNode;
	while (current) {
		MapObject* next = current->m_next; // 预先获取下一个节点
		if (pred(current)) {
			return current;
		}
		current = next;                       // 移动到下一个节点
	}
	return nullptr; // 如果没有找到，返回nullptr;
}
