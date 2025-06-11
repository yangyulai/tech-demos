#ifndef __GAME_CPLAYEROBJ_H__
#define __GAME_CPLAYEROBJ_H__

#include "qglobal.h"
#include "LocalDB.h"
#include "network/packet.h"
#include "network/sockettask.h"
#include "../gamegatewaySession.h"
#include "BaseCreature.h"
#include "HashManage.h"
#include "Package.h"
#include "Trade.h"
#include "ShortCuts.h"
#include "Pet.h"
#include "cmd/Guild_cmd.h"
#include "cmd/Rank_cmd.h"
#include "cmd/relation_cmd.h"
#include "cmd/Stall_cmd.h"
#include "MonsterObj.h"
#include "cmd/Group_cmd.h"
#include <unordered_map>

#include "PetManager.h"
#include "cmd/Trade_cmd.h"

#define _MAX_TAGSNUM_	8	//人物最大任务标签数
#define _MAX_MOVE_NUM_	3	//人物移动间隔计算步数
#define _MAX_DOUBLE_TIME_	3600*96	//人物累计双倍最大时间

#define SENDMSG2SUPER(cmd,stusername,subrt,sublen)\
BUFFER_CMD(cmd,cmdname,stBasePacket::MAX_PACKET_SIZE);\
if(stusername) s_strncpy_s(cmdname->szName,stusername,_MAX_NAME_LEN_ -1);\
cmdname->msg.push_back((char*)subrt,sublen,__FUNC_LINE__);\
CUserEngine::getMe().SendMsg2SuperSvr(cmdname,sizeof(*cmdname)+cmdname->msg.getarraysize());\

#define SENDMSG2SUPERBYONLYID(cmd,stuseronlyid,subrt,sublen)\
BUFFER_CMD(cmd,cmdname,stBasePacket::MAX_PACKET_SIZE);\
if(stuseronlyid) cmdname->i64SrcOnlyId=stuseronlyid;\
cmdname->msg.push_back((char*)subrt,sublen,__FUNC_LINE__);\
CUserEngine::getMe().SendMsg2SuperSvr(cmdname,sizeof(*cmdname)+cmdname->msg.getarraysize());\

#define SENDMSG2BLOBALBYONLYID(cmd,stuseronlyid,subrt,sublen)\
BUFFER_CMD(cmd,cmdname,stBasePacket::MAX_PACKET_SIZE);\
if(stuseronlyid) cmdname->i64SrcOnlyId=stuseronlyid;\
cmdname->msg.push_back((char*)subrt,sublen,__FUNC_LINE__);\
CUserEngine::getMe().SendMsg2GlobalSvr(cmdname,sizeof(*cmdname)+cmdname->msg.getarraysize());\

#pragma pack(push,1)


#define SUBCMD_GETABILITY						175	//获取目标玩家属性
struct stCretGetAbility:public stClientGameSvrCmd<SUBCMD_GETABILITY>{
	DWORD dwTargetTmpId;
	stARpgAbility TargetAbility;
	stCretGetAbility(){
		ZEROCMD;
	}
};

#pragma pack(pop)

enum ReadyState{
	_READYSTATE_NONE_=0,
	_READYSTATE_SVR_READY_,
	_READYSTATE_ISALL_READY_,
	PLAYER_READY_ONLINE,
	PLAYER_READY_OFFLINE,
};

enum emPkModel{
	PKMODEL_PEACEMODE,			//和平模式：只对怪物进行攻击起效
	PKMODEL_TEAMMODE,			//队伍模式：对怪物以及非本队伍的玩家进行攻击起效
	PKMODEL_GUILDMODE,			//行会模式：对怪物以及非本行会的玩家进行攻击起效  （现为军团模式）
	PKMODEL_GUILDWARMODE,		//宣战模式：对怪物以及非本行会的行会战玩家进行攻击起效
	PKMODEL_NULL,
	PKMODEL_GOODANDEVILMODE,	//善恶模式：对怪物以及无保护状态的玩家进行攻击起效（灰名，黄名，红名玩家）
	PKMODEL_ALLTHEMODE,			//全体模式：对所有怪物和玩家进行攻击起效
};

enum emDieToRelive{
	RELIVE_MAPHOME,				//回城复活
	RELIVE_INSITU,				//原地复活
	RELIVE_REJUVENATION,		//回春复活
	RELIVE_ROSE,				//玫瑰复活
};

class compare_LimitItem{
public:
	bool operator()( const CItem *const firstArg, const CItem * const secArg)const
	{
		bool bResult= false;
		if( firstArg->m_Item.dwExpireTime > secArg->m_Item.dwExpireTime )
		{
			bResult = true;
		}
		else if(firstArg->m_Item.dwExpireTime== secArg->m_Item.dwExpireTime)//时间相同则根据唯一ID排序
		{
			return (firstArg->m_Item.i64ItemID > secArg->m_Item.i64ItemID);
		}
		
		return bResult ;
	}
};

