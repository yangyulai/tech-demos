#include "Package.h"
#include "PlayerObj.h"
#include "Chat.h"
#include "timemonitor.h"
#include "MapMagicEvent.h"
#include "MapItemEvent.h"
#include "GameMap.h"
//////////////////////////////////////////////////////////////////////////

int64_t hex2i64(const char* luatmpid){
	char szh[16]={0};
	const char* ph=strchr(luatmpid,'_');
	if(ph==NULL){
		return strtoul(luatmpid,NULL,16);
	}else{
		strncpy( szh,luatmpid,min(ph-luatmpid,8) );
		ph++;
		return ( strtoul(szh,NULL,16) | ( ((int64_t)strtoul(ph,NULL,16)) << 32) );
	}
}

bool CmpByType(const CItem* a, const CItem* b) {
	auto itemBaseA = a->GetItemDataBase();
	auto itemBaseB = b->GetItemDataBase();
	return itemBaseA->dwType < itemBaseB->dwType;
}

bool CmpByBaseId(const CItem* a, const CItem* b) { 
	return (BYTE)a->m_Item.dwBaseID > (BYTE)b->m_Item.dwBaseID;	//降序 321
}

bool CmpBySortType(const CItem* a, const CItem* b) {
	auto itemBaseA = a->GetItemDataBase();
	auto itemBaseB = b->GetItemDataBase();
	if(itemBaseA->nSortType < itemBaseA->nSortType){
		return true;
	}else if(itemBaseA->nSortType == itemBaseA->nSortType){
		if(itemBaseA->nSortType == 1){
			if(itemBaseA->btEquipStation < itemBaseA->btEquipStation){
				return true;
			}else if(itemBaseA->btEquipStation == itemBaseA->btEquipStation){
				if(itemBaseA->nRare > itemBaseA->nRare){
					return true;
				}else if(itemBaseA->nRare == itemBaseA->nRare){
					if(itemBaseA->dwNeedLevel < itemBaseA->dwNeedLevel){
						return true;
					}else{
						return (itemBaseA->dwNeedLevel == itemBaseA->dwNeedLevel) && (itemBaseA->nID < itemBaseA->nID);
					}
				}
			}
		}else{
			if(itemBaseA->nSubSortType < itemBaseA->nSubSortType){
				return true;
			}else{
				return (itemBaseA->nSubSortType == itemBaseA->nSubSortType) && (itemBaseA->nID < itemBaseA->nID);
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

bool  CPackage::FindFreeSpace(int nIndex,int& nRetIdX)
{
	FUNCTION_BEGIN;

	if (nIndex>=0 && nIndex<(int)m_vUsed.size() && !m_vUsed[nIndex]){
		nRetIdX = nIndex;
		return true;
	}else {
		for (DWORD i=0;i<m_dwMaxCount;i++){
			if (!m_vUsed[i]){
				nRetIdX = i;
				return true;
			}
		}
	}
	return false;
}

void CPackage::SendDeleteItem(int64_t i64ID,DWORD dwBaseID)
{
	FUNCTION_BEGIN;
	stCretDeleteItem delitem;
	delitem.btPosition = m_nType;
	delitem.i64Id =  i64ID;
	m_PlayerObj->SendMsgToMe(&delitem,sizeof(delitem)); 
}

void CPackage::SendUpdateandAddItem(CItem* pItem)
{
	FUNCTION_BEGIN;
	stCretUpdateItem item;
	item.btPosition = m_nType;
	if (pItem) item.item = pItem->m_Item;
	m_PlayerObj->SendMsgToMe(&item,sizeof(item)); 
}

void CPackage::SendBagItemCountChanged(CItem* pItem){
	FUNCTION_BEGIN;
	stCretItemCountChanged itemcount;
	itemcount.btPosition = pItem->m_Item.Location.btLocation;
	itemcount.itemid = pItem->m_Item.i64ItemID;
	itemcount.dwCount = pItem->GetItemCount();
	m_PlayerObj->SendMsgToMe(&itemcount,sizeof(itemcount)); 
}

void CPackage::SendGetItem(stItem item){
	FUNCTION_BEGIN;
	stCretAddItemMsg itemmsg;
	itemmsg.item = item;
	m_PlayerObj->SendMsgToMe(&itemmsg,sizeof(itemmsg)); 
}



bool CPackage::AddItemToPack(CItem *pItem,DWORD nIndex,bool boSendMsg,bool boAutoMerge,bool boSendNewMsg,const char* pszLogStr,CCreature * pDestCret,DWORD dwPrice,Byte btPriceType)
{
	FUNCTION_BEGIN;
	if (nIndex<0 || nIndex>=m_dwMaxCount) nIndex=0;
	if (FindItem(pItem->m_Item.i64ItemID))
	{
		return false;
	}
	DWORD dwCount = pItem->GetItemCount();
	stItem newitem = pItem->m_Item;
	if(pItem)
	{
		auto itemBase = pItem->GetItemDataBase();
		if (itemBase && boAutoMerge && !pItem->GetOutLock())
		{
			DWORD ItemMaxCnt= itemBase->dwMaxCount;
			int VariableMaxCnt= itemBase->nVariableMaxCount;
			if(VariableMaxCnt>0){
                bool IsNeedChangeMaxCnt=CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("itemidchangemaxcnt",true,m_PlayerObj);
				if(IsNeedChangeMaxCnt){
					ItemMaxCnt=VariableMaxCnt;
				}
			}
			if (ItemMaxCnt>1 && pItem->m_Item.dwCount<ItemMaxCnt)
			{
				if (auto items = FindItemsByBaseID(pItem->GetItemBaseID()))
				{
					for (auto item:*items)
					{
						if (!item->GetOutLock() && item->m_Item.dwBinding == pItem->m_Item.dwBinding && item->m_Item.btQuality == pItem->m_Item.btQuality && pItem->m_Item.dwExpireTime == item->m_Item.dwExpireTime)
						{
							if (item->m_Item.dwCount + pItem->m_Item.dwCount <= ItemMaxCnt)
							{
								item->m_Item.dwCount += pItem->m_Item.dwCount;

								SendBagItemCountChanged(item);
								SendDeleteItem(pItem->m_Item.i64ItemID, pItem->m_Item.dwBaseID);
								DeleteFromHpMap(pItem);
								if (m_nType == ITEMCELLTYPE_PACKAGE)
									m_PlayerObj->TriggerEvent(m_PlayerObj, ITEMGET, pItem->GetItemBaseID());
								if (pszLogStr) {
									pItem->SetItemLog(vformat("%s%s", pszLogStr, "-mergedel"), m_PlayerObj, pDestCret, false, pItem->GetItemCount(), dwPrice, btPriceType);
									item->SetItemLog(vformat("%s%s", pszLogStr, "-mergeadd"), m_PlayerObj, pDestCret, false, pItem->GetItemCount(), dwPrice, btPriceType);
								}

								if (pItem->m_Item.dwExpireTime > 0)
								{
									m_PlayerObj->set_LimitItemPacket.erase(pItem);
								}
								CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);

								return true;
							}
							else if (ItemMaxCnt > item->m_Item.dwCount)
							{
								int nAddCount = ItemMaxCnt - item->m_Item.dwCount;
								pItem->m_Item.dwCount -= nAddCount;
								item->m_Item.dwCount = ItemMaxCnt;
								SendBagItemCountChanged(item);
								SendBagItemCountChanged(pItem);
								if (pszLogStr) {
									item->SetItemLog(vformat("%s%s", pszLogStr, "-mergeadd"), m_PlayerObj, pDestCret, false, nAddCount, dwPrice, btPriceType);
								}
							}
						}
					}
				}
			}
		}
		int nRetX;
		if (FindFreeSpace(nIndex,nRetX))
		{
			pItem->m_Item.Location.btIndex = nRetX;
			pItem->m_Item.Location.btLocation = m_nType;
			pItem->m_Item.Location.btTableID = m_nIndex;
			m_PackItemList.emplace(pItem->GetItemID(),pItem);
			m_itemsByBaseId[pItem->GetItemBaseID()].push_back(pItem);
			m_vUsed[pItem->m_Item.Location.btIndex] = true;
			if(pItem->m_Item.dwExpireTime>0) //限时物品插入
			{
				m_PlayerObj->set_LimitItemPacket.insert( pItem);
			}
			if (boSendMsg)
			{
				SendUpdateandAddItem(pItem);
			}

			if (m_nType == ITEMCELLTYPE_PACKAGE)
				m_PlayerObj->TriggerEvent(m_PlayerObj,ITEMGET, pItem->GetItemBaseID());
			if(pszLogStr)  pItem->SetItemLog(pszLogStr,m_PlayerObj,pDestCret,false,pItem->GetItemCount(),dwPrice,btPriceType);
			return true;
		}
	}
	return false;
}

bool CPackage::RemoveFromPack(int nBaseID,int64_t i64Id,int nLoc ,bool boSend)
{
	FUNCTION_BEGIN;
	if(nLoc <(int)m_vUsed.size()){
		m_vUsed[nLoc] = false;
	}

	if(CItem* pItem = FindItem(i64Id)){
		m_PackItemList.erase(i64Id);
		auto it = m_itemsByBaseId.find(nBaseID);
		if (it!=m_itemsByBaseId.end())
		{
			auto itt = std::find(it->second.begin(), it->second.end(), pItem);
			if (itt!= it->second.end())
			{
				it->second.erase(itt);
			}
		}

		DeleteFromHpMap(pItem);
		return true;
	}
	return false;
}

bool CPackage::DeleteFromPack(int nBaseID,int64_t i64Id,const char* pszLogStr)
{
	FUNCTION_BEGIN;
	if(CItem* pItem = FindItem(i64Id)){
		if(RemoveFromPack(nBaseID,i64Id,pItem->m_Item.Location.btIndex)){
			int nCount = pItem->GetItemCount();
			pItem->SetItemCount(0);
			const char* pszDelStr = vformat("delItem-%s",pszLogStr?pszLogStr:"other");
			pItem->SetItemLog((const char*)pszDelStr,m_PlayerObj,NULL,false,-nCount);
			CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);
			return true;
		}
	}
	return false;
}

bool CPackage::DeleteFromHpMap(CItem *pItem){
	FUNCTION_BEGIN;
	bool boFind=false;
	if (m_nType==ITEMCELLTYPE_PACKAGE){
		if (m_PlayerObj->m_Packet.m_vHpItemMap.size()){
			m_PlayerObj->m_Packet.m_vHpItemMap.remove(pItem);
			boFind=true;
		}
	}
	return boFind;
}

CItem* CPackage::FindItem(int64_t i64ID)
{
	auto it = m_PackItemList.find(i64ID);
	if (it == m_PackItemList.end()) {
		return nullptr;
	}

	CItem* pItem = it->second;
	if (!pItem) {
		m_PackItemList.erase(it);
		return nullptr;
	}
	if (pItem->m_Item.dwCount==0)
	{
		m_PackItemList.erase(it);
		SendDeleteItem(pItem->m_Item.i64ItemID,pItem->m_Item.dwBaseID);
		DeleteFromHpMap(pItem);
		CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);
		return nullptr;
	}
	return pItem;
}

CItem* CPackage::FindItemByLocation(int nIndex)
{
	FUNCTION_BEGIN;
	for (auto it =  m_PackItemList.begin();it != m_PackItemList.end();it ++){
		CItem* pItem= it->second;
		if (pItem && pItem->m_Item.Location.btIndex == nIndex){
			return pItem;
		}
	}
	return NULL;
}

CItem* CPackage::FindItemByBaseID(DWORD dwBaseId)
{
	auto it = m_itemsByBaseId.find(dwBaseId);
	if (it == m_itemsByBaseId.end()) return nullptr;
	return it->second.empty() ? *it->second.begin() : nullptr;
}

CItem* CPackage::FindItemByPos(int nPos)
{
	FUNCTION_BEGIN;
	for (auto it = m_PackItemList.begin(); it != m_PackItemList.end(); it++) {
		CItem* pItem = it->second;
		if (pItem && pItem->GetEquipStation() == nPos) {
			return pItem;
		}
	}
	return nullptr;
}

void  CPackage::Clear()
{
	FUNCTION_BEGIN;
	for (auto it =  m_PackItemList.begin();it != m_PackItemList.end();it ++){
		CItem* pItem= it->second;
		if (pItem){
			DeleteFromHpMap(pItem);
			CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);
		}
	}

	m_itemsByBaseId.clear();
	m_PackItemList.clear();
	m_vUsed.clear();
	m_vUsed.resize(m_dwMaxCount);
}


CPlayerPackage:: CPlayerPackage()
{
	FUNCTION_BEGIN;
	m_boSendStoreItems = false;
	m_vHpItemMap.clear();
	m_LastSellItems.clear();
	m_ActBag.clear();
	m_nOpenBagCount =1;
	m_nOpenStorageCount =1;
	m_nOpenTmpBagCount = 1;
	m_PlayerObj= NULL;
	ZeroMemory(&m_stEquip,sizeof(m_stEquip));
	m_dwLastResortBagTime=time(NULL)+5;  					//上次重新排列包裹的时间    存档
	m_dwLastResortSrotageTime=time(NULL)+10;					//上次重新排列仓库的时间    存档
	m_dwLastDrugShareTick =GetTickCount64();
	m_nOpenBagCellCount = 0;
	m_nOpenStorageCellCount = 0;
}

CPlayerPackage::~CPlayerPackage()
{
	FUNCTION_BEGIN;
	for (int i=0;i<EQUIP_MAX_COUNT;i++){
		CUserEngine::getMe().ReleasePItem(m_stEquip[i],__FUNC_LINE__);
	}
	m_vHpItemMap.clear();
	m_LastSellItems.clear();
	for (auto item : m_ActBag) {
		CUserEngine::getMe().ReleasePItem(item, __FUNC_LINE__);
	}
	m_ActBag.clear();
}

void CPlayerPackage::initOnePackage(int nId){
	FUNCTION_BEGIN;
	if(nId>0 && nId<=_MAX_STORAGE_COUNT){
		int StorageCount = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("stroagetotalcnt", _MAX_STORAGECELL_COUNT, m_PlayerObj,_MAX_STORAGECELL_COUNT);
		m_StoragePacket[nId-1].Init(m_PlayerObj,ITEMCELLTYPE_STORE,nId-1, StorageCount);
		m_nOpenStorageCount = nId;
	}
}

CItem* CPlayerPackage::FindFirstItemInBagByBaseId(DWORD dwBaseId){
	FUNCTION_BEGIN;
	for(int i=0;i<m_nOpenBagCount;i++){
		if (m_BagPacket[i].m_boOpened){			
			const auto items = m_BagPacket[i].FindItemsByBaseID(dwBaseId);
			if (!items) continue;
			for (auto& item : *items) {
				if(item){
					return item;
				}
			}
		}
	}
	return NULL;
}
CItem* CPlayerPackage::FindFirstItemInBodyByBaseId(DWORD dwBaseId){//通过BASIID在身上寻找物品
	FUNCTION_BEGIN;
	if (dwBaseId==0){return NULL;};
	for(int i=0;i<EQUIP_MAX_COUNT;i++)
	{
		CItem* pItem= m_stEquip[i];
		if(pItem  && (pItem->GetItemBaseID()==dwBaseId))
		{
			return pItem;
		}
	}
	return NULL;
}

CItem* CPlayerPackage::FindFirstItemInStorageByBaseId(DWORD dwBaseId)//根据ID在仓库里寻找东西
{
	FUNCTION_BEGIN;
	if (dwBaseId==0){return NULL;}
	for (int i=0;i<m_nOpenStorageCount;i++)
	{
		if (m_StoragePacket[i].m_boOpened)
		{
			for (auto it =  m_StoragePacket[i].m_PackItemList.begin();it != m_StoragePacket[i].m_PackItemList.end();it ++)
			{	
				CItem* pItem= it->second;
				if ( pItem && (pItem->GetItemBaseID()==dwBaseId) )
				{
					return pItem;
				}
			}
		}
	}
	return NULL;
}


//---------------------------------------------------
CItem* CPlayerPackage::FindItemInBagById64(const char* luatmpid){
	FUNCTION_BEGIN;
	int64_t tmpid=0;
	if (luatmpid && luatmpid[0]!=0){
		tmpid=CItem::strToi642(luatmpid);
		if (tmpid!=0){
			for(int i=0;i<m_nOpenBagCount;i++){
				if (m_BagPacket[i].m_boOpened){
					CItem* item=m_BagPacket[i].FindItem(tmpid);
					if(item){
						return item;
					}
				}
			}
		}
	}
	return NULL;
}

CItem* CPlayerPackage::FindItemInTmpBagById64(const char* luatmpid)
{
	FUNCTION_BEGIN;
	int64_t tmpid = 0;
	if (luatmpid && luatmpid[0] != 0) {
		tmpid = CItem::strToi642(luatmpid);
		if (tmpid != 0) {
			for (int i = 0; i < m_nOpenTmpBagCount; i++) {
				if (m_TmpBagPacket[i].m_boOpened) {
					CItem* item = m_TmpBagPacket[i].FindItem(tmpid);
					if (item) {
						return item;
					}
				}
			}
		}
	}
	return NULL;
}

void CPlayerPackage:: Init(CPlayerObj* player) 
{
	FUNCTION_BEGIN;
	m_vHpItemMap.clear();
	m_PlayerObj = player;
	m_nOpenBagCount = max(0,min(m_nOpenBagCount, _MAX_BAG_COUNT));
	for (int i=0;i<m_nOpenBagCount;i++)
	{
		int nCount = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("BagInitCellNum", _MAX_BAGCELL_COUNT, player, _MAX_BAGCELL_COUNT);
		m_BagPacket[i].Init(m_PlayerObj,ITEMCELLTYPE_PACKAGE,i, nCount);//普通物品
	}

	m_nOpenStorageCount = max(0,min(m_nOpenStorageCount,_MAX_STORAGE_COUNT));//仓库现在最多拓展到8页
	for (int i=0;i<m_nOpenStorageCount;i++)
	{
		int StorageCount = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<int>("stroagetotalcnt", _MAX_STORAGECELL_COUNT, player, _MAX_STORAGECELL_COUNT);
		m_StoragePacket[i].Init(m_PlayerObj,ITEMCELLTYPE_STORE,i, StorageCount);
	}
	for (int i = 0; i < m_nOpenTmpBagCount; i++) {
		m_TmpBagPacket[i].Init(m_PlayerObj, ITEMCELLTYPE_TMPPACKAGE, i, _MAX_TMPBAGCELL_COUNT);
	}
}

