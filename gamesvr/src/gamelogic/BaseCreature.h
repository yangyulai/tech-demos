#pragma once

#include "Buff.h"
#include "qglobal.h"
#include "network/packet.h"
#include "network/sockettask.h"
#include "../gamegatewaySession.h"
#include "GameMap.h"
#include "Magic.h"
#include "quest.h"
#include "Script.h"
#include "Item.h"
#include "MapObject.h"
#include "cmd/Package_cmd.h"
#include "Point.h"

inline static constexpr int LOST_TARGET_INTERVAL = 10 * 1000;
inline static constexpr int APPEAR_SIZE_BITS = 2;
inline static constexpr int APPEAR_INDEX_BITS = 8 - APPEAR_SIZE_BITS;
inline static constexpr int APPEAR_MAX_SIZE = (1 << APPEAR_SIZE_BITS) - 1;   // 3
inline static constexpr int APPEAR_MAX_INDEX = (1 << APPEAR_INDEX_BITS) - 1;  // 63

class CCreature;
class CGameMap;
class CMonster;
class CNpcObj;
class CPetObj;
class CRobot;

struct stProcessMessage{
	ULONGLONG dwDelayTick;
	CCreature* pCret;
	stQueueMsg* pMsg;
	bool bofreebuffer;
	stProcessMessage(){
		dwDelayTick=0;
		pMsg=NULL;
		pCret=NULL;
		bofreebuffer=true;
	}
	void clear(){
		if (pMsg && bofreebuffer){	FreePacketBuffer(pMsg);	}
		dwDelayTick=0;
		pMsg=NULL;
		pCret=NULL;
		bofreebuffer=true;
	}
};

//对象可以看见的集合
inline static const std::array<std::bitset<32>, CRET_COUNT> CreatureVisibilitySettings = []()
	{
		std::array<std::bitset<32>, CRET_COUNT> visibilitySettings;
		for (std::size_t i = CRET_NONE; i < CRET_COUNT; ++i)
		{
			auto& visibilityMask = visibilitySettings[i];
			switch (eCretType cret_type = static_cast<eCretType>(i))
			{
			case CRET_PLAYER:
			{
				visibilityMask.set(CRET_PLAYER, true);
				visibilityMask.set(CRET_NPC, true);
				visibilityMask.set(CRET_MONSTER, true);
				visibilityMask.set(CRET_PET, true);
				visibilityMask.set(CRET_ITEM, true);
				visibilityMask.set(CRET_MAGIC, true);
			}
			break;
			case CRET_MONSTER:
			{
				visibilityMask.set(CRET_PLAYER, true);
				visibilityMask.set(CRET_PET, true);
			}
			break;
			case CRET_PET:
			{
				visibilityMask.set(CRET_PLAYER, true);
				visibilityMask.set(CRET_MONSTER, true);
				visibilityMask.set(CRET_PET, true);
			}
			break;
			case CRET_ITEM:
			case CRET_MAGIC:
			case CRET_NPC:
			case CRET_COUNT:
			case CRET_NONE:
				break;
		default: ;
			}
		}
		return visibilitySettings;
	}();
enum eLifeState{
	NOTDIE=0,			
	ISDIE,			
	ISSKELETON,		
	ISROTSKELETON,

	ISDELAY2GHOST,
};

enum emPkStatus{
	PK_STATUS_RUNTIME,	//PK灰名时间
	PK_STATUS_PKSWORD,	//PK杀戮值
};

#define  CAN_NOT_HIT_TARGET			0x01			//能攻击目标 默认可以攻击
#define  CAN_NOT_MAGIC_TARGET		0x02			//能魔法攻击目标 默认可以被魔法攻击
#define  CAN_NOT_SEL_TARGET			0x04			//能选择目标 默认可以被选中
#define  CAN_VISIT_TARGET			0x80			//能对话的目标 默认不能对话

struct stSpaveMoveInfo{
	enum{
		_SPACEMOVE_NONE_=0,
		_SPACE_WAIT_SWITCHSVR_CHECK_,
		_SPACE_WAIT_SWITCHSVR_,				//
		_SPACE_WAIT_SWITCHZONE_,
		_SPACEMOVE_FINISH_,
	};

	CCreature* pCret;
	CGameMap* DstMap;
	DWORD dwCloneMapId;
	WORD X;
	WORD Y;
	BYTE btMoveType;
	bool boNotCheckObj;
	BYTE btGetRandXY;
	BYTE btMoveState;
	ULONGLONG dwDelayTick;
	ULONGLONG space_move_tick;
	BYTE btMoveType_Svr;
	BYTE btDstNameType;
	char szDstName[_MAX_NAME_LEN_];
	DWORD dwZoneid;
	WORD wTradeid;
	DWORD dwSvrIdx;
	stSpaveMoveInfo(){
		ZEROSELF;
	}
};

