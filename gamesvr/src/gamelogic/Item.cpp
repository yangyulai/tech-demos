#include "Item.h"
#include "cmd\Package_cmd.h"
#include "UsrEngn.h"
#include "Chat.h"
#include "Config.h"

CItem::CItem()
{
	m_boOutLock = false;
	m_dwNextCanUseTick = 0;
}

std::shared_ptr<stItemDataBase> CItem::GetItemDataBase() const
{
	if (auto sp = m_itemDataBase.lock()) {
		return sp;
	}
	auto fresh = sJsonConfig.GetItemDataById(m_Item.dwBaseID);
	if (!fresh)
	{
		g_logger.error("获取物品数据失败, 物品ID: %d", m_Item.dwBaseID);
		return nullptr;
	}
	m_itemDataBase = fresh;
	return fresh;
}
std::shared_ptr<stEffectDataLoaderBase> CItem::GetEffectDataBase() const
{
	auto itemBase = GetItemDataBase();
	if (!itemBase) return nullptr;
	auto fresh = itemBase->GetEffectDataBase();
	if (!fresh)
	{
        g_logger.error("获取物品效果数据失败, 物品ID: %d, 效果ID: %d" __FUNC_LINE__, itemBase->nID, itemBase->dwWarriorEffectId);
		return nullptr;
	}
	return fresh;
}

int64_t CItem::GenerateItemID()
{
	FUNCTION_BEGIN;
	return GameService::getMe().ItemIdGenerate();
}

int64_t CItem::GenerateVirtualItemID()
{
	FUNCTION_BEGIN;
	static int64_t g_i64VirtualItemID=_random(10000)+1000;
	if (g_i64VirtualItemID >= 0x00000000FFFFFFFE){
		g_i64VirtualItemID=_random(10000)+1000;
	}
	g_i64VirtualItemID++;
	return g_i64VirtualItemID;
}

CItem* CItem::CreateItem(DWORD dwId,emCreateType btCreateType,DWORD dwCount,DWORD dwMonType,const char* szMakeFrom,int frommapid,const char* bornfrom,const char *szmaker)
{
	FUNCTION_BEGIN;
	CItem* pItem=NULL;
	auto pItemLoadBase = sJsonConfig.GetItemDataById(dwId);
	if (pItemLoadBase){
		pItem=CUserEngine::getMe().m_itemPool.construct();
	}
	if (pItem){
		pItem->m_Item.dwBaseID = dwId;
		auto itemBase = pItem->GetItemDataBase();
		if (!itemBase) return nullptr;
		pItem->m_Item.btBornFrom=btCreateType;
		pItem->m_Item.BornFromMapid=frommapid;
		strcpy_s(pItem->m_Item.bornfrom,sizeof(pItem->m_Item.bornfrom)-1,bornfrom);
		strcpy_s(pItem->m_Item.szMaker,sizeof(pItem->m_Item.szMaker)-1,szmaker);
		pItem->m_Item.borntime=(DWORD)time(NULL);
		//属性标识ID

		if(itemBase)
		{
			pItem->m_Item.setdwBinding(itemBase->btBindType);
			pItem->m_Item.dwCount = 1;
			if (pItem->GetMaxCount()>= dwCount)
			{
				//获得随机数据///////////////////////////////////////////////////////////////////////
				pItem->RandomProperty(btCreateType,dwMonType);
				//获得强化数据///////////////////////////////////////////////////////////////////////
				//pItem->GetItemStrengNum();
				pItem->SetItemCount(dwCount);
				if (btCreateType!=_CREATE_NPC_CHEST){
					if (itemBase->dwType!=ITEM_TYPE_GOLD){
						pItem->m_Item.i64ItemID = GenerateItemID();
					}else{
						pItem->m_Item.i64ItemID = GenerateVirtualItemID();
					}
				}else{
					pItem->m_Item.i64ItemID = GenerateVirtualItemID();
				}
				static zLogger::zLevel tmploglvl(zLogger::zDEBUG.name,zLogger::zDEBUG.writelevel,6,zLogger::zDEBUG.realtimewrite,zLogger::zDEBUG.showcolor);
				g_logger.log(tmploglvl,"创建物品对象 %s %.16x %I64d %s",pItem->GetItemName(),size_t(pItem),pItem->m_Item.i64ItemID,szMakeFrom);
				pItem->SetSpecialProperty();
				pItem->SetNpProperty();
				return pItem;
			}
		}
	}
	CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);
	return NULL;
}

void CItem::SetSpecialProperty()
{
	FUNCTION_BEGIN;
	auto itemBase = GetItemDataBase();
	if (!itemBase) return;
	if(itemBase->btItemLimitTimeType==1)
	{
		m_Item.dwExpireTime = (DWORD)time(NULL) + itemBase->dwItemLimitTime;
	}else
	{
		m_Item.dwExpireTime = itemBase->dwItemLimitTime;
	}
}

BYTE CItem::GetWeaponJob() {
	FUNCTION_BEGIN;
	auto itemBase = GetItemDataBase();
	if (!itemBase) return 0;
	BYTE btJob = itemBase->btJobType;
	for (BYTE i = BOXER_JOB;i < MAX_JOB; i++)
	{
		if (btJob & ( 1 << i - 1)) return i;
	}
	return 0;
}

bool CItem::SaveEquipRndAtt(std::vector<stEquipAtt> EquipAtt,int random) {
	FUNCTION_BEGIN;
	if (EquipAtt.empty()) return false;
	int i = 0;
	for (i = 0; i < random;) {
		int index = _random(EquipAtt.size()-1, 0);
		if (index < EquipAtt.size()) {
			auto att = EquipAtt[index];
			if (att.btType == 0) {i++; continue;}
			if (CheckNpRepeat(att.btType) < 2) {
				int nNum = _random(att.nMaxNum, att.nMinNum);
				AddNpData(att.btFrom, att.btType, nNum);
				i++;
			}
		}
		else {
			i++;
		}
	}
	return true;
}

void CItem::SetCanUseTick(DWORD dwTick)
{
	if (auto itemBase = GetItemDataBase())
	{
		m_dwNextCanUseTick = dwTick ? dwTick : GetTickCount64() + itemBase->dwCdByTick * 0.97f;
	}
}