bool CPlayerPackage::doCretCmd(stBaseCmd* pcmd,int ncmdlen)
{
	FUNCTION_BEGIN;
	switch(pcmd->value)
	{
	case stCretItems::_value:
		{
			_CHECK_PACKAGE_LEN(stCretItems,ncmdlen);
			switch(((stCretItems*)pcmd)->btPosition)
			{
			case ITEMCELLTYPE_STORE:
				{	
					if(!m_boSendStoreItems){
						m_boSendStoreItems = true;
						SendStorageItems();
					}
				}break;
			case ITEMCELLTYPE_PACKAGE:
				{
					SendBagItems();
				}break;
			case ITEMCELLTYPE_EQUIP:
				{
					SendEquipItems();
				}break;
			case ITEMCELLTYPE_TMPPACKAGE: 
			    {
				    SendTmpBagItems();
			    }break;
			}
		}break;
	case stCretDestoryItem::_value ://销毁物品
		{
			_CHECK_PACKAGE_LEN(stCretDestoryItem,ncmdlen);
			ServerGetDestoryItem(((stCretDestoryItem*)pcmd)->i64Id);
		}break;
	case stCretProcessingItem::_value://佩戴,脱下
		{
			_CHECK_PACKAGE_LEN(stCretProcessingItem,ncmdlen);
			stCretProcessingItem* pdstcmd=((stCretProcessingItem*)pcmd);
			if(pdstcmd->dwtmpid==0||pdstcmd->dwtmpid==m_PlayerObj->GetObjectId()){
				switch(pdstcmd->srcLocation.btLocation){
				case ITEMCELLTYPE_EQUIP:
					{
						switch(pdstcmd->destLocation.btLocation){
						case ITEMCELLTYPE_PACKAGE:
							{
								pdstcmd->nErrorCode =  ServerGetTakeOffItem(0,pdstcmd->srcLocation,pdstcmd->destLocation);
								m_PlayerObj->SendMsgToMe(pdstcmd,sizeof(*pdstcmd));
							}break;
						}
					}break;
				case ITEMCELLTYPE_STORE:
					{
						switch(pdstcmd->destLocation.btLocation){
						case ITEMCELLTYPE_PACKAGE:
							{
								pdstcmd->nErrorCode =  ServerGetGetBack(pdstcmd->i64Id,pdstcmd->destLocation,pdstcmd->destLocation);
								if(pdstcmd->nErrorCode != -1)
									m_PlayerObj->SendMsgToMe(pdstcmd,sizeof(*pdstcmd));
							}break;
						case ITEMCELLTYPE_STORE:
							{
								ServerGetExchangeItem(pdstcmd->i64Id,pdstcmd->destLocation);
							}break;
						}
					}break;
				case ITEMCELLTYPE_PACKAGE:
					{
						switch(pdstcmd->destLocation.btLocation){
						case ITEMCELLTYPE_EQUIP:
							{
								ServerGetWearItem(pdstcmd->i64Id,pdstcmd->destLocation.btIndex);
							}break;
						case ITEMCELLTYPE_STORE:
							{
								pdstcmd->nErrorCode = ServerGetStorage(pdstcmd->i64Id,pdstcmd->destLocation,pdstcmd->destLocation);
								if(pdstcmd->nErrorCode != -1)
									m_PlayerObj->SendMsgToMe(pdstcmd,sizeof(*pdstcmd));
							}break;
						case ITEMCELLTYPE_PACKAGE:
							{
								ServerGetExchangeItem(pdstcmd->i64Id,pdstcmd->destLocation);
							}break;
						case ITEMCELLTYPE_TMPPACKAGE:  //背包物品进临时背包
							{
								pdstcmd->nErrorCode=ServerBagItemToTmpBag(pdstcmd->i64Id, pdstcmd->destLocation, pdstcmd->destLocation);
								if (pdstcmd->nErrorCode != 0) {
									m_PlayerObj->SendMsgToMe(pdstcmd, sizeof(*pdstcmd));
								}
						    }break;
						}
					}break;
				case ITEMCELLTYPE_TMPPACKAGE:
					{
						switch (pdstcmd->destLocation.btLocation) {
							case ITEMCELLTYPE_PACKAGE:  //临时背包物品拖进背包
							{
								pdstcmd->nErrorCode=ServerTmpBagItemToBag(pdstcmd->i64Id, pdstcmd->destLocation, pdstcmd->destLocation);
								if (pdstcmd->nErrorCode != 0) {
									m_PlayerObj->SendMsgToMe(pdstcmd, sizeof(*pdstcmd));
								}
							}break;
							case ITEMCELLTYPE_TMPPACKAGE:  
							{
								ServerGetExchangeItem(pdstcmd->i64Id, pdstcmd->destLocation);
							}break;
						}
					}break;
				}
			}
		}break;
	case stCretOpenNew::_value://打开新的位置
		{
			_CHECK_PACKAGE_LEN(stCretOpenNew,ncmdlen);
			switch(((stCretOpenNew*)pcmd)->btPosition)
			{
			case ITEMCELLTYPE_PACKAGE:
				{
					ServerGetOpenNewBagPage();
				}break;
			case ITEMCELLTYPE_STORE:
				{
					ServerGetOpenNewStoragePage();
				}break;
			}

		}break;
	case MAKECMDVALUE(CMD_CLIENT_GAMESVR_ITEM,SUBCMD_ITEM_CRET_RESORTBAG):
		{
			ServerGetResortBag();
		}break;
		//case  MAKECMDVALUE(CMD_CLIENT_GAMESVR_ITEM,SUBCMD_ITEM_CRET_RESORTSTORAGE):
	case stCretResortStorage::_value:
		{
			_CHECK_PACKAGE_LEN(stCretResortStorage,ncmdlen);
			ServerGetResortStorage(((stCretResortStorage*)pcmd)->btTabId);
		}break;
	case stCretSplitItem::_value:
		{
			_CHECK_PACKAGE_LEN(stCretSplitItem,ncmdlen);
			ServerGetSplitItem(((stCretSplitItem*)pcmd)->i64Id,((stCretSplitItem*)pcmd)->nCount);
		}break;
	case stCretViewEquip::_value:
		{
			_CHECK_PACKAGE_LEN(stCretViewEquip,ncmdlen);
			ServerGetViewEuqip(((stCretViewEquip*)pcmd)->szName,((stCretViewEquip*)pcmd)->btType,((stCretViewEquip*)pcmd)->btFlag, ((stCretViewEquip*)pcmd)->i64Id);
		}break;
	case stCretForsakeItem::_value:
		{
			_CHECK_PACKAGE_LEN(stCretForsakeItem,ncmdlen);
			stCretForsakeItem* pDstCmd=(stCretForsakeItem*)pcmd;
			stCretForsakeItem Forsakecmd;
			Forsakecmd.i64id=pDstCmd->i64id;
			Forsakecmd.nGoldNum = pDstCmd->nGoldNum;
			pDstCmd->nGoldNum = pDstCmd->nGoldNum>2000?2000:pDstCmd->nGoldNum;
			if (m_PlayerObj){
				if(CUserEngine::getMe().isCrossSvr() || m_PlayerObj->isSwitchSvr()){
					Forsakecmd.btErrorCode=ITEM_FAIL_CROSSSVR;
					m_PlayerObj->SendMsgToMe(&Forsakecmd,sizeof(Forsakecmd));
					return false;
				}
				CItem* pItem= FindItemInBag(Forsakecmd.i64id);
				bool istmpbag = false;
				if (!pItem) {
					pItem = FindItemInTmpBag(Forsakecmd.i64id);
					istmpbag = true;
				}

				if (!pItem){
					Forsakecmd.btErrorCode=ITEM_FORSAKE_NOITEM;
					m_PlayerObj->SendMsgToMe(&Forsakecmd,sizeof(Forsakecmd));
					return false;
				}

				if (!pItem->GetBinding()==0){
					if (pItem->CanDestory()){
						Forsakecmd.btErrorCode=ITEM_FORSAKE_BINDING;
					}else{
						Forsakecmd.btErrorCode=ITEM_FAIL_DESTORY;
					}

					m_PlayerObj->SendMsgToMe(&Forsakecmd,sizeof(Forsakecmd));
					return false;
				}
				if (pItem && pItem->m_Item.dwBaseID >= 154 && pItem->m_Item.dwBaseID <= 156){
					if (m_PlayerObj && m_PlayerObj->m_GuildInfo.dwPowerLevel < _GUILDMEMBER_POWERLVL_MASTER) {
						m_PlayerObj->sendTipMsg(m_PlayerObj, "`6`非会长禁止丢弃该道具");
						return false;
					}
				}
				else{
					if (m_PlayerObj->m_nVipLevel < 2) {
						return false;
					}
				}
				if (CALL_LUARET("ForsakeItemCheck", true, m_PlayerObj, pItem) == false)
				{
					Forsakecmd.btErrorCode = ITEM_FORSAKE_NOMOVE;
					m_PlayerObj->SendMsgToMe(&Forsakecmd, sizeof(Forsakecmd));
					return false;
				}
				if (istmpbag) {
					if (!RemoveItemFromTmpBag(pItem, 0xffffffff, true, true)) {
						Forsakecmd.btErrorCode = ITEM_FORSAKE_NOMOVE;
						m_PlayerObj->SendMsgToMe(&Forsakecmd, sizeof(Forsakecmd));
						return false;
					}
				}
				else {
					if (!RemoveItemFromBag(pItem, 0xffffffff, true, true)) {
						Forsakecmd.btErrorCode = ITEM_FORSAKE_NOMOVE;
						m_PlayerObj->SendMsgToMe(&Forsakecmd, sizeof(Forsakecmd));
						return false;
					}
				}
				m_PlayerObj->SendMsgToMe(&Forsakecmd,sizeof(Forsakecmd));
			}
		}break;
	default:
		return false;
	}
	return true;
}

bool   CPlayerPackage::CheckCanAddToBag(CItem* pItem)
{
	FUNCTION_BEGIN;
	if (!pItem)	return false;
	auto itemBase = pItem->GetItemDataBase();
	if (!itemBase) return false;
	if (pItem->GetItemCount()==0) return false;
	if (FindItemInBag(pItem->m_Item.i64ItemID))
	{
		g_logger.error("试图添加到包裹的物品编号在包裹内存在 角色名 %s ID %I64d",m_PlayerObj->getName(),pItem->m_Item.i64ItemID);
		return false;
	}
	if (GetFreeBagCellCount(itemBase->GetBagType())==0)
	{
		DWORD ItemMaxCnt=itemBase->dwMaxCount;
		int VariableMaxCnt=itemBase->nVariableMaxCount;
		if(VariableMaxCnt>0){
			bool IsNeedChangeMaxCnt=CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("itemidchangemaxcnt",true,m_PlayerObj);
			if(IsNeedChangeMaxCnt){
				ItemMaxCnt=VariableMaxCnt;
			}
		} 
		if (ItemMaxCnt > 1)
		{
			for (int i=0;i<m_nOpenBagCount;i++)
			{
				if (m_BagPacket[i].m_boOpened){
					for (auto it =  m_BagPacket[i].m_PackItemList.begin();it != m_BagPacket[i].m_PackItemList.end();it ++){
						CItem* item= it->second;
						if (item && item->m_Item.dwBaseID == pItem->m_Item.dwBaseID)
						{
							if (item->GetItemCount()+ pItem->GetItemCount()< ItemMaxCnt)
							{
								return true;
							}
						}
					}
				}
			}
		} 
		else
		{
			return false;
		}


	}

	return true;
}


bool CPlayerPackage::CheckCanAddToTmpBag(CItem *pItem)
{
	FUNCTION_BEGIN;
	if (!pItem)	return false;
	auto itemBase = pItem->GetItemDataBase();
	if (!itemBase) return false;
	if (pItem->GetItemCount() == 0) return false;
	if (FindItemInTmpBag(pItem->m_Item.i64ItemID))
	{
		g_logger.error("试图添加到临时包裹的物品编号在临时包裹内存在 角色名 %s ID %I64d", m_PlayerObj->getName(), pItem->m_Item.i64ItemID);
		return false;
	}
	if (GetFreeTmpBagCellCount(itemBase->GetBagType()) == 0)
	{
		DWORD ItemMaxCnt = itemBase->dwMaxCount;
		int VariableMaxCnt = itemBase->nVariableMaxCount;
		if (VariableMaxCnt > 0) {
			bool IsNeedChangeMaxCnt = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("itemidchangemaxcnt", true, m_PlayerObj);
			if (IsNeedChangeMaxCnt) {
				ItemMaxCnt = VariableMaxCnt;
			}
		}
		if (ItemMaxCnt > 1)
		{
			for (int i = 0; i < m_nOpenTmpBagCount; i++)
			{
				if (m_TmpBagPacket[i].m_boOpened) {
					for (auto it = m_TmpBagPacket[i].m_PackItemList.begin(); it != m_TmpBagPacket[i].m_PackItemList.end(); it++) {
						CItem* item = it->second;
						if (item && item->m_Item.dwBaseID == pItem->m_Item.dwBaseID)
						{
							if (item->GetItemCount() + pItem->GetItemCount() < ItemMaxCnt)
							{
								return true;
							}
						}
					}
				}
			}
		}
		else
		{
			return false;
		}
	}
	return true;
}

