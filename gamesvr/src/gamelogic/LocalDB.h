#pragma once

#include "qglobal.h"
#include "HashManage.h"
#include "zLogger.h"
#include "lua_base.h"
#include "cmd/Item_cmd.h"
#include <array>

#include "GameDef.h"
#include "JsonConfig.h"

enum emLanguageRes{
	RES_LANG_ERROR=0,
	RES_LANG_GROUP_INVITE_OVERTIME,  //%s 对您的组队邀请未予理睬。
	RES_LANG_GROUP_JOIN_OVERTIME,   //您的入队申请未被理睬。
	RES_LANG_GROUP_OVERTIME,         //%s 对此请求未予理睬。
	RES_LANG_GROUP_RECALL_OVERTIME,  //队员 %s 对召唤请求未予理睬。
	RES_LANG_GROUP_REFUSE_INVITE,    //%s 拒绝了您的组队邀请。
	RES_LANG_GROUP_CANNOT_EXIT_WHEN_FIGHT,  //战斗中无法暂离队伍。
	RES_LANG_GROUP_MASTER_CANNOT_TEMP_LEFT, //队长不可暂离队伍
	RES_LANG_GROUP_CANNOT_RECALL_WHEN_FIGHT,//战斗中无法召回。
	RES_LANG_GROUP_MEMBER_TOO_FAR,         //%s 已经离开了视野，请重新发送归队请求。
	RES_LANG_GROUP_CANNOT_BACK_WHEN_FIGHT,  //您的队伍正在战斗中，请等待队伍战斗结束重新归队。
	RES_LANG_GROUP_HAVENOT_FLY_STONE,      //您没有“%s”。
	RES_LANG_GROUP_HAVENOT_FLY_STONE_TO_BACK, //%s 没有“%s”，无法归队。
	RES_LANG_GROUP_CANNOT_DO_WHEN_FIGHT,  //战斗中无法进行此操作。
	RES_LANG_GROUP_REFUSE_TOBE_MASTER,  //%s 拒绝了成为队长的请求。
	RES_LANG_GROUP_CANNOT_DO_WHEN_GROUP_FIGHT, //队伍正在进行战斗，无法进行此操作
	RES_LANG_GROUP_MASTER_CHANGED,     //队长已经发生变化。
	RES_LANG_GROUP_CANNOT_BE_MASTER_WHEN_LEFT, //暂离后无法成为队长。
	RES_LANG_GROUP_CHANGE_TO_MASTER,  //%s 被提升为队长。
	RES_LANG_GROUP_CHANGE_TO_MASTER_SYSTEM,   //%s 被系统提升为队长。
	RES_LANG_GROUP_KICK, //你已经被请离了队伍。
	RES_LANG_GROUP_KICK_TO_ALL, //%s 被请离了队伍。
	RES_LANG_GROUP_QUIT, // %s 退出了队伍。
	RES_LANG_GROUP_MEMBER_BACK, //%s 回到了队伍。
	RES_LANG_GROUP_JOIN_IN_GROUP,//%s 加入了队伍。
	RES_LANG_GROUP_JOIN_IN_GROUP_SELF,  //您加入到了 %s 的队伍。
	RES_LANG_GROUP_MEMBER_FIGHT_CANNOT_BACK,   //该队员正处于战斗中，暂时无法归队。
	RES_LANG_GROUP_MEMBER_REFUSE_BACK,   //队员 %s 拒绝了您的归队召唤
	RES_LANG_GROUP_CANNOT_BACK_FIGHT,   //战斗中，无法归队。
	RES_LANG_GROUP_CANNOT_BACK_WHEN_GROUP_FIGHT, //队伍正在战斗中，无法归队。
	RES_LANG_GROUP_FLY_STONE_NAME  , //神行遁符

	RES_LANG_GROUP_GET_STH,        //您获得了 %s 物品。
	RES_LANG_GROUP_BAG_FULL_LOST,  //您的包裹满，因此未参加物品分配。
	RES_LANG_GROUP_BAG_ALL_FULL,   //所有队员包裹全满，战利品 %s 丢失了
	RES_LANG_GROUP_MEMBER_GIVEUP,  //%s 玩家放弃
	RES_LANG_GROUP_MEMBER_ROLL_POINT,//%s 需求点数为 %d 点
	RES_LANG_GROUP_MEMBER_GET_ITEM,//%s 玩家获得了 %s 物品。

