#include "ShortCuts.h"
#include "BaseCreature.h"

void CShortCuts::Init(BYTE btIdx){
	m_btIdx=btIdx;
	m_boUseing=false;
}

bool CShortCuts::AddShortCuts(__int64 i64Id,emShortCutsType emShortCuts,BYTE btShortCuts,BYTE btRow,BYTE btCol){
	stShortCuts* pShortCuts=m_cShortCutsHashMap.FindByid(MAKE_SHORTCUTS_ID(btRow,btCol));
	if (pShortCuts && pShortCuts->i64Id!=i64Id){
		if (DeleteShortCuts(btRow,btCol)){
			pShortCuts=NULL;
		}
	}
	bool boNew=false;
	if (!pShortCuts){
		pShortCuts=CLD_DEBUG_NEW stShortCuts;
		boNew=true;
	}
	if (pShortCuts){
		pShortCuts->i64Id=i64Id;
		pShortCuts->emShortCuts=emShortCuts;
		pShortCuts->btShortCuts=btShortCuts;
		pShortCuts->btRow=btRow;
		pShortCuts->btCol=btCol;
		if (boNew){
			if (!m_cShortCutsHashMap.addValue(pShortCuts)){
				g_logger.error("快捷键设置有重复 键值 %d Row %d Col %d",btShortCuts,btRow,btCol);
				SAFE_DELETE(pShortCuts);
				return false;
			}
		}
	}else return false;
	return true;
}

stShortCuts* CShortCuts::FindShortCuts(BYTE row,BYTE col){
	return m_cShortCutsHashMap.FindByid(MAKE_SHORTCUTS_ID(row,col));
}

stShortCuts* CShortCuts::FindShortCuts(__int64 i64ID){
	return m_cShortCutsHashMap.FindByI64Id(i64ID);
}

bool CShortCuts::DeleteShortCuts(BYTE row,BYTE col){
	stShortCuts* pShortCuts=m_cShortCutsHashMap.FindByid(MAKE_SHORTCUTS_ID(row,col));
	if (pShortCuts){
		m_cShortCutsHashMap.removeValue(pShortCuts);
		SAFE_DELETE(pShortCuts);
		return true;
	}
	return false;
}

void CShortCuts::Clear() {
	for (auto& pair : m_cShortCutsHashMap) {
		SAFE_DELETE(pair.second);
	}
	m_cShortCutsHashMap.clear();
}

//////////////////////////////////////////////////////////////////////////

void CShortCutsManager::Init(CCreature* Owner,BYTE btCurIdx){
	for (int i=0;i!=MAX_SHORTCUTSPLAN;i++)
	{
		m_AllShortCuts[i].Init(i);
	}
	if (btCurIdx>=0 && btCurIdx<MAX_SHORTCUTSPLAN){
		m_btCurIdx=btCurIdx;
	}else {m_btCurIdx=0;}
	m_AllShortCuts[m_btCurIdx].m_boUseing=true;
	m_Owner=Owner;
}

bool CShortCutsManager::AddShortCuts(stShortCuts* pShortCuts){
	if (pShortCuts){
		return AddShortCuts(pShortCuts->i64Id,pShortCuts->emShortCuts,pShortCuts->btShortCuts,pShortCuts->btRow,pShortCuts->btCol);
	}
	return false;
}

bool CShortCutsManager::AddShortCuts(__int64 i64Id,emShortCutsType emShortCuts,BYTE btShortCuts,BYTE btRow,BYTE btCol){
	return m_AllShortCuts[m_btCurIdx].AddShortCuts(i64Id,emShortCuts,btShortCuts,btRow,btCol);
}

stShortCuts* CShortCutsManager::FindShortCuts(BYTE row,BYTE col){
	return m_AllShortCuts[m_btCurIdx].FindShortCuts(row,col);
}

stShortCuts* CShortCutsManager::FindShortCuts(__int64 i64ID){
	return m_AllShortCuts[m_btCurIdx].FindShortCuts(i64ID);
}

bool CShortCutsManager::DeleteShortCuts(BYTE row,BYTE col){
	return m_AllShortCuts[m_btCurIdx].DeleteShortCuts(row,col);
}

bool CShortCutsManager::SetCurShortCuts(BYTE btCurIdx){
	if (btCurIdx>=0 && btCurIdx<MAX_SHORTCUTSPLAN){
		m_AllShortCuts[m_btCurIdx].m_boUseing=false;
		m_AllShortCuts[btCurIdx].m_boUseing=true;
		m_btCurIdx=btCurIdx;
		return true;
	}
	return false;
}

void CShortCutsManager::SendShortCuts(){
	stSetShortCuts retcmd;
			
	for (int idx=0;idx!=MAX_SHORTCUTSPLAN;idx++)
	{
		int size = m_AllShortCuts[idx].m_cShortCutsHashMap.size();
		int count = 0;
		for (CShortCutsHashManager::iterator it=m_AllShortCuts[idx].m_cShortCutsHashMap.begin(); it!=m_AllShortCuts[idx].m_cShortCutsHashMap.end(); it++) 
		{
			stShortCuts* pShortCuts=it->second;
			if (pShortCuts)
			{
				count++;
				retcmd.shortcuts=*pShortCuts;
				retcmd.ErrorCode=SETSHORTCUTS_SUCCESS;
				if(count == size)
				{
					retcmd.ErrorCode=SETSHORTCUTS_SENT2Client;
				}
				retcmd.oldrow=255;
				retcmd.oldcol=255;
				m_Owner->SendMsgToMe(&retcmd,sizeof(stSetShortCuts));
			}
		}
	}
}

bool CShortCutsManager::Save(char* dest,DWORD& retlen){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	retlen=0;
	if ( maxsize< (sizeof(int)) ){ return false;}

	int count=0;
	int len = sizeof(count);

	for (int idx=0;idx!=MAX_SHORTCUTSPLAN;idx++)
	{
		for (CShortCutsHashManager::iterator it=m_AllShortCuts[idx].m_cShortCutsHashMap.begin(); it!=m_AllShortCuts[idx].m_cShortCutsHashMap.end(); it++) 
		{
			stShortCuts* pShortCuts=it->second;
			if (pShortCuts)
			{
				memcpy((void *)(dest+len), pShortCuts,sizeof(stShortCuts));
				len += sizeof(stShortCuts);
				count++;
			}
		}
	}
	*((int*)(dest))=count;
	retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen);
	base64_encode(dest,len,pin,retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CShortCutsManager::Load(const char *dest, int retlen,int nver){
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

	if (maxsize< (sizeof(int))){ return false;}

	int count= *((int*)(dest));
	int len=sizeof(count);

	while (count>0){
		DWORD datalen=safe_max(maxsize-len,0);
		if (datalen>=sizeof(stShortCuts)*count){
			stShortCuts shortcuts;
			memcpy(&shortcuts,(void *)(dest+len),sizeof(stShortCuts));
			len += sizeof(stShortCuts);
			if (shortcuts.btPlan>=0 && shortcuts.btPlan<MAX_SHORTCUTSPLAN){
				m_AllShortCuts[shortcuts.btPlan].AddShortCuts(shortcuts.i64Id,shortcuts.emShortCuts,shortcuts.btShortCuts,shortcuts.btRow,shortcuts.btCol);
			}
		}
		else if (datalen!=0){
			return false;
		}
		count--;
	}
	return true;
}