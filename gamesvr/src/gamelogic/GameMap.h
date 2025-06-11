#pragma once

#include <tuple>
#include <event/EventDispatcher.h>

#include "JsonStruct.h"
#include "qglobal.h"
#include "LocalDB.h"
#include "MapEvent.h"
#include "lua_base.h"
#include "MapBase.h"
#include "quest.h"
#include "Script.h"
#include "stl/SyncHashMap.h"

#pragma warning(disable : 4200)


class CMapItemEvent;
class CMapMagicEvent;
class CGameMap;
class CMonster;

struct stGateObj{
	CGameMap* DMap;
	int dwmapid;
	int nDx;
	int nDy;
	int nscriptidx;
	stGateObj(){
		ZEROSELF;
	}
};

struct stMapProperty{
	union{
		DWORD mapproperty;
		struct{								//都是!=0
			BYTE bitPkNoSword:1;			//PK不加PK值
			BYTE bitDieNoDropEquip:1;		//死亡不掉身上装备
			BYTE bitDieNoDropAll:1;			//死亡不掉所有装备
			BYTE bitNoSafeZone:1;			//没有安全区
			BYTE bitNoOffLineSaveHome:1;	//下线回城
			BYTE bitNoSpaceExceptHome:1;	//禁止除了回城的传送
			BYTE bitNoSpaceAll:1;			//禁止所有传送
			BYTE bitNoUseItem:1;			//禁止使用物品
			BYTE bitNoForcePk:1;			//禁止PK
			BYTE bitNoSendMail : 1;			//禁止被击杀邮件
			BYTE bitNoSendNotice : 1;			//禁止被击杀公告
			BYTE bitNoDropYuanShen : 1;			//死亡不掉元神觉醒
			BYTE bitNoMosterDieDrop : 1;			//打怪爆率属性在地图内不生效
			BYTE bitNoCleaner : 1;			//不能使用吸尘器
			BYTE bitNoBianShi : 1;			//不触发鞭尸
			BYTE bitNoDropDown : 1;			//掉落衰减不生效
			BYTE bitMoveRecord : 1;			//传送记录
			BYTE bitNoRandomStone : 1;			//不能使用随机石
			BYTE bitNoHomeStone : 1;			//不能使用回城石
			BYTE bitNoRelive : 1;			//复活戒指不生效
			BYTE bitNoXiaoFeiXie : 1;			//不能使用小飞鞋
			BYTE bitNoResNeng : 1;			//不消耗 剑师能量
		};
	};
	stMapProperty(){
		mapproperty=0;
	}
};

enum eGameDir{
	DR_NULL=0,
	DR_UP= 1,
	DR_RIGHT= 2,
	DR_DOWN= 4,
	DR_LEFT= 8,
	DR_UPRIGHT= (DR_UP | DR_RIGHT),
	DR_UPLEFT= (DR_UP | DR_LEFT),
	DR_DOWNRIGHT= (DR_DOWN | DR_RIGHT),
	DR_DOWNLEFT= (DR_DOWN | DR_LEFT),
	DR_MID=0x0f,		//中间 所有方向
};

enum eGameDirIndex{
	DRI_UP = 0,			//上 (以屏幕)
	DRI_UPRIGHT = 1,	//右上
	DRI_RIGHT = 2,		//右
	DRI_DOWNRIGHT = 3,	//右下
	DRI_DOWN = 4,		//下
	DRI_DOWNLEFT = 5,	//左下
	DRI_LEFT = 6,		//左
	DRI_UPLEFT = 7,		//左上
	DRI_NUM = 8,		//任意方向
};

extern eGameDirIndex gameDirIndexs[];	//DR_UP 方向转换成 0~7
extern const eGameDir gameDirs[];	//0~7 转化成方向 DR_UP
extern const char* szIndexDir[];		//0~7 转发成 ↑

extern const BYTE maxPlayerViewRangeX;
extern const BYTE maxPlayerViewRangeY;

extern const BYTE maxOnlineRange;

extern const BYTE nDelaySearchCretCount;
extern const BYTE nDelaySearchValue;

extern const BYTE iSceneW;
extern const BYTE iSceneH;


class CCreature;
class CPlayerObj;
class CNpcObj;
class CPetObj;
class CRobot;

#define MAX_CELL_COUNT			15
#define MAX_CELL_ITEMCOUNT		5

enum emBattleCamp{
	emBattle_Null,		//没有阵营
	emBattle_Neutral,	//中立阵营
	emBattle_Red,		//红方
	emBattle_Blue,		//蓝方
	emBattle_Max,		//最大阵营
};

struct stSvrMapId {
#pragma pack(push, 1)
	struct my_union {
		uint32_t mapid;
		uint8_t  line;
		uint16_t  cloneId;
		uint8_t reserve;
	};
#pragma pack(pop)
	union {
		my_union part;
		int64_t all;
	};