	RES_LANG_ITEM_GETITEM, //得到 %s 物品。	
	RES_LANG_ITEM_GETITEM_MANY, //得到 %s 物品 %d 个。
	RES_LANG_ITEM_MISSION_OVERTIME,//超过限制时间，任务失败！
	RES_LANG_ITEM_MISSION_FAILED,//任务失败
	RES_LANG_ITEM_NOT_ENOUGH_SPACE,//包裹剩余空格不够，请整理包裹！
	RES_LANG_ITEM_GOLD_MAX,//您的金币超过了上限！
	RES_LANG_CANNOT_RESORT, //现在不允许整理包裹。
	RES_LANG_CHAT_CANNOT_LEVEL, //您的等级不足 %d 级,无法使用此聊天功能。。
	RES_LANG_CHAT_CANNOT_GOLD, //您没有 %d 个贝币,无法使用此聊天功能。
	RES_LANG_CHAT_CANNOT_SP, //您的体力不足 %d ,无法使用此聊天功能。
	RES_LANG_CHAT_CONSUMPTION,	//“发言消耗了5点体力，500金币”
	RES_LANG_CHAT_FAST,	//“您发言频率过快,请稍候再发。”
	RES_LANG_CHAT_HAVEITEM,	//使用该频道需要在背包中放有“%s”道具
	RES_LANG_CHAT_HAVEITEMCONSUMPTION, //发言消耗了一个"%s"道具
	RES_LANG_CHAT_MSGTOOLONG, //抱歉，该频道支持的最大发言长度为"%d"。

	RES_LANG_PACKAGE_EXT_STONE,  //包裹扩展石
	RES_LANG_PACKAGE_EXT_STONE2,//包裹扩展石(高级)
	RES_LANG_STORAGE_EXT_STONE,  //仓库扩展石 /仓库扩展符
	RES_LANG_STORAGE_EXT_STONE2,//仓库扩展石(高级)
	RES_LANG_PACKAGE_EXT_NO_STONE,  //对不起，您的背包中没有延展石（付费增值道具/礼品）
	RES_LANG_PACKAGE_EXT_NO_STONE2, //对不起，您的背包中没有精致延展石（付费增值道具/礼品）
	RES_LANG_STORAGE_EXT_NO_STONE,  //对不起，您的背包中没有延展石（付费增值道具/礼品）/仓库扩展符
	RES_LANG_STORAGE_EXT_NO_STONE2, //对不起，您的背包中没有精致延展石（付费增值道具/礼品）
	RES_LENG_SERVEREXP_RATE,		//服务器 %d 倍经验已经开启,持续时间为 %d 分钟
	RES_LENG_SERVEREXP_RATE_RECOVERY,		//服务器 %d 经验活动已经关闭

	RES_LENG_NPCTRADE_COUNTRYDIFF,		//您不属于 %s伯侯 管辖，我无法为您服务

	RES_LANG_CRET_DOUBLEEXPTIME,		//您的双倍经验时间已经结束
	RES_LANG_CRET_PETDOUBLEEXPTIME,		//您的宠物双倍经验时间已经结束
	RES_LANG_CRET_BLESSINGTIME,			//您的祝福时间已经结束

	RES_LANG_CRET_MALLBUYOK,			//您成功购买了 %s 道具 %d 个，消耗 %d %s
	RES_LANG_CRET_MALLRETURNJIFEN,		//您的积分增加了%s
	RES_LANG_CURRENCYSTR,				//元宝/礼金/积分/现币/古币/金/银/贝

	RES_LANG_GROUPHAVEOTHERCLAN,		//您的队伍里有其他氏族成员,无法进入氏族地图

