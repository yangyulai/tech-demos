#include "Trade.h"
#include "Chat.h"
#include "NpcTrade.h"
#include "cmd/Trade_cmd.h"
#include "Npc.h"
 
CTempItemList::CTempItemList()
{ 
	FUNCTION_BEGIN;
	init();
}

CTempItemList::~CTempItemList(){
	for (TempItemList::iterator it= m_TempItemListMap.begin();it!= m_TempItemListMap.end();it++)
	{
		stTempOutItem *pTempList = it->second;
		SAFE_DELETE(pTempList);
	}
	m_TempItemListMap.clear();
}

void CTempItemList::init()
{
	FUNCTION_BEGIN;
	RemoveAllTempListItem();
	m_pTarget = NULL;
	m_dwGold = 0;
	m_btGoldType = 0;
}

bool CTempItemList::AddTempListItem(CItem *pItem,int nNum)
{
	FUNCTION_BEGIN;
	if(nNum == 0 || !pItem || pItem->GetOutLock()) return false;
	TempItemList::iterator it = m_TempItemListMap.find(pItem->GetItemID());
	if(it !=m_TempItemListMap.end()){
		return false;
	}else{
		if(pItem->GetItemCount())
		{
			stTempOutItem *pTempList = CLD_DEBUG_NEW stTempOutItem;
			pTempList->pItem = pItem;
			pTempList->nNum = nNum;
			m_TempItemListMap[pItem->GetItemID()] = pTempList;
			pItem->SetOutLock(true);
			return true;
		}
	}

	return false;
}

int CTempItemList::AddTempListItem_2(CItem* pItem, int nNum)
{
	FUNCTION_BEGIN;
	if (nNum == 0 || !pItem || pItem->GetOutLock()) return 0;

	TempItemList::iterator it = m_TempItemListMap.find(pItem->GetItemID());
	if (it != m_TempItemListMap.end()) {
		if (it->second->nNum + nNum <= pItem->GetItemCount()) {
			it->second->nNum += nNum;
			return  it->second->nNum;
		}	
	}else {
		if (pItem->GetItemCount())
		{
			stTempOutItem* pTempList = CLD_DEBUG_NEW stTempOutItem;
			pTempList->pItem = pItem;
			pTempList->nNum = nNum;
			m_TempItemListMap[pItem->GetItemID()] = pTempList;
			pItem->SetOutLock(true);
			return nNum;
		}
	}
	return 0;
}

stTempOutItem* CTempItemList::FindTempListItem(__int64 i64ItemID)
{
	FUNCTION_BEGIN;
	TempItemList::iterator it = m_TempItemListMap.find(i64ItemID);
	if(it !=m_TempItemListMap.end()){
		stTempOutItem *pTempList = it->second;
		return pTempList;
	}
	return NULL;
}
bool CTempItemList::RemoveTempListItem(__int64 i64ItemID,int nNum)
{
	FUNCTION_BEGIN;
	TempItemList::iterator it = m_TempItemListMap.find(i64ItemID);
	if(it !=m_TempItemListMap.end()){
		stTempOutItem *pTempList = it->second;
		if(pTempList->pItem) pTempList->pItem->SetOutLock(false);
		if (pTempList->nNum >= nNum) {
			m_TempItemListMap.erase(it);
			SAFE_DELETE(pTempList);
		}else {
			pTempList->nNum -= nNum;
		}
		return true;
	}
	return false;
}

void CTempItemList::RemoveAllTempListItem(bool boSend)
{
	FUNCTION_BEGIN;
	if (!m_TempItemListMap.empty()) {
		for (TempItemList::iterator it = m_TempItemListMap.begin(); it != m_TempItemListMap.end(); it++)
		{
			stTempOutItem* pTempList = it->second;
			if (pTempList->pItem) {
				pTempList->pItem->SetOutLock(false);
			}
			SAFE_DELETE(pTempList);
		}
	}
	m_TempItemListMap.clear();
}