void CItem::SetNpProperty()
{
	FUNCTION_BEGIN;
	auto itemBase = GetItemDataBase();
	if (!itemBase) return;
	if (itemBase && itemBase->nMaxRndAtt && itemBase->nMinRndAtt) {
		int nRandomNum = _random(itemBase->nMaxRndAtt, itemBase->nMinRndAtt);
		if (nRandomNum == 0) return;
		if (itemBase->btEquipStation == EQUIP_WEAPONS) {
			BYTE JOB = GetWeaponJob();
			switch (JOB)
			{
			case BOXER_JOB: {
				SaveEquipRndAtt(sConfigMgr.m_WeaponBoxerAtt, nRandomNum);
			}break;
			case MAGE_JOB: {
				SaveEquipRndAtt(sConfigMgr.m_WeaponMageAtt, nRandomNum);
			}break;
			case SWORD_JOB: {
				SaveEquipRndAtt(sConfigMgr.m_WeaponSwordAtt, nRandomNum);
			}break;
			case GUN_JOB:{
				SaveEquipRndAtt(sConfigMgr.m_WeaponGunRndAtt, nRandomNum);
			}break;
			}
		}else{
			if (itemBase->btEquipStation == MECHANICAL_GUARD) {
				switch (itemBase->btGuardType)
				{
				case TYPE_ATTACK: {
					SaveEquipRndAtt(sConfigMgr.m_GuardAttackAtt, nRandomNum);
				}break;
				case TYPE_DEFENSE: {
					SaveEquipRndAtt(sConfigMgr.m_GuardDefenseAtt, nRandomNum);
				}break;
				case TYPE_SUPPORT: {
					SaveEquipRndAtt(sConfigMgr.m_GuardSupportAtt, nRandomNum);
				}break;
				}
			}else{
				SaveEquipRndAtt(sConfigMgr.m_ArmorRndAtt, nRandomNum);
			}
		
		}
	}
}

bool CItem::IfLimitedTime()//物品是否逾期,逾期则不能使用,逾期返回true
{
	FUNCTION_BEGIN;
	if(m_Item.dwExpireTime==0 || m_Item.dwExpireTime>(DWORD)time(NULL) )
	{
		return false;
	}
	return true;	
}
CItem* CItem::LoadItem(stItem *pstItem,const char* szMakeFrom)
{
	FUNCTION_BEGIN;

	if (!pstItem) return NULL;
	if (!pstItem->dwBaseID) return NULL;
	CItem* pItem=NULL;
	auto pItemLoadBase=sJsonConfig.GetItemDataById(pstItem->dwBaseID);
	if (pItemLoadBase){
		pItem=CUserEngine::getMe().m_itemPool.construct();
	}
	if (pItem){
		CopyMemory(&pItem->m_Item, pstItem, sizeof(stItem));
		if (pItem->m_Item.i64ItemID == 0)
		{
			if (pItem->m_Item.btBornFrom != _CREATE_NPC_CHEST) {
				if (pItemLoadBase->dwType != ITEM_TYPE_GOLD) {
					pItem->m_Item.i64ItemID = GenerateItemID();
				}
				else {
					pItem->m_Item.i64ItemID = GenerateVirtualItemID();
				}
			}
			else {
				pItem->m_Item.i64ItemID = GenerateVirtualItemID();
			}
		}		

		if(!pItemLoadBase){
			g_logger.error("物品 %d(%I64d) 基本数据不存在!",pItem->m_Item.dwBaseID,pItem->m_Item.i64ItemID);
			CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);
			return NULL;
		}
		if (pItem->m_Item.dwCount > pItemLoadBase->dwMaxCount)
		{
			if(pItemLoadBase->nVariableMaxCount==0){
				g_logger.error("物品 %d(%I64d) 当前数量(%d)超过最大数量,重置为最大数量!",pItem->m_Item.dwBaseID,pItem->m_Item.i64ItemID,pItem->m_Item.dwCount);
				pItem->m_Item.dwCount = pItemLoadBase->dwMaxCount;
			}else if(pItemLoadBase->nVariableMaxCount>0){
				 if(pItem->m_Item.dwCount > pItemLoadBase->nVariableMaxCount){
					g_logger.error("物品 %d(%I64d) 当前数量(%d)超过最大可变数量,重置为最大可变数量!",pItem->m_Item.dwBaseID,pItem->m_Item.i64ItemID,pItem->m_Item.dwCount);
					pItem->m_Item.dwCount = pItemLoadBase->nVariableMaxCount;
				}
			}
		}

		if( pItem->IfLimitedTime() ) 
		{
			g_logger.error("限时物品删除 %d(%I64d) !",pItem->m_Item.dwBaseID,pItem->m_Item.i64ItemID);
			CUserEngine::getMe().ReleasePItem(pItem,__FUNC_LINE__);
			return NULL;
		}
		static zLogger::zLevel tmploglvl(zLogger::zDEBUG.name,zLogger::zDEBUG.writelevel,6,zLogger::zDEBUG.realtimewrite,zLogger::zDEBUG.showcolor);
		g_logger.log(tmploglvl,"创建物品对象 %s %.16x %I64d %s",pItem->GetItemName(),size_t(pItem),pItem->m_Item.i64ItemID,szMakeFrom);
	
	
	}
	return pItem;
}

bool CItem::LoadItem()
{
	FUNCTION_BEGIN;
	auto itemBase = sJsonConfig.GetItemDataById(m_Item.dwBaseID);
	if(!itemBase)
	{
		g_logger.error("物品 %d(%I64d) 基本数据不存在!", m_Item.dwBaseID, m_Item.i64ItemID);
		return false;
	};
	if (m_Item.dwCount > itemBase->dwMaxCount)
	{
		if(itemBase->nVariableMaxCount==0){
			g_logger.error("物品 %d(%I64d) 当前数量(%d)超过最大数量,重置为最大数量!",m_Item.dwBaseID,m_Item.i64ItemID,m_Item.dwCount);
			m_Item.dwCount = itemBase->dwMaxCount;
		}else if(itemBase->nVariableMaxCount>0 ){
			if(m_Item.dwCount> itemBase->nVariableMaxCount){
				g_logger.error("物品 %d(%I64d) 当前数量(%d)超过最大可变数量,重置为最大可变数量!",m_Item.dwBaseID,m_Item.i64ItemID,m_Item.dwCount);
				m_Item.dwCount = itemBase->nVariableMaxCount;
			}
        }
	}
	return true;
}

DWORD CItem::GetDisappearTime()
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->dwDisappearTime;
	}
	return 0;
}

int CItem::GetFaceId() const
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->nFaceId;
	}
	return 0;
}

int CItem::GetDataStNum()
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->DataSYNum;
	}
	return 0;
}

emTypeDef CItem::GetType() const
{
	if (auto itemBase = GetItemDataBase())
	{
		return (emTypeDef)itemBase->dwType;
	}
	return (emTypeDef)0xff;
}

BYTE CItem::GetSex() const
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->btSexType;
	}
	return 0;
}

int CItem::GetLevelOrder() const
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->nQuality;
	}
	return 1;
}

BYTE CItem::GetGuardType() const
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->btGuardType;
	}
	return 0;
}

bool CItem::CanDestory()
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->boNoDestory;
	}
	return false;
}

