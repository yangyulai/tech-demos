#include "MapBase.h"
#include <cassert>
#include "utils/Utils.h"
#include <zLogger.h>
#include <array>

#include "MapCell.h"
#include "MapDef.h"

__declspec(thread) std::vector<MapObject*> tl_rvector;
__declspec(thread) std::vector<MapObject*> tl_xvector;
__declspec(thread) std::vector<MapObject*> tl_yvector;

MapBase::MapBase(std::string_view name)
: m_nWidth(0)
, m_nHeight(0)
, m_name(name)
{
	m_genObjId = 0;
	m_isActive = false;
	m_regionNumX = 0;
	m_regionNumY = 0;
	m_pActiveRegion = nullptr;
}

MapBase::~MapBase()
{
	m_genObjId = 0;
	for (auto it : this->m_objMap)
	{
		CN_DEL(it.second)
	}
	AILOCKT(m_enterQueue);
	while (!this->m_enterQueue.empty())
	{
		auto obj = this->m_enterQueue.front();
		CN_DEL(obj)
	}
}

void MapBase::ActiveRegion(MapRegion* region)
{
	if (region) {
		assert(region->m_refCount >= 0);
		if (region->m_refCount == 0) {//deactivate to active
			region->m_pPrev = nullptr;
			region->m_pNext = nullptr;
			if (!m_pActiveRegion) {
				m_pActiveRegion = region;
			}
			else {
				region->m_pNext = m_pActiveRegion;
				m_pActiveRegion->m_pPrev = region;
				m_pActiveRegion = region;
			}
			region->ActiveMe();
		}
		region->m_refCount++;
	}
}

void MapBase::DeactivateRegion(MapRegion* region)
{
	if (region) {
		region->m_refCount--;
		assert(region->m_refCount >= 0);
		if (region->m_refCount == 0) {
			MapRegion* prev = region->m_pPrev;
			MapRegion* next = region->m_pNext;
			if (prev) {
				prev->m_pNext = next;
			}
			else {
				m_pActiveRegion = next;
			}
			if (next) {
				next->m_pPrev = prev;
			}
			region->m_pPrev = nullptr;
			region->m_pNext = nullptr;
			region->DeactivateMe();
		}
	}
}
void MapBase::GetAllRegions(const MapRegion* cur_region, const MapRegion* next_region)
{
	entered_regions.clear();
	exited_regions.clear();
	syncing_regions.clear();
	bool has_null = cur_region == nullptr || next_region == nullptr;
	int16_t diff_x = has_null ? 0 : next_region->m_regionX - cur_region->m_regionX;
	int16_t diff_y = has_null ? 0 : next_region->m_regionY - cur_region->m_regionY;
	if (std::abs(diff_x) > 2 || std::abs(diff_y) > 2 || has_null)
	{
		for (auto& [x, y] : commonAoi)
		{
			if(next_region)
			{
				if (MapRegion* r = GetRegionByRegionPos(next_region->m_regionX + x, next_region->m_regionY + y))
				{
					entered_regions.push_back(r);
				}
			}
			if(cur_region)
			{
				if (MapRegion* r = GetRegionByRegionPos(cur_region->m_regionX + x, cur_region->m_regionY + y))
				{
					exited_regions.push_back(r);
				}
			}
		}
	}
	else
	{
		int index = (diff_x + 2) + (diff_y + 2) * 5;
		auto& aoi = sAoiArray[index];
		for (size_t i = 0; i < 3; ++i)
		{
			for (auto& [x, y] : aoi[i])
			{
				if (MapRegion* r = GetRegionByRegionPos(next_region->m_regionX + x, next_region->m_regionY + y))
				{
					if (i == 0) entered_regions.push_back(r);
					else if (i == 1) exited_regions.push_back(r);
					else if (i == 2) syncing_regions.push_back(r);
				}
			}
		}
	}
}

void MapBase::EnterNotify(MapObject* obj, bool isTrigger, int8_t objType)
{
	for (auto& it : entered_regions)
	{
		if (isTrigger) ActiveRegion(it);
		it->ExecuteAllObjects([&](MapObject* other)
		{
			OnEnterMapNotify(other, obj);
		});
	}
}

void MapBase::LeaveNotify(MapObject* obj, bool isTrigger, int8_t objType)
{
	for (auto& it : exited_regions)
	{
		if (isTrigger) DeactivateRegion(it);
		it->ExecuteAllObjects([&](MapObject* other)
		{
			OnLeaveMapNotify(obj, other);
		});
	}
}

