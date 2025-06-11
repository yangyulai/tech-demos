#pragma once
#include <SyncHashMap.h>
#include <unordered_set>
#include "MapObject.h"

#define REGION_SIZE 12 //�����С
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
	MapObject* m_headNode;      //��������
	int16_t m_refCount;        //���ü������ж������Ƿ񱻼���
	uint16_t m_regionX;         //regionX�±�x Ԥ��
	uint16_t m_regionY;         //regionY�±�y Ԥ��
	std::unordered_set<MapObject*> m_objects; //�����ڵĶ��󼯺�
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
		MapObject* next = current->m_next; // Ԥ�Ȼ�ȡ��һ���ڵ�
		if (pred(current)) {
			return current;
		}
		current = next;                       // �ƶ�����һ���ڵ�
	}
	return nullptr; // ���û���ҵ�������nullptr;
}