bool CPlayerPackage::AddItemToTmpBag(CItem* pItem, bool boSendNewMsg /*= false*/, bool boQuestItem /*= false*/, bool boSendMsg /*= true*/, const char* pszLogStr /*= NULL*/, CCreature * pDestCret /*= NULL*/, DWORD dwPrice /*= 0*/, Byte btPriceType /*= 0*/)
{
	FUNCTION_BEGIN;
	if (CheckCanAddToTmpBag(pItem))
	{
		pItem->SetOutLock(false);
		if (FindItemInTmpBag(pItem->m_Item.i64ItemID))
		{
			g_logger.error("试图添加到临时包裹的物品编号在临时包裹内存在 角色名 %s ID %I64d", m_PlayerObj->getName(), pItem->m_Item.i64ItemID);
		}
		else {
			if (pItem->m_Item.Location.btTableID >= 0 && pItem->m_Item.Location.btTableID < m_nOpenTmpBagCount)
			{
				int itemid = pItem->m_Item.dwBaseID;
				if (m_TmpBagPacket[pItem->m_Item.Location.btTableID].AddItemToPack(pItem, pItem->m_Item.Location.btIndex, boSendMsg, true, boSendNewMsg, pszLogStr, pDestCret, dwPrice, btPriceType))
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool CPlayerPackage::AddItemToBag(CItem* pItem,bool boSendNewMsg,bool boQuestItem,bool boSendMsg,const char* pszLogStr,CCreature * pDestCret,DWORD dwPrice,Byte btPriceType)
{
	FUNCTION_BEGIN;
	if (CheckCanAddToBag(pItem))
	{
		pItem->SetOutLock(false);
		if (FindItemInBag(pItem->m_Item.i64ItemID))
		{
			g_logger.error("试图添加到包裹的物品编号在包裹内存在 角色名 %s ID %I64d",m_PlayerObj->getName(),pItem->m_Item.i64ItemID);
		}else{
			if (pItem->m_Item.Location.btTableID >=0 && pItem->m_Item.Location.btTableID < m_nOpenBagCount)
			{
				int itemid=pItem->m_Item.dwBaseID;
				if (m_BagPacket[pItem->m_Item.Location.btTableID].AddItemToPack(pItem,pItem->m_Item.Location.btIndex,boSendMsg,true,boSendNewMsg,pszLogStr,pDestCret,dwPrice,btPriceType))
				{	
					//进包裹自动使用
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("itemautouse",m_PlayerObj,itemid);
					return true;
				}
			}
		}
	}
	//g_logger.error("试图添加到包裹的物品添加失败 角色名 %s ID %I64d",m_PlayerObj->getName(),pItem->m_Item.i64ItemID);
	return false;
}
bool  CPlayerPackage::RemoveItemFromBag(CItem* pItem,DWORD nCount,bool boAutoMerge,bool boSendMsg)
{
	FUNCTION_BEGIN;
	if (!pItem || pItem->GetOutLock()) return false;
	if (nCount==0) return false;
	if (nCount==0xffffffff) nCount =  pItem->GetItemCount();
	if (pItem->GetItemCount() < nCount) return false;  //物品数量少于需求数量 失败
	if(FindItemInBag(pItem->GetItemID(),nCount)){
		if (pItem->GetItemCount() > nCount)    //物品数量多与需求数量 创建新物品
		{
			CItem * pNewItem = NULL;
			pNewItem = CItem::CreateItem(pItem->m_Item.dwBaseID,_CREATE_NPC_BUY,1,0,__FUNC_LINE__);
			if (pNewItem)
			{
				if (boSendMsg)
				{
					SendBagDeleteItem(pItem->m_Item.i64ItemID,pItem->m_Item.dwBaseID);
				}
				memcpy(pNewItem,pItem,sizeof(CItem));
				pNewItem->m_Item.i64ItemID = CItem::GenerateItemID();
				pNewItem->SetItemCount(pItem->GetItemCount() - nCount);
				pNewItem->m_Item.BornFromMapid=pItem->m_Item.BornFromMapid;
				strcpy_s(pNewItem->m_Item.bornfrom,sizeof(pNewItem->m_Item.bornfrom)-1,pItem->m_Item.bornfrom);
				strcpy_s(pNewItem->m_Item.szMaker,sizeof(pNewItem->m_Item.szMaker)-1,pItem->m_Item.szMaker);
				if (m_BagPacket[pItem->m_Item.Location.btTableID].RemoveFromPack(pItem->GetItemBaseID() ,pItem->GetItemID(),pItem->m_Item.Location.btIndex))
				{
					bool addOk = AddItemToBag(pNewItem,false);
					if(!addOk) CUserEngine::getMe().ReleasePItem(pNewItem,__FUNC_LINE__);
					pItem->SetItemCount(nCount);
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("itemremove", m_PlayerObj, pItem->GetItemBaseID());
				}
			}
			return true;
		}


		if (pItem->m_Item.Location.btLocation == ITEMCELLTYPE_PACKAGE&&(pItem->m_Item.Location.btTableID>=0&&pItem->m_Item.Location.btTableID<m_nOpenBagCount))  //是包裹的物品
		{
			if (m_BagPacket[pItem->m_Item.Location.btTableID].RemoveFromPack(pItem->GetItemBaseID(),pItem->GetItemID(),pItem->m_Item.Location.btIndex))
			{
				if (boSendMsg)
				{
					SendBagDeleteItem(pItem->m_Item.i64ItemID,pItem->m_Item.dwBaseID);
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("itemremove", m_PlayerObj, pItem->GetItemBaseID());
				}
				return true;
			}
		}
	}else{
		g_logger.debug("玩家 %s 物品没有找到 nBaseID=%d i64Id=%I64u %s",m_PlayerObj?m_PlayerObj->getName():"",pItem->GetItemBaseID(),pItem->GetItemID(),__FUNC_LINE__);
	}
	return false;
}

bool  CPlayerPackage::RemoveItemFromTmpBag(CItem* pItem, DWORD nCount, bool boAutoMerge, bool boSendMsg)
{
	FUNCTION_BEGIN;
	if (!pItem || pItem->GetOutLock()) return false;
	if (nCount == 0) return false;
	if (nCount == 0xffffffff) nCount = pItem->GetItemCount();
	if (pItem->GetItemCount() < nCount) return false;  //物品数量少于需求数量 失败
	if (FindItemInTmpBag(pItem->GetItemID(), nCount)) {
		if (pItem->GetItemCount() > nCount)    //物品数量多与需求数量 创建新物品
		{
			CItem * pNewItem = NULL;
			pNewItem = CItem::CreateItem(pItem->m_Item.dwBaseID, _CREATE_NPC_BUY, 1, 0,  __FUNC_LINE__);
			if (pNewItem)
			{
				if (boSendMsg)
				{
					SendTmpBagDeleteItem(pItem->m_Item.i64ItemID, pItem->m_Item.dwBaseID);
				}
				memcpy(pNewItem, pItem, sizeof(CItem));
				pNewItem->m_Item.i64ItemID = CItem::GenerateItemID();
				pNewItem->SetItemCount(pItem->GetItemCount() - nCount);
				pNewItem->m_Item.BornFromMapid = pItem->m_Item.BornFromMapid;
				strcpy_s(pNewItem->m_Item.bornfrom, sizeof(pNewItem->m_Item.bornfrom) - 1, pItem->m_Item.bornfrom);
				strcpy_s(pNewItem->m_Item.szMaker, sizeof(pNewItem->m_Item.szMaker) - 1, pItem->m_Item.szMaker);
				if (m_TmpBagPacket[pItem->m_Item.Location.btTableID].RemoveFromPack(pItem->GetItemBaseID(), pItem->GetItemID(), pItem->m_Item.Location.btIndex))
				{
					bool addOk = AddItemToTmpBag(pNewItem, false);
					if (!addOk) CUserEngine::getMe().ReleasePItem(pNewItem, __FUNC_LINE__);
					pItem->SetItemCount(nCount);
				}
			}
			m_PlayerObj->TriggerEvent(m_PlayerObj, ITEMGET, pItem->GetItemBaseID());
			return true;
		}


		if (pItem->m_Item.Location.btLocation == ITEMCELLTYPE_TMPPACKAGE && (pItem->m_Item.Location.btTableID >= 0 && pItem->m_Item.Location.btTableID < m_nOpenTmpBagCount))  //是包裹的物品
		{
			if (m_TmpBagPacket[pItem->m_Item.Location.btTableID].RemoveFromPack(pItem->GetItemBaseID(), pItem->GetItemID(), pItem->m_Item.Location.btIndex))
			{
				if (boSendMsg)
				{
					SendTmpBagDeleteItem(pItem->m_Item.i64ItemID, pItem->m_Item.dwBaseID);
				}
				m_PlayerObj->TriggerEvent(m_PlayerObj, ITEMGET, pItem->GetItemBaseID());
				//m_PlayerObj->TriggerEvent(m_PlayerObj, ITEMGET, 1);
				return true;
			}
		}
	}
	else {
		g_logger.debug("玩家 %s 物品没有找到 nBaseID=%d i64Id=%I64u %s", m_PlayerObj ? m_PlayerObj->getName() : "", pItem->GetItemBaseID(), pItem->GetItemID(), __FUNC_LINE__);
	}
	return false;
}

CItem* CPlayerPackage::FindItemInBagByBaseID(DWORD baseid,DWORD nCount)
{
	FUNCTION_BEGIN;
	if (nCount==0) return NULL;
	for (int i=0;i<m_nOpenBagCount;i++)
	{
		CItem* pItem =  m_BagPacket[i].FindItemByBaseID(baseid);
		if (pItem && pItem->GetItemCount()>=nCount)
		{
			return pItem;
		}
	}
	return NULL;
}

CItem* CPlayerPackage::FindItemInBag(int64_t i64Id,DWORD nCount)
{
	FUNCTION_BEGIN;
	if (nCount==0) return NULL;
	for (int i=0;i<m_nOpenBagCount;i++)
	{
		CItem* pItem =  m_BagPacket[i].FindItem(i64Id);
		if (pItem && pItem->GetItemCount()>=nCount)
		{
			return pItem;
		}
	}
	return NULL;
}

CItem* CPlayerPackage::FindItemInTmpBag(int64_t i64Id, DWORD nCount /*=1*/)
{
	FUNCTION_BEGIN;
	if (nCount == 0) return NULL;
	for (int i = 0; i < m_nOpenTmpBagCount; i++)
	{
		CItem* pItem = m_TmpBagPacket[i].FindItem(i64Id);
		if (pItem && pItem->GetItemCount() >= nCount)
		{
			return pItem;
		}
	}
	return NULL;
}

bool CPlayerPackage::AddItemToStorage(CItem* pItem,DWORD nCount)
{
	FUNCTION_BEGIN;
	if (!pItem) return false;
	auto itemBase = pItem->GetItemDataBase();
	if (!itemBase) return false;
	if (nCount==0) return false;
	if (nCount==0xffffffff) nCount=pItem->GetItemCount();
	if (pItem->GetItemCount()==0) return false;
	if (itemBase->dwType == ITEM_TYPE_NORMAL)
	{
		return false;
	}
	if (itemBase->dwMaxCount>= nCount)
	{
		pItem->SetItemCount(nCount);
	}
	else return false;

	if (FindItemInStorage(pItem->m_Item.i64ItemID))
	{
		return false;
	}
	for (int i=0;i<m_nOpenStorageCount;i++)
	{
		if (m_StoragePacket[i].m_boOpened&& m_StoragePacket[i].AddItemToPack(pItem,pItem->m_Item.Location.btIndex))
		{
			return true;
		}
	}
	return false;
}


CItem* CPlayerPackage::FindItemInStorage(int64_t i64Id)   //寻找仓库中的物品
{
	FUNCTION_BEGIN;
	for (int i=0;i<m_nOpenStorageCount;i++)
	{
		CItem* pItem =  m_StoragePacket[i].FindItem(i64Id);
		if (pItem)
		{
			return pItem;
		}
	}
	return NULL;
}

bool CPlayerPackage::DeleteItemInStorage(CItem* pItem,const char* pszLogStr)   //删除仓库中的物品
{
	FUNCTION_BEGIN;
	if(pItem && pItem->m_Item.dwExpireTime>0) //限时物品删除
	{
		m_PlayerObj->set_LimitItemPacket.erase(pItem);
	}	

	for (int i=0;i<m_nOpenStorageCount;i++)
	{
		bool b =  m_StoragePacket[i].DeleteFromPack(pItem->GetItemBaseID(),pItem->GetItemID(),pszLogStr);
		if (b)
		{
			return true;
		}
	}
	return false;
}

bool CPlayerPackage::FindItemInBagAllByBaseId(DWORD baseid,DWORD nCount)
{
	FUNCTION_BEGIN;
	if (nCount==0) return false;
	DWORD itemnum=0;
	for (int i=0;i<m_nOpenBagCount;i++)
	{
		const auto items = m_BagPacket[i].FindItemsByBaseID(baseid);
		if (!items) continue;
		for (auto& pItem : *items) {
			auto itemBase = pItem->GetItemDataBase();
			if (!itemBase) continue;
			if (pItem->m_Item.dwBaseID==baseid && !pItem->GetOutLock()){
				if (itemBase->dwMaxCount==1){
					if(itemBase->nVariableMaxCount==0){
					    itemnum++;
					}else{
						itemnum=itemnum+pItem->GetItemCount();
					}
				}
				else if (itemBase->dwMaxCount>1)
				{
					itemnum=itemnum+pItem->GetItemCount();
				}

				if (itemnum>=nCount){return true;}
			}
		}
	}
	if (itemnum>=nCount){return true;}
	else {return false;}
}

bool CPlayerPackage::FindItemInBagAllByBaseIdWithBindSta(DWORD baseid, DWORD nCount,BYTE btBind)
{
	FUNCTION_BEGIN;
	if (nCount == 0) return false;
	DWORD itemnum = 0;
	for (int i = 0; i < m_nOpenBagCount; i++)
	{
		const auto items = m_BagPacket[i].FindItemsByBaseID(baseid);
		if (!items) continue;
		for (auto& pItem : *items) {
			auto itemBase = pItem->GetItemDataBase();
			if (!itemBase) continue;
			if (pItem->m_Item.dwBaseID == baseid && !pItem->GetOutLock() && btBind == pItem->GetBinding()) {
				if (itemBase->dwMaxCount == 1) {
					if (itemBase->nVariableMaxCount == 0) {
						itemnum++;
					}
					else {
						itemnum = itemnum + pItem->GetItemCount();
					}
				}
				else if (itemBase->dwMaxCount > 1)
				{
					itemnum = itemnum + pItem->GetItemCount();
				}

				if (itemnum >= nCount) { return true; }
			}
		}
	}
	if (itemnum >= nCount) { return true; }
	else { return false; }
}


DWORD CPlayerPackage::GetItemCountInBagAllByBaseId(DWORD baseid)
{
	FUNCTION_BEGIN;
	int itemnum=0;
	for (int i=0;i<m_nOpenBagCount;i++)
	{
		const auto items = m_BagPacket[i].FindItemsByBaseID(baseid);
		if (!items) continue;
		for (auto& pItem : *items) {
			auto itemBase = pItem->GetItemDataBase();
			if (!itemBase) continue;
			if (pItem->m_Item.dwBaseID==baseid && !pItem->GetOutLock()){
				if (itemBase->dwMaxCount==1){
					if (itemBase->nVariableMaxCount == 0) {
						itemnum++;
					}
					else
					{
						itemnum = itemnum + pItem->GetItemCount();
					}

				}
				else if (itemBase->dwMaxCount>1)
				{
					itemnum=itemnum+pItem->GetItemCount();
				}
			}
		}
	}
	return itemnum;
}

DWORD CPlayerPackage::GetItemCountInStoreAllByBaseId(DWORD baseid){
	FUNCTION_BEGIN;
	int itemnum=0;
	for (int i=0;i<m_nOpenStorageCount;i++){
		const auto items = m_BagPacket[i].FindItemsByBaseID(baseid);
		if (!items) continue;
		for (auto& pItem : *items) {
			auto itemBase = pItem->GetItemDataBase();
			if (!itemBase) continue;
			if (pItem->m_Item.dwBaseID==baseid && !pItem->GetOutLock()){
				if (itemBase->dwMaxCount==1){
					itemnum++;
				}else if (itemBase->dwMaxCount>1){
					itemnum=itemnum+pItem->GetItemCount();
				}
			}
		}
	}
	return itemnum;
}

bool CPlayerPackage::FindItemInBagAllByType(DWORD itemtype){
	FUNCTION_BEGIN;
	for (int i=0;i<m_nOpenBagCount;i++)
	{
		for (auto it =  m_BagPacket[i].m_PackItemList.begin();it != m_BagPacket[i].m_PackItemList.end();it ++){
			CItem* pItem= it->second;
			if (!pItem) continue;
			auto itemBase = pItem->GetItemDataBase();
			if (!itemBase) continue;
			if (itemBase->dwType==ITEM_TYPE_EQUIP && itemBase->btEquipStation==itemtype){
				return true;
			}
		}
	}
	return false;
}

bool CPlayerPackage::DeleteItemInBag(CItem* pItem,DWORD nCount,bool boSendMsg,bool boNoScrpitEvent,const char* pszLogStr,CCreature * pDescCret)
{
	FUNCTION_BEGIN;
	if(!pItem || pItem->GetOutLock()) return false;
	if (pItem->m_Item.Location.btLocation!=ITEMCELLTYPE_PACKAGE) return false;
	if (nCount==0) return false;
	if (nCount==0xffffffff) nCount =  pItem->GetItemCount();
	if (pItem->GetItemCount() < nCount)  return false;  //物品数量不足删除的 直接返回失败
	if(FindItemInBag(pItem->GetItemID(),nCount)){
		if (pItem->GetItemCount() > nCount)
		{
			pItem->SetItemCount(pItem->GetItemCount()-nCount);  //更新物品 不做删除
			if (boSendMsg)SendBagItemCountChanged(pItem);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("itemremove", m_PlayerObj, pItem->GetItemBaseID());
			if(pszLogStr)
				pItem->SetItemLog(pszLogStr,m_PlayerObj,pDescCret,false,-(int)nCount);

			return true;
		}

		if(pItem->m_Item.dwExpireTime>0) //限时物品删除
		{
			m_PlayerObj->set_LimitItemPacket.erase(pItem);
		}

		for (int i=0;i<m_nOpenBagCount;i++)
		{
			m_BagPacket[i].RemoveFromPack(pItem->GetItemBaseID(),pItem->GetItemID(),pItem->m_Item.Location.btIndex);
		}
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("itemremove", m_PlayerObj, pItem->GetItemBaseID());
		pItem->SetItemCount(0);
		if (boSendMsg) SendBagDeleteItem(pItem->m_Item.i64ItemID,pItem->m_Item.dwBaseID);
		const char* pszDelStr = vformat("delItem-%s",pszLogStr?pszLogStr:"other");
		pItem->SetItemLog(pszDelStr,m_PlayerObj,pDescCret,false,-(int)nCount);
		CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);

		return true;
	}else{
		g_logger.debug("玩家 %s 物品没有找到 nBaseID=%d i64Id=%I64u %s",m_PlayerObj?m_PlayerObj->getName():"",pItem->GetItemBaseID(),pItem->GetItemID(),__FUNC_LINE__);
	}
	return false;	
}

bool CPlayerPackage::DeleteItemInTmpBag(CItem* pItem, DWORD nCount /*= 0xffffffff*/, bool boSendMsg /*= false*/, bool boNoScrpitEvent /*= false*/, const char* pszLogStr /*= NULL*/, CCreature * pDescCret /*= NULL*/)
{
	if (!pItem || pItem->GetOutLock()) return false;
	if (pItem->m_Item.Location.btLocation != ITEMCELLTYPE_TMPPACKAGE) return false;
	if (nCount == 0) return false;
	if (nCount == 0xffffffff) nCount = pItem->GetItemCount();
	if (pItem->GetItemCount() < nCount)  return false;  //物品数量不足删除的 直接返回失败
	if (FindItemInTmpBag(pItem->GetItemID(), nCount)) {
		if (pItem->GetItemCount() > nCount)
		{
			pItem->SetItemCount(pItem->GetItemCount() - nCount);  //更新物品 不做删除
			if (boSendMsg)SendBagItemCountChanged(pItem);
			if (pszLogStr)
				pItem->SetItemLog(pszLogStr, m_PlayerObj, pDescCret, false, -(int)nCount);

			return true;
		}

		for (int i = 0; i < m_nOpenTmpBagCount; i++)
		{
			m_TmpBagPacket[i].RemoveFromPack(pItem->GetItemBaseID(), pItem->GetItemID(), pItem->m_Item.Location.btIndex);
		}
		pItem->SetItemCount(0);
		if (boSendMsg) SendTmpBagDeleteItem(pItem->m_Item.i64ItemID, pItem->m_Item.dwBaseID);
		const char* pszDelStr = vformat("delItem-%s", pszLogStr ? pszLogStr : "other");
		pItem->SetItemLog(pszDelStr, m_PlayerObj, pDescCret, false, -(int)nCount);
		CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);

		return true;
	}
	else {
		g_logger.debug("玩家临时背包 %s 物品没有找到 nBaseID=%d i64Id=%I64u %s", m_PlayerObj ? m_PlayerObj->getName() : "", pItem->GetItemBaseID(), pItem->GetItemID(), __FUNC_LINE__);
	}
	return false;
}

bool CPlayerPackage::DeleteItemInBagAllByBaseId(DWORD dwBaseId,DWORD num,const char* pszLogStr)
{
	FUNCTION_BEGIN;
	if (num==0) return false;
	if (!FindItemInBagAllByBaseId(dwBaseId,num)){return false;}
	bool isDelItem = false;
	for (int i=0;i<m_nOpenBagCount;i++)
	{
		const auto items = m_BagPacket[i].FindItemsByBaseID(dwBaseId);
		if (!items) continue;
		for (auto& item : *items) {
			if (num)
			{
				if (item&&!item->GetOutLock()&&dwBaseId==item->m_Item.dwBaseID)
				{

					if (item->GetItemCount()<=num)
					{
						num -= item->GetItemCount();
						SendBagDeleteItem(item->GetItemID(),item->m_Item.dwBaseID);		
						m_BagPacket[i].DeleteFromPack(item->GetItemBaseID(),item->GetItemID(), pszLogStr);
						isDelItem = true;
					}
					else
					{
						if(pszLogStr) item->SetItemLog(pszLogStr,m_PlayerObj,NULL,false,-(int)num);
						item->SetItemCount(item->GetItemCount()-num);
						SendBagItemCountChanged(item);
						isDelItem = true;
						
						break;
					}
				}

			}else break;
		}
	}
	if (isDelItem)
	{
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("itemremove", m_PlayerObj, dwBaseId);
	}
	return true;
}
DWORD  CPlayerPackage::GetItemCountByBindType(DWORD dwBaseId, BYTE btBindType) {
	FUNCTION_BEGIN;
	DWORD itemnum = 0;
	for (int i = 0; i < m_nOpenBagCount; i++)
	{
		const auto items = m_BagPacket[i].FindItemsByBaseID(dwBaseId);
		if (!items) continue;
		for (auto& pItem : *items){
			if (pItem && pItem->m_Item.dwBaseID == dwBaseId && !pItem->GetOutLock() && pItem->GetBinding()==btBindType && pItem->GetCanTrade() && !pItem->GetExistNpData()) {
				if (auto itemBase = pItem->GetItemDataBase())
				{
					if (itemBase->dwMaxCount == 1) {
						itemnum++;
					}
					else if (itemBase->dwMaxCount > 1)
					{
						itemnum = itemnum + pItem->GetItemCount();
					}
				}

			}
		}
	}
	return itemnum;
}
DWORD  CPlayerPackage::DelAllItemInBagByBindType(DWORD dwBaseId, DWORD dwLimitNum,BYTE btBindType, const char* pszLogStr) {
	FUNCTION_BEGIN;
	if (dwLimitNum == 0) return 0;
	DWORD dwDelNum = 0;
	bool isDelItem = false;
	for (int i = 0; i < m_nOpenBagCount; i++)
	{
		const auto items = m_BagPacket[i].FindItemsByBaseID(dwBaseId);
		if (!items) continue;
		for (auto& item:*items)
		{
			if (dwLimitNum)
			{
				if (item && !item->GetOutLock() && dwBaseId == item->m_Item.dwBaseID && item->GetBinding() == btBindType && item->GetCanTrade() && !item->GetExistNpData())
				{

					if (item->GetItemCount() <= dwLimitNum)
					{
						dwLimitNum -= item->GetItemCount();
						dwDelNum += item->GetItemCount();
						SendBagDeleteItem(item->GetItemID(), item->m_Item.dwBaseID);
						m_BagPacket[i].DeleteFromPack(item->GetItemBaseID(), item->GetItemID(), pszLogStr);
						isDelItem = true;
					}
					else
					{
						if (pszLogStr) item->SetItemLog(pszLogStr, m_PlayerObj, NULL, false, -(int)dwLimitNum);
						item->SetItemCount(item->GetItemCount() - dwLimitNum);
						dwDelNum += dwLimitNum;
						SendBagItemCountChanged(item);
						isDelItem = true;
						break;
					}
				}

			}
			else break;
		}
	}
	if (isDelItem)
	{
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("itemremove", m_PlayerObj, dwBaseId);
	}
	return dwDelNum;
}
bool CPlayerPackage::DeleteItemTmpId(int64_t tmpid,DWORD nCount,const char* pszLogStr){
	FUNCTION_BEGIN;
	if (nCount==0) return false;
	CItem* item=FindItemInBag(tmpid,nCount);
	if (item){
		if(pszLogStr) item->SetItemLog(pszLogStr,m_PlayerObj,NULL,false,nCount);
		if (item->GetItemCount()<nCount){return false;}
		if (item->GetItemCount()>nCount){
			item->SetItemCount(item->GetItemCount()-nCount);
			SendBagItemCountChanged(item);
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("itemremove", m_PlayerObj, item->GetItemBaseID());
			if(pszLogStr)
				item->SetItemLog(pszLogStr,m_PlayerObj,NULL,false,-(int)nCount);

			return true;
		}
		for (int i=0;i<m_nOpenBagCount;i++)
		{
			m_BagPacket[i].RemoveFromPack(item->GetItemBaseID(),item->GetItemID(),item->m_Item.Location.btIndex);
		}
		SendBagDeleteItem(item->m_Item.i64ItemID,item->m_Item.dwBaseID);
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("itemremove", m_PlayerObj, item->GetItemBaseID());
		item->SetItemCount(0);
		const char* pszDelStr = vformat("delItem-%s",pszLogStr?pszLogStr:"other");
		item->SetItemLog(pszDelStr,m_PlayerObj,NULL,false,-(int)nCount);

		CUserEngine::getMe().ReleasePItem(item,__FUNC_LINE__);
		return true;
	}
	return false;
}

CItem* CPlayerPackage::FindItemInBody(int64_t i64Id)
{
	FUNCTION_BEGIN;
	for (int i=0;i<EQUIP_MAX_COUNT;i++)
	{
		if (m_stEquip[i] &&  m_stEquip[i]->m_Item.i64ItemID == i64Id)
			return m_stEquip[i];
	}
	return NULL;
}

CItem* CPlayerPackage::FindItemInBodyby64(const char* luatmpid )
{
	FUNCTION_BEGIN;
	if (luatmpid && luatmpid[0]!=0)
	{
		int64_t i64Id=CItem::strToi642(luatmpid);
		if (i64Id!=0)
		{
			for (int i=0;i<EQUIP_MAX_COUNT;i++)
			{
				if (m_stEquip[i] &&  m_stEquip[i]->m_Item.i64ItemID == i64Id)
					return m_stEquip[i];
			}
		}
	}
	return NULL;
}

bool CPlayerPackage::DeleteItemInBody(CItem* pItem,const char* pszLogStr){
	FUNCTION_BEGIN;
	if (pItem){
		for (int i=0;i<EQUIP_MAX_COUNT;i++)
		{
			if (m_stEquip[i] && m_stEquip[i]==pItem && m_stEquip[i]->m_Item.i64ItemID==pItem->m_Item.i64ItemID){

				if(pItem->m_Item.dwExpireTime!=0 )//限时物品
				{
					m_PlayerObj->set_LimitItemPacket.erase(pItem);
				}

				auto pitemdata = m_stEquip[i]->GetItemDataBase();
				if (pitemdata && pitemdata->vitemskills.size() && isNeedDeleteSkill(i, pitemdata->nID)) {
					for (DWORD i = 0; i < pitemdata->vitemskills.size(); i++) {
						stItemSkill itemskill = pitemdata->vitemskills[i];
						m_PlayerObj->DeleteSkill(itemskill.dwSkillId);
					}
				}
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("EuqipRemove", m_PlayerObj, pItem->GetItemBaseID(),true);
				SendEquipDeleteItem(m_stEquip[i]->m_Item.i64ItemID);
				if(pszLogStr) m_stEquip[i]->SetItemLog(pszLogStr,m_PlayerObj,NULL,false,-(int)pItem->GetItemCount());
				m_stEquip[i]=NULL;
				CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);
				return true;
			}
		}
	}
	return false;
}

bool CPlayerPackage::RemoveItemFromBody(CItem* pItem){
	FUNCTION_BEGIN;
	if (pItem){
		for (int i=0;i<EQUIP_MAX_COUNT;i++)
		{
			if (m_stEquip[i] && m_stEquip[i]==pItem && m_stEquip[i]->m_Item.i64ItemID==pItem->m_Item.i64ItemID){
				auto pitemdata = m_stEquip[i]->GetItemDataBase();
				if (pitemdata && pitemdata->vitemskills.size() && isNeedDeleteSkill(i, pitemdata->nID)) {
					for (DWORD i = 0; i < pitemdata->vitemskills.size(); i++) {
						stItemSkill itemskill = pitemdata->vitemskills[i];
						m_PlayerObj->DeleteSkill(itemskill.dwSkillId);
					}
				}
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("EuqipRemove", m_PlayerObj, pItem->GetItemBaseID(),false);
				SendEquipDeleteItem(m_stEquip[i]->m_Item.i64ItemID);
				m_stEquip[i]=NULL;
				return true;
			}
		}
	}
	return false;
}

void  CPlayerPackage::SendBagItems()
{
	FUNCTION_BEGIN;
	PTR_CMD(stCretItems,BagItems,getsafepacketbuf());
	BagItems->btType=0;
	BagItems->dwSortCD = (m_dwLastResortBagTime>(DWORD)time(NULL))?(m_dwLastResortBagTime-time(NULL)) :0;
	BagItems->btPosition = ITEMCELLTYPE_PACKAGE;
	BagItems->btOpenCount = m_nOpenBagCount;
	int nCount=0;
	bool boSendAllOnce=true;
	for (int i=0;i<m_nOpenBagCount;i++)
	{
		for (auto it =  m_BagPacket[i].m_PackItemList.begin();it != m_BagPacket[i].m_PackItemList.end();it ++){
			CItem* pItem= it->second;
			if (pItem)
			{
				if(nCount>=_MAXSENDCLIENTITEMS_){
					m_PlayerObj->SendMsgToMe(BagItems,sizeof(*BagItems)+BagItems->items.getarraysize());
					BagItems->items.clear();
					BagItems->btType=1;
					nCount=0;
					boSendAllOnce=false;
				}
				BagItems->items.push_back(pItem->m_Item,__FUNC_LINE__);
				nCount++;
			}
		}
	}
	if(boSendAllOnce){
		BagItems->btType=3;
	}else{
		BagItems->btType=2;
	}
	m_PlayerObj->SendMsgToMe(BagItems,sizeof(*BagItems)+BagItems->items.getarraysize());
}


void CPlayerPackage::SendTmpBagItems()
{
	FUNCTION_BEGIN;
	PTR_CMD(stCretItems, BagItems, getsafepacketbuf());
	BagItems->btType = 0;
	BagItems->dwSortCD = 0;
	BagItems->btPosition = ITEMCELLTYPE_TMPPACKAGE;
	BagItems->btOpenCount = m_nOpenTmpBagCount;
	bool boSendAllOnce = true;
	for (int i = 0; i < m_nOpenTmpBagCount; i++)
	{
		for (auto it = m_TmpBagPacket[i].m_PackItemList.begin(); it != m_TmpBagPacket[i].m_PackItemList.end(); it++) {
			CItem* pItem = it->second;
			if (pItem)
			{
				BagItems->items.push_back(pItem->m_Item, __FUNC_LINE__);
			}
		}
	}
	BagItems->btType = 3;
	m_PlayerObj->SendMsgToMe(BagItems, sizeof(*BagItems) + BagItems->items.getarraysize());
}

void CPlayerPackage::SendTmpBagAddAndUpdateItem(CItem *pItem)
{
	FUNCTION_BEGIN;
	stCretUpdateItem tmpbagitem;
	tmpbagitem.btPosition = ITEMCELLTYPE_TMPPACKAGE;
	if (pItem)
	{
		tmpbagitem.item = pItem->m_Item;
		m_PlayerObj->SendMsgToMe(&tmpbagitem, sizeof(tmpbagitem));
	}
}

void CPlayerPackage::SendTmpBagDeleteItem(int64_t i64ID, DWORD dwBaseID)
{
	FUNCTION_BEGIN;
	stCretDeleteItem bagitem;
	bagitem.btPosition = ITEMCELLTYPE_TMPPACKAGE;
	bagitem.i64Id = i64ID;
	m_PlayerObj->SendMsgToMe(&bagitem, sizeof(bagitem));
}

void CPlayerPackage::SendSoldItems()
{
	FUNCTION_BEGIN;
	PTR_CMD(stCretItems,SoldItems,getsafepacketbuf());
	SoldItems->btType=0;
	SoldItems->dwSortCD = 0;
	SoldItems->btPosition = ITEMCELLTYPE_SOLDITEM;
	int nCount=0;
	bool boSendAllOnce=true;
	for (std::list<CItem*>::reverse_iterator it =m_LastSellItems.rbegin();it!=m_LastSellItems.rend();it++)
	{
		CItem* pItem = *it;
		if(pItem){
			if(nCount>=_MAXSENDCLIENTITEMS_){
				m_PlayerObj->SendMsgToMe(SoldItems,sizeof(*SoldItems)+SoldItems->items.getarraysize());
				SoldItems->items.clear();
				SoldItems->btType=1;
				nCount=0;
				boSendAllOnce=false;
			}
			SoldItems->items.push_back(pItem->m_Item,__FUNC_LINE__);
			nCount++;
		}
	}
	if(boSendAllOnce){
		SoldItems->btType=3;
	}else{
		SoldItems->btType=2;
	}
	m_PlayerObj->SendMsgToMe(SoldItems,sizeof(*SoldItems)+SoldItems->items.getarraysize());
}

void CPlayerPackage::SendStorageItems()
{
	FUNCTION_BEGIN;
	PTR_CMD(stCretItems,StorageItems,getsafepacketbuf());
	StorageItems->btType=0;
	StorageItems->dwSortCD = (m_dwLastResortSrotageTime>(DWORD)time(NULL))?(m_dwLastResortSrotageTime-time(NULL)) :0;
	StorageItems->btPosition = ITEMCELLTYPE_STORE;
	StorageItems->btOpenCount = m_nOpenStorageCount;
	int nCount=0;
	bool boSendAllOnce=true;
	for (int i=0;i<m_nOpenStorageCount;i++)
	{
		for (auto it =  m_StoragePacket[i].m_PackItemList.begin();it != m_StoragePacket[i].m_PackItemList.end();it ++){
			CItem* pItem= it->second;
			if (pItem )
			{
				if(nCount>=_MAXSENDCLIENTITEMS_){
					m_PlayerObj->SendMsgToMe(StorageItems,sizeof(*StorageItems)+StorageItems->items.getarraysize());
					StorageItems->items.clear();
					StorageItems->btType=1;
					nCount=0;
					boSendAllOnce= false;
				}
				StorageItems->items.push_back(pItem->m_Item,__FUNC_LINE__);
				nCount++;
			}
		}
	}
	if(boSendAllOnce){
		StorageItems->btType=3;
	}else{
		StorageItems->btType=2;
	}
	m_PlayerObj->SendMsgToMe(StorageItems,sizeof(*StorageItems)+StorageItems->items.getarraysize());
}

void CPlayerPackage::SendEquipItems()
{
	FUNCTION_BEGIN;

	BUFFER_CMD(stCretItems,EquipItems,stBasePacket::MAX_PACKET_SIZE);
	EquipItems->btType=0;
	EquipItems->dwSortCD = 0;
	EquipItems->btPosition = ITEMCELLTYPE_EQUIP;
	int nCount = 0;
	bool boSendAllOnce = true;
	for (int i=0;i<EQUIP_MAX_COUNT;i++){
		if (m_stEquip[i]){
			if(nCount >= _MAXSENDCLIENTITEMS_){
				m_PlayerObj->SendMsgToMe(EquipItems,sizeof(*EquipItems)+EquipItems->items.getarraysize());
				EquipItems->items.clear();
				EquipItems->btType = 1;
				nCount = 0;
				boSendAllOnce = false;
			}
			EquipItems->items.push_back(m_stEquip[i]->m_Item,__FUNC_LINE__);
			nCount++;
		}
	}
	if(boSendAllOnce){
		EquipItems->btType = 3;
	}else{
		EquipItems->btType = 2;
	}
	m_PlayerObj->SendMsgToMe(EquipItems,sizeof(*EquipItems)+EquipItems->items.getarraysize());
}

void CPlayerPackage::SendEquipAddAndUpdateItem(CItem* pItem)         
{
	FUNCTION_BEGIN;
	stCretUpdateItem equipitem;
	equipitem.btPosition = ITEMCELLTYPE_EQUIP;
	if (pItem)
	{
		equipitem.item = pItem->m_Item;
		m_PlayerObj->SendMsgToMe(&equipitem,sizeof(equipitem)); 
	}
}

void CPlayerPackage::SendEquipDuraChanged(CItem* pItem)
{
	FUNCTION_BEGIN;
	stCretUpdateItemDura equipdura;
	equipdura.btPosition = pItem->m_Item.Location.btIndex;
	equipdura.nDura = pItem->GetDura();
	m_PlayerObj->SendMsgToMe(&equipdura,sizeof(stCretUpdateItemDura));
}

void CPlayerPackage::SendEquipDeleteItem(int64_t i64ID)			   //身上物品减少
{
	stCretDeleteItem equipitem;
	equipitem.btPosition = ITEMCELLTYPE_EQUIP;
	equipitem.i64Id = i64ID;
	m_PlayerObj->SendMsgToMe(&equipitem,sizeof(equipitem)); 
}

void CPlayerPackage::SendBagItemCountChanged(CItem* pItem){
	stCretItemCountChanged itemcount;
	itemcount.btPosition = pItem->m_Item.Location.btLocation;
	itemcount.itemid = pItem->m_Item.i64ItemID;
	itemcount.dwCount = pItem->GetItemCount();
	m_PlayerObj->SendMsgToMe(&itemcount,sizeof(itemcount)); 
}

void CPlayerPackage::SendBagAddAndUpdateItem(CItem* pItem)         
{
	FUNCTION_BEGIN;
	stCretUpdateItem bagitem;
	bagitem.btPosition = ITEMCELLTYPE_PACKAGE;
	if (pItem)
	{
		bagitem.item = pItem->m_Item;
		m_PlayerObj->SendMsgToMe(&bagitem,sizeof(bagitem)); 
	}
}

void CPlayerPackage::SendBagDeleteItem(int64_t i64ID,DWORD dwBaseID)		
{
	FUNCTION_BEGIN;
	stCretDeleteItem bagitem;
	bagitem.btPosition = ITEMCELLTYPE_PACKAGE;
	bagitem.i64Id = i64ID;
	m_PlayerObj->SendMsgToMe(&bagitem,sizeof(bagitem)); 
}
void CPlayerPackage::SendStorageAddAndUpdateItem(CItem* pItem)         
{
	FUNCTION_BEGIN;
	stCretUpdateItem bagitem;
	bagitem.btPosition = ITEMCELLTYPE_STORE;
	if (pItem)
	{
		bagitem.item = pItem->m_Item;
		m_PlayerObj->SendMsgToMe(&bagitem,sizeof(bagitem)); 
	}
}

void CPlayerPackage::SendStorageDeleteItem(int64_t i64ID)	
{
	FUNCTION_BEGIN;
	stCretDeleteItem bagitem;
	bagitem.btPosition = ITEMCELLTYPE_STORE;
	bagitem.i64Id = i64ID;
	m_PlayerObj->SendMsgToMe(&bagitem,sizeof(bagitem)); 
}

bool CPlayerPackage::ServerGetWearItem(int64_t i64SrcId,int nPos,bool boLuaWear,CItem* pLuaItem)                    //佩戴装备
{
	FUNCTION_BEGIN;
	CItem*pItem= NULL;
	if(pLuaItem){
		pItem = pLuaItem;
	}else{
		pItem= FindItemInBag(i64SrcId);//背包里找装备
	}
	stCretProcessingItem WearItemCmd;
	WearItemCmd.nErrorCode=-1;
	WearItemCmd.i64Id = i64SrcId;
	int panduanfanhui = ServerGetWearItem_PanDuan(pItem, nPos,boLuaWear);
	DWORD SuitType = 0;
	if (panduanfanhui != ITEM_SUCCESS){
		WearItemCmd.nErrorCode = panduanfanhui;
	}else{
		bool boItemSkillCool=true;
		if(m_stEquip[nPos]){
			auto pitemdata = m_stEquip[nPos]->GetItemDataBase();
			if(pitemdata && pitemdata->vitemskills.size()){
				for (DWORD i=0;i<pitemdata->vitemskills.size();i++){
					stItemSkill itemskill=pitemdata->vitemskills[i];
					stMagic* pmagic=m_PlayerObj->m_cMagic.findskill(itemskill.dwSkillId);
					if(pmagic && !pmagic->isCooling()){
						boItemSkillCool=false;
					}
				}
			}
		}
		if(boItemSkillCool){
				WearItemCmd.srcLocation=pItem->m_Item.Location;
				if (m_stEquip[nPos]){					
					int stItemId = 0;
					WearItemCmd.destLocation=m_stEquip[nPos]->m_Item.Location;
					if (RemoveItemFromBag(pItem)||pLuaItem)	{					
						auto pitemdata=m_stEquip[nPos]->GetItemDataBase();
						if (pitemdata)
							stItemId = pitemdata->nID;
						if(pitemdata && pitemdata->vitemskills.size() && isNeedDeleteSkill(nPos, pitemdata->nID)){
							for(DWORD i=0;i<pitemdata->vitemskills.size();i++){
								stItemSkill itemskill=pitemdata->vitemskills[i];
								m_PlayerObj->DeleteSkill(itemskill.dwSkillId);
							}
						}
						WearItemCmd.nErrorCode = ITEM_SUCCESS;
						m_PlayerObj->SendMsgToMe(&WearItemCmd,sizeof(WearItemCmd));//提前发包 以方便客户端判断属性高低
						SuitType = m_stEquip[nPos]->GetSuitType();
						m_stEquip[nPos]->m_Item.Location = pItem->m_Item.Location;
						m_BagPacket[m_stEquip[nPos]->m_Item.Location.btTableID].AddItemToPack(m_stEquip[nPos], m_stEquip[nPos]->m_Item.Location.btIndex);
						// 脱下时候，触发脱装备
						DWORD dwSuitType = m_stEquip[nPos]->GetSuitType();
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerTakeOffEquip", m_PlayerObj, nPos, dwSuitType, stItemId, m_stEquip[nPos]);

						m_stEquip[nPos]=pItem;
						m_stEquip[nPos]->m_Item.Location.btLocation=ITEMCELLTYPE_EQUIP;
						m_stEquip[nPos]->m_Item.Location.btTableID =0 ;
						m_stEquip[nPos]->m_Item.Location.btIndex=nPos ;
						pitemdata = m_stEquip[nPos]->GetItemDataBase();
						if(pitemdata && pitemdata->vitemskills.size()){
							for(DWORD i=0;i<pitemdata->vitemskills.size();i++){
								stItemSkill itemskill=pitemdata->vitemskills[i];
								m_PlayerObj->StudySkill(itemskill.dwSkillId, itemskill.btSkillLevel);
							}
						}
						SendEquipAddAndUpdateItem(m_stEquip[nPos]);
						m_PlayerObj->SetAbilityFlag(ABILITY_FLAG_EQUIP);
						m_PlayerObj->TriggerEvent(m_PlayerObj,EQUIPITEM, 1);
						//m_PlayerObj->TriggerEvent(m_PlayerObj,ITEMGET,m_stEquip[nPos]->m_Item.dwBaseID);
					}else  WearItemCmd.nErrorCode = ITEM_FAIL_NOITEM;
				}else{
					if (RemoveItemFromBag(pItem)||pLuaItem){
						m_stEquip[nPos]=pItem;
						m_stEquip[nPos]->m_Item.Location.btLocation=ITEMCELLTYPE_EQUIP;
						m_stEquip[nPos]->m_Item.Location.btTableID = 0;
						m_stEquip[nPos]->m_Item.Location.btIndex=nPos;
						WearItemCmd.destLocation=m_stEquip[nPos]->m_Item.Location;
						auto pitemdata = m_stEquip[nPos]->GetItemDataBase();
						if(pitemdata && pitemdata->vitemskills.size()){
							for(DWORD i=0;i<pitemdata->vitemskills.size();i++){
								stItemSkill itemskill=pitemdata->vitemskills[i];
								m_PlayerObj->StudySkill(itemskill.dwSkillId, itemskill.btSkillLevel);
							}
						}
						
						WearItemCmd.nErrorCode = ITEM_SUCCESS;
						m_PlayerObj->SendMsgToMe(&WearItemCmd,sizeof(WearItemCmd));

						SendEquipAddAndUpdateItem(m_stEquip[nPos]);
						m_PlayerObj->SetAbilityFlag(ABILITY_FLAG_EQUIP);
						m_PlayerObj->TriggerEvent(m_PlayerObj,EQUIPITEM, 1);
						//m_PlayerObj->TriggerEvent(m_PlayerObj,ITEMGET,m_stEquip[nPos]->m_Item.dwBaseID);
					}else  WearItemCmd.nErrorCode = ITEM_FAIL_NOITEM;
				}
		}else  WearItemCmd.nErrorCode=ITEM_FAIL_NOCDTIME;	//物品技能没有冷却

	}
	if(WearItemCmd.nErrorCode!=ITEM_SUCCESS){
		if (WearItemCmd.nErrorCode != ITEM_MILITARYRANK)
		{
			m_PlayerObj->SendMsgToMe(&WearItemCmd, sizeof(WearItemCmd));
		}
		return false;
	}else{
		auto itemBase = pItem->GetItemDataBase();
		if (!itemBase) return false;
		switch (nPos)
		{
		case EQUIP_WEAPONS:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::weapon);
			m_PlayerObj->UpdateAppearance(FeatureIndex::weapon, id);
			id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::dress, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::dress, id);
			id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::helmet, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::helmet, id);
			id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::pants, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::pants, id);
			id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::shoe, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::shoe, id);
		}break;
		case EQUIP_CLOTHES:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId",0, m_PlayerObj, FeatureIndex::dress, itemBase);
			m_PlayerObj->UpdateAppearance(FeatureIndex::dress, id);
		}break;
		case EQUIP_HELMET:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::helmet, itemBase);
			m_PlayerObj->UpdateAppearance(FeatureIndex::helmet, id);

		}break;
		case EQUIP_PANTS:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::pants, itemBase);
			m_PlayerObj->UpdateAppearance(FeatureIndex::pants, id);
		}break;
		case EQUIP_SHOES:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::shoe, itemBase);
			m_PlayerObj->UpdateAppearance(FeatureIndex::shoe, id);
		}break;
		case MECHANICAL_HELMET:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::helmet, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::helmet, id);
		}break;
		case MECHANICAL_BODY:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::dress, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::dress, id);
		}break;
		case MECHANICAL_LEG:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::pants, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::pants, id);
		}break;
		case MECHANICAL_ARM:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::shoe, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::shoe, id);
		}break;
		case MECHANICAL_GUARD:
		{
			int id = m_PlayerObj->GetGuardFeature();
			m_PlayerObj->UpdateAppearance(FeatureIndex::guard, id);
		}break;
		default:
			break;
		}
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerWearEquip", m_PlayerObj, m_stEquip[nPos], SuitType);
		return true;
	}

	return false;
}
bool CPlayerPackage::isNeedDeleteSkill(int nPos,int nId) {
	if (nPos == EQUIP_RING_LEFT || nPos == EQUIP_RING_RIGHT) {
		nPos = (nPos == EQUIP_RING_LEFT) ? EQUIP_RING_RIGHT : EQUIP_RING_LEFT;
		if (!m_stEquip[nPos]) return false;
		auto itemBase = m_stEquip[nPos]->GetItemDataBase();
		if (!itemBase) return false;
		if (itemBase->nID == nId) {
			return false;
		}
	}
	return true;
}
int CPlayerPackage::getRealEquipStation(int nPos){
	return nPos;
}

