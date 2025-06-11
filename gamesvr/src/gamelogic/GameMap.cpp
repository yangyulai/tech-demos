#include "GameMap.h"
#include "stringex.h"
#include "timeMonitor.h"
#include "UsrEngn.h"
#include "../gamesvr.h"
#include "../dbsvrGameConnecter.h"
#include "MonsterObj.h"
#include "Npc.h"
#include "MagicRange.h"
#include "MagicExtend.h"
#include "Chat.h"
#include "JsonConfig.h"
#include "MapCell.h"
#include "MapMagicEvent.h"
#include "MapItemEvent.h"
#include "Robot.h"
eGameDirIndex gameDirIndexs[16];
const eGameDir gameDirs[]={ DR_UP,DR_UPRIGHT,DR_RIGHT,DR_DOWNRIGHT,DR_DOWN,DR_DOWNLEFT,DR_LEFT,DR_UPLEFT };
const char* szIndexDir[]={"↑","↑→","→","→↓","↓","←↓","←","←↑"};

const BYTE maxOnlineRange=16;

const BYTE nDelaySearchCretCount=50;
const BYTE nDelaySearchValue=10;

const BYTE maxPlayerViewRangeX=12;
const BYTE maxPlayerViewRangeY=12;

struct stAutoFullDirIndexs{
	stAutoFullDirIndexs(){
		memset(gameDirIndexs,DRI_DOWN,sizeof(gameDirIndexs));
		gameDirIndexs[DR_UP]=DRI_UP;
		gameDirIndexs[DR_UPRIGHT]=DRI_UPRIGHT;
		gameDirIndexs[DR_RIGHT]=DRI_RIGHT;
		gameDirIndexs[DR_DOWNRIGHT]=DRI_DOWNRIGHT;
		gameDirIndexs[DR_DOWN]=DRI_DOWN;
		gameDirIndexs[DR_DOWNLEFT]=DRI_DOWNLEFT;
		gameDirIndexs[DR_LEFT]=DRI_LEFT;
		gameDirIndexs[DR_UPLEFT]=DRI_UPLEFT;
	}
};
stAutoFullDirIndexs g_autofulldirindexs;

CGameMap::CGameMap(std::shared_ptr<stServerMapInfo>& pinfo,stSvrMapId mapfullid)
:MapBase(pinfo->name)
,m_svrMapId(mapfullid)
, m_mapDataBase(pinfo)
{
	FUNCTION_BEGIN;
	m_typeObjMap.resize(CRET_COUNT);
	m_nLuaX=0;
	m_nLuaY=0;
	m_nextRunTickCount = GetTickCount64() + _random(1000);
	m_mapPop.mapproperty= GetMapDataBase()->dwMapProperty;
	m_boClearMon=false;
	m_szMapName[0]=0;
	if (pinfo->szMapFileName[0] == 0)
	{
		sprintf_s((char*)pinfo->szMapFileName.c_str(), sizeof(pinfo->szMapFileName) - 1, "%u", getMapId());
	}
	m_Timer = CLD_DEBUG_NEW CScriptTimerManager(NULL,NULL);//创建定时器
	m_dwCloneMapType = 0;
	InitEvent();
}

CGameMap::~CGameMap(){
	FUNCTION_BEGIN;
	CUserEngine::getMe().putMapCloneid(getMapCloneId());
	SAFE_DELETE(m_Timer);//释放计时器
}

void CGameMap::InitEvent()
{
	timer_.AddTimer(1000, [this]()
		{
			if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("MapRunonemiao", this);
			}
		});
	timer_.AddTimer(2000, [this]()
		{
			RunMonGen();
			if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("MapRuntomiao", this);
			}
		});
	timer_.AddTimer(10000, [this]()
		{
			if (CUserEngine::getMe().m_scriptsystem.m_LuaVM) {
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("MapRuntenmiao", this);
			}
		});
}
void CGameMap::GenNpc()
{
	for (auto& data : sJsonConfig.npc_data)
	{
		if (!data.deleted && data.mapId == getMapId())
		{
			AddNpc(data);
		}
	}
}
void CGameMap::AddDelayedMonsterSpawn(int monGenId, int delaytime)
{
	if (!monGenId)
		return;
	auto gen = sJsonConfig.GetMonGenData(monGenId);
	if (!gen) return;
	auto gen_time = gen->dwGenTime;
	delaytime += gen_time;
	if (const auto monDb = sJsonConfig.GetMonsterDataBase(gen->dwMonsterId)){
		if (monDb->Isboss)
			CUserEngine::getMe().m_bossRefreshTime.entries[monDb->nID] = time(NULL) + delaytime/1000;
	}
	timer_.AddTimer(delaytime, [this, gen]() {
		AddGenMonsters(gen, true, 1);
		},false);
}
void CGameMap::RunMonGen()
{
	if (m_lastSpawnTime >= sJsonConfig.last_spawn_time)
		return;
	m_lastSpawnTime = sJsonConfig.last_spawn_time;

	std::set<int> tmp;
	std::set<int> diff;
	for (auto& data:sJsonConfig.spawn_data)
	{
		if (data.dwMapId == getMapId())
		{
			tmp.emplace(data.dwGenId);
		}
	}
	std::set_difference(tmp.begin(), tmp.end(), m_spawnMonsters.begin(), m_spawnMonsters.end(), std::inserter(diff, diff.begin()));
	for (auto it : diff)
	{

		auto gen = sJsonConfig.GetMonGenData(it);
		if (!gen) continue;
		auto pMonData = sJsonConfig.GetMonsterDataBase(gen->dwMonsterId);
		if (!pMonData)
			return;
		AddGenMonsters(gen, true,gen->nCount);
		m_spawnMonsters.insert(it);
	}
}
void CGameMap::GetValidRandomPosition(PosType& x,PosType& y) const
{
	auto point = GetRandomPointInMap();
	x = point.x;
	y = point.y;
}

std::shared_ptr<stServerMapInfo> CGameMap::GetMapDataBase() const
{
	if (auto ptr = m_mapDataBase.lock()) {
		return ptr;
	}
	auto fresh = sJsonConfig.GetMapDataBase(getMapId());
	if (!fresh)
	{
		g_logger.error("获取地图数据失败, 物品ID: %d", getMapId());
		return nullptr;
	}
	m_mapDataBase = fresh;
	return fresh;
}

Point CGameMap::GetRandomPointInMap() const
{
	auto walkablePoints = m_walkablePoints.find(GetMapDataBase()->dwMapFileID);
	if (walkablePoints != m_walkablePoints.end())
	{
		auto& points = walkablePoints->second;
		if (!points.empty())
		{
			int randomIndex = _random(0, points.size() - 1);
			return points[randomIndex];
		}
	}
	assert(false);
	return {0,0};
}
bool CGameMap::isSameCross() const
{
	if (CUserEngine::getMe().isCrossSvr()) {
		if (GetMapDataBase()->CrossRefreshMon) {
			return true;
		}
	}
	else {
		if (!GetMapDataBase()->CrossRefreshMon) {
			return true;
		}
	}
	return false;
}
DWORD CGameMap::getSvrIdType() const
{
	return MAKELONG(GetMapDataBase()->nServerIndex , _GAMESVR_TYPE );
}

DWORD CGameMap::getMapScriptId() const
{
	return GetMapDataBase()->dwMapscriptid;
}

DWORD CGameMap::getMapId() const
{
	return m_svrMapId.part.mapid;
}

DWORD CGameMap::getMapCloneId() const
{
	return m_svrMapId.part.cloneId;
}

DWORD CGameMap::getMapUnionId(){
	DWORD dwmapid=getMapId();
	DWORD dwcloneid=getMapCloneId();
	return (dwcloneid<<16 & 0xffff0000) | (dwmapid & 0x0000ffff);
}

int64_t CGameMap::getFullMapId() const
{
	return m_svrMapId.all;
}

const char* CGameMap::getMapName() const
{
	return GetMapDataBase()->name.c_str();
}

int CGameMap::getViewChangeX() const
{
	return GetMapDataBase()->nViewChangeX;
}

int CGameMap::getViewChangeY() const
{
	return GetMapDataBase()->nViewChangeY;
}

