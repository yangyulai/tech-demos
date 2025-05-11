#pragma once
#include <iguana/iguana.hpp>
struct mydb_mongen_tbl {
	int dwGenId; //本表编号
	int dwMapId; //地图ID
	int dwMonsterId; //怪物ID
	std::string szMonShowName; //显示名称
	int nX; //中心点X
	int nY; //中心点Y
	int nRange; //刷新半径
	int nCount; //刷新个数
	int dwGenTime; //刷新时间间隔
	int dwGenTimetype; //刷新时间间隔类型
};
#if __cplusplus < 202002L
YLT_REFL(mydb_mongen_tbl, dwGenId, dwMapId, dwMonsterId, szMonShowName, nX, nY, nRange, nCount, dwGenTime, dwGenTimetype)
#endif
struct mydb_npcgen_tbl {
	int id; //NPC唯一标识符
	std::string name; //NPC显示名称
	int type; //NPC类型标识
	std::string mapName; //所属地图名称
	int mapId; //所属地图ID
	int scriptId; //关联的脚本ID
	int monsterId; //关联的怪物模板ID
	int x; //生成坐标X轴
	int y; //生成坐标Y轴
	int direction; //生成时的朝向角度
};
#if __cplusplus < 202002L
YLT_REFL(mydb_npcgen_tbl, id, name, type, mapName, mapId, scriptId, monsterId, x, y, direction)
#endif
struct mydb_mapgate_tbl {
	int dwSrcMapId; //来源地图编号
	int16_t wSx; //传送口起点 X 坐标
	int16_t wSy; //传送口起点 Y 坐标
	int dwDstMapId; //目标地图编号
	int16_t wDx; //传送口目标 X 坐标
	int16_t wDy; //传送口目标 Y 坐标
	int nscriptidx; //触发脚本索引
};
#if __cplusplus < 202002L
YLT_REFL(mydb_mapgate_tbl, dwSrcMapId, wSx, wSy, dwDstMapId, wDx, wDy, nscriptidx)
#endif
struct mydb_playerability_tbl {
	int dwLevel; //等级
	int dwJob; //职业
	int i64NeedExp; //战斗经验
	int dwHp; //生命
	int dwMp; //精神
	int dwPP; //体力
	int dwMinPAtk; //物攻下限
	int dwMaxPAtk; //物攻
	int dwMaxMAtk; //魔攻
	int dwPDef; //物防
	int dwMDef; //魔防
	int dwHit; //命中率
	int dwJuck; //闪避率
	int nHpRestore; //生命回复
	int nMpRestore; //精神回复
	int nPpRestore; //体力回复
	int nAttackInterval; //攻击间隔（毫秒）
	int nReleaseInterval; //释放间隔（毫秒）
	int nMoveInterval; //移动间隔（毫秒）
	int nPveCof; //PVE系数
	int nPvpCof; //PVP系数
	int nPpMoveCost; //体力移动消耗
	int nKillMaxMonLv; //击杀最大怪物等级（影响击杀获得奖励）
	int nKillMinMonLv; //击杀最小怪物等级（影响击杀获得奖励）
};
#if __cplusplus < 202002L
YLT_REFL(mydb_playerability_tbl, dwLevel, dwJob, i64NeedExp, dwHp, dwMp, dwPP, dwMinPAtk, dwMaxPAtk, dwMaxMAtk, dwPDef, dwMDef, dwHit, dwJuck, nHpRestore, nMpRestore, nPpRestore, nAttackInterval, nReleaseInterval, nMoveInterval, nPveCof, nPvpCof, nPpMoveCost, nKillMaxMonLv, nKillMinMonLv)
#endif
