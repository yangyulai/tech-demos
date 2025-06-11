#pragma once

#include "MapObject.h"

class MapCellObject {
	MapObject* head = nullptr;

public:
	void AddObject(MapObject* obj);

	void RemoveObject(MapObject* obj);

	template<typename Func>
	void ForEachObject(Func&& func) const;
	// 安全遍历（允许在回调中安全移除当前对象）
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
		MapObject* next = current->cell_next; // 预先获取下一个节点
		func(current);                        // 处理当前节点
		current = next;                       // 移动到下一个节点
	}
}

template <typename Func>
void MapCellObject::ForEachSafe(Func&& func)
{
	MapObject* current = head;
	while (current) {
		MapObject* next = current->cell_next;
		bool keep_going = func(current);
		if (!keep_going) break;     // 允许提前终止遍历
		current = next;
	}
}

template <typename Predicate>
MapObject* MapCellObject::FindIf(Predicate&& pred)
{
	MapObject* current = head;
	while (current) {
		MapObject* next = current->cell_next; // 预先获取下一个节点
		if (pred(current)) {
			return current;
		}
		current = next;                       // 移动到下一个节点
	}
	return nullptr; // 如果没有找到，返回nullptr;
}