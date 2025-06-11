#include "ActivityCommon.h"
#include "UsrEngn.h"
#include "LocalDB.h"
#include "Item.h"
#include "GameMap.h"

bool SendSysMail(uint64_t dReceiveId, const char* szReceiveName, const char* szMailTitle, const char* szMailNotice, std::vector<stItem*> vecItemTab) {
	FUNCTION_BEGIN;
	if (!szMailTitle || !szMailNotice) return false;
	BUFFER_CMD(stMailSendNewMailInner, newMail, stBasePacket::MAX_PACKET_SIZE);
	newMail->MailDetail.i64ReceiverID = dReceiveId;
	CopyString(newMail->MailDetail.szReceiverName, UTG(szReceiveName));
	CopyString(newMail->MailDetail.szTitle, UTG(szMailTitle));
	CopyString(newMail->MailDetail.szNotice, UTG(szMailNotice));
	newMail->MailDetail.btGoldType = 0;
	newMail->MailDetail.dwGold = 0;

	int nMaxMailItemSize = min(vecItemTab.size(), 12);
	for (int i = 0; i < nMaxMailItemSize; i++) {
		stItem* item = vecItemTab[i];
		if (item)
		{
			newMail->MailDetail.ItemArr.push_back(*item, __FUNC_LINE__);
		}
	}
	for (int i = 0; i < vecItemTab.size(); i++) {
		stItem* pItem = vecItemTab[i];
		if (pItem)
		{
			CUserEngine::getMe().PushMailedItem(pItem);
		}
	}
	return CUserEngine::getMe().SendMailMsg2Super(newMail, sizeof(*newMail) + newMail->MailDetail.ItemArr.getarraysize(), CUserEngine::getMe().isCrossSvr() ? dReceiveId : 0xFFFFFFFFFFFFFFFF);
}

bool ActivityCommon::SendMail(double receiveid, const char* strPlayerName, const char* strTitle, const char* strContent, sol::table itemtab)
{
	if (!itemtab.valid()) { return false; }
	if (itemtab.size() >= 1){
		std::vector<stItem*> vecItemtab;
		vecItemtab.reserve(6);
		for (size_t i = 1; i <= itemtab.size(); i++)
		{
			if (itemtab[i].valid()){
				auto item = itemtab[i];
				int num = item["num"].get_or(0);
				int id = item["index"].get_or(0);
				int bind = item["bind"].get_or(0);
				if (id <= 0 || num <= 0) continue;
				
				auto itemdb = sJsonConfig.GetItemDataById(id);
				if (itemdb){

					auto addmailfunc = [&](int itemcount) {
						auto pitem = CUserEngine::getMe().GetMailedItem(id, itemcount, bind, 0, strTitle, "系统");
						vecItemtab.push_back(pitem);
						if (vecItemtab.size() >= 6)
						{
							SendSysMail((uint64_t)receiveid, strPlayerName, strTitle, strContent,vecItemtab);
							vecItemtab.clear();
						}
					};

					int maxcount = max(itemdb->nVariableMaxCount, itemdb->dwMaxCount);
					if (num < maxcount){
						addmailfunc(num);
					}
					else {
						int splitnum = num / maxcount;
						if (splitnum >= 30)
						{
							g_logger.error("邮件道具：%d,拆分的个数过大，请检查", id);
							return false;
						}
						int leftcnt = num % maxcount;
						for (int n = 1; n <= splitnum; n++)
						{
							addmailfunc(maxcount);
						}
						if (leftcnt > 0)
						{
							addmailfunc(leftcnt);
						}
					}
				}
				else {
					g_logger.error("不存在的邮件道具：%d", id);
					return false;
				}
			}
		}
		if (vecItemtab.size() > 0)
		{
			SendSysMail(receiveid, strPlayerName, strTitle, strContent, vecItemtab);
			vecItemtab.clear();
		}
		return true;
	}
	else {
		std::vector<stItem*> vecItemtab;
		SendSysMail(receiveid, strPlayerName, strTitle, strContent, vecItemtab);
		return true;
	}
	return false;
}

bool ActivityCommon::UpdateMapId2Global(DWORD dwMapUnionId, DWORD dwGuildId, WORD wSvrId)
{
	stGuildCloneMapRet cmd;
	cmd.dwMapUnionId = dwMapUnionId;
	cmd.dwGuildId = dwGuildId;
	cmd.wSvrId = wSvrId;
	CUserEngine::getMe().SendMsg2GlobalSvr(&cmd, sizeof(cmd));
	return true;
}