#pragma once 

#include "MonsterObj.h"
#include "CMD/Guild_cmd.h"
#include "Package.h"

class CRobot final :public CMonster{
public:
	CRobot(const CRobot& other) = delete;
	CRobot(CRobot&& other) noexcept = delete;
	CRobot& operator=(const CRobot& other) = delete;
	CRobot& operator=(CRobot&& other) noexcept = delete;

	CRobot(PosType x, PosType y, char* szName, std::shared_ptr<stMonsterDataBase>& pMoninfo);
	~CRobot() override;
	void Update() override;
	void RunTargetAI() override;
	bool Die() override;

	stMagic* ChooseActSkill(const std::function<bool(stMagic*, stMagic*)>& predicate);

	void EnterMapNotify(MapObject* obj) override;
	void GetBaseProperty() override;								//从基本属性数据库加载基本属性
	void ChangeProperty(bool bosend = true, const char* ff = "") override;
	void DoChangeProperty(stARpgAbility& abi, bool boNotif, const char* ff) override;
	void BuildPlayerFeature(stPlayerFeature& feature);
	const char* getShowName(char* szbuffer = NULL, int nmaxlen = 0) override;		//获得显示用的名字
	int GetSexType() const { return (int)this->m_siFeature.sex; }					//获取玩家性别类型
	emJobType GetJobType() { return (emJobType)this->m_siFeature.job; }		//获取玩家职业类型
	bool LoadHumanBaseData(stLoadPlayerData* pgamedata);							//加载人物数据并且计算属性
	bool LoadHumanData(stLoadPlayerData* pgamedata);								//复杂属性
	bool LoadEquip(const char* dest, int retlen, int nver);
	bool isEnemy(CCreature* pTarget = NULL) override;

	void StudySkill(DWORD dwSkillid, BYTE btLevel);
	bool DeleteSkill(DWORD dwSkillid);
	void ClearSkill();

	void calculateLuaAbility(stARpgAbility* abi);
	void calculateLuaTimeAbility(stARpgAbility* abi);	//计算限时属性
	void calculateAttrPointAbility(stARpgAbility* abi);
	bool CalculatingSpeed() override;

	bool ResChange(ResID rid, int nChange, const char* szEvtName);							// 资源更改
	DWORD getVipType();

	int64_t m_i64UserOnlyID;				//唯一角色ID
	int     m_nVipLevel;
	DWORD m_dwHeadPortrait;
	stMagic* m_curMagic;

	// 数据配置的人物数据
	int m_nAttackInterval;	//攻击间隔
	int m_nReleaseInterval;	//释放间隔
	int m_nMoveInterval;		//移动间隔
	int m_nPpMoveCost;		//移动消耗体力
	float m_fHpDrugRate;
	float m_fMpDrugRate;

	stARpgAbility m_stEquipAbility;	//装备的基本属性
	stARpgAbility m_stLuaAbility;	//脚本的基本属性
	stARpgAbility m_stLuaTimeAbility;	//脚本的限时属性
	stARpgAbility m_stAttrPointAbility;	 //属性点的属性
	stARpgAbility m_stPassiveSkillAbility;	 //被动技能的属性

	stSpecialAbility  m_stSpecialAbility;			 //特殊属性
	stSpecialAbility  m_stSpecialEquipAbility;		 //装备特殊属性
	stSpecialAbility  m_stSpecialLuaAbility;		 //lua特殊属性
	stSpecialAbility  m_stSpecialBuffAbility;		 //buff特殊属性

	stSimpleFeature m_siFeature;
	stSimpleAbility m_siAbility;
	stGSGuildInfo		m_GuildInfo;	//行会信息
	stRes m_res;							// 资源

	CPlayerPackage		m_Packet;		//包裹及仓库
	//CPetManage			m_Petm;			//宠物管理器
};