	RES_LANG_USERCONFIGNOTPRIVITE,		//%s 玩家不接受任何私聊信息
	RES_LANG_YOUNOTCLAN,				//您当前没有加入氏族，不能进入氏族的领地。
	RES_LANG_VISTENPCDOWN,				//在飞行中与地面NPC交互，自动降落至地面。
	RES_LANG_ONFLYMOVEMAP,				//在飞行期间，您将不会遇怪。如需遇怪，请降落地面。
	RES_LANG_NOHPITEM,
	RES_LANG_NOMPITEM,
	RES_LANG_NOZHONGCHENGITEM,
	//////////////////////////////////////////////////////////////////////////
	//APRG
	RES_LANG_MAPCHANGE,					//地图[%s]不允许传送。
	RES_LANG_MAPNORELIVE,				//您所在的地图，不能复活。
	RES_LANG_YOUDIE,					//您被 %s 杀死了。
	RES_LANG_YOUKILL,					//您杀死了 %s 。
	RES_LANG_MAPNOGROUP,				//您目前所在地图，不能组队。
	RES_LANG_MAPNOSAMEGUILDGROUP,		//您目前所在地图，不能与不同公会的玩家组队。
	RES_LANG_MAPMASTERNOGROUP,			//该队长目前所在地图，不能组队。
	RES_LANG_MAPMASTERNOSAMEGUILDGROUP,	//该队长目前所在地图，不同公会玩家不能组队。
	RES_LANG_JOINOTHERGROUP,			//%s 加入到了别人的队伍了。
	RES_LANG_GROUP_MAPNEEDSAMEGUILD,	//您的队伍队长目前所在的地图，需要同公会的成员才能组队，您被系统自动退出队伍。
 	RES_LANG_GROUP_MAPNEEDNOGROUP,		//您的队伍目前所在的地图，不允许组队，您被系统自动退出队伍。
	RES_LANG_PET_LEVELUP,				//您的%s宝宝升到%d级。
	RES_LANG_ITEM_STUDYSKILLSUCCESS,	//成功学习 %s 技能。
	RES_LANG_ITEM_STRENGSUCCESS,		//%s 的 %s 武器第 %d 次强化成功，大家恭喜他吧。
	RES_LANG_ITEM_NOUSEITEM,			//您目前所在地图，禁止使用物品。
	RES_LANG_OTHER_LEADER,				//您的引导员权限已经失效!
	RES_LANG_OTHER_GM,					//您的GM权限已经失效!
	RES_LANG_OTHER_SERVER,				//服务器将于 %s 停机维护,维护原因 %s
	RES_LANG_OTHER_SERVER_RUN_SC,		//服务器将于 %d 秒后停机维护,维护原因 %s
	RES_LANG_OTHER_CHATGOLD,			//发言消耗了%d金币
	RES_LANG_OTHER_NEEDITEM,			//施放技能缺少%s物品
	RES_LANG_MAIL_FAIL,					//您发送的邮件因为某些原因失败，您的附件及发送邮件的费用已通过邮件返还，请查收邮件。
	RES_LANG_CONSIGN_RETURNMAIL,		//对不起，您刚刚提交到寄售商店的物品没能上架成功，您的附件及费用已通过邮件返还，请查收邮件。
	RES_LANG_CONSIGN_FAILMAIL,			//对不起，您刚刚在寄售商店购买的物品因为某些原因未能成功，您花费的货币已通过邮件返还，请查收邮件。
	RES_LANG_RELATION_FRIEND,			//%s 玩家已经添加您为好友
	RES_LANG_TRADE_MALLITEM,			//该类型的货币不能购买商城道具。
	RES_LANG_MAX,
};

enum emAbilityFlag :uint32_t {
	ABILITY_FLAG_BASE = 0x1,
	ABILITY_FLAG_EQUIP = 0x2,
	ABILITY_FLAG_BUFF = 0x4,
	ABILITY_FLAG_LUA = 0x8,
	ABILITY_FLAG_BEAST = 0x10,
	ABILITY_FLAG_GEM = 0x20,
	ABILITY_FLAG_EQUIPBUFF = 0x40,
	ABILITY_FLAG_ATTRPOINT = 0x80,
	ABILITY_FLAG_PASSIVESKILL = 0x100,
	ABILITY_FLAG_ALL = MAXUINT32
};

enum emSpecialAbilityFlag :uint32_t {
	SPECIALABILITY_EQUIP = 0x1,
	SPECIALABILITY_LUA = 0x2,
	SPECIALABILITY_BUFF = 0x4,
	SPECIALABILITY_SKILL = 0x8,
	SPECIALABILITY_FLAG_ALL = MAXUINT32
};

#define DB_EFFECT_TBL			"mydb_effect_base_tbl"					//效果数据库表
#define DB_SPECIALEFFECT_TBL	"mydb_specialeffect_tbl"					//效果数据库表
#define DB_ITEMS_TBL			"mydb_item_base_tbl"				//物品数据库表
#define DB_EFITEMS_TBL			"mydb_item_effect_base_tbl"			//药物效果数据库
#define DB_ARITEMS_TBL			"mydb_item_droprandom_base_tbl"     //掉落随机属性表
#define DB_MONSTERS_TBL			"mydb_monster_tbl"			//物品数据库表
#define DB_MAGICS_TBL			"mydb_magic_tbl"			//技能数据库表
#define DB_MAGICS_BUFF_TBL		"mydb_magicbuff_tbl"		//技能BUFF表
#define DB_DRUG_BUFF_TBL		"mydb_drugbuff_tbl"			//药品BUFF表
#define DB_MAGICS_EFFECT_TBL	"mydb_magiceffect_tbl"		//魔法效果表
#define DB_MAPINFO_TBL			"mydb_mapinfo_tbl"			//地图数据库
#define DB_MAPGATE_TBL			"mydb_mapgate_tbl"			//门点数据库
#define DB_MONGEN_TBL			"mydb_mongen_tbl"			//刷怪数据库
#define DB_NPCGEN_TBL			"mydb_npcgen_tbl"			//刷NPC数据库
#define	DB_MONDROPITEM_TBL		"mydb_mondropitem_tbl"		//怪物掉落装备数据库
#define DB_SUBMONDROPITEM_TBL	"mydb_submondropitem_tbl"	//怪物掉落装备子数据库
#define	DB_PLAYERUPLEVEL_TBL	"mydb_playerability_tbl"	//人物升级获得属性数据库
#define DB_PETABILITY_TBL		"mydb_petability_tbl"		//APRG宠物升级获得属性数据库

