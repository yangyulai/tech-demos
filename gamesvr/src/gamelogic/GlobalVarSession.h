#ifndef _GlobalVarSession_H_8F9DAEFENFEFOEIROERER123LKFORR
#define _GlobalVarSession_H_8F9DAEFENFEFOEIROERER123LKFORR
#include "HashManage.h"
#include "zsingleton.h"
#include "qglobal.h"


struct stSysVars{
	char szVarName[_MAX_NAME_LEN_*2];
	char szVarValue[_MAX_DESC_LEN_];
	stSysVars(){
		ZEROSELF;
	}
};

struct stLimitHashVarName:LimitStrCaseHash< stSysVars*> {
	static __inline const char* mhkey(stSysVars*& e){
		return e->szVarName;
	}
};

class GlobalVars: public Singleton<GlobalVars>,public zLHashManager3<
	stSysVars*,
	stLimitHashVarName
>{
public:
	stSysVars* FindByName(const char* name){
		AILOCKT(*this);
		stSysVars* value=NULL;
		if (name && m_e1.find(name,value)){
			return value;
		}
		return NULL;
	}

	GlobalVars();
	~GlobalVars();

	void save(const char* szVarName,const char* szVarValue);
	const char* get(const char* szVarName);
	void del(const char* szVarName);
};

#endif