const char* CGameMap::getFullMapName(){
	FUNCTION_BEGIN;
	if (m_szMapName[0]==0){
		char szMapName[256]={0};
		strcpy_s(szMapName,sizeof(szMapName)-1,getMapName());
		filtershowname(szMapName,strlen(szMapName));
		strcpy_s(m_szMapName, sizeof(m_szMapName) - 1, szMapName);
	}
	return m_szMapName;
}

void CGameMap::setFullMapName(const char* pname){
	FUNCTION_BEGIN;
	if ( pname && strcmp(pname,m_szMapName)!=0 ){ strcpy_s(m_szMapName,sizeof(m_szMapName)-1,pname); }
}

BYTE CGameMap::getSafeType(int nX,int nY){
	FUNCTION_BEGIN;
	if (auto pinfo=GetMapCellInfo(nX, nY); pinfo) {
		if (pinfo->safe_zone != 0) { return 1; }
	}
	return 0;
}

DWORD CGameMap::getHomeMapId(){
	FUNCTION_BEGIN;
	return GetMapDataBase()->dwReliveMapid;
}

DWORD CGameMap::getHomeX(){
	FUNCTION_BEGIN;
	return GetMapDataBase()->dwReliveX;
}

DWORD CGameMap::getHomeY(){
	FUNCTION_BEGIN;
	return GetMapDataBase()->dwReliveY;
}

const char* CGameMap::getMapShowName(char* szbuffer,int nmaxlen){
	FUNCTION_BEGIN;
	if (szbuffer){
		strcpy_s(szbuffer,nmaxlen-1,getFullMapName());
		filtershowname(szbuffer,strlen(szbuffer));
		return szbuffer;
	}
	return getFullMapName();
}

bool CGameMap::Init(DWORD dwMapFileID){
	FUNCTION_BEGIN;
	if ( LoadMapData( vformat("%s%s.svrmap",extractfilepath(GameService::getMe().m_svrcfg.szMapDir), GetMapDataBase()->filename.c_str()), dwMapFileID)){
		g_logger.debug("地图(%s [%d : \"%s%s.svrmap\"])加载成功!", getMapName(),getMapId(),extractfilepath(GameService::getMe().m_svrcfg.szMapDir), GetMapDataBase()->filename.c_str());
		return true;
	}
	g_logger.error("地图(%s [%d : \"%s%s.svrmap\"])加载失败!", getMapName(),getMapId(),extractfilepath(GameService::getMe().m_svrcfg.szMapDir), GetMapDataBase()->filename.c_str());
	return false;
}

bool CGameMap::InitCloneMap(const CGameMap* pCopySrcMap) {
	FUNCTION_BEGIN;
	if (pCopySrcMap) {
		m_nWidth = pCopySrcMap->m_nWidth;
		m_nHeight = pCopySrcMap->m_nHeight;
		m_mapPop = pCopySrcMap->m_mapPop;
		m_mapFileId = pCopySrcMap->m_mapFileId;
		InitRegion();
		InitCloneCell(pCopySrcMap);
		return true;
	}
	return false;
}

bool CGameMap::Clone(const CGameMap* pCopySrcMap){
	FUNCTION_BEGIN;
	if ( pCopySrcMap && getMapId()==pCopySrcMap->getMapId() ){
		if (!InitCloneMap(pCopySrcMap)){ return false;}
		return true;
	}
	return false;
}
bool CGameMap::IsPlayer(const MapObject* obj)
{
	return obj->GetType() == CRET_PLAYER;
}
void CGameMap::OnEnterMapNotify(MapObject* obj, MapObject* notifyObj)
{

	auto objType = obj->GetType();
	auto notifyObjType = notifyObj->GetType();
	if (obj == notifyObj) return;
	eCretType cret_type = static_cast<eCretType>(objType);
	auto& visibilityMask = CreatureVisibilitySettings[objType];
	if (visibilityMask.test(notifyObjType))//人可见的对象需要同步给人
	{
		notifyObj->EnterMapNotify(obj);
	}
	if (IsPlayer(notifyObj))
	{
		obj->EnterMapNotify(notifyObj);
	}
}

void CGameMap::OnLeaveMapNotify(MapObject* obj, MapObject* notifyObj)
{
	auto objType = obj->GetType();
	auto notifyObjType = notifyObj->GetType();
	if (obj == notifyObj) return;
	eCretType cret_type = static_cast<eCretType>(objType);
	auto& visibilityMask = CreatureVisibilitySettings[objType];
	if (visibilityMask.test(notifyObjType))//人可见的对象需要同步给人
	{
		notifyObj->LeaveMapNotify(obj);
	}
	if (IsPlayer(notifyObj))
	{
		obj->LeaveMapNotify(notifyObj);
	}
}

void CGameMap::OnMapMoveNotify(MapObject* obj, MapObject* notifyObj)
{
	auto objType = obj->GetType();
	auto notifyObjType = notifyObj->GetType();
	if (obj == notifyObj) return;
	eCretType cret_type = static_cast<eCretType>(objType);
	auto& visibilityMask = CreatureVisibilitySettings[notifyObjType];
	if (visibilityMask.test(notifyObjType))//人可见的对象需要同步给人
	{
		notifyObj->MapMoveNotify(obj);
	}
	if (IsPlayer(notifyObj))
	{
		obj->MapMoveNotify(notifyObj);
	}
}

void CGameMap::OnLeaveMap(MapObject* obj)
{
	if (!obj || !obj->GetObjectId())return;
	MapBase::OnLeaveMap(obj);
	auto objType = obj->GetType();
	switch (eCretType cret_type = static_cast<eCretType>(objType))
	{
	case CRET_PLAYER:
	{
		if (auto player = dynamic_cast<CPlayerObj*>(obj)) {
			mapGroupRemoveNum(player);
			player->ClearDelayMsg();
			player->ClearMsg();
			if (player->IsOffline())
			{
				g_logger.debug("玩家(%s)离开地图(%s) 该玩家已经离线", player->Name().c_str(), getMapName());
				PushToDel(obj);
			}
			else if (player->m_changeMapId.all) {
				auto moveMap = CUserEngine::getMe().m_maphash.FindByFullId(player->m_changeMapId.all);
				if (moveMap) {
					g_logger.debug("%s 玩家切地图 %s 目标地图id = %d", player->getName(), getMapName(), player->m_changeMapId.part.mapid);
					player->ChangeClientLoadMapData(moveMap);
				}
				else if (sJsonConfig.GetMapDataBase(player->m_changeMapId.part.mapid)) {
					player->HandleMoveCross();
					g_logger.debug("%s 玩家切换游戏服务器 %s 目标地图id = %d 服务器id = %d", player->getName(), getMapName(), player->m_changeMapId.part.mapid, player->m_changeMapId.part.line);
				}else
					g_logger.debug("玩家(%s)离开地图错误的状态(%s)", player->Name().c_str(), getMapName());
			}
			player->m_pEnvir = nullptr;
		}
	}break;
	case CRET_MONSTER:
	case CRET_PET:
	case CRET_NPC:
	case CRET_ITEM:
	case CRET_MAGIC:
	{
		PushToDel(obj);
	}
	break;
	case CRET_NONE:
		break;
	}
	
}

void CGameMap::OnMapMove(MapObject* obj)
{
	if (obj->isPlayer()){
		if (auto player = static_cast<CPlayerObj*>(obj)) {
			player->SendPosToGroupMember();
		}
	}
}

void CGameMap::OnEnterMap(MapObject* obj)
{
	if (!obj || !obj->GetObjectId()) return;
	MapBase::OnEnterMap(obj);
	if (obj->isPlayer())
	{
		if (auto player = dynamic_cast<CPlayerObj*>(obj)) {
			g_logger.debug("玩家(%s)进入地图(%s)", player->Name().c_str(), getMapName());
			player->MapChanged();
			mapGroupAddNum(player);
			player->SendPosToRelationMember();
		}
	}
}

bool CGameMap::IsMapCanPass(int nx, int ny)
{
	if (auto pMapCellInfo = GetMapCellInfo(nx, ny); pMapCellInfo && (pMapCellInfo->walkable)) {
		return true;
	}
	return false;
}