#pragma pack(push,1)
struct stCertAbility:public stClientGameSvrCmd< SUBCMD_CERTABILITY >{
	DWORD dwTempId;
	stSimpleCretAbility	Ability;
	BYTE btType;	//0:人物, 1:英雄战士, 2:英雄法师, 3:英雄道士
	int64_t i64FightScore;
	stCertAbility(){
		ZEROCMD
	}
};
#pragma pack(pop)

class CCreature:public MapObject
{
public:
	CCreature(const CCreature& other) = delete;
	CCreature(CCreature&& other) noexcept = delete;
	CCreature& operator=(const CCreature& other) = delete;
	CCreature& operator=(CCreature&& other) noexcept = delete;

	CCreature(std::string_view name, uint8_t type, PosType x, PosType y, DWORD dwTmpId=0);
	~CCreature() override;
	void InitEvent();
	std::MemoryStream m_featureStream;
	template<typename T>
	void UpdateAppearance(FeatureIndex index, T value);
	virtual bool Operate(stBaseCmd* pcmd, int ncmdlen, stProcessMessage* pMsg);	//处理所有消息事件
	virtual bool GetGameMessage(stProcessMessage& ProcessMsg);
	//获得角色所在地图
	CGameMap* GetEnvir() {	return m_pEnvir; }			
	bool SetEnvir(CGameMap* pEnvir);				//设置地图
	void AfterSpaceMove(MapObject* obj);
	void EnterMapNotify(MapObject* obj) override;
	void OnLeaveTargetCheck(MapObject* obj);
	void LeaveMapNotify(MapObject* obj) override;
	void MapMoveNotify(MapObject* obj) override;
	virtual void LoseTarget();
	virtual void SetAttackTarget(CCreature* pTarget, bool ignoreTarget = false);
	bool TryWalk(int wantdir, bool boNotCheckObj=false);
	virtual bool MoveTo(int nDir, int nmovesetp, int ncx, int ncy, bool boNotCheckObj);
	virtual bool CanHit();
	virtual bool CanWalk();
	virtual bool CanRun();
	virtual bool CanMagic();
	template <class CMD>
	static stQueueMsg* CopyQueueMsg(CMD* pcmd,int ncmdlen=sizeof(CMD));
	void pushDelayMsg(CCreature* BaseObject,stQueueMsg* sMsg,int nDelay);
	void pushMsg(CCreature* BaseObject,stQueueMsg* sMsg);
	void pushDelayMsg(CCreature* BaseObject,void* pcmd,int ncmdlen,int nDelay);
	void pushMsg(CCreature* BaseObject,void* pcmd,int ncmdlen);	
	virtual void GetBaseProperty();						//从基本属性数据库加载基本属性

