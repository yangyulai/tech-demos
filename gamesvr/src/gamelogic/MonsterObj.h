#pragma once
#include "BaseCreature.h"

enum class emMonsterType :uint8_t{
	NORMAL			= 0,		//普通怪物
	ELITEMON		= 1,		//精英怪
	BOSS			= 2,		//BOSS
	PET				= 3,		//宠物
	PRACTISE		= 4,		//练功师
	ORE				= 5,		//强制防御,掉血一点,矿石
	FORTRESSDOOR	= 6,		//军团要塞门，只统计伤害，不扣血
	FORTRESSBALL	= 7			//军团要塞，能量球，友方回血，敌方扣血
};

struct stHatredInfo {
	int value;
	time_t last_time;
	bool operator<(const stHatredInfo& other) const {
		return value < other.value;
	}
};
struct stMonAiCfg {
	union
	{
		uint32_t nMonType;
		struct
		{
			uint32_t bitNoAttack : 1;			//=1 不能攻击
			uint32_t bitNoAutoAtt : 1;			//=1 不主动攻击
			uint32_t bitNoStruckAtt : 1;		//=1 不还击
			uint32_t bitGoStop : 1;			//=1 主动攻击或则还击 后 会回到追击或还击前的位置

			uint32_t bitLowHpRunAway : 1;		//=1 血少就逃跑
			uint32_t bitFlytoTarget : 1;		//=1 可以如果目标脱离攻击范围直接飞到目标附近
			uint32_t bitCallNearMon : 1;		//=1 召集附近的怪物	
			uint32_t bitLowHpFlyAway : 1;		//=1 血少就飞到附近躲起来	
			uint32_t bitGoHome : 1;			//=1 会自己回到出生点
			uint32_t bitWondering : 1;			//=1 在不激活状态也会随机行走
			uint32_t bitNoPushed : 1;			//=1 不能被撞动
			uint32_t bitCanNotMove : 1;		//=1 不能移动
		};
	};
};
class CPlayerObj;

class CMonster :public CCreature
{
	friend class CGameMap;
public:
	CMonster(const CMonster& other) = delete;
	CMonster(CMonster&& other) noexcept = delete;
	CMonster& operator=(const CMonster& other) = delete;
	CMonster& operator=(CMonster&& other) noexcept = delete;
	CMonster(uint8_t type, PosType x, PosType y, const std::shared_ptr<stMonsterDataBase>& monInfo, uint32_t mon_gen_id,DWORD dwTmpId);
	~CMonster() override;
	void InitAiConfig();
	bool Operate(stBaseCmd* pcmd, int ncmdlen, stProcessMessage* pMsg) override;
	bool Die() override;				//死亡
	void EnterMapNotify(MapObject* obj) override;
	void AddMonsterSkills();
	void Update() override;
	void SendRefMsg(void* pcmd,int ncmdlen,bool exceptme=false) override;
	void SendMsgToMe(void* pcmd,int ncmdlen,int zliblvl = Z_DEFAULT_COMPRESSION) override{}
	DWORD getOwnerId() const { return m_dwOwnerShipId; }
	virtual void RunMonsterAI();
	bool GetAttackDir(CCreature* Target,int nRange,int &nDir);
	virtual bool AttackTarget(stMagic* pMagic,int nRange);
	virtual void RunNoTargetAI(bool boNoActive = false);
	virtual void RunTargetAI();
	void SetAttackTarget(CCreature* pTarget, bool ignoreTarget = false) override;
	void MonsterMoveToHome();
	void GotoHomeXY();
	bool MonsterTryWalk(int wantdir, int nsetp, int nx, int ny, bool boNotCheckObj);
	void MakeGhost(bool delaydispose,const char* ff) override;
	const char* getShowName(char* szbuffer=NULL,int nmaxlen=0) override;		//获得显示用的名字
	int getFeatureId(MapObject* obj);
	bool CanHit() override;
	bool CanWalk() override;
	bool CanRun() override;
	bool CanMagic() override;
	void GetBaseProperty() override;						//从基本属性数据库加载基本属性
	bool MoveTo(int nDir, int nmovesetp, int ncx, int ncy, bool boNotCheckObj) override;
	static void SelectDropItem(std::shared_ptr<stDropItemDataBase>& pDropItemData, CPlayerObj* pPlayer, std::vector<CItem*>& tmpV);
	void DoChangeProperty(stARpgAbility& abi,bool boNotif,const char* ff) override;
	bool randomDropItem(CPlayerObj* player=NULL);
	bool MonGetReward(CPlayerObj* player);
	bool MonsterAttack(CCreature* pTarget, stMagic* pMagic, int nDir,DWORD dwPlayTime=0,bool boSendAttack=true);	//怪物攻击
	bool DamageSpell(__int64 nDamage) override;
	virtual void Struck(CCreature* pHiter);
	bool CalculatingSpeed() override;	//计算速度
	int CalculatingRestoreHp() override;	//计算回血
	bool OnCretStruck(stCretStruckFull* cmd, unsigned int ncmdlen) override; //处理被攻击的消息
	bool OnCretAction(stCretActionRet* pcmd, unsigned int ncmdlen, stProcessMessage* param) override; //处理施法动作的消息
	bool isEnemy(CCreature* pTarget=NULL) override;
	bool AddExp(double nAdd, const char* szopt = NULL) override;
	bool LevelUp(int nAdd = 1, bool bsend = true) override;
	int GetNoAutoAtt() const {return m_AiCfg.bitNoAutoAtt;};
	void SetNoAutoAtt(int NoAutoAtt){m_AiCfg.bitNoAutoAtt=NoAutoAtt;};
	int GetCanNotMove() const {return m_AiCfg.bitCanNotMove;};
	void SetCanNotMove(int bitCanNotMove){m_AiCfg.bitCanNotMove=bitCanNotMove;};
	int GetNoStruckAtt() const {return m_AiCfg.bitNoStruckAtt;};
	void SetNoStruckAtt(int bitNoStruckAtt){m_AiCfg.bitNoStruckAtt=bitNoStruckAtt;};
	int GetPetCastingRange();
	int GetPetAttackRange();
	int GetOreCostEnergy();
	bool IsSpecialType() const;
	emMonsterType GetMonType();

	stMonAiCfg m_AiCfg;
	int32_t m_monsterId;
	mutable std::weak_ptr<stMonsterDataBase> m_monsterDataBase;
	std::shared_ptr<stMonsterDataBase> GetMonsterDataBase() const;

	DWORD m_dwOwnerShipId;				//怪物归属权ID,标示是谁打的
	ULONGLONG m_dwNextOwnerShipTime;	//下次计算归属权的时间
	DWORD m_dwOwnerShipIntervalTick;	//计算归属权的间隔时间
	char m_szMonsterShowName[_MAX_NAME_LEN_];
	DWORD m_dwDifficultyCof;
	emMonsterType m_btType;					//怪物类型，一般是数据库配，这里用来给脚本自己修改
	int m_nLastPetrifactionTime;	//最后石化时间
	BYTE m_btZhongJiLvl;
	int m_guildId;					//怪物记录行会信息
	std::vector<DWORD> m_vAtkPlayer;				//击打过怪物的玩家
	std::priority_queue<stHatredInfo> m_hatred;		//仇恨值
	uint32_t m_monGenId = 0;
	bool m_isGoingHome = false;
};