void CTempItemList::AddItemToOhterBag(std::string & szItemNames,emTradeType emTrade)
{
	FUNCTION_BEGIN;
	szItemNames = "";
	BUFFER_CMD(stFinishTrade,retcmd,stBasePacket::MAX_PACKET_SIZE);
	retcmd->dwGold = m_dwGold;
	bool boLog = false;
	for (TempItemList::iterator it=m_TempItemListMap.begin(); it!=m_TempItemListMap.end(); it++) 
	{
		stTempOutItem *pTempOutItem = it->second;
		stToClientItemAndCount TempToClientItemAndCount;
		s_strncpy_s(TempToClientItemAndCount.szItemName,pTempOutItem->pItem->GetItemName(),sizeof(TempToClientItemAndCount.szItemName)-1);
		TempToClientItemAndCount.nCount =pTempOutItem->nNum;
		retcmd->itemid2count.push_back(TempToClientItemAndCount,__FUNC_LINE__);
		pTempOutItem->pItem->SetOutLock(false);	

		if(pTempOutItem->pItem->GetItemCount() !=(DWORD)pTempOutItem->nNum){
			g_logger.error("交易添加到包里面的物品数量和交易数量不符合");
			//int nttt= pTempOutItem->pItem->GetItemCount();
		}
		pTempOutItem->pItem->SetItemLog("exchangeitemok",m_pMe, m_pTarget,false,0,m_pMe->m_Trade.m_dwGold);
		boLog = true;
		char sztmp[_MAX_PATH];
		sprintf_s(sztmp,sizeof(sztmp),"%s:%d,",pTempOutItem->pItem->GetItemName(),pTempOutItem->nNum);
		szItemNames += sztmp;
		__int64 i64ItemId=pTempOutItem->pItem->GetItemID();
		BYTE btType=(BYTE)pTempOutItem->pItem->GetType();
		BYTE btStation=pTempOutItem->pItem->GetEquipStation();
		m_pTarget->m_Packet.AddItemToBag(pTempOutItem->pItem,true);			//移除的时候已经修改了pitem里面的数量
		pTempOutItem->pItem = NULL;

	}

	if(!boLog && m_dwGold>0){
		GameService::getMe().Send2LogSvr(_SERVERLOG_ITEM_,1,0,m_pMe,
			//"'%s','%s',%I64d,'%s',%d,'%s',%d,%d,%d,'%s',%d,%I64u,'%s','%s',%d,%d,%d,%d,%d,'%s','%s'",
			"'%s','%s',%I64d,'%s',%d,'%s',%d,%d,%d,'%s',%d,%I64u,'%s','%s',%d,%d,%d,%d,%d,'%s','%s',%d",
			"exchangeitemok",
			m_pMe->getAccount(),
			m_pMe->m_i64UserOnlyID,
			m_pMe->getName(),
			m_pMe->m_dwLevel,
			m_pTarget->getName(),
			m_pTarget->m_dwLevel,
			0,
			0,
			"gold",
			0,
			0xFFFFFFFFFF,
			"",
			m_pMe->GetEnvir()->getFullMapName(),
			m_pMe->GetX(),
			m_pMe->GetY(),
			0,
			m_dwGold,
			0,
			"",
			m_pTarget->getAccount(),
			0
			);
	}
	GameService::getMe().Send2LogSvr(_SERVERLOG_TRADEITEM,1,0,m_pMe,
		//"'%s','%s',%I64d,'%s',%d,'%s',%d,%d,%d,'%s',%d,%I64u,'%s','%s',%d,%d,%d,%d,%d,'%s','%s'",
		"'%s','%s',%I64d,'%s','%s',%I64d,'%s','%s','%d',%s",
		"exchangeitemok",
		m_pMe->getAccount(),
		m_pMe->m_i64UserOnlyID,
		m_pMe->getName(),
		m_pTarget->getAccount(),
		m_pTarget->m_i64UserOnlyID,
		m_pTarget->getName(),
		m_pMe->GetEnvir()->getFullMapName(),
		m_pMe->m_Trade.m_dwGold,
		szItemNames
		);

	m_pTarget->SendMsgToMe(retcmd,sizeof(stFinishTrade)+retcmd->itemid2count.getarraysize());
}

void CTempItemList::RemoveItemFromMyBag()
{
	FUNCTION_BEGIN;
	for (TempItemList::iterator it=m_TempItemListMap.begin(); it!=m_TempItemListMap.end(); it++) 
	{
		stTempOutItem *pTempOutItem = it->second;
		pTempOutItem->pItem->SetOutLock(false);
		pTempOutItem->pItem->SetItemLog("exchangeremoveitem",m_pMe);
		m_pMe->m_Packet.RemoveItemFromBag(pTempOutItem->pItem,pTempOutItem->nNum,false,true);			//移除的时候已经修改了pitem里面的数量
	}
}

