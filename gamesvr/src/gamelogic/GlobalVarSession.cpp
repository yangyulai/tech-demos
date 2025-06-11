#include "GlobalVarSession.h"

GlobalVars::GlobalVars(){
	FUNCTION_BEGIN;
}

GlobalVars::~GlobalVars(){
	FUNCTION_BEGIN;
	GlobalVars::iterator it,itr;
	for(it=this->begin(),itr=it;itr!=this->end();it=itr){
		itr++;
		stSysVars* pVar=it->second;
		if(pVar){
			SAFE_DELETE(pVar);
		}
	}
}


void GlobalVars::save(const char* szVarName,const char* szVarValue){
	FUNCTION_BEGIN;
	stSysVars *pvar=this->FindByName(szVarName);
	if(pvar){
		strcpy_s(pvar->szVarValue,sizeof(pvar->szVarValue)-1,szVarValue);
	}else{
		pvar=CLD_DEBUG_NEW stSysVars;
		strcpy_s(pvar->szVarName,sizeof(pvar->szVarName)-1,szVarName);
		strcpy_s(pvar->szVarValue,sizeof(pvar->szVarValue)-1,szVarValue);
		this->addValue(pvar);
	}
}

const char* GlobalVars::get(const char* szVarName){
	FUNCTION_BEGIN;
	stSysVars *pvar=this->FindByName(szVarName);
	if(pvar){
		return pvar->szVarValue;
	}
	return "";
}

void GlobalVars::del(const char* szVarName){
	FUNCTION_BEGIN;
	stSysVars *pvar=this->FindByName(szVarName);
	if(pvar){
		this->removeValue(pvar);
		SAFE_DELETE(pvar);
	}
}