struct stPlayerPlug{
	ULONGLONG dwPlugDataTick;
	DWORD dwPlugNum;
	ULONGLONG dwAllPlugDataTick;
	DWORD dwAllPlugNum;
	bool boImprisonment;
	stPlayerPlug(){
		ZEROSELF;
		dwPlugDataTick=GetTickCount64();
		dwAllPlugDataTick=GetTickCount64();
	}
	void Reset(){
		ZEROSELF;
		dwPlugDataTick=GetTickCount64();
		dwAllPlugDataTick=GetTickCount64();
	}
	void ResetPlug(){
		dwPlugDataTick=GetTickCount64();
		dwPlugNum=0;
	}
	void ResetAllPlug(){
		dwAllPlugDataTick=GetTickCount64();
		dwAllPlugNum=0;
	}
};

enum emKickState{
	NONE_KICK=0,
	KICK_RELOGIN=1,				//回城复活
	KICK_SECONDPASSWORDERROR=2,				//回城复活

	KICK_RENAME=10,				//原地复活

	KICK_CHANGEGAMESVROK=100,				//切服
};
struct stLuaEffCof
{
	DWORD effectid;
	float cof;
	stLuaEffCof()
	{
		ZEROSELF;
	}
};

struct stTiShi {
	int64_t updateTime = 0;
	BYTE mapchange = 0;
	std::string lastTishiMsg;
	inline void SetLastTishi(const std::string& str) {
		lastTishiMsg = str;
	}
	inline const std::string GetLastTishi() {
		return lastTishiMsg;
	}
};
class TiShiManager {
	CPlayerObj* m_owner;
	std::map<int, std::unordered_map<std::string, stTiShi>> m_mapTips;
	std::unordered_map<std::string, int64_t> m_mapCallBack;

public:
	TiShiManager(CPlayerObj* owner);
	void Run();
	void AddCheckList(int id, std::string path, uint32_t delaytime);
	void MapChange();
	void AddMapChangeList(int id, std::string path);
	void AddLuaCallBack(std::string strFuncStr, int32_t delaytime);
};
struct stTuBiao {
	int64_t updateTime;
	uint8_t state;
	stTuBiao() {
		ZEROSELF;
	}
};
class TuBiaoManager {
	CPlayerObj* m_owner;
	std::unordered_map<uint16_t, stTuBiao> m_mapTuBiao;
public:
	TuBiaoManager(CPlayerObj* owner);
	void Run();
	void AddCheckList(uint16_t tubiao_id, uint32_t delaytime);
	void KeepStorage(uint16_t tubiao_id, uint8_t state);
	uint8_t GetShowState(uint16_t icon_id);
	bool NeedUpdate(uint16_t icon_id);
};
struct stLoadBuffData
{
	int64_t timeLeft;
	uint32_t buffid;
	uint32_t nDura;
	uint8_t level;
	stLoadBuffData(int id, int lv, int64_t left, uint32_t dura)
	{
		buffid = id;
		level = lv;
		timeLeft = left;
		nDura = dura;
	}
};

class CPlayerObj final :public CCreature{
protected:
	bool Operate(stBaseCmd* pcmd,int ncmdlen,stProcessMessage* pMsg) override;	//1.6	处理消息对列,包括服务器端和客户端消息
public:
	CPlayerObj(const CPlayerObj& other) = delete;
	CPlayerObj(CPlayerObj&& other) noexcept = delete;
	CPlayerObj& operator=(const CPlayerObj& other) = delete;
	CPlayerObj& operator=(CPlayerObj&& other) noexcept = delete;

	CPlayerObj(PosType x, PosType y, CGameGateWayUser* pGateUser, char* szName);
	~CPlayerObj() override;
	bool HasTriggerRegions() const override { return true; }
	bool isSwitchSvr() override;
	bool LocalMapTransfer(PosType x, PosType y);
	bool TransferToMapId(uint16_t mapId, uint16_t cloneId, PosType x, PosType y);
	bool TransferToMap(CGameMap* targetMap, PosType x, PosType y);
	bool MoveToMap(uint16_t mapId, uint16_t cloneId, PosType x, PosType y, uint16_t svrId);
	bool TransferMap(CGameMap* targetMap, PosType x, PosType y);
	bool CrossServerTransfer(uint16_t mapId, uint16_t cloneId, PosType x, PosType y, uint16_t svrId);
	bool ProcessUserMessage(stBaseCmd* pcmd, int ncmdlen, stQueueMsgParam* bufferparam);	//将客户端发送的消息加入到处理队列
	void InitTimer();
	void OnUserLoginReady();
	void UpdateToSuperSvr();
	void UpdateToGlobalSvr();
	void UpdateMemberName(const char* szNewName);
	void fullToSuperSvrPlayerBaseData(stUpdateSupersvrPlayerBaseData& data);
	void getCharHumanInfo(char* szHumanInfo);
	bool xfxDecode(int nCheckNum, const char* pszXfxMsg);
	void Update() override;
	bool CalculatingSpeed() override;	//计算速度
	void player_run();

	void MakeGhost(bool delaydispose, const char* ff) override;	//改角色已经无效,进入等待删除状态,无法使用

