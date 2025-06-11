#include <register_lua.hpp>
#include "Script.h"
#include "BaseCreature.h"
#include "UsrEngn.h"
#include "Chat.h"
#include <winapiini.h>


bool vars_set_var_n(const CVars* _vars,const std::string& name, double value,bool needsave){
	char setvalue[_MAX_QUEST_LEN_];
	sprintf(setvalue,"%lf",value);
	return ((CVars*)_vars)->set_var_c(name,setvalue,(needsave?CVar::_NEED_SAVE_:CVar::_DONT_SAVE_) );
}

double vars_get_var_n(const CVars* _vars,const std::string& name){
	double dbret=0.0;
	BYTE lifetype=0;
	char* pgetvalue=nullptr;
	((CVars*)_vars)->get_var_c(name,pgetvalue,lifetype);
	if (pgetvalue){
		dbret=strtod(pgetvalue,NULL);
	}
	return dbret;
}

const std::string vars_get_var_str(const CVars* _vars, const std::string& name) {
	return ((CVars*)_vars)->get_var_cstr(name);
}
bool vars_set_var_str(const CVars* _vars, const std::string& name, const char* value, bool needsave, int len)
{
	if (value || len > 0) {
		char setvalue1[_MAX_QUEST_LEN_];
		sprintf(setvalue1, "%lf", (double)time(NULL));
		((CVars*)_vars)->set_var_c("var_save_time", setvalue1, CVar::_NEED_SAVE_);
		return ((CVars*)_vars)->set_var_c(name, value, (needsave ? CVar::_NEED_SAVE_ : CVar::_DONT_SAVE_), len);

	}
	return false;
}

bool vars_set_var_s(const CVars* _vars,const std::string& name, const char* value,bool needsave){
	if (value) {
		return ((CVars*)_vars)->set_var_c(name,value,(needsave?CVar::_NEED_SAVE_:CVar::_DONT_SAVE_) );
	}
	return false;
}

const char* vars_get_var_s(const CVars* _vars,const std::string& name){//?????ÓÐÎÊÌâ
	//static std::string tmpstr;
	BYTE lifetype=0;
	char* pgetvalue=nullptr;
	((CVars*)_vars)->get_var_c(name,pgetvalue,lifetype);
	//return tmpstr.c_str();
	return pgetvalue;
}

bool CQuestInfo_reg_evt(const CQuestInfo* pobj,WORD evt,DWORD evtex,const char* szeventfunc){
	toremd_1_3(evtid,evt,evtex);
	((CQuestInfo*)pobj)->seteventid(evt,evtex);
	return ((CQuestInfo*)pobj)->reg_evt(evtid._value,szeventfunc);
}

bool CQuestInfo_unreg_evt(const CQuestInfo* pobj,WORD evt,DWORD evtex){
	toremd_1_3(evtid,evt,evtex);
	((CQuestInfo*)pobj)->seteventid(0,0);
	return ((CQuestInfo*)pobj)->unreg_evt(evtid._value);
}

bool CQuestInfo_mapreg_evt(const CQuestInfo* pobj,WORD evtex,WORD mapx,WORD mapy,BYTE range,const char* szeventfunc){
	toremd_1_2_3_4(evtid,MAPAREA,evtex,mapx,mapy);
	((CQuestInfo*)pobj)->seteventid(MAPAREA,evtex);
	return ((CQuestInfo*)pobj)->reg_evt(evtid._value,szeventfunc,range,mapx,mapy);
}

bool CQuestInfo_mapunreg_evt(const CQuestInfo* pobj,WORD evtex,WORD mapx,WORD mapy){
	toremd_1_2_3_4(evtid,MAPAREA,evtex,mapx,mapy);
	((CQuestInfo*)pobj)->seteventid(0,0);
	return ((CQuestInfo*)pobj)->unreg_evt(evtid._value);
}

void CScriptSystem::BindQuest(CLuaVM *luavm){
	sol::state_view lua(luavm->lua());
	lua.new_usertype<CVars>("Vars",
		"has", &CVars::find_var,
		"remove", &CVars::remove_var,
		"clear", &CVars::clear,
		"setn", &vars_set_var_n,
		"getn", &vars_get_var_n,
		"sets", &vars_set_var_s,
		"gets", &vars_get_var_s,
		"setcs", &vars_set_var_str,
		"getcs", &vars_get_var_str
	);
	lua.new_usertype<CQuestInfo>("QuestInfo",

		"id", sol::readonly(&CQuestInfo::_id),
		"type", sol::readonly(&CQuestInfo::_quest_type),
		"evtid", sol::readonly(&CQuestInfo::_evtid),
		"evtnum", sol::readonly(&CQuestInfo::_evtnum),
		"addg", & CQuestInfo::add_questinfo,
		"disable", & CQuestInfo::disable_questinfo,
		"disableall", & CQuestInfo::disable_all_questinfo,
		"reg_evt", CQuestInfo_reg_evt,
		"unreg_evt", CQuestInfo_unreg_evt,
		"mapcellreg_evt", CQuestInfo_mapreg_evt,
		"mapcellunreg_evt", CQuestInfo_mapunreg_evt,
		"clear_evt", &CQuestInfo::clear_evt
	);
	lua.new_usertype<CQuest>("Quest",

		"id", sol::readonly_property(&CQuest::getQuestId),
		"name", sol::readonly_property(&CQuest::getQuestName),
		"info", sol::readonly(&CQuest::m_info),
		"isupdate", sol::readonly(&CQuest::_update),
		"vars", sol::readonly(&CQuest::_vars),
		"evtvar", sol::readonly(&CQuest::m_evtvar)
	);
	registerClass<CQuestList>(lua, "QuestList",
		"add", &CQuestList::add_quest,
		"find", &CQuestList::find_quest,
		"remove", &CQuestList::remove_quest,
		"clear", &CQuestList::clear,
		"reg_evt", &CQuestList::regevt
		);
	registerClass<stQuestCompleteId>(lua, "stQuestCompleteId",
		"qid", &stQuestCompleteId::dwCompleteId,
		"status", &stQuestCompleteId::status
	);
}