void CTempItemList::sendItemByMail(CPlayerObj* pPlayer)
{
	FUNCTION_BEGIN;
	BUFFER_CMD(stMailSendNewMailInner, newMail, stBasePacket::MAX_PACKET_SIZE);
	newMail->MailDetail.dwGold = 0;
	for (TempItemList::iterator it = m_TempItemListMap.begin(); it != m_TempItemListMap.end(); it++)
	{
		stTempOutItem* pTempOutItem = it->second;
		if (pTempOutItem && pTempOutItem->pItem) {
			newMail->MailDetail.ItemArr.push_back(pTempOutItem->pItem->m_Item, __FUNC_LINE__);
			m_pMe->m_Trade.SendTradeLog(pTempOutItem->nNum, pTempOutItem->pItem->GetItemName());
		}
	}
	if (m_dwGold && m_btGoldType) {
		stItem* pItem = pPlayer->GetMailedItem(m_btGoldType, m_dwGold, 0);
		if (pItem) {
			newMail->MailDetail.ItemArr.push_back(*pItem, __FUNC_LINE__);
			const char* szName = "";
			switch (m_btGoldType) {
			case tosizet(ResID::charge):szName = "能源晶石"; break;
			case tosizet(ResID::game):szName = "决战币";
			}
			m_pMe->m_Trade.SendTradeLog(m_dwGold, szName);
		}
	}
	if (newMail->MailDetail.ItemArr.getarraysize() == 0) return;
	strcpy_s(newMail->MailDetail.szReceiverName, _MAX_NAME_LEN_, pPlayer->getName());
	sprintf_s(newMail->MailDetail.szTitle, _MAX_MAIL_TITLE_LEN, "面对面交易");
	sprintf_s(newMail->MailDetail.szNotice, _MAX_MAILNOTICE_LEN, "以下是你所获得的交易道具");
	BUFFER_CMD(stSendMailMsgSuperSrv, pSend, stBasePacket::MAX_PACKET_SIZE);
	pSend->i64SrcOnlyId = pPlayer->m_i64UserOnlyID;
	pSend->msg.push_back((char*)newMail, sizeof(*newMail) + newMail->MailDetail.ItemArr.getarraysize(), __FUNC_LINE__);
	if (CUserEngine::getMe().isCrossSvr()) {
		CPlayerObj* player = CUserEngine::getMe().m_playerhash.FindByOnlyId(pPlayer->m_i64UserOnlyID);
		if (player == NULL)
			player = CUserEngine::getMe().m_playerhash.FindByName(pPlayer->getName());
		if (player)
			CUserEngine::getMe().SendMailMsg2Super(newMail, sizeof(*newMail) + newMail->MailDetail.ItemArr.getarraysize(), player->m_i64UserOnlyID);
	}
	else {
		CUserEngine::getMe().SendMsg2SuperSvr(pSend, sizeof(*pSend) + pSend->msg.getarraysize());
	}

}

void CTempItemList::SendTradeLog(int num, const char* szName)
{
	FUNCTION_BEGIN;
	if (m_pMe && m_pTarget) {
		GameService::getMe().Send2LogSvr(_SERVERLOG_TRADEITEM, 1, 0, m_pMe,
			"'%s','%s',%I64d,'%s','%s',%I64d,'%s','%s','%d',%s",
			"FaceTrade",
			m_pMe->getAccount(),
			m_pMe->m_i64UserOnlyID,
			m_pMe->getName(),
			m_pTarget->getAccount(),
			m_pTarget->m_i64UserOnlyID,
			m_pTarget->getName(),
			m_pMe->GetEnvir()->getFullMapName(),
			num,
			szName
		);
	}
}

int CTempItemList::GetCountFromList(BYTE btType,bool bAbType)
{
	FUNCTION_BEGIN;
	int ret = 0;
	for (TempItemList::iterator it= m_TempItemListMap.begin();it!= m_TempItemListMap.end();it++){
		stTempOutItem* pTempOutItem = it->second;
		auto itemBase = pTempOutItem->pItem->GetItemDataBase();
		if (!itemBase) return false;
		if (itemBase->GetBagType(bAbType) & btType)
		{
			ret++;
		}
	}

	return ret;
}

bool CTempItemList::OtherCanLoadItem(CItem *pItem,bool bTrade)
{
	FUNCTION_BEGIN;
	TempItemList til = m_pTarget->m_Trade.m_TempItemListMap;
	if(!pItem){

		int nNormalLeft = 0;
		int nFightLeft = 0;
		int nMessionLeft = 0;
		for (TempItemList::iterator it= til.begin();it!= til.end();it++){
			stTempOutItem* pTempOutItem = it->second;
			if(pTempOutItem->pItem->GetItemCount() == (DWORD)pTempOutItem->nNum){
				switch(pTempOutItem->pItem->m_Item.Location.btTableID){
				case 5:
					nFightLeft++;
					break;
				case 4:
					nMessionLeft++;
					break;
				default:
					nNormalLeft++;
				}
			}
		}

		nNormalLeft = m_pTarget->m_Packet.GetFreeBagCellCount(1) +nNormalLeft;	//放完常规物品剩下的包裹数量

		nNormalLeft -= GetCountFromList(1,true) ;

		bool bCanLoad = true;

		if(nNormalLeft < 0){
			bCanLoad = false;
		}
		else{
			if(nFightLeft <0){
				if(nNormalLeft+nFightLeft<0){
					bCanLoad = false;
				}
				else{
					nNormalLeft +=nFightLeft;
					if(nNormalLeft+nMessionLeft <0){
						bCanLoad = false;
					}
					else{

					}
				}
			}
		}

		if(!bCanLoad){
			stCancelTrade retcmd;
			retcmd.btType = TRADE_CANCEL_PAKNOCELL;
			s_strncpy_s(retcmd.szName,m_pTarget->getName(),_MAX_NAME_LEN_ -1);
			m_pMe->SendMsgToMe(&retcmd,sizeof(retcmd));
			if(bTrade) m_pTarget->SendMsgToMe(&retcmd,sizeof(retcmd));
			return false;
		}
	}
	else{
		auto itemBase = pItem->GetItemDataBase();
		if (!itemBase) return false;
		int nPetBox = m_pTarget->m_Trade.GetCountFromList(itemBase->GetBagType());
		if(m_pTarget->m_Packet.GetFreeBagCellCount(itemBase->GetBagType()) - m_pTarget->m_Trade.GetCountFromList(itemBase->GetBagType())<=0) 
		{
			stCancelTrade retcmd;
			retcmd.btType = TRADE_CANCEL_PAKNOCELL;
			s_strncpy_s(retcmd.szName,m_pTarget->getName(),_MAX_NAME_LEN_ -1);
			m_pMe->SendMsgToMe(&retcmd,sizeof(retcmd));
			if(bTrade) m_pTarget->SendMsgToMe(&retcmd,sizeof(retcmd));
			return false;
		}
	}

	return true;
}