	void Disappear() override;
	void OnPlayerOffLine();
	void OnPlayerReOnLine();
	void OnPlayerBeforeChangeGameSvr();
	void OnPlayerBeforeChangeSvrSaveData();
	void OnPlayerChangeSvrSuccess();
	void SendRefMsg(void* pcmd, int ncmdlen, bool exceptme = false) override;
	void SendMsgToMe(void* pcmd, int ncmdlen, int zliblvl = Z_DEFAULT_COMPRESSION) override;
	void EnterMapNotify(MapObject* obj) override;

	void ChangeClientLoadMapData(CGameMap* map);
	void MapChanged() override;
	void AddLoadedBuff();
	double GetHpPer() { return (double)m_nNowHP / (double)m_stAbility[AttrID::MaxHP]; } // 当前血量占比
	static bool OnstCretMove(CPlayerObj* player, stCretMove* pcmd, unsigned int ncmdlen, stProcessMessage* param);	//处理客户端发来的走动消息
	static bool OnstCretAttack(CPlayerObj* player, stCretAttack* pcmd, unsigned int ncmdlen, stProcessMessage* param); //处理客户端发来的攻击消息
	static bool OnstCretChat(CPlayerObj* player, stCretChat* pcmd, unsigned int ncmdlen, stProcessMessage* param, bool boStallBack); //处理客户端发来的聊天消息的消息
	static bool OnstCretUseItem(CPlayerObj* player, stCretGetUseItem* pcmd, unsigned int ncmdlen, stProcessMessage* param);	//处理客户端发来的物品使用的消息
	bool doCretItemCmd(stBaseCmd* pcmd, int ncmdlen);	//处理物品消息
	bool doCretTradeCmd(stBaseCmd* pcmd, int ncmdlen);	//处理交易消息
	bool doCretQuestCmd(stBaseCmd* pcmd, int ncmdlen);		//处理任务消息
	bool doCretGroupCmd(stBaseCmd* pcmd, int ncmdlen);	//处理组队消息
	bool doCretGroupFromSuperSvr(stBaseCmd* pcmd, int ncmdlen);//处理从superserver返回的组队消息
	bool doShortCutsCmd(stBaseCmd* pcmd, int ncmdlen);	//处理快捷键消息
	bool doCretMailCmd(stBaseCmd* pcmd, int ncmdlen);	//处理邮件消息
	bool doMailFromSuperSvr(stBaseCmd* pcmd, int ncmdlen); //处理从superserver返回的邮件消息
	bool doConsignCmd(stBaseCmd* pcmd, int ncmdlen);     //寄售消息
	bool doConsignFromSuperSvr(stBaseCmd* pcmd, int ncmdlen);//处理从superserver返回的寄售消息
	bool doStallCmdFromSupSvr(stBaseCmd* pcmd, int ncmdlen);//处理从superserver返回的 随身摊位
	bool doRelationCmd(stBaseCmd* pcmd, int ncmdlen);		//好友关系消息
	bool doRelationFromSuperSvr(stBaseCmd* pcmd, int ncmdlen);//处理从superserver返回的好友关系消息
	void GetBaseProperty() override;								//从基本属性数据库加载基本属性
	int CalculatingRestoreHp() override;	//计算回血
	int CalculatingRestoreMp() override;	//计算回蓝
	int CalculatingRestorePp() override;	//计算体力
	void ChangeProperty(bool bosend = true, const char* ff = "") override;
	void DoChangeProperty(stARpgAbility& abi, bool boNotif, const char* ff) override;
	bool MoveTo(int nDir, int nmovesetp, int ncx, int ncy, bool boNotCheckObj) override;
	///////////////////////////////////////
	void SetAbilityFlag(emAbilityFlag type = ABILITY_FLAG_ALL);
	void SetSpecialAbilityFlag(emSpecialAbilityFlag type = SPECIALABILITY_FLAG_ALL);
	void BuildPlayerFeature(stPlayerFeature& feature);
	void fullCretSimpleAbility(stSimpleAbility* psa);
	const char* getShowName(char* szbuffer = NULL, int nmaxlen = 0) override;		//获得显示用的名字
	int GetSexType() const { return (int)this->m_siFeature.sex; }					//获取玩家性别类型
	emJobType GetJobType() { return (emJobType)this->m_siFeature.job; }		//获取玩家职业类型
	const char* getAccount() const { if (m_pGateUser) return m_pGateUser->m_szAccount; else return NULL; };
	const char* getClientIp() const { if (m_pGateUser) return m_pGateUser->szclientip; else return nullptr; };
	const char* getGateIp() const { if (m_pGateUser) return m_pGateUser->szgateip; else return nullptr; };
	const char* getSubPlatform() const { if (m_pGateUser) return m_pGateUser->m_szTxSubPlatformName; else return " "; };
	const char* getMeshineid() const { if (m_pGateUser) return m_pGateUser->m_szMeshineid; else return " "; };
	const char* getClientBundleId() const { if (m_pGateUser) return m_pGateUser->m_szClientBundleId; else return ""; }
	const char* getClientPlatform() const { if (m_pGateUser) return m_pGateUser->m_szClientPlatform; else return ""; }
	const char* getClientVersion() const { if (m_pGateUser) return m_pGateUser->m_szClientVersion; else return ""; }