int CPlayerPackage::ServerGetWearItem_PanDuan(CItem*pItem,int nPos,bool boLuaWear){
	if (pItem){
		if (!pItem->CheckJobType(m_PlayerObj->m_siFeature.job)) {
			return ITEM_FAIL_NOJOB;//职业错误
		}
		auto itemBase = pItem->GetItemDataBase();
		if (!itemBase)
		{
			return ITEM_FAIL_UKNOWN;
		}
		if (!(m_PlayerObj->checkFirstLvAtt(pItem))) {
			return ITEM_FAIL_NOTDRESS;
		}
		if (!pItem->GetOutLock()){
			if (!m_PlayerObj->isDie()||boLuaWear){
				bool boPlayer = true;
				if (itemBase->btSexType==NO_SEX 
					|| itemBase->btSexType == m_PlayerObj->m_siFeature.sex){
						if (itemBase->dwType == ITEM_TYPE_EQUIP){
							if (nPos>= EQUIP_WEAPONS && nPos<EQUIP_MAX_COUNT){
								if (itemBase->btEquipStation == (DWORD)nPos || itemBase->btEquipStation == getRealEquipStation(nPos)
									|| ((itemBase->btEquipStation==EQUIP_RING_LEFT || itemBase->btEquipStation==EQUIP_RING_RIGHT) 
										&& (nPos==EQUIP_RING_LEFT || nPos==EQUIP_RING_RIGHT))
									){
									if (pItem->GetWearLevel() <= m_PlayerObj->m_dwLevel){
										return ITEM_SUCCESS;
									}else  return  ITEM_FAIL_LOWLEVEL; //佩戴等级不足								
								}else  return ITEM_FAIL_WRONG_POSITION;//位置错误
							}else return ITEM_FAIL_WRONG_POSITION;//位置错误
						}else  return ITEM_FAIL_WRONG_TYPE;//类型错误
				}else  return ITEM_FAIL_WRONG_SEX;//性别错误
			}else return ITEM_USER_DIE;
		}else return ITEM_FAIL_LOCKED;//装备已锁定
	}else return ITEM_FAIL_NOITEM;//没有物品
}