int CItem::GetRare() const
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->nRare;
	}
	return 0;
}

BYTE CItem::GetEquipStation() const
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->btEquipStation;
	}
	return 0xff;
}

DWORD CItem::GetMaxDbCount(DWORD dwBaseId)
{
	FUNCTION_BEGIN;
	auto pstBase = sJsonConfig.GetItemDataById(dwBaseId);
	if (pstBase)
	{
		return  pstBase->dwMaxCount;
	}
	return 0;
}

void CItem::RandomProperty(emCreateType btCreateType,DWORD dwMonType)
{
	FUNCTION_BEGIN;
	static zLogger::zLevel tmploglvl(zLogger::zDEBUG.name,zLogger::zDEBUG.writelevel,6,zLogger::zDEBUG.realtimewrite,zLogger::zDEBUG.showcolor);
	g_logger.log(tmploglvl,"物品对象 %.8x RandomProperty",this);
	auto itemBase = GetItemDataBase();
	if (!itemBase) return;
	m_Item.nDura = itemBase->dwMaxDura;
	m_Item.nMaxDura = itemBase->dwMaxDura;
}

bool  CItem::GetItemProperty(CPlayerObj* pPlayer, int iPos, stARpgAbility& pattr)
{
	pattr.Clear();
	if (auto effect = GetEffectDataBase())
	{
		pattr.attrs += effect->attrs;
	}
	auto npAttr = GetNpPropertyTrans();
	auto dataAttr = GetLuaDataTrans();
	pattr += npAttr;
	pattr += dataAttr;
	return true;
}