	bool LoadMap(stLoadPlayerData* pgamedata);
	bool LoadHumanBaseData(stLoadPlayerData* pgamedata);							//加载人物数据并且计算属性，一些基础属性 等级地图之类的
	bool LoadHumanData(stLoadPlayerData* pgamedata);								//复杂属性，如果角色登陆不成功，这些东西就不用解析了  装备 宠物 任务之类复杂数据结构
	bool checkFirstLvAtt(CItem* pItem);

	int fullPlayerSaveData(stSavePlayerDataCmd* pcmdbuf, int nmaxlen, int saveType, int nCrossTradeId = 0);		//填充存档数据
	int fullPlayerChangeSvrData(stSavePlayerDataCmd* pcmdbuf, int nmaxlen, int saveType);	//填充不存档数据（跨服需要时）
	bool HandleMoveCross();
	void CrossSvrSavePlayerData(BYTE btSaveType);

	bool Die() override;								//玩家死亡
	void SaveData();
	void NotifyMap();
	bool isDataLoaded() const { return m_btReadyState >= _READYSTATE_SVR_READY_; }
	bool ClientIsReady() const { return m_btReadyState >= _READYSTATE_ISALL_READY_; }
	bool IsOnline() const { return m_btReadyState == PLAYER_READY_ONLINE; }
	bool IsOffline() const { return m_btReadyState == PLAYER_READY_OFFLINE; }
	bool isClientReady() override;
	bool CanWalk() override;
	void SendClientSet(bool boisnewhuman); //发送客户端设置
	bool isGuildWar();
	bool isEnemy(CCreature* pTarget = NULL) override;							//True为敌人
	bool DamageSpell(int64_t nDamage) override;

	bool PlayerAttack(int nX, int nY, DWORD dwTemId, DWORD dwMagicID);
	bool OnCretStruck(stCretStruckFull* pcmd, unsigned int ncmdlen) override; //处理被攻击的消息
	bool OnCretAction(stCretActionRet* pcmd, unsigned int ncmdlen, stProcessMessage* param) override; //处理施法动作的消息
	void ChangePetTarget(CCreature* pTarget = NULL);		//改变宠物攻击目标
	void SetRmbGoldLog(int nChanged, int nOld, const char* szEvtName);					///大米改变记录
	bool GoldChanged(double nChanged, bool boAutiAddicted = false, const char* szopt = NULL);							//金币改变,并通知客户端
	bool ResChange(ResID rid, int nChange, const char* szEvtName);							// 资源更改
	bool CanGetMonReward(CMonster* Monster = NULL);                               // 击杀怪物是否可以获得奖励
	bool PkChange(int nChanged);															//PK值改变
	void SendPlayerProperty();								//上线的时候发送一次人物属性
	stShortCuts* GetShortCuts(DWORD dwId);
	bool AddShortCuts(DWORD dwId, emShortCutsType emType, BYTE btValue, BYTE btRaw, BYTE btCol);
	void GroupInfoChanged();								//人物信息发送到组队
	void SendPosToGroupMember();												//发送人物当前位置给队伍成员
	void SendAllPosToGroupMember();												//发送人物位置地图给队伍成员
	void SendPosToRelationMember();												//发送人物地图变更给好友
	void UpdateSimpleFeatureToGlobalSvr();										//更新人物外观到global服务器
	void GroupMonGetReward(CMonster* pMonster, DWORD dwLevel, CGameMap* pMap, int nX, int nY, int nRange = 32, bool selfGet = true) const;
	void ServerGetLeaveGroup();
	void RankTopToSuper(emRankType RankType);									//排行榜所需数据发送到SUPERSVR
	//////////////////////////////////////////////////////////////////////////
	int EquipIsOk(DWORD dwBaseid);						//检查人物身上是否装备了这个物品
	int PacketItemIsOk(DWORD dwBaseid, DWORD num);		//检查人物包裹中是否有这个物品，和物品数量是否满足
	int PacketItemIsOkWithBindSta(DWORD dwBaseid, DWORD num,BYTE btBind);		//检查人物包裹中是否有这个物品，和物品数量是否满足(绑定或非绑定)
	int GroupIsOk();									//检查是否组队
	int ItemDeleteByBaseIdIsOk(DWORD dwBaseid, DWORD num, const char* pszLogStr);		//删除人物身上物品，名字，数量
	int ItemDeleteIDIsOk(const char* luatmpid, DWORD num, const char* pszLogStr);//删除人物身上的任务物品
	int ItemDeleteInBag(CItem* pItem, DWORD nCount, const char* pszLogStr);		//删除包裹中的物品  如果nCount= -1 则按物品数据的数量操作
	int ItemDeleteInBody(CItem* pItem, const char* pszLogStr);		//删除身上该物品