int CPlayerPackage::ServerGetTakeOffItem( int64_t i64SrcId,stItemLocation &srcLocation,stItemLocation &destLocation,bool boForce)       //摘下装备
{
	FUNCTION_BEGIN;
	int nRet = 0;
	DWORD dwSuitType = 0;
	int stItemId = 0;
	CItem* pItem = nullptr;
	if (!m_PlayerObj->isDie()){
		if (srcLocation.btIndex >= EQUIP_WEAPONS && srcLocation.btIndex < EQUIP_MAX_COUNT){
			if (m_stEquip[srcLocation.btIndex]){
				pItem = m_stEquip[srcLocation.btIndex];
				if (destLocation.btTableID>=0 && destLocation.btTableID<m_nOpenBagCount){
					bool boItemSkillCool=true;					
					auto pitemdata=m_stEquip[srcLocation.btIndex]->GetItemDataBase();
					stItemId = pitemdata->nID;
					if(pitemdata && pitemdata->vitemskills.size()){
						for (DWORD i=0;i<pitemdata->vitemskills.size();i++){
							stItemSkill itemskill=pitemdata->vitemskills[i];
							stMagic* pmagic = m_PlayerObj->m_cMagic.findskill(itemskill.dwSkillId);
							if (pmagic && !pmagic->isCooling()) {
								boItemSkillCool = false;
							}
						}
					}
					if(boItemSkillCool){	//冷却后才能取下
						int nRetX;
						
							if (m_BagPacket[destLocation.btTableID].FindFreeSpace(destLocation.btIndex, nRetX)) {
								destLocation.btIndex = nRetX;
								m_BagPacket[destLocation.btTableID].AddItemToPack(m_stEquip[srcLocation.btIndex], nRetX, false);
								auto pitemdata = m_stEquip[srcLocation.btIndex]->GetItemDataBase();
								if (pitemdata && pitemdata->vitemskills.size() && isNeedDeleteSkill(srcLocation.btIndex, pitemdata->nID)) {
									for (DWORD i = 0; i < pitemdata->vitemskills.size(); i++) {
										stItemSkill itemskill = pitemdata->vitemskills[i];
										m_PlayerObj->DeleteSkill(itemskill.dwSkillId);
									}
								}
								dwSuitType = m_stEquip[srcLocation.btIndex]->GetSuitType();
								m_stEquip[srcLocation.btIndex] = NULL;
								m_PlayerObj->SetAbilityFlag(ABILITY_FLAG_EQUIP);
									
								m_PlayerObj->TriggerEvent(m_PlayerObj, EQUIPITEM, 1);
								
							}
							else  nRet = ITEM_FAIL_FULL;  //包裹没有空地
			
					}else nRet=ITEM_FAIL_NOCDTIME;	//物品技能没有冷却
				}else nRet = ITEM_FAIL_WRONGBAG; //请求脱下来的包裹序号错误
			}else nRet = ITEM_FAIL_NOITEM;//身上没该装备
		}else nRet = ITEM_FAIL_WRONG_POSITION;//请求脱下来的装备位置错误
	}else nRet = ITEM_USER_DIE;

	if (nRet==0){ 
		switch (srcLocation.btIndex)
		{
		case EQUIP_WEAPONS:
		{
			m_PlayerObj->UpdateAppearance(FeatureIndex::weapon, 0);
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::dress, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::dress, id);
			id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::helmet, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::helmet, id);
			id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::pants, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::pants, id);
			id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::shoe, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::shoe, id);
		}break;
		case EQUIP_CLOTHES:
		case MECHANICAL_BODY:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::dress, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::dress, id);
		}break;
		case EQUIP_HELMET:
		case MECHANICAL_HELMET:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::helmet, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::helmet, id);

		}break;
		case EQUIP_PANTS:
		case MECHANICAL_LEG:
		{
			int id = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call("GetFeatureId", 0, m_PlayerObj, FeatureIndex::pants, nullptr);
			m_PlayerObj->UpdateAppearance(FeatureIndex::pants, id);
		}break;
		case EQUIP_SHOES:
		{
			m_PlayerObj->UpdateAppearance(FeatureIndex::shoe, 0);
		}break;
		case MECHANICAL_GUARD:
		{
			m_PlayerObj->UpdateAppearance(FeatureIndex::guard, 0);
		}break;
		default:
			break;
		}
		CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("PlayerTakeOffEquip",m_PlayerObj, srcLocation.btIndex, dwSuitType, stItemId, pItem);
	}
	return nRet;
}

int CPlayerPackage::ServerGetGetBack(int64_t i64Id,stItemLocation &srcLocation,stItemLocation &destLocation,bool boForce)
{
	FUNCTION_BEGIN;
	CItem* pItem =NULL;
	int nStorageId=0;
	for (nStorageId=0;nStorageId<m_nOpenStorageCount;nStorageId++)
	{
		pItem = m_StoragePacket[nStorageId].FindItem(i64Id);
		if (pItem)
		{
			break;
		}
	}
	int nBagId = destLocation.btTableID;
	int nIndex = destLocation.btIndex;
	int nRet = 0;
	if (pItem)
	{
		auto itemBase = pItem->GetItemDataBase();
		if (itemBase && nBagId>=0 && nBagId< m_nOpenBagCount)
		{
			if (m_BagPacket[nBagId].m_boOpened)
			{
				if (m_BagPacket[nBagId].m_PackItemList.size()<m_BagPacket[nBagId].m_dwMaxCount )
				{
					if (CheckCanAddToBag(pItem))
					{
						int nBaseID = pItem->GetItemBaseID();
						int nLoc = pItem->m_Item.Location.btIndex;
						if (m_BagPacket[nBagId].AddItemToPack(pItem,nIndex,false,true,false,"outstorage"))
						{
							m_StoragePacket[nStorageId].RemoveFromPack(nBaseID,i64Id,nLoc);
							if(itemBase == NULL){	//新的方式指针没有释放,只是重新初始化了
								return -1;
							}else{
								destLocation.btIndex = pItem->m_Item.Location.btIndex;
							}
							nRet =  ITEM_SUCCESS;
						}else
						{
							nRet =  ITEM_FAIL_FULL;
						}
					}else nRet = ITEM_FAIL_BAG_FULL;
				}else nRet =  ITEM_FAIL_BAG_FULL;
			}else nRet =  ITEM_FAIL_NOTOPENED;
		}else nRet =  ITEM_FAIL_WRONGBAG;
	}else nRet = ITEM_FAIL_NOITEM;

	return nRet;
}

int  CPlayerPackage::ServerGetStorage(int64_t i64Id,stItemLocation &srcLocation,stItemLocation &destLocation,bool boForce)
{
	FUNCTION_BEGIN;
	CItem* pItem =NULL;
	int nBagId=0;
	for (nBagId=0;nBagId<m_nOpenBagCount;nBagId++)
	{
		pItem = m_BagPacket[nBagId].FindItem(i64Id);
		if (pItem)
		{
			break;
		}
	}
	int nRet = 0;
	int nStorageId = destLocation.btTableID;
	int nIndex = destLocation.btIndex;
	if (pItem)
	{
		auto itemBase = pItem->GetItemDataBase();
		if (itemBase && !pItem->GetOutLock())
		{
			if (nStorageId>=0 && nStorageId< m_nOpenStorageCount && nStorageId< m_nOpenStorageCount)
			{
				if (m_StoragePacket[nStorageId].m_boOpened)
				{
					if (m_StoragePacket[nStorageId].m_PackItemList.size()<m_StoragePacket[nStorageId].m_dwMaxCount )
					{
							int nBaseID = pItem->GetItemBaseID();
							int nLoc = pItem->m_Item.Location.btIndex;
							if (m_StoragePacket[nStorageId].AddItemToPack(pItem,nIndex, true,true,false,"instorage"))
							{  //pItem可能被回收,使用之前先进行判断
								if (m_BagPacket[nBagId].RemoveFromPack(nBaseID,i64Id,nLoc,false)){
									CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("itemremove", m_PlayerObj, pItem->GetItemBaseID());
								}

								if(itemBase = NULL){	//新的方式指针没有释放,只是重新初始化了
									return -1;
								}else{
									destLocation.btIndex = pItem->m_Item.Location.btIndex;
								}

								nRet= ITEM_SUCCESS;
							}
						
					}else nRet= ITEM_FAIL_FULL;
				}else nRet= ITEM_FAIL_NOTOPENED;
			}else  nRet= ITEM_FAIL_WRONGSTORAGE;
		}else  nRet= ITEM_FAIL_LOCKED;
	}else nRet= ITEM_FAIL_NOITEM;

	return nRet;
}

int CPlayerPackage::ServerBagItemToTmpBag(int64_t i64Id, stItemLocation &srcLocation, stItemLocation &destLocation, bool boForce /*= false*/)
{
	FUNCTION_BEGIN;
	CItem* pItem = NULL;
	int nBagId = 0;
	for (nBagId = 0; nBagId < m_nOpenBagCount; nBagId++)
	{
		pItem = m_BagPacket[nBagId].FindItem(i64Id);
		if (pItem)
		{
			break;
		}
	}
	int nRet = 0;
	int nTmpBagId = destLocation.btTableID;
	int nIndex = destLocation.btIndex;
	if (pItem)
	{
		auto itemBase = pItem->GetItemDataBase();
		if (itemBase && !pItem->GetOutLock())
		{
			if (nTmpBagId >= 0 && nTmpBagId < m_nOpenTmpBagCount)
			{
				if (m_TmpBagPacket[nTmpBagId].m_boOpened)
				{
					if (itemBase->btCanPutTmpBag == 0) {
						return ITEM_FAIL_CANNOTSHORTBAR;
					}
					if (m_TmpBagPacket[nTmpBagId].m_PackItemList.size() < m_TmpBagPacket[nTmpBagId].m_dwMaxCount)
					{
						int nBaseID = pItem->GetItemBaseID();
						int nLoc = pItem->m_Item.Location.btIndex;
						if (m_TmpBagPacket[nTmpBagId].AddItemToPack(pItem, nIndex, true, true, "intmpbag"))
						{  //pItem可能被回收,使用之前先进行判断
							if (m_BagPacket[nBagId].RemoveFromPack(nBaseID, i64Id, nLoc, false)) {
							}

							if (itemBase == NULL) {	//新的方式指针没有释放,只是重新初始化了
								return -1;
							}
							else {
								destLocation.btIndex = pItem->m_Item.Location.btIndex;
							}

							nRet = ITEM_SUCCESS;
						}

					}
					else nRet = ITEM_FAIL_FULL;
				}
				else nRet = ITEM_FAIL_NOTOPENED;
			}
			else  nRet = ITEM_FAIL_WRONGSTORAGE;
		}
		else  nRet = ITEM_FAIL_LOCKED;
	}
	else nRet = ITEM_FAIL_NOITEM;

	return nRet;
}

int CPlayerPackage::ServerTmpBagItemToBag(int64_t i64Id, stItemLocation &srcLocation, stItemLocation &destLocation, bool boForce /*= false*/)
{
	FUNCTION_BEGIN;
	CItem* pItem = NULL;
	int nTmpBagId = 0;
	for (nTmpBagId = 0; nTmpBagId < m_nOpenTmpBagCount; nTmpBagId++)
	{
		pItem = m_TmpBagPacket[nTmpBagId].FindItem(i64Id);
		if (pItem)
		{
			break;
		}
	}
	int nBagId = destLocation.btTableID;
	int nIndex = destLocation.btIndex;
	int nRet = 0;
	if (pItem)
	{
		if (nBagId >= 0 && nBagId < m_nOpenBagCount)
		{
			if (m_BagPacket[nBagId].m_boOpened)
			{
				if (m_BagPacket[nBagId].m_PackItemList.size() < m_BagPacket[nBagId].m_dwMaxCount)
				{
					if (CheckCanAddToBag(pItem))
					{
						int nBaseID = pItem->GetItemBaseID();
						int nLoc = pItem->m_Item.Location.btIndex;
						if (m_BagPacket[nBagId].AddItemToPack(pItem, nIndex, true, true, "outtmpbag"))
						{
							m_TmpBagPacket[nTmpBagId].RemoveFromPack(nBaseID, i64Id, nLoc);
							destLocation.btIndex = pItem->m_Item.Location.btIndex;
							nRet = ITEM_SUCCESS;
						}
						else
						{
							nRet = ITEM_FAIL_FULL;
						}
					}
					else nRet = ITEM_FAIL_BAG_FULL;
				}
				else nRet = ITEM_FAIL_BAG_FULL;
			}
			else nRet = ITEM_FAIL_NOTOPENED;
		}
		else nRet = ITEM_FAIL_WRONGBAG;
	}
	else nRet = ITEM_FAIL_NOITEM;

	return nRet;
}