const mydb_mapgate_tbl* CGameMap::GetGate(PosType nX, PosType nY)
{
	if (auto pMapCellInfo = GetMapCellInfo(nX, nY); pMapCellInfo)
	{
		if (!pMapCellInfo->is_gate) {
			return nullptr;
		}
		return sJsonConfig.GetGateData(getMapId(), nX, nY);
	}
	return nullptr;
}

void CGameMap::AddGate(int nX, int nY) {
	if (auto pMapCellInfo=GetMapCellInfo(nX, nY); pMapCellInfo)
	{
		pMapCellInfo->is_gate = true;
	}
}

bool CGameMap::CheckCanWalkWithOtherSvr(int nX, int nY,int nZ, bool boNotCheckObj,CCreature* Target){
	FUNCTION_BEGIN;
	if (nZ > 0){	
		return (nX>=0 && nY>=0 && nX<m_nWidth && nY<m_nHeight); 
	}
	return CanWalk(Target,nX,nY,nZ,boNotCheckObj);
}

CMonster* CGameMap::LuaGetOneMonsterNear(CPlayerObj* p) 
{
	int nMaxDist = p->quest_vars_get_var_n("AutoGuaJiRange");
	int nX = p->quest_vars_get_var_n("AutoGuaJiX");
	int nY = p->quest_vars_get_var_n("AutoGuaJiY");

	if (!nMaxDist) return nullptr;

	if (const auto center = GetRegionByPos(nX,nY)){
		int regionSize = nMaxDist*2 / REGION_SIZE + 1;
		std::vector<MapRegion*> regions;
		regions.clear();

		if(center->m_monCount>0) regions.emplace_back(center);
		for (const Point& point : MonSpiralPoints) {
			if (std::abs(point.x) > regionSize || std::abs(point.y) > regionSize) break; //超出挂机区域范围
			if (auto tmpregion = GetRegionByRegionPos(center->m_regionX + point.x, center->m_regionY + point.y)) {
				if (tmpregion->m_monCount > 0)
					regions.emplace_back(tmpregion);
			}
		}
		auto func = [nMaxDist, nX, nY](const MapObject* obj) {
			return obj->isMonster() && obj->ChebyshevDistance(nX, nY) <= nMaxDist;
		};
		for (const auto& region : regions) {
			if (auto obj = region->FindIf(func))
				return obj->toMonster();
		}
	}
	return nullptr;
}

bool CGameMap::GetRandValidPosInRange(int32_t& dx, int32_t& dy, int32_t range, int32_t checkObjCount)
{
	FUNCTION_BEGIN;
	FUNCTION_MONITOR(16, "CGameMap::GetRandValidPosInRange()");
	int32_t lx = safe_max<PosType>(dx - range, 0);
	int32_t hx = safe_min<PosType>(dx + range, m_nWidth);
	int32_t ly = safe_max<PosType>(dy - range, 0);
	int32_t hy = safe_min<PosType>(dy + range, m_nHeight);
	int32_t totalWeight = safe_max(hx - lx + 1, 1) * safe_max(hy - ly + 1, 1);
	int32_t randValue = _random(totalWeight);
	int32_t tmpDx = -1;
	int32_t tmpDy = -1;
	bool found = false;
	for (int32_t ix = lx; ix <= hx; ix++)
	{
		for (int32_t iy = ly; iy <= hy; iy++)
		{
			randValue--;
			checkObjCount--;
			if (CanWalk(ix, iy, 0, (checkObjCount <= 0)))
			{
				tmpDx = ix;
				tmpDy = iy;
				if (randValue <= 0)
				{
					found = true;
					break;
				}
			}
		}
		if (found)
		{
			break;
		}
	}
	if (tmpDx != -1 && tmpDy != -1)
	{
		dx = tmpDx;
		dy = tmpDy;
		return true;
	}
	return false;
}
bool CGameMap::CanWalk(int nX, int nY,int nZ, bool boNotCheckObj){
	return CanWalk(NULL,nX,nY,nZ,boNotCheckObj);
}

bool CGameMap::CanWalk(CCreature* Target,int nX, int nY,int nZ, bool boNotCheckObj)
{
	auto pMapCellInfo = GetMapCellInfo(nX, nY);
	if (!pMapCellInfo)
		return false;
	if (!pMapCellInfo->walkable)
		return false;
	if (!boNotCheckObj)
	{
		if (auto cellObjects = GetCellObjects(nX, nY))
		{
			return cellObjects->empty();
		}
	}
	return true;
}

Point CGameMap::RandomXYInRange(PosType nX, PosType nY, int nRange, bool boNoCheckObj)
{
	PosType nDX = nX;
	PosType nDY = nY;
	GetRandXYInRange(nDX, nDY, 0, nRange);
	GetNearXY(nDX, nDY, nDX, nDY, nRange, (nRange > 15) ? 5 : 40);
	if (CanWalk(nDX, nDY, false))
	{
		return { nDX,nDY };
	}
	return GetRandomPointInMap();
}
CMonster* CGameMap::CreateMonsterByType(int dwGenId, std::shared_ptr<stMonsterDataBase>& pMoninfo, int nType, Point point)
{
	switch ((emMonsterType)nType)
	{
	case emMonsterType::PET:
	{
		auto pet = CN_NEW CMonster(CRET_PET,point.x,point.y, pMoninfo, 0, 0);
		return pet;
	}
	default:
	{
		return CN_NEW CMonster(CRET_MONSTER, point.x, point.y,pMoninfo, dwGenId, 0);
	}
	}
}

void CGameMap::AddGenMonsters(mydb_mongen_tbl* mongen, bool checkObj,int count)
{
	if (getMapId() != mongen->dwMapId)
	{
		return;
	}
	if (auto monsterDb = sJsonConfig.GetMonsterDataBase(mongen->dwMonsterId))
	{
		for (int i = 0; i < count; ++i)
		{
			AddMonster(monsterDb, mongen->nX, mongen->nY, mongen->nRange, mongen->dwGenId, 0);
		}
	}
	else
		g_logger.error("怪物[%d]不存在", mongen->dwMonsterId);
}

void CGameMap::AddMonsters(uint32_t monsterId,PosType x,PosType y,PosType range,int count,uint32_t genId)
{
	if (auto monsterDb = sJsonConfig.GetMonsterDataBase(monsterId))
	{
		for (int i = 0; i < count; ++i)
		{
			AddMonster(monsterDb, x, y, range, false, 0);
		}
	}else
		g_logger.error("怪物[%d]不存在", monsterId);
}

CCreature* CGameMap::AddMonster(std::shared_ptr<stMonsterDataBase>& monsterDb, PosType x, PosType y, PosType range, uint32_t genId, uint32_t guildId, bool isFixedPoint){
	FUNCTION_BEGIN;
	if (!monsterDb)
		return nullptr;
	Point point = { x,y };
	if (!isFixedPoint)
		point = RandomXYInRange(x, y, range, false);
	if (auto pCret = CreateMonsterByType(genId, monsterDb, monsterDb->monster_type, point)) {
		pCret->SetEnvir(this);
		pCret->AddMonsterSkills();
		pCret->GetBaseProperty();
		pCret->ChangeProperty();
		pCret->m_nNowHP = pCret->m_stAbility[AttrID::MaxHP];
		pCret->m_nNowMP = pCret->m_stAbility[AttrID::MaxMP];
		pCret->m_wHomeMapID = getMapId();
		pCret->m_wHomeCloneMapId = getMapCloneId();
		pCret->m_nHomeX = pCret->m_nCurrX;
		pCret->m_nHomeY = pCret->m_nCurrY;
		pCret->m_guildId = guildId;
		if (!DoEnterMap(pCret)) {
			g_logger.debug("刷怪失败 %s [%s %d] %d : %d", monsterDb->name, getMapName(), getMapId(), pCret->GetX(), pCret->GetY());
			SAFE_DELETE(pCret)
			return nullptr;
		}
		return pCret;
	}
	return nullptr;
}

