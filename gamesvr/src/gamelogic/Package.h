#pragma once

#include "Item.h"
#include "cmd\Package_cmd.h"

#pragma pack(push,1)
#pragma pack(pop) 

class CPackage
{
public:
	std::unordered_map<int64_t,CItem*> m_PackItemList;
	std::unordered_map<uint32_t, std::vector<CItem*>> m_itemsByBaseId;
	std::vector<bool> m_vUsed;
	DWORD   m_dwMaxCount;
	int   m_nIndex;   //编号
	int   m_nType;    //类型
	bool  m_boOpened;
	CPlayerObj* m_PlayerObj;
	CPackage(){
		m_dwMaxCount=0;
		m_nIndex=0;   //编号
		m_nType=ITEMCELLTYPE_NONE;    //类型
		m_boOpened=false;
		m_PlayerObj= NULL;
	};
	~CPackage(){
		Clear();
	};
	void Init(CPlayerObj* pPlayerObj, int nType,int nIndex,int nMaxCount ,bool boMissionItemOnly= false,bool boFightItemOnly = false)
	{
		m_PlayerObj=pPlayerObj;
		m_boOpened = true;
		m_nType = nType;
		m_nIndex = nIndex;
		m_dwMaxCount =nMaxCount;
		m_vUsed.clear();
		m_vUsed.resize(m_dwMaxCount);
	}

public:
	bool AddItemToPack(CItem *pItem,DWORD nIndex,bool boSendMsg= true,bool boAutoMerge =true, bool boSendNewMsg= false,const char* pszLogStr = NULL,CCreature * pDestCret = NULL,DWORD dwPrice=0,Byte btPriceType=0);   //添加到包裹中
	bool RemoveFromPack(int nBaseID,int64_t i64Id,int nLoc = -1,bool boSend=false);  //从包裹中移出
	bool DeleteFromPack(int nBaseID,int64_t i64Id,const char* pszLogStr = NULL);  //从包裹中移出并且删除该物品
	bool DeleteFromHpMap(CItem *pItem);	//从回血Map中移出
	bool FindFreeSpace(int nIndex,int& nRetIdX);
	CItem* FindItem(int64_t i64ID);     //在包裹里找物品
	CItem* FindItemByLocation(int nIndex);
	CItem* FindItemByBaseID(DWORD dwBaseId);    //根据物品基本ID找物品
	CItem* FindItemByPos(int nPos);    //根据装备pos找物品
	void Clear();
	void SendDeleteItem(int64_t i64ID,DWORD dwBaseID);
	void SendUpdateandAddItem(CItem* pItem);
	void SendGetItem(stItem item);
	void SendBagItemCountChanged(CItem* pItem);
	void ChangeBagCellCount(int nNewBagCount);
	void ChangeStorageCellCount(int nNewBagCount);
	[[nodiscard]] const std::vector<CItem*>* FindItemsByBaseID(uint32_t dwBaseId) const {
		auto it = m_itemsByBaseId.find(dwBaseId);
		return (it != m_itemsByBaseId.end()) ? &it->second : nullptr;
	}
};

class CPlayerPackage
{
public:
	CPackage m_StoragePacket[_MAX_STORAGE_COUNT];		//仓库物品类
	CPackage m_BagPacket[_MAX_BAG_COUNT];				//包裹物品类
	CPackage m_TmpBagPacket[_MAX_TMPBAG_COUNT];         //临时背包
	CItem* m_stEquip[EQUIP_MAX_COUNT];					//身上装备
	std::list<CItem*> m_vHpItemMap;						//回血物品Map
	std::list<CItem*> m_LastSellItems;
	std::list<CItem*> m_ActBag;							//活动临时包裹
	CPlayerObj* m_PlayerObj;							//包裹归属的角色
	int  m_nOpenBagCount;								//打开的包裹数量（页）      存档
	int  m_nOpenStorageCount;							//打开的仓库数量（页）      存档
	int  m_nOpenTmpBagCount;                            //临时背包数
	bool m_boLock;										//是否锁定					存档
	DWORD m_dwLastResortBagTime;						//上次重新排列包裹的时间    存档
	DWORD m_dwLastResortSrotageTime;					//上次重新排列仓库的时间    存档
	//ULONGLONG m_dwLastDrugHpTick;							//喝血公共CD时间
	//ULONGLONG m_dwLastDrugMpTick;							//喝蓝公共CD时间
	ULONGLONG m_dwLastDrugShareTick;						//统一药品公共CD时间