bool  CItem::GetItemSpecialProperty(CPlayerObj* pPlayer, int iPos, stSpecialAbility* pattr)
{
	FUNCTION_BEGIN;
	if (pattr) {
		pattr->Clear();
		emJobType jobType = NO_JOB;
		if (pPlayer) { jobType = pPlayer->GetJobType(); }

		for (int i=0;i<m_Item.btNpPropertyCount;i++){
			if (m_Item.stNpProperty[i].dwNpNum==0){continue;}
			switch (m_Item.stNpProperty[i].btNpType)
			{
			case	NONPAREIL_TYPE_FullSkillLv: {pattr->nFullSkillLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_HundSkill: {pattr->nHundSkillLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_CounPunchLv: {pattr->nCounterpunchLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_ThrillLv: { pattr->nThrillLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_SteelSkinLv: { pattr->nSteelSkinLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_SelfMedicLv: { pattr->nSelfMedicationLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_MagicRefLv: { pattr->nMagicRefineLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_SpritStrenLv: { pattr->nSpiritStrengthenLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_MuseLv: { pattr->nMuseLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_PunctLv: { pattr->nPunctureLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_JuckLv: { pattr->nJuckLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_ConcentLv: { pattr->nConcentrationLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_RestoreLv: { pattr->nRestoreLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_SnipeLv: { pattr->nSnipeLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_ContFireLv: { pattr->nContinuousFiringLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_LiveyLv: { pattr->nLivelyLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_FireMstLv: { pattr->nFirearmsMasterLv += m_Item.stNpProperty[i].dwNpNum; }break;
			case	NONPAREIL_TYPE_SpellMstLv: { pattr->nSpellMasterLv += m_Item.stNpProperty[i].dwNpNum; }break;
			}
		}
		return true;
	}
	return false;
}

DWORD CItem::GetWearLevel()
{
	FUNCTION_BEGIN;
	DWORD nLevel = 0;
	auto itemBase = GetItemDataBase();
	if (!itemBase) return nLevel;
	nLevel = itemBase->dwNeedLevel;
	if (IsMechBody()){
		for (size_t i = 0; i < 4; i++)
		{
			auto itemId = m_Item.dwLuaData[i];
			if (itemId > 0)
			{
				auto pItemDB = sJsonConfig.GetItemDataById(itemId);
				if (pItemDB && pItemDB->nRare == 1) {
					nLevel += 1;
				}
				else {
					nLevel += 2;
				}
			}
		}
	}
	nLevel = max(0,nLevel);
	return nLevel;
}

DWORD CItem::GetFixCost()
{
	FUNCTION_BEGIN;
	DWORD nCostVal = 0;
	auto itemBase = GetItemDataBase();
	if (itemBase)
	{
		nCostVal = itemBase->dwFixCost;
	}
	nCostVal = max(0, nCostVal);
	return nCostVal;
}

DWORD CItem::GetBaseFixCost()
{
	FUNCTION_BEGIN;
	DWORD nCostVal = 0;
	auto itemBase = GetItemDataBase();
	if (itemBase)
	{
		nCostVal = itemBase->dwBaseFixCost;
	}
	nCostVal = max(0, nCostVal);
	return nCostVal;
}

void CItem::SetBornFrom(const char *bornfrom)
{
	if (bornfrom) {
		strcpy_s(m_Item.bornfrom, sizeof(m_Item.bornfrom) - 1, bornfrom);
	}
}

const char* CItem::GetBornFrom()
{
	return m_Item.bornfrom;
}

void CItem::SetBornTime(DWORD borntime)
{
	if (borntime) {
		m_Item.borntime = borntime;
	}
}

DWORD CItem::GetBornTime()
{
	return m_Item.borntime;
}

void CItem::Repair()
{
	FUNCTION_BEGIN;
	auto itemBase = GetItemDataBase();
	if (itemBase)
	{
		if(m_Item.nDura == 0xeefeeefe ||m_Item.nDura == 0xfeeefeee) 
		{
			g_logger.forceLog(zLogger::zERROR,"物品对象 %.8x 删除后继续使用。Repair" ,this);
		}
		m_Item.nDura= GetMaxDura();
	}
}

bool CItem::SaveLuaData(DWORD pos, DWORD data){
	FUNCTION_BEGIN;
	if (pos >= 1 && pos <= 8)
	{
		if(m_Item.dwLuaData[pos-1] == 0xeefeeefe || m_Item.dwLuaData[pos-1] == 0xfeeefeee) 
		{
			g_logger.forceLog(zLogger::zERROR,"物品对象 %.8x 删除后继续使用。SaveLuaData" ,this);
		}
		m_Item.dwLuaData[pos-1] = data;
		return true;
	}
	return false;
}

bool CItem::LoadLuaData(sol::table table){
	FUNCTION_BEGIN;
	if (!table.valid()) { return false; }
	sol::state_view lua = table.lua_state();
	sol::table subtable = lua.create_table();
	if (subtable.valid()) {
		for (size_t i = 1; i <= 4; i++)
		{
			if (m_Item.dwLuaData[i - 1] > 0){
				subtable["id"] = m_Item.dwLuaData[i - 1];
				subtable["bind"] = m_Item.dwLuaData[i + 4 - 1];
				table[i] = subtable;
			}
			else
				break;
		}
		return true;
	}
	return false;
}

void setItemPropertyTable (sol::table& table,BYTE btFrom,BYTE nptype,int npnum, BYTE btLv, BYTE btMaxLv) {
	FUNCTION_BEGIN;
	if (!table.valid()) { return; }
	sol::state_view lua = table.lua_state();
	sol::table subtable = lua.create_table();
	if (subtable.valid()) {
		subtable["from"]=btFrom;
		subtable["type"]=nptype;
		subtable["num"]=npnum;
		subtable["lv"]= btLv;
		subtable["maxlv"]= btMaxLv;
		table[table.size()+1] = subtable;
	}
}

bool CItem::SaveNpData(BYTE btFrom,BYTE bttype,int npnum, BYTE btLv, BYTE btMaxLv){
	FUNCTION_BEGIN;
	bool isOk = false;
	for (int i=0;i<m_Item.btNpPropertyCount && i < _MAX_NP_ALL_COUNT;i++) {
		if(btFrom == m_Item.stNpProperty[i].ntNpFrom && m_Item.stNpProperty[i].btNpType==bttype){
			m_Item.stNpProperty[i].dwNpNum += npnum;
			m_Item.stNpProperty[i].btNpLevel += btLv;
			if (m_Item.stNpProperty[i].dwNpNum == 0 && m_Item.btNpPropertyCount <= _MAX_NP_ALL_COUNT){
				for (int j = i, next = j + 1; next < m_Item.btNpPropertyCount; j++, next++) {
					m_Item.stNpProperty[j] = m_Item.stNpProperty[next];
				}
				m_Item.btNpPropertyCount--;
				m_Item.stNpProperty[m_Item.btNpPropertyCount].clear();
			}
			isOk = true;
			break;
		}
	}
	if (!isOk && m_Item.btNpPropertyCount < _MAX_NP_ALL_COUNT) {
		int pos=m_Item.btNpPropertyCount;
		m_Item.stNpProperty[pos].ntNpFrom=btFrom;
		m_Item.stNpProperty[pos].btNpType=bttype;
		m_Item.stNpProperty[pos].dwNpNum=npnum;
		m_Item.stNpProperty[pos].btNpLevel = btLv;
		m_Item.stNpProperty[pos].btNpMaxLevel = btMaxLv;
		m_Item.btNpPropertyCount++;
		isOk = true;
	}

	return isOk;
}

bool CItem::ChangeNpMaxLv(BYTE btFrom, BYTE bttype, BYTE btMaxLv) {
	FUNCTION_BEGIN;
	if (m_Item.btNpPropertyCount <= _MAX_NP_ALL_COUNT) {
		for (int i = 0; i < m_Item.btNpPropertyCount; i++) {
			if (btFrom == m_Item.stNpProperty[i].ntNpFrom && m_Item.stNpProperty[i].btNpType == bttype) {
				m_Item.stNpProperty[i].btNpMaxLevel = btMaxLv;
				break;
			}
		}
	}
	return true;
}


bool CItem::LoadNpData(sol::table table){
	FUNCTION_BEGIN;
	if (!table.valid()){return false;}
	if (table){
		for (int i=0;i<m_Item.btNpPropertyCount;i++)
		{
			setItemPropertyTable(table,m_Item.stNpProperty[i].ntNpFrom,m_Item.stNpProperty[i].btNpType,m_Item.stNpProperty[i].dwNpNum, m_Item.stNpProperty[i].btNpLevel, m_Item.stNpProperty[i].btNpMaxLevel);
		}
		return true;
	}
	return false;
}

sol::table CItem::LoadNpDataByFrom(BYTE btFrom, sol::state_view ts) {
	sol::state_view lua(ts);
	auto table = lua.create_table();
	for (int i = 0; i < m_Item.btNpPropertyCount; i++)
	{

		if (m_Item.stNpProperty[i].ntNpFrom == btFrom) {
			setItemPropertyTable(table, m_Item.stNpProperty[i].ntNpFrom, m_Item.stNpProperty[i].btNpType, m_Item.stNpProperty[i].dwNpNum, m_Item.stNpProperty[i].btNpLevel, m_Item.stNpProperty[i].btNpMaxLevel);
		}
	}
	return table;
}

bool CItem::GetExistNpData() {
	auto itemBase = GetItemDataBase();
	if (!itemBase) return false;
	for (int i = 0; i < m_Item.btNpPropertyCount; i++)
	{
		if (m_Item.stNpProperty[i].dwNpNum ){
			if (itemBase->btEquipStation == 0 && m_Item.stNpProperty[i].btNpType == 6){ // 武器幸运油不算
				continue;
			}
			return true;
		}
	}
	return false;
}

bool CItem::ClearNpByFromType( BYTE btFrom,BYTE bttype )
{
	FUNCTION_BEGIN;
	bool boFind = false;
	if(m_Item.btNpPropertyCount<_MAX_NP_ALL_COUNT){
		for(int i=0;i<=m_Item.btNpPropertyCount;i++){
			if (boFind) {
				m_Item.stNpProperty[i-1] = m_Item.stNpProperty[i];
			}else {
				if(btFrom == m_Item.stNpProperty[i].ntNpFrom && m_Item.stNpProperty[i].btNpType==bttype){
					boFind = true;
				}
			}
		}
	}
	if (boFind) {
		m_Item.stNpProperty[m_Item.btNpPropertyCount].ntNpFrom = 0;
		m_Item.stNpProperty[m_Item.btNpPropertyCount].btNpType = 0;
		m_Item.stNpProperty[m_Item.btNpPropertyCount].dwNpNum = 0;
		m_Item.stNpProperty[m_Item.btNpPropertyCount].btNpLevel = 0;
		m_Item.stNpProperty[m_Item.btNpPropertyCount].btNpMaxLevel = 0;
		m_Item.btNpPropertyCount -= 1;
	}
	return boFind;
}

bool CItem::ClearNpByFrom( BYTE btFrom)
{
	FUNCTION_BEGIN;
	if (NONPAREIL_TYPE_MAXCOUNT <= btFrom) {
		return false;
	}
	stNonpareil tmpNP[_MAX_NP_ALL_COUNT];
	BYTE tmpCount = 0;
	for (int i = 0; i < m_Item.btNpPropertyCount; i++) {
		if (m_Item.stNpProperty[i].ntNpFrom != btFrom) {
			tmpNP[tmpCount] = m_Item.stNpProperty[i];
			tmpCount++;
		}
	}
	CopyMemory(&m_Item.stNpProperty[0], &tmpNP[0], sizeof(tmpNP));
	m_Item.btNpPropertyCount = tmpCount;
	return true;
}

bool CItem::ClearNpByType(BYTE bttype){
	FUNCTION_BEGIN;
	if(NONPAREIL_TYPE_MAXCOUNT <= bttype){
		return false;
	}
	stNonpareil tmpNP[_MAX_NP_ALL_COUNT];
	BYTE tmpCount = 0;
	for(int i=0;i<m_Item.btNpPropertyCount;i++){
		if(m_Item.stNpProperty[i].btNpType!=bttype){
			tmpNP[tmpCount] = m_Item.stNpProperty[i];
			tmpCount++;
		}
	}
	CopyMemory(&m_Item.stNpProperty[0], &tmpNP[0], sizeof(tmpNP));
	m_Item.btNpPropertyCount = tmpCount;
	return true;
}

bool CItem::ClearNpData(){
	FUNCTION_BEGIN;
	m_Item.btNpPropertyCount=0;
	ZeroMemory(&m_Item.stNpProperty[0],sizeof(m_Item.stNpProperty));
	return true;
}


bool CItem::AddNpData(BYTE btPos,BYTE bttype,int npnum,BYTE btLv,BYTE btMaxLv){
	FUNCTION_BEGIN;
	if(m_Item.btNpPropertyCount<_MAX_NP_ALL_COUNT){
		int pos=m_Item.btNpPropertyCount;
		m_Item.stNpProperty[pos].ntNpFrom=btPos;
		m_Item.stNpProperty[pos].btNpType=bttype;
		m_Item.stNpProperty[pos].dwNpNum=npnum;
		m_Item.stNpProperty[pos].btNpLevel = btLv;
		m_Item.stNpProperty[pos].btNpMaxLevel = btMaxLv;
		m_Item.btNpPropertyCount++;
		return true;
	}
	return false;
}

int CItem::CheckNpRepeat(BYTE btType) {
	FUNCTION_BEGIN;
	int tmpCount = 0;
	if (m_Item.btNpPropertyCount < _MAX_NP_ALL_COUNT) {
		for (int i = 0; i < m_Item.btNpPropertyCount; i++) {
			if (m_Item.stNpProperty[i].btNpType == btType) {
				tmpCount++;
			}
		}
	}
	return tmpCount;
}

bool CItem::DelNpDataByPos(BYTE btPos){
	FUNCTION_BEGIN;
	bool boFind = false;
	if(m_Item.btNpPropertyCount <= _MAX_NP_ALL_COUNT){
		for(int i=0; i < m_Item.btNpPropertyCount;){
			if(m_Item.stNpProperty[i].ntNpFrom == btPos){
				m_Item.stNpProperty[i].ntNpFrom = 0;
				m_Item.stNpProperty[i].btNpType = 0;
				m_Item.stNpProperty[i].dwNpNum  = 0;
				m_Item.stNpProperty[i].btNpLevel = 0;
				m_Item.stNpProperty[i].btNpMaxLevel = 0;
				for(int j=i, next=j+1; next < m_Item.btNpPropertyCount; j++, next++){
					m_Item.stNpProperty[j] = m_Item.stNpProperty[next];
				}
				m_Item.btNpPropertyCount--;
				m_Item.stNpProperty[m_Item.btNpPropertyCount].clear();
				boFind = true;
			}else{
				i++;
			}
		}
	}
	return boFind;
}

bool CItem::ChangeNpData(BYTE btFrom,BYTE bttype,int npnum,BYTE btLv,BYTE btMaxLv){
	FUNCTION_BEGIN;
	bool boFind = false;
	if(m_Item.btNpPropertyCount<=_MAX_NP_ALL_COUNT){
		for(int i=0;i<m_Item.btNpPropertyCount;i++){
			if(btFrom == m_Item.stNpProperty[i].ntNpFrom && m_Item.stNpProperty[i].btNpType==bttype){
				m_Item.stNpProperty[i].dwNpNum = npnum;
				m_Item.stNpProperty[i].btNpLevel= btLv;
				m_Item.stNpProperty[i].btNpMaxLevel = btMaxLv;
				boFind = true;
				break;
			}
		}

		if(!boFind && m_Item.btNpPropertyCount<_MAX_NP_ALL_COUNT){
			int pos=m_Item.btNpPropertyCount;
			m_Item.stNpProperty[pos].ntNpFrom = btFrom;
			m_Item.stNpProperty[pos].btNpType = bttype;
			m_Item.stNpProperty[pos].dwNpNum = npnum;
			m_Item.stNpProperty[pos].btNpLevel = btLv;
			m_Item.stNpProperty[pos].btNpMaxLevel = btMaxLv;
			m_Item.btNpPropertyCount++;

		}
		return true;
	}
	return false;
}



void CItem::SetItemLog(const char* szText, CCreature * pSrcCret,char* pszDestName,int nDestLvl,bool boForceRecord,int UseCount,DWORD dwPrice,Byte btPriceType)
{
	FUNCTION_BEGIN;
	/*
	opttype	物品的操作类型
	account	操作的玩家帐号
	srconlyid	操作的玩家的onlyid
	srcname	操作的角色名称
	srclvl	操作的角色等级
	destname操作的目标名称（交易等操作才有）
	destlvl 操作的目标等级
	srcgoldCount交易的身上金币数量
	goldchange 交易的金币改变值
	itemname 物品名称
	itemcount 物品数量
	usecount 物品使用数量
	itemi64id 物品的id
	itematatus 物品的属性（针对有附加属性的道具）
	optmapname 操作地图
	optmapX
	optmapY

	opttype,account,srconlyid,srcname,srclvl,destname,destlvl,
	srcgoldCount,goldchange,itemname,itemcount,itemi64id,itematatus,optmapname,optmapX,optmapY
	*/
	auto itemBase = GetItemDataBase();
	if(itemBase){
		if (itemBase->nLogOpd && CUserEngine::getMe().m_openDay >= itemBase->nLogOpd)	// 判断开服天数
			return;
		if (itemBase->dwLog>0 && (DWORD)abs(UseCount)>= itemBase->dwLog)
		{
			CPlayerObj* pDestPlayer = NULL;
			if(pszDestName){
				pDestPlayer = CUserEngine::getMe().m_playerhash.FindByName(pszDestName);
			}
			GameService::getMe().Send2LogSvr(_SERVERLOG_ITEM_, 7,0,(pSrcCret && pSrcCret->isPlayer())?pSrcCret->toPlayer():NULL,
				"'%s','%s',%I64d,'%s',%d,'%s',%d,%I64u,'%s',%d,%d,%d,%d,%d,%d,'%s'",
				szText,
				((pSrcCret && pSrcCret->isPlayer())?pSrcCret->toPlayer()->getAccount():""),
				((pSrcCret && pSrcCret->isPlayer())?pSrcCret->toPlayer()->m_i64UserOnlyID:0),
				(pSrcCret?pSrcCret->getName():""),
				((pSrcCret && pSrcCret->isPlayer())?pSrcCret->m_dwLevel:0),
				(GetItemName()?GetItemName():""),
				m_Item.dwCount,
				m_Item.i64ItemID,
				(pSrcCret?pSrcCret->GetEnvir()->getFullMapName():""),
				(pSrcCret?pSrcCret->m_nCurrX:0),
				(pSrcCret?pSrcCret->m_nCurrY:0),
				UseCount,
				dwPrice,
				btPriceType,
				itemBase->nLogType,
				((pSrcCret && pSrcCret->isPlayer()) ? pSrcCret->toPlayer()->m_pGateUser->szclientip : "")
				);
		}
		else {
			//g_logger.debug("玩家:%s,道具:%s,数量:%d,变化数量:%d,日志类型:%s,", pSrcCret ? pSrcCret->getName() : "", GetItemName() ? GetItemName() : "", m_Item.dwCount, UseCount, szText);
		}
	}
}

void CItem::SetItemLog(const char* szText, CCreature * pSrcCret,CCreature * pDestCret,bool boForceRecord,int UseCount,DWORD dwPrice,Byte btPriceType)
{
	FUNCTION_BEGIN;
	SetItemLog(szText,pSrcCret,pDestCret?(char*)pDestCret->getName(): (char*)"",
		(pDestCret && pDestCret->isPlayer())?pSrcCret->m_dwLevel:0,boForceRecord,UseCount,dwPrice,btPriceType);
}

void CItem::LuaSetItemLog(const char* szText,CCreature * pSrcCret,CCreature * pDestCret, bool boForceRecord, int UseCount){
	FUNCTION_BEGIN;
	SetItemLog((char*)szText,pSrcCret,pDestCret,boForceRecord,UseCount);
}

bool CItem::CanLog()
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->dwLog>=1;
	}
	return false;
}

BYTE CItem::NoticeDay()
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->btNotice;
	}
	return 0;
}

void CItem::SetMakerName(const char* szNewMaker){
	//if(((DWORD)&m_Item.szMaker[0]) == 0xeefeeefe || ((DWORD)&m_Item.szMaker[0]) == 0xfeeefeee) 
	//{
	//	g_logger.forceLog(zLogger::zERROR,"物品对象 %.16x 删除后继续使用。SetMakerName" ,this);
	//}
	strcpy_s(m_Item.szMaker,_MAX_NAME_LEN_-1,szNewMaker);
}

DWORD CItem::GetSellPrice()
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->nSellPrice;
	}
	return 0;
}

void   CItem::SetItemCount(int nCount) 
{
	//if(m_Item.dwCount == 0xeefeeefe ||m_Item.dwCount == 0xfeeefeee) 
	//{
	//	g_logger.forceLog(zLogger::zERROR,"物品对象 %.16x 删除后继续使用。SetItemCount" ,this);
	//}
	m_Item.dwCount = nCount;
} //设置物品数量


void CItem::SetItemLocation(BYTE nCount)
{
	m_Item.Location.btIndex = nCount;
}
BYTE CItem::GetItemLocation()
{
	return m_Item.Location.btIndex;
}

DWORD CItem::GetMaxCount() const
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->dwMaxCount;
	}
	return 0;
}


void CItem::SetOutLock(bool b) 
{
	FUNCTION_BEGIN;
	if((BYTE)m_boOutLock == 0xee ||(BYTE)m_boOutLock == 0xfe) 
	{
		g_logger.forceLog(zLogger::zERROR,"物品对象 %.16x 删除后继续使用。SetOutLock" ,this);
	}
	m_boOutLock = b;
}

const char* CItem::GetItemName() const
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->szName.c_str();
	}
	return nullptr;
}

void CItem::SetBinding(BYTE btBinding)
{
	if(m_Item.dwBinding == 0xee ||m_Item.dwBinding == 0xfe) 
	{
		g_logger.forceLog(zLogger::zERROR,"物品对象 %.16x 删除后继续使用。SetBinding" ,this);
	}
	m_Item.setdwBinding(btBinding);
}

void CItem::SetDura(int nDura)	
{
	FUNCTION_BEGIN;
	if (nDura < 0) 
		nDura = 0; 
	if(m_Item.nDura == 0xeefeeefe ||m_Item.nDura == 0xfeeefeee) 
	{
		g_logger.forceLog(zLogger::zERROR,"物品对象 %.16x 删除后继续使用。SetDura" ,this);
	}
	m_Item.nDura= nDura;
}

void CItem::SetPreventDropCount(int nCount)
{
	FUNCTION_BEGIN;
	if (nCount < 0)
		nCount = 0;
	if (m_Item.nPreventDropCount == 0xeefeeefe || m_Item.nPreventDropCount == 0xfeeefeee)
	{
		g_logger.forceLog(zLogger::zERROR, "物品对象 %.16x 删除后继续使用。SetPreventDropCount", this);
	}
	m_Item.nPreventDropCount = nCount;
}

void CItem::SetMaxDura(int nMaxDura){
	FUNCTION_BEGIN;
	if (nMaxDura < 0)
		nMaxDura = 0;
	if(m_Item.nMaxDura == 0xeefeeefe ||m_Item.nMaxDura == 0xfeeefeee) 
	{
		g_logger.forceLog(zLogger::zERROR,"物品对象 %.16x 删除后继续使用。SetMaxDura" ,this);
	}
	m_Item.nMaxDura=nMaxDura;
}

DWORD CItem::GetBuyPrice()
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->nBuyPrice;
	}
	return 0xffffffff;
}

void CItem::GetItemNPLog(char* strlog,int nsize)
{
	FUNCTION_BEGIN;
	std::stringstream str;
	auto itemBase = GetItemDataBase();
	if(itemBase->dwType == ITEM_TYPE_EQUIP ){
		str << (int)m_Item.btStrengCount << "@";
		str << "0=2|";
	}
	for (int i=0;i<m_Item.btNpPropertyCount;i++)
	{
		str << vformat("%d=%d|",m_Item.stNpProperty[i].btNpType,m_Item.stNpProperty[i].dwNpNum);
	}
	strcpy_s(strlog,nsize-1,str.str().c_str());
}

int CItem::GetSuitId()
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->dwSuitId;
	}
	return 0;
}

int CItem::GetSuitType()
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->nSuitType;
	}
	return 0;
}