	virtual void ChangeProperty(bool bosend = true,const char* ff="");						//根据基本属性,武器,被动技能计算人物属性,并且发回给客户端
	void ChangePropertyCheckPlayer(emAbilityFlag type = ABILITY_FLAG_ALL);
	virtual void DoChangeProperty(stARpgAbility& abi,bool boNotif,const char* ff);
	void StatusValueChange(BYTE btIndex, int nChange, const char* szLog = nullptr, bool isForce = false);						//状态值更改
	//==========================================================================
	virtual void SendRefMsg(void* pcmd,int ncmdlen,bool exceptme=false);										//广播给可视范围内的角色
	void SendMsgToMe(void* pcmd,int ncmdlen,int zliblvl = Z_DEFAULT_COMPRESSION) override{}
	virtual bool isInViewRange(CCreature* pCret,bool boPrintFalselog =false);																//是否在可视范围之内
	virtual bool isInViewRangeXY(WORD x,WORD y);
	void ProcessMsgList();
	void Update() override;																							//角色提取消息
	void ClearMsg();
	virtual void MakeGhost(bool delaydispose,const char* ff);											//标记对象为可以删除 从列表中删除
	virtual void Disappear();											//消失
	void fullMoveRet(stCretMoveRet* pcmd, BYTE errorcode);				//填充move返回命令
	#pragma region 传送
	bool DoSpaceMove(stSpaveMoveInfo* pSpaceMoveInfo);																					//执行切换地图
	bool CheckMapSpace(stSpaveMoveInfo* pSpaceMoveInfo);
	bool HandleCrossServerMove(stSpaveMoveInfo* pSpaceMoveInfo);
	void FillPreChangeGameSvrCmd(stPreChangeGameSvrCmd* checkcmd, stSpaveMoveInfo* pSpaceMoveInfo);
	bool HandleMoveLocalSameMap(stSpaveMoveInfo* pSpaceMoveInfo);
	bool HandleMoveLocalDiffMap(stSpaveMoveInfo* pSpaceMoveInfo);
	void FillFastTransferPlayerCmd(stFastTransferPlayer* cmd, CPlayerObj* player);
	void FillNotifyPlayerChangeGameSvrCmd(stNotifyPlayerChangeGameSvrCmd* ncc, CPlayerObj* player);
	#pragma endregion
	virtual void MapChanged();				//通知客户端地图改变
	virtual bool isClientReady(){ return true; }
	void FeatureChanged();					//外观改变
	virtual bool NameChanged();						//名字改变
	virtual bool LevelChanged(BYTE btShow=0);					//等级改变
	virtual const char* getShowName(char* szbuffer=NULL,int nmaxlen=0);		//获得显示用的名字
	const char* LuaGetName() { return GTU(getName()); };
	bool isDie() { return (m_LifeState!=NOTDIE);}															//死亡状态
	bool isCanVisit(){ return ((m_btTargetState & CAN_VISIT_TARGET)!=0); }										//是否可以访问
	bool isCanHit(){return ((m_btTargetState & CAN_NOT_HIT_TARGET)==0);}										//是否可以被攻击
	bool isCanMagic(){return ((m_btTargetState & CAN_NOT_MAGIC_TARGET)==0);}									//是否可以被魔法攻击
	bool isCanSelect(){return ((m_btTargetState & CAN_NOT_SEL_TARGET)==0);}									//是否可以被选中
	virtual bool CanBeSee(CCreature* pCret);																			//是否可见
	virtual bool isBoss();       //是否boss
	virtual bool isEliteMon();   //是否精英怪
	virtual bool isLittleMon();  //是否小怪

	void SendReliveMsg();
	virtual bool Die();									//死亡
	virtual bool Relive(int64_t nHp=0, int64_t nMp=0);
	bool LuaRelive(double nHp = 0, double nMp = 0);
	void SendLiftStateChange();
	stSpaveMoveInfo* getSwitchSvrInfo() const { return m_pswitchsvrmoveinfo;}
	void setSwitchSvrInfo(stSpaveMoveInfo* p){ m_pswitchsvrmoveinfo=p;m_switchsvrtime=time(NULL); }

	virtual bool isSwitchSvr();		//是否正在切换服务器
	bool isSwitchSvrTimeOut();

	virtual bool isValidObj();

	virtual bool isSafeZone(CCreature* pTarget=NULL);										//人物是否在安全区
	virtual bool testHomeXY();	

	bool GetFrontPosition(int &nX,int &nY);
	CCreature* GetPoseCreate();

	virtual void StruckDamage(int64_t nDamage, CCreature *pSrcCret = NULL, Byte btDamageSrc = 0);
	virtual bool DamageSpell(int64_t nDamage);
	virtual bool SpellCanUse(int64_t nDamage);
	virtual bool CalculatingPk(CCreature* pCret=NULL,emPkStatus PkStatus=PK_STATUS_PKSWORD);
	virtual	bool CalculatingSpeed();	//计算速度
	virtual int CalculatingRestoreHp();	//计算回血
	virtual int CalculatingRestoreMp();	//计算回蓝
	virtual int CalculatingRestorePp();	//计算体力
	virtual int64_t CalculatingDamage(CCreature* pTarget, const std::shared_ptr<stMagicDataBase>& pMagicData, stInt& TmpDamageType,int& nAggro,int nTargetNum,int nPlayerTargetNum,int nTotalTargetNum);	//计算伤害
	virtual bool CalculatingTarget(std::vector<CCreature*>& vTarget,stMagic* pMagic,DWORD dwTmpId,int nX,int nY);	//计算目标
	virtual bool DoAttack(std::vector<CCreature*> vTarget,stCretAttack* pAttackCmd, stMagic* pMagic, int nDir, DWORD dwPlayTime=0,bool boSendAttack=true);
	virtual bool OnCretStruck(stCretStruckFull* pcmd, unsigned int ncmdlen) { return true; } //处理被攻击的消息
	virtual bool OnCretAction(stCretActionRet* pcmd, unsigned int ncmdlen, stProcessMessage* param); //处理施法动作的消息
	int64_t CalculatingBaseDamage(CCreature* pTarget, const std::shared_ptr<stMagicDataBase>& pMagicData, stInt& TmpDamageType, int nTargetNum, int nPlayerTargetNum, int nTotalTargetNum);	//计算基础伤害
	void ReleaseCretStruckFull(CCreature* Target, const std::shared_ptr<stMagicDataBase>&, int nTargetNum, int nPlayerTargetNum,int nTotalTargetNum,int32_t delay = 0);
	bool isSkillRange(int nX,int nY,BYTE btRange);
	virtual bool isEnemy(CCreature* pTarget=NULL);
	bool isWudi() {return (m_btWudi==1);}
	BYTE isMiaosha() {return m_btMiaosha;}

