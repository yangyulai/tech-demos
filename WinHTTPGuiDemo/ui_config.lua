-- ui_config.lua
-- 一个按钮就是一个表，包含 label/id/位置/尺寸，以及可选的回调函数名
buttons = {
    { label = "道具buff",   id = 3001, x = 10,  y = 10,  w = 100, h = 24, action = "mydb_drugbuff_tbl.xlsx" },
    { label = "效果表",   id = 3010, x = 370, y = 44,  w = 100, h = 24, action = "mydb_effect_base_tbl.xlsx" },
    { label = "道具表",   id = 3011, x = 490, y = 44,  w = 100, h = 24, action = "mydb_item_base_tbl.xlsx" },
    { label = "技能表",   id = 3010, x = 370, y = 44,  w = 100, h = 24, action = "mydb_magic_tbl.xlsx" },
    { label = "技能buff",   id = 3011, x = 490, y = 44,  w = 100, h = 24, action = "mydb_magicbuff_tbl.xlsx" },
    { label = "传送阵",   id = 3001, x = 10,  y = 10,  w = 100, h = 24, action = "mydb_mapgate_tbl.xlsx" },
    { label = "地图",   id = 3001, x = 10,  y = 10,  w = 100, h = 24, action = "mydb_mapinfo13824_tbl.xlsx" },
    { label = "掉落表",   id = 3001, x = 10,  y = 10,  w = 100, h = 24, action = "mydb_mondropitem_tbl.xlsx" },
    { label = "刷怪表",   id = 3001, x = 10,  y = 10,  w = 100, h = 24, action = "mydb_mongen_tbl.xlsx" },
    { label = "怪物表",   id = 3002, x = 130, y = 10,  w = 100, h = 24, action = "mydb_monster_tbl.xlsx" },
    { label = "npc配置",   id = 3003, x = 250, y = 10,  w = 100, h = 24, action = "mydb_npcgen_tbl.xlsx" },
    { label = "宠物表",   id = 3004, x = 370, y = 10,  w = 100, h = 24, action = "mydb_petability_tbl.xlsx" },
    { label = "人物等级",   id = 3005, x = 490, y = 10,  w = 100, h = 24, action = "mydb_playerability_tbl.xlsx" },
    { label = "任务表",   id = 3006, x = 610, y = 10,  w = 100, h = 24, action = "mydb_questdata_tbl.xlsx" },
    { label = "特殊效果表",   id = 3007, x = 10, y = 44,  w = 100, h = 24, action = "mydb_specialeffect_tbl.xlsx" },
    { label = "掉落包",   id = 3009, x = 250, y = 44,  w = 100, h = 24, action = "mydb_submondropitem_tbl.xlsx" },
}
describe = "在这里填写工具的使用说明，支持多行显示…\r\n可包含快捷键说明11。"