bool MapBase::DoEnterMap(MapObject* obj)
{
	if (obj->IsDelete()) return false;
	bool isTrigger = obj->HasTriggerRegions();
	auto objType = obj->GetType();
	auto objId = obj->GetObjectId();
	if (objId == 0)
	{
		objId = GenObjId();
	}
	obj->SetObjectId(objId);
	MapRegion* region = GetRegionByObj(obj);
	assert(region);
	obj->m_region = region;
	assert(obj && obj->GetObjectId() && objType>0);
	assert(m_objMap.emplace(objId, obj).second);
	m_typeObjMap[objType][obj->GetObjectId()] = obj;
	//obj->m_map = this;
	SetCellObj(obj);
	region->AddObject(obj);
	GetAllRegions(nullptr, region);
	EnterNotify(obj, isTrigger, objType);
	OnEnterMap(obj);
	return true;
}

bool MapBase::DoLeaveMap(MapObject* obj)
{
	bool isTrigger = obj->HasTriggerRegions();
	auto objType = obj->GetType();
	auto objId = obj->GetObjectId();
	assert(obj && objId && objType >0);
	MapRegion* region = GetRegionByObj(obj);
	assert(region);
	obj->m_region = nullptr;
	GetAllRegions(region, nullptr);
	LeaveNotify(obj, isTrigger, objType);
	assert(m_objMap.erase(objId) > 0);
	m_typeObjMap[objType].erase(objId);
	//obj->m_map = nullptr;
	DelCellObj(obj);
	region->RemoveObject(obj);
	OnLeaveMap(obj);
	return true;
}

bool MapBase::DoMapMove(MapObject* obj, PosType pos_x, PosType pos_y)
{
	bool isTrigger = obj->HasTriggerRegions();
	auto objType = obj->GetType();
	auto objId = obj->GetObjectId();
	assert(obj && objId);
	auto cur_region = GetRegionByPos(obj->m_nCurrX, obj->m_nCurrY);
	auto next_region = GetRegionByPos(pos_x, pos_y);
	assert(cur_region && next_region);
	DelCellObj(obj);
	obj->SetPoint(pos_x, pos_y);
	if (cur_region == next_region) {
		for (auto& [x, y] : commonAoi)
		{
			if (MapRegion* r = GetRegionByRegionPos(cur_region->m_regionX + x, cur_region->m_regionY + y))
			{
				r->ExecuteAllObjects([&](MapObject* other)
					{
						OnMapMoveNotify(obj, other);
					});
			}
		}

	}else
	{
		cur_region->RemoveObject(obj);
		next_region->AddObject(obj);
		obj->m_region = next_region;
		GetAllRegions(cur_region, next_region);
		LeaveNotify(obj, isTrigger, objType);
		EnterNotify(obj, isTrigger, objType);
		for (auto& it : syncing_regions)
		{
			it->ExecuteAllObjects([&](MapObject* other)
				{
					OnMapMoveNotify(obj, other);
				});
		}
	}
	OnMapMove(obj);
	SetCellObj(obj);
	return true;
}

void MapBase::SetCellObj(MapObject* obj)
{
	Point point(obj->GetX(),obj->GetY());
	auto it = m_pMapCellInfo.find(point);
	if (it == m_pMapCellInfo.end())
	{
		auto tmp = std::make_unique<MapCellObject>();
		it = m_pMapCellInfo.emplace(point, std::move(tmp)).first;
	}
	it->second->AddObject(obj);
}
void MapBase::DelCellObj(MapObject* obj)
{

	auto cellObjects = GetCellObjects(obj->m_nCurrX, obj->m_nCurrY);
	assert(cellObjects);
	cellObjects->RemoveObject(obj);
}

bool MapBase::IsActive()
{
	return m_isActive || !m_enterQueue.s_empty() || !m_delHash.empty() || m_pActiveRegion;
}
bool MapBase::PushToDel(MapObject* obj, unsigned char afterRound)
{
	if (!obj) {
		return false;
	}
	m_delHash.emplace_back(obj,0,afterRound);
	return true;
}