CNpcObj* CGameMap::AddNpc(mydb_npcgen_tbl& pNpcInfo){
	CNpcObj* pNpc = CLD_DEBUG_NEW CNpcObj(pNpcInfo.x, pNpcInfo.y, &pNpcInfo);

	pNpc->SetEnvir(this);
	pNpc->GetBaseProperty();
	pNpc->ChangeProperty();
	pNpc->m_nNowHP = pNpc->m_stAbility[AttrID::MaxHP];
	pNpc->m_nNowMP = pNpc->m_stAbility[AttrID::MaxMP];
	pNpc->m_wHomeMapID = getMapId();
	pNpc->m_wHomeCloneMapId = getMapCloneId();
	pNpc->m_nHomeX = pNpc->m_nCurrX;
	pNpc->m_nHomeY = pNpc->m_nCurrY;
	pNpc->m_btTargetState = (CAN_NOT_HIT_TARGET | CAN_NOT_MAGIC_TARGET/*|CAN_NOT_SEL_TARGET*/ | CAN_VISIT_TARGET);
	pNpc->m_btDirection = pNpcInfo.direction;
	pNpc->m_dwScriptId = pNpcInfo.scriptId;
	if (!DoEnterMap(pNpc)) {
		g_logger.error("NPC刷新失败 [%s %d] %d : %d", getMapName(), getMapId(), pNpc->GetX(), pNpc->GetY());
		SAFE_DELETE(pNpc)
		return nullptr;
	}
	return pNpc;
}

CPetObj* CGameMap::RegenPet(DWORD dwMonid,DWORD dwLevel,int nsrcX,int nsrcY, CPlayerObj* p){
	return nullptr;
}


CRobot* CGameMap::AddRobotMon(stLoadPlayerData* pGameData, bool isNeedLoadData, DWORD dwEffectId)
{
	FUNCTION_BEGIN;
	if (!pGameData)	return nullptr;
	Point point = { (PosType)pGameData->x,(PosType)pGameData->y };
	auto pMoninfo = sJsonConfig.GetMonsterDataBase(910102);
	if (!pMoninfo) return nullptr;
	if (CRobot* pCret = CLD_DEBUG_NEW CRobot(point.x, point.y, pGameData->szName,pMoninfo)) {
		pCret->LoadHumanBaseData(pGameData);
		if (isNeedLoadData && !pCret->LoadHumanData(pGameData)) {
			g_logger.debug("刷Robot LoadHumanData失败 %s [%s %d] %d : %d", pCret->GetName(), getMapName(), getMapId(), pCret->GetX(), pCret->GetY());
			SAFE_DELETE(pCret)
			return nullptr;
		}
		pCret->SetName(pGameData->szName);
		pCret->SetDisplayName(pGameData->szName);
		if (isNeedLoadData) {
			pCret->m_siAbility.dec(pCret->quest_vars_get_var_n("ap1"), pCret->quest_vars_get_var_n("ap2"), pCret->quest_vars_get_var_n("ap3"), pCret->quest_vars_get_var_n("ap4"), pCret->quest_vars_get_var_n("ap5"));
			pCret->GetBaseProperty();	
		}

		if (dwEffectId > 0)
		{
			pCret->quest_vars_set_var_n("RobotAbi", dwEffectId, false); //设置配置的属性id
		}
		pCret->DoChangeProperty(pCret->m_stAbility, false, __FUNC_LINE__);
		pCret->m_LifeState = NOTDIE;
		pCret->StatusValueChange(stCretStatusValueChange::hp, pCret->m_stAbility[AttrID::MaxHP], __FUNC_LINE__);
		pCret->StatusValueChange(stCretStatusValueChange::mp, pCret->m_stAbility[AttrID::MaxMP], __FUNC_LINE__);
		pCret->StatusValueChange(stCretStatusValueChange::pp, pCret->m_stAbility[AttrID::MaxPP], __FUNC_LINE__);
		pCret->m_wHomeMapID = getMapId();
		pCret->m_wHomeCloneMapId = getMapCloneId();
		pCret->m_nHomeX = pCret->m_nCurrX;
		pCret->m_nHomeY = pCret->m_nCurrY;
		pCret->SetEnvir(this);

		if (!DoEnterMap(pCret)) {
			g_logger.debug("刷Robot失败 %s [%s %d] %d : %d", pCret->GetName(), getMapName(), getMapId(), pCret->GetX(), pCret->GetY());
			SAFE_DELETE(pCret)
				return nullptr;
		}
		return pCret;
	}
	return nullptr;
}
	

void CGameMap::run(){
	FUNCTION_BEGIN;
	//if (GetTickCount64() > m_nextRunTickCount) {
		if (IsActive())
		{
			MapBase::run();
			m_Timer->run();//地图计时器
			timer_.Update();
		}
		//m_nextRunTickCount = GetTickCount64() + 400;
	//}
}

bool CGameMap::GetNextPosition(int nX,int  nY,BYTE btDir, int nSetp, int  &snx, int &sny){
	FUNCTION_BEGIN;
	snx = nX;
	sny = nY;
	if (btDir>7){ return false; }
	eGameDir nDir=gameDirs[btDir];
	if (nDir & DR_UP){
		sny-=nSetp;
	}else if (nDir & DR_DOWN){
		sny+=nSetp;
	}
	if (nDir & DR_LEFT){
		snx-=nSetp;
	}else if (nDir & DR_RIGHT){
		snx+=nSetp;
	}
	snx=safe_min<PosType>(safe_max(snx,0),m_nWidth);
	sny=safe_min<PosType>(safe_max(sny,0),m_nHeight);
	return (snx!=nX || sny!=nY);
}

BYTE CGameMap::GetNextDirection(int nSx,int nSy,int nDx,int nDy){
	FUNCTION_BEGIN;
	BYTE btdir=DR_NULL;
	if (nDx>nSx){
		btdir |= DR_RIGHT;
	}else if(nDx<nSx){
		btdir |= DR_LEFT;
	}
	if (nDy>nSy){
		btdir |= DR_DOWN;
	}else if(nDy<nSy){
		btdir |= DR_UP;
	}
#define DR_X	(DR_LEFT | DR_RIGHT)	
#define DR_Y	(DR_UP | DR_DOWN)
	int ndifx=abs(nDx-nSx);
	int ndify=abs(nDy-nSy);
	if (  ndify>2 && ndifx<=safe_min((ndify>>1),2) ){
		btdir=(btdir & (~DR_X));
	}else if (  ndifx>2 && ndify<=safe_min((ndifx>>1),2) ){
		btdir=(btdir & (~DR_Y));
	}
	return ((btdir!=0)?(gameDirIndexs[btdir]):0);
}

BYTE CGameMap::GetNextDirection(BYTE btdir,int nTurnCount){
	FUNCTION_BEGIN;
	BYTE nretdir=btdir;
	if ( nTurnCount>=DRI_NUM ){ if ( nTurnCount==DRI_NUM ){ nTurnCount=0; }else{ nTurnCount=nTurnCount%DRI_NUM; } }
	BYTE i=DRI_NUM-btdir;
	if (i>nTurnCount){
		nretdir+=nTurnCount;
	}else{
		nretdir=nTurnCount-i;
	}
	return nretdir;
}

BYTE CGameMap::GetReverseDirection(BYTE btdir){
	FUNCTION_BEGIN;
	return GetNextDirection(btdir,4);
}

const char* CGameMap::GetDirDis(BYTE btdir){
	FUNCTION_BEGIN;
	if (btdir>=DRI_NUM){  btdir=0; }
	return szIndexDir[btdir];
}

bool CGameMap::IsInRangeXY(int nSX,int  nSY,int  nRange,int  nDx, int nDy){
	if (abs(nSX - nDx)<=nRange && abs(nSY-nDy)<=nRange)
	{
		return true;
	}
	return false;
}

std::tuple<int,int> CGameMap::GetMapObjectCount(PosType nX, PosType nY) const
{
	int player_count = 0;
	int item_count = 0;
	if (auto cellObjects = GetCellObjects(nX,nY))
	{
		cellObjects->ForEachObject([&](MapObject* obj)
			{
				if (obj->GetType() == CRET_PLAYER)
				{
					player_count++;
				}
				else if (obj->GetType() == CRET_ITEM)
				{
					item_count++;
				}
			});
	}
	return std::make_tuple(player_count ,item_count);
}