	int m_nOpenBagCellCount;								//背包格子数量  存档
	int m_nOpenStorageCellCount;							//仓库格子数量  存档
private:
	bool m_boSendStoreItems;
public:
	CPlayerPackage();
	~CPlayerPackage();
	void Init(CPlayerObj* player);
	bool doCretCmd(stBaseCmd* pcmd,int ncmdlen);

	void SendSoldItems();//发送已卖道具
	void SendBagItems();								//发送包裹物品
	void SendBagAddAndUpdateItem(CItem* pItem);			//包裹物品增加和更新
	void SendBagDeleteItem(int64_t i64ID,DWORD dwBaseID);				//包裹物品减少

	void SendTmpBagItems();
	void SendTmpBagAddAndUpdateItem(CItem *pItem);
	void SendTmpBagDeleteItem(int64_t i64ID, DWORD dwBaseID);

	void SendBagItemCountChanged(CItem* pItem);
	void SendStorageAddAndUpdateItem(CItem* pItem);		//仓库物品增加和更新
	void SendStorageDeleteItem(int64_t i64ID);			//仓库物品减少
	
	void SendStorageItems();							//发送仓库物品	
	void SendEquipItems();								//发送身上装备
	void SendEquipAddAndUpdateItem(CItem* pItem);		//更新身上的装备
	void SendEquipDeleteItem(int64_t i64ID);			//删除身上的装备
	void SendEquipDuraChanged(CItem* pItem);
	int ServerGetGetBack(int64_t i64Id,stItemLocation &srcLocation,stItemLocation &destLocation,bool boForce = false);		//从仓库取回指定ID的物品,返回操作信息,并且从仓库中移除该物品,添加到包裹中
	int ServerGetStorage(int64_t i64Id,stItemLocation &srcLocation,stItemLocation &destLocation,bool boForce = false);	//存储指定ID的物品,返回操作信息,直接从包裹转移到仓库中
	int ServerBagItemToTmpBag(int64_t i64Id, stItemLocation &srcLocation, stItemLocation &destLocation, bool boForce = false); //背包物品进临时背包
	int ServerTmpBagItemToBag(int64_t i64Id, stItemLocation &srcLocation, stItemLocation &destLocation, bool boForce = false); //临时背包物品进背包
	int getRealEquipStation(int nPos);
	bool isNeedDeleteSkill(int nPos,int nId);
	bool ServerGetWearItem( int64_t i64SrcId,int nPos,bool boLuaWear=false,CItem* pLuaItem = NULL);                     //佩戴装备
	int ServerGetWearItem_PanDuan(CItem*pItem,int nPos,bool boLuaWear);
	int ServerGetTakeOffItem(int64_t i64SrcId, stItemLocation& srcLocation, stItemLocation& destLocation, bool boForce = false);          //摘下装备

	bool ServerGetSplitItem(int64_t i64Id,DWORD nCount);                   //拆分
	void ServerGetUseItem(int64_t i64Id,DWORD dwCretTmpId,DWORD dwTmepID);  //使用物品
	void ServerGetDestoryItem(int64_t i64Id);                              //分解物品
	void ServerGetExchangeItem(int64_t i64Id,stItemLocation location);
	void ServerGetExchangeItemEx(CItem* pItem,BYTE btLocation,BYTE btTableID,BYTE  btIndex);