bool CTempItemList::CanAddGold(int nAddGold,int goldByte)
{
	FUNCTION_BEGIN;
	bool boAdd = false;
	if (goldByte == tosizet(ResID::charge) && nAddGold <= m_pMe->m_res[ResID::charge]) {
		boAdd = true;
	}
	if (goldByte == tosizet(ResID::game) && nAddGold <= m_pMe->m_res[ResID::game]) {
		boAdd = true;
	}
	// (nAddGold + m_pTarget->m_dwGold > _MAX_CARRYGOLD_(m_pTarget->m_dwLevel)
	if(!boAdd)
	{
		m_pMe->m_Trade.SendTipsMsg(TRADE_CANCEL_MONEYOVER);
		return false;
	}
	return true;
}

bool CTempItemList::MyGoldEnough(bool bTrade)
{
	FUNCTION_BEGIN;
	if(m_pMe->m_dwGold >= m_dwGold) return true;
	stCancelTrade retcmd;
	retcmd.btType = TRADE_CANCEL_MONEYOVER;
	s_strncpy_s(retcmd.szName,m_pMe->getName(),_MAX_NAME_LEN_ -1);
	m_pMe->SendMsgToMe(&retcmd,sizeof(retcmd));
	if(bTrade) m_pTarget->SendMsgToMe(&retcmd,sizeof(retcmd));
	return false;
}