bool CGameMap::GetSpiralDropPosition(PosType pos_x, PosType pos_y, int drop_count, std::vector<Point>& points)
{
	int cell_item_count = 0;
	points.reserve(drop_count);
	for (size_t i = 0;i<3;++i)
	{
		for (auto& point : SpiralPoints) {
			PosType x = pos_x + point.x;
			PosType y = pos_y + point.y;
			if (auto cell = GetMapCellInfo(x, y))
			{
				if (cell->is_gate)
					continue;
				auto [player_count, item_count] = GetMapObjectCount(x, y);
				if (player_count>0)
					continue;
				if (item_count > cell_item_count)
					continue;
				points.emplace_back(x, y);
				drop_count--;
				if (drop_count <= 0) {
					return true;
				}
			}
		}
		cell_item_count++;
	}
	return drop_count == 0;
}

int CGameMap::GetMaxRange(int nSX,int  nSY){
	FUNCTION_BEGIN;
	return ( max(max( m_nWidth-nSX,nSX ),max( m_nHeight-nSY,nSY )) ) ; 
}

bool CGameMap::GetNearXY(int nSX,int  nSY, PosType&nDx, PosType&nDy,int nRange, int nCheckObjCount){
	FUNCTION_BEGIN;
	nDx=nSX;
	nDy=nSY;
	if (nRange<0){ nRange=GetMaxRange(nSX,nSY); }
	int nbackcheckobjcount=nCheckObjCount;
	if (CanWalk(nDx,nDy,0,(nCheckObjCount<=0))){
		return true;
	}
	bool boFind=false;
	PosType nLx,nHx,nLy,nHy;
	for (PosType nSpace=1;nSpace<=nRange;nSpace++){
		nLx=safe_max<PosType>(nSX-nSpace,0);
		nHx=safe_min<PosType>(nSX+nSpace,m_nWidth);

		nLy=safe_max<PosType>(nSY-nSpace,0);
		nHy=safe_min<PosType>(nSY+nSpace,m_nHeight);

		for (int ix=nLx;ix<=nHx;ix++){
			for (int iy=nLy;iy<=nHy;iy++){
				if (abs(iy-nSY)==nSpace || 	abs(ix-nSX)==nSpace){
					nCheckObjCount--;
					if (CanWalk(ix,iy,0,(nCheckObjCount<=0))){
						nDx=ix;
						nDy=iy;
						boFind=true;
						break;
					}
				}
			}
			if (boFind){ break;	}
		}
		if (boFind){ break;	}
	}
	if (!boFind){
		return GetRandXYInMap(nDx,nDy,nbackcheckobjcount);
	}
	return true;
}

bool CGameMap::GetNearNoCretXY(int nSX,int nSY,int &nDx, int &nDy,int nRange, int nCheckObjCount/* =0 */){
	FUNCTION_BEGIN;
	nDx=nSX;
	nDy=nSY;
	if (nRange<0){ nRange=GetMaxRange(nSX,nSY); }
	int nbackcheckobjcount=nCheckObjCount;
	if (CanWalk(nDx,nDy,0,(nCheckObjCount<=0))){
		return true;
	}
	PosType nLx,nHx,nLy,nHy;
	for (int nSpace=1;nSpace<=nRange;nSpace++){
		nLx=safe_max<PosType>(nSX-nSpace,0);
		nHx=safe_min<PosType>(nSX+nSpace,m_nWidth);

		nLy=safe_max<PosType>(nSY-nSpace,0);
		nHy=safe_min<PosType>(nSY+nSpace,m_nHeight);

		for (int ix=nLx;ix<=nHx;ix++){
			for (int iy=nLy;iy<=nHy;iy++){
				if (abs(iy-nSY)==nSpace || 	abs(ix-nSX)==nSpace){
					nCheckObjCount--;
					if (CanWalk(ix,iy,0,(nCheckObjCount<=0))){
						nDx=ix;
						nDy=iy;
						return true;
					}
				}
			}
		}
	}
	return false;
}

bool CGameMap::GetSuiJiJiaoBenFanWei(int  &nDx, int &nDy,int nInRange,int nOutRange){
	FUNCTION_BEGIN;
	if (nOutRange==0){
		return true;
	}
	int nx, ny;
	int ddx = _random(nOutRange,nInRange);
	if (_random(1) == 0)
	{
		nx = nDx+ddx;
	}else{
		nx = nDx-ddx;
	}
	if (nx<0)
	{
		nx=0;
	} 
	if (nx > m_nWidth)
	{
		nx=m_nWidth;
	}
	

	int ddy = _random(nOutRange,nInRange);
	if (_random(1) == 0)
	{
		ny = nDy+ddy;
	}else{
		ny = nDy-ddy;
	}
	if (ny<0)
	{
		ny=0;
	} 
	if (ny > m_nHeight)
	{
		ny=m_nHeight;
	}

	nDx=nx;
	nDy=ny;
	
	return true;
}


bool CGameMap::GetRandXYInRange(PosType &nDx, PosType& nDy,int nInRange,int nOutRange){
	FUNCTION_BEGIN;
	if (nOutRange==0){
		return true;
	}
	int ran, signX, signY, nx, ny;
	signX = 1;
	signY = 1;

	ran = _random(3);
	static const int sixs[]={1,-1,-1,1};
	static const int siys[]={1,1,-1,-1};
	signX=sixs[ran];
	signY=siys[ran];

	int nOutRangeX=min(min(m_nWidth-nDx,nOutRange),nDx);
	nx = nDx + signX * (nInRange + _random(nOutRangeX - nInRange) + 1);
	if (nx >= m_nWidth ){
		nx = m_nWidth - 1;
	}else if (nx < 0 ){
		nx = 0;
	}

	int nOutRangeY=min(min(m_nHeight-nDy,nOutRange),nDy);
	ny = nDy + signY * (nInRange + _random(nOutRangeY - nInRange) + 1);
	if (ny >= m_nHeight ){
		ny = m_nHeight - 1;
	}else if (ny < 0 ){
		ny = 0;
	}

	nDx=nx;
	nDy=ny;
	return true;
}


void setRandXYTable (sol::table& table,int tabidx,int value1,int value2,int value3){
	if (!table.valid()){return;}
	sol::state_view lua = table.lua_state();
	sol::table subtable = lua.create_table();
	subtable["x"]=value1;
	subtable["y"]=value2;
	subtable["cret"]=value3;
	table[tabidx]=subtable;
}
//(空表,需要得到的数目,坐标X,坐标Y,最小范围,最大范围)
bool CGameMap::LuaGetRandXYTableInRange(sol::table table, int nTableSize, int nDx, int nDy, int nInRange, int nOutRange)
{
	FUNCTION_BEGIN;
	// Optional check to ensure the table is valid.
	if (!table.valid())
	{
		return false;
	}

	int nIdx = 0;
	int nLoop = 0;
	do
	{
		int nRandX = nDx;
		int nRandY = nDy;
		if (GetSuiJiJiaoBenFanWei(nRandX, nRandY, nInRange, nOutRange) && CanWalk(nRandX, nRandY, 0, true))
		{
			int count1 = 1;
			if (nRandX != nDx && nRandY != nDy)
			{
				setRandXYTable(table, nIdx + 1, nRandX, nRandY, GetCretCount(NULL, nRandX, nRandY));
				nIdx++;
				nTableSize--;
			}
		}
		nLoop++;
		if (nLoop > 200)
		{
			break;
		}
	} while (nTableSize > 0);
	return true;
}

bool CGameMap::GetRandXYInMap(PosType  &nnx, PosType&nny, int nCheckObjCount,int nLoop){
	FUNCTION_BEGIN;
	int step, edge;

	if( m_nWidth < 150 ){
		if( m_nWidth < 50 ){
			step = 5;
		}else{	
			step = 15;
		}
	}else{
		step = 30;
	}

	if( m_nHeight < 150 ){
		if( m_nHeight < 50 ){
			edge = 5;
		}else{	
			edge = 15;
		}
	}else{
		edge = 30;
	}

	int x_n=m_nWidth/step;
	int y_n=m_nHeight/edge;
	int range=max(step,edge);
	int x_rn =0;
	int y_rn =0;
	int nfindcount=safe_min<int>( 250 , x_n*y_n+1 );
	for (int i=0;i<nfindcount;i++){
		if( CanWalk(nnx, nny,0,(nCheckObjCount<=0) ) ){
			return true;
		}else{
			x_rn = _random(x_n);
			y_rn = _random(y_n);
			nnx=x_rn*step;
			nny=y_rn*edge;
			GetRandXYInRange(nnx,nny,0,range);
		}
		nCheckObjCount--;
	} 
	return false;
}