void MapBase::ProcessDelList()
{
	for (auto it = m_delHash.begin(); it != m_delHash.end();)
	{
		if (it->curround >= it->allround) {
			CN_DEL(it->obj);
			it = m_delHash.erase(it);
		}
		else {
			++(it->curround);
			++it;
		}
	}
}
std::vector<MapObject*> MapBase::GetObjsByType(unsigned char objType) const
{
	tl_rvector.clear();
	for (auto it : this->m_typeObjMap[objType])
	{
		if (it.second) {
			tl_rvector.push_back(it.second);
		}
	}
	return tl_rvector;
}

MapObject* MapBase::GetObjByTypeCondition(unsigned char objType, const std::function<bool(MapObject*)>& predicate) const
{
	for (auto it : this->m_typeObjMap[objType])
	{
		if (predicate(it.second))
		{
			return it.second;
		}
	}
}

std::vector<MapObject*> MapBase::GetObjectsByTypeCondition(unsigned char objType, const std::function<bool(MapObject*)>& predicate) const
{
	tl_rvector.clear();
	for (auto it : this->m_typeObjMap[objType])
	{
		if (predicate(it.second))
		{
			tl_rvector.push_back(it.second);
		}
	}
	return tl_rvector;
}

uint32_t MapBase::GetObjCountByType(unsigned char objType) const
{
	auto size = m_typeObjMap[objType].size();
	return static_cast<uint32_t>(size);
}

MapObject* MapBase::GetObjById(unsigned int objid)
{
	if (objid != 0) {
		auto it = this->m_objMap.find(objid);
		if (it != this->m_objMap.end()) {
			return it->second;
		}
	}
	return nullptr;
}

MapObject* MapBase::GetObjByTypeId(unsigned char objType, unsigned int objid)
{
	if (objid != 0) {
		auto it = m_typeObjMap[objType].find(objid);
		if (it != m_typeObjMap[objType].end()) {
			return it->second;
		}
	}
	return	nullptr;
}

void MapBase::OnEnterMap(MapObject* obj)
{
	if (obj) {

	}
}

void MapBase::OnLeaveMap(MapObject* obj)
{
	if (obj)
	{
		obj->SetObjectId(0);
	}
}

void MapBase::run()
{
	ProcessEnterQueue();
	ProcessDelList();
	MapRegion* pRegion = m_pActiveRegion;
	while (pRegion != nullptr)
	{
		pRegion->Run();
		pRegion = pRegion->m_pNext;
	}
}

void MapBase::ClearObjMap()
{

}
bool MapBase::EnterMap(MapObject* obj)
{
	if (!obj) {
		return false;
	}
	m_enterQueue.Push(obj);
	return true;
}

void MapBase::ProcessEnterQueue()
{
	AILOCKT(m_enterQueue);
	while (!this->m_enterQueue.empty())
	{
		auto obj = this->m_enterQueue.front();
		this->m_enterQueue.pop();
		this->DoEnterMap(obj);
	}
}

unsigned int MapBase::GenObjId()
{
	do
	{
		if (this->m_genObjId == -1) {
			this->m_genObjId = 0;
		}
		++this->m_genObjId;
	} while (!(this->m_objMap.find(this->m_genObjId) == this->m_objMap.end()));
	return this->m_genObjId;
}

MapRegion* MapBase::GetRegionByPos(PosType px, PosType py)
{
	int rx = px / REGION_SIZE;
	int ry = py / REGION_SIZE;
	return GetRegionByRegionPos(rx, ry);
}

std::tuple<MapRegion*, int, int> MapBase::GetRegion(int px, int py)
{
	auto rx = px / REGION_SIZE;
	auto ry = py / REGION_SIZE;
	return std::make_tuple(GetRegionByRegionPos(rx, ry), rx, ry);
}
MapRegion* MapBase::GetRegionByObj(MapObject* obj)
{
	return GetRegionByPos(obj->m_nCurrX, obj->m_nCurrY);
}

MapRegion* MapBase::GetRegionByRegionPos(int rx, int ry)
{
	if (rx < m_pRegionArray.size() && ry < m_pRegionArray[rx].size())
	{
		if (m_pRegionArray[rx][ry] == nullptr)
		{
			m_pRegionArray[rx][ry] = std::make_unique<MapRegion>(this, rx, ry);
		}
		return m_pRegionArray[rx][ry].get();
	}
	return nullptr;
}
void MapBase::InitRegion() {
	m_regionNumX = (m_nWidth + REGION_SIZE - 1) / REGION_SIZE;
	m_regionNumY = (m_nHeight + REGION_SIZE - 1) / REGION_SIZE;
	m_pRegionArray.resize(m_regionNumX);
	for (int32_t x = 0; x < m_regionNumX; ++x) {
		auto& row = m_pRegionArray[x];
		row.resize(m_regionNumY);
	}
}