	void TriggerEvent(CCreature* currentuser,EVTTYPE evtid, DWORD evtnum,WORD wTags=0,WORD mapx=0,WORD mapy=0);				//人物触发事件函数

	virtual bool AddExp(double nAdd, const char* szopt = NULL);
	virtual bool LevelUp(int nAdd = 1, bool bsend=true);
	stMagic* FindSkill(DWORD dwSkillid);				//找到已学习的技能

	#pragma region lua相关
	bool LuaSelfAddBuff(DWORD dwBuffID,BYTE btLevel,double dwKpTime,int dura);
	bool LuaSelfAddBuff2(DWORD dwBuffID, BYTE btLevel, double dwKpTime, int dura, CCreature* pA = nullptr);
	stBuff* LuaFindBuff(DWORD dwBuffID);
	stBuff* LuaGetBuffByType(int nState);
	bool LuaRemoveBuff(DWORD dwBuffID);
	bool LuaRemoveBuffState(int nState);
	void LuaBuffClear();
	bool LocalMapTransfer(int nx,int ny,int nrange,bool boNotCheckObj=true); 
	bool LuaGetBuffState(int nState);
	void LuaFeatureChanged();							//发送外观改变
	void LuaUpdateAppearance(BYTE index, int id);							//发送外观改变
	BYTE GetBattleCamp() const {return m_btBattleCamp;}
	void SendCretStruck(CCreature* pCret,int npower,int ntype);		//发送伤害扣篮包
	CCreature* GetLastHitter() const {return m_pLastHiter;}
	void SetLastHitter(CCreature* pLastHitter);
	double quest_vars_get_var_n(const std::string& name);
	bool quest_vars_set_var_n(const std::string& name, double value,bool needsave);
	char* quest_vars_get_var_s(const std::string& name);
	void SelfCretAction(DWORD dwMagicId,BYTE btlevel);
	void LuaSendRefMsg(char *szMsg,bool boexceptme = false);
	int GetPublicCDTime();
	CPlayerObj * getPlayer();
	void learnSkill(DWORD dwSkillid,BYTE btLevel);
	int  GetSpeedPer();
	void calculateLuaSpecialAbility(stSpecialAbility* abi);
	void calculateBuffAbility();

private:
	bool IsHit(CCreature* pTarget, const std::shared_ptr<stMagicDataBase>& pMagicData);
	bool IsCrit(CCreature* pTarget);
	int GetAtk(const std::shared_ptr<stMagicDataBase>& pMagicData);
	int GetDef(CCreature* pTarget,BYTE btDamageType);
	int GetCounterAttack(const std::shared_ptr<stMagicDataBase>& pMagicData, CCreature* pTarget,int nAttackVal, stInt& TmpDamageType);
	int PassiveMagicAddAttack(const std::shared_ptr<stMagicDataBase>& pMagicData,int nAttackVal);
public:
	std::CSyncList<stProcessMessage> m_MsgList;
	int m_nAgePoint;				//年龄点数(按小时计算)
	BYTE m_btTargetState;					//目标状态
	eLifeState m_LifeState;					//死亡状态  isdie
	time_t m_dwDieTime;					//死亡时间
	time_t m_dwSpaceMoveTime;
	time_t m_dwMapChangedTime;
	uint8_t m_moveType; //移动类型
	uint8_t m_moveStep; //移动类型
	DWORD m_dwScriptId;
	DWORD m_deCretTypeId;								//NPC基本ID	
	CGameMap* m_pEnvir;  //地图
	stSpaveMoveInfo* m_pswitchsvrmoveinfo;			//正在切换的服务器数据
	time_t m_switchsvrtime;
	BYTE  m_nSpaceMoveMovetype;
	DWORD m_dwLevel;			//等级
	DWORD m_dwLastLevelUpTime;	//上次升级时间
	int m_nNowHP;				//当前血量
	int m_nNowMP;				//当前蓝量
	int m_nNowPP;					//当前体力
	stARpgAbility m_stAbility;		//最终属性
	stARpgAbility m_stExtraAbility;			//额外属性(加成类属性)
	stARpgAbility m_stBaseAbility;			//数据库的基本属性(人物等级对应的基础属性)
	stARpgAbility m_stBuffAbility;			//BUFF的基本属性
	stARpgAbility m_stFirstConvertAbility;	//一级转化二级的属性;
	stARpgAbility m_stTempAbility;			//计算评分的属性;
	bool m_boCheckViewRange;		//检查视野的时候是否调用脚本函数
	WORD m_wHomeMapID;					// 回程地图
	DWORD m_wHomeCloneMapId;
	int m_nHomeX;							// 回城X
	int m_nHomeY;							// 回城Y
	BYTE m_btWudi;							//无敌
	BYTE m_btMiaosha;						//秒杀
	CMagicManage m_cMagic;					//魔法管理器
	CBUFFManager m_cBuff;					//BUFF管理器
	CQuestList m_QuestList;					//玩家和怪物，NPC等附加的任务列表
	CVars m_vars;							//同上，任务变量，可以保存想保存的东西，比如完成任务标记，记录任务目标完成数等
	CScriptTimerManager* m_Timer;			//计时器
	BYTE m_btDirection;					//方向
	BYTE m_nViewRangeX;					//视野
	BYTE m_nViewRangeY;					//视野
	bool m_boGmHide;
	ULONGLONG m_dwNextRunTick;//该对象上次被处理的时间
	DWORD m_nRunIntervalTick;//处理该对象的间隔时间
	ULONGLONG m_dwLastCheckAggroTick;
	bool m_NoCdMode;					//无CD模式
	DWORD m_dwCurAttTargetTmpId;
	CCreature* m_curAttTarget;//目标
	ULONGLONG m_dwLoseTargetTick;
	BYTE m_btDamageType;	//伤害类型
	WORD m_wDamageLoss;		//伤害损耗
	DWORD m_dwLastHiterTmpId;//最后一次攻击者的临时ID
	CCreature* m_pLastHiter;//最后一次的攻击者
	ULONGLONG m_dwNextMoveTick;
	int m_dwMoveIntervalTime;	//移动间隔时间 怪物在数据库中定义 角色为默认设置
	ULONGLONG m_dwNextHitTick;   //下次可攻击时间
	WORD m_dwHitIntervalTime;			//攻击时间间隔
	ULONGLONG m_dwNextCastTick;			//下次可施法时间
	WORD m_dwCastIntervalTime;			//施法时间间隔
	WORD m_dwWalkIntervalTime;			//移动间隔
	DWORD m_dwPkRunTime;				//灰名时间
	bool m_boHpToZero;					//HP为0标记
	bool m_boImmunePoisoning;			//是否免疫施毒术
	bool m_boNoByPush;					//是否不能被推动
	bool m_boNoParalysis;				//禁止被石化
	BYTE m_btBattleCamp;				//战场阵营
	bool m_boBattleWuDi;				//战场无敌状态
	bool m_boAllowSpace;				//是否允许传送
	bool isTransMove;
	Point m_changeMapPos;
	DWORD m_dwNoviceGuideId;				//新手引导id 存档
	#pragma endregion
};

