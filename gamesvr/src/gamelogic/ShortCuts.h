#pragma once

#include "LocalDB.h"
#include "cmd\magic_cmd.h"

#define MAX_SHORTCUTSPLAN		1		//最大快捷键方案数
#define MAX_ROW					10		//行 用的客户端的列 相反
#define MAX_COL					254		//列 用的客户端的行 相反
#define MAKE_SHORTCUTS_ID(row,col)	( (((WORD)(row))<<8) | (((WORD)(col))) )

struct stLimitHashShortCutsId:LimitHash< WORD,stShortCuts*> {
	static __inline const WORD mhkey(stShortCuts*& e){
		return MAKE_SHORTCUTS_ID(e->btRow,e->btCol);
	}
};

struct stMultiHashShortCutsI64Id:MultiHash< __int64,stShortCuts*> {
	static __inline const __int64 mhkey(stShortCuts*& e){
		return e->i64Id;
	}
};

class CShortCutsHashManager : public zLHashManager3<
	stShortCuts*,
	stLimitHashShortCutsId,
	stMultiHashShortCutsI64Id
>{
public:
	stShortCuts* FindByid(WORD wID){
		AILOCKT(*this);
		stShortCuts* value=NULL;
		if (m_e1.find(wID,value)){
			return value;
		}
		return NULL;
	}
	stShortCuts* FindByI64Id(__int64 i64ID){
		AILOCKT(*this);
		stShortCuts* value=NULL;
		if (m_e2.find(i64ID,value)){
			return value;
		}
		return NULL;
	}
};

class CShortCuts
{
public:
	BYTE	m_btIdx;		//第几套方案
	bool	m_boUseing;		//是否使用中

	CShortCuts(){
		m_btIdx=(BYTE)-1;
		m_boUseing=false;
	}
	~CShortCuts(){
		Clear();
	}
	void	Init(BYTE btIdx);
	bool	AddShortCuts(__int64 i64Id,emShortCutsType emShortCuts,BYTE btShortCuts,BYTE btRow,BYTE btCol);
	stShortCuts* FindShortCuts(BYTE row,BYTE col);
	stShortCuts* FindShortCuts(__int64 i64ID);	//可重复的
	bool	DeleteShortCuts(BYTE row,BYTE col);
	void	Clear();
public:
	CShortCutsHashManager m_cShortCutsHashMap;
};

class CCreature;

class CShortCutsManager
{
public:
	CShortCuts m_AllShortCuts[MAX_SHORTCUTSPLAN];		//快捷键方案
	BYTE m_btCurIdx;									//当前使用的方案
	CCreature* m_Owner;
public:
	CShortCutsManager(){
		m_btCurIdx=(BYTE)-1;
		m_Owner=NULL;
	}
	void	Init(CCreature* Owner,BYTE btCurIdx=0);
	bool	AddShortCuts(stShortCuts* pShortCuts);
	bool	AddShortCuts(__int64 i64Id,emShortCutsType emShortCuts,BYTE btShortCuts,BYTE btRow,BYTE btCol);
	stShortCuts* FindShortCuts(BYTE row,BYTE col);
	stShortCuts* FindShortCuts(__int64 i64ID);	//可重复的
	bool	DeleteShortCuts(BYTE row,BYTE col);
	bool	SetCurShortCuts(BYTE btCurIdx);

	void	SendShortCuts();

	bool	Save(char* dest,DWORD& retlen);
	bool	Load(const char* dest,int retlen,int nver);
};