stARpgAbility CItem::GetNpPropertyTrans() {
	stARpgAbility pattr;
	for (int i = 0; i < m_Item.btNpPropertyCount; i++) {
		if (m_Item.stNpProperty[i].dwNpNum == 0) { continue; }
		switch (m_Item.stNpProperty[i].btNpType)
		{
		case	NONPAREIL_TYPE_Strength: { pattr.attrs[tosizet(AttrID::Strength)] += m_Item.stNpProperty[i].dwNpNum; } break; //力量
		case	NONPAREIL_TYPE_Agility: { pattr.attrs[tosizet(AttrID::Agility)] += m_Item.stNpProperty[i].dwNpNum; }break;  //敏捷
		case	NONPAREIL_TYPE_Wisdom: {pattr.attrs[tosizet(AttrID::Wisdom)] += m_Item.stNpProperty[i].dwNpNum; } break;   //智慧
		case	NONPAREIL_TYPE_Physique: {pattr.attrs[tosizet(AttrID::Physique)] += m_Item.stNpProperty[i].dwNpNum; } break; //体质
		case	NONPAREIL_TYPE_Intel: {pattr.attrs[tosizet(AttrID::Intelligence)] += m_Item.stNpProperty[i].dwNpNum; } break; //智力
		case	NONPAREIL_TYPE_Hit: {pattr.attrs[tosizet(AttrID::Hit)] += m_Item.stNpProperty[i].dwNpNum * 100; } break;
		case	NONPAREIL_TYPE_Juck: {pattr.attrs[tosizet(AttrID::Juck)] += m_Item.stNpProperty[i].dwNpNum * 100; }  break;
		case	NONPAREIL_TYPE_MoveSpeed: {pattr.attrs[tosizet(AttrID::MoveSpeedPer)] += m_Item.stNpProperty[i].dwNpNum; } break;
		case	NONPAREIL_TYPE_AttackSpeed: {pattr.attrs[tosizet(AttrID::AttackSpeedPer)] += m_Item.stNpProperty[i].dwNpNum; } break;
		case	NONPAREIL_TYPE_FirstAbi: {
			pattr.attrs[tosizet(AttrID::Strength)] += m_Item.stNpProperty[i].dwNpNum;
			pattr.attrs[tosizet(AttrID::Agility)] += m_Item.stNpProperty[i].dwNpNum;
			pattr.attrs[tosizet(AttrID::Wisdom)] += m_Item.stNpProperty[i].dwNpNum;
			pattr.attrs[tosizet(AttrID::Physique)] += m_Item.stNpProperty[i].dwNpNum;
			pattr.attrs[tosizet(AttrID::Intelligence)] += m_Item.stNpProperty[i].dwNpNum;
		} break;
		case	NONPAREIL_TYPE_MDef: {pattr.attrs[tosizet(AttrID::MDef)] += m_Item.stNpProperty[i].dwNpNum; } break;
		case	NONPAREIL_TYPE_PAttack: {
			pattr.attrs[tosizet(AttrID::MaxPAtk)] += m_Item.stNpProperty[i].dwNpNum;
			pattr.attrs[tosizet(AttrID::MinPAtk)] += m_Item.stNpProperty[i].dwNpNum;
		} break;
		case	NONPAREIL_TYPE_MAttack: {
			pattr.attrs[tosizet(AttrID::MaxPAtk)] += m_Item.stNpProperty[i].dwNpNum;
			pattr.attrs[tosizet(AttrID::MinPAtk)] += m_Item.stNpProperty[i].dwNpNum;
		} break;				  
		case	NONPAREIL_TYPE_PDefPer: {pattr.attrs[tosizet(AttrID::PDef)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_RelSpeedPer: {pattr.attrs[tosizet(AttrID::ReleaseSpeedPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MaxHpPer: {pattr.attrs[tosizet(AttrID::MaxHpPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MaxHp: { pattr.attrs[tosizet(AttrID::MaxHP)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MaxMp: { pattr.attrs[tosizet(AttrID::MaxMP)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MinPAtk: { pattr.attrs[tosizet(AttrID::MinPAtk)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MaxPAtk: { pattr.attrs[tosizet(AttrID::MaxPAtk)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MaxMAtk: { pattr.attrs[tosizet(AttrID::MaxMAtk)] += m_Item.stNpProperty[i].dwNpNum; } break;
		case	NONPAREIL_TYPE_MinMAtk: { pattr.attrs[tosizet(AttrID::MinMAtk)] += m_Item.stNpProperty[i].dwNpNum; }break;
			//case	NONPAREIL_TYPE_PAtkInc: { pattr.attrs[tosizet(AttrID::PAtkInc)] += m_Item.stNpProperty[i].dwNpNum; }break;
			//case	NONPAREIL_TYPE_MAtkInc: { pattr.attrs[tosizet(AttrID::MAtkInc)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_PAtkPer: { pattr.attrs[tosizet(AttrID::PAtkPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MAtkPer: { pattr.attrs[tosizet(AttrID::MAtkPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_PDef: { pattr.attrs[tosizet(AttrID::PDef)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MDefPer: { pattr.attrs[tosizet(AttrID::MDefPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_HpRest: { pattr.attrs[tosizet(AttrID::HpRestore)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_HpRestPer: { pattr.attrs[tosizet(AttrID::HpRestorePer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_FinMaxHp: { pattr.attrs[tosizet(AttrID::FinalMaxHp)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MaxMpPer: { pattr.attrs[tosizet(AttrID::MaxMpPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MpRest: { pattr.attrs[tosizet(AttrID::MpRestore)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MpRestPer: { pattr.attrs[tosizet(AttrID::MpRestorePer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MpCost: { pattr.attrs[tosizet(AttrID::MpCost)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MpCostPer: { pattr.attrs[tosizet(AttrID::MpCostPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MaxPP: { pattr.attrs[tosizet(AttrID::MaxPP)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MaxPpPer: { pattr.attrs[tosizet(AttrID::MaxPpPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_PpRestore: { pattr.attrs[tosizet(AttrID::PpRestore)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_PpRestorePer: { pattr.attrs[tosizet(AttrID::PpRestorePer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_AtkSpeedPha: { pattr.attrs[tosizet(AttrID::AttackSpeedPhase)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MoveSpeedPha: { pattr.attrs[tosizet(AttrID::MoveSpeedPhase)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_RelSpeedPha: { pattr.attrs[tosizet(AttrID::ReleaseSpeedPhase)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_PuncRate: { pattr.attrs[tosizet(AttrID::PunctureRate)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MPuncRate: { pattr.attrs[tosizet(AttrID::MPunctureRate)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_PuncDecRate: { pattr.attrs[tosizet(AttrID::PunctureDecRate)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MPuncDecRate: { pattr.attrs[tosizet(AttrID::MPunctureDecRate)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_RefMul: { pattr.attrs[tosizet(AttrID::ReflectMul)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_RefDecRate: { pattr.attrs[tosizet(AttrID::ReflectDecRate)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_SuckRate: { pattr.attrs[tosizet(AttrID::SuckBloodRate)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_SuceMul: { pattr.attrs[tosizet(AttrID::SuckBloodMul)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_CritRate: { pattr.attrs[tosizet(AttrID::CritRate)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_CritMul: { pattr.attrs[tosizet(AttrID::CritMul)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_CritResist: { pattr.attrs[tosizet(AttrID::CritResist)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_CritDec: { pattr.attrs[tosizet(AttrID::CritDec)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_MonDamPer: { pattr.attrs[tosizet(AttrID::MonDamagePer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_PvpPer: { pattr.attrs[tosizet(AttrID::PvpMul)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_PDropRate: { pattr.attrs[tosizet(AttrID::PDropRate)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_ExpMul: { pattr.attrs[tosizet(AttrID::ExpMul)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_BlueRate: { pattr.attrs[tosizet(AttrID::BlueEquipRate)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_GoldRate: { pattr.attrs[tosizet(AttrID::GoldEquipRate)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_WeaStrenLv: { pattr.attrs[tosizet(AttrID::WeaponStrengthenLv)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_ArmorStrenLv: { pattr.attrs[tosizet(AttrID::ArmorStrengthenLv)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_BatExpMul: { pattr.attrs[tosizet(AttrID::BattleExpMul)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_RealDamInc: { pattr.attrs[tosizet(AttrID::RealDamageInc)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_RealDamDec: { pattr.attrs[tosizet(AttrID::RealDamageDec)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_RealDamPer: { pattr.attrs[tosizet(AttrID::RealDamagePer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_RealDamDecPer: { pattr.attrs[tosizet(AttrID::RealDamageDecPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_AtkAdd: { pattr.attrs[tosizet(AttrID::AttackAdd)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_AtkDec: { pattr.attrs[tosizet(AttrID::AttackDec)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_FixAddDam: { pattr.attrs[tosizet(AttrID::FixAddDamage)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_FixDecDam: { pattr.attrs[tosizet(AttrID::FixDecDamage)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_DamAddPer: { pattr.attrs[tosizet(AttrID::DamageAddPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_DamDecPer: { pattr.attrs[tosizet(AttrID::DamageDecPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_AllAttackAdd: { pattr.attrs[tosizet(AttrID::AllAttackAdd)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_RangeAttackMul: { pattr.attrs[tosizet(AttrID::RangeAttackMul)] += m_Item.stNpProperty[i].dwNpNum; }break;
		case	NONPAREIL_TYPE_GoldAddPer: { pattr.attrs[tosizet(AttrID::GoldAddPer)] += m_Item.stNpProperty[i].dwNpNum; }break;
		}
	}
	return pattr;
}
stARpgAbility CItem::GetLuaDataTrans() const
{
	stARpgAbility dataAttr;
	auto itemBase = GetItemDataBase();
	if (!itemBase) return dataAttr;
	for (size_t i = 0; i < 4; i++)
	{
		auto itemId = m_Item.dwLuaData[i];
		if (itemId > 0)
		{
			if (auto itemData = sJsonConfig.GetItemDataById(itemId) ) {
				if (auto pEffect = itemData->GetEffectDataBase()) {
					dataAttr.attrs += pEffect->attrs;
				}
			}
		}
	}
	return dataAttr;
}

DWORD CItem::GetNextId()
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->nNextID;
	}
	return 0;
}

void CItem::SetDropProtectCount(int nCount)//获取装备投保次数
{
	FUNCTION_BEGIN;
	if (nCount < 0)
		nCount = 0;
	if (m_Item.nDropProtectCount == 0xeefeeefe || m_Item.nDropProtectCount == 0xfeeefeee)
	{
		g_logger.forceLog(zLogger::zERROR, "物品对象 %.16x 删除后继续使用。SetDropProtectCount", this);
	}
	m_Item.nDropProtectCount = nCount;
}

void CItem::SetExtraStrenghLvl(BYTE btLvl)
{
	FUNCTION_BEGIN;
	if (btLvl < 0)
		btLvl = 0;
	if (m_Item.btStrenghAdd == 0xeefeeefe || m_Item.btStrenghAdd == 0xfeeefeee)
	{
		g_logger.forceLog(zLogger::zERROR, "物品对象 %.16x 删除后继续使用。SetEquipEnhancement", this);
	}
	m_Item.btStrenghAdd = btLvl;
}

void CItem::SetBroken(BYTE btLv)
{
	FUNCTION_BEGIN;
	if (btLv < 0)
		btLv = 0;
	if (m_Item.btBroken == 0xeefeeefe || m_Item.btBroken == 0xfeeefeee)
	{
		g_logger.forceLog(zLogger::zERROR, "物品对象 %.16x 删除后继续使用。SetBroken", this);
	}
	m_Item.btBroken = btLv;
}

void CItem::SetPrefix(BYTE btLv)
{
	FUNCTION_BEGIN;
	if (btLv < 0)
		btLv = 0;
	if (m_Item.btPrefix == 0xeefeeefe || m_Item.btPrefix == 0xfeeefeee)
	{
		g_logger.forceLog(zLogger::zERROR, "物品对象 %.16x 删除后继续使用。SetPrefix", this);
	}
	m_Item.btPrefix = btLv;
}

bool CItem::CheckJobType(BYTE btJob) {
	FUNCTION_BEGIN;
	auto itemBase = GetItemDataBase();
	if (!itemBase) return false;
	if (itemBase->btJobType == 0 || (itemBase->btJobType & (1 << btJob - 1)))
	{
		return true;
	}
	return false;
}

const char* CItem::LuaGetItemName() const
{
	if (auto itemBase = GetItemDataBase())
	{
		return itemBase->szName.c_str();
	}
	return NULL;
}

const int CItem::GetItemNpPropertyByType(int nType)
{
	int num=0;
	for(int i=0;i<m_Item.btNpPropertyCount;i++){
		if(m_Item.stNpProperty[i].btNpType == nType){
			num+=m_Item.stNpProperty[i].dwNpNum;
		}
	}
	return num;
}

const char* CItem::GetItemLuaId() const
{
	return m_Item.GetItemLuaId();
}

const long long CItem::strToi642(const char* luatmpid)
{
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

bool CItem::IsCurrency() {
	FUNCTION_BEGIN;
	if (auto itemBase = GetItemDataBase())
	{
		if (stRes::IsCcy(itemBase->nID)) {
			return true;
		}
	}
	return false;
}

bool CItem::IsMechBody()
{
	auto itemBase = GetItemDataBase();
	if (!itemBase) return false;
	return itemBase->btEquipStation >= emEquipPosition::MECHANICAL_HELMET && itemBase->btEquipStation <= emEquipPosition::MECHANICAL_LEG;
}