bool CGameMap::LuaGetXYInMap(){
	FUNCTION_BEGIN;
	PosType nX=_random(m_nWidth-1);
	PosType nY=_random(m_nHeight-1);
	if (GetRandXYInMap(nX,nY))
	{
		m_nLuaX=nX;
		m_nLuaY=nY;
		return true;
	}
	else{
		m_nLuaX=0;
		m_nLuaY=0;
		return false;
	}
}

CCreature* CGameMap::GetCretInXY(int nX,int  nY,bool bodie){
	return GetCretInRange(nX, nY, 0, 0, bodie);
}

CCreature* CGameMap::GetCretInRange(int nX,int nY,DWORD dwTmpID,int nRange,bool bodie)
{
	const PosType nLx = std::clamp<PosType>(nX - nRange, 0, m_nWidth - 1);
	const PosType nHx = std::clamp<PosType>(nX + nRange, 0, m_nWidth - 1);
	const PosType nLy = std::clamp<PosType>(nY - nRange, 0, m_nHeight - 1);
	const PosType nHy = std::clamp<PosType>(nY + nRange, 0, m_nHeight - 1);
	for(PosType ix=nLx;ix<=nHx;ix++){
		for(PosType iy=nLy;iy<=nHy;iy++){
			if (auto pMapCellInfo = GetCellObjects(ix, iy); pMapCellInfo) {
				auto object = pMapCellInfo->FindIf([&](MapObject* obj)
					{
						if (auto crt = dynamic_cast<CCreature*>(obj))
						{
							if (!crt->isValidObj())
								return false;
							if (dwTmpID && crt->GetObjectId() != dwTmpID)
								return false;
							if (crt->isDie() != bodie)
								return false;
							if (!crt->isCanHit() || !crt->isCanMagic())
								return false;
							return true;
						}
						return false;

					});
				if (object)
				{
					return static_cast<CCreature*>(object);
				}
			}
		}
	}
	return NULL;
}

int CGameMap::GetCretsInMagicRange(int nX,int nY,int nDir,int nDX,int nDY,std::vector<CCreature*>& v,DWORD TargetId, const std::shared_ptr<stMagicDataBase>& pMagicData,CCreature* pAttack,int nMax,DWORD dwTmpID,bool bodie ){
	FUNCTION_BEGIN;
	FUNCTION_MONITOR(48,"CGameMap::GetCretsInMagicRange");
	DWORD dwOldTargetId = TargetId;
	int ng=0;
	if (!pMagicData){return ng;}
	if (!pAttack){return ng;}
	if (SpeclaiMagicRange(nX, nY, nDir, nDX, nDY, v, TargetId, pMagicData, pAttack, nMax, dwTmpID, bodie) == true) {return v.size();}
	const stMagicRanges* pMagicRange=NULL;
	CMagicRangeDefine::getMe().get(pMagicData->btShape,(nDir==DRI_NUM)?0:nDir,pMagicRange);
	if (pMagicRange) {
		stRelativePos stPos;
		stPos.t = 0;
		stPos.w = 10000;
		//CCreature* pTarget=NULL;
		//bool boTarget=true;
		bool boGetTarget = (!(nDX >= 0 && nDY >= 0));
		MagicRange::const_iterator it;
		for (it = pMagicRange->lib.begin(); it != pMagicRange->lib.end(); it++)
		{
			DWORD dwTargetX = 0;
			DWORD dwTargetY = 0;
			stPos = (*it);
			stPos.getAbsolutePos(nX, nY, nDir, dwTargetX, dwTargetY);
			if ((int)dwTargetX == nDX && (int)dwTargetY == nDY) { boGetTarget = true; }
			if (nMax != -1 && v.size() >= (DWORD)nMax) { break; }
			if (pMagicRange->lib.size() > 1)
			{
				//非单体技能 TargetId 设为 0，原因参考GetCretInRangeAll（）逻辑
				TargetId = 0;
			}
			auto ListTarget = GetCretInRangeAll(dwTargetX, dwTargetY, TargetId, 0, bodie);
			for (auto it = ListTarget.begin(); it != ListTarget.end(); ++it) {
				CCreature* pTarget = *it;
				if (pTarget
					&& (dwTmpID == 0 || (dwTmpID != 0 && dwTmpID != pTarget->GetObjectId()))
					&& pTarget->isValidObj() && (pTarget->isDie() == bodie)
					&& pTarget->isCanHit() && pTarget->isCanMagic())
				{
					//if (pMagicData->nDamageType != NO_DAMAGE && !pAttack->isEnemy(pTarget)) { continue; }
					//if (pTarget->m_dwTmpId==TargetId){continue;}
					pTarget->m_btDamageType = stPos.t;
					pTarget->m_wDamageLoss = stPos.w;
					v.push_back(pTarget);
					if (nMax != -1 && v.size() >= (DWORD)nMax) { break; }			
				}
			}
		}

		// 保证近战攻击，不空刀
		if (v.size() == 0 && pMagicData->btAttackType == NEARLY_ATTACK)
		{
			CCreature* pNewTarget = FindCretByTmpId(dwOldTargetId, nDX, nDY);
			int nDistance = 99;
			if (pNewTarget && pNewTarget->isValidObj()){
				nDistance = max(abs(nX - pNewTarget->m_nCurrX), abs(nY - pNewTarget->m_nCurrY));	//距离
			}
			if (pNewTarget && pNewTarget->isValidObj() && (pNewTarget->isDie() == bodie)
				&& pNewTarget->isCanHit() && pNewTarget->isCanMagic()
				&& pNewTarget->GetEnvir() == this				// 同一个地图
				&& nDistance <= pMagicData->btMaxRange + 1		// 出刀瞬间怪物一格，多判断一格距离
				) {							// 战士攻击距离2格	
				pAttack->m_curAttTarget = pNewTarget;
				pNewTarget->m_btDamageType = stPos.t;
				pNewTarget->m_wDamageLoss = stPos.w;

				v.push_back(pNewTarget);
			}
			return v.size();
		}

		//保证单体攻击，可以打到目标
		if (v.size() == 0 && pMagicData->btShape == SHAPE_FAR_POINT) {
			auto ListTarget = GetCretInRangeAll(nX, nY, TargetId, 2, bodie);
			if (ListTarget.size()) {
				for (auto it = ListTarget.begin(); it != ListTarget.end(); ++it) {
					CCreature* pTarget = *it;
					if (pTarget
						&& (dwTmpID == 0 || (dwTmpID != 0 && dwTmpID != pTarget->GetObjectId()))
						&& pTarget->isValidObj() && (pTarget->isDie() == bodie)
						&& pTarget->isCanHit() && pTarget->isCanMagic())
					{
						pTarget->m_btDamageType = stPos.t;
						pTarget->m_wDamageLoss = stPos.w;
						v.push_back(pTarget);
						break;
					}
				}
			}
		}
	}
	return v.size();
}

