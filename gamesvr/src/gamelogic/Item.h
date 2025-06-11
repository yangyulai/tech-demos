#pragma once

#include "LocalDB.h"
#include "..\gamesvr.h"
#include "lua_base.h"
#include "CMD\Item_cmd.h"
class CCreature;

enum emCreateType
{
	_CREATE_MON_DROP,  //掉落的
	_CREATE_NPC_BUY,   //购买的
	_CREATE_NPC_UPGRAGE, //属性被更改的
	_CREATE_MALL_BUY,	//商城购买的
	_CREATE_NPC_CHEST,	//宝箱所需的
	_CREATE_EXCHANGE,	//直接兑换
	_CREATE_MAIL,		//邮件
	_CREATE_MAX,
};

class CItem
{
public:
	CItem();
	static int64_t GenerateItemID();	//生成物品的id
	static int64_t GenerateVirtualItemID();
	static CItem* CreateItem(DWORD dwId,emCreateType btCreateType,DWORD dwCount,DWORD dwMonType=0,const char* szMakeFrom="",int frommapid=0,const char* bornfrom="",const char *szmaker="");	//创建物品
	static CItem* LoadItem(stItem *pstItem,const char* szMakeFrom="");			//从人物数据库获取物品
	static DWORD	GetMaxDbCount(DWORD dwBaseId);
	static const int64_t strToi642(const char* luatmpid);
	bool   LoadItem();
	void	SetSpecialProperty();
	void	SetNpProperty();		//创建物品随机产生属性
	void    RandomProperty(emCreateType btCreateType,DWORD dwMonType);							//随机获得物品属性
	bool    GetItemProperty(CPlayerObj* pPlayer, int iPos, stARpgAbility& pattr);		//获得物品属性 返回到数组中
	bool    GetItemSpecialProperty(CPlayerObj* pPlayer, int iPos, stSpecialAbility* pattr);		//获得物品特殊属性 返回到数组中
	BYTE    GetWeaponJob();
	bool    SaveEquipRndAtt(std::vector<stEquipAtt> weaponAtt,int random = 0);
	bool	GetCanUseTick() const	{return (GetTickCount64()>=m_dwNextCanUseTick);}
	void	SetCanUseTick(DWORD dwTick=0);
	bool	GetOutLock()	{return m_boOutLock;}
	int64_t GetItemID()		{return m_Item.i64ItemID;} //获得物品id
	double  LuaGetItemI64Id() {return (double&)m_Item.i64ItemID;}
	DWORD	GetItemCount() const	{return m_Item.dwCount;}	//获取物品数量
	DWORD   GetItemBaseID() const	{return m_Item.dwBaseID;}  //获得物品基本ID
	const char*   GetItemName() const;   //获得物品名称
	bool    GetIdent()		{return true;}   //获得是否鉴定
	int     GetDura() const { return m_Item.nDura; }     //获得持久
	int     GetPreventDropCount() const { return m_Item.nPreventDropCount; }     //获得物品幸运次数
	BYTE    GetBinding() const    {return m_Item.dwBinding;}
	void    SetBinding(BYTE btBinding);
	void    SetDura(int nDura);
	void    SetPreventDropCount(int nCount);
	int     GetMaxDura() const   {return m_Item.nMaxDura;}
	void	SetMaxDura(int nMaxDura);
	DWORD   GetBuyPrice();
	DWORD   GetSellPrice();
	void    SetItemCount(int nCount);
	void    SetItemLocation(BYTE nCount);
	BYTE   GetItemLocation();
	DWORD   GetMaxCount() const;
	DWORD	GetDisappearTime();
	int		GetFaceId() const;
	int		GetDataStNum();
	emTypeDef GetType()	const;
	BYTE	GetSex() const;
	int		GetLevelOrder() const;
	BYTE	GetGuardType() const;
	bool	CanDestory();
	int		GetRare() const;
	BYTE	GetEquipStation() const;
	DWORD	GetWearLevel();     //获得佩戴等级
	DWORD	GetFixCost();     //获取装备维修费用 （损坏）
	DWORD	GetBaseFixCost(); //获取装备基础修理费用 （耐久）
	bool    GetCanBuy(){return true;}           //获取是否能买
	bool    GetCanRepair(){return true;}        //获取是否能修理
	bool    GetCanTrade(){if (GetBinding() >0) return false; else return true;}         //获取是否能交易
	void    SetBornFrom(const char *bornfrom);//设置物品来源
	const char* GetBornFrom();
	void SetBornTime(DWORD borntime);
	DWORD GetBornTime();
	void    Repair();
	bool	SaveLuaData(DWORD pos,DWORD data);
	bool	LoadLuaData(sol::table table);
	bool	SaveNpData(BYTE btFrom,BYTE bttype,int npnum, BYTE btLv = 0, BYTE btMaxLv = 0);
	bool	LoadNpData(sol::table table);
	sol::table	LoadNpDataByFrom(BYTE btFrom,sol::state_view ts);
	bool    GetExistNpData();
	bool	ClearNpData();
	bool    ClearNpByFromType(BYTE btFrom,BYTE bttype);
	bool    ClearNpByFrom(BYTE btFrom);
	bool	ClearNpByType(BYTE bttype);
	bool	AddNpData(BYTE btPos,BYTE bttype,int npnum,BYTE btLv = 0,BYTE btMaxLv = 0);
	int 	CheckNpRepeat(BYTE bttype);
	bool	DelNpDataByPos(BYTE btPos);
	bool	ChangeNpData(BYTE btFrom,BYTE bttype,int npnum,BYTE btLv = 0,BYTE btMaxLv = 0);
	bool	ChangeNpMaxLv(BYTE btFrom,BYTE bttype,BYTE btMaxLv = 0);
	int		GetnValue(){return m_Item.nValue;}
	void	SetnValue(int nValue){ m_Item.nValue = nValue; }
	int		GetMaxValue(){return m_Item.nMaxValue;}
	void	SetMaxValue(int nMaxValue) { m_Item.nMaxValue = nMaxValue; }
	DWORD	GetLimitTime(){return m_Item.dwExpireTime;}
	bool	IfLimitedTime();//物品是否逾期,逾期则不能使用,逾期返回true
	BYTE	GetLevel(){return m_Item.dwLevel;}
	void	SetLevel(DWORD level){ m_Item.dwLevel = level; }
	BYTE    GetStrengCount() {return m_Item.btStrengCount;}
	void    SetStrengCount(BYTE count) {m_Item.btStrengCount = count;}
	BYTE    GetStrengCountMech() { return m_Item.btStrengCountMech; }
	void    SetStrengCountMech(BYTE count) { m_Item.btStrengCountMech = count; }
	BYTE    GetGuardLv() { return m_Item.btGuardLv; }
	void    SetGuardLv(BYTE level) { m_Item.btGuardLv = level; }
	BYTE    GetGuardEvolveLv() { return m_Item.btGuardEvolveLv; }
	void    SetGuardEvolveLv(BYTE level) { m_Item.btGuardEvolveLv = level; }
	int	    GetEffId() { return m_Item.dwEffId;}//
	void	SetEffId(int dwEffId) { m_Item.dwEffId = dwEffId;}
	int		GetRefineCnt() { return m_Item.btRefineCnt;}
	void	SetRefineCnt(BYTE btRefineCnt) { m_Item.btRefineCnt = btRefineCnt;}
	int		GetdwEffIdEx() { return m_Item.dwEffIdEx;}
	void	SetdwEffIdEx(int dwEffIdEx) { m_Item.dwEffIdEx = dwEffIdEx;}
	DWORD	GetNextId();
	const char*   getBase64String(){return toBase64String(m_Item,GetItemName());}
	void SetLimitTime(DWORD dwTime){ m_Item.dwExpireTime =  dwTime;	};
	void    SetDropProtectCount(int nCount);
	int   GetDropProtectCount() const { return m_Item.nDropProtectCount; };     //获得投保次数
	void   SetExtraStrenghLvl(BYTE btLvl);
	BYTE   GetExtraStrenghLvl() const { return m_Item.btStrenghAdd; };
	void   SetBroken(BYTE btLv);
	BYTE   GetBroken() const { return m_Item.btBroken; };			// 获得物品损坏状态
	void   SetPrefix(BYTE btLv);
	BYTE   GetPrefix() const { return m_Item.btPrefix; };			// 获得装备前缀
	bool   CheckJobType(BYTE btJob = 0);							//职业判断
	bool   IsCurrency();							                //是否是货币
	bool   IsMechBody();
	const char* LuaGetItemName() const;
	const int GetItemNpPropertyByType(int nType);
	const char*	GetItemLuaId() const;;
	void	GetTrueItem()	{ m_Item.i64ItemID=GenerateItemID(); }	//把虚拟物品变成一个真实的物品
	const char* GetMakerName() const {return m_Item.szMaker;}
	void	SetMakerName(const char* szNewMaker);
	void	SetOutLock(bool b);
	void	SetItemLog(const char* szText, CCreature * pSrcCret,char* pszDestName,int nDestLvl = 0,bool boForceRecord = false,int UseCount=0,DWORD dwPrice=0,Byte btPriceType=0);
	void	SetItemLog(const char* szText,CCreature * pSrcCret,CCreature * pDestCret = NULL,bool boForceRecord = false,int UseCount=0,DWORD dwPrice=0,Byte btPriceType=0);
	void	LuaSetItemLog(const char* szText,CCreature * pSrcCret,CCreature * pDestCret, bool boForceRecord, int UseCount);
	bool	CanLog();
	BYTE	NoticeDay();
	int     GetLocation() { return m_Item.Location.btLocation; }
	int     GetIndex() { return m_Item.Location.btIndex; }
	DWORD   GetMechEffid() { return m_Item.mechEnhanceEffect.effid; }
	void	SetMechEffid(DWORD dwEffId) { m_Item.mechEnhanceEffect.effid = dwEffId; }
	DWORD   GetMechSpecialEffid() { return m_Item.mechEnhanceEffect.specialEffid; }
	void	SetMechSpecialEffid(DWORD dwEffId) { m_Item.mechEnhanceEffect.specialEffid = dwEffId; }
	DWORD   GetMechExtraEffid() { return m_Item.dwEffIdEx; }
	void	SetMechExtraEffid(DWORD dwEffId) { m_Item.dwEffIdEx = dwEffId; }
	void GetItemNPLog(char* strlog,int nsize);
	int  GetSuitId();
	int  GetSuitType();
	stARpgAbility GetNpPropertyTrans();
	stARpgAbility GetLuaDataTrans() const;
	std::shared_ptr<stItemDataBase> GetItemDataBase() const;
	std::shared_ptr<stEffectDataLoaderBase> GetEffectDataBase() const;
public:
	stItem m_Item;	//物品
	mutable std::weak_ptr<stItemDataBase> m_itemDataBase;
	bool m_boOutLock;							//出售,给予等锁定
	ULONGLONG m_dwNextCanUseTick;					//下次可以使用的时间
};


