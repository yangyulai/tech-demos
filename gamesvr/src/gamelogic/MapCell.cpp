#include "MapCell.h"

void MapCellObject::AddObject(MapObject* obj)
{
	obj->cell_next = head;
	if (head) head->cell_prev = obj;
	head = obj;
}

void MapCellObject::RemoveObject(MapObject* obj)
{
	if (obj->cell_prev) obj->cell_prev->cell_next = obj->cell_next;
	else head = obj->cell_next;
	if (obj->cell_next) obj->cell_next->cell_prev = obj->cell_prev;
	obj->cell_prev = nullptr;
	obj->cell_next = nullptr;
}