bool CGameMap::SpeclaiMagicRange(int nX, int nY, int nDir, int nDX, int nDY, std::vector<CCreature*>& v, DWORD TargetId, const std::shared_ptr<stMagicDataBase>& pMagicData, CCreature* pAttack, int nMax, DWORD dwTmpID, bool bodie)
{
	if (!pMagicData)
	{
		return false;
	}
	BYTE btShape = pMagicData->btShape;
	if (btShape == (BYTE)(-2) || btShape == (BYTE)(-3))
	{//随机冰冻3个玩家 点面是-2表示是全屏选人，如果是全屏所有人那么填比较大的数据
		v.clear();
		
		std::map<int, CCreature*> mapplayer;
		std::map<int, CCreature*>::iterator it;
		int nSize = 0;
		std::vector<int> RandSource;
		if (pMagicData->btShape == (BYTE)(-2))
		{//只选择玩家
			ForeachVisualObjects(nX,nY, [&](MapObject* obj)
				{
					if (obj->GetType() != CRET_PLAYER)
						return;
					if (CPlayerObj* pTarget = dynamic_cast<CPlayerObj*>(obj)) {
						if (pTarget->isValidObj() && (pTarget->isDie() == bodie) && pTarget->isCanHit() && pTarget->isCanMagic())
						{
							nSize = mapplayer.size() + 1;
							mapplayer.insert(std::pair<int, CCreature*>(nSize, pTarget));
							RandSource.push_back(nSize);
						}
					}
				});
		}
		else if(btShape == (BYTE)(-3))
		{
			CCreature* pTarget = NULL;
			auto ListTarget = GetCretInRangeAll(0, 0, 0, m_nWidth + m_nHeight, bodie);
			for (auto it = ListTarget.begin(); it != ListTarget.end(); ++it)
			{
				pTarget = *it;
				if (pTarget && !pTarget->isMonster() && pTarget->isValidObj() && (pTarget->isDie() == bodie) && pTarget->isCanHit() && pTarget->isCanMagic())
				{
					nSize = mapplayer.size() + 1;
					mapplayer.insert(std::pair<int, CCreature*>(nSize, pTarget));
					RandSource.push_back(nSize);
				}
			}
		}
		
		int nRandom = pMagicData->wAttackNum;
		if (nRandom >= mapplayer.size())
		{
			for (it = mapplayer.begin(); it != mapplayer.end(); it++)
			{
				v.push_back(it->second);
			}
		}
		else
		{
			std::list<int> result;						

			CUserEngine::getMe().GetRandom(RandSource, nRandom, result);

			int nKey = 0;
			std::list<int>::iterator ik;
			for (ik = result.begin(); ik != result.end(); ik++)
			{			
				nKey = *ik;
				it = mapplayer.find(nKey);
				if (it != mapplayer.end())
				{
					v.push_back(it->second);
				}
			}
		}
		return true;
	}
	return false;
}

CMapItemEvent* CGameMap::GetItemInXY(int nX, int  nY, int64_t itemid){
	if (auto cellObjects = GetCellObjects(nX, nY); cellObjects) {
		auto object = cellObjects->FindIf([itemid](MapObject* obj)
			{
				if (auto mapItem = dynamic_cast<CMapItemEvent*>(obj))
				{
					if (mapItem->OwnerItem && (itemid==0 || mapItem->m_i64ItemId==itemid))
					{
						return true;
					}
				}
				return false;
			});
		if (object)
		{
			return static_cast<CMapItemEvent*>(object);
		}
	}
	return NULL;
}

bool CGameMap::AddItemToMap(int nX,int nY,CMapItemEvent* ptmpMapItem){
	return DoEnterMap(ptmpMapItem);
}

bool CGameMap::DelItemFromMap(int nX,int nY,CMapItemEvent* ptmpMapItem){
	return DoLeaveMap(ptmpMapItem);
}

bool CGameMap::AddMagicToMap(int nX,int nY,CMapMagicEvent* ptmpMapMagic){
	return DoEnterMap(ptmpMapMagic);
}

bool CGameMap::DelMagicFromMap(int nX,int nY,CMapMagicEvent* ptmpMapMagic){
	return DoLeaveMap(ptmpMapMagic);
}

bool CGameMap::MoveCretTo(CCreature* pCret, int nDx, int nDy, int nDz, bool boNotCheckObj){
	FUNCTION_BEGIN;
	if (pCret){
		if (CanWalk(nDx,nDy,nDz,boNotCheckObj)){
			return DoMapMove(pCret, nDx, nDy);
		}
	}
	return false;
}

int CGameMap::GetCretCount(CCreature* Target,int nX,int nY,int nMax){
	FUNCTION_BEGIN;
	int nRet=0;
	if (auto cellObjects = GetCellObjects(nX, nY); cellObjects) {
		cellObjects->ForEachObject([&](MapObject* obj)
			{
				if (auto crt = dynamic_cast<CCreature*>(obj))
				{
					nRet++;
					if (nRet >= nMax) { return nRet; };
				}
			});
	}
	return nRet;
}

CCreature* CGameMap::LuaAddMonster(const sol::table& table){
	FUNCTION_BEGIN;
	if (!table.valid())
		return nullptr;
	int monId = table.get_or("monsterId", 0);
	auto pMonData=sJsonConfig.GetMonsterDataBase(monId);
	if (!pMonData)
		return nullptr;
	PosType x = table.get_or<PosType>("x", 0);
	PosType y = table.get_or<PosType>("y", 0);
	PosType range = table.get_or<PosType>("range", 1);
	uint32_t guildId = table.get_or("guildid", 0);
	bool isFixedPoint = table.get_or("isFixedPoint", 0);
	return AddMonster(pMonData, x, y, range, false, guildId, isFixedPoint);
}

CPlayerObj* CGameMap::GetPlayer(int32_t objectId)
{
	auto object = GetObjById(objectId);
	if (!object)
		return nullptr;
	return dynamic_cast<CPlayerObj*>(object);
}

CCreature* CGameMap::GetCreature(int32_t objectId)
{
	auto object = GetObjById(objectId);
	if (!object)
		return nullptr;
	return dynamic_cast<CCreature*>(object);
}
CCreature* CGameMap::FindCretByTmpId(DWORD dwTmpid, int nX, int  nY)
{
	CCreature* pTarget = GetCreature(dwTmpid);
	if (pTarget && pTarget->isValidObj() && (pTarget->isDie() == false) && pTarget->isCanHit() && pTarget->isCanMagic())
	{
		return pTarget;

	}
	return nullptr;
}

void CGameMap::SendMsgToMapAllUser(void *pbuf,int nLen){
	FUNCTION_BEGIN;
	ForeachObjectsByType(CRET_PLAYER, [&](MapObject* obj)
		{
			if (CPlayerObj* player = dynamic_cast<CPlayerObj*>(obj)) {
				player->SendMsgToMe(pbuf, nLen);
			}
		});
}

void CGameMap::SetNpcEmblemId(DWORD dwEmblemId) {
	FUNCTION_BEGIN;
	ForeachObjectsByType(CRET_NPC, [&](MapObject* obj)
	{
		if (CNpcObj* npc = dynamic_cast<CNpcObj*>(obj)) {
			npc->SetEmblem(dwEmblemId);
		}
	});
}

bool CGameMap::SetMapPop(const char* szMapStr,bool boAdd){
	FUNCTION_BEGIN;
	g_logger.debug("MapId = %d,MapName = %s, CGameMap::SetMapPop(%s,%s)",this->getMapId(),this->getMapName(),szMapStr,boAdd?"true":"false");
	if (szMapStr && szMapStr[0]!=0){
		if (!boAdd){
			m_mapPop.mapproperty=0;
		}
		WORD tmppop=0;
		CEasyStrParse parse;
		char szTmpStr[_MAX_TIP_LEN_];
		strcpy_s(szTmpStr,_MAX_TIP_LEN_-1,szMapStr);
		parse.SetParseStr(szTmpStr,",|");
		for (int i=0;i<parse.ParamCount();i++)
		{
			tmppop=atoi(parse[i]);
			if (tmppop){
				switch (tmppop)
				{
				case 1:m_mapPop.bitPkNoSword=1;break;			//PK不加PK值
				case 2:m_mapPop.bitDieNoDropEquip=1;break;		//死亡不掉身上装备
				case 3:m_mapPop.bitDieNoDropAll=1;break;		//死亡不掉所有装备
				case 4:m_mapPop.bitNoSafeZone=1;break;			//没有安全区
				case 5:m_mapPop.bitNoOffLineSaveHome=1;break;	//下线回城
				case 6:m_mapPop.bitNoSpaceExceptHome=1;break;	//禁止除了回城的传送
				case 7:m_mapPop.bitNoSpaceAll=1;break;			//禁止所有传送
				case 8:m_mapPop.bitNoUseItem=1;break;			//禁止使用物品
				case 9:m_mapPop.bitNoForcePk=1;break;			//禁止PK
				case 10:m_mapPop.bitNoSendMail = 1; break;		//禁止被击杀邮件
				case 11:m_mapPop.bitNoSendNotice = 1; break;		//禁止被击杀公告
				case 12:m_mapPop.bitNoDropYuanShen = 1; break;		//死亡不掉元神觉醒
				case 13:m_mapPop.bitNoMosterDieDrop = 1; break;		//打怪爆率属性在地图内不生效
				case 14:m_mapPop.bitNoCleaner = 1; break;		//不能使用吸尘器
				case 15:m_mapPop.bitNoBianShi = 1; break;		//不触发鞭尸
				case 16:m_mapPop.bitNoDropDown = 1; break;		//掉落衰减不生效
				case 17:m_mapPop.bitMoveRecord = 1; break;		//传送记录
				case 18:m_mapPop.bitNoRandomStone = 1; break;	//不能使用随机石
				case 19:m_mapPop.bitNoHomeStone = 1; break;		//不能使用回城石
				case 20:m_mapPop.bitNoRelive = 1; break;		//复活特戒不生效
				case 21:m_mapPop.bitNoXiaoFeiXie = 1; break;		//不能使用小飞鞋
				case 22:m_mapPop.bitNoResNeng = 1; break;		//不消化 剑师能量
				}
			}
		}
		return true;
	}
	return false;
}

