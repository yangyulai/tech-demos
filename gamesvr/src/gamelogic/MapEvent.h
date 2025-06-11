#pragma once
#include "MapObject.h"

enum emMapEventType{
	MAPEVENT_NULL,					//û���¼�
	MAPEVENT_ITEM,					//��Ʒ����
	MAPEVENT_MAGIC,					//ħ������
};

enum emDisappearType{
	DISAPPEAR_TIME,					//ʱ�䵽����ʧ
	DISAPPEAR_ADD,					//��Ʒ������ʧ
};

enum emOwnerType{
	OWNER_NULL,						//û������
	OWNER_MONSTER,					//��������
	OWNER_NPC,						//NPC����
	OWNER_PLAYER,					//�������
	OWNER_PET,						//��������
};

enum emMagicType{
	MAGIC_NULL,						//û��ħ������
	MAGIC_FIRE,						//��ǽ
	MAGIC_AIR,						//��ǽ
	MAGIC_FIRERAIN,					//����
	MAGIC_TIANHUO,					//���
};

class CGameMap;
class CItem;
class CCreature;
class CPlayerObj;
struct stRefMsgQueue;

class CMapEvent :public MapObject
{
public:
	CMapEvent(const CMapEvent& other) = delete;
	CMapEvent(CMapEvent&& other) noexcept = delete;
	CMapEvent& operator=(const CMapEvent& other) = delete;
	CMapEvent& operator=(CMapEvent&& other) noexcept = delete;

	CMapEvent(std::string_view name, uint8_t type, PosType x, PosType y);
	~CMapEvent() override;
 	void Update() override;
	emMapEventType GetEventType() {return m_emEventType;}	//�õ��¼�����
	emOwnerType GetOwnerType() {return m_emOwnerType;}	//�õ���������
	emDisappearType GetDisappearType() {return m_emDisappearType;}	//�õ���ʧ����
	void SetDisappearType(emDisappearType DisappearType) {m_emDisappearType=DisappearType;}	//������ʧ����
	bool isCanContinue() const;
	//ħ���Ƿ񻹳���
	bool isCanBegin() const;
	//�Ƿ��ܿ�ʼRUN
	void SetTime(time_t BeginTime,time_t ContinueTime) {m_tBeginTime=BeginTime;m_tContinueTime=ContinueTime;}	//���ó���ʱ��
	int GetContinueTime() const;
	//�õ�ʣ�µĳ���ʱ��
	time_t GetBeginTime(){return m_tBeginTime;}
	bool isValidObj(){ return m_OwnerMap!=NULL;}
	emMapEventType m_emEventType;		//��ͼ�¼�����
	emOwnerType m_emOwnerType;			//��������(ʲô����������)
	emDisappearType m_emDisappearType;	//��ʧ����
	time_t m_tBeginTime;			//��ʼʱ��
	time_t m_tContinueTime;			//����ʱ��
	CGameMap* m_OwnerMap;			//������ͼ
	DWORD m_dwNextRunTick;			//�´�RUNʱ��
	int m_dwIntervalTick;			//���ʱ��
	uint64_t next_check_tick = 0;

};
