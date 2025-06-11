#ifndef _GAME_TRADE_H_FJIJFWIEURIWURI278345285HJJK44
#define _GAME_TRADE_H_FJIJFWIEURIWURI278345285HJJK44
#include <map>
#include "network/packet.h"
#include "Item.h"

#define _TIME_CANCEL_TRADE_	30000		//���׿�������ʱ��
#define _MAX_TRADE_ITEMCOUNT_	12		//���������
#define _MAX_SEND_ITEMCOUNT_ 5			//����������
#define _SYSTEM_NOCHECK_SCRIPTID_	2	//ϵͳ�����Ľű�ID
#define _MAX_TRADECOUNT_	10	        //ÿ�ս���������
#define _TIME_KEEPTRADELIST_	300	    //�����б���ʱ�� 5����

enum BuyMallItem_log{	//������־
	_BUYMALLITEMLOG_YUANBAO	=0,
	_BUYMALLITEMLOG_ZENGBAO	=1,
	_BUYMALLITEMLOG_JIFEN	=2,
};

enum emTradeType{
	TRADE_NORMAL,		 //��ͨ����
	TRADE_FACETOFACE ,	 //��ҽ��� ��ֻ��ͬ����Ҳſɻ��ཻ�ף�
};

enum emTradeListType {
	TRADE_UPDATE = 1,	  //����
	TRADE_ADD,	          //����
	TRADE_REMOVE,	      //�Ƴ�
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
	TempItemList m_TempItemListMap;			//����ʱ��ŵ���Ʒ
	CPlayerObj *m_pMe;						//�����Ľ�ɫ
	CPlayerObj *m_pTarget;					//���׶���
	DWORD m_dwGold;							//���׻�������
	BYTE m_btGoldType;						//���׻������� ��ResID


protected:
	void init();
	bool OtherCanLoadItem(CItem *pItem,bool bTrade = false);				//�����ܷ�װ����Ʒ
	bool CanAddGold(int nAddGold = 0, int GoldByte = 0);	//���������
	bool MyGoldEnough(bool bTrade =false);
	int GetListCount() {return m_TempItemListMap.size();}
	CTempItemList();
	virtual ~CTempItemList();
	bool AddTempListItem(CItem *pItem,int nNum);						     //�����Ʒ
	int AddTempListItem_2(CItem *pItem,int nNum);						     //�����Ʒ�ɵ��ӣ�����������Ϣ
	stTempOutItem* FindTempListItem(__int64 i64ItemID);					     //���ҽ�����Ʒ
	bool RemoveTempListItem(__int64 i64ItemID, int nNum = 1);				 //ɾ����Ʒ
	void RemoveAllTempListItem(bool boSend=false);							 //ɾ��������Ʒ
	void AddItemToOhterBag(std::string & szItemNames,emTradeType emTrade);	 //����Ʒ��ӵ����˰���
	void RemoveItemFromMyBag();											     //����Ʒ�Ƴ��Լ��İ���
	void sendItemByMail(CPlayerObj* pPlayer);							     //����Ʒͨ���ʼ�����
	void SendToAll(void *pBuf,int nLen);							         //������Ϣ����ؽ�ɫ
	void SendTradeLog(int num, const char* szName);							 //������Ʒ��־
public:
	int GetCountFromList(BYTE btType,bool bAbType = false);								//���߾�����Ʒ�������Ͳ�������
};

////////////////////////////////////////////////////////////////
//class CSend : public CTempItemList
//{
//private:
//	bool m_boSend;							//�Ѿ���ʼ���趫����
//	CCreature		*m_pVisitNPC;	//��ǰ���ʵ�npc
//public:
//	CSend(CPlayerObj* owner);
//	virtual ~CSend() {;};
//	void Run();	
//	_inline bool GetSendState() {return m_boSend;}
//	bool doCretCmd(stBaseCmd* pcmd,int ncmdlen);	//ִ�н�������
//	void InitSend() {__super::init();m_boSend = false;m_pVisitNPC = NULL;};
//	void FinishSend();
//	bool CheckCanSend(CPlayerObj* pPlayer);
//	void CancelSend(const char *pszCancelName,BYTE btType);
//};
//////////////////////////////////////////////////////////////////
class CTrade : public CTempItemList
{
private:
	bool m_boTrade;							//���׽�����
	bool m_boReadyTrade;					//׼�����ף��ȴ�˫������ȷ�Ͻ���
	bool m_boCommit;						//�Ƿ�ȷ�Ͻ���
	bool m_boLock;							//�Ƿ�����
	DWORD	m_dwRequestTime;				//�����׷���ʱ��
	emTradeType m_emTradeType;				//��������
public:
	CTrade(CPlayerObj* owner);
	virtual ~CTrade() {;};
	void Run();	
	bool doCretCmd(stBaseCmd* pcmd,int ncmdlen);	//ִ�н�������
	void SendTradeList(CPlayerObj* player);  //�����������б�
	void UpadateTradeList(CPlayerObj* p, CPlayerObj* target, BYTE btType);               //�Ƴ�/����/����������
	_inline bool GetTradeState() {return m_boTrade;}	//�����Ƿ���״̬
	_inline bool GetReadyTradeState() {return m_boReadyTrade; }
	_inline bool GetCommitState() {return m_boCommit;}	//�����Ƿ��ύ����״̬
	_inline bool GetLockState(){return m_boLock;}		//��������״̬
	_inline void SetReadyTradeState(bool bo) {m_boReadyTrade = bo;}
	void OpenOhterLock();								//�򿪶Է�������״̬
	void AllCancelTrade(const char *pszCancelName,BYTE btType = 2);				//ȫȡ������
	void SendTradeItem();				               //������Ʒ��Ϣ
	void SendBeginSta();				               //���׿�ʼ֪ͨ˫�����
	void SendBreakTips(BYTE btType);				               //��������
	void SendRequestMsg(CPlayerObj* pTarget);
	void SendCommitMsg(CPlayerObj* pTarget);
	void SendTipsMsg(BYTE btType);

private:
	void InitTrade();//��ʼ������
	bool CheckTrade(CPlayerObj* pPlayer,bool bStart = false);						//�ж��Ƿ��ܿ�������
	void ReadyTrade(CPlayerObj *pPlayer,emTradeType emTrade);						//׼����ʼ����
	void FinishTrade(BYTE btType = 0);												//��ɽ���
	void CancelTrade(const char *pszCancelName,BYTE btType = 2);					//ȡ������
	_inline void SetRequestTime(DWORD dwTime) {m_dwRequestTime = dwTime;};			//���õȴ��Է���Ӧʱ��

	BYTE CheckMyCanTrade();															//�ж��Լ��ܷ���
	BYTE CheckTradeOtherCanStart(CPlayerObj* pPlayer,bool bStart = false);			//�ж϶Է��ܷ���

	bool SetCommit();																//���ý���ȷ��״̬
	bool ColseOhterAct();																//���׵�ʱ��ر�������幦��

};
#endif