#define _RESERVE_MON_ID_		5000
#define _MIN_MON_ID_			10000		
#define _MAX_MON_ID_			0xffffff
#define _MIN_MAGICID_			200
#define _MAX_MAGICID_			0xffff
#define _RESERVE_MAP_ID_		200
#define _MAX_MAP_ID_			0xffff
#define SKILL_FORMULA			"SkillFormula"		//技能公式
#define BUFF_FORMULA			"BuffFormula"		//BUFF公式

////=======================================
//效果表，物品，BUFF都用，和人物属性一样
enum emMCType{
	NO_TYPE,		//没有魔法攻击
	MC_TYPE,		//自然
	SC_TYPE,		//灵魂
	FULL_TYPE,		//全系
};


////=======================================
//物品表
enum emSexType{
	NO_SEX,				//没有性别
	MAN_SEX,			//男
	WOMAN_SEX,			//女
};

enum emTypeDef
{
	ITEM_TYPE_GOLD = 0,					// 0货币
	ITEM_TYPE_NORMAL = 1,               // 1普通物品，多为任务品和材料
	ITEM_TYPE_EQUIP = 2,				// 2装备
	ITEM_TYPE_DRUG = 3,					// 3药品，会响应“使用”按钮，点使用后触发脚本ID
	ITEM_TYPE_RESPONSEE = 4,			// 4道具,响应“使用”按钮,点使用后触发脚本ID
	ITEM_TYPE_VEHICLE = 5,				// 5载具
	ITEM_TYPE_PET = 6,					// 6宠物
	ITEM_TYPE_IMPRINT = 7,				// 7称号印记
	ITEM_TYPE_TITLE = 8,				// 8称号
	ITEM_TYPE_CLOUTH = 9,				// 9时装
	ITEM_TYPE_LEGION_BADGE = 10,		// 10军团徽章
	ITEM_TYPE_MECHANICAL = 11,			// 11机械装备
	ITEM_TYPE_VIRTUAL = 12,				// 12 虚拟物品
	ITEM_TYPE_ZIRCON = 13,				// 13 锆石
};

enum emDrugType{
	DRUG_TYPE_DEFALT = 0,
	DRUG_TYPE_HP,		//红药
	DRUG_TYPE_MP,		//蓝药
	DRUG_TYPE_HPANDMP,	//红蓝同时加的药
};

enum emShareCdType {
	CD_TYPE_DEFALT = 0, // 默认不用共享cd
	CD_TYPE_Drug,		// 药品共享cd
	CD_TYPE_TimeDrug,	// 闪避时空药水cd
};

enum emRareType{
	RARE_TYPE_NORMAL,		// 普通装备 白色品质
	RARE_TYPE_BLUE,			// 蓝色品质
	RARE_TYPE_GOLD,			// 金色品质
};

enum emGuardType {
	TYPE_NONE    = 0,			
	TYPE_ATTACK  = 1,			// 守护攻击型
	TYPE_DEFENSE = 2,			// 守护防御型
	TYPE_SUPPORT = 3,			// 守护辅助型
};

struct stAttrNum{
	BYTE btattrtype;
	DWORD dwattrnum;
	stAttrNum(){
		ZeroMemory(this,sizeof(*this));
	}
};

struct stARpgAbility;

//魔法
enum emActiveType {
	ACTIVETYPE_NONE,		//不区分
	ACTIVETYPE_ACTIVE,		//主动
	ACTIVETYPE_PASSIVE,		//被动
};

enum emAttackType{
	NONE_ATTACK,		//不区分
	NEARLY_ATTACK,		//近身
	FAR_ATTACK,			//远程
};

