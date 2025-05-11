local cpp_file_path = "./JsonStruct.h"
local initial_globals = {}
for k in pairs(_G) do
    initial_globals[k] = true
end

mydb_mongen_tbl = {
	{column_name="本表编号", name = "dwGenId", type = "int", desc = "本表编号"},
	{column_name="地图ID",name = "dwMapId", type = "int", desc = "地图ID"},
	{column_name="怪物ID",name = "dwMonsterId", type = "int", desc = "怪物ID"},
	{column_name="显示名称",name = "szMonShowName", type = "std::string", desc = "显示名称"},
	{column_name="中心点X",name = "nX", type = "int", desc = "中心点X"},
	{column_name="中心点Y",name = "nY", type = "int", desc = "中心点Y"},
	{column_name="刷新半径",name = "nRange", type = "int", desc = "刷新半径"},
	{column_name="刷新个数",name = "nCount", type = "int", desc = "刷新个数"},
	{column_name="刷新时间间隔",name = "dwGenTime", type = "int", desc = "刷新时间间隔"},
	{column_name="刷新时间间隔类型",name = "dwGenTimetype", type = "int", desc = "刷新时间间隔类型"},
}
mydb_npcgen_tbl = {
    {column_name="NPC编号", name = "id", type = "int", desc = "NPC唯一标识符"},
    {column_name="NPC名字", name = "name", type = "std::string", desc = "NPC显示名称"},
    {column_name="NPC类型", name = "type", type = "int", desc = "NPC类型标识"},
    {column_name="刷新地图", name = "mapName", type = "std::string", desc = "所属地图名称"},
    {column_name="刷新地图编号", name = "mapId", type = "int", desc = "所属地图ID"},
    {column_name="脚本ID", name = "scriptId", type = "int", desc = "关联的脚本ID"},
    {column_name="怪物ID", name = "monsterId", type = "int", desc = "关联的怪物模板ID"},
    {column_name="坐标X", name = "x", type = "int", desc = "生成坐标X轴"},
    {column_name="坐标Y", name = "y", type = "int", desc = "生成坐标Y轴"},
    {column_name="朝向", name = "direction", type = "int", desc = "生成时的朝向角度"}
}
mydb_playerability_tbl = {
    { column_name = "等级",               name = "dwLevel",         type = "int", desc = "等级" },
    { column_name = "职业",               name = "dwJob",           type = "int", desc = "职业" },
    { column_name = "战斗经验",           name = "i64NeedExp",      type = "int", desc = "战斗经验" },
    { column_name = "生命",               name = "dwHp",            type = "int", desc = "生命" },
    { column_name = "精神",               name = "dwMp",            type = "int", desc = "精神" },
    { column_name = "体力",               name = "dwPP",            type = "int", desc = "体力" },
    { column_name = "物攻下限",           name = "dwMinPAtk",       type = "int", desc = "物攻下限" },
    { column_name = "物攻上限",           name = "dwMaxPAtk",       type = "int", desc = "物攻" },
    { column_name = "魔攻",               name = "dwMaxMAtk",       type = "int", desc = "魔攻" },
    { column_name = "物防",               name = "dwPDef",          type = "int", desc = "物防" },
    { column_name = "魔防",               name = "dwMDef",          type = "int", desc = "魔防" },
    { column_name = "命中率",             name = "dwHit",           type = "int", desc = "命中率" },
    { column_name = "闪避率",             name = "dwJuck",          type = "int", desc = "闪避率" },
    { column_name = "生命回复",           name = "nHpRestore",      type = "int", desc = "生命回复" },
    { column_name = "精神回复",           name = "nMpRestore",      type = "int", desc = "精神回复" },
    { column_name = "体力回复",           name = "nPpRestore",      type = "int", desc = "体力回复" },
    { column_name = "攻击间隔",           name = "nAttackInterval", type = "int", desc = "攻击间隔（毫秒）" },
    { column_name = "释放间隔",           name = "nReleaseInterval",type = "int", desc = "释放间隔（毫秒）" },
    { column_name = "移动间隔",           name = "nMoveInterval",   type = "int", desc = "移动间隔（毫秒）" },
    { column_name = "PVE系数",            name = "nPveCof",         type = "int", desc = "PVE系数" },
    { column_name = "PVP系数",            name = "nPvpCof",         type = "int", desc = "PVP系数" },
    { column_name = "体力移动消耗",       name = "nPpMoveCost",     type = "int", desc = "体力移动消耗" },
    { column_name = "击杀最大怪物等级",   name = "nKillMaxMonLv",   type = "int", desc = "击杀最大怪物等级（影响击杀获得奖励）" },
    { column_name = "击杀最小怪物等级",   name = "nKillMinMonLv",   type = "int", desc = "击杀最小怪物等级（影响击杀获得奖励）" },
}
mydb_mapgate_tbl = {
    { column_name = "srcmapid", name = "dwSrcMapId", type = "int", desc = "来源地图编号" },
    { column_name = "sx",   name = "wSx",        type = "int16_t", desc = "传送口起点 X 坐标" },
    { column_name = "sy",   name = "wSy",        type = "int16_t", desc = "传送口起点 Y 坐标" },
    { column_name = "dstmapid", name = "dwDstMapId", type = "int", desc = "目标地图编号" },
    { column_name = "dx", name = "wDx",        type = "int16_t", desc = "传送口目标 X 坐标" },
    { column_name = "dy", name = "wDy",        type = "int16_t", desc = "传送口目标 Y 坐标" },
    { column_name = "scriptidx",   name = "nscriptidx", type = "int", desc = "触发脚本索引" },
}

local current_globals = {}
for k, v in pairs(_G) do
	if not initial_globals[k] then  -- 过滤掉初始存在的全局变量
		current_globals[k] = v
	end
end
function export_struct()
	local file = io.open(cpp_file_path, "w")
	if file then
		file:write("#pragma once\n")
		file:write("#include <iguana/iguana.hpp>\n")
		for tbl_name,tbl  in pairs(current_globals ) do
			if(type(tbl)=="table") then
				file:write(string.format("struct %s {\n",tbl_name))
				for kk,vv in ipairs(tbl) do
					file:write(string.format("	%s %s; //%s\n",vv.type,vv.name,vv.desc))
				end
				file:write("};\n#if __cplusplus < 202002L\nYLT_REFL(")
				file:write(tbl_name)
				file:write(", ")
				for kk,vv in ipairs(tbl) do
					file:write(vv.name)
					if(kk~=#tbl) then
						file:write(", ")
					end
				end
				file:write(")\n#endif\n")
			end
		end
		file:close()
	else
		print("无法打开文件进行写入！")
	end
end