	stSvrMapId() : all(0) {}
	stSvrMapId(int64_t all_) : all(all_) {}
	stSvrMapId(uint32_t mapid, uint8_t line, uint16_t cloneId = 0, uint8_t reserve = 0)
		: part{ mapid, line, cloneId, reserve } {
	}
	// 必须显式定义 operator==
	bool operator==(const stSvrMapId& other) const {
		return all == other.all;
	}
};

template<>
struct std::hash<stSvrMapId> {
	size_t operator()(const stSvrMapId& k) const noexcept {
		return hash<int64_t>{}(k.all);
	}
};

class CGameMap:public MapBase
{
	void OnEnterMapNotify(MapObject* obj, MapObject* notifyObj) override;
	void OnLeaveMapNotify(MapObject* obj, MapObject* notifyObj) override;
	void OnMapMoveNotify(MapObject* obj, MapObject* notifyObj) override;
	void OnLeaveMap(MapObject* obj) override;
	void OnEnterMap(MapObject* obj) override;
	void OnMapMove(MapObject* obj) override;
public:
	CGameMap(const CGameMap& other) = delete;
	CGameMap(CGameMap&& other) noexcept = delete;
	CGameMap& operator=(const CGameMap& other) = delete;
	CGameMap& operator=(CGameMap&& other) noexcept = delete;
	CGameMap(std::shared_ptr<stServerMapInfo>& pinfo, stSvrMapId mapfullid);
	~CGameMap() override;
	
	bool Init(DWORD dwMapFileID);
	bool InitCloneMap(const CGameMap* pCopySrcMap);
	bool Clone(const CGameMap* pCopySrcMap=NULL);
	static bool IsPlayer(const MapObject* obj);
	Point GetRandomPointInMap() const;
	bool isSameCross() const;
	DWORD getSvrIdType() const;
	DWORD getMapScriptId() const;
	DWORD getMapId() const;
	DWORD getMapCloneId() const;
	DWORD getMapUnionId();//合并了MAPID和CLONEID
	int64_t getFullMapId() const;
	int getViewChangeX() const;
	int getViewChangeY() const;
	const char* getMapName() const;
	const char* getFullMapName();
	void setFullMapName(const char* pname);
	BYTE getSafeType(int nX,int nY);
	DWORD getHomeMapId();
	DWORD getHomeX();
	DWORD getHomeY();
	const char* getMapShowName(char* szbuffer,int nmaxlen);
	const mydb_mapgate_tbl* GetGate(PosType nX, PosType nY);
	void AddGate(int nX, int nY);

	//////////////////////////////////////////////////////////////////////////
	bool CheckCanWalkWithOtherSvr(int nX, int nY,int nZ,bool boNotCheckObj=false,CCreature* Target=NULL);
	bool CanWalk(int nX, int nY,int nZ, bool boNotCheckObj=false);//boNotCheckObj false检查坐标是否有人或者怪
	bool CanWalk(CCreature* Target,int nX, int nY,int nZ, bool boNotCheckObj=false);//boNotCheckObj false检查坐标是否有人或者怪
	static CMonster* CreateMonsterByType(int dwGenId, std::shared_ptr<stMonsterDataBase>& pMoninfo, int nType, Point point);
	void AddGenMonsters(mydb_mongen_tbl* mongen, bool checkObj,int count);
	void AddMonsters(uint32_t monsterId, PosType x, PosType y, PosType range, int count,uint32_t genId = 0);
	Point RandomXYInRange(PosType nX, PosType nY, int nRange, bool boNoCheckObj=false);
	bool IsMapCanPass(int nx, int ny);

	CMonster* LuaGetOneMonsterNear(CPlayerObj* p);
	bool GetRandValidPosInRange(int32_t& dx, int32_t& dy, int32_t range, int32_t checkObjCount);
	CCreature* AddMonster(std::shared_ptr<stMonsterDataBase>& monsterDb, PosType x, PosType y, PosType range, uint32_t genId = 0, uint32_t guildId = 0, bool isFixedPoint = false);
	CNpcObj* AddNpc(mydb_npcgen_tbl& pNpcInfo);
	CPetObj* RegenPet(DWORD dwMonid,DWORD dwLevel,int nsrcX,int nsrcY,CPlayerObj *p=NULL);
	CRobot* AddRobotMon(stLoadPlayerData* pGameData, bool isNeedLoadData, DWORD dwEffectId = 0);
	void run() override;		//
	bool GetNextPosition(int nX,int  nY,BYTE btDir, int nSetp, int  &snx, int &sny);
	static BYTE GetNextDirection(int nSx,int nSy,int nDx,int nDy);
	static BYTE GetNextDirection(BYTE btdir,int nTurnCount=1);
	static BYTE GetReverseDirection(BYTE btdir);
	static const char* GetDirDis(BYTE btdir);
	//////////////////////////////////////////////////////////////////////////
	int GetMaxRange(int nSX,int  nSY);

