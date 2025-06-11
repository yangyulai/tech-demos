#ifndef __GAME_NPCTRADE_H__dfkojeifj2l3krr2
#define __GAME_NPCTRADE_H__dfkojeifj2l3krr2
#include "zsingleton.h"
#include "zXMLParser.h"
#include "HashManage.h"
#include "Npc.h"
#include "zLogger.h"
#include "cmd/Trade_cmd.h"

struct stNpcSellDB{		//npc出售数据库
	DWORD nID;
	char szSellItem[4000];
	char szSellNpcId[4000];
	stNpcSellDB(){
		ZEROSELF;
	}
};

static dbCol NpcSellDB_define[] = {
	{_DBC_SO_("sellid", DB_DWORD, stNpcSellDB, nID)}, 
	{_DBC_SO_("sellitem", DB_STR, stNpcSellDB, szSellItem)}, 
	{_DBC_SO_("allownpcid", DB_STR, stNpcSellDB, szSellNpcId)}, 
	{_DBC_MO_NULL_(stNpcSellDB)}, 
};

class CPlayerObj;

struct stNpcSell{
	std::vector<int> vNpcIdList;
	LimitHash<int,stItem> hashItemList;
	std::string hashItemListstr;
	int nSellType;
};

class CNpcTrade : public Singleton<CNpcTrade>
{
public:
	LimitHash<int,stNpcSell> m_cHashNpcTradeItem;

public:
	CNpcTrade(){;}
	~CNpcTrade();
	static int CreateBaseItem(char* szItemXml,std::vector<stItem>& itemlist);
	void sendNpcSellItem(CPlayerObj *pPlayer,stGetNpcSellListAndRet* pCmd);
	bool checkItemCanBuy(int nSellId,int nNpcId,DWORD dwItemId,int nType,stItem& item);
	void buyItem(CPlayerObj *pPlayer,stToNpcBuy* pSrcCmd);
	void buyBackItem(CPlayerObj *pPlayer,stToNpcBuyBack* pSrcCmd);
	void sellItem(CPlayerObj *pPlayer,stToNpcSell* pSrcCmd);
	void repairItem(CPlayerObj *pPlayer,stToNpcRepair* pSrcCmd);
	bool doCretCmd(CPlayerObj *pPlayer,stBaseCmd* pcmd,int ncmdlen,char* pszBillNo = NULL);
};
#endif