	void ServerGetResortBag();      									   //重新整理背包
	void ServerGetResortStorage(BYTE btTabId);                             //重新整理仓库
	void ServerGetOpenNewBagPage();                                        //打开新的包裹
	void ServerGetOpenNewStoragePage();                                    //打开新的仓库
	void OpenNewBagPage(int nOpenBagCount);									//打开新的包裹
	void OpenNewStoragePage(int nStrorageCount);							//打开新的仓库
	//DWORD GetRepairAllPrices();											   //获取全身修理需要的金币数量
	void ServerGetRepairAll();											   //修理全身装备
	//////////////////////////////
	void ServerAutoKeepHp(DWORD dwKeepHp,DWORD dwForceKeepHp);				//自动回血
	//////////////////////////////////////////////////////////////////////////
	void ServerGetViewEuqip(char* szName,BYTE btType,BYTE btFlag, int64_t i64Id = 0);
	DWORD  GetFreeBagCellCount(DWORD dwType);                                //获得包裹剩余的格子数量 参数包括 CELLCOUNT_NORMAL  CELLCOUNT_MISSION CELLCOUNT_FIGHT
	DWORD  GetFreeTmpBagCellCount(DWORD dwType);
	DWORD  GetFreeSorageCellCount();                                         //获取仓库剩余格子数量
	CItem* FindItemInBagByBaseID(DWORD baseid, DWORD nCount = 1);
	CItem* FindItemInBag(int64_t i64Id,DWORD nCount = 1);					  //寻找包裹中的物品
	CItem* FindItemInTmpBag(int64_t i64Id, DWORD nCount = 1);                   //临时背包寻找物品
	bool FindItemInBagAllByBaseId(DWORD baseid,DWORD nCount);
	bool FindItemInBagAllByBaseIdWithBindSta(DWORD baseid,DWORD nCount,BYTE btBind); // 检查背包里的绑定/非绑定物品是否足够
	DWORD GetItemCountInBodyAllByBaseId(DWORD baseid);
	DWORD GetItemCountInBagAllByBaseId(DWORD baseid);
	DWORD GetItemCountInStoreAllByBaseId(DWORD baseid);
	bool FindItemInBagAllByType(DWORD itemtype);							//查找是否有某种类型的物品
	
	bool   AddItemToBag(CItem* pItem,bool boSendNewMsg=false,bool boQuestItem=false,bool boSendMsg = true,const char* pszLogStr = NULL,CCreature * pDestCret = NULL,DWORD dwPrice=0,Byte btPriceType=0);					  //将物品添加到包裹中 如果nCount =0xffffffff 则按物品数据的数量操作
	bool   AddItemToTmpBag(CItem* pItem, bool boSendNewMsg = false, bool boQuestItem = false, bool boSendMsg = true, const char* pszLogStr = NULL, CCreature * pDestCret = NULL, DWORD dwPrice = 0, Byte btPriceType = 0);
	bool   CheckCanAddToBag(CItem* pItem);
	bool   CheckCanAddToTmpBag(CItem *pItem);
	bool   RemoveItemFromBag(CItem* pItem, DWORD nCount = 0xffffffff, bool boAutoMerge = true, bool boSendMsg = false);//移出包裹中的物品 如果nCount = -1 则按物品数据的数量操作 pNewItem如果不是NULL 则返回移出的新物品指针
	bool   RemoveItemFromTmpBag(CItem* pItem,DWORD nCount=0xffffffff,bool boAutoMerge = true,bool boSendMsg = false);//移出包裹中的物品 如果nCount = -1 则按物品数据的数量操作 pNewItem如果不是NULL 则返回移出的新物品指针
	bool   DeleteItemInBag(CItem* pItem,DWORD nCount=0xffffffff,bool boSendMsg = false,bool boNoScrpitEvent = false,const char* pszLogStr = NULL,CCreature * pDescCret = NULL);					  //删除包裹中的物品  如果nCount= -1 则按物品数据的数量操作 
	bool   DeleteItemInTmpBag(CItem* pItem, DWORD nCount = 0xffffffff, bool boSendMsg = false, bool boNoScrpitEvent = false, const char* pszLogStr = NULL, CCreature * pDescCret = NULL);
	bool   DeleteItemInBodyAllByBaseId(DWORD dwBaseId,DWORD num,const char* pszLogStr = NULL);
	bool   DeleteItemTmpId(int64_t tmpid,DWORD nCount=1,const char* pszLogStr = NULL);						//删除物品用临时ID
	bool   DeleteItemInBagAllByBaseId(DWORD dwBaseId,DWORD num,const char* pszLogStr = NULL);
	DWORD   DeleteAllItemInBagByBaseId(DWORD dwBaseId,const char* pszLogStr = NULL);
	DWORD  GetItemCountByBindType(DWORD dwBaseId, BYTE btBindType);
	DWORD  DelAllItemInBagByBindType(DWORD dwBaseId, DWORD dwLimitNum,BYTE btBindType, const char* pszLogStr = NULL);
	bool   AddItemToStorage(CItem* pItem,DWORD nCount = 0xffffffff);
	CItem* FindItemInStorage(int64_t i64Id);							  //寻找仓库中的物品
	bool   DeleteItemInStorage(CItem* pItem,const char* pszLogStr = NULL);							  //删除仓库中的物品