enum emAtomType{
	ATOM_NO, 			//无元素
	ATOM_JIN, 			//金
	ATOM_MU, 			//木
	ATOM_SHUI, 			//水
	ATOM_HUO, 			//火
	ATOM_TU, 			//土
};

enum emDamageType {
	DAMAGETYPE_NONE,		//不区分
	DAMAGETYPE_PHYSICAL,	//物理
	DAMAGETYPE_MAGIC,		//魔法
};

enum emDamageEffectType {
	Normal = 0,	    //普通伤害
	PCritical = 1,	//暴击伤害
	Miss = 2,		//Miss
	StrikeBack = 3,	//反击
};

enum emModelType{
	MODELTYPE_NONE,			//不区分
	MODELTYPE_HUMAN,		//人
	MODELTYPE_MON,			//怪物
};

enum emEnemyType {
	ENEMYTYPE_NONE,			//不区分
	ENEMYTYPE_FRIEND,		//友方
	ENEMYTYPE_ENEMY,		//敌方
};


enum emMagicSkillType {
	MAGICTYPE_BASIC,			//基础,使用后进入CD	
	MAGICTYPE_COMMON,			//常规伤害
	MAGICTYPE_GROUND,			//地面魔法
	MAGICTYPE_MOVE,				//移动
	MAGICTYPE_SHIELD,			//护盾
	MAGICTYPE_BUFF,				//BUFF
};

enum emMagicFuncType{
	MAGICFUNC_NONE,				//不区分
	MAGICFUNC_ASSIST,			//辅助魔法
	MAGICFUNC_OTHER,			//其他魔法
};

enum emShapeType {
	SHAPE_SELF = -1,			//自身
	SHAPE_NEAR_POINT = 0,		//近程单点
	SHAPE_FAR_POINT = 6,		//远程单点
};

struct stSkillSet{
	DWORD dwSkillId;
	BYTE btSkillLv;
	stSkillSet(){
		ZEROSELF;
	}
};

enum emSubDropMod{
	emNULL_MOD,			//没有方式
	emNORMAL_MOD,		//普通包方式
	emBAG_MOD,			//爆率包方式
};

enum emSubDropType{
	emNULL_NUM,			//没有计数
	emPLAYER_NUM,		//玩家计数
	emSERVER_NUM,		//服务器计数
	emRANDOM_NUM,		//随机计数
};

//爆率
enum emDropItemType{
	DROPITEM_BINDING=1,			//拾取绑定
	DROPITEM_MAXLV30=2,			//差距30等级不爆物品
	DROPITEM_BAGORMAP=4,			//爆到地面或者身上,暂时默认到地面
	DROPITEM_SUPERMOSTER=8,		//只有精英怪爆出
};

//=======================================
struct stServerConfig{
	char szServerName[_MAX_NAME_LEN_];

	char szMapDir[MAX_PATH];
	char szScriptDir[MAX_PATH];		
	char szMongenDir[MAX_PATH];		//商人方向

	char szHomeMap[_MAX_NAME_LEN_];
	int nHomeX;
	int nHomeY;
	DWORD dwsavesvrparaminterval;	//保存系统参数间隔时间

	DWORD shutdowntime;
	DWORD itemtmpid;
	DWORD grouptmpid;

	//DWORD specialsearchinterval;
	DWORD searchinterval;

	DWORD dwminactioninterval;    //转身最小间隔时间
	DWORD dwsavercdintervaltime; //数据保存间隔时间

	static bool refresh(stServerConfig* pdata,stServerConfig* param){
		*param=*pdata;
		return true;
	}

	stServerConfig(){
		ZeroMemory(this,sizeof(*this));
		sprintf_s(szServerName,sizeof(szServerName),"%s","测试");

		sprintf_s(szMapDir,sizeof(szMapDir),"%s",".\\map\\");
		sprintf_s(szScriptDir,sizeof(szScriptDir),"%s",".\\script\\");
		sprintf_s(szMongenDir,sizeof(szMongenDir),"%s",".\\mongen\\");

		sprintf_s(szHomeMap,sizeof(szHomeMap),"%s","0");
		nHomeX=130;
		nHomeY=130;

		dwsavesvrparaminterval=15*60; //15分钟保存一次

		shutdowntime=0;
		itemtmpid=0;
		grouptmpid=0;

		//specialsearchinterval=1000*2;
		//searchinterval=1000*60*60*6;
		searchinterval=1000*2;

		dwminactioninterval=300;    //所有动作之间最小间隔时间(走 跑 攻击 魔法)
		dwsavercdintervaltime=15*60; //数据保存间隔时间
	}
};

