#include "NpcTrade.h"
#include "stringex.h"
#include "Chat.h"
#include "../gamesvr.h"

CNpcTrade::~CNpcTrade()
{
	FUNCTION_BEGIN;
}

int CNpcTrade::CreateBaseItem(char* szItemXml,std::vector<stItem>& itemlist){
	FUNCTION_BEGIN;
	zXMLParser xml;
	if (!xml.initStr(szItemXml))
	{
		return false;
	}

	xmlNodePtr root = xml.getRootNode("j");
	int nType = 0;
	xml.getNodePropNum(root,"t",nType);
	if (root)
	{
		DWORD nodenum=0;
		xmlNodePtr node=xml.getChildNode(root,NULL);
		int n=0;
		while (node)
		{
			if (strcmp(node->Value(),"i")==0)
			{
				stItem item;
				xml.getNodePropNum(node,"id",item.dwBaseID);
				if(auto pItemLoadBase = sJsonConfig.GetItemDataById(item.dwBaseID)){
					xml.getNodePropNum(node,"co",item.dwCount);
					if(item.dwCount ==0) item.dwCount=1;
					item.nMaxDura = pItemLoadBase->dwMaxDura;
					item.nDura = pItemLoadBase->dwMaxDura;
					itemlist.push_back(item);
				}else{
					return 0;
				}
			}
			node=xml.getNextNode(node,NULL);
		}
	}

	return nType;
}

void CNpcTrade::sendNpcSellItem(CPlayerObj *pPlayer,stGetNpcSellListAndRet* pCmd){
	
}

bool CNpcTrade::checkItemCanBuy(int nSellId,int nNpcId,DWORD dwItemId,int nType,stItem& item){
	stNpcSell sell ;
	if(m_cHashNpcTradeItem.find(nSellId,sell)){
		if(sell.nSellType != nType){
			return false;
		}

		if(!sell.hashItemList.find(dwItemId,item)){
			return false;
		}

		for(DWORD i=0;i<sell.vNpcIdList.size();i++){
			if(nNpcId == sell.vNpcIdList[i]){
				return true;
			}
		}
	}
	return false;
}


void CNpcTrade::buyBackItem(CPlayerObj *pPlayer,stToNpcBuyBack* pSrcCmd){
	
}

void CNpcTrade::buyItem(CPlayerObj *pPlayer,stToNpcBuy* pSrcCmd){
	
}

void CNpcTrade::sellItem(CPlayerObj *pPlayer,stToNpcSell* pSrcCmd){

	
}

void CNpcTrade::repairItem(CPlayerObj *pPlayer,stToNpcRepair* pSrcCmd){

	
}

CPlayerObj* m_pMe = NULL;
bool CNpcTrade::doCretCmd(CPlayerObj *pPlayer,stBaseCmd* pcmd,int ncmdlen,char* pszBillNo)
{
	FUNCTION_BEGIN;
	m_pMe = pPlayer;
	switch(pcmd->value)
	{
	case stToNpcVisit::_value:
		{
			ULONGLONG thistick=GetTickCount64();
			if (thistick>=m_pMe->m_dwVisitNpcIntervalTick){
				_CHECK_PACKAGE_LEN(stToNpcVisit,ncmdlen);
				stToNpcVisit *pSrcCmd =(stToNpcVisit *)pcmd;
				CCreature* pCret=m_pMe->GetEnvir()->GetCreature(pSrcCmd->dwtmpid);
				if ( pCret && (pCret->isCanVisit()) && !m_pMe->isDie() && m_pMe->isInViewRange(pCret) ){

					bool boCheckVisit = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("check_visit_npc",true,m_pMe,pCret);
					if (boCheckVisit) {
						m_pMe->m_pVisitNPC = pCret;
						if (pCret->isNpc()){
							m_pMe->TriggerEvent(m_pMe,NPCVISIT, pCret->toNpc()->getNpcScriptId());
							stAutoSetScriptParam autoparam(m_pMe);
							CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall("npcvisit", m_pMe, pCret->toNpc()->getNpcScriptId(), pCret->toNpc()->getNpcId());
						}
					}
				}
				m_pMe->m_dwVisitNpcIntervalTick=thistick+600;
			}
		}
		break;
	case stGetNpcSellListAndRet::_value:
		{
			if(CUserEngine::getMe().isCrossSvr()){
				return false;
			}
			_CHECK_PACKAGE_LEN(stGetNpcSellListAndRet,ncmdlen);
			CNpcTrade::getMe().sendNpcSellItem(m_pMe,(stGetNpcSellListAndRet*)pcmd);
		}break;
	case stToNpcBuy::_value:
		{
			if(CUserEngine::getMe().isCrossSvr()){
				return false;
			}
			_CHECK_PACKAGE_LEN(stToNpcBuy,ncmdlen);
			stToNpcBuy *pSrcCmd =(stToNpcBuy *)pcmd;
			CNpcTrade::getMe().buyItem(m_pMe,pSrcCmd);
		}break;
	case stToNpcSell::_value:
		{
			if(CUserEngine::getMe().isCrossSvr()){
				return false;
			}
			_CHECK_PACKAGE_LEN(stToNpcSell,ncmdlen);
			stToNpcSell *pSrcCmd =(stToNpcSell *)pcmd;
			if(pSrcCmd->ItemArr.getarraysize()<stBasePacket::MAX_PACKET_SIZE){
				CNpcTrade::getMe().sellItem(m_pMe,pSrcCmd);
			}
		}break;
	case stToNpcRepair::_value:
		{
			if(CUserEngine::getMe().isCrossSvr()){
				return false;
			}
			_CHECK_PACKAGE_LEN(stToNpcRepair,ncmdlen);
			stToNpcRepair *pSrcCmd =(stToNpcRepair *)pcmd;
			repairItem(m_pMe,pSrcCmd);
		}break;
	case stToNpcBuyBack::_value:
		{
			if(CUserEngine::getMe().isCrossSvr()){
				return false;
			}
			_CHECK_PACKAGE_LEN(stToNpcBuyBack,ncmdlen);
			stToNpcBuyBack *pSrcCmd =(stToNpcBuyBack *)pcmd;
			CNpcTrade::getMe().buyBackItem(m_pMe,pSrcCmd);
		}break;
	}

	return true;
}