template <typename T>
void CCreature::UpdateAppearance(FeatureIndex index, T value)
{
	static_assert(std::is_trivially_copyable_v<T>,
		"UpdateAppearance: T must be trivially copyable");
	constexpr int8_t sz = GetSizeCode<T>();
	static_assert(sz <= APPEAR_MAX_SIZE,
		"UpdateAppearance: sizeof(T) exceeds supported max");
	auto idx = static_cast<uint8_t>(index);
	if (idx > APPEAR_MAX_INDEX) {
		throw std::out_of_range("FeatureIndex out of range");
	}
	uint8_t flag = static_cast<uint8_t>((sz << 6) | idx);
	m_featureStream.Write(&flag, 0, sizeof(flag));
	m_featureStream.Write(&value, 0, sizeof(value));
	//g_logger.debug("UpdateAppearance: index=%s, value=%d", to_string(index), value);
}

template <class CMD>
stQueueMsg* CCreature::CopyQueueMsg(CMD* pcmd, int ncmdlen)
{
	stQueueMsg* pmsg=NULL;
	if (NewPacketBuffer(pmsg,sizeof(stQueueMsg)+ncmdlen)){
		CopyMemory(&pmsg->cmdBuffer,pcmd,ncmdlen);
		pmsg->cmdsize=ncmdlen;
		pmsg->pluscmdoffset=0;
	}
	return pmsg;
}
