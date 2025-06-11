#pragma once

#include "MapObject.h"
#include <atomic>
#include "stl/HashMap.h"
#include "utils/define.h"
#include "MapRegion.h"
#include <functional>
#include <memory>
#include <SyncList.h>
#include <tuple>
#include "MapDef.h"

#pragma pack(push,1)
struct MapCellData {
	union {
		BYTE flags;
		struct {
			BYTE walkable : 1;
			BYTE safe_zone : 1;		//
			BYTE transparent : 1;
			BYTE mining_allowed : 1;
			BYTE is_gate : 1;
			BYTE reserved1 : 1;
			BYTE spawn_group : 1;	//用于刷怪分组
		};
	};
	MapCellData() : flags(0) {}
};
#pragma pack(pop)

struct stMapBaseDelObj
{
	stMapBaseDelObj(MapObject* obj, unsigned char curround, unsigned char allround)
		: obj(obj),
		  curround(curround),
		  allround(allround)
	{
	}

	MapObject* obj;
	unsigned char curround;
	unsigned char allround;
};

enum class VisibilityEvent:uint8_t
{
	Entered,
	Exited,
	InView
};
class MapCellObject;
class MapBase
{
	using MapCellDataPtr = std::shared_ptr<std::vector<std::vector<std::unique_ptr<MapCellData>>>>;

#pragma pack(push,1)
	struct CSvrMapHeader {
		char mark[4];
		float fmapver;
		char sDesc[32];
		WORD wAttr;
		WORD wWidth;
		WORD wHeight;
		WORD wHigh;
		MapCellData flagdata[0];
		CSvrMapHeader() {
			ZeroMemory(this, sizeof(CSvrMapHeader));
		}
	};
#pragma pack(pop)
protected:
	std::atomic_uint32_t m_genObjId;
	std::HashMap<unsigned int, MapObject*> m_objMap;
	std::vector<std::HashMap<unsigned int, MapObject*>> m_typeObjMap;
	std::CSyncQueue<MapObject*> m_enterQueue;
	std::list<stMapBaseDelObj> m_delHash;
	std::atomic_bool m_isActive;
	inline static thread_local std::vector<MapRegion*> entered_regions;
	inline static thread_local std::vector<MapRegion*> exited_regions;
	inline static thread_local std::vector<MapRegion*> syncing_regions;
public:
	void ActiveRegion(MapRegion* region);
	void DeactivateRegion(MapRegion* region);
	void GetAllRegions(const MapRegion* cur_region, const MapRegion* next_region);
	void EnterNotify(MapObject* obj, bool isTrigger, int8_t objType);
	bool DoMapMove(MapObject* obj, PosType pos_x, PosType pos_y);
	bool DoLeaveMap(MapObject* obj);
	void LeaveNotify(MapObject* obj, bool isTrigger, int8_t objType);
	void SetCellObj(MapObject* obj);
	void DelCellObj(MapObject* obj);

	MapBase(std::string_view name);
	MapBase(const MapBase& other) = delete;
	MapBase& operator=(const MapBase& other) = delete;
	MapBase(MapBase&& other) = delete;
	MapBase& operator=(MapBase&& other) = delete;
	virtual ~MapBase();
	int GetObjsCount() const { return static_cast<int>(m_objMap.size()); }
	bool IsActive();
	bool PushToDel(MapObject* obj, unsigned char afterRound = 1);
	void ProcessDelList();
	std::vector<MapObject*> GetObjsByType(unsigned char objType) const;
	MapObject* GetObjByTypeCondition(unsigned char objType, const std::function<bool(MapObject*)>& predicate) const;

	template<typename Func>
	void ForeachObjectsByType(unsigned char objType, Func&& func) const;
	template <class Func>
	void ForeachVisualObjects(MapObject* curObj, Func&& func);
	template <class Func>
	void ForeachVisualObjects(PosType x, PosType y, Func&& func);

	std::vector<MapObject*> GetObjectsByTypeCondition(unsigned char objType, const std::function<bool(MapObject*)>& predicate) const;
	uint32_t GetObjCountByType(unsigned char objType) const;
	MapObject* GetObjById(unsigned int objid);
	MapObject* GetObjByTypeId(unsigned char objType, unsigned int objid);
	MapCellData* GetMapCellInfo(PosType nX, PosType nY);
	MapCellObject* GetCellObjects(PosType nX, PosType nY) const;
	MapRegion* GetRegionByPos(PosType px, PosType py);       //根据坐标来获取区域
	std::tuple<MapRegion*, int, int> GetRegion(int px, int py);
	MapRegion* GetRegionByObj(MapObject* obj);
	bool DoEnterMap(MapObject* obj);
	bool EnterMap(MapObject* obj);
	unsigned int GenObjId();

	virtual void OnEnterMap(MapObject* obj);
	virtual void OnEnterMapNotify(MapObject* obj, MapObject* notifyObj) {}
	virtual void OnMapMove(MapObject* obj) {}
	virtual void OnMapMoveNotify(MapObject* obj, MapObject* notifyObj) {}
	virtual void OnLeaveMap(MapObject* obj);
	virtual void OnLeaveMapNotify(MapObject* obj, MapObject* notifyObj) {}
	virtual void run();
	virtual bool isThisSvr() const { return false; }
	PosType m_nWidth;
	PosType m_nHeight;
	std::string m_name;
private:
	void ClearObjMap();
	void ProcessEnterQueue();
	//------------------------------------region-------------------------------------------------------//
protected:
	std::unordered_map<Point, std::unique_ptr<MapCellObject>> m_pMapCellInfo;
	std::vector<std::vector<std::unique_ptr<MapRegion>>> m_pRegionArray;
	int32_t m_regionNumX;
	int32_t m_regionNumY;
	MapRegion* m_pActiveRegion;                                 //激活状态的区域,需要确认是否是有人物在视野内才激活
	uint32_t m_mapFileId;
	MapCellDataPtr m_mapCellInfo;

	MapRegion* GetRegionByRegionPos(int rx, int ry);     //根据region坐标来获取区域
	void InitRegion();
	void InitCell(CSvrMapHeader* map_header, DWORD dwMapFileID);
	void InitCloneCell(const MapBase* pCopySrcMap);
	bool InitMapCells(CSvrMapHeader* map_header, DWORD dwMapFileID);
	bool LoadMapData(const char* szMapFile, DWORD dwMapFileID);

	inline static std::HashMap<int, std::vector<Point>> m_walkablePoints;
	inline static std::HashMap<int, MapCellDataPtr> m_mapCells;
};

template <typename Func>
void MapBase::ForeachObjectsByType(unsigned char objType, Func&& func) const
{
	for (auto it : this->m_typeObjMap[objType])
	{
		func(it.second);
	}
}


template <typename Func>
void MapBase::ForeachVisualObjects(MapObject* curObj, Func&& func)
{
	ForeachVisualObjects(curObj->GetX(), curObj->GetY(), std::forward<Func>(func));
}

template <typename Func>
void MapBase::ForeachVisualObjects(PosType x,PosType y, Func&& func)
{
	auto region = GetRegionByPos(x,y);
	if (!region) return;
	for (auto& [x, y] : commonAoi)
	{
		if (MapRegion* r = GetRegionByRegionPos(region->m_regionX + x, region->m_regionY + y))
		{
			r->ExecuteAllObjects(std::forward<Func>(func));
		}
	}
}
