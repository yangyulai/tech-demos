#pragma once

#include "MapObject.h"

class MapCellObject {
	MapObject* head = nullptr;

public:
	void AddObject(MapObject* obj);

	void RemoveObject(MapObject* obj);

	template<typename Func>
	void ForEachObject(Func&& func) const;
	// ��ȫ�����������ڻص��а�ȫ�Ƴ���ǰ����
	template<typename Func>
	void ForEachSafe(Func&& func);

	template<typename Predicate>
	MapObject* FindIf(Predicate&& pred);

	bool empty() const { return head == nullptr; }
};


template <typename Func>
void MapCellObject::ForEachObject(Func&& func) const
{
	MapObject* current = head;
	while (current) {
		MapObject* next = current->cell_next; // Ԥ�Ȼ�ȡ��һ���ڵ�
		func(current);                        // ����ǰ�ڵ�
		current = next;                       // �ƶ�����һ���ڵ�
	}
}

template <typename Func>
void MapCellObject::ForEachSafe(Func&& func)
{
	MapObject* current = head;
	while (current) {
		MapObject* next = current->cell_next;
		bool keep_going = func(current);
		if (!keep_going) break;     // ������ǰ��ֹ����
		current = next;
	}
}

template <typename Predicate>
MapObject* MapCellObject::FindIf(Predicate&& pred)
{
	MapObject* current = head;
	while (current) {
		MapObject* next = current->cell_next; // Ԥ�Ȼ�ȡ��һ���ڵ�
		if (pred(current)) {
			return current;
		}
		current = next;                       // �ƶ�����һ���ڵ�
	}
	return nullptr; // ���û���ҵ�������nullptr;
}