inline static const dbCol stServerConfigDefine[] = {
	{_DBC_SO_("servername", DB_STR, stServerConfig, szServerName)},
	{_DBC_SO_("mapdir", DB_STR, stServerConfig, szMapDir)},
	{_DBC_SO_("scriptdir", DB_STR, stServerConfig, szScriptDir)},
	{_DBC_SO_("mongendir", DB_STR, stServerConfig, szMongenDir)},
	{_DBC_SO_("homemap", DB_STR, stServerConfig, szHomeMap)},
	{_DBC_SO_("homex", DB_DWORD, stServerConfig, nHomeX)},
	{_DBC_SO_("homey", DB_DWORD, stServerConfig, nHomeY)},
	{_DBC_SO_("savesvrparaminterval", DB_DWORD, stServerConfig, dwsavesvrparaminterval)},
	{_DBC_SO_("shutdowntime", DB_DWORD, stServerConfig, shutdowntime)},
	{_DBC_SO_("itemtmpid", DB_DWORD, stServerConfig, itemtmpid)},
	{_DBC_SO_("grouptmpid", DB_DWORD, stServerConfig, grouptmpid)},
	{_DBC_SO_("searchinterval", DB_DWORD, stServerConfig, searchinterval)},
	{_DBC_SO_("minactioninterval", DB_DWORD, stServerConfig, dwminactioninterval)},
	{_DBC_SO_("savercdintervaltime", DB_DWORD, stServerConfig, dwsavercdintervaltime)},
	{_DBC_MO_NULL_(stServerConfig)},
};


//=======================================
typedef stunion3< DWORD ,WORD,BYTE,BYTE > stServerLimitMapId;
#define make_svrlimitmapid(varname,mapid,countryid,lineid)		stServerLimitMapId	varname;varname._p1=(mapid);varname._p2=(countryid);varname._p3=(lineid);



#pragma pack(push,1)
//属性列表
struct stSpecialAbility
{
	int nFullSkillLv;				// 所有技能等级
	int nHundSkillLv;				// 百级技能等级
	int nFirearmsMasterLv;			// 枪械大师等级
	int nContinuousFiringLv;		// 连射等级			
	int nSnipeLv;					// 狙击等级
	int nLivelyLv;					// 灵动等级
	int nSteelSkinLv;				// 钢铁皮肤等级
	int nThrillLv;					// 兴奋等级
	int nCounterpunchLv;			// 反击等级
	int nSelfMedicationLv;			// 自我医疗等级
	int nConcentrationLv;			// 精神集中等级
	int nJuckLv;					// 闪避等级
	int nPunctureLv;				// 穿刺等级
	int nRestoreLv;					// 回复等级
	int nMuseLv;					// 冥思等级
	int nSpiritStrengthenLv;		// 精神强化等级
	int nMagicRefineLv;				// 魔法精修等级
	int nSpellMasterLv;				// 法术大师等级

	stSpecialAbility() {
		ZeroMemory(this, sizeof(*this));
	}
	void Clear() {
		ZeroMemory(this, sizeof(*this));
	}
	stSpecialAbility& operator+=(const stSpecialAbility &abi) {
		nFullSkillLv += abi.nFullSkillLv;
		nHundSkillLv += abi.nHundSkillLv;
		nFirearmsMasterLv += abi.nFirearmsMasterLv;
		nContinuousFiringLv += abi.nContinuousFiringLv;
		nSnipeLv += abi.nSnipeLv;
		nLivelyLv += abi.nLivelyLv;
		nSteelSkinLv += abi.nSteelSkinLv;
		nThrillLv += abi.nThrillLv;
		nCounterpunchLv += abi.nCounterpunchLv;
		nSelfMedicationLv += abi.nSelfMedicationLv;
		nConcentrationLv += abi.nConcentrationLv;
		nJuckLv += abi.nJuckLv;
		nPunctureLv += abi.nPunctureLv;
		nRestoreLv += abi.nRestoreLv;
		nMuseLv += abi.nMuseLv;
		nSpiritStrengthenLv += abi.nSpiritStrengthenLv;
		nMagicRefineLv += abi.nMagicRefineLv;
		nSpellMasterLv += abi.nSpellMasterLv;
		return *this;
	}
	void Add(const mydb_specialeffect_tbl&abi) {
		nFullSkillLv += abi.nFullSkillLv;
		nHundSkillLv += abi.nHundSkillLv;
		nFirearmsMasterLv += abi.nFirearmsMasterLv;
		nContinuousFiringLv += abi.nContinuousFiringLv;
		nSnipeLv += abi.nSnipeLv;
		nLivelyLv += abi.nLivelyLv;
		nSteelSkinLv += abi.nSteelSkinLv;
		nThrillLv += abi.nThrillLv;
		nCounterpunchLv += abi.nCounterpunchLv;
		nSelfMedicationLv += abi.nSelfMedicationLv;
		nConcentrationLv += abi.nConcentrationLv;
		nJuckLv += abi.nJuckLv;
		nPunctureLv += abi.nPunctureLv;
		nRestoreLv += abi.nRestoreLv;
		nMuseLv += abi.nMuseLv;
		nSpiritStrengthenLv += abi.nSpiritStrengthenLv;
		nMagicRefineLv += abi.nMagicRefineLv;
		nSpellMasterLv += abi.nSpellMasterLv;
	}
};


