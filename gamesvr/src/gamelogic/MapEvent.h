#pragma once
#include "MapObject.h"

enum emMapEventType{
	MAPEVENT_NULL,					//没有事件
	MAPEVENT_ITEM,					//物品类型
	MAPEVENT_MAGIC,					//魔法类型
};

enum emDisappearType{
	DISAPPEAR_TIME,					//时间到了消失
	DISAPPEAR_ADD,					//物品捡起消失
};

enum emOwnerType{
	OWNER_NULL,						//没有所属
	OWNER_MONSTER,					//怪物所属
	OWNER_NPC,						//NPC所属
	OWNER_PLAYER,					//玩家所属
	OWNER_PET,						//宠物所属
};

enum emMagicType{
	MAGIC_NULL,						//没有魔法类型
	MAGIC_FIRE,						//火墙
	MAGIC_AIR,						//风墙
	MAGIC_FIRERAIN,					//火雨
	MAGIC_TIANHUO,					//天火
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
	emMapEventType GetEventType() {return m_emEventType;}	//得到事件类型
	emOwnerType GetOwnerType() {return m_emOwnerType;}	//得到所属类型
	emDisappearType GetDisappearType() {return m_emDisappearType;}	//得到消失类型
	void SetDisappearType(emDisappearType DisappearType) {m_emDisappearType=DisappearType;}	//设置消失类型
	bool isCanContinue() const;
	//魔法是否还持续
	bool isCanBegin() const;
	//是否能开始RUN
	void SetTime(time_t BeginTime,time_t ContinueTime) {m_tBeginTime=BeginTime;m_tContinueTime=ContinueTime;}	//设置持续时间
	int GetContinueTime() const;
	//得到剩下的持续时间
	time_t GetBeginTime(){return m_tBeginTime;}
	bool isValidObj(){ return m_OwnerMap!=NULL;}
	emMapEventType m_emEventType;		//地图事件类型
	emOwnerType m_emOwnerType;			//所属类型(什么东西产生的)
	emDisappearType m_emDisappearType;	//消失类型
	time_t m_tBeginTime;			//开始时间
	time_t m_tContinueTime;			//持续时间
	CGameMap* m_OwnerMap;			//所属地图
	DWORD m_dwNextRunTick;			//下次RUN时间
	int m_dwIntervalTick;			//间隔时间
	uint64_t next_check_tick = 0;

};
