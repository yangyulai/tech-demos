#pragma once
#include "BaseCreature.h"

enum class emMonsterType :uint8_t{
	NORMAL			= 0,		//��ͨ����
	ELITEMON		= 1,		//��Ӣ��
	BOSS			= 2,		//BOSS
	PET				= 3,		//����
	PRACTISE		= 4,		//����ʦ
	ORE				= 5,		//ǿ�Ʒ���,��Ѫһ��,��ʯ
	FORTRESSDOOR	= 6,		//����Ҫ���ţ�ֻͳ���˺�������Ѫ
	FORTRESSBALL	= 7			//����Ҫ�����������ѷ���Ѫ���з���Ѫ
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
			uint32_t bitNoAttack : 1;			//=1 ���ܹ���
			uint32_t bitNoAutoAtt : 1;			//=1 ����������
			uint32_t bitNoStruckAtt : 1;		//=1 ������
			uint32_t bitGoStop : 1;			//=1 �����������򻹻� �� ��ص�׷���򻹻�ǰ��λ��

			uint32_t bitLowHpRunAway : 1;		//=1 Ѫ�پ�����
			uint32_t bitFlytoTarget : 1;		//=1 �������Ŀ�����빥����Χֱ�ӷɵ�Ŀ�긽��
			uint32_t bitCallNearMon : 1;		//=1 �ټ������Ĺ���	
			uint32_t bitLowHpFlyAway : 1;		//=1 Ѫ�پͷɵ�����������	
			uint32_t bitGoHome : 1;			//=1 ���Լ��ص�������
			uint32_t bitWondering : 1;			//=1 �ڲ�����״̬Ҳ���������
			uint32_t bitNoPushed : 1;			//=1 ���ܱ�ײ��
			uint32_t bitCanNotMove : 1;		//=1 �����ƶ�
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
	bool Die() override;				//����
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
	const char* getShowName(char* szbuffer=NULL,int nmaxlen=0) override;		//�����ʾ�õ�����
	int getFeatureId(MapObject* obj);
	bool CanHit() override;
	bool CanWalk() override;
	bool CanRun() override;
	bool CanMagic() override;
	void GetBaseProperty() override;						//�ӻ����������ݿ���ػ�������
	bool MoveTo(int nDir, int nmovesetp, int ncx, int ncy, bool boNotCheckObj) override;
	static void SelectDropItem(std::shared_ptr<stDropItemDataBase>& pDropItemData, CPlayerObj* pPlayer, std::vector<CItem*>& tmpV);
	void DoChangeProperty(stARpgAbility& abi,bool boNotif,const char* ff) override;
	bool randomDropItem(CPlayerObj* player=NULL);
	bool MonGetReward(CPlayerObj* player);
	bool MonsterAttack(CCreature* pTarget, stMagic* pMagic, int nDir,DWORD dwPlayTime=0,bool boSendAttack=true);	//���﹥��
	bool DamageSpell(__int64 nDamage) override;
	virtual void Struck(CCreature* pHiter);
	bool CalculatingSpeed() override;	//�����ٶ�
	int CalculatingRestoreHp() override;	//�����Ѫ
	bool OnCretStruck(stCretStruckFull* cmd, unsigned int ncmdlen) override; //������������Ϣ
	bool OnCretAction(stCretActionRet* pcmd, unsigned int ncmdlen, stProcessMessage* param) override; //����ʩ����������Ϣ
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

	DWORD m_dwOwnerShipId;				//�������ȨID,��ʾ��˭���
	ULONGLONG m_dwNextOwnerShipTime;	//�´μ������Ȩ��ʱ��
	DWORD m_dwOwnerShipIntervalTick;	//�������Ȩ�ļ��ʱ��
	char m_szMonsterShowName[_MAX_NAME_LEN_];
	DWORD m_dwDifficultyCof;
	emMonsterType m_btType;					//�������ͣ�һ�������ݿ��䣬�����������ű��Լ��޸�
	int m_nLastPetrifactionTime;	//���ʯ��ʱ��
	BYTE m_btZhongJiLvl;
	int m_guildId;					//�����¼�л���Ϣ
	std::vector<DWORD> m_vAtkPlayer;				//�������������
	std::priority_queue<stHatredInfo> m_hatred;		//���ֵ
	uint32_t m_monGenId = 0;
	bool m_isGoingHome = false;
};