void MapBase::InitCell(CSvrMapHeader* map_header, DWORD dwMapFileID)
{
	auto it = m_walkablePoints.find(dwMapFileID);
	if (it == m_walkablePoints.end())
	{
		std::vector<Point> points;
		it = m_walkablePoints.emplace(dwMapFileID, points).first;
		auto cellIt = m_mapCells.emplace(dwMapFileID, std::make_shared<std::vector<std::vector<std::unique_ptr<MapCellData>>>>()).first;

		MapCellData* pMapCellDataInfo = (MapCellData*)((BYTE*)map_header + sizeof(CSvrMapHeader));
		cellIt->second->resize(m_nWidth);
		for (PosType i = 0; i < m_nWidth; ++i) {
			cellIt->second->at(i).resize(m_nHeight);
			for (PosType j = 0; j < m_nHeight; ++j) {
				MapCellData flag_data;
				flag_data.flags = pMapCellDataInfo[i * m_nHeight + j].flags;
				if (flag_data.walkable || flag_data.mining_allowed) {
					cellIt->second->at(i).at(j) = std::make_unique<MapCellData>();
					cellIt->second->at(i).at(j)->flags = flag_data.flags;
					it->second.emplace_back(i, j);
				}
			}
		}
	}

}
void MapBase::InitCloneCell(const MapBase* pCopySrcMap)
{

}

bool MapBase::InitMapCells(CSvrMapHeader* map_header, DWORD dwMapFileID) {
	if (map_header->wWidth <= 0 || map_header->wHeight <= 0) return false;
	m_nWidth = map_header->wWidth;
	m_nHeight = map_header->wHeight;
	assert(m_nWidth && m_nHeight);
	InitRegion();
	InitCell(map_header, dwMapFileID);
	return true;
}

bool MapBase::LoadMapData(const char* szMapFile, DWORD dwMapFileID)
{
	m_mapFileId = dwMapFileID;
	if (szMapFile==nullptr)
	{
		return false;
	}
	HANDLE hMapFile = CreateFileA(szMapFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hMapFile == INVALID_HANDLE_VALUE)
		return false;
	int nFileSize = GetFileSize(hMapFile, NULL);

	HANDLE hMapHandle = CreateFileMapping(hMapFile, NULL, PAGE_READONLY, 0, nFileSize, NULL);
	if (hMapHandle == 0) {
		CloseHandle(hMapFile);
		return false;
	}
	BYTE* pMap = (BYTE*)MapViewOfFile(hMapHandle, FILE_MAP_READ, 0, 0, nFileSize);
	if (pMap == nullptr)
	{
		CloseHandle(hMapFile);
		return false;
	} 
	CSvrMapHeader* pMapHeader = (CSvrMapHeader*)pMap;
	if (pMapHeader->wWidth * pMapHeader->wHeight * sizeof(MapCellData) + sizeof(CSvrMapHeader) > (DWORD)nFileSize){
		CloseHandle(hMapFile);
		UnmapViewOfFile(pMap);
		return false;
	}
	if (!InitMapCells(pMapHeader, dwMapFileID)) {
		CloseHandle(hMapFile);
		UnmapViewOfFile(pMap);
		return false;
	}
	UnmapViewOfFile(pMap);
	CloseHandle(hMapHandle);
	return true;
}


MapCellData* MapBase::GetMapCellInfo(PosType nX, PosType nY)
{
	if ((nX >= 0) && (nX < m_nWidth) && (nY >= 0) && (nY < m_nHeight)) {
		if (!m_mapCellInfo)
		{
			auto it = m_mapCells.find(m_mapFileId);
			if (it!= m_mapCells.end())
			{
				m_mapCellInfo = it->second;
			}

		}
		if (m_mapCellInfo) return m_mapCellInfo->at(nX).at(nY).get();
	}
	return nullptr;
}

MapCellObject* MapBase::GetCellObjects(PosType nX, PosType nY) const
{
	auto it = m_pMapCellInfo.find({ nX,nY });
	if (it!=m_pMapCellInfo.end())
	{
		return it->second.get();
	}
	return nullptr;
}