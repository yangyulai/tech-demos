#pragma once

#include "LocalDB.h"
#include "event/EventDispatcher.h"

class CCreature;
class CMagicManage;
struct stARpgAbility;
struct stCretAttack;
struct stProcessMessage;
class CGameMap;

class stBuff;

using BuffFunction = std::function<BYTE(CCreature*, stBuff*, int&)>;
constexpr size_t MAX_BUFF_FUNC = 255;

//״̬���ͣ������״̬��ʾ��ӦFeatureStatus
enum emMagicStateType:uint8_t
{
	MAGICSTATE_NULL = 0,				//û��״̬
	MAGICSTATE_HP = 1,					//Ѫ���,������
	MAGICSTATE_MP = 2,					//�����
	MAGICSTATE_RELIVE = 3,				//����
	MAGICSTATE_SPEEDSLOW = 4,			//����
	MAGICSTATE_DIZZY = 5,				//��ѣ
	MAGICSTATE_PETRIFACTION = 6,		//ʯ��
	MAGICSTATE_ABI = 7,					//�������
	MAGICSTATE_ABI_PERCENT = 8,			//(���԰ٷֱ�)
	MAGICSTATE_MAGICSHIELD = 9,			//��ʦ�ķ���
	MAGICSTATE_SWORDSHIELD = 10,		//��ʿ�Ŀ�ħ����
	MAGICSTATE_MULTEXP = 11,			//�౶����

	MAGICSTATE_MAXCOUNT,				//���״ֵ̬
};

enum emBuffTime{
	TimeNo		=	0,	//û��ʱ������
	TimeOut		=	1,	//����Ϊ��λ�ĳ���ʱ������ ,�����ڿ�ʼ�����ֵ��������,�����޸�_keep_tick�ӳ�ʱ��
	TimeRun		=	2,	//����Ϊ��λ�ļ��ʱ������,�����ڿ�ʼ��������ÿ���������ִ��һ�Σ������޸�_last_tick�ӳ�ʱ��
	SecondOut	=	3,	//��Ϊ��λ�ĳ���ʱ������,�����ڿ�ʼ�����ֵ��������,�����޸�_keep_tick�ӳ�ʱ��
	SecondRun	=	4,	//��Ϊ��λ�ļ��ʱ������,�����ڿ�ʼ��������ÿ���������ִ��һ�Σ������޸�_last_tick�ӳ�ʱ��
	TimeInfinite =  5,	//����buff
};

enum emBuffRunType :uint8_t
{
	BUFF_NEW,		//��BUFF
	BUFF_RUN,		//BUFF�Ѿ����룬������
	BUFF_DEL,		//BUFFʧЧ
};

class stBuff{
public:
	stBuff(const std::shared_ptr<stBuffDataBase>& buffDataBase);
	uint32_t GetBuffId() const;
	BYTE	GetLevel() const;;	//���BUFF�ȼ�
	BYTE	GetTimeType() const;
	const char* GetBuffName() const;
	int64_t GetTimeLeft() const;

	int64_t m_keepTime;
	int64_t m_addTime;
	int64_t m_eventId;

	uint32_t dwBuffID;
	int nDura;
	DWORD m_AttackCretTmpId;

	uint8_t m_AttakCretType;
	uint8_t m_buffLevel;
	bool bInEffect;		//�Ƿ���Ч�У�������ڻ��⼼�ܣ�buffʧЧ
	bool boNotShow;
	emBuffRunType m_emRunType;
	emMagicStateType m_emStateType;
	mutable std::weak_ptr<stBuffDataBase> m_buffDataBase;
	std::shared_ptr<stBuffDataBase> GetBuffDataBase() const;

};

class CBUFFManager{
public:
	using BuffPtr = std::unique_ptr<stBuff>;
protected:
	enum {
		MAX_NUM = 60,
	};
	std::unordered_map<uint32_t, BuffPtr> idMap_;
	std::unordered_multimap<emMagicStateType, stBuff*> stateMap_;

	ULONGLONG m_dwBuffTimeTick;
	CCreature* m_Owner;
	EventDispatcher m_event;
public:
	std::atomic<bool> m_BuffStateList[MAGICSTATE_MAXCOUNT];
	inline static std::array<BuffFunction, MAX_BUFF_FUNC + 1> g_buff_registry;

public:
	CBUFFManager(CCreature* Owner){ 
		m_dwBuffTimeTick=0;
		m_Owner=Owner;
		for (int i=0;i<MAGICSTATE_MAXCOUNT;i++)
		{
			m_BuffStateList[i] = false;
		}	
	}
	~CBUFFManager(){ clear(); }
	void Run() { m_event.run(); }
	bool AddBuff(DWORD dwBuffID, BYTE btLevel, int64_t dwKpTime=0, int nDura = 0, CCreature *pA=nullptr);
	bool AddBuff(std::shared_ptr<stBuffDataBase>& buffDataBase, int64_t dwKpTime=0, int nDura = 0);
	stBuff* FindBuff(DWORD dwBuffID);
	bool RemoveBuff(DWORD dwBuffID);
	bool RemoveBuff(stBuff* buff);
	bool RemoveBuff(emMagicStateType StateType);
	bool LuaRemoveBuff(DWORD dwBuffID) {return RemoveBuff(dwBuffID);}
	void clear();
	BYTE GetBuffLevel(emMagicStateType StateType);
	bool GetBuffState(emMagicStateType StateType);
	int64_t BuffFeature();
	int GetBuffCof(emMagicStateType StateType);
	void OfflineRemoveBuff();
	static void initBuffFun();//��UsrEngn�е���
	bool savetmpbuff(char* dest,DWORD& retlen);
	bool savebuff(char* dest,DWORD& retlen);
	bool loadbuff(const char* dest,int retlen,int nver);
	stBuff* FindByStateType(emMagicStateType StateType);
	stBuff* FindByBuffID(DWORD dwBuffid);
	bool IsControlBuff(BYTE StateType);
	void calculateBuffAbility();
	static void Exec_BuffLogicFunc(CCreature* owner, stBuff* buff, int& nNum);

};