//////////////////////////////////////////////////////////////////////////
#pragma pack(pop)

#pragma pack(push,1)

//属性列表
struct stARpgAbility
{
	std::array<int, tosizet(AttrID::Count)> attrs;	
	__int64 i64FightScore;		//战斗评分
	int& operator[](AttrID id) {
		return attrs[static_cast<uint8_t>(id)];
	}
	const int& operator[](AttrID id) const{
		return attrs[static_cast<uint8_t>(id)];
	}
	// 判断传入的整数是否从1开始且小于AttrID::Count
	static bool IsValidAttrID(size_t id) {
		return id >= tosizet(AttrID::Strength) && id < tosizet(AttrID::Count);
	}

	stARpgAbility() {
		ZeroMemory(this, sizeof(*this));
	}
	void Clear() {
		ZeroMemory(this, sizeof(*this));
	}

	void CalExtraAttr(stARpgAbility& abi) {
		auto self = *this;
		abi.Clear();
		abi[AttrID::MaxPAtk] = static_cast<int>(ceil(self[AttrID::MaxPAtk] * self[AttrID::PAtkPer] / Denom)) + self[AttrID::AllAttackAdd];
		abi[AttrID::MinPAtk] = static_cast<int>(floor(self[AttrID::MinPAtk] * self[AttrID::PAtkPer] / Denom)) + self[AttrID::AllAttackAdd];
		abi[AttrID::MaxMAtk] = static_cast<int>(ceil(self[AttrID::MaxMAtk] * self[AttrID::MaxMAtk] / Denom)) + self[AttrID::AllAttackAdd];
		abi[AttrID::MinMAtk] = static_cast<int>(floor(self[AttrID::MinMAtk] * self[AttrID::MaxMAtk] / Denom)) + self[AttrID::AllAttackAdd];
		abi[AttrID::PDef] = static_cast<int>(ceil(self[AttrID::PDef] * self[AttrID::PDefPer] / Denom));
		abi[AttrID::MDef] = static_cast<int>(ceil(self[AttrID::MDef] * self[AttrID::MDefPer] / Denom));
		auto hp = self[AttrID::MaxHP] * (1.0f + self[AttrID::MaxHpPer] / Denom) * (1.0f + self[AttrID::FinalMaxHp] / Denom);
		abi[AttrID::MaxHP] = hp - self[AttrID::MaxHP];
		auto mp = self[AttrID::MaxMP] * (1.0f + self[AttrID::MaxMpPer] / Denom) * (1.0f + self[AttrID::FinalMaxMp] / Denom);
		abi[AttrID::MaxMP] = mp - self[AttrID::MaxMP];
		abi[AttrID::MaxPP] = self[AttrID::MaxPP] * self[AttrID::MaxPpPer] / Denom;
		abi[AttrID::HpRestore] = self[AttrID::HpRestore] * self[AttrID::HpRestorePer] / Denom;
		abi[AttrID::MpRestore] = self[AttrID::MpRestore] * self[AttrID::MpRestorePer] / Denom;
		abi[AttrID::PpRestore] = self[AttrID::PpRestore] * self[AttrID::PpRestorePer] / Denom;
	}
	// 扣除百分比属性
	void DecPerAttr(const stARpgAbility& abi) {
		attrs -= abi.attrs;
	}
	
