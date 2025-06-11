#ifndef _GAME_TRADE_H_FJIJFWIEURIWURI278345285HJJK44
#define _GAME_TRADE_H_FJIJFWIEURIWURI278345285HJJK44
#include <map>
#include "network/packet.h"
#include "Item.h"

#define _TIME_CANCEL_TRADE_	30000		//交易开启允许时间
#define _MAX_TRADE_ITEMCOUNT_	12		//最大交易数量
#define _MAX_SEND_ITEMCOUNT_ 5			//最大给予数量
#define _SYSTEM_NOCHECK_SCRIPTID_	2	//系统不检查的脚本ID
#define _MAX_TRADECOUNT_	10	        //每日交易最大次数
#define _TIME_KEEPTRADELIST_	300	    //交易列表保存时间 5分钟

enum BuyMallItem_log{	//氏族日志
	_BUYMALLITEMLOG_YUANBAO	=0,
	_BUYMALLITEMLOG_ZENGBAO	=1,
	_BUYMALLITEMLOG_JIFEN	=2,
};

enum emTradeType{
	TRADE_NORMAL,		 //普通交易
	TRADE_FACETOFACE ,	 //玩家交易 （只有同屏玩家才可互相交易）
};

enum emTradeListType {
	TRADE_UPDATE = 1,	  //更新
	TRADE_ADD,	          //增加
	TRADE_REMOVE,	      //移除
};

class CPlayerObj;

struct stTempOutItem{
	CItem *pItem;
	int nNum;
	stTempOutItem(){
		ZEROSELF;
	}
};

struct stBuyItem{
	DWORD dwItemid;
	DWORD dwMaxCount;
	DWORD dwCount;
	stBuyItem(){
		ZEROSELF;
	}
};

class CTempItemList
{
protected:
	typedef std::map<__int64,stTempOutItem*> TempItemList;
	TempItemList m_TempItemListMap;			//交易时存放的物品
	CPlayerObj *m_pMe;						//所属的角色
	CPlayerObj *m_pTarget;					//交易对象
	DWORD m_dwGold;							//交易货币数量
	BYTE m_btGoldType;						//交易货币类型 参ResID


protected:
	void init();
	bool OtherCanLoadItem(CItem *pItem,bool bTrade = false);				//对象能否装下物品
	bool CanAddGold(int nAddGold = 0, int GoldByte = 0);	//检查金币数量
	bool MyGoldEnough(bool bTrade =false);
	int GetListCount() {return m_TempItemListMap.size();}
	CTempItemList();
	virtual ~CTempItemList();
	bool AddTempListItem(CItem *pItem,int nNum);						     //添加物品
	int AddTempListItem_2(CItem *pItem,int nNum);						     //添加物品可叠加，返回数量信息
	stTempOutItem* FindTempListItem(__int64 i64ItemID);					     //查找交易物品
	bool RemoveTempListItem(__int64 i64ItemID, int nNum = 1);				 //删除物品
	void RemoveAllTempListItem(bool boSend=false);							 //删除所有物品
	void AddItemToOhterBag(std::string & szItemNames,emTradeType emTrade);	 //将物品添加到别人包裹
	void RemoveItemFromMyBag();											     //将物品移除自己的包裹
	void sendItemByMail(CPlayerObj* pPlayer);							     //将物品通过邮件发送
	void SendToAll(void *pBuf,int nLen);							         //发送消息给相关角色
	void SendTradeLog(int num, const char* szName);							 //交易物品日志
public:
	int GetCountFromList(BYTE btType,bool bAbType = false);								//更具具体物品包裹类型查找数量
};

////////////////////////////////////////////////////////////////
//class CSend : public CTempItemList
//{
//private:
//	bool m_boSend;							//已经开始给予东西了
//	CCreature		*m_pVisitNPC;	//当前访问的npc
//public:
//	CSend(CPlayerObj* owner);
//	virtual ~CSend() {;};
//	void Run();	
//	_inline bool GetSendState() {return m_boSend;}
//	bool doCretCmd(stBaseCmd* pcmd,int ncmdlen);	//执行交易命令
//	void InitSend() {__super::init();m_boSend = false;m_pVisitNPC = NULL;};
//	void FinishSend();
//	bool CheckCanSend(CPlayerObj* pPlayer);
//	void CancelSend(const char *pszCancelName,BYTE btType);
//};
//////////////////////////////////////////////////////////////////
class CTrade : public CTempItemList
{
private:
	bool m_boTrade;							//交易进行中
	bool m_boReadyTrade;					//准备交易，等待双方二次确认交易
	bool m_boCommit;						//是否确认交易
	bool m_boLock;							//是否锁定
	DWORD	m_dwRequestTime;				//请求交易发起时间
	emTradeType m_emTradeType;				//交易类型
public:
	CTrade(CPlayerObj* owner);
	virtual ~CTrade() {;};
	void Run();	
	bool doCretCmd(stBaseCmd* pcmd,int ncmdlen);	//执行交易命令
	void SendTradeList(CPlayerObj* player);  //发送申请人列表
	void UpadateTradeList(CPlayerObj* p, CPlayerObj* target, BYTE btType);               //移除/增加/更新申请人
	_inline bool GetTradeState() {return m_boTrade;}	//返回是否交易状态
	_inline bool GetReadyTradeState() {return m_boReadyTrade; }
	_inline bool GetCommitState() {return m_boCommit;}	//返回是否提交交易状态
	_inline bool GetLockState(){return m_boLock;}		//返回锁定状态
	_inline void SetReadyTradeState(bool bo) {m_boReadyTrade = bo;}
	void OpenOhterLock();								//打开对方的锁定状态
	void AllCancelTrade(const char *pszCancelName,BYTE btType = 2);				//全取消交易
	void SendTradeItem();				               //交易物品信息
	void SendBeginSta();				               //交易开始通知双方玩家
	void SendBreakTips(BYTE btType);				               //操作错误
	void SendRequestMsg(CPlayerObj* pTarget);
	void SendCommitMsg(CPlayerObj* pTarget);
	void SendTipsMsg(BYTE btType);

private:
	void InitTrade();//初始化交易
	bool CheckTrade(CPlayerObj* pPlayer,bool bStart = false);						//判断是否能开启交易
	void ReadyTrade(CPlayerObj *pPlayer,emTradeType emTrade);						//准备开始交易
	void FinishTrade(BYTE btType = 0);												//完成交易
	void CancelTrade(const char *pszCancelName,BYTE btType = 2);					//取消交易
	_inline void SetRequestTime(DWORD dwTime) {m_dwRequestTime = dwTime;};			//设置等待对方回应时间

	BYTE CheckMyCanTrade();															//判断自己能否交易
	BYTE CheckTradeOtherCanStart(CPlayerObj* pPlayer,bool bStart = false);			//判断对方能否交易

	bool SetCommit();																//设置交易确认状态
	bool ColseOhterAct();																//交易的时候关闭其他面板功能

};
#endif