void  CPlayerPackage::ServerGetUseItem(int64_t i64Id,DWORD dwCretTmpId,DWORD dwTmepID)                           //使用物品
{
	FUNCTION_BEGIN;
	stCretUseItemRet useitemretcmd;						//0x0315
	useitemretcmd.i64id = i64Id;
	useitemretcmd.btErrorCode =ITEM_FAIL_NONEED;
	bool boNeedDel = false;  //使用完是否删除道具
	CItem* pItem = FindItemInBag(i64Id);
	if (!pItem){pItem = FindItemInBody(i64Id);}
	if (!pItem) { pItem = FindItemInTmpBag(i64Id); }
	if (!pItem) { pItem = FindItemInActBag(i64Id); }
	if(!pItem){
		g_logger.debug("no find item %I64d",i64Id);
	}
	if (pItem)
	{
		char szfmdis[256];
		sprintf_s(szfmdis,sizeof(szfmdis)-1,"CPlayerPackage::ServerGetUseItem() i64id %I64d %s",i64Id,pItem->GetItemName());
		FUNCTION_MONITOR(48,szfmdis);
		auto itemBase = pItem->GetItemDataBase();

		if (!itemBase) return;
		if (!pItem->GetOutLock())
		{   //道具使用者必须是活的
			if ((m_PlayerObj->GetObjectId() == dwTmepID && !m_PlayerObj->isDie()))
			{
				CCreature* pTarget = NULL;
				if (dwCretTmpId==0 || dwCretTmpId == m_PlayerObj->GetObjectId() ||itemBase->dwType== ITEM_TYPE_RESPONSEE) {
					pTarget = m_PlayerObj;
				}
				if (pTarget)
				{
					if ((pTarget->isPlayer() && ((pTarget->m_dwLevel>= itemBase->dwNeedLevel))
						&& 	(pItem->IfLimitedTime()==false	)))//限时物品,未到期,才能使用
					{
						if (pItem->GetCanUseTick()){
							int privilegelv=m_PlayerObj->m_nVipLevel;
							if(itemBase->nPrivilegeUseLimit && privilegelv<itemBase->nPrivilegeUseLimit){
								CPlayerObj::sendTipMsgByXml(m_PlayerObj,GameService::getMe().GetStrRes(3,"bag"));
								return;
							}
							DWORD dwTikc = itemBase->dwCdByTick; // CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<DWORD>("useItemCdHandle", 0, m_PlayerObj, itemBase->dwType, pItem, itemBase->dwCdByTick);
							pItem->SetCanUseTick(dwTikc);
							switch (itemBase->dwType)
							{
							case ITEM_TYPE_DRUG: //药品，会响应“使用”按钮，点使用后吃掉并增加药效
								{
									ULONGLONG dwCurTick=GetTickCount64();
									switch (itemBase->btShareCdType)
									{
									case CD_TYPE_Drug:
									{
										if (dwCurTick >= m_dwLastDrugShareTick) {
											if (itemBase->dwScriptid)
											{
												m_PlayerObj->m_pCurrItem = pItem;
												m_PlayerObj->m_i64CurrItemTmpId = pItem->GetItemID();
												char func_name[QUEST_FUNC_LEN];
												sprintf(func_name, "%s_%d", "itemuse", itemBase->dwScriptid);
												stAutoSetScriptParam autoparam(m_PlayerObj);
												bool boUse = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>(func_name, pItem, true);
												if (boUse)
												{
													int nShareCd = m_PlayerObj->GetDrugShareCd();
													m_dwLastDrugShareTick = dwCurTick + nShareCd;
													useitemretcmd.dwCdTime = nShareCd;
													useitemretcmd.btCdType = itemBase->btShareCdType;
													useitemretcmd.btErrorCode = ITEM_SUCCESS;
												}
												else
												{
													useitemretcmd.btErrorCode = ITEM_FAIL_FAILNOTIPS;
												}
												m_PlayerObj->m_pCurrItem = NULL;
											}
											else useitemretcmd.btErrorCode = ITEM_DRUG_NODRUG;
										}
										else useitemretcmd.btErrorCode = ITEM_FAIL_CDTIMING;
									}break;
									default:
									{
										useitemretcmd.btErrorCode = ITEM_DRUG_NODRUG;
									}break;
									}
								}break;
							case ITEM_TYPE_RESPONSEE:
								{
									if (itemBase->dwScriptid)
									{
										m_PlayerObj->m_pCurrItem=pItem;
										m_PlayerObj->m_i64CurrItemTmpId=pItem->GetItemID();
										char func_name[QUEST_FUNC_LEN];
										sprintf(func_name, "%s_%d", "itemuse", itemBase->dwScriptid);
										stAutoSetScriptParam autoparam(m_PlayerObj);
										CCreature * pLastNpc= m_PlayerObj->m_pVisitNPC;
										m_PlayerObj->m_pVisitNPC= NULL;
										bool boUse = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>(func_name, pItem, true);
										if (boUse)
										{
											useitemretcmd.dwCdTime = itemBase->dwCdByTick;
											useitemretcmd.btCdType = itemBase->btShareCdType;
											useitemretcmd.btErrorCode = ITEM_SUCCESS;
										}
										else
											useitemretcmd.btErrorCode = ITEM_FAIL_FAILNOTIPS;
										
										if (!m_PlayerObj->m_pVisitNPC || (m_PlayerObj->m_pVisitNPC && m_PlayerObj->m_pVisitNPC->m_dwScriptId!=_SYSTEM_NOCHECK_SCRIPTID_)){
											m_PlayerObj->m_pVisitNPC=pLastNpc;
										}
										m_PlayerObj->m_pCurrItem=NULL;
									}
								}break;
							default:
								{
									useitemretcmd.btErrorCode = ITEM_FAIL_WRONG_TYPE;
								}break;
							}
						}else useitemretcmd.btErrorCode = ITEM_FAIL_CDTIMING;
					}else  useitemretcmd.btErrorCode = ITEM_FAIL_USELOWLEVEL;
				}else
				{
					if (useitemretcmd.btErrorCode ==ITEM_FAIL_NONEED)
					{
						useitemretcmd.btErrorCode = ITEM_FAIL_NOTFIND_TARGET;
					}	
				}
			}else useitemretcmd.btErrorCode = ITEM_USER_DIE;
		}else useitemretcmd.btErrorCode = ITEM_FAIL_LOCKED;
	}else useitemretcmd.btErrorCode = ITEM_FAIL_NOITEM;

	if (useitemretcmd.btErrorCode==ITEM_SUCCESS && pItem && boNeedDel)
	{
		if (!DeleteItemInBag(pItem, 1, true, false, "useitem"))
		{
			DeleteItemInTmpBag(pItem, 1, true, false, "useitem");
		}
	}

	m_PlayerObj->SendMsgToMe(&useitemretcmd,sizeof(useitemretcmd));
}

bool  CPlayerPackage::ServerGetSplitItem(int64_t i64Id,DWORD nCount)                     //拆分
{
	FUNCTION_BEGIN;
	stCretSplitItem splititemcmd;
	if (nCount==0) return true;
	CItem* pItem = FindItemInBag(i64Id);
	if (pItem)
	{
		if (!pItem->GetOutLock())
		{
			if (pItem->GetItemCount()> nCount)
			{
				if (m_BagPacket[pItem->m_Item.Location.btTableID].m_PackItemList.size()< m_BagPacket[pItem->m_Item.Location.btTableID].m_dwMaxCount)
				{
					pItem->SetItemLog("splititem",m_PlayerObj, NULL, false, pItem->GetItemCount() - nCount);
					pItem->SetItemCount(pItem->GetItemCount()-nCount);
					SendBagItemCountChanged(pItem);
					CItem* pNewItem = CItem::LoadItem(&pItem->m_Item,__FUNC_LINE__);
					if (pNewItem)
					{
						pNewItem->m_Item.i64ItemID = CItem::GenerateItemID();
						pNewItem->SetItemCount(nCount);
						pNewItem->SetItemLog("splititemcreatenew",m_PlayerObj, NULL, false, nCount);
						m_BagPacket[pItem->m_Item.Location.btTableID].AddItemToPack(pNewItem,0,true,false);
						splititemcmd.i64Id= i64Id;
						splititemcmd.nCount= nCount;
						splititemcmd.nErrorCode =  ITEM_SUCCESS;
					}else splititemcmd.nErrorCode =  ITEM_FAIL_UKNOWN;
				}else splititemcmd.nErrorCode =  ITEM_FAIL_NOSPACE;
			}else splititemcmd.nErrorCode =  ITEM_FAIL_NOTENOUGH;
		}else  splititemcmd.nErrorCode =  ITEM_FAIL_LOCKED;

	}
	else if (!pItem)
	{
		pItem =  FindItemInStorage(i64Id);
		if (pItem)
		{
			if (pItem->GetItemCount()> nCount)
			{
				if (GetFreeSorageCellCount()>0)
				{
					if (m_StoragePacket[pItem->m_Item.Location.btTableID].m_PackItemList.size()< m_StoragePacket[pItem->m_Item.Location.btTableID].m_dwMaxCount)
					{
						pItem->SetItemLog("splititem",m_PlayerObj, NULL, false, pItem->GetItemCount() - nCount);
						pItem->SetItemCount(pItem->GetItemCount()-nCount);
						SendBagItemCountChanged(pItem);
						CItem* pNewItem = CItem::LoadItem(&pItem->m_Item,__FUNC_LINE__);
						if (pNewItem)
						{
							pNewItem->m_Item.i64ItemID = CItem::GenerateItemID();
							pNewItem->SetItemCount(nCount);
							pNewItem->SetItemLog("useitemcreatenew",m_PlayerObj, NULL, false, nCount);
							m_StoragePacket[pItem->m_Item.Location.btTableID].AddItemToPack(pNewItem,0,true,false);
							splititemcmd.i64Id= i64Id;
							splititemcmd.nCount= nCount;
							splititemcmd.nErrorCode =  ITEM_SUCCESS;
						}else splititemcmd.nErrorCode = ITEM_FAIL_UKNOWN;
					}else splititemcmd.nErrorCode = ITEM_FAIL_NOSPACE;
				}else splititemcmd.nErrorCode = ITEM_FAIL_NOSPACE;
			}else splititemcmd.nErrorCode =  ITEM_FAIL_NOTENOUGH;
		}
	}
	if(!pItem){
		splititemcmd.nErrorCode = ITEM_FAIL_NOITEM;
	}
	m_PlayerObj->SendMsgToMe(&splititemcmd,sizeof(splititemcmd));

	return splititemcmd.nErrorCode == ITEM_SUCCESS;
}
void  CPlayerPackage::ServerGetDestoryItem(int64_t i64Id)                             //分解物品
{
	FUNCTION_BEGIN;
	bool ItemIsDel = false;
	CItem* pItem = FindItemInBag(i64Id);
	if (pItem && !pItem->GetOutLock() && pItem->CanDestory())
	{
		pItem->SetItemLog("destoryitem",m_PlayerObj, NULL, false, pItem->GetItemCount());
		DWORD dwBaseId=pItem->m_Item.dwBaseID;
		if (DeleteItemInBag(pItem))
		{
			SendBagDeleteItem(i64Id,dwBaseId);
			ItemIsDel = true;
		}
	}
	else 
	{
		pItem = FindItemInStorage(i64Id);
		if (pItem && !pItem->GetOutLock() && pItem->CanDestory())
		{
			pItem->SetItemLog("destoryitem",m_PlayerObj, NULL, false, pItem->GetItemCount());
			if (DeleteItemInStorage(pItem))
			{
				SendStorageDeleteItem(i64Id);
				ItemIsDel = true;
			}
		}
	}
	if (!ItemIsDel) {
		pItem = FindItemInTmpBag(i64Id);
		if (pItem && !pItem->GetOutLock() && pItem->CanDestory())
		{
			pItem->SetItemLog("destoryitem", m_PlayerObj, NULL, false, pItem->GetItemCount());
			DWORD dwBaseId = pItem->m_Item.dwBaseID;
			if (DeleteItemInTmpBag(pItem))
			{
				SendTmpBagDeleteItem(i64Id, dwBaseId);
			}
		}
	}
}
void  CPlayerPackage::ServerGetExchangeItemEx(CItem* pItem,BYTE btLocation,BYTE btTableID,BYTE  btIndex){
	stItemLocation location;
	location.btIndex = btIndex;
	location.btTableID = btTableID;
	location.btLocation = btLocation;
	ServerGetExchangeItem(pItem->GetItemID(),location);
}

void  CPlayerPackage::ServerGetExchangeItem(int64_t i64Id,stItemLocation location)
{
	FUNCTION_BEGIN;
	switch (location.btLocation)
	{
	case ITEMCELLTYPE_STORE:
		{
			CItem* pItem=FindItemInStorage(i64Id);
			if (pItem)
			{
				if ((location.btTableID>=0 && location.btTableID<m_nOpenStorageCount && location.btTableID <m_nOpenStorageCount ))
				{
					CItem* pFindItem =m_StoragePacket[location.btTableID].FindItemByLocation(location.btIndex); //找到原来的物品
					if (pFindItem)
					{
						if (m_StoragePacket[location.btTableID].RemoveFromPack(pFindItem->GetItemBaseID(),pFindItem->GetItemID(),pFindItem->m_Item.Location.btIndex))
						{
							if (m_StoragePacket[pItem->m_Item.Location.btTableID].RemoveFromPack(pItem->GetItemBaseID(),i64Id,pItem->m_Item.Location.btIndex))
							{
								if (!m_StoragePacket[location.btTableID].AddItemToPack(pFindItem,pItem->m_Item.Location.btIndex,true,false))
								{
									AddItemToStorage(pFindItem);
								}
								if (!m_StoragePacket[location.btTableID].AddItemToPack(pItem,location.btIndex,true,false))
								{
									AddItemToStorage(pItem);
								}

							}else
							{
								AddItemToStorage(pFindItem);
							}
						}
					}
					else
					{
						if (m_StoragePacket[pItem->m_Item.Location.btTableID].RemoveFromPack(pItem->GetItemBaseID(),i64Id,pItem->m_Item.Location.btIndex))
						{
							if (!m_StoragePacket[location.btTableID].AddItemToPack(pItem,location.btIndex,true,false))
							{
								AddItemToStorage(pItem);
							}
						}
					}
				}
			}
		}break;
	case ITEMCELLTYPE_PACKAGE:
		{
			CItem* pItem=FindItemInBag(i64Id);
			if (pItem && !pItem->GetOutLock())
			{
				if ((location.btTableID>=0 && location.btTableID<m_nOpenBagCount && location.btTableID <m_nOpenBagCount ) || ( location.btTableID == 4) || ( location.btTableID == 5) )
				{
					CItem* pFindItem =m_BagPacket[location.btTableID].FindItemByLocation(location.btIndex); //找到原来的物品
					if (pFindItem&& !pFindItem->GetOutLock())
					{
						if (m_BagPacket[location.btTableID].RemoveFromPack(pFindItem->GetItemBaseID(),pFindItem->GetItemID(),pFindItem->m_Item.Location.btIndex))
						{
							if (m_BagPacket[pItem->m_Item.Location.btTableID].RemoveFromPack(pItem->GetItemBaseID(),pItem->GetItemID(),pItem->m_Item.Location.btIndex))
							{
								if (!m_BagPacket[location.btTableID].AddItemToPack(pFindItem,pItem->m_Item.Location.btIndex,true,false))
								{	
									AddItemToBag(pFindItem);
								}
								if (!m_BagPacket[location.btTableID].AddItemToPack(pItem,location.btIndex,true,false))
								{
									AddItemToBag(pItem);
								}
							}else
							{
								AddItemToBag(pFindItem);
							}
						}
					}
					else
					{
						if (m_BagPacket[pItem->m_Item.Location.btTableID].RemoveFromPack(pItem->GetItemBaseID(),pItem->GetItemID(),pItem->m_Item.Location.btIndex))
						{
							if (!m_BagPacket[location.btTableID].AddItemToPack(pItem,location.btIndex,true,false))
							{
								AddItemToBag(pItem);
							}
						}
					}
				}
			}
		}break;
	case ITEMCELLTYPE_TMPPACKAGE:
		{
			CItem* pItem = FindItemInTmpBag(i64Id);
			if (pItem && !pItem->GetOutLock())
			{
				if ((location.btTableID >= 0 && location.btTableID < m_nOpenTmpBagCount))
				{
					CItem* pFindItem = m_TmpBagPacket[location.btTableID].FindItemByLocation(location.btIndex); //找到原来的物品
					if (pFindItem && !pFindItem->GetOutLock())
					{
						if (m_TmpBagPacket[location.btTableID].RemoveFromPack(pFindItem->GetItemBaseID(), pFindItem->GetItemID(), pFindItem->m_Item.Location.btIndex))
						{
							if (m_TmpBagPacket[pItem->m_Item.Location.btTableID].RemoveFromPack(pItem->GetItemBaseID(), pItem->GetItemID(), pItem->m_Item.Location.btIndex))
							{
								if (!m_TmpBagPacket[location.btTableID].AddItemToPack(pFindItem, pItem->m_Item.Location.btIndex, true, false))
								{
									AddItemToTmpBag(pFindItem);
								}
								if (!m_TmpBagPacket[location.btTableID].AddItemToPack(pItem, location.btIndex, true, false))
								{
									AddItemToTmpBag(pItem);
								}
							}
							else
							{
								AddItemToTmpBag(pFindItem);
							}
						}
					}
					else
					{
						if (m_TmpBagPacket[pItem->m_Item.Location.btTableID].RemoveFromPack(pItem->GetItemBaseID(), pItem->GetItemID(), pItem->m_Item.Location.btIndex))
						{
							if (!m_TmpBagPacket[location.btTableID].AddItemToPack(pItem, location.btIndex, true, false))
							{
								AddItemToTmpBag(pItem);
							}
						}
					}
				}
			}
		}break;
	}
}

void  CPlayerPackage::ServerGetResortBag()      									   //重新整理背包
{
	FUNCTION_BEGIN;
	if ((DWORD)time(NULL) <= m_dwLastResortBagTime)
	{
		//stPackageError packageerror;
		//packageerror.btErrorCode=ITEM_FAIL_NORESORTBAG;
		//m_PlayerObj->SendMsgToMe(&packageerror,sizeof(packageerror));
		CPlayerObj::sendLuaMsg(m_PlayerObj,0,"opendialog`bag`2");
		return;
	}else m_dwLastResortBagTime = time(NULL)+5;
	/*if (m_PlayerObj->m_Trade.GetTradeState()) 
	{
		CPlayerObj::sendTipMsgByXml(m_PlayerObj,GameService::getMe().GetStrRes(1,"bag"));
		CPlayerObj::sendLuaMsg(m_PlayerObj,0,"opendialog`bag`2");
		return;
	}*/

	std::list<CItem*> TempList;
	std::list<CItem*>::iterator iter;
	DWORD dwType = 0;
	DWORD nTotalCount = 0;
	for (int i=0;i<m_nOpenBagCount;i++){
		for (auto it =  m_BagPacket[i].m_PackItemList.begin();it != m_BagPacket[i].m_PackItemList.end();it ++){
			CItem* pItem= it->second;
			if (pItem){
				if (pItem->GetOutLock())
				{
					TempList.clear();
					return;
				}
				TempList.push_back(pItem);
			}
		}
		nTotalCount += m_BagPacket[i].m_PackItemList.size();
	}
	if (TempList.size()>0)
	{
		TempList.sort(CmpBySortType);

		if (TempList.size()!=nTotalCount )
		{
			TempList.clear();
			g_logger.debug("%s 整理包裹 数量发生变化 取消整理.",m_PlayerObj->getName());
			return;
		}
		for (int i=0;i<m_nOpenBagCount;i++)
		{
			m_BagPacket[i].m_PackItemList.clear();
			m_BagPacket[i].m_itemsByBaseId.clear();
			m_BagPacket[i].m_vUsed.clear();
			m_BagPacket[i].m_vUsed.resize(m_BagPacket[i].m_dwMaxCount);
		}
		m_vHpItemMap.clear();


		for (iter = TempList.begin();iter != TempList.end();iter++)
		{
			CItem* pItem = *iter;
			if (pItem)
			{
				bool boAdd = false;
				if (!boAdd)
				{
					for (int i=0;i<m_nOpenBagCount;i++)
					{
						if (m_BagPacket[i].AddItemToPack(pItem,0,false))
						{
							break;
						}
					}
				}
			}
		}
		TempList.clear();
	}

	SendBagItems();
	CPlayerObj::sendLuaMsg(m_PlayerObj,0,"opendialog`bag`3");
}