void CTempItemList::SendToAll(void *pBuf,int nLen)
{
	FUNCTION_BEGIN;
	m_pMe->SendMsgToMe(pBuf,nLen);
	if(m_pTarget) m_pTarget->SendMsgToMe(pBuf,nLen);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CTrade::CTrade(CPlayerObj* owner)
{
	FUNCTION_BEGIN;
	InitTrade();
	m_pMe = owner;
}

void CTrade::InitTrade() //初始化交易
{
	__super::init();
	m_boTrade = false;
	m_boReadyTrade = false;
	m_boCommit = false;
	m_boLock= false;
	m_dwRequestTime = 0; 
	m_emTradeType = TRADE_NORMAL;
}

void CTrade::SendTradeList(CPlayerObj* pTarget)
{
	FUNCTION_BEGIN;
	if (pTarget)
	{
		if (!pTarget->m_waittradelist.empty()) {
			DWORD now = time(NULL);
			for (int i = 0; i < (int)pTarget->m_waittradelist.size(); i++) {
				if ((now - pTarget->m_waittradelist[i].time) >= _TIME_KEEPTRADELIST_) {
					CPlayerObj* Player = CUserEngine::getMe().m_playerhash.FindByOnlyId(pTarget->m_waittradelist[i].i64Id);
					if (Player) Player->m_Trade.InitTrade();
					pTarget->m_waittradelist.erase(pTarget->m_waittradelist.begin() + i);
				}
			}
		}
		BUFFER_CMD(stSendTradeListRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
		stTradeList retcmdlist;
		if (!pTarget->m_waittradelist.empty())
		{
			for (int i = 0; i < (int)pTarget->m_waittradelist.size(); i++) {
				retcmdlist.i64Id = pTarget->m_waittradelist[i].i64Id;
				strcpy_s(retcmdlist.szName, _MAX_NAME_LEN_ - 1, pTarget->m_waittradelist[i].szName);
				retcmdlist.time = pTarget->m_waittradelist[i].time;
				retcmd->tradelist.push_back(retcmdlist, __FUNC_LINE__);
			}
		}
		retcmd->btType = TRADE_FACETOFACE;
		pTarget->SendMsgToMe(retcmd, sizeof(stSendTradeListRet)+ retcmd->tradelist.getarraysize());
	}
}

void CTrade::UpadateTradeList(CPlayerObj* pApply, CPlayerObj* pTarget, BYTE btType)
{
	FUNCTION_BEGIN;
	if (pApply && pTarget && btType) {
		if (btType == TRADE_ADD) 
		{
			if (!pTarget->m_waittradelist.empty() && pTarget->m_waittradelist.size() >= 5) {
				pTarget->m_waittradelist.erase(pTarget->m_waittradelist.begin());
			}
			stTradeList listinfo;
			listinfo.i64Id = pApply->m_i64UserOnlyID;
			CopyString(listinfo.szName,pApply->GetName());
			listinfo.time = time(NULL);
			pTarget->m_waittradelist.push_back(std::move(listinfo));
		}
		else if(btType == TRADE_REMOVE) 
		{
			if (!pTarget->m_waittradelist.empty()) {
				for (int i = 0; i < (int)pTarget->m_waittradelist.size(); i++) {
					if (pTarget->m_waittradelist[i].i64Id == pApply->m_i64UserOnlyID) {
						pTarget->m_waittradelist.erase(pTarget->m_waittradelist.begin() + i);
					}
				}
			}
		}
		else if (btType == TRADE_UPDATE) 
		{
			if (!pTarget->m_waittradelist.empty()) {
				for (int i = 0; i < (int)pTarget->m_waittradelist.size(); i++) {
					if (pTarget->m_waittradelist[i].i64Id == pApply->m_i64UserOnlyID) {
						pTarget->m_waittradelist[i].time = time(NULL);
					}
				}
			}
		}
	}
}

void CTrade::SendTradeItem()
{
	FUNCTION_BEGIN;
	BUFFER_CMD(stTradeMsgRet, retcmd, stBasePacket::MAX_PACKET_SIZE);
	retcmd->TargetId = m_pMe->m_i64UserOnlyID;
	retcmd->TargetLv = m_pMe->m_dwLevel;
	retcmd->dwGold = m_dwGold;
	for (TempItemList::iterator it = m_TempItemListMap.begin(); it != m_TempItemListMap.end(); it++)
	{
		stTempOutItem* pTempOutItem = it->second;
		if (pTempOutItem && pTempOutItem->pItem)
			retcmd->stItemList.push_back(pTempOutItem->pItem->m_Item, __FUNC_LINE__);
	}
	SendToAll(retcmd, sizeof(stTradeMsgRet) + retcmd->stItemList.getarraysize());
}

void CTrade::SendBeginSta()
{
	FUNCTION_BEGIN;
	stBeginTrade retcmd;
	retcmd.TargetId = m_pMe->m_i64UserOnlyID;
	retcmd.TargetLv = m_pMe->m_dwLevel;
	if (m_pTarget) m_pTarget->SendMsgToMe(&retcmd, sizeof(retcmd));
}

void CTrade::SendBreakTips(BYTE btType)
{
	FUNCTION_BEGIN;
	if (btType) {
		stBreakTrade retcmd;
		retcmd.btType = btType;
		strcpy_s(retcmd.szName, _MAX_NAME_LEN_ - 1, m_pMe->getName());
		if (m_pMe) m_pMe->SendMsgToMe(&retcmd, sizeof(retcmd));
	}
}
void CTrade::SendRequestMsg(CPlayerObj* pTarget)
{
	FUNCTION_BEGIN;
	if (pTarget) {
		stRequestTrade retcmd;
		retcmd.dwPlayerId = m_pMe->m_i64UserOnlyID;
		retcmd.boMeReady = m_pMe->m_Trade.m_boReadyTrade;
		retcmd.boTargetReady = pTarget->m_Trade.m_boReadyTrade;
		m_pMe->SendMsgToMe(&retcmd, sizeof(retcmd));
	}
}
void CTrade::SendCommitMsg(CPlayerObj* pTarget)
{
	FUNCTION_BEGIN;
	if (pTarget) {
		stCommitTrade retcmd;
		retcmd.i64Id = m_pMe->m_i64UserOnlyID;
		retcmd.boMeReady = m_pMe->m_Trade.m_boCommit;
		retcmd.boTargetReady = pTarget->m_Trade.m_boCommit;
		m_pMe->SendMsgToMe(&retcmd, sizeof(retcmd));
	}
}

void CTrade::SendTipsMsg(BYTE btType)
{
	FUNCTION_BEGIN;
	if (m_pMe) {
		CChat::sendClient(m_pMe, GameService::getMe().GetStrRes(btType, "facetrade"));
	}
}

void CTrade::AllCancelTrade(const char *pszCancelName,BYTE btType)
{
	FUNCTION_BEGIN;
	if (m_pTarget) m_pTarget->m_Trade.CancelTrade(pszCancelName, btType);
	CancelTrade(pszCancelName,btType);
}


void CTrade::CancelTrade(const char *pszCancelName,BYTE btType)
{
	FUNCTION_BEGIN;
	RemoveAllTempListItem();
	stCancelTrade retcmd;
	retcmd.btType = btType;
	if(pszCancelName) s_strncpy_s(retcmd.szName,pszCancelName,_MAX_NAME_LEN_ -1);
	m_pMe->SendMsgToMe(&retcmd,sizeof(retcmd));
	InitTrade();
}

BYTE CTrade::CheckMyCanTrade()
{
	FUNCTION_BEGIN;
	if (m_pMe->GetEnvir()->isNoSafeZone() )
		return TRADE_CANCEL_NOSAFE;
	if (m_pMe->m_btDayTradeCnt >= 10)
		return TRADE_CANCEL_CNTLIMIT;
	return TRADE_CANCEL_NO;
}

BYTE CTrade::CheckTradeOtherCanStart(CPlayerObj* pPlayer,bool bStart)
{
	FUNCTION_BEGIN;
	BYTE btType = TRADE_CANCEL_NO;

	if(!pPlayer){
		btType = TRADE_CANCEL_OUTLINE;
	}
	else if (m_pMe->GetEnvir()->isNoSafeZone() ) {
		BYTE btType = TRADE_CANCEL_NOSAFE;
	}
	else if (pPlayer->m_btDayTradeCnt >= 10) {
		btType = TRADE_CANCEL_TARGETCNTLIMIT;
	}
	else if(bStart && pPlayer->m_Trade.GetTradeState()){
		btType = TRADE_CANCEL_TRADEING;
	}
	else if(!m_pMe->isInViewRange(pPlayer)){
		btType = TRADE_CANCEL_MOVEOUT;
	}
	else if(!(pPlayer->m_dwUserConfig & USERCONFIG_CANTRADE)){
		btType = TRADE_CANCEL_CONFIGNOAGREE;
	}
	return btType;
}
bool CTrade::CheckTrade(CPlayerObj* pPlayer,bool bStart)
{
	FUNCTION_BEGIN;
	BYTE btErrorType  = CheckMyCanTrade();
	if(btErrorType == TRADE_CANCEL_NO){
		btErrorType  = CheckTradeOtherCanStart(pPlayer,bStart);
		if(btErrorType == TRADE_CANCEL_NO){
			return true;
		}
	}
	return false;
}

void CTrade::ReadyTrade(CPlayerObj *pPlayer,emTradeType emTrade) //准备开始交易
{
	m_pTarget = pPlayer;
	m_boTrade = true;
	m_emTradeType = emTrade;
}	

bool CTrade::SetCommit()
{
	FUNCTION_BEGIN;
	if(m_boTrade)
	{
		m_boCommit = true;
		return true;
	}

	return false;
}

void CTrade::FinishTrade(BYTE btType)
{
	FUNCTION_BEGIN;
	m_pTarget->m_btDayTradeCnt += 1;
	m_pMe->m_btDayTradeCnt += 1;
	// 移除物品
	m_pTarget->m_Trade.RemoveItemFromMyBag();
	RemoveItemFromMyBag();
	// 物品交换，邮件发送
	m_pTarget->m_Trade.sendItemByMail(m_pMe);
	sendItemByMail(m_pTarget);
	// 金币交换
	if (m_dwGold) {
		if (m_btGoldType == tosizet(ResID::charge)) {
			m_pMe->ResChange(ResID::charge, -m_dwGold, "面对面交易");
		}else if (m_btGoldType == tosizet(ResID::game)) {
			m_pMe->ResChange(ResID::game, -m_dwGold, "面对面交易");
		}
	}
	if (m_pTarget->m_Trade.m_dwGold) {
		if (m_pTarget->m_Trade.m_btGoldType == tosizet(ResID::charge)) {
			m_pTarget->ResChange(ResID::charge, -(m_pTarget->m_Trade.m_dwGold), "面对面交易");
		}
		else if (m_pTarget->m_Trade.m_btGoldType == tosizet(ResID::game)) {
			m_pTarget->ResChange(ResID::game, -(double)(m_pTarget->m_Trade.m_dwGold), "面对面交易");
		}
	}
	m_pTarget->m_Trade.InitTrade();
	InitTrade();

}

void CTrade::Run()
{
	FUNCTION_BEGIN;
	if (m_pMe->m_btDayTradeCnt != 0 ) {
		zTime ztime;
		int nNowHour = ztime.getHour();
		if (nNowHour == 0) m_pMe->m_btDayTradeCnt = 0;
	}
	//if( GetTradeState() && CheckTrade(m_pTarget) ){
		/*if(m_pTarget && m_dwRequestTime >0 && timeGetTime() - m_dwRequestTime >_TIME_CANCEL_TRADE_){
			m_pTarget->m_Trade.CancelTrade(m_pTarget->getName(),TRADE_CANCEL_TIMEOUT);
			CancelTrade(m_pTarget->getName(),TRADE_CANCEL_TIMEOUT);
		} */
	//}
}

bool CTrade::ColseOhterAct()
{
	FUNCTION_BEGIN;
	//m_pMe->m_Send.CancelSend(m_pMe->getName(),SEND_CANCEL_STARTTRADE);
	return true;
}

void CTrade::OpenOhterLock()
{
	FUNCTION_BEGIN;
	if(m_pTarget->m_Trade.m_boLock)
	{
		m_pTarget->m_Trade.m_boLock = false;
		stLockTrade retcmd;
		retcmd.dwTmpID = m_pTarget->m_i64UserOnlyID;
		retcmd.boLock = false;
		SendToAll(&retcmd,sizeof(retcmd));
	}
}

bool CTrade::doCretCmd(stBaseCmd* pcmd,int ncmdlen)
{
	FUNCTION_BEGIN;

	switch(pcmd->value)
	{
	case stStartTrade::_value:	//发起交易
		{
			_CHECK_PACKAGE_LEN(stStartTrade,ncmdlen);
			stStartTrade *pStartTrade = (stStartTrade *)pcmd;
			CPlayerObj *pToPlayer = CUserEngine::getMe().m_playerhash.FindByOnlyId(pStartTrade->dwTargetID);
			if(!pToPlayer || (pToPlayer && pToPlayer == m_pMe)) return false;
			BYTE btErrorType = CheckMyCanTrade(); //当前状态
			if (btErrorType == TRADE_CANCEL_NO) {
				btErrorType = CheckTradeOtherCanStart(pToPlayer, true);
			}
			if (btErrorType == TRADE_CANCEL_NO) {
				if (GetTradeState()) {
					if (m_pTarget) {
						if (pToPlayer == m_pTarget) {
							UpadateTradeList(m_pMe, pToPlayer, TRADE_UPDATE);
						}
						else {
							// 旧申请对象更新列表
							UpadateTradeList(m_pMe, m_pTarget, TRADE_REMOVE);
							SendTradeList(m_pTarget);
							// 添加新的交易目标对象
							ReadyTrade(pToPlayer, (emTradeType)pStartTrade->btType);
							SetRequestTime(timeGetTime());
							UpadateTradeList(m_pMe, pToPlayer, TRADE_ADD);
						}
						SendTradeList(pToPlayer);
					}
				}else {
					ReadyTrade(pToPlayer, (emTradeType)pStartTrade->btType);
					SetRequestTime(timeGetTime());
					if (pStartTrade->btType == TRADE_FACETOFACE) {
						UpadateTradeList(m_pMe, pToPlayer, TRADE_ADD);
						SendTradeList(pToPlayer);
					}
				}
			}else {
				CPlayerObj::sendTipMsgByXml(m_pMe, GameService::getMe().GetStrRes(btErrorType, "facetrade"));
			}
		}break;
	case stAnswerTrade::_value:	//回应交易
		{
			_CHECK_PACKAGE_LEN(stAnswerTrade,ncmdlen);
			BYTE btErrorCode = TRADE_CANCEL_NO;
			if(!GetTradeState()){
				stAnswerTrade *pAnswerTrade = (stAnswerTrade *)pcmd;
				CPlayerObj* pTarget = CUserEngine::getMe().m_playerhash.FindByOnlyId(pAnswerTrade->targetid);
				if (pTarget) {
					if (pAnswerTrade->boAgree) {		//同意交易
						ReadyTrade(pTarget, TRADE_FACETOFACE); // 设置自己的交易状态
						SendBeginSta();
						pTarget->m_Trade.SendBeginSta();
						if (m_pTarget) m_pTarget->m_Trade.SetRequestTime(0);
					}else{
						pTarget->m_Trade.InitTrade();
						btErrorCode = TRADE_CANCEL_CONFIGNOAGREE;
					}
					UpadateTradeList(pTarget, m_pMe, TRADE_REMOVE);
					SendTradeList(m_pMe);
					m_pTarget->m_Trade.SendTipsMsg(btErrorCode);
				}else {	// 对方不在线
					/*UpadateTradeList(m_pMe, pTarget, TRADE_REMOVE);
					SendTradeList(pTarget);*/
					btErrorCode = TRADE_CANCEL_OUTLINE;
					m_pMe->m_Trade.SendTipsMsg(btErrorCode);
				}
			}
		}break;
	case stAddTradeItem::_value: //添加交易物品
		{
			_CHECK_PACKAGE_LEN(stAddTradeItem,ncmdlen);
			BYTE btErrorCode = TRADE_CANCEL_NO;
			if (GetTradeState()){
				if(!GetLockState())
				{
					stAddTradeItem *pAddTradeItem = (stAddTradeItem *)pcmd;
					if(pAddTradeItem->btType != 0) {
						if (pAddTradeItem->dwGold > 0) {
							if (CanAddGold(pAddTradeItem->dwGold, pAddTradeItem->btType)) {
								m_dwGold += pAddTradeItem->dwGold;
								m_btGoldType = pAddTradeItem->btType;
								SendTradeItem();
							}
						}else{
							CPlayerObj::sendTipMsgByXml(m_pMe, GameService::getMe().GetStrRes(TRADE_CANCEL_MONEYOVER, "facetrade"));
						}
					} else {
						if(GetListCount() < _MAX_TRADE_ITEMCOUNT_ && pAddTradeItem->nNum){
							CItem* pAddItem = m_pMe->m_Packet.FindItemInBag(pAddTradeItem->i64ItemID,pAddTradeItem->nNum); 
							if(pAddItem && pAddItem->GetCanTrade() ){
								int nRefNum = AddTempListItem_2(pAddItem, pAddTradeItem->nNum);
								if (nRefNum > 0) {
									SendTradeItem();
								}
							}else btErrorCode = TRADE_CANCEL_BINDING;
						}else btErrorCode = TRADE_CANCEL_LISTMAX;
					}
				}else btErrorCode = TRADE_CANCEL_LOCK;
			}else btErrorCode = TRADE_CANCEL_BREAK;
			if (btErrorCode != TRADE_CANCEL_NO) m_pMe->m_Trade.SendTipsMsg(btErrorCode);
		}break;
	case stRemoveTradeItem::_value:
		{
			_CHECK_PACKAGE_LEN(stRemoveTradeItem,ncmdlen);
			if (GetTradeState()){
				if(!GetLockState()) {
					stRemoveTradeItem *pRemoveTradeItem = (stRemoveTradeItem *)pcmd;
					if (pRemoveTradeItem->btType == 0)
					{
						if (RemoveTempListItem(pRemoveTradeItem->i64ItemID, pRemoveTradeItem->nNum))
					        SendTradeItem();
					}else{
						m_dwGold = ((m_dwGold - pRemoveTradeItem->dwGold) < 0) ? 0 : m_dwGold - pRemoveTradeItem->dwGold;
						if (m_dwGold == 0) m_btGoldType = 0;
						SendTradeItem();
					}
				}else{
					m_pMe->m_Trade.SendTipsMsg(TRADE_CANCEL_LOCK);
				}
			}else{
				m_pMe->m_Trade.SendTipsMsg(TRADE_CANCEL_BREAK);
			}
		}break;
	case stCancelTrade::_value:	//中断交易
		{
			_CHECK_PACKAGE_LEN(stCancelTrade,ncmdlen);
			AllCancelTrade(m_pMe->getName(),TRADE_CANCEL_BREAK);
		}break;
	case stLockTrade::_value:
		{
			_CHECK_PACKAGE_LEN(stLockTrade, ncmdlen);
			stLockTrade* pLock = (stLockTrade*)pcmd;
			if (GetTradeState())
			{
				if (!GetReadyTradeState()) { // 玩家准备交易状态不能解锁
					m_boLock = pLock->boLock;
					stLockTrade retcmd;
					retcmd.dwTmpID = m_pMe->m_i64UserOnlyID;
					retcmd.boLock = pLock->boLock;
					SendToAll(&retcmd, sizeof(retcmd));
				}else{
					m_pMe->m_Trade.SendTipsMsg(TRADE_CANCEL_ONTRADE);
				}
			}
		}break;
	case stRequestTrade::_value:
	{
		_CHECK_PACKAGE_LEN(stRequestTrade, ncmdlen);
		stRequestTrade* pRequest = (stRequestTrade*)pcmd;
		if (GetTradeState())
		{
			if (GetLockState() && m_pTarget->m_Trade.GetLockState())
			{
				SetReadyTradeState(true);
				m_pMe->m_Trade.SendRequestMsg(m_pTarget);
				m_pTarget->m_Trade.SendRequestMsg(m_pMe);
			}else{
				m_pMe->m_Trade.SendTipsMsg(TRADE_CANCEL_NOTLOCK);
			}
		}
	}break;
	case stCommitTrade::_value:	//确认交易
		{
			_CHECK_PACKAGE_LEN(stCommitTrade,ncmdlen);
			bool boCheckPlayer=true;
			if(m_pTarget && (m_pTarget->isSwitchSvr())){
				CancelTrade(m_pTarget->getName(),TRADE_CANCEL_OUTLINE);
				boCheckPlayer=false;
			}
			if(m_pMe && (m_pMe->isSwitchSvr()))
			{
				CancelTrade(m_pMe->getName(),TRADE_CANCEL_OUTLINE);
				boCheckPlayer=false;
			}
			if(boCheckPlayer){
				if(GetLockState() && m_pTarget->m_Trade.GetLockState())
				{
					if(SetCommit())
					{	
						m_pMe->m_Trade.SendCommitMsg(m_pTarget);
						m_pTarget->m_Trade.SendCommitMsg(m_pMe);
						bool boTargetCommit = m_pTarget->m_Trade.GetCommitState();
						if (boTargetCommit) {	//对方是否确定交易了
							FinishTrade();
						}
					}
				}
			}
		}break;
	}
	//if(pcmd->subcmd >= SUBCMD_SEND_CRETSTARTSEND && pcmd->subcmd <= SUBCMD_SEND_CERTREMOVEITEM)
	//	m_pMe->m_Send.doCretCmd(pcmd,ncmdlen);
	if(pcmd->subcmd >= SUBCMD_TONPC_GETSELLLISTANDRET)
		CNpcTrade::getMe().doCretCmd(m_pMe,pcmd,ncmdlen);
	return true;
}
