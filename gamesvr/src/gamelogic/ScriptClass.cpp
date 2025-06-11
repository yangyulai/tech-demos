#include <register_lua.hpp>
#include "Script.h"
#include "BaseCreature.h"
#include "UsrEngn.h"
#include "Npc.h"
#include "JsonConfig.h"
#include "PetManager.h"

void CScriptSystem::BindClass(CLuaVM *luavm){
	sol::state_view lua(luavm->lua());
	registerClass<CScriptSql>(lua, "CScriptSql",
		"setsql", &CScriptSql::SetSql,
		"getcount", &CScriptSql::GetCount,
		"selectsql", &CScriptSql::SelectSql,
		"executesql", &CScriptSql::ExecuteSql,
		"getdata", &CScriptSql::GetData,
		"supergetcount", &CScriptSql::SuperGetCount,
		"superexecutesql", &CScriptSql::SuperExecuteSql,
		"superselectsql", &CScriptSql::SuperSelectSql
	);
	lua.new_usertype<CNpcObj>("CNpcObj",
		"npcid", sol::property(&CNpcObj::getNpcId),
		"scriptid", sol::property(&CNpcObj::getNpcScriptId)
	);
	registerClass<GameService>(lua,"GameService",
		"gm", &GameService::m_btAllisGm
	);
	lua.new_usertype<CPetObj>("CPetObj",
		"getmaster", &CPetObj::getMaster,
		"getpetdata", &CPetObj::getPetData,
		"setpetmagic", &CPetObj::SetCurPetMagic,
		"studyskill", &CPetObj::StudySkill,
		"deleteskill", &CPetObj::DeleteSkill,
		"clearskill", &CPetObj::ClearSkill,
		"GetZhaoHuanChangeLv", &CPetObj::GetZhaoHuanChangeLv,
		"lv", sol::property(&CPetObj::GetLevel),
		"getpetid", &CPetObj::GetPetId
	);
	lua.new_usertype<CPetManage>("CPetManage",
		"addpetobj", (CPetObj * (CPetManage::*)(int, CPlayerObj*, int, bool)) & CPetManage::addPetObj,
		"delpetobj", (bool(CPetManage::*)(double)) & CPetManage::delPetObj,
		"getpetobj", &CPetManage::getPetObj,
		"getpetobjbymonid", &CPetManage::getPetObjByMonID
	);
	registerClass<CScriptTimer>(lua, "Timer",
		"sttimer", &CScriptTimer::m_stTimer,
		"runcount", &CScriptTimer::m_runcount,
		"maxcount", &CScriptTimer::m_maxruncount,
		"checktime", &CScriptTimer::m_checknum,
		"play", &CScriptTimer::Play,
		"stop", &CScriptTimer::Stop,
		"pause", &CScriptTimer::Pause,
		"reset", &CScriptTimer::Reset,
		"getstatus", &CScriptTimer::GetStatus,
		"gettime", &CScriptTimer::GetTime,
		"checkkeep", &CScriptTimer::CheckKeep,
		"callevt", &CScriptTimer::CallEvt
	);
	registerClass<CScriptTimerManager>(lua, "TimerManager",
		"settimedate", &CScriptTimerManager::settimedate,
		"settm", &CScriptTimerManager::settime,
		"clear", &CScriptTimerManager::clear,
		"findtimer", &CScriptTimerManager::find_timer,
		"removetimer", &CScriptTimerManager::remove_timer
	);
	lua.new_usertype<stTiShi>("stTiShi",
		"GetLastTishi", &stTiShi::GetLastTishi,
		"SetLastTishi", &stTiShi::SetLastTishi
	);
	lua.new_usertype<stTuBiao>("stTuBiao",
		"state", &stTuBiao::state
	);
	lua.new_usertype<TiShiManager>("TiShiManager",
		"AddCheckList", &TiShiManager::AddCheckList,
		"AddMapChangeList", &TiShiManager::AddMapChangeList,
		"MapChange", &TiShiManager::MapChange,
		"AddCallBack", &TiShiManager::AddLuaCallBack
	);
	lua.new_usertype<TuBiaoManager>("TuBiaoManager",
		"AddCheckList", &TuBiaoManager::AddCheckList,
		"GetShowState", &TuBiaoManager::GetShowState,
		"KeepStorage", &TuBiaoManager::KeepStorage,
		"NeedUpdate", &TuBiaoManager::NeedUpdate
	);
	lua.new_usertype<CItem>("Item",
		"name", sol::property(&CItem::GetItemName),
		"id", sol::property(&CItem::GetItemBaseID),
		"tmpid", sol::property(&CItem::GetItemLuaId),
		"wearlevel", sol::property(&CItem::GetWearLevel),
		"levelorder", sol::property(&CItem::GetLevelOrder),
		"location", sol::property(&CItem::GetItemLocation, &CItem::SetItemLocation),
		"count", sol::property(&CItem::GetItemCount, &CItem::SetItemCount),
		"maxcount", sol::property(&CItem::GetMaxCount),
		"binding", sol::property(&CItem::GetBinding, &CItem::SetBinding),
		"dura", sol::property(&CItem::GetDura, &CItem::SetDura),
		"pdcount", sol::property(&CItem::GetPreventDropCount, &CItem::SetPreventDropCount),
		"maxdura", sol::property(&CItem::GetMaxDura),
		"rare", sol::property(&CItem::GetRare),
		"datanum", sol::property(&CItem::GetDataStNum),
		"sex", sol::property(&CItem::GetSex),
		"type", sol::property(&CItem::GetType),
		"guardtype", sol::property(&CItem::GetGuardType),
		"station", sol::property(&CItem::GetEquipStation),
		"maker", sol::property(&CItem::GetMakerName, &CItem::SetMakerName),
		"bornfrom", sol::property(&CItem::GetBornFrom, &CItem::SetBornFrom),
		"borntime", sol::property(&CItem::GetBornTime, &CItem::SetBornTime),
		"getcanuse", &CItem::GetCanUseTick,
		"nvalue", sol::property(&CItem::GetnValue, &CItem::SetnValue),
		"maxnvalue", sol::property(&CItem::GetMaxValue, &CItem::SetMaxValue),
		"level", sol::property(&CItem::GetLevel, &CItem::SetLevel),
		"limittime", sol::property(&CItem::GetLimitTime),
		"tostring", &CItem::getBase64String,
		"strengcount", sol::property(&CItem::GetStrengCount, &CItem::SetStrengCount),
		"strengcountmech", sol::property(&CItem::GetStrengCountMech, &CItem::SetStrengCountMech),
		"effid", sol::property(&CItem::GetEffId, &CItem::SetEffId),
		"refinecnt", sol::property(&CItem::GetRefineCnt, &CItem::SetRefineCnt),
		"effidEX", sol::property(&CItem::GetdwEffIdEx, &CItem::SetdwEffIdEx),
		"nextid", sol::property(&CItem::GetNextId),
		"dpcount", sol::property(&CItem::GetDropProtectCount, &CItem::SetDropProtectCount),
		"btstrenghadd", sol::property(&CItem::GetExtraStrenghLvl, &CItem::SetExtraStrenghLvl),
		"broken", sol::property(&CItem::GetBroken, &CItem::SetBroken),
		"prefix", sol::property(&CItem::GetPrefix, &CItem::SetPrefix),
		"fixcost", sol::property(&CItem::GetFixCost),
		"basefixcost", sol::property(&CItem::GetBaseFixCost),
		"lock", sol::property(&CItem::GetOutLock),
		"setcanuse", &CItem::SetCanUseTick,									//设置物品冷却时间
		"savezircon", &CItem::SaveLuaData,									//保存LUA数据
		"loadzircon", &CItem::LoadLuaData,									//读取LUA数据
		"savenp", &CItem::SaveNpData,										//保存极品数据(已存在的会增加,
		"changenpdata", &CItem::ChangeNpData,								//改变极品数据(已存在的会覆盖,
		"changenpmaxlv", &CItem::ChangeNpMaxLv,
		"loadnp", &CItem::LoadNpData,										//读取极品数据
		"loadnpbyfrom", &CItem::LoadNpDataByFrom,							//读取极品数据根据属性来源
		"clearaallnp", &CItem::ClearNpData,									//清除所有极品属性
		"clearnp", &CItem::ClearNpByFromType,								//清除某条属性
		"clearnpbyfrom", &CItem::ClearNpByFrom,								//清除某来源属性
		"clearnpbytype", &CItem::ClearNpByType,								//清除某类属性
		"addnpdata", &CItem::AddNpData,										//增加一条极品属性
		"delnpdatabypos", &CItem::DelNpDataByPos,							//删除一条极品属性(同一位置的上下限会一起删除,
		"setitemlog", &CItem::LuaSetItemLog,								//物品日志
		"gettrueitem", &CItem::GetTrueItem,									//把虚拟物品变成一个真实的物品
		"guardlv", sol::property(&CItem::GetGuardLv, &CItem::SetGuardLv),
		"guardevolvelv", sol::property(&CItem::GetGuardEvolveLv, &CItem::SetGuardEvolveLv),
		"db", sol::property(&CItem::GetItemDataBase),

		"canlog", & CItem::CanLog,											//是否记录日志
		"setlimittime", & CItem::SetLimitTime,
		"getsuittype", & CItem::GetSuitType,
		"getsuitid", & CItem::GetSuitId,
		"noticeday", & CItem::NoticeDay,
		"getlocation", & CItem::GetLocation,		//获取物品所在的格子类型
		"getindex", & CItem::GetIndex,				//获取物品所在的位置
		"mecheffid", sol::property(&CItem::GetMechEffid, &CItem::SetMechEffid),
		"mechspecialeffid", sol::property(&CItem::GetMechSpecialEffid, &CItem::SetMechSpecialEffid),
		"mechextraeffid", sol::property(&CItem::GetMechExtraEffid, &CItem::SetMechExtraEffid)
	);
	lua.new_usertype<CPlayerPackage>("CPlayerPackage",
		"getitembylocation", &CPlayerPackage::GetItemTableByLocation,		//根据物品的位置类型找出该类型的所有物品，并返回表
		"randomdropitem", &CPlayerPackage::RandomDropItem,					//随即掉落物品
		"storagecount", &CPlayerPackage::m_nOpenStorageCount,				//仓库数量
		"openstoragepackge", &CPlayerPackage::initOnePackage,
		"findfirstiteminbagbybaseid", &CPlayerPackage::FindFirstItemInBagByBaseId,
		"findfirstiteminbodybybaseId", &CPlayerPackage::FindFirstItemInBodyByBaseId,//通过BASIID在身上寻找物品
		"findfirstiteminstoragebybaseId", &CPlayerPackage::FindFirstItemInStorageByBaseId,//根据ID在仓库里寻找东西
		"finditeminbagbyid64", &CPlayerPackage::FindItemInBagById64,
		"finditemintmpbagbyid64", &CPlayerPackage::FindItemInTmpBagById64,
		"finditeminbodybyid64", &CPlayerPackage::FindItemInBodyby64,
		"exchangeitem", &CPlayerPackage::ServerGetExchangeItemEx,
		"getitemcoutbybaseid", &CPlayerPackage::GetItemCountInBagAllByBaseId,//通过baseId得到背包里所有这个物品的数目
		"findalliteminbagbybaseid", &CPlayerPackage::FindAllItemInBagByBaseId,
		"getitemcountinstorebybaseid", &CPlayerPackage::GetItemCountInStoreAllByBaseId,
		"checkcanaddtobag", &CPlayerPackage::CheckCanAddToBag,
		"finditeminbagbybaseid", &CPlayerPackage::FindItemInBagByBaseID,
		"openbagcellcount", sol::property(&CPlayerPackage::getBagCellCount, &CPlayerPackage::setBagCellCount),			//背包格子数量
		"storagecellcount", sol::property(&CPlayerPackage::getStorageCellCount, &CPlayerPackage::setStorageCellCount),	//仓库格子数量
		"finditeminbagbypos", &CPlayerPackage::FindItemInBagByPos,//通过位置找到在背包的装备
		"finditeminbodybypos", &CPlayerPackage::FindItemInBodyByPos,//通过位置找到在身上的装备
		"getitemcountinbodyallbybaseid", &CPlayerPackage::GetItemCountInBodyAllByBaseId,
		"deleteiteminbagandbodyallbybaseid", &CPlayerPackage::DeleteItemInBagAndBodyAllByBaseId,
		"deleteiteminbodyallbybaseid", &CPlayerPackage::DeleteItemInBodyAllByBaseId,
		"getitemcountbybindtype", &CPlayerPackage::GetItemCountByBindType,
		"deliteminbagbybindtype", &CPlayerPackage::DelAllItemInBagByBindType,
		"opennewbagpage", &CPlayerPackage::OpenNewBagPage,
		"opennewstoragepage", &CPlayerPackage::OpenNewStoragePage,
		"servergetresortbag", &CPlayerPackage::ServerGetResortBag,
		"deleteiteminbag", &CPlayerPackage::DeleteItemInBag,
		"additemtoactbag", &CPlayerPackage::AddItemToActBag,
		"sendactbag", &CPlayerPackage::SendActBag,
		"sendactbagdelitem", &CPlayerPackage::SendActBagDeleteItem,
		"sendactbagupdateitem", &CPlayerPackage::SendActBagUpdateItem,
		"clearactbag", &CPlayerPackage::ClearActBag
	);
	lua.new_usertype<CBUFFManager>("CBUFFManager",
		"OfflineRemoveBuff", &CBUFFManager::OfflineRemoveBuff
	);
	lua.new_usertype<CGameMap>("CGameMap",
		"tm", &CGameMap::m_Timer,						//地图计时器
		"var", &CGameMap::m_vars,						//变量
		"x", &CGameMap::m_nLuaX,
		"y", &CGameMap::m_nLuaY,
		"clearmon", &CGameMap::m_boClearMon,
		"mapinfo", sol::property(&CGameMap::GetMapDataBase),
		"dwclonemapid", sol::property([](const CGameMap& self) {return self.m_svrMapId.part.cloneId; }),
		"lineid", sol::property([](const CGameMap& self) {return self.m_svrMapId.part.line; }),
		"mapid", sol::property([](const CGameMap& self) {return self.GetMapDataBase()->dwMapID; }),
		"mapunionid", sol::property([](CGameMap& self) {return self.getMapUnionId(); }),
		"mapfullid", sol::property([](CGameMap& self) {return self.getFullMapId(); }),
		"scriptid", sol::property([](CGameMap& self) {return self.getMapScriptId(); }),
		"mapname", sol::property([](const CGameMap& self) {return self.GetMapDataBase()->szName; }),
		"mapproperty", sol::property([](const CGameMap& self) {return self.GetMapDataBase()->dwMapProperty; }),//地图的属性，包括能否PK等
		"randomxy", &CGameMap::LuaGetXYInMap,
		"randomxytablerange", &CGameMap::LuaGetRandXYTableInRange,//(空表,需要得到的数目,坐标X,坐标Y,最小范围,最大范围)
		"resetpop", &CGameMap::ResetMapPop,
		"setpop", &CGameMap::SetMapPop,
		"pknosword",sol::property(&CGameMap::isPkNoSword),
		"dienodropequip", sol::property(&CGameMap::isDieNoDropEquip),
		"dienodropall", sol::property(&CGameMap::isDieNoDropAll),
		"nosafezone", sol::property(&CGameMap::isNoSafeZone),
		"noofflinesavehome", sol::property(&CGameMap::isNoOffLineSaveHome),
		"nospaceexcepthome", sol::property(&CGameMap::isNoSpaceExceptHome),
		"nospaceall", sol::property(&CGameMap::isNoSpaceAll),
		"nouseitem", sol::property(&CGameMap::isNoUseItem),
		"noforcepk", sol::property(&CGameMap::isNoForcePk),
		"nosendmail", sol::property(&CGameMap::isNoSendMail),
		"nosendnotice", sol::property(&CGameMap::isNoSendNotice),
		"nodropyuanshen", sol::property(&CGameMap::isNoDropYuanShen),
		"nocleaner", sol::property(&CGameMap::isNoCleaner),
		"nobianshi", sol::property(&CGameMap::isNoBianShi),
		"nodropdown", sol::property(&CGameMap::isNoDropDown),
		"moverecord", sol::property(&CGameMap::isMoveRecord),
		"norandomstone", sol::property(&CGameMap::isNoRandomStone),
		"nohomestone", sol::property(&CGameMap::isNoHomeStone),
		"noxiaofeixie", sol::property(&CGameMap::isNoXiaoFeiXie),
		"iscrossmap", sol::property(&CGameMap::isCrossMap),
		"norelive", sol::property(&CGameMap::isNoRelive),
		"getmapgroupnum", &CGameMap::getMapGroupNum,
		"getsvridtype", &CGameMap::getSvrIdType,
		"getsafetype", &CGameMap::getSafeType,
		"AddMonster", &CGameMap::LuaAddMonster,
		"GetCreature", &CGameMap::GetCreature,
		"GetPlayer", &CGameMap::GetPlayer,
		"GetMonCountById", &CGameMap::GetMonCountById,
		"sendmaptipmsg", &CGameMap::sendMapTipMsg,
		"clonemapexisttime", &CGameMap::m_dwCloneMapExistTime,
		"maptype", &CGameMap::m_dwCloneMapType,
		"setnpcemblemid",&CGameMap::SetNpcEmblemId,
		"GetOneMonsterNear", &CGameMap::LuaGetOneMonsterNear
	);
	// 注册 JsonConfig 类
	registerClass<JsonConfig>(lua,"JsonConfig",
		"GetNpcGen", &JsonConfig::GetNpcGen,
		"GetMagicDataBase", &JsonConfig::GetMagicDataBase,
		"GetEffectDataById",&JsonConfig::GetEffectDataById
	);

}