	int getTotalTargetPlayerNum(std::vector<CCreature*> vTarget);// 获取被攻击玩家数
	int getTotalTargetNum(std::vector<CCreature*> vTarget);		// 获取被攻击目标总数
	int ItemDeleteInAll(CItem* pItem, DWORD nCount, const char* pszLogStr);		//删除玩家所有位置的这个物品 如果nCount= -1 则按物品数据的数量操作
	DWORD GetVisitNpcId();								//得到当前访问NPC的ID
	CCreature* GetVisitNpc();								//得到当前访问NPC
	const char* GetLuaItemName(DWORD itembaseid);		//得到物品名字
	int  CheckLattice(DWORD dwBaseid, DWORD num);		//得到包裹空格
	CItem* GetCurrItem(const char* luatmpid);			//得到最后一次使用的物品
	CItem* GetEquipItem(const char* luatmpid);			//根据唯一ID得到装备物品
	CItem* CreateLuaItem(DWORD dwBaseid, DWORD num, int frommapid = 0, const char* bornfrom = "", const char* szmaker = "");		//创建脚本所需物品
	int  SendToBag(CItem* item, bool boLog, const char* pszLogstr = NULL);	//发送物品到包裹
	int  SendToBagNoDel(CItem* item, bool boLog, const char* pszLogstr = NULL);		//发送物品到包裹失败不删除
	void GetGroupPlayer(const char* szFunc);			//得到该组队位置的玩家
	CItem* GetPosEquip(int pos);						//得到该位置的物品指针
	void  SendItem(CItem* pItem);						//刷新物品
	bool SetVisitNpc(CCreature* pNpc);					//设置访问NPC
	bool WearItem(CItem* pItem, int nPos);				//佩戴装备
	bool TakeOffItem(CItem* pItem, int nPos);			//摘下装备
	bool ChangeName(const char* szNewName, const char* szOptMode);				//改变名字
	bool ChangeNameFinal(const char* szOldName);				//改变名字之后
	void AddRelation(int nType, const char* szName);		//添加好友关系

	void SendDropItem(CItem* pItem, int nX, int nY);      //发送自己打出来的物品信息
	bool GetSecondOK();									//得到二级验证
	void SetSecondOK(bool boSet);						//设置二级验证
	void SendItemToClient(CItem* pItem);				//发送物品数据给客户端
	DWORD GetFreeCellCount();							//获得包裹剩下的所有空格
	bool CheckVisitNpc();								//检查访问NPC是否在范围内
	//void GetCurLevelSkill();							//学习当前等级技能
	bool GuildWarKill(DWORD dwGuildId, int nKill, int nDie);//公会战杀人
	bool GuildWarCheck(CPlayerObj* pTarget);			//公会战检查,目标是杀人者
	bool GuildAllianceCheck(CPlayerObj* pTarget);
	void ChangeSex(int nSex);							//改变性别
	void ChangeJob(int nJob);							//改变职业
	int GetRelationSize(BYTE btRelationType);			//获得关系列表中的数量
	void KillMonster(CMonster* pMonster);
	void StudySkill(DWORD dwSkillid, BYTE btLevel, bool bCost = true);
	bool DeleteSkill(DWORD dwSkillid);
	//////////////////////////////////////////////////////////////////////////
	bool SaveClientSet(char* dest, DWORD& retlen);
	bool LoadClientSet(const char* dest, int retlen, int nver);
	void PlayDeathDropped();		//人物死亡掉落
	static bool sendTipMsg(CPlayerObj* player, const char* pszMsg);
	static bool sendTipMsgByXml(CPlayerObj* player, const char* pszPat, ...);
	static bool sendTipMsgByXml(int64_t i64OnlyId, const char* pszPat, ...);
	static void sendLuaMsg(CPlayerObj* pDstPlayer, DWORD dwDataType, const char* szData);
	sol::table LuaGetMapOtherGroupMember(sol::this_state ts);	//得到同一地图内的队伍玩家
	void getMsgFromTxServer(const char* pszFunName, bool boForceRefresh);					//从腾迅服务器获取vip信息
	//////////////////////////////////////////////////////////////////////////
	void ChangePetProperty();
	CPetObj* GetMagicPet() { return m_Petm.m_pMagicPet; };
	DWORD getVipType();
	int getVipLevel();
	void LoopLimitItemPacket();
	bool AddItemLimitTime(CItem* pItem, DWORD dwLimitTime);//dwLimitTime 秒
	void LuaSetPetState(emPetState emState);
	void LuaLoadAllPets(sol::table table);
	char* GetCrossTradeAccount();
	char* GetCrossTradeName();
	BYTE getPlatFormType();

	void PushDelayAttackMsg(stCretAttack* param);
	void PushDelayMoveMsg(stCretMove* param);
	void ClearDelayMsg(int flag = 0);
	bool DealDelayMsg(int flag);
	void ChangePkMode(DWORD dwPkMode, bool boServer = false);
	BYTE GetPkMode();
	void addLuaEffid(DWORD dwEffid);
	void addSpecialLuaEffid(DWORD dwEffid);
	void addCofLuaEffid(DWORD dwEffid, float cof);
	void calculateLuaAbility(stARpgAbility* abi);
	void calculateLuaTimeAbility(stARpgAbility* abi);	//计算限时属性
	void calculateAttrPointAbility(stARpgAbility* abi);
	void FirstConvertAbility(stARpgAbility* abi, stARpgAbility* converabi);
	void UpdatePlayerInfo();
	void SendSpecialRingSkillCd(stMagic* pMagic);
	int  CheckPosCollide(int ndx, int ndy, int& count); //检测当前格子是否能被野蛮冲撞推走
	int  CheckPlayerCollide(CCreature* pCret); //检测CCreature* pCret是否能被野蛮冲撞推走	
	stItem* GetMailedItem(DWORD id, DWORD num, BYTE binding = 1);
	float GetExpRate(int nGroupNum = 0);//获取玩家经验倍率
	void KickPlayer(BYTE cmd, BYTE subcmd);						//踢人
	void DropTest(DWORD dwNum, DWORD dwMonsterId);
	void SendWeChatInfo(int ptid);
	void SendGroupChangeInfo(BYTE ntype, int guildid, const char* guildname);
	void PlayerUnderBattle();			// 玩家战斗或者受击(脱战)
	void PlayerUnderPk();				// 玩家进行pvp
	void SetReadyState(ReadyState state, const char* log);
	void DealDelayList();
	void ChangeDayLoginLog();
	bool IsEnoughAttackCostNeng(stCretAttack* pcmd);
	bool DoAttackCostNeng();
	int CalcMonDamage(CMonster* pMon);
	int getMiningNeng();
	int GetDrugShareCd();
	bool UpdateChargeNoChange(int nChanged, const char* szEvtName);
	bool IsFighting() { return time(NULL) < m_i64BattleSec; };
	bool IsPking() { return time(NULL) < m_i64PkSec; };
	int DoPunctureMagic(BYTE damageType);									 // 穿刺技能
	int DoExcitedMagic(int attackValue = 0);					 // 兴奋技能
	int DoContinuousFiringMagic(int attackValue = 0);	     // 连射技能
	int DoShanBiMagic();									 // 闪避技能
	bool DoCounterSkill(CCreature* target = NULL);  		 //反击技能 
	void ClearFighting() { m_i64BattleSec = time(NULL); };
	void ClearPking() { m_i64PkSec = time(NULL); };
	int GetGuardFeature();
	void SetHeadPortrait(int HeadPortrait) { m_dwHeadPortrait = HeadPortrait; };
public://lua方法
	void LuaChangeProperty(BYTE emType);
	bool luaPlayerAttack(int nX, int nY, DWORD dwMagicID);
	bool LuaLevelUp(int lv = 1);
	void LuaSetSecondPass(const char* szPass);			//设置二级密码
	void LuaSetSecondMailAddress(const char* szMail);	//设置二级邮箱
	void LuaSendBuffState2Client(DWORD dwBuffID, BYTE btLevel = 1);
	void LuaSendBuffAllState2Client(DWORD dwBuffID, BYTE btState = 1, int nTime = 0);
	void LuaCallLuaByFuncName(const char* evtfunc);
	bool LuaSendTipMsg(const char* pszMsg);
private:
	bool AddExp(int& nNew, const char* szopt = NULL);								//经验改变
	void LevelUp(int newLv);													//升级了
	bool UpdateCharge(int nChange, const char* szEvtName, DWORD& dwtransid);
	void SetCcyLog(ResID rid, const char* szEventName, int nChange, int nOld, DWORD dwtransid = 0);
public:
	BYTE m_btSelUserState;				//用户状态(正常 暂离 退出游戏 stUserReSetState )
	bool m_boIsSelOk;
	time_t m_addloadoktime;
	CGameGateWayUser* m_pGateUser;	//网关代理
	char	szclientip[24];			//客户端IP
	stRes m_res;						// 资源
	int64_t m_i64UserOnlyID;				//唯一角色ID
	char m_szLegion[_MAX_NAME_LEN_];		//军团
	int64_t m_i64BattleSec;				//脱离战斗的时间
	int64_t m_i64RunSec;				//脱离跑步的时间
	int64_t m_i64PkSec;					//脱离玩家打玩家的时间
	// 数据配置的人物数据
	int m_nAttackInterval;	//攻击间隔
	int m_nReleaseInterval;	//释放间隔
	int m_nMoveInterval;		//移动间隔
	int m_nPpMoveCost;		//移动消耗体力
	int m_nKillMaxMonLv;		//击杀怪物最大等级
	int m_nKillMinMonLv;		//击杀怪物最小等级