	bool IsInRangeXY(int nSX,int  nSY,int  nRange,int  nDx, int nDy);
	std::tuple<int, int> GetMapObjectCount(PosType nX, PosType nY) const;
	bool GetSpiralDropPosition(PosType pos_x, PosType pos_y, int drop_count, std::vector<Point>& points);
	bool GetNearXY(int nSX,int  nSY, PosType&nDx, PosType&nDy,int nRange, int nCheckObjCount=0);
	bool GetNearNoCretXY(int nSX,int  nSY,int  &nDx, int &nDy,int nRange, int nCheckObjCount=0);
	bool GetRandXYInRange(PosType& nDx, PosType& nDy,int nInRange,int nOutRange);

	bool GetSuiJiJiaoBenFanWei(int  &nDx, int &nDy,int nInRange,int nOutRange);

	bool LuaGetRandXYTableInRange(sol::table table,int nTableSize,int nDx,int nDy,int nInRange,int nOutRange);
	bool GetRandXYInMap(PosType &nnx, PosType &nny, int nCheckObjCount=1 ,int nLoop=200);
	bool LuaGetXYInMap();
	std::vector<CCreature*> GetCretInRangeAll(int nX, int nY, DWORD dwTmpID, int range=0, bool requireAlive=false) const;

	CCreature* GetCretInXY(int nX,int  nY,bool bodie=false);	//获取一个角色(false 未死  ture已死)
	CCreature* GetCretInRange(int nX,int nY,DWORD dwTmpID,int nRange=0,bool bodie=false);
	int GetCretsInMagicRange(int nX,int nY,int nDir,int nDX,int nDY,std::vector<CCreature*>& v,DWORD TargetId, const std::shared_ptr<stMagicDataBase>& pMagicData,CCreature* pAttack,int nMax=MAXLONG,DWORD dwTmpID=0,bool bodie=false );
	int GetCretCount(CCreature* Target, int nX, int nY, int nMax = MAXLONG);
	bool SpeclaiMagicRange(int nX, int nY, int nDir, int nDX, int nDY, std::vector<CCreature*>& v, DWORD TargetId, const std::shared_ptr<stMagicDataBase>& pMagicData, CCreature* pAttack, int nMax = MAXLONG, DWORD dwTmpID = 0, bool bodie = false);
	
	CMapItemEvent* GetItemInXY(int nX, int  nY, int64_t itemid=0);	//获取一个物品(false 未死  ture已死)

	bool AddItemToMap(int nX,int nY,CMapItemEvent* ptmpMapItem);
	bool DelItemFromMap(int nX,int nY,CMapItemEvent* ptmpMapItem);

	bool AddMagicToMap(int nX,int nY,CMapMagicEvent* ptmpMapMagic);
	bool DelMagicFromMap(int nX,int nY,CMapMagicEvent* ptmpMapMagic);
	bool MoveCretTo(CCreature* pCret, int nDx, int nDy, int nDz, bool boNotCheckObj);
	CCreature* LuaAddMonster(const sol::table& table);
	CPlayerObj* GetPlayer(int32_t objectId);
	CCreature* GetCreature(int32_t objectId);
	CCreature* FindCretByTmpId(DWORD dwTmpid, int nX, int  nY);

	void SendMsgToMapAllUser(void *pbuf,int nLen);					//给地图上的所有玩家发信息
	void SetNpcEmblemId(DWORD dwEmblemId);							//设置npc军团徽章
	void ResetMapPop() {m_mapPop.mapproperty= GetMapDataBase()->dwMapProperty;};
	bool SetMapPop(const char* szMapStr,bool boAdd=false);
	bool isPkNoSword() const {return (m_mapPop.bitPkNoSword!=0);};					//PK不加PK值
	bool isDieNoDropEquip() const {return (m_mapPop.bitDieNoDropEquip!=0);};		//死亡不掉身上装备
	bool isDieNoDropAll() const {return (m_mapPop.bitDieNoDropAll!=0);};			//死亡不掉所有装备
	bool isNoSafeZone() const {return (m_mapPop.bitNoSafeZone!=0);};				//没有安全区
	bool isNoOffLineSaveHome() const {return (m_mapPop.bitNoOffLineSaveHome!=0);};	//下线回城
	bool isNoSpaceExceptHome() const {return (m_mapPop.bitNoSpaceExceptHome!=0);};	//禁止除了回城的传送
	bool isNoSpaceAll() const {return (m_mapPop.bitNoSpaceAll!=0);};				//禁止所有传送
	bool isNoUseItem() const {return (m_mapPop.bitNoUseItem!=0);};					//禁止使用物品
	bool isNoForcePk() const {return (m_mapPop.bitNoForcePk!=0);};					//禁止PK
	bool isNoSendMail() const { return (m_mapPop.bitNoSendMail != 0); };			//禁止被击杀邮件
	bool isNoSendNotice() const { return (m_mapPop.bitNoSendNotice != 0); };			//禁止被击杀公告
	bool isNoDropYuanShen() const { return (m_mapPop.bitNoDropYuanShen != 0); };			//死亡不掉元神觉醒
	bool isNoMosterDieDrop() const { return (m_mapPop.bitNoMosterDieDrop != 0); };			//打怪爆率属性在地图内不生效
	bool isNoCleaner() const { return (m_mapPop.bitNoCleaner != 0); };			//不能使用吸尘器
	bool isNoBianShi() const { return (m_mapPop.bitNoBianShi != 0); };			//不触发鞭尸
	bool isNoDropDown() const { return (m_mapPop.bitNoDropDown != 0); };			//掉落衰减不生效
	bool isMoveRecord() const { return (m_mapPop.bitMoveRecord != 0); };			//传送记录
	bool isNoRandomStone() const { return (m_mapPop.bitNoRandomStone != 0); };			//不能使用随机石
	bool isNoHomeStone() const { return (m_mapPop.bitNoHomeStone != 0); };			//不能使用回城石
	bool isNoRelive() const { return (m_mapPop.bitNoRelive != 0); };					//复活特戒不生效
	bool isNoXiaoFeiXie() const { return (m_mapPop.bitNoXiaoFeiXie != 0); };			//不能使用小飞鞋
	bool isNoResNeng() const { return (m_mapPop.bitNoResNeng != 0); };				//不消耗剑师能量
	//////////////////////////////////////////////////////////////////////////
	bool isCrossMap() const { return (GetMapDataBase()->CrossRefreshMon != 0); };			
	void mapGroupAddNum(CPlayerObj* player);
	void mapGroupRemoveNum(CPlayerObj* player);
	DWORD getMapGroupNum(DWORD dwGroupId);
	bool GetGroupPlayer(DWORD dwGroupId, std::unordered_set<CPlayerObj*>& playerSet);
	double quest_vars_get_var_n(const std::string& name);
	bool quest_vars_set_var_n(const std::string& name, double value,bool needsave);
	bool isPeakDuelMap();
	int GetMonCountById(int monId) const;
	bool sendMapTipMsg(const char* pszMsg);
	void InitEvent();
	void GenNpc();
	void AddDelayedMonsterSpawn(int monGenId, int delaytime);
	void RunMonGen();
	void GetValidRandomPosition(PosType& x, PosType& y) const;

	char m_szMapName[_MAX_NAME_LEN_];
	int m_nLuaX;
	int m_nLuaY;
	CScriptTimerManager* m_Timer;	//地图计时器
	const stSvrMapId m_svrMapId;
	mutable std::weak_ptr<stServerMapInfo> m_mapDataBase;			  //mapinfo表中的数据
	std::shared_ptr<stServerMapInfo> GetMapDataBase() const;
	stMapProperty m_mapPop;					//地图的基本属性
	DWORD m_dwCloneMapExistTime;		//副本地图存活时间
	bool m_boClearMon;				//清怪标记
	CVars m_vars;
	DWORD m_dwCloneMapType;			//副本类型
	std::HashMap<DWORD, std::unordered_set<CPlayerObj*>> m_groupMembers;
	ULONGLONG m_nextRunTickCount;
private:
	TimerSystem timer_;
	std::time_t m_lastSpawnTime = 0;
	std::set<int> m_spawnMonsters;
};


class CMapHashManager
{
public:
	CMapHashManager() { m_nextRunTickCount = 0; }
	void run(time_t curTime);
	void ClearCloneMap();
	template <typename Func>
	void s_ForEach(Func&& func);
	CGameMap* FindById(uint32_t mapid, BYTE lineId, uint16_t cloneId=0) {
		lineId = lineId == 0 ? GameService::getMe().m_lineId : lineId;
		return m_gameMaps.s_find({mapid,lineId,cloneId});
	}
	CGameMap* FindByFullId(stSvrMapId tmpId) {
		return m_gameMaps.s_find(tmpId);
	}
	CGameMap* FindByOnlyId(int64_t onlyId) {
		return m_gameMaps.s_find(stSvrMapId(onlyId));
	}
	void s_clear() {
		m_gameMaps.s_clear();
	}
	ULONGLONG m_nextRunTickCount;
	std::SyncHashMap<stSvrMapId, CGameMap*> m_gameMaps;
};

template <typename Func>
void CMapHashManager::s_ForEach(Func&& func)
{
	m_gameMaps.s_ForEach(std::forward<Func>(func));
}