void  CPlayerPackage::ServerGetResortStorage(BYTE btTabId)                                         //重新整理仓库
{
	FUNCTION_BEGIN;
	if(btTabId> m_nOpenStorageCount){
		return;
	}

	if ((DWORD)time(NULL) <= m_dwLastResortSrotageTime)
	{
		stPackageError packageerror;
		packageerror.btErrorCode=ITEM_FAIL_NORESORTSTORAGE;
		m_PlayerObj->SendMsgToMe(&packageerror,sizeof(packageerror));
		return;
	}
	else m_dwLastResortSrotageTime = time(NULL)+10;

	std::list<CItem*> TempList;
	std::list<CItem*>::iterator iter;
	DWORD dwType = 0;

	for (auto it =  m_StoragePacket[btTabId].m_PackItemList.begin();it != m_StoragePacket[btTabId].m_PackItemList.end();it ++){
		CItem* pItem= it->second;
		if (pItem){
			TempList.push_back(pItem);
		}
	}
	m_StoragePacket[btTabId].m_PackItemList.clear();
	m_StoragePacket[btTabId].m_itemsByBaseId.clear();
	m_StoragePacket[btTabId].m_vUsed.clear();
	m_StoragePacket[btTabId].m_vUsed.resize(m_StoragePacket[btTabId].m_dwMaxCount);

	TempList.sort(CmpBySortType);

	bool bocontinue = false;
	for (iter = TempList.begin();iter != TempList.end();iter++)
	{
		CItem* pItem = *iter;
		if (pItem)
		{
			if (m_StoragePacket[btTabId].AddItemToPack(pItem,0,false))
			{
			}
		}
	}
	TempList.clear();
	SendStorageItems();
}


void  CPlayerPackage::ServerGetOpenNewBagPage()                                        //打开新的包裹
{
	FUNCTION_BEGIN;
	stCretOpenNew OpenNew;
	OpenNew.btPosition = ITEMCELLTYPE_PACKAGE;
	OpenNew.btNewCount = m_nOpenBagCount;
	m_PlayerObj->SendMsgToMe(&OpenNew,sizeof(OpenNew));
}

void  CPlayerPackage::ServerGetOpenNewStoragePage()//打开新的仓库/ 服务器得到打包新仓库的包
{
	FUNCTION_BEGIN;
	stCretOpenNew OpenNew;
	OpenNew.btPosition = ITEMCELLTYPE_STORE;
	OpenNew.btNewCount = m_nOpenStorageCount;
	m_PlayerObj->SendMsgToMe(&OpenNew,sizeof(OpenNew));
}

//打开新的包裹
void CPlayerPackage::OpenNewBagPage(int nOpenBagCount)
{	
	FUNCTION_BEGIN;
	stCretOpenNew OpenNew;
	OpenNew.btPosition = ITEMCELLTYPE_PACKAGE;
	if (m_nOpenBagCount<3 && nOpenBagCount==3)
	{
		m_nOpenBagCount = 3;
		m_BagPacket[2].Init(m_PlayerObj,ITEMCELLTYPE_PACKAGE,2,25);
		OpenNew.btNewCount = m_nOpenBagCount;
		m_PlayerObj->SendMsgToMe(&OpenNew,sizeof(OpenNew));
	}else if (m_nOpenBagCount<4 && nOpenBagCount==4)
	{
		m_nOpenBagCount = 4;
		m_BagPacket[3].Init(m_PlayerObj,ITEMCELLTYPE_PACKAGE,3,25);
		OpenNew.btNewCount = m_nOpenBagCount;
		m_PlayerObj->SendMsgToMe(&OpenNew,sizeof(OpenNew));
	}
}

//打开新的仓库
void CPlayerPackage::OpenNewStoragePage(int nStrorageCount)
{
	FUNCTION_BEGIN;
	stCretOpenNew OpenNew;
	OpenNew.btPosition = ITEMCELLTYPE_STORE;
	if (m_nOpenStorageCount<3 && nStrorageCount==3)
	{
		m_nOpenStorageCount = 3;
		OpenNew.btNewCount = m_nOpenStorageCount;
		m_StoragePacket[2].Init(m_PlayerObj,ITEMCELLTYPE_STORE,2,25);	
		m_PlayerObj->SendMsgToMe(&OpenNew,sizeof(OpenNew));
	}else if (m_nOpenStorageCount<4 && nStrorageCount==4)
	{
		m_nOpenStorageCount = 4;
		OpenNew.btNewCount = m_nOpenStorageCount;		
		m_StoragePacket[3].Init(m_PlayerObj,ITEMCELLTYPE_STORE,3,25);
		m_PlayerObj->SendMsgToMe(&OpenNew,sizeof(OpenNew));
	}
}

//DWORD CPlayerPackage::GetRepairAllPrices()
//{
//	FUNCTION_BEGIN;
//	DWORD dwRepairGold=0;
//	for (int i=0;i<EQUIP_MAX_COUNT;i++)
//	{
//		if (m_stEquip[i])
//		{
//			dwRepairGold+= m_stEquip[i]->GetRepairPrice();
//		}
//	}
//
//	return dwRepairGold;
//}

void CPlayerPackage::ServerGetRepairAll()
{
	FUNCTION_BEGIN;
	for (int i=0;i<EQUIP_MAX_COUNT;i++)
	{
		if (m_stEquip[i])
		{
			m_stEquip[i]->Repair();
			SendEquipDuraChanged(m_stEquip[i]);
		}
	}
}