	stLoadPlayerData* m_pTmpGamedata;
	stSvrMapId m_changeMapId;
	time_t m_dwWaitChangeSvrSaveDataTime;
	bool m_boWaitChangeSvrSaveData;
	bool m_boIsWaitChangeSvr;
	int m_nWaitChangeSvr_newsvr;//=pSpaveMoveInfo->DstMap->getSvrIdType();
	int m_nWaitChangeSvr_oldsvr;//=pUser->GetEnvir()->getSvrIdType();
	in_addr m_nWaitChangeSvr_ip;
	int m_nWaitChangeSvr_port;
	int m_nWaitChangeSvr_mapid;
	char m_szTGWip[_MAX_PATH];
	int m_nWaitChangeSvr_mapsublineid;
	DWORD m_dwChangeZoneid;
	DWORD m_dwSrcZoneId;	//跨服后保存来源服务器id,用于切回来源服务器
	DWORD m_dwSrcTrueZoneId;	//跨服后保存区id,用于切回来源服务器
	DWORD m_dwSrcGameSvrIdType;
	DWORD m_dwChangeTrueZoneid;
	BYTE m_btReadyState;
	bool m_bFakePlayer;
	emKickState m_btKickState;	
	ULONGLONG m_dwDoKickTick;
	DWORD m_dwDoKickTickCount;
	BYTE m_btCrossKickType;
	DWORD m_dwCrossKickZoneId;
	WORD m_wSrcTrade;
	WORD m_wCrossKickTradeid;
	DWORD m_dwBossTmpid;					//存归属自己的boss的tmpid
	bool m_boIsChangeSvr;					//是否是跨服务器费过来的
	BYTE m_btChangeSvrSpaceMoveType;		//跨服飞的类型
	bool m_boIsNewHum;
	BYTE m_btGmLvl;
	stARpgAbility m_stEquipAbility;	        //装备的基本属性
	stARpgAbility m_stLuaAbility;	        //脚本的基本属性
	stARpgAbility m_stLuaTimeAbility;	    //脚本的限时属性
	stARpgAbility m_stAttrPointAbility;		//属性点的属性
	stARpgAbility m_stPassiveSkillAbility;	//被动技能的属性
	stARpgAbility m_stGmAbility;			//GM修改的属性

	stSpecialAbility  m_stSpecialAbility;			 //特殊属性
	stSpecialAbility  m_stSpecialEquipAbility;		 //装备特殊属性
	stSpecialAbility  m_stSpecialLuaAbility;		 //lua特殊属性
	stSpecialAbility  m_stSpecialBuffAbility;		 //buff特殊属性