	CItem* FindItemInBody(int64_t i64Id);
	CItem* FindItemInBodyby64(const char* luatmpid );
	bool   DeleteItemInBody(CItem* pItem,const char* pszLogStr = NULL);								  //删除身上的装备
	bool   RemoveItemFromBody(CItem* pItem);							  //移除身上的装备

	void GetPlayerEquipProper(stARpgAbility* pattr);                     //获得装备总属性
	void GetPlayerEquipSpecialProper(stSpecialAbility* pattr);           //获得装备总特殊属性

	CItem* GetItemMsg(int64_t i64Id);									//获取物品信息
	bool saveBagPacket(char* dest,DWORD& retlen);
	bool saveStoragePacket(char* dest,DWORD& retlen);
	bool saveEquip(char* dest,DWORD& retlen);
	bool saveTmpBagPacket(char *dest, DWORD& retlen);
	bool load(const char* dest,int retlen,int nver);					//读档
	bool RandomDropItem(CItem* pItem,DWORD dwCount,int nInRange=1,int nOutRange=9,const char* szLog="");	//随机掉落一件物品(死亡掉落)
	sol::table GetItemTableByLocation(emItemCellType CellType,sol::this_state ts);	//根据物品的位置类型找出该类型的所有物品，并返回表
	bool GetItemTabByLocWithType(sol::table& table, emItemCellType CellType, sol::table& typetab);	//根据物品的位置类型找出该类型物品，并返回表
	void initOnePackage(int nId);
	CItem* FindFirstItemInBagByBaseId(DWORD dwBaseId);
	CItem* FindFirstItemInBodyByBaseId(DWORD dwBaseId);//通过BASIID在身上寻找物品
	CItem* FindFirstItemInStorageByBaseId(DWORD dwBaseId);//根据ID在仓库里寻找东西
	CItem* FindItemInBagById64(const char* luatmpid);
	CItem* FindItemInTmpBagById64(const char* luatmpid);
	bool   DeleteItemInBagAndBodyAllByBaseId(DWORD dwBaseId,DWORD num,const char* pszLogStr = NULL);

	sol::table FindAllItemInBagByBaseId(DWORD dwBaseId,sol::this_state ts);  //通过baseid寻找背包里所有相同id的物品
	//========================================================================================
	int getBagCellCount();
	void setBagCellCount(int nBagCellcount);
	int getStorageCellCount();
	void setStorageCellCount(int nBagCellcount);
	CItem* FindItemInBagByPos(int nPos);
	CItem* FindItemInBodyByPos(int nPos);
	
	bool AddItemToActBag(CItem* pItem);
	void SendActBagUpdateItem(CItem* pItem);			//包裹物品增加和更新
	bool DeleteItemInActBag(CItem* pItem , DWORD dwCount);
	bool SendActBag();
	void SendActBagDeleteItem(int64_t i64ID, DWORD dwBaseID);
	void ClearActBag();
	CItem* FindItemInActBag(int64_t i64Id);
};