void CPlayerPackage::ServerAutoKeepHp(DWORD dwKeepHp,DWORD dwForceKeepHp){
	FUNCTION_BEGIN;
	if (m_PlayerObj && m_PlayerObj->m_boAutoOpenHp && !m_PlayerObj->isDie() && m_PlayerObj->GetEnvir() && !m_PlayerObj->GetEnvir()->isNoUseItem()){
		if (m_PlayerObj->m_nNowHP<m_PlayerObj->m_stAbility[AttrID::MaxHP]){
			if (m_vHpItemMap.size()){
				CItem* pItem;
				std::list<CItem*>::iterator iter;
				ULONGLONG dwCurTick=GetTickCount64();
				if(dwCurTick>= m_dwLastDrugShareTick){
					if (dwForceKeepHp && m_PlayerObj->m_nNowHP<(int64_t)(dwForceKeepHp/100.0*m_PlayerObj->m_stAbility[AttrID::MaxHP])){//强制补血
						pItem=NULL;
						for (iter=m_vHpItemMap.begin();iter!=m_vHpItemMap.end();iter++)
						{
							pItem=(*iter);
							if(pItem){
								std::map<DWORD,DWORD>::iterator it=CUserEngine::getMe().m_mKeepHpSet.find(pItem->GetItemBaseID());
								if(it!=CUserEngine::getMe().m_mKeepHpSet.end() && (m_PlayerObj->m_dwHpSet & it->second)!=0){
									if (pItem->GetCanUseTick() 
										&& pItem->GetType()==ITEM_TYPE_DRUG 
										&& pItem->GetWearLevel()<=m_PlayerObj->m_dwLevel 
										&&  dwCurTick>= m_dwLastDrugShareTick )
									{
										ServerGetUseItem(pItem->GetItemID(),m_PlayerObj->GetObjectId(),m_PlayerObj->GetObjectId());
										break;
									}
								}
							}
						}
					}
					if (dwKeepHp && m_PlayerObj->m_nNowHP<(int64_t)(dwKeepHp/100.0*m_PlayerObj->m_stAbility[AttrID::MaxHP])){//正常补血
						pItem=NULL;
						for (iter=m_vHpItemMap.begin();iter!=m_vHpItemMap.end();iter++)
						{
							pItem=(*iter);
							if(pItem){
								std::map<DWORD,DWORD>::iterator it=CUserEngine::getMe().m_mKeepHpSet.find(pItem->GetItemBaseID());
								if(it!=CUserEngine::getMe().m_mKeepHpSet.end() && (m_PlayerObj->m_dwHpSet & it->second)!=0){
									if (pItem->GetCanUseTick() 
										&& pItem->GetType()==ITEM_TYPE_DRUG 
										&& pItem->GetWearLevel()<=m_PlayerObj->m_dwLevel &&  dwCurTick>= m_dwLastDrugShareTick)
									{
										ServerGetUseItem(pItem->GetItemID(),m_PlayerObj->GetObjectId(),m_PlayerObj->GetObjectId());
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void CPlayerPackage::ServerGetViewEuqip(char* szName,BYTE btType,BYTE btFlag, int64_t i64Id)
{
	FUNCTION_BEGIN;

	BUFFER_CMD(stCretViewEquipRet , pViewRet, stBasePacket::MAX_PACKET_SIZE) ;
	pViewRet->btErrorCode = 0;
	pViewRet->btType=btType;
	pViewRet->btFlag=btFlag;
	if (szName){
		CPlayerObj* player = CUserEngine::getMe().m_playerhash.FindByOnlyId(i64Id);
		if (!player) player = CUserEngine::getMe().m_playerhash.FindByName(szName);
		if (player) {
			pViewRet->simpleAbility = player->m_stAbility;
			pViewRet->btSex = player->m_siFeature.sex;
			pViewRet->btJob = player->m_siFeature.job;
			pViewRet->btFace = player->m_siFeature.face;	//脸部
			pViewRet->btHair = player->m_siFeature.hair;	//头发
			pViewRet->btEye = player->m_siFeature.eye;	//眼睛
			pViewRet->btNose = player->m_siFeature.nose;	//鼻子
			pViewRet->btMouth = player->m_siFeature.mouth;	//嘴巴
			pViewRet->dwLevel = player->m_dwLevel;
			pViewRet->i64FightScore = player->m_stAbility.i64FightScore;
			pViewRet->dwHeadPortrait = player->m_dwHeadPortrait;
			pViewRet->dwDressId = CALL_LUARET<DWORD>("GetFeatureId", 0, player,1); //时装

			strcpy_s(pViewRet->szName, sizeof(pViewRet->szName), player->getName());
			strcpy_s(pViewRet->szClanName, sizeof(pViewRet->szClanName), player->m_GuildInfo.szGuildName);
			if(btType == 0){
				for (int i = 0; i < EQUIP_MAX_COUNT; i++) {
					if (player->m_Packet.m_stEquip[i]) {
						pViewRet->items.push_back(player->m_Packet.m_stEquip[i]->m_Item, __FUNC_LINE__);
					}
				}
			}
		}else 
		{
			stCretViewEquip supercmd;
			supercmd.btType=btType;
			supercmd.btFlag=btFlag;
			supercmd.i64Id = i64Id;
			strcpy_s(supercmd.szName,_MAX_NAME_LEN_,szName);
			{SENDMSG2SUPER(stSendOhterMsgSuperSrv,m_PlayerObj->getName(),&supercmd,sizeof(supercmd));}
			return;
		}
	}else pViewRet->btErrorCode = 1;
	m_PlayerObj->SendMsgToMe(pViewRet, sizeof(stCretViewEquipRet) + pViewRet->items.getarraysize());
}

DWORD   CPlayerPackage::GetFreeBagCellCount(DWORD dwType )
{
	FUNCTION_BEGIN;
	DWORD nResult = 0;
	if (dwType & 1)
	{
		for (int i=0;i<m_nOpenBagCount;i++)
		{
			nResult+=m_BagPacket[i].m_dwMaxCount - m_BagPacket[i].m_PackItemList.size();
		}
	}

	return nResult;

}

DWORD CPlayerPackage::GetFreeTmpBagCellCount(DWORD dwType)
{
	FUNCTION_BEGIN;
	DWORD nResult = 0;
	if (dwType & 1)
	{
		for (int i = 0; i < m_nOpenTmpBagCount; i++)
		{
			nResult += m_TmpBagPacket[i].m_dwMaxCount - m_TmpBagPacket[i].m_PackItemList.size();
		}
	}

	return nResult;
}

DWORD  CPlayerPackage::GetFreeSorageCellCount()
{
	FUNCTION_BEGIN;
	int nResult = 0;
	for (int i=0;i<m_nOpenStorageCount;i++)
	{
		nResult+=m_StoragePacket[i].m_dwMaxCount - m_StoragePacket[i].m_PackItemList.size();
	}
	return nResult;
}

void CPlayerPackage::GetPlayerEquipProper(stARpgAbility* pattr)                     //获得装备总属性
{
	FUNCTION_BEGIN;
	if (pattr && m_PlayerObj) {
		pattr->Clear();
		stARpgAbility temppattr;
		for (int i = 0; i < EQUIP_MAX_COUNT; i++) {
			if (m_stEquip[i] && !m_stEquip[i]->GetBroken() && (m_stEquip[i]->GetDura() > 0)) {
				m_stEquip[i]->GetItemProperty(m_PlayerObj, i, temppattr);
				(*pattr) += temppattr;
			}
		}
	}
}
void CPlayerPackage::GetPlayerEquipSpecialProper(stSpecialAbility* pattr)                     //获得装备总属性
{
	FUNCTION_BEGIN;
	if (pattr && m_PlayerObj) {
		pattr->Clear();
		stSpecialAbility temppattr;
		for (int i = 0; i < EQUIP_MAX_COUNT; i++) {
			if (m_stEquip[i] && !m_stEquip[i]->GetBroken() && (m_stEquip[i]->GetDura() > 0)) { // 判断物品是否损坏
				m_stEquip[i]->GetItemSpecialProperty(m_PlayerObj, i, &temppattr);
				(*pattr) += temppattr;
			}
		}
	}
}

CItem* CPlayerPackage::GetItemMsg(int64_t i64Id)
{
	FUNCTION_BEGIN;
	CItem *pItem = NULL;
	pItem = FindItemInBag(i64Id);
	if(!pItem) pItem = FindItemInStorage(i64Id);
	if(!pItem){
		for (int i=0;i<EQUIP_MAX_COUNT;i++)
		{
			if (m_stEquip[i] && m_stEquip[i]->GetItemID() == i64Id)
			{
				pItem = m_stEquip[i];
			}
		}
	}

	return pItem;
}

bool CPlayerPackage::saveBagPacket(char* dest,DWORD& retlen){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	retlen=0;
	if ( maxsize<sizeof(int)){ return false;	}//检测当前存档空间
	/*	新版本
	数量 int
	数据 bin
	*/
	int len=0;
	int count=0;//物品数量
	len += sizeof(count);

	for (int i= 0;i< m_nOpenBagCount;i++)
	{
		if (m_BagPacket[i].m_boOpened)
		{
			for (auto it =  m_BagPacket[i].m_PackItemList.begin();it != m_BagPacket[i].m_PackItemList.end();it ++){
				CItem* pItem= it->second;
				if (pItem && !pItem->IfLimitedTime() )
				{
					if ((maxsize  - len) >=sizeof(stItem))
					{
						memcpy((void *)(dest+len), &pItem->m_Item, sizeof(stItem));
						len += sizeof(stItem);
						maxsize -=sizeof(stItem);
						count++;
					}else return false;
				}
			}
		}
	}

	*((int*)(dest))=count;	//当前包裹物品数量
	retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen);
	base64_encode(dest,len,pin,retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CPlayerPackage::saveStoragePacket(char* dest,DWORD& retlen){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	retlen=0;
	if ( maxsize< sizeof(int) ){ return false;	}//检测当前存档空间
	/*	新版本
	数量 int
	数据 bin
	*/
	int len=0;
	int count=0;//物品数量
	len += sizeof(count);

	for (int i= 0;i< m_nOpenStorageCount;i++)
	{
		if (m_StoragePacket[i].m_boOpened)
		{
			for (auto it =  m_StoragePacket[i].m_PackItemList.begin();it != m_StoragePacket[i].m_PackItemList.end();it ++){
				CItem* pItem= it->second;
				if (pItem && !pItem->IfLimitedTime() )
				{
					if ((maxsize  - len) >=sizeof(stItem))
					{
						memcpy((void *)(dest+len), &pItem->m_Item, sizeof(stItem));
						len += sizeof(stItem);
						maxsize -=sizeof(stItem);
						count++;
					}else return false;
				}
			}
		}
	}

	*((int*)(dest))=count;	//当前包裹物品数量
	retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen);
	base64_encode(dest,len,pin,retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CPlayerPackage::saveEquip(char* dest,DWORD& retlen){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	retlen=0;
	if ( maxsize<sizeof(int)){ return false;	}//检测当前存档空间
	/*	新版本
	数量 int
	数据 bin
	*/
	int len=0;
	int count=0;//物品数量
	len += sizeof(count);

	for (int i= 0;i< EQUIP_MAX_COUNT;i++)
	{
		if (m_stEquip[i] && !m_stEquip[i]->IfLimitedTime()	)
		{
			if ((maxsize  - len) >=sizeof(stItem))
			{
				memcpy((void *)(dest+len), &m_stEquip[i]->m_Item, sizeof(stItem));
				len += sizeof(stItem);
				maxsize -=sizeof(stItem);
				count++;
			}else return false;
		}
	}

	*((int*)(dest))=count;	//当前包裹物品数量
	retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen);
	base64_encode(dest,len,pin,retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CPlayerPackage::saveTmpBagPacket(char *dest, DWORD& retlen)
{
	FUNCTION_BEGIN;
	int maxsize = retlen;
	retlen = 0;
	if (maxsize < sizeof(int)) { return false; }//检测当前存档空间
											  /*	新版本
											  数量 int
											  数据 bin
											  */
	int len = 0;
	int count = 0;//物品数量
	len += sizeof(count);

	for (int i = 0; i < m_nOpenTmpBagCount; i++)
	{
		if (m_TmpBagPacket[i].m_boOpened)
		{
			for (auto it = m_TmpBagPacket[i].m_PackItemList.begin(); it != m_TmpBagPacket[i].m_PackItemList.end(); it++) {
				CItem* pItem = it->second;
				if (pItem && !pItem->IfLimitedTime())
				{
					if ((maxsize - len) >= sizeof(stItem))
					{
						memcpy((void *)(dest + len), &pItem->m_Item, sizeof(stItem));
						len += sizeof(stItem);
						maxsize -= sizeof(stItem);
						count++;
					}
					else return false;
				}
			}
		}
	}

	*((int*)(dest)) = count;	//当前包裹物品数量
	retlen = ROUNDNUMALL(len, 3) / 3 * 4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin = ptlsbuf;
	ZeroMemory(pin, retlen);
	base64_encode(dest, len, pin, retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CPlayerPackage::load(const char* dest,int retlen,int nver){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	if (maxsize==0) return true;

	retlen=ROUNDNUMALL(retlen,4)/4*3;
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen);
	base64_decode((char*)dest,maxsize,pin,retlen);
	memcpy((char*)dest,pin,retlen);
	maxsize=retlen;

	if (maxsize<sizeof(int)){ return false;}
	int count= *((int*)(dest));
	int len=sizeof(int);//物品数据开始的偏移

	int nstructlen = sizeof(stItem);
	if (maxsize < int(count*nstructlen+sizeof(int))  ){return false;}

	for (int i=0;i< count ;i++)
	{
		int ncurrver = nver;
		stItem* pItemData= NULL;
		char szupdatebuffer[2][sizeof(stItem)+1024*4];
		ZeroMemory(&szupdatebuffer[0],2*(sizeof(stItem)+1024*4));
		int nowbuffer=0;
		char* poldbuffer=(char*)&dest[len];

		stItem item;
		memcpy(&item,(stItem*)poldbuffer,sizeof(stItem));
		if (item.i64ItemID)
		{
			CItem* pItem =NULL;
			pItem = CItem::LoadItem(&item,__FUNC_LINE__);
			if (pItem)
			{
				if (!FindItemInBody(item.i64ItemID))
				{
					if (!FindItemInStorage(item.i64ItemID))
					{
						if (!FindItemInBag(item.i64ItemID))
						{
							if (!FindItemInTmpBag(item.i64ItemID))
							{
								switch (item.Location.btLocation)
								{
								case ITEMCELLTYPE_EQUIP:
									{
										m_stEquip[item.Location.btIndex] = pItem;
									}break;
									case ITEMCELLTYPE_STORE:			// 仓库
									{
										if (item.Location.btTableID >= 0 && item.Location.btTableID < m_nOpenStorageCount &&  item.Location.btTableID < m_nOpenStorageCount)
										{
											if (m_StoragePacket[item.Location.btTableID].AddItemToPack(pItem, item.Location.btIndex, false, false))
											{

											}
											else CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
										}
										else CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);										
									}break;
									case ITEMCELLTYPE_PACKAGE:		// 包裹的格子
									{										
										if ((item.Location.btTableID >= 0 && item.Location.btTableID < m_nOpenBagCount && item.Location.btTableID < m_nOpenBagCount) || (item.Location.btTableID >= 4 && item.Location.btTableID < _MAX_BAG_COUNT))
										{
											if (m_BagPacket[item.Location.btTableID].AddItemToPack(pItem, item.Location.btIndex, false, false))
											{
											}
											else CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
										}
										else CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);											
									}break;
									case ITEMCELLTYPE_TMPPACKAGE:		// 临时背包的格子
									{
										if ((item.Location.btTableID >= 0 && item.Location.btTableID < m_nOpenTmpBagCount))
										{
											if (m_TmpBagPacket[item.Location.btTableID].AddItemToPack(pItem, item.Location.btIndex, false, false))
											{
											}
											else CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
										}
										else CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
									}break;
								}
							}else CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
						}else CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);
					}else CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);
				}else CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);
			}
		}
		len+= nstructlen;
	}
	return true;
}

bool CPlayerPackage::RandomDropItem(CItem* pItem, DWORD dwCount, int nInRange, int nOutRange, const char* szLog)
{
	CGameMap* curMap = m_PlayerObj->GetEnvir();

	if (!curMap) {
		return false;
	}
	if (pItem && !pItem->GetOutLock() && (!pItem->GetBinding() || pItem->GetType() == ITEM_TYPE_EQUIP)) {
		bool boRemove = false;
		if (!RemoveItemFromBody(pItem)) {
			if (!RemoveItemFromBag(pItem, dwCount, true, true)) {

			}
			else boRemove = true;
		}
		else boRemove = true;

		if (boRemove) {
			std::vector<Point> dropPoints;
			curMap->GetSpiralDropPosition(m_PlayerObj->GetX(), m_PlayerObj->GetY(), 1, dropPoints);
			for (auto point:dropPoints)
			{
				CMapItemEvent* pItemEvent = CLD_DEBUG_NEW CMapItemEvent(pItem, point.x, point.y);
				pItemEvent->Init(m_PlayerObj->GetEnvir(), m_PlayerObj->m_i64UserOnlyID, m_PlayerObj->m_i64UserOnlyID, OWNER_PLAYER);
				pItemEvent->boIsPlayerDrop = true;
				if (!m_PlayerObj->GetEnvir()->AddItemToMap(point.x, point.y, pItemEvent)) {
					g_logger.error("地面物品添加失败 %s (%d,%d)", pItem->GetItemName(), point.x, point.y);
					pItemEvent->OwnerItem = NULL;
					SAFE_DELETE(pItemEvent);
					AddItemToBag(pItem);
				}
				return true;
			}
		}
	}
	return false;
}

sol::table CPlayerPackage::GetItemTableByLocation(emItemCellType CellType, sol::this_state ts) {
	sol::state_view lua(ts);
	auto table = lua.create_table();
	int idx=0;
	switch (CellType)
	{
	case ITEMCELLTYPE_EQUIP://装备
		{
			for (int i=0;i<EQUIP_MAX_COUNT;i++)
			{
				if (m_stEquip[i])
				{
					table[idx+1]=m_stEquip[i];
					idx++;
				}
			}
			if (idx){return table;}
		}break;
	case ITEMCELLTYPE_STORE://仓库
		{
			for (int i=0;i<m_nOpenStorageCount;i++)
			{
				if (m_StoragePacket[i].m_boOpened){
					for (auto it =  m_StoragePacket[i].m_PackItemList.begin();it != m_StoragePacket[i].m_PackItemList.end();it ++){
						CItem* item= it->second;
						if (item)
						{
							table[idx+1]=item;
							idx++;
						}
					}
				}
			}
			if (idx){return table;}
		}break;		
	case ITEMCELLTYPE_PACKAGE://包裹的格子
		{
			for (int i=0;i<m_nOpenBagCount;i++)
			{
				if (m_BagPacket[i].m_boOpened){
					for (auto it =  m_BagPacket[i].m_PackItemList.begin();it != m_BagPacket[i].m_PackItemList.end();it ++){
						CItem* item= it->second;
						if (item)
						{
							table[idx+1]=item;
							idx++;
						}
					}
				}
			}
			if (idx){return table;}
		}break;
	case ITEMCELLTYPE_TMPPACKAGE://临时包裹  ITEMCELLTYPE_LINGSHBAOGUO
		{
			for (int i = 0;i < m_nOpenTmpBagCount;i++)
			{
				if (m_TmpBagPacket[i].m_boOpened) {
					for (auto it = m_TmpBagPacket[i].m_PackItemList.begin();it != m_TmpBagPacket[i].m_PackItemList.end();it++) {
						CItem* item = it->second;
						if (item)
						{
							table[idx + 1] = item;
							idx++;
						}
					}
				}
			}
			if (idx) { return table; }
		}break;
	}
	return table;
}

bool CPlayerPackage::GetItemTabByLocWithType(sol::table& table, emItemCellType CellType, sol::table& typetab) {
	FUNCTION_BEGIN;
	if (!table.valid()) { return false; }
	if (!typetab.valid()) { return false; }
	if (table && typetab) {
		int idx = 0;
		switch (CellType)
		{
		case ITEMCELLTYPE_EQUIP://装备
		{
			if (typetab[ITEM_TYPE_EQUIP])
			{
				for (int i = 0; i < EQUIP_MAX_COUNT; i++)
				{
					if (m_stEquip[i])
					{
						table[idx + 1] = m_stEquip[i];
						idx++;
					}
				}
			}
			if (idx) { return true; }
		}break;
		case ITEMCELLTYPE_STORE://仓库
		{
			for (int i = 0; i < m_nOpenStorageCount; i++)
			{
				if (m_StoragePacket[i].m_boOpened) {
					for (auto it = m_StoragePacket[i].m_PackItemList.begin(); it != m_StoragePacket[i].m_PackItemList.end(); it++) {
						CItem* item = it->second;
						if (item && typetab[item->GetType()])
						{
							table[idx + 1] = item;
							idx++;
						}
					}
				}
			}
			if (idx) { return true; }
		}break;
		case ITEMCELLTYPE_PACKAGE://包裹的格子
		{
			for (int i = 0; i < m_nOpenBagCount; i++)
			{
				if (m_BagPacket[i].m_boOpened) {
					for (auto it = m_BagPacket[i].m_PackItemList.begin(); it != m_BagPacket[i].m_PackItemList.end(); it++) {
						CItem* item = it->second;
						if (item && typetab[item->GetType()])
						{
							table[idx + 1] = item;
							idx++;
						}
					}
				}
			}
			if (idx) { return true; }
		}break;
		case ITEMCELLTYPE_TMPPACKAGE://临时包裹  ITEMCELLTYPE_LINGSHBAOGUO
		{
			for (int i = 0; i < m_nOpenTmpBagCount; i++)
			{
				if (m_TmpBagPacket[i].m_boOpened) {
					for (auto it = m_TmpBagPacket[i].m_PackItemList.begin(); it != m_TmpBagPacket[i].m_PackItemList.end(); it++) {
						CItem* item = it->second;
						if (item)
						{
							table[idx + 1] = item;
							idx++;
						}
					}
				}
			}
			if (idx) { return true; }
		}break;
		}
	}
	return false;
}

int CPlayerPackage::getBagCellCount()
{
	return m_nOpenBagCellCount;
}

void CPlayerPackage::setBagCellCount( int nBagCellcount )
{
	if (nBagCellcount > m_nOpenBagCellCount)
	{
		m_nOpenBagCellCount = nBagCellcount;
		m_BagPacket[0].ChangeBagCellCount(m_nOpenBagCellCount);
	}
}

void CPackage::ChangeBagCellCount( int nNewBagCount )
{
	if (nNewBagCount > (int)m_dwMaxCount) 
	{
		m_dwMaxCount = nNewBagCount;
		m_vUsed.resize(m_dwMaxCount);
	}
}

int CPlayerPackage::getStorageCellCount()
{
	return m_nOpenStorageCellCount;
}

void CPlayerPackage::setStorageCellCount(int nCellcount)
{
	if (nCellcount > m_nOpenStorageCellCount)
	{
		m_nOpenStorageCellCount = nCellcount;
		m_StoragePacket[0].ChangeStorageCellCount(m_nOpenBagCellCount);
	}
}

void CPackage::ChangeStorageCellCount(int nNewBagCount)
{
	if (nNewBagCount > (int)m_dwMaxCount)
	{
		m_dwMaxCount = nNewBagCount;
		m_vUsed.resize(m_dwMaxCount);
	}
}

CItem* CPlayerPackage::FindItemInBagByPos(int nPos)
{
	for (int i = 0; i < m_nOpenBagCount; i++) {
		if (m_BagPacket[i].m_boOpened) {
			CItem* item = m_BagPacket[i].FindItemByPos(nPos);
			if (item) {
				return item;
			}
		}
	}
	return nullptr;
}

CItem* CPlayerPackage::FindItemInBodyByPos(int nPos)
{
	FUNCTION_BEGIN;
	if (nPos >= 0 && nPos < EQUIP_MAX_COUNT)
	{
		if (m_stEquip[nPos])
			return m_stEquip[nPos];
	}
	return NULL;
}

DWORD CPlayerPackage::GetItemCountInBodyAllByBaseId(DWORD baseid)
{
	FUNCTION_BEGIN;
	int itemnum=0;
	for(int i = 0; i < EQUIP_MAX_COUNT; i++){
		if(m_stEquip[i] && m_stEquip[i]->m_Item.dwBaseID == baseid){
			itemnum++;
		}
	}
	return itemnum;
}

bool CPlayerPackage::DeleteItemInBagAndBodyAllByBaseId(DWORD dwBaseId,DWORD num,const char* pszLogStr){
	FUNCTION_BEGIN;
	if (num==0) return false;
	DWORD bodycount = GetItemCountInBodyAllByBaseId(dwBaseId);
	DWORD bagcount = GetItemCountInBagAllByBaseId(dwBaseId);

	if(bagcount + bodycount >= num){
		if(bagcount >= num){
			DeleteItemInBagAllByBaseId(dwBaseId, num, pszLogStr);
		}else{
			if(bagcount > 0){
				DeleteItemInBagAllByBaseId(dwBaseId, bagcount, pszLogStr);
				DeleteItemInBodyAllByBaseId(dwBaseId, num-bagcount, pszLogStr);
			}else{
				DeleteItemInBodyAllByBaseId(dwBaseId, num, pszLogStr);
			}
		}

		return true;
	}
	return false;
}

sol::table CPlayerPackage::FindAllItemInBagByBaseId(DWORD dwBaseId, sol::this_state ts)
{
	sol::state_view lua(ts);
	auto table = lua.create_table();
	int idx=1;
	for (int i=0;i<m_nOpenBagCount;i++)
	{
		const auto* items = m_BagPacket[i].FindItemsByBaseID(dwBaseId);
		if (!items) {
			continue;
		}
		for (CItem* pItem : *items) {
			if (pItem && !pItem->GetOutLock()) {
				table[idx++] = pItem;
			}
		}
	}
	return table;
}

bool CPlayerPackage::DeleteItemInBodyAllByBaseId(DWORD dwBaseId,DWORD num,const char* pszLogStr){
	FUNCTION_BEGIN;
	if(num > 0 && GetItemCountInBodyAllByBaseId(dwBaseId) >= num){
		for (int i=0;i<EQUIP_MAX_COUNT;i++){
			if (m_stEquip[i] && m_stEquip[i]->m_Item.dwBaseID == dwBaseId){
				if(m_stEquip[i]->m_Item.dwExpireTime!=0 ){
					m_PlayerObj->set_LimitItemPacket.erase(m_stEquip[i]);
				}
				auto pitemdata = m_stEquip[i]->GetItemDataBase();
				if (pitemdata && pitemdata->vitemskills.size() && isNeedDeleteSkill(i, pitemdata->nID)) {
					for (DWORD i = 0; i < pitemdata->vitemskills.size(); i++) {
						stItemSkill itemskill = pitemdata->vitemskills[i];
						m_PlayerObj->DeleteSkill(itemskill.dwSkillId);
					}
				}
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("EuqipRemove", m_PlayerObj, m_stEquip[i]->GetItemBaseID(),true);
				SendEquipDeleteItem(m_stEquip[i]->m_Item.i64ItemID);
				if(pszLogStr) m_stEquip[i]->SetItemLog(pszLogStr,m_PlayerObj,NULL,false,m_stEquip[i]->GetItemCount());
				CUserEngine::getMe().ReleasePItem(m_stEquip[i],__FUNC_LINE__);
				m_stEquip[i]=NULL;

				return true;
			}
		}
	}
	return false;
}


bool CPlayerPackage::DeleteItemInActBag(CItem* pItem, DWORD dwCount)
{
	for (auto it = m_ActBag.begin(); it != m_ActBag.end();) {
		CItem* item = (*it);

		if (item->GetItemID() == pItem->GetItemID() && item->GetItemCount() >= dwCount)
		{
			item->SetItemCount(item->GetItemCount() - dwCount);
			if (item->GetItemCount() <= 0)
			{
				if (m_PlayerObj)
					SendActBagDeleteItem(item->GetItemID(), item->GetItemBaseID());
				CUserEngine::getMe().ReleasePItem(item, __FUNC_LINE__);
				it = m_ActBag.erase(it);
			}
			else {
				++it;
				if (m_PlayerObj)
					SendActBagUpdateItem(item);
			}
			return true;
		}
		else {
			++it;
		}
	}
	return false;
}

bool CPlayerPackage::AddItemToActBag(CItem* pItem)
{
	if (!pItem) return false;
	if (m_ActBag.size() > 0){
		for (auto it = m_ActBag.begin(); it != m_ActBag.end(); it++) {
			CItem* item = (*it);
			if (pItem->GetItemBaseID() == item->GetItemBaseID()) {
				if (item->GetMaxCount() >= (pItem->GetItemCount() + item->GetItemCount())) {
					item->SetItemCount(pItem->GetItemCount() + item->GetItemCount());
					CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
					if (m_PlayerObj)
					{
						SendActBagUpdateItem(item);
					}					
					return true;
				}
			}
		}
	}
	pItem->m_Item.Location.btLocation = ITEMCELLTYPE_ACTBAG;
	pItem->m_Item.Location.btIndex = m_ActBag.size() + 1;
	m_ActBag.push_back(pItem);
	if (m_PlayerObj)
		SendActBagUpdateItem(pItem);
	return true;
}

bool CPlayerPackage::SendActBag()
{
	FUNCTION_BEGIN;
	stCretItems ActBagItems;
	ActBagItems.btPosition = ITEMCELLTYPE_ACTBAG;
	for (auto it = m_ActBag.begin(); it != m_ActBag.end(); it++) {
		CItem* item = (*it);
		ActBagItems.items.push_back(item->m_Item, __FUNC_LINE__);
	}
	if (m_PlayerObj)
		m_PlayerObj->SendMsgToMe(&ActBagItems, sizeof(ActBagItems) + ActBagItems.items.getarraysize());
	return true;
}

void CPlayerPackage::SendActBagDeleteItem(int64_t i64ID, DWORD dwBaseID) {
	stCretDeleteItem equipitem;
	equipitem.btPosition = ITEMCELLTYPE_ACTBAG;
	equipitem.i64Id = i64ID;
	if (m_PlayerObj)
		m_PlayerObj->SendMsgToMe(&equipitem, sizeof(equipitem));
}

void CPlayerPackage::ClearActBag()
{
	for (auto it = m_ActBag.begin(); it != m_ActBag.end(); it++) {
		CItem* item = (*it);
		if (m_PlayerObj)
			SendActBagDeleteItem(item->GetItemID(), item->GetItemBaseID());
		CUserEngine::getMe().ReleasePItem(item, __FUNC_LINE__);
	}
	m_ActBag.clear();
}

void CPlayerPackage::SendActBagUpdateItem(CItem* pItem)
{
	FUNCTION_BEGIN;
	stCretUpdateItem ActBagItem;
	ActBagItem.btPosition = ITEMCELLTYPE_ACTBAG;
	if (pItem){
		ActBagItem.item = pItem->m_Item;
		if (m_PlayerObj)
			m_PlayerObj->SendMsgToMe(&ActBagItem, sizeof(ActBagItem));
	}
}


CItem* CPlayerPackage::FindItemInActBag(int64_t i64Id)
{
	for (auto it = m_ActBag.begin(); it != m_ActBag.end(); it++) {
		CItem* item = (*it);
		if (item->GetItemID() == i64Id)
		{
			return item;
		}
	}
	return nullptr;
}