	char m_szUserId[_MAX_NAME_LEN_];//SDK 用户ID
	char m_szAppId[_MAX_NAME_LEN_];//客户端APPID
	time_t m_dwNextSaveRcdTime;   //下次保存数据的时间
	DWORD  m_dwSaveCount;			//累计存档次数
	DWORD  m_dwAccountDataSaveCount;
	time_t m_dwCreateTime;	//对象创建时间
	DWORD m_dwLoginTime;          //登陆时间
	DWORD m_dwLineOutTime;		//上次下线时间
	DWORD m_dwPlayerCreateTime;		//角色创建时间
	DWORD m_tLoginOuttime ;		//人物离线时间
	DWORD m_dwPlayerOnlineTime;		//角色在线时间
	time_t m_dwAccountOnlineTime;		//账号在线时间
	time_t m_dwLastfullSaveRefTime;
	ULONGLONG m_dwVisitNpcIntervalTick;
	uint32_t  m_abilityFlag = 0;
	uint32_t  m_specialAbiFlag = 0;
	ULONGLONG m_dwSaveMoveTimeBefore;	//上次赋值时间
	ULONGLONG m_dwSaveMoveTimeForNext;//最后一次移动 攻击的的时间
	std::list<stCretMove*> m_cDelayMoveList; //延迟移动 攻击队列
	ULONGLONG m_dwSaveAttackTimeForNext;				// 下次物理技能时间
	ULONGLONG m_dwSaveReleaseTimeForNext;				// 下次魔法技能时间
	std::list<stCretAttack*> m_cDelayAttackList;		
	std::list<int> m_cDelayDealList;					// 处理列表
	bool m_boCanSendFeature;
	std::vector<stTradeList> m_waittradelist; // 等待交易队列
	BYTE m_nMoveIv;
	DWORD m_dwChatModel;					//客户端聊天过滤限制 emUserChatFilter
	DWORD m_dwUserConfig;					//玩家功能设置
	BYTE m_btPkModel;						//PK模式
	DWORD m_dwShutDownCheckTime;
	DWORD m_dwGold;							//金币 （还没用到）
	DWORD m_dwLastChargeCcyTime;			//最后一次充值时间
	bool m_boAccountDataLoadOk;
	int64_t m_i64MaxExp;					// 升级需要经验
	ULONGLONG m_ChangeRmbTick;
	stSimpleFeature m_siFeature;
	stSimpleAbility m_siAbility;
	CCreature		*m_pVisitNPC;	//当前访问的npc
	bool m_boCanChangeName;
	char m_szOldName[_MAX_OLDNAME_];
	in_addr	clientip;
	DWORD   m_dwLastOperateTime;            //上次操作时间
	DWORD	m_dwLastChatTime[15];			//上次说话时间
	DWORD	m_dwChatCount[15];				//说话次数
	int     m_nVipLevel;
	int     m_nAddStorePage;				//仓库扩展页数
	DWORD m_dwBanChatTime;		//禁言时间
	DWORD m_dwBanPlayerTime;	//封号时间
	CPlayerPackage		m_Packet;		//包裹及仓库
	CTrade				m_Trade;		//交易
	CShortCutsManager	m_ShortCut;		//快捷键
	TiShiManager		m_tiShi;		//红点提示
	TuBiaoManager		m_tuBiao;		//红点提示
	stGSGroupInfo		m_GroupInfo;	//组队信息
	LimitHash<int64_t, stGMember> m_cGroupMemberHash; //组队成员的id
	stGSGuildInfo		m_GuildInfo;	//行会信息
	CPetManage			m_Petm;			//宠物管理器
	std::vector<stLoadBuffData> m_loadBuffData;
	CQuestCompleteMark m_QuestCompleteMark;//任务完成标记
	bool  m_boMapEvt;			//玩家触发地图范围事件只能一次
	CVars m_dayVars;
	CItem* m_pCurrItem;			//当前玩家使用的物品任务用
	int64_t m_i64CurrItemTmpId;	//当前玩家使用的物品临时ID任务用
	std::vector<BYTE> m_vClientSet;	//客户端设置
	stPlayerPlug m_stPlug;			//玩家外挂检查
	//////////////////////////////////////////////////////////////////////////
	DWORD m_dwDoubleTime;		//记录双倍时间
	DWORD m_dwDoubleRate;		//记录倍数
	//////////////////////////////////////////////////////////////////////////
	bool m_boTmpGm;				//临时GM标记
	DWORD m_dwTmpGmTime;		//临时GM时间
	//////////////////////////////////////////////////////////////////////////
	DWORD m_dwSelLineTime;		//允许切线时间
	//////////////////////////////////////////////////////////////////////////
	time_t m_tAutoKeepHpTime;	//自动回血时间
	bool m_boAutoOpenHp;		//自动回血是否开启
	DWORD m_dwKeepHp;			//正常补血数据
	DWORD m_dwForceKeepHp;		//强制补血数据
	DWORD m_dwHpSet;			//喝药设置
	//////////////////////////////////////////////////////////////////////////
	CRelationList m_xFriendList;//好友列表
	CRelationList m_xBlockList;	//黑名单列表
	CRelationList m_xEnemyList;	//仇人列表
	//////////////////////////////////////////////////////////////////////////
	bool m_boSecondPassOk;								//二级密码正确
	char m_szSecondPassWord[_MAX_SECONDPASS_LEN_];		//二级密码
	char m_szSecondMailAddress[_MAX_SECONDPASS_LEN_];	//二级密码邮箱
	BYTE m_btLeader;			//引导员等级
	time_t m_tLeaderPeriodTime;	//引导员有效时间
	bool m_boSendPlayerInfo;	//标记是否发送玩家信息到SUPER
	int m_nPlayerMin;			//每分钟
	int m_nPlayerHour;			//每小时
	int m_nPlayerDay;			//每天
	int m_nPlayerWeek;			//每周
	BYTE m_btDayTradeCnt;	   //每天交易次数
	std::set<DWORD> m_WarGuildSet;	//战斗的行会
	std::set<DWORD> m_AllianceGuildSet;//联盟行会
	std::set<CItem*,compare_LimitItem> set_LimitItemPacket;		//限时物品 <ptim>  
	DWORD m_dwOriginalZoneid;			//原始区id
	emClientType m_emClientVer;
	bool  m_isOpenPacketLog;			//发包记录
	DWORD m_dwJyRate;					//经验万分比
	DWORD m_dwHeadPortrait;				//玩家头像 存档
	int m_nOriginalZone;				//存档
	DWORD m_dwUseRmb;					//存档
	std::vector<DWORD> m_vLuaEffid;			//lua 脚本人物属性
	std::vector<DWORD> m_vSpecialLuaEffid;	//lua 脚本人物属性
	std::vector<stLuaEffCof> m_vCofLuaEffid;	//lua 脚本人物属性
	stSimpleCretAbility m_stSimpleAbilityKey;
	bool m_isRun;			      //是否跑步状态
	uint8_t m_transferState;
};

struct stLimitHashPlayerAccount:LimitStrCaseHash< CPlayerObj*> {
	static __inline const char* mhkey(CPlayerObj*& e){
		return e->m_pGateUser->m_szAccount;
	}
};

struct stLimitHashPlayerName:LimitStrCaseHash< CPlayerObj*> {
	static __inline const char* mhkey(CPlayerObj*& e){
		return e->getName();
	}
};

struct stLimitHashPlayerOnlyId:LimitHash< int64_t,CPlayerObj*> {
	static __inline const int64_t mhkey(CPlayerObj*& e){
		return e->m_i64UserOnlyID;
	}
};

class CPlayersHashManager : public zLHashManager6< 
	CPlayerObj*,
	stLimitHashPlayerAccount,
	stLimitHashPlayerName,
	stLimitHashPlayerOnlyId
>{
public:
	CPlayerObj* FindByAccount(const char* account){
		AILOCKT(*this);
		CPlayerObj* value=NULL;
		if (m_e1.find(account,value)){
			return value;
		}
		return NULL;
	}
	CPlayerObj* FindByName(const char* name){
		AILOCKT(*this);
		CPlayerObj* value=NULL;
		if (name && m_e2.find(name,value)){
			return value;
		}
		return NULL;
	}

	CPlayerObj* FindByOnlyId(int64_t i64onlyid){
		AILOCKT(*this);
		CPlayerObj* value=NULL;
		if (m_e3.find(i64onlyid,value)){
			return value;
		}
		return NULL;
	}
};

#endif __GAME_CPLAYEROBJ_H__