	stARpgAbility& operator=(const stARpgAbility& abi) {
		attrs = abi.attrs;
		return *this;
	}
	stARpgAbility operator + (const stARpgAbility& abi) {
		stARpgAbility self = *this;
		self += abi;
		return self;
	}
	stARpgAbility operator - (const stARpgAbility& abi) {
		stARpgAbility self = *this;
		self -= abi;
		return self;
	}
	stARpgAbility& operator+=(const stARpgAbility& abi) {
		attrs += abi.attrs;
		return *this;
	}
	stARpgAbility& operator+=(const stSimpleAbility& abi) {
		attrs[tosizet(AttrID::Strength)] += abi.nStrength;
		attrs[tosizet(AttrID::Physique)] += abi.nPhysique;
		attrs[tosizet(AttrID::Agility)] += abi.nAgility;
		attrs[tosizet(AttrID::Wisdom)] += abi.nWisdom;
		attrs[tosizet(AttrID::Intelligence)] += abi.nIntelligence;
		return *this;
	}
	stARpgAbility& operator+=(const stEffectDataLoaderBase& abi) {
		attrs = attrs + abi.attrs;
		return *this;
	}

	stARpgAbility& operator+=(const stMonsterDataBase& abi) {
		attrs[tosizet(AttrID::MaxHP)] += abi.nMaxHP;
		attrs[tosizet(AttrID::MaxMP)] += abi.nMaxMP;
		attrs[tosizet(AttrID::MaxPAtk)] += abi.nMaxPAtk;
		attrs[tosizet(AttrID::MinPAtk)] += abi.nMinPAtk;
		attrs[tosizet(AttrID::PDef)] += abi.nPDef;
		attrs[tosizet(AttrID::MDef)] += abi.nMDef;
		attrs[tosizet(AttrID::Hit)] += abi.nHit;
		attrs[tosizet(AttrID::Juck)] += abi.nJuck;
		attrs[tosizet(AttrID::HitVal)] += abi.nHitVal;
		attrs[tosizet(AttrID::JuckVal)] += abi.nJuckVal;
		attrs[tosizet(AttrID::HpRestore)] += abi.nHpRestore;
		attrs[tosizet(AttrID::MpRestore)] += abi.nMpRestore;
		attrs[tosizet(AttrID::CritRate)] += abi.nCritRate;
		attrs[tosizet(AttrID::CritDec)] += abi.nCritDefense;
		attrs[tosizet(AttrID::CritMul)] += abi.nAtkCrit;
		attrs[tosizet(AttrID::CritResist)] += abi.nCritResist;

		//nDecAttackDamage += abi.nDecAttackDamage;
		return *this;
	}
	stARpgAbility& operator-=(const stARpgAbility& abi) {
		attrs -= abi.attrs;
		return *this;
	}
	stARpgAbility& operator-=(const stMonsterDataBase& abi) {
		attrs[tosizet(AttrID::MaxHP)] -= abi.nMaxHP;
		attrs[tosizet(AttrID::MaxMP)] -= abi.nMaxMP;
		attrs[tosizet(AttrID::MaxPAtk)] -= abi.nMaxPAtk;
		attrs[tosizet(AttrID::MinPAtk)] -= abi.nMinPAtk;
		attrs[tosizet(AttrID::PDef)] -= abi.nPDef;
		attrs[tosizet(AttrID::MDef)] -= abi.nMDef;
		attrs[tosizet(AttrID::Hit)] -= abi.nHit;
		attrs[tosizet(AttrID::Juck)] -= abi.nJuck;
		attrs[tosizet(AttrID::HitVal)] -= abi.nHitVal;
		attrs[tosizet(AttrID::JuckVal)] -= abi.nJuckVal;
		attrs[tosizet(AttrID::HpRestore)] -= abi.nHpRestore;
		attrs[tosizet(AttrID::MpRestore)] -= abi.nMpRestore;
		attrs[tosizet(AttrID::CritRate)] -= abi.nCritRate;
		attrs[tosizet(AttrID::CritDec)] -= abi.nCritDefense;
		attrs[tosizet(AttrID::CritMul)] -= abi.nAtkCrit;
		attrs[tosizet(AttrID::CritResist)] -= abi.nCritResist;
		//nDecAttackDamage -= abi.nDecAttackDamage;
		return *this;
	}


	void Add(const stEffectDataLoaderBase& abi) {
		attrs += abi.attrs;
	}
	
	
	// 百分比属性
	void perProperty(double cof) {
		//nPDef = cof * nPDef;
	}

	const char* LuaGetFightScore() {
		_GET_TH_LOOPCHARBUF(64, true);
		ZeroMemory(ptlsbuf, 64);
		_i64toa(i64FightScore, ptlsbuf, 10);
		const char* ret = vformat("%s", ptlsbuf);
		return ret;
	}
	void LuaSetFightScore(const char* szFightScore) {
		i64FightScore = _atoi64(szFightScore);
	}
};


//////////////////////////////////////////////////////////////////////////
#pragma pack(pop)