void CGameMap::mapGroupAddNum(CPlayerObj* player){
	FUNCTION_BEGIN;
	if(player && player->m_GroupInfo.dwGroupId){
		auto& groupMembers = m_groupMembers[player->m_GroupInfo.dwGroupId];
		auto memberIt = groupMembers.find(player);
		if (memberIt!=groupMembers.end())
		{
			g_logger.error("玩家 %s:已经进队，队伍id = %d,指针%.8x", player->getName(), player->m_GroupInfo.dwGroupId, player);
			return;
		}
		groupMembers.insert(player);
		g_logger.debug("玩家 %s:进队，队伍id = %d,指针%.8x",player->getName(),player->m_GroupInfo.dwGroupId,player);
	}
}

void CGameMap::mapGroupRemoveNum(CPlayerObj* player){
	FUNCTION_BEGIN;
	if(player && player->m_GroupInfo.dwGroupId){
		auto groupIt = m_groupMembers.find(player->m_GroupInfo.dwGroupId);
		if (groupIt !=m_groupMembers.end())
		{
			auto& groupMembers = groupIt->second;
			auto memberIt = groupMembers.find(player);
			if (memberIt != groupMembers.end())
			{
				g_logger.debug("玩家 %s 离队 队伍id = %d,指针%.8x", player->getName(), player->m_GroupInfo.dwGroupId, player);
				groupMembers.erase(memberIt);
			}
		}
	}
}

DWORD CGameMap::getMapGroupNum(DWORD dwGroupId){
	FUNCTION_BEGIN;
	if(dwGroupId){
		auto groupIt =m_groupMembers.find(dwGroupId);
		if(groupIt !=m_groupMembers.end()){
			return groupIt->second.size();
		}
	}
	return 0;
}

bool CGameMap::GetGroupPlayer(DWORD dwGroupId, std::unordered_set<CPlayerObj*>& playerSet) {
	auto groupIt = m_groupMembers.find(dwGroupId);
	if (groupIt != m_groupMembers.end()) {
		playerSet = groupIt->second;
		return true;
	}
	return false;
}

double CGameMap::quest_vars_get_var_n(const std::string& name){
	FUNCTION_BEGIN;
	double dbret=0.0;
	BYTE lifetype=0;
	char* pgetvalue=nullptr;
	((CVars*)&m_vars)->get_var_c(name,pgetvalue,lifetype);
	if (pgetvalue){
		dbret=strtod(pgetvalue,NULL);
	}
	return dbret;
}
bool CGameMap::isPeakDuelMap() {
	if (quest_vars_get_var_n("mappeakduelcc") > 0) {
		return true;
	}
	return false;
}
bool CGameMap::quest_vars_set_var_n(const std::string& name, double value,bool needsave){
	char setvalue[_MAX_QUEST_LEN_];
	sprintf(setvalue,"%lf",value);
	return ((CVars*)&m_vars)->set_var_c(name,setvalue,(needsave?CVar::_NEED_SAVE_:CVar::_DONT_SAVE_) );
}

void CMapHashManager::ClearCloneMap(){
	FUNCTION_BEGIN;
	std::vector<CGameMap*> cloneMapList;
	s_ForEach([&](CGameMap* map)
		{
			if (map->getMapCloneId())
			{
				cloneMapList.push_back(map);
			}
		});
	for (auto it:cloneMapList)
	{
		m_gameMaps.s_remove(it->getFullMapId());
		SAFE_DELETE(it);

	}
}

void CMapHashManager::run(time_t curTime){
	FUNCTION_BEGIN;
	ULONGLONG dwRunStartTick=::GetTickCount64();
	if (dwRunStartTick>m_nextRunTickCount){
		std::vector<CGameMap*> cloneMapList;
		s_ForEach([&](CGameMap* map)
		{
			map->run();
			if (map->getMapCloneId() == 0)
				return;
			if (curTime < map->m_dwCloneMapExistTime)
				return;
			if (map->IsActive())
				return;
			stUpdateExistCloneMap sendmapcmd;
			sendmapcmd.boAdd = false;
			sendmapcmd.nMapID = MAKELONG(map->getMapId(), map->getMapCloneId());
			GameService* gamesvr = GameService::instance();
			gamesvr->Send2LoginSvrs(&sendmapcmd, sizeof(sendmapcmd));
			gamesvr->Send2DBSvrs(&sendmapcmd, sizeof(sendmapcmd));
			cloneMapList.push_back(map);
		});
		for (auto& it:cloneMapList)
		{
			m_gameMaps.s_remove(it->getFullMapId());
			SAFE_DELETE(it);
		}
		m_nextRunTickCount=GetTickCount64()+50;
	}
}



std::vector<CCreature*> CGameMap::GetCretInRangeAll(int nX, int nY, DWORD dwTmpID, int range, bool requireAlive) const
{
	const int minX = std::clamp(nX - range, 0, m_nWidth - 1);
	const int maxX = std::clamp(nX + range, 0, m_nWidth - 1);
	const int minY = std::clamp(nY - range, 0, m_nHeight - 1);
	const int maxY = std::clamp(nY + range, 0, m_nHeight - 1);
	std::vector<CCreature*> results;
	for (int x = minX; x <= maxX; ++x) {
		for (int y = minY; y <= maxY; ++y) {
			if (const auto cell = GetCellObjects(x, y)) {
				cell->ForEachObject([&](MapObject* obj) {
					CCreature* creature = dynamic_cast<CCreature*>(obj);
					if (!creature) return;
					bool valid =
						(dwTmpID == 0 || creature->GetObjectId() == dwTmpID) &&
						creature->isValidObj() &&
						(!requireAlive || !creature->isDie()) &&
						creature->isCanHit() &&
						creature->isCanMagic() &&
						!creature->m_boGmHide;

					if (valid) {
						results.emplace_back(creature);
					}
					});
			}
		}
	}
	return results;
}

int CGameMap::GetMonCountById(int monId) const
{
	int count = 0;
	for (auto& it:m_typeObjMap[CRET_MONSTER])
	{
		CMonster* pMon = dynamic_cast<CMonster*>(it.second);
		if (!pMon) continue;
		if (pMon->isDie()) continue;
		if (pMon->GetMonsterDataBase()->nID == monId)
		{
			count++;
		}
	}
	return count;
}

bool CGameMap::sendMapTipMsg(const char* pszMsg)
{
	if (!pszMsg)
	{
		g_logger.error("CGameMap::sendTipMsg 传入pszMsg 参数为空 !!!");
		return false;
	}

	BUFFER_CMD(stTipMsg, sendCmd, stBasePacket::MAX_PACKET_SIZE);
	sendCmd->nPosX = 0;
	sendCmd->nPosY = 0;
	sendCmd->szTipMsg.push_str(UTG(pszMsg));
	SendMsgToMapAllUser(sendCmd, sizeof(*sendCmd) + sendCmd->szTipMsg.getarraysize());
	return true;
}
