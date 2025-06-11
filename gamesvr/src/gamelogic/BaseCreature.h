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

//������Կ����ļ���
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
	PK_STATUS_RUNTIME,	//PK����ʱ��
	PK_STATUS_PKSWORD,	//PKɱ¾ֵ
};

#define  CAN_NOT_HIT_TARGET			0x01			//�ܹ���Ŀ�� Ĭ�Ͽ��Թ���
#define  CAN_NOT_MAGIC_TARGET		0x02			//��ħ������Ŀ�� Ĭ�Ͽ��Ա�ħ������
#define  CAN_NOT_SEL_TARGET			0x04			//��ѡ��Ŀ�� Ĭ�Ͽ��Ա�ѡ��
#define  CAN_VISIT_TARGET			0x80			//�ܶԻ���Ŀ�� Ĭ�ϲ��ܶԻ�

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
	BYTE btType;	//0:����, 1:Ӣ��սʿ, 2:Ӣ�۷�ʦ, 3:Ӣ�۵�ʿ
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
	virtual bool Operate(stBaseCmd* pcmd, int ncmdlen, stProcessMessage* pMsg);	//����������Ϣ�¼�
	virtual bool GetGameMessage(stProcessMessage& ProcessMsg);
	//��ý�ɫ���ڵ�ͼ
	CGameMap* GetEnvir() {	return m_pEnvir; }			
	bool SetEnvir(CGameMap* pEnvir);				//���õ�ͼ
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
	virtual void GetBaseProperty();						//�ӻ����������ݿ���ػ�������

	virtual void ChangeProperty(bool bosend = true,const char* ff="");						//���ݻ�������,����,�������ܼ�����������,���ҷ��ظ��ͻ���
	void ChangePropertyCheckPlayer(emAbilityFlag type = ABILITY_FLAG_ALL);
	virtual void DoChangeProperty(stARpgAbility& abi,bool boNotif,const char* ff);
	void StatusValueChange(BYTE btIndex, int nChange, const char* szLog = nullptr, bool isForce = false);						//״ֵ̬����
	//==========================================================================
	virtual void SendRefMsg(void* pcmd,int ncmdlen,bool exceptme=false);										//�㲥�����ӷ�Χ�ڵĽ�ɫ
	void SendMsgToMe(void* pcmd,int ncmdlen,int zliblvl = Z_DEFAULT_COMPRESSION) override{}
	virtual bool isInViewRange(CCreature* pCret,bool boPrintFalselog =false);																//�Ƿ��ڿ��ӷ�Χ֮��
	virtual bool isInViewRangeXY(WORD x,WORD y);
	void ProcessMsgList();
	void Update() override;																							//��ɫ��ȡ��Ϣ
	void ClearMsg();
	virtual void MakeGhost(bool delaydispose,const char* ff);											//��Ƕ���Ϊ����ɾ�� ���б���ɾ��
	virtual void Disappear();											//��ʧ
	void fullMoveRet(stCretMoveRet* pcmd, BYTE errorcode);				//���move��������
	#pragma region ����
	bool DoSpaceMove(stSpaveMoveInfo* pSpaceMoveInfo);																					//ִ���л���ͼ
	bool CheckMapSpace(stSpaveMoveInfo* pSpaceMoveInfo);
	bool HandleCrossServerMove(stSpaveMoveInfo* pSpaceMoveInfo);
	void FillPreChangeGameSvrCmd(stPreChangeGameSvrCmd* checkcmd, stSpaveMoveInfo* pSpaceMoveInfo);
	bool HandleMoveLocalSameMap(stSpaveMoveInfo* pSpaceMoveInfo);
	bool HandleMoveLocalDiffMap(stSpaveMoveInfo* pSpaceMoveInfo);
	void FillFastTransferPlayerCmd(stFastTransferPlayer* cmd, CPlayerObj* player);
	void FillNotifyPlayerChangeGameSvrCmd(stNotifyPlayerChangeGameSvrCmd* ncc, CPlayerObj* player);
	#pragma endregion
	virtual void MapChanged();				//֪ͨ�ͻ��˵�ͼ�ı�
	virtual bool isClientReady(){ return true; }
	void FeatureChanged();					//��۸ı�
	virtual bool NameChanged();						//���ָı�
	virtual bool LevelChanged(BYTE btShow=0);					//�ȼ��ı�
	virtual const char* getShowName(char* szbuffer=NULL,int nmaxlen=0);		//�����ʾ�õ�����
	const char* LuaGetName() { return GTU(getName()); };
	bool isDie() { return (m_LifeState!=NOTDIE);}															//����״̬
	bool isCanVisit(){ return ((m_btTargetState & CAN_VISIT_TARGET)!=0); }										//�Ƿ���Է���
	bool isCanHit(){return ((m_btTargetState & CAN_NOT_HIT_TARGET)==0);}										//�Ƿ���Ա�����
	bool isCanMagic(){return ((m_btTargetState & CAN_NOT_MAGIC_TARGET)==0);}									//�Ƿ���Ա�ħ������
	bool isCanSelect(){return ((m_btTargetState & CAN_NOT_SEL_TARGET)==0);}									//�Ƿ���Ա�ѡ��
	virtual bool CanBeSee(CCreature* pCret);																			//�Ƿ�ɼ�
	virtual bool isBoss();       //�Ƿ�boss
	virtual bool isEliteMon();   //�Ƿ�Ӣ��
	virtual bool isLittleMon();  //�Ƿ�С��

	void SendReliveMsg();
	virtual bool Die();									//����
	virtual bool Relive(int64_t nHp=0, int64_t nMp=0);
	bool LuaRelive(double nHp = 0, double nMp = 0);
	void SendLiftStateChange();
	stSpaveMoveInfo* getSwitchSvrInfo() const { return m_pswitchsvrmoveinfo;}
	void setSwitchSvrInfo(stSpaveMoveInfo* p){ m_pswitchsvrmoveinfo=p;m_switchsvrtime=time(NULL); }

	virtual bool isSwitchSvr();		//�Ƿ������л�������
	bool isSwitchSvrTimeOut();

	virtual bool isValidObj();

	virtual bool isSafeZone(CCreature* pTarget=NULL);										//�����Ƿ��ڰ�ȫ��
	virtual bool testHomeXY();	

	bool GetFrontPosition(int &nX,int &nY);
	CCreature* GetPoseCreate();

	virtual void StruckDamage(int64_t nDamage, CCreature *pSrcCret = NULL, Byte btDamageSrc = 0);
	virtual bool DamageSpell(int64_t nDamage);
	virtual bool SpellCanUse(int64_t nDamage);
	virtual bool CalculatingPk(CCreature* pCret=NULL,emPkStatus PkStatus=PK_STATUS_PKSWORD);
	virtual	bool CalculatingSpeed();	//�����ٶ�
	virtual int CalculatingRestoreHp();	//�����Ѫ
	virtual int CalculatingRestoreMp();	//�������
	virtual int CalculatingRestorePp();	//��������
	virtual int64_t CalculatingDamage(CCreature* pTarget, const std::shared_ptr<stMagicDataBase>& pMagicData, stInt& TmpDamageType,int& nAggro,int nTargetNum,int nPlayerTargetNum,int nTotalTargetNum);	//�����˺�
	virtual bool CalculatingTarget(std::vector<CCreature*>& vTarget,stMagic* pMagic,DWORD dwTmpId,int nX,int nY);	//����Ŀ��
	virtual bool DoAttack(std::vector<CCreature*> vTarget,stCretAttack* pAttackCmd, stMagic* pMagic, int nDir, DWORD dwPlayTime=0,bool boSendAttack=true);
	virtual bool OnCretStruck(stCretStruckFull* pcmd, unsigned int ncmdlen) { return true; } //������������Ϣ
	virtual bool OnCretAction(stCretActionRet* pcmd, unsigned int ncmdlen, stProcessMessage* param); //����ʩ����������Ϣ
	int64_t CalculatingBaseDamage(CCreature* pTarget, const std::shared_ptr<stMagicDataBase>& pMagicData, stInt& TmpDamageType, int nTargetNum, int nPlayerTargetNum, int nTotalTargetNum);	//��������˺�
	void ReleaseCretStruckFull(CCreature* Target, const std::shared_ptr<stMagicDataBase>&, int nTargetNum, int nPlayerTargetNum,int nTotalTargetNum,int32_t delay = 0);
	bool isSkillRange(int nX,int nY,BYTE btRange);
	virtual bool isEnemy(CCreature* pTarget=NULL);
	bool isWudi() {return (m_btWudi==1);}
	BYTE isMiaosha() {return m_btMiaosha;}

	void TriggerEvent(CCreature* currentuser,EVTTYPE evtid, DWORD evtnum,WORD wTags=0,WORD mapx=0,WORD mapy=0);				//���ﴥ���¼�����

	virtual bool AddExp(double nAdd, const char* szopt = NULL);
	virtual bool LevelUp(int nAdd = 1, bool bsend=true);
	stMagic* FindSkill(DWORD dwSkillid);				//�ҵ���ѧϰ�ļ���

	#pragma region lua���
	bool LuaSelfAddBuff(DWORD dwBuffID,BYTE btLevel,double dwKpTime,int dura);
	bool LuaSelfAddBuff2(DWORD dwBuffID, BYTE btLevel, double dwKpTime, int dura, CCreature* pA = nullptr);
	stBuff* LuaFindBuff(DWORD dwBuffID);
	stBuff* LuaGetBuffByType(int nState);
	bool LuaRemoveBuff(DWORD dwBuffID);
	bool LuaRemoveBuffState(int nState);
	void LuaBuffClear();
	bool LocalMapTransfer(int nx,int ny,int nrange,bool boNotCheckObj=true); 
	bool LuaGetBuffState(int nState);
	void LuaFeatureChanged();							//������۸ı�
	void LuaUpdateAppearance(BYTE index, int id);							//������۸ı�
	BYTE GetBattleCamp() const {return m_btBattleCamp;}
	void SendCretStruck(CCreature* pCret,int npower,int ntype);		//�����˺�������
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
	int m_nAgePoint;				//�������(��Сʱ����)
	BYTE m_btTargetState;					//Ŀ��״̬
	eLifeState m_LifeState;					//����״̬  isdie
	time_t m_dwDieTime;					//����ʱ��
	time_t m_dwSpaceMoveTime;
	time_t m_dwMapChangedTime;
	uint8_t m_moveType; //�ƶ�����
	uint8_t m_moveStep; //�ƶ�����
	DWORD m_dwScriptId;
	DWORD m_deCretTypeId;								//NPC����ID	
	CGameMap* m_pEnvir;  //��ͼ
	stSpaveMoveInfo* m_pswitchsvrmoveinfo;			//�����л��ķ���������
	time_t m_switchsvrtime;
	BYTE  m_nSpaceMoveMovetype;
	DWORD m_dwLevel;			//�ȼ�
	DWORD m_dwLastLevelUpTime;	//�ϴ�����ʱ��
	int m_nNowHP;				//��ǰѪ��
	int m_nNowMP;				//��ǰ����
	int m_nNowPP;					//��ǰ����
	stARpgAbility m_stAbility;		//��������
	stARpgAbility m_stExtraAbility;			//��������(�ӳ�������)
	stARpgAbility m_stBaseAbility;			//���ݿ�Ļ�������(����ȼ���Ӧ�Ļ�������)
	stARpgAbility m_stBuffAbility;			//BUFF�Ļ�������
	stARpgAbility m_stFirstConvertAbility;	//һ��ת������������;
	stARpgAbility m_stTempAbility;			//�������ֵ�����;
	bool m_boCheckViewRange;		//�����Ұ��ʱ���Ƿ���ýű�����
	WORD m_wHomeMapID;					// �س̵�ͼ
	DWORD m_wHomeCloneMapId;
	int m_nHomeX;							// �س�X
	int m_nHomeY;							// �س�Y
	BYTE m_btWudi;							//�޵�
	BYTE m_btMiaosha;						//��ɱ
	CMagicManage m_cMagic;					//ħ��������
	CBUFFManager m_cBuff;					//BUFF������
	CQuestList m_QuestList;					//��Һ͹��NPC�ȸ��ӵ������б�
	CVars m_vars;							//ͬ�ϣ�������������Ա����뱣��Ķ�����������������ǣ���¼����Ŀ���������
	CScriptTimerManager* m_Timer;			//��ʱ��
	BYTE m_btDirection;					//����
	BYTE m_nViewRangeX;					//��Ұ
	BYTE m_nViewRangeY;					//��Ұ
	bool m_boGmHide;
	ULONGLONG m_dwNextRunTick;//�ö����ϴα������ʱ��
	DWORD m_nRunIntervalTick;//����ö���ļ��ʱ��
	ULONGLONG m_dwLastCheckAggroTick;
	bool m_NoCdMode;					//��CDģʽ
	DWORD m_dwCurAttTargetTmpId;
	CCreature* m_curAttTarget;//Ŀ��
	ULONGLONG m_dwLoseTargetTick;
	BYTE m_btDamageType;	//�˺�����
	WORD m_wDamageLoss;		//�˺����
	DWORD m_dwLastHiterTmpId;//���һ�ι����ߵ���ʱID
	CCreature* m_pLastHiter;//���һ�εĹ�����
	ULONGLONG m_dwNextMoveTick;
	int m_dwMoveIntervalTime;	//�ƶ����ʱ�� ���������ݿ��ж��� ��ɫΪĬ������
	ULONGLONG m_dwNextHitTick;   //�´οɹ���ʱ��
	WORD m_dwHitIntervalTime;			//����ʱ����
	ULONGLONG m_dwNextCastTick;			//�´ο�ʩ��ʱ��
	WORD m_dwCastIntervalTime;			//ʩ��ʱ����
	WORD m_dwWalkIntervalTime;			//�ƶ����
	DWORD m_dwPkRunTime;				//����ʱ��
	bool m_boHpToZero;					//HPΪ0���
	bool m_boImmunePoisoning;			//�Ƿ�����ʩ����
	bool m_boNoByPush;					//�Ƿ��ܱ��ƶ�
	bool m_boNoParalysis;				//��ֹ��ʯ��
	BYTE m_btBattleCamp;				//ս����Ӫ
	bool m_boBattleWuDi;				//ս���޵�״̬
	bool m_boAllowSpace;				//�Ƿ�������
	bool isTransMove;
	Point m_changeMapPos;
	DWORD m_dwNoviceGuideId;				//��������id �浵
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
