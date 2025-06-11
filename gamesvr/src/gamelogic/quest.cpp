#include "quest.h"
#include "UsrEngn.h"
#include "PlayerObj.h"
#include "Npc.h"
#include "Script.h"
#include "../gamesvr.h"
#include "Chat.h"
#include "Config.h"

void stQuestDB::init(){
	setConditions(szquestconditions);
	setItemReward(this,szquestreward,VQuestreward);
}

void stQuestDB::setConditions(char* pszconditions){
	FUNCTION_BEGIN;
	zXMLParser xml;
	if (!xml.initStr(pszconditions))
	{
		g_logger.error("解析任务达成条件字符串失败 %I64d [%s]",i64qid,pszconditions);
		return;
	}

	xmlNodePtr root = xml.getRootNode("j");
	if (root)
	{
		DWORD nodenum=0;
		xmlNodePtr node=xml.getChildNode(root,NULL);
		int n=0;
		while (node)
		{
			if (strcmp(node->Value(),"i")==0)
			{
				stQuestConditions qc;
				xml.getNodePropNum(node,"id",qc.nId);
				xml.getNodePropNum(node,"co",qc.nCount);
				xml.getNodePropNum(node,"ip",qc.nItemPos);
				xml.getNodePropNum(node,"lv",qc.nItemLvl);
				xml.getNodePropNum(node,"de",qc.nDelete);
				xml.getNodePropNum(node,"ce",qc.nDeleteEquip);
				if(qc.nCount ==0) qc.nCount=1;
				vconditions.push_back(qc);
				if(vconditions.size()>= QUEST_REWARD_COUNT){
					return;
				}
			}
			node=xml.getNextNode(node,NULL);
		}
	}
}

void stQuestDB::setItemReward(stQuestDB* pQuest,char* pszreward, std::vector<stItem> &vi){
	FUNCTION_BEGIN;
	zXMLParser xml;
	if (!xml.initStr(pszreward))
	{
		if(pQuest){
			g_logger.error("解析任务奖励字符串失败 %s,任务编号为%d",pszreward,pQuest->i64qid);
		}else{
			g_logger.error("解析任务奖励字符串失败 %s",pszreward);
		}
	}
}

char* stQuestDB::setEventLuaFunName(int nEventType){
	

	return NULL;
}

CVars::CVars(){
	FUNCTION_BEGIN;
	needThreadSafe = false;
}

CVars::~CVars(){
	FUNCTION_BEGIN;
	clear();
 
}

bool CVars::_save(char* dest,DWORD& retlen ){
	int maxsize=retlen;
	retlen=0;
	if (maxsize< sizeof(int)){ return false; }

	int count=0;
	int len=sizeof(count);

	for (const_var_iterator it=_vars.begin(); it!=_vars.end(); ++it) {
		const CVar* pvar=&it->second;
		if (pvar->_life_type!=CVar::_DONT_SAVE_){

			int tmp = it->first.length();
			if(maxsize<(len+tmp+(int)sizeof(int)+1)) { return false; }
			memcpy((void *)(dest+len), &tmp, sizeof(int));
			len += sizeof(int);
			memcpy((void *)(dest+len), it->first.c_str(),tmp);
			len += tmp;

			*((BYTE*)(&dest[len]))=pvar->_life_type;
			len++;

			tmp = pvar->_value.length();
			if(maxsize<(len+tmp+(int)sizeof(int))) { return false; }
			memcpy((void *)(dest+len), &tmp, sizeof(int));
			len += sizeof(int);
			memcpy((void *)(dest+len), pvar->_value.c_str(), tmp);
			len += tmp;

			count++;
		}
	}
	*((int*)dest)=count;
	retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen + 1);
	base64_encode(dest,len,pin,retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CVars::save(char* dest,DWORD& retlen){
	FUNCTION_BEGIN;
	if(needThreadSafe){
		do 
		{
			AILOCKT(omplock);
			return _save(dest,retlen);

		}while(false);
	}else{
		return _save(dest,retlen);
	}
	

	
}

bool CVars::_load(const char* dest,int retlen,int nver){
	FUNCTION_BEGIN;
	int maxsize=retlen;

	retlen=ROUNDNUMALL(retlen,4)/4*3;
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen+1);
	base64_decode((char*)dest,maxsize,pin,retlen);
	memcpy((char*)dest,pin,retlen);
	maxsize=retlen;

	if (maxsize< sizeof(int)){ return false; }

	int count= *((int*)(dest));
	int len=sizeof(count);

	CVar tmpvar;
	while (count>0){
		count--;

		if(maxsize<(len+(int)sizeof(int))) { return false; }
		int tmplen = *((int*)(dest + len));
		if(maxsize<(len+tmplen+(int)sizeof(int)+1)) { return false; }
		len += sizeof(int);
		std::string name((char *)dest+len, tmplen);
		len += tmplen;

		tmpvar._life_type=BYTE(dest[len]);
		len++;

		tmplen = *((int*)(dest + len));
		if(maxsize<(len+tmplen+(int)sizeof(int))) { return false; }
		len += sizeof(int);
		std::string value((char *)dest+len, tmplen);
		len += tmplen;

		if (tmpvar._life_type!=CVar::_DONT_SAVE_){
			tmpvar._value=value;
			_vars[name] = tmpvar;
		}
	}
	return true;
}


bool CVars::load(const char* dest,int retlen,int nver){

	if(needThreadSafe){
		do 
		{
			AILOCKT(omplock);
			return _load(dest,retlen,nver);
		}while(false);
	}else{
		return _load(dest,retlen,nver);
	}
}
void CVars::_show(std::string& showstr){

	std::stringstream os;
	for (const_var_iterator it=_vars.begin(); it!=_vars.end(); ++it) {
		const CVar* pvar=&it->second;
		os << it->first << " (" << pvar->_life_type << ") = " << pvar->_value << " ;\n";
	}
	showstr += os.str();
}


void CVars::show(std::string& showstr){
	FUNCTION_BEGIN;
	if(needThreadSafe){
		do 
		{
			AILOCKT(omplock);
			return _show(showstr);
		}while(false);
	}else{
		return _show(showstr);
	}
}


#include "lookaside_alloc.h"
CQuestInfo::QUESTINFOMAP CQuestInfo::m_infos;
_STACK_AUTOEXEC_( stAutoClearQuestInfo,__noop,CQuestInfo::clear_questinfo() );
stAutoClearQuestInfo autoclearquestinfo;

CQuestInfo* CQuestInfo::add_questinfo(DWORD qid,DWORD qtype,const char* name,const char* dis,BYTE timetype,DWORD dwtime,BYTE runtype,DWORD interval){
	FUNCTION_BEGIN;
	if (qid==CQuestInfo::INVALID_QUESTID){ return NULL; }
	CQuestInfo::qioit it=CQuestInfo::m_infos.find(qid);
	CQuestInfo* pinfo=NULL;
	bool bocreat=false;
	if (it!=CQuestInfo::m_infos.end()){
		pinfo=it->second;
	}else{
		pinfo=CLD_DEBUG_NEW CQuestInfo;
		if (!pinfo){ return NULL; }
		bocreat=true;
	}
	pinfo->_quest_type=qtype;
	pinfo->_id=qid;
	pinfo->_name=name;
	pinfo->_dis=dis;
	pinfo->_time_type=timetype;
	pinfo->_time_param=dwtime;
	pinfo->_run_type=runtype;
	pinfo->_interval=interval;
	pinfo->_isdelete=false;
	if (bocreat){ CQuestInfo::m_infos.insert(CQuestInfo::QUESTINFOMAP::value_type(qid,pinfo)); }
	return pinfo;
}
CQuestInfo* CQuestInfo::find_questinfo(DWORD qid){
	FUNCTION_BEGIN;
	CQuestInfo::qioit it=CQuestInfo::m_infos.find(qid);
	if (it!=CQuestInfo::m_infos.end()){ 
		return it->second;
	}
	return NULL;
}
bool CQuestInfo::disable_questinfo(DWORD qid){
	FUNCTION_BEGIN;
	CQuestInfo::qioit it=CQuestInfo::m_infos.find(qid);
	if (it!=CQuestInfo::m_infos.end()){ 
		CQuestInfo* pinfo=it->second;
		if (pinfo){ pinfo->_isdelete=true;	}
	}
	return true;
}

bool CQuestInfo::disable_all_questinfo(){
	FUNCTION_BEGIN;
	for(CQuestInfo::qioit it=CQuestInfo::m_infos.begin();it!=CQuestInfo::m_infos.end();it++){ 
		CQuestInfo* pinfo=it->second;
		if (pinfo){ pinfo->_isdelete=true;	}
	}
	return true;
}

bool CQuestInfo::clear_questinfo(){
	FUNCTION_BEGIN;
	for(CQuestInfo::qioit it=CQuestInfo::m_infos.begin();it!=CQuestInfo::m_infos.end();it++){ 
		CQuestInfo* pinfo=it->second;
		it->second=NULL;
		SAFE_DELETE(pinfo);
	}
	CQuestInfo::m_infos.clear();
	return true;
}

void CQuestInfo::seteventid(WORD evtid,DWORD evtnum){
	FUNCTION_BEGIN;
	_evtid=evtid;
	_evtnum=evtnum;
}

bool CQuestInfo::reg_Questevt(CQuest* pQuest,const char* szeventfunc,BYTE range,WORD mapx,WORD mapy){
	FUNCTION_BEGIN;
	if ( szeventfunc[0]==0 ){ return false;	}
	toremd_1_3(evtid,pQuest->m_info->_questdb->btquesttargettype,pQuest->m_info->_questdb->vconditions[0].nId);
	seteventid(pQuest->m_info->_questdb->btquesttargettype,pQuest->m_info->_questdb->vconditions[0].nId);
	//	if(_events.getCountByKey(evtid._value) >0) return false;
	stQuestEvent* tmp=NULL;
	tmp=CLD_DEBUG_NEW stQuestEvent;
	if (!tmp){return false;}
	if (strlen(szeventfunc)>=sizeof(tmp->szeventfunc)){SAFE_DELETE(tmp); return false;}
	ZeroMemory(tmp,sizeof(stQuestEvent));
	tmp->questid=pQuest->m_info->_id;
	tmp->quest = pQuest;
	strcpy_s(tmp->szeventfunc,sizeof(tmp->szeventfunc)-1,szeventfunc);
	tmp->mapx=mapx;
	tmp->mapy=mapy;
	tmp->maprange=range;
	_events.insert(evtid._value,tmp);
	return true;
}
bool CQuestInfo::reg_evt(stEventMapID event,const char* szeventfunc,BYTE range,WORD mapx,WORD mapy){
	FUNCTION_BEGIN;
	if ( szeventfunc[0]==0 ){ return false;	}
	//qeit it=_events.find(event);
	//if (it!=_events.end()){ return false; }
	//if(_events.getCountByKey(event) >0) return false;
	stQuestEvent* tmp=NULL;
	tmp=CLD_DEBUG_NEW stQuestEvent;
	if (!tmp){return false;}
	if (strlen(szeventfunc)>=sizeof(tmp->szeventfunc)){SAFE_DELETE(tmp); return false;}
	ZeroMemory(tmp,sizeof(stQuestEvent));
	tmp->questid=_id;
	strcpy_s(tmp->szeventfunc,sizeof(tmp->szeventfunc)-1,szeventfunc);
	tmp->mapx=mapx;
	tmp->mapy=mapy;
	tmp->maprange=range;
	_events.insert(event,tmp);
	return true;
}
bool CQuestInfo::unreg_evt(stEventMapID event){
	FUNCTION_BEGIN;
	/*
	qeit it=_events.find(event);
	if (it==_events.end()){return false;}
	stQuestEvent* tmp=it->second;
	it->second=NULL;
	SAFE_DELETE(tmp);
	_events.erase(event);
	return true;
	*/
	stQuestEvent* tmp = NULL;
	_events.find(event,tmp);
	if(tmp){
		_events.remove(event,tmp);
		SAFE_DELETE(tmp);
	}
	return true;
}

bool CQuestInfo::clear_evt(bool boByLua){
	FUNCTION_BEGIN;
	if(!boByLua){
		if (_events.size()!=0)
		{
			for (qeit it=_events.begin();it!=_events.end();it++)
			{
				stQuestEvent* tmp=it->second;
				it->second=NULL;
				SAFE_DELETE(tmp);
			}
			_events.clear();
		}
	}else{
		if (_events.size()!=0)
		{
			qeit it,itnext;
			for (it=_events.begin(),itnext = it;it!=_events.end();it=itnext)
			{
				itnext++;
				stQuestEvent* tmp=it->second;
				if(tmp->quest == NULL){
					it->second=NULL;
					SAFE_DELETE(tmp);
					_events.erase(it);
				}

			}
		}
	}

	return true;
}

const char* CQuestInfo::find_evt(WORD evtid,DWORD evtnum){
	FUNCTION_BEGIN;
	toremd_1_3(evt,evtid,evtnum);
	stQuestEvent* tmpevt=NULL;
	_events.find(evt._value,tmpevt);
	if(tmpevt){
		return &(tmpevt->szeventfunc[0]);
	}
	return NULL;
	/*
	qeit it=_events.find(evt._value);
	if (it!=_events.end())
	{
	stQuestEvent* tmpevt=NULL;
	tmpevt=it->second;
	if (tmpevt){
	return &(tmpevt->szeventfunc[0]);
	}
	}
	return NULL;
	*/
}

bool CQuestInfo::timebool(){
	FUNCTION_BEGIN;
	if (_questdb)
	{
		return (_questdb->ntimecheck!=0);
	}
	return false;
}


CQuestInfo::~CQuestInfo(){
	clear_evt(false);
}

CQuest::CQuest(CQuestInfo* info)
	:m_info(info),m_boTerminate(false){
		FUNCTION_BEGIN;
		_update=true;
		m_timer=NULL;
		m_nStartTime = 0;
}

void CQuest::QuestTerminate(){
	FUNCTION_BEGIN;
	m_boTerminate=true;
}

bool CQuest::save(char* dest,int& retlen){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	retlen=0;
	if (maxsize< (sizeof(DWORD)+sizeof(_timeout)+sizeof(_timerun)+sizeof(stEvtVar)) ){ return false;}

	*((DWORD*)(dest))=m_info->_id;
	int len= sizeof(DWORD);

	memcpy((void *)(dest+len), &_timeout,sizeof(_timeout));
	len += sizeof(_timeout);

	memcpy((void *)(dest+len), &_timerun,sizeof(_timerun));
	len += sizeof(_timerun);

	if (m_timer && ((CQuestInfo*)m_info)->timebool()){
		DWORD miaoshu=((CScriptTimer*)m_timer)->GetTime();
		m_evtvar.timercheck=m_evtvar.timercheck-miaoshu;
		if (m_evtvar.timercheck<0){m_evtvar.timercheck=0;}
		// 		int timecheck=0;
		// 		_vars.get_var("timecheck",timecheck);
		// 		timecheck=timecheck-(int)miaoshu;
		// 		if (timecheck<0){timecheck=0;}
		// 		_vars.set_var("timecheck",timecheck);
	}
	memcpy((void *)(dest+len), &m_evtvar,sizeof(stEvtVar));
	len += sizeof(stEvtVar);

	*((int*)(dest+len))=0;
	len+=sizeof(int);
	DWORD datalen=maxsize-len;
	if (_vars.save(dest+len,datalen)){
		*((int*)(dest+len-sizeof(int)))=datalen;
		len += datalen;

		retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
		_GET_TH_LOOPCHARBUF(retlen + 1, true);
		char* pin= ptlsbuf;
		ZeroMemory(pin,retlen + 1);
		base64_encode(dest,len,pin,retlen);
		memcpy(dest, pin, retlen);
		return true;
	}else{
		return false;
	}
}

bool CQuest::load(const char* dest,int retlen,int nver){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	if(maxsize==0)return true;

	retlen=ROUNDNUMALL(retlen,4)/4*3;
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen + 1);
	base64_decode((char*)dest,maxsize,pin,retlen);
	memcpy((char*)dest,pin,retlen);
	maxsize=retlen;

	if (maxsize< (sizeof(DWORD)+sizeof(_timeout)+sizeof(_timerun)+sizeof(stEvtVar)) ){ return false;}

	DWORD qid= *((DWORD*)(dest));
	int len=sizeof(qid);

	if (qid==CQuestInfo::INVALID_QUESTID){
		m_info=NULL;
	}else{
		m_info=CQuestInfo::find_questinfo(qid);
	}

	if (m_info && qid==m_info->_id){
		memcpy(&_timeout,(void *)(dest+len),sizeof(_timeout));
		len += sizeof(_timeout);
		memcpy(&_timerun,(void *)(dest+len),sizeof(_timerun));
		len += sizeof(_timerun);
		memcpy(&m_evtvar,(void *)(dest+len),sizeof(stEvtVar));
		len += sizeof(stEvtVar);	
		int qlen = *((int*)(dest+len));
		len+=sizeof(int);
		if(qlen>(maxsize-len))return false;
		_GET_TH_LOOPCHARBUF(qlen+1, true);
		ZeroMemory(ptlsbuf, qlen+1);
		char* pszVar = ptlsbuf;
		memcpy(pszVar,dest+len,qlen);
		len += qlen;
		if (_vars.load(pszVar,qlen,nver)){
			_update=true;
			return true;
		}else{
			return false;
		}
	}else{
	}

	return false;
}

void CQuest::show(std::string& showstr){
	FUNCTION_BEGIN;
	std::stringstream os;
	os << "任务: " << m_info->_id << "(" << m_info->_time_type << "," << _timeout._start_time << "," << _timeout._keep_time << ")\n";
	os << "(" << m_info->_run_type << "," << _timerun._last_time << "," << _timerun._time_interval << ")\n";
	std::string tmpstr;
	_vars.show(tmpstr);
	showstr += os.str();
	showstr += tmpstr;
}

bool CQuest::checktime(time_t cktime){
	FUNCTION_BEGIN;
	if (cktime>=_timeout._start_time)
	{
		return true;
	}
	return false;
}

void CQuest::settime(time_t start,time_t keep,time_t last,time_t interval){
	FUNCTION_BEGIN;
	_timeout._start_time = start;
	_timeout._keep_time = keep;
	_timerun._last_time = last;
	_timerun._time_interval = interval;
}

void CQuest::savetimer(void* timer){
	FUNCTION_BEGIN;
	m_timer=timer;
}


CQuestList::~CQuestList(){
	FUNCTION_BEGIN;
	clear();
}

CQuest* CQuestList::add_quest(DWORD qid){
	FUNCTION_BEGIN;
	if ( qid==CQuestInfo::INVALID_QUESTID ){ return NULL;	}
	CQuestInfo* pinfo=CQuestInfo::find_questinfo(qid);
	if (!pinfo){ return NULL; }
	quest_iterator it=_quests.find(qid);
	if ( it!=_quests.end() || _quests.size()>=MAX_NUM ){ return NULL; }
	CQuest* quest=CLD_DEBUG_NEW CQuest(pinfo);
	if (!quest){ return NULL; }
	_quests[qid]=quest;
	add_quest_events(quest);
	return quest;
}

bool CQuestList::add_loaded_quest(CQuest* quest){
	FUNCTION_BEGIN;
	if ( quest==NULL || quest->m_info==NULL || quest->m_info->_id==CQuestInfo::INVALID_QUESTID ){ return false;	}
	quest_iterator it=_quests.find(quest->m_info->_id);
	if ( it!=_quests.end() || _quests.size()>=MAX_NUM ){ return false; }
	_quests[quest->m_info->_id]=quest;
	if (!add_quest_events(quest)){
		_quests.erase(quest->m_info->_id);
		return false;
	}
	return true;
}

bool CQuestList::add_quest_events(CQuest* quest){
	FUNCTION_BEGIN;

	CQuestInfo* pinfo=(CQuestInfo*)quest->m_info;
	if (pinfo->_events.size()>0){
		for(qeit it=pinfo->_events.begin();it!=pinfo->_events.end();it++){
			if (pinfo->_evtid==MAPAREA || pinfo->_evtid==MAPCELL){
				reg_evt(quest,it->first,it->second->szeventfunc,it->second->maprange,it->second->mapx,it->second->mapy);
			}
			else {reg_evt(quest,it->first,it->second->szeventfunc);}		
		}
	}
	return true;
}

void CQuestList::remove_quest(CQuest* quest){
	FUNCTION_BEGIN;
	if (quest){
		unreg_evt(quest);
		_quests.erase(quest->m_info->_id);
		SAFE_DELETE(quest);
	}
}
CQuest* CQuestList::find_quest(DWORD qid){
	FUNCTION_BEGIN;
	if(_quests.size()==0) {return NULL;}
	quest_iterator it=_quests.find(qid);
	if (it==_quests.end()){ return NULL; }
	return it->second;
}

void CQuestList::init(CPlayerObj* pPlayer){

}
void CQuestList::clear(){
	FUNCTION_BEGIN;
	CQuest* quest=NULL;
	for (qeit it=_events.begin();it!=_events.end();it++)
	{
		stQuestEvent* tmp=it->second;
		it->second=NULL;
		SAFE_DELETE(tmp);
	}
	_events.clear();
	for (quest_iterator it=_quests.begin(); it!=_quests.end(); ++it){
		quest=it->second;
		it->second=NULL;
		SAFE_DELETE(quest);
	}
	_quests.clear();
}

void CQuestList::show(std::string& showstr){
	FUNCTION_BEGIN;
	for (const_quest_iterator it=_quests.begin(); it!=_quests.end(); ++it) {
		it->second->show(showstr);
	}
}

bool CQuestList::save(char* dest,DWORD& retlen){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	retlen=0;
	if ( maxsize< sizeof(int)){ return false;	}

	int count=0;
	int len = sizeof(count);

	//存档顺序结构  
	//数量 数据 
	//int bin 

	for (const_quest_iterator it=_quests.begin(); it!=_quests.end(); ++it) {
		if (!it->second->m_info->_isdelete){

			*((int*)(dest+len))=0;
			len+=sizeof(int);
			int datalen=maxsize-len;
			if (it->second->save(dest+len,datalen)){
				*((int*)(dest+len-sizeof(int)))=datalen;
				len += datalen;
				count++;
			}else{
				return false;
			}
		}
	}
	*((int*)(dest))=count;
	retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen + 1);
	base64_encode(dest,len,pin,retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CQuestList::load(const char* dest,int retlen,int nver){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	if (maxsize==0) return true;

	retlen=ROUNDNUMALL(retlen,4)/4*3;
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen + 1);
	base64_decode((char*)dest,maxsize,pin,retlen);
	memcpy((char*)dest,pin,retlen);
	maxsize=retlen;

	if (maxsize< sizeof(int)){ return false;	}
	int count= *((int*)(dest));
	int len=sizeof(count);
	CQuest* pquest=NULL;
	CQuestInfo* pinfo=NULL;
	while (count>0){
		count--;
		int datalen=safe_max(maxsize-len,0);
		if (datalen>=sizeof(int)){
			int qlen = *((int*)(dest+len));
			len+=sizeof(int);
			if (datalen<qlen){ return false; }

			_GET_TH_LOOPCHARBUF(qlen, true);
			char* pszQuest = ptlsbuf;

			memcpy(pszQuest,dest+len,qlen);
			len+=qlen;
			pquest=CLD_DEBUG_NEW CQuest(pinfo);  
			if (pquest && pquest->load(pszQuest,qlen,nver)){
				if (!add_loaded_quest(pquest)){
					SAFE_DELETE(pquest);
				}
			}else{
				SAFE_DELETE(pquest);
			}
 
		}
	}
	return true;
}

bool CQuestList::reg_evt(CQuest* quest,stEventMapID event,const char* szeventfunc,BYTE range,WORD mapx,WORD mapy){
	FUNCTION_BEGIN;
	stQuestEvent* tmp=NULL;
	tmp=CLD_DEBUG_NEW stQuestEvent;
	if (!tmp){return false;}
	if (strlen(szeventfunc)>=sizeof(tmp->szeventfunc)){SAFE_DELETE(tmp); return false;}
	ZeroMemory(tmp,sizeof(stQuestEvent));
	tmp->quest=quest;
	strcpy_s(tmp->szeventfunc,sizeof(tmp->szeventfunc)-1,szeventfunc);
	tmp->mapx=mapx;
	tmp->mapy=mapy;
	tmp->maprange=range;
	_events.insert(event,tmp);
	return true;
}

bool CQuestList::unreg_evt(CQuest* quest,stEventMapID event){
	FUNCTION_BEGIN;

	qeit it,itnext;
	for(it=_events.getmap().begin(),itnext=it;it!=_events.getmap().end();it=itnext){
		itnext++;
		if ( it->second->quest==quest && (it->first==event || event==-1) ){
			stQuestEvent* tmp=it->second;
			it->second=NULL;
			SAFE_DELETE(tmp);
			_events.erase(it);
		}
	}
	return true;

}

qeits CQuestList::get_evts(stEventMapID event){
	FUNCTION_BEGIN;
	return _events.equal_range(event);
}

//获取已经注册了的事件
stQuestEvent* CQuestList::has_evt(stEventMapID event){
	FUNCTION_BEGIN;
	stQuestEvent* tmp = NULL;
	_events.find(event,tmp);
	return tmp;
}

stQuestEvent* CQuestList::find_evt(WORD evtid, DWORD evtnum){
	FUNCTION_BEGIN;
	toremd_1_3(findid,evtid,evtnum);
	return has_evt(findid._value);
}

stQuestEvent* CQuestList::find_evtbyquest(CQuest* quest){
	FUNCTION_BEGIN;
	if (!quest){return NULL;}
	stQuestEvent* tmpevt=NULL;
	for (qeit it=_events.begin();it!=_events.end();it++)
	{
		tmpevt = it->second;
		if (tmpevt && tmpevt->quest == quest)
		{
			return it->second;
		}
	}
	return NULL;
}

void CQuestList::createQuestInfo(stQuestInfo* pinfo,const CQuestInfo* pQuestInfo,CPlayerObj* pPlayer){
	FUNCTION_BEGIN;
	if(!pPlayer){return;}
	pinfo->id=pQuestInfo->_id;
	pinfo->questsection=pQuestInfo->_questdb->btquestsection;
	pinfo->questtype=pQuestInfo->_quest_type;
	pinfo->questlv=pQuestInfo->_questdb->nminlv;
	pinfo->timelimit=0;
	pinfo->beginnpcid=pQuestInfo->_questdb->beginnpcid;
	pinfo->endnpcid=pQuestInfo->_questdb->endnpcid;
	pinfo->questsubtype = pQuestInfo->_questdb->btquestsubtype;
	bool bbegin=false;
	bool bend=false;
	if(pQuestInfo->_questdb->nloopcount>0){
		sprintf_s(pinfo->questname,_MAX_DESC_LEN_-1,GameService::getMe().GetStrRes(4,"quest"),pQuestInfo->_questdb->nloopcount);
	}else{
		strcpy_s(pinfo->questname,_MAX_DESC_LEN_-1,pQuestInfo->_name.c_str());
	}
	strcpy_s(pinfo->jiangli, _MAX_DESC_LEN_ * 2 - 1, pQuestInfo->_questdb->szquestreward);
	char szAddDesc[100]={0};
	if(pinfo->queststatus == (BYTE)QUESTNO){
		sprintf_s(pinfo->targetdis,_MAX_DESC_LEN_-1,"%s%s",szAddDesc,pQuestInfo->_questdb->szbegintargetdesc);
		sprintf_s(pinfo->dis,_MAX_DESC_LEN_-1,"%s%s",szAddDesc,pQuestInfo->_questdb->szquestdescnot);
	}else if(pinfo->queststatus == QUESTNEW || pinfo->queststatus ==QUESTDOING){
		sprintf_s(pinfo->targetdis, _MAX_DESC_LEN_ - 1, "%s%s", szAddDesc, pQuestInfo->_questdb->szquesttargetdesc);
		sprintf_s(pinfo->dis,_MAX_DESC_LEN_-1,"%s%s",szAddDesc,pQuestInfo->_questdb->szquestdescing);
		if(pinfo->queststatus == QUESTNEW){
			sprintf_s(pinfo->dis,_MAX_DESC_LEN_-1,"%s~~%s",pinfo->dis,pQuestInfo->_questdb->szScreenShowMsg);
		}
	}else{
		sprintf_s(pinfo->targetdis,_MAX_DESC_LEN_-1,"%s%s",szAddDesc,pQuestInfo->_questdb->szendtargetdesc);
		sprintf_s(pinfo->dis,_MAX_DESC_LEN_-1,"%s%s",szAddDesc,pQuestInfo->_questdb->szquestdescfin);
	}



	if (pQuestInfo->_questdb->btitemchoose==1){
		pinfo->itemchoose=true;
	}

	CQuest* pTmpQuest = pPlayer->m_QuestList.find_quest(pinfo->id);
	if(pTmpQuest){
		pinfo->queststatus=pTmpQuest->m_evtvar.status;
		pinfo->btStar=pTmpQuest->m_evtvar.btStar;
	}
}

//人物登陆时发送所有任务
bool CQuestList::loginaddtome(stQuestLogin* retcmd,DWORD pid,bool boSendToMe){
	return true;
}

void CQuestList::setQuestStatus(bool boFirstCheck,CPlayerObj* player,DWORD dwQuestType,DWORD dwCheckId,int nSelect){

}

stCheckItemPurity CQuestList::missionCheckItemConditions(CPlayerObj* player,stQuestConditions &questCondition){
	stCheckItemPurity stCheck;
	return stCheck;
}

static unsigned char g_szClientDataBuffer[1024*1024];
bool CQuestList::doQuestCmd(CPlayerObj* player,stBaseCmd* pcmd,int ncmdlen){	//客户端提交任务
	FUNCTION_BEGIN;
	if(!player){return false;}
	switch (pcmd->value)
	{
	case stQuestDoing::_value:
		{
			_CHECK_PACKAGE_LEN(stQuestDoing,ncmdlen);
		}break;
	case stQuestDelete::_value:
		{
			_CHECK_PACKAGE_LEN(stQuestDelete,ncmdlen);
			stQuestDelete* questdelete=(stQuestDelete*)pcmd;
			DeleteQuest(player,questdelete->id);
		}break;
	case stQuestClientVersion::_value:
		{
			_CHECK_PACKAGE_LEN(stQuestClientVersion,ncmdlen);
			stQuestClientVersion* pSrcCmd=(stQuestClientVersion*)pcmd;
			if (pSrcCmd->ClientVersion && pSrcCmd->ClientVersion[0]!=0){
				stAutoSetScriptParam autoparam(player);
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr("ClientVersion()",pSrcCmd->ClientVersion);
			}
		}break;
	case stQuestClientData::_value:
		{
			_CHECK_PACKAGE_LEN(stQuestClientData,ncmdlen);
			stQuestClientData* pSrcCmd=(stQuestClientData*)pcmd;
			int maxsize = 1024 * 4;
			if (pSrcCmd->DataArr.getarraysize()< maxsize-5 && CUserEngine::getMe().m_scriptsystem.m_LuaVM){
				int size = pSrcCmd->DataArr.getarraysize();
				bool isvalid = true;
				for (int i = 0; i < size; i++) {
					if (pSrcCmd->DataArr[i] == '%') {
						isvalid = false;
						break;
					}
				}
				if(isvalid){
					stAutoSetScriptParam autoparam(player);
					char luaszfunc[1024 * 4] = { 0 };
					sprintf_s(luaszfunc, sizeof(luaszfunc) - 1, "__a__%s", pSrcCmd->DataArr.getptr());
					//DWORD dwCurTime = GetTickCount();
					//g_logger.error("收到网关 包号:%s 时间:%d", luaszfunc, dwCurTime);
					luaszfunc[pSrcCmd->DataArr.getarraysize() + 5] = '\0';
					if(CUserEngine::getMe().m_scriptsystem.m_LuaVM->IsExistFunctionEx(luaszfunc)){
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(luaszfunc,pSrcCmd->dwClientType);
					}else{
						g_logger.debug( "calllua: %s",pSrcCmd->DataArr.getptr() );
					}
				}else{
					g_logger.debug( "has special char calllua: %s",pSrcCmd->DataArr.getptr() );
				}
			}
		}break;
	}
	return true;
}

void CQuestList::questRegEvent(CQuest* pQuest){
	FUNCTION_BEGIN;
	if(pQuest){
		if(pQuest->m_info->_questdb->btquesttargettype == MAPCELL){
			db_reg_evt(pQuest,pQuest->m_info->_questdb->btquesttargettype,
				pQuest->m_info->_questdb->vconditions[0].nId,
				pQuest->m_info->_questdb->nmapposx,
				pQuest->m_info->_questdb->nmapposy,
				1,0);
		}else if(pQuest->m_info->_questdb->btquesttargettype == EQUIPITEM){
			db_reg_evt(pQuest,pQuest->m_info->_questdb->btquesttargettype,
				1,
				pQuest->m_info->_questdb->nmapposx,
				pQuest->m_info->_questdb->nmapposy,
				1,0);

		}else{
			for(DWORD i=0;i<pQuest->m_info->_questdb->vconditions.size();i++){
				if(pQuest->m_info->_questdb->vconditions[i].nId>0){
					db_reg_evt(pQuest,pQuest->m_info->_questdb->btquesttargettype,
						pQuest->m_info->_questdb->vconditions[i].nId);
				}
			}
		}
	}
}

void CQuestList::questCreateQuest(CPlayerObj* player,DWORD dwQuestid,int nChoose,bool boChange, int nstar){
	FUNCTION_BEGIN;
	if(!player){return ;}
	CQuest* tmpquest =add_quest(dwQuestid);
	if(tmpquest){
		tmpquest->m_nStartTime = time(NULL);
		if(tmpquest->m_info->_questdb->i64frontqid >0)
			player->m_QuestCompleteMark.setdata(tmpquest->m_info->_questdb->i64frontqid,QUESTFINISH);
		questRegEvent(tmpquest);
		RefreshQuestStatus(player,dwQuestid,0);
		CreateQuest(player,tmpquest,boChange, nstar);
		GameService::getMe().Send2LogSvr(_SERVERLOG_QUEST_,0,0,player,"'%s','%s','%s',%I64d,%d,%d,%d,%d",
			"Accept",
			player->getAccount(),
			player->getName(),
			player->m_i64UserOnlyID,
			player->m_dwLevel,
			tmpquest->m_nStartTime,
			tmpquest->m_info->_questdb->btquesttype,
			dwQuestid);
		if(tmpquest->m_info->_questdb->btquestsection){
			player->quest_vars_set_var_n("NowQuestSection",tmpquest->m_info->_questdb->btquestsection,true);
		}
		TriggerEvent(player,(EVTTYPE)tmpquest->m_info->_evtid,
			tmpquest->m_info->_evtnum,0,0,0,true);
	}
}

bool CQuestList::questFinishQuest(CPlayerObj* player,DWORD dwQuestid,int nMultiReward,bool compel){
	return false;
}

void CQuestList::FinishQuest(CPlayerObj* player,DWORD qid, bool isLastOne){
	FUNCTION_BEGIN;
	if(!player){return;}
	stQuestFinish retcmd;
	retcmd.id=qid;
	retcmd.queststatus=QUESTFINISH;
	retcmd.isLastOne = 0;
	if (isLastOne)
	{
		retcmd.isLastOne = 1;
	}	

	player->SendMsgToMe(&retcmd,sizeof(retcmd));
}

void CQuestList::optQuestSubmit(CPlayerObj* player,stQuestSubmit* questsubmit){
	FUNCTION_BEGIN;
	if(!player){return;}
	if (player->isDie()) {
		CPlayerObj::sendTipMsgByXml(player,GameService::getMe().GetStrRes(1,"quest"));
		return;
	}
	char* pszQuestid = strstr(questsubmit->funcname,"~");
	if(pszQuestid){
		int nQuestid = atoi(pszQuestid+1);

		if (!questsubmit->boShowOne) {
		}
		else {

			if (strstr(questsubmit->funcname, "queststart~")) {		//可接的任务
				CQuest* tmpquest = find_quest(nQuestid);
				if (!tmpquest) {
					questCreateQuest(player, nQuestid, 0);
				}
			}
			else
				if (strstr(questsubmit->funcname, "questfinish~")) {		//可完成任务
					questFinishQuest(player, nQuestid);
				}
		}
	}else{
		if(CUserEngine::getMe().m_scriptsystem.m_LuaVM->IsExistFunctionEx(questsubmit->funcname)){
			stAutoSetScriptParam aparam(player);
			if (questsubmit->szinput[0]!=0){
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(questsubmit->funcname,0,questsubmit->chooseidx,questsubmit->szinput);
			}
			else{
				CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(questsubmit->funcname,0,questsubmit->chooseidx);
			}
		}else{
			g_logger.debug( "calllua: %s",questsubmit->funcname);
		}
	}
}

bool CQuestList::CheckQuestReceive(CPlayerObj* player,DWORD qid){
	return true;
}

bool CQuestList::CheckQuestShow(CPlayerObj* player,DWORD qid,CCreature* npc){
	
	return true;
}

void CQuestList::DoingQuest(CPlayerObj* player,DWORD qid,BYTE status,const char* menu,const char* pszExtends){
	
}

CQuest* CQuestList::FindQuestByType(CPlayerObj* player,BYTE btQuestType){
	if(!player){return NULL;}
	quest_iterator it,itnext;
	for (it=_quests.begin(),itnext=it;it!=_quests.end();it=itnext)
	{
		itnext++;
		CQuest* tmpqst= it->second;
		if(tmpqst && tmpqst->m_info->_questdb->btquesttype==btQuestType){
			return tmpqst;
		}
	}
	return NULL;
}
void CQuestList::DeleteQuest(CPlayerObj* player,DWORD qid,bool boForce){
	FUNCTION_BEGIN;
	if(!player){return;}
	stQuestDelete retcmd;
	retcmd.id=qid;
	CQuest* tmpqst=find_quest(qid);
	if (tmpqst)
	{
		if (!boForce && tmpqst->m_info->_quest_type == SYSTEM){
			retcmd.bterrorcode=1;
		}
		else
		{
			remove_quest(tmpqst);
			retcmd.bterrorcode=0;
		}
		player->SendMsgToMe(&retcmd,sizeof(retcmd));
	}
}

void CQuestList::RefreshQuestStatus(CPlayerObj* player,DWORD qid,BYTE status,CQuest*pQuest){
	FUNCTION_BEGIN;
	if(!player){return;}
	CQuest* tmpqst= NULL;
	if(pQuest){
		tmpqst = pQuest;
	}else{
		tmpqst=find_quest(qid);
	}

	if (tmpqst)
	{
		tmpqst->m_evtvar.status=(QUESTSTATUS)status;
		if (status==QUESTFINISH)
		{
			player->m_QuestCompleteMark.setdata(qid, (QUESTSTATUS)status);
			player->m_Timer->remove_timer(qid);
			tmpqst->savetimer(NULL);
		}

	}

}

void CQuestList::CreateQuest(CPlayerObj* player,CQuest* quest,bool boChange, int nstar)
{
	FUNCTION_BEGIN;
	if(!player){return;}
	if(!quest || !(quest->m_info)){return;}
	if(!quest->m_info->_questdb){return;}

	quest->m_evtvar.btStar = nstar;

	stQuestCreate retcmd;
	retcmd.info.id=quest->m_info->_id;
	retcmd.info.questtype=quest->m_info->_quest_type;

	strcpy_s(retcmd.info.questname,_MAX_QUEST_LEN_-1,quest->m_info->_name.c_str());
	strcpy_s(retcmd.info.dis,_MAX_DESC_LEN_-1,quest->m_info->_dis.c_str());
	retcmd.info.queststatus = QUESTNEW;
	createQuestInfo(&retcmd.info, quest->m_info, player);
	retcmd.info.timelimit = (DWORD)quest->m_info->_questdb->ntimecheck;
	if (retcmd.info.timelimit != 0)
	{
		player->m_Timer->settimedate(CScriptTimer::CHECKALL, quest->m_info->_id, retcmd.info.timelimit, quest->m_info->_questdb->szeventfunc);
		quest->savetimer(player->m_Timer->find_timer(quest->m_info->_id));
	}

	player->SendMsgToMe(&retcmd, sizeof(retcmd));
}

void CQuestList::TriggerEvent(CPlayerObj* currentuser,EVTTYPE evtid, DWORD evtnum,WORD wTags,WORD mapx,WORD mapy,bool boFirstCheck,int nSelect){
	FUNCTION_BEGIN;
	if (!currentuser){	return;	}
	FUNCTION_MONITOR(48,currentuser->getName());
	stAutoSetScriptParam autoparam(currentuser);
	stEventMapID userevtid=0;
	stQuestEvent* tmpevt=NULL;
	if (evtid==MAPAREA){
		for (WORD rangex=mapx-1;rangex<=mapx+1;rangex++)
		{
			for (WORD rangey=mapy-1;rangey<=mapy+1;rangey++)
			{
				toremd_1_2_3_4(evtmapid,evtid,evtnum,rangex,rangey);
				userevtid=evtmapid._value;
				tmpevt = findallevt(CUserEngine::getMe().m_globalquestinfo._events,userevtid);
				if (tmpevt)
				{
					if (tmpevt->maprange==0 && rangex==mapx && rangey==mapy){
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(tmpevt->szeventfunc);
					}
					else if (tmpevt->maprange==1){
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(tmpevt->szeventfunc);
					}
				}
				tmpevt = has_evt(userevtid);
				if (tmpevt)
				{
					if (tmpevt->maprange==0 && rangex==mapx && rangey==mapy){
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(tmpevt->szeventfunc);
					}
					else if (tmpevt->maprange==1){
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(tmpevt->szeventfunc);
					}
				}
			}
		}
	}
	else {
		toremd_1_3(evtmapid,evtid,evtnum);
		userevtid=evtmapid._value;
		const_qeits its = CUserEngine::getMe().m_globalquestinfo._events.equal_range(userevtid);
		for(const_qeit it = its.first;it != its.second; it++){
			tmpevt = it->second;
			if (tmpevt)
			{
				if(!tmpevt->quest){	//全局成就
					CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(tmpevt->szeventfunc);
				}
			}
			tmpevt = NULL;
		}

		const_qeits its2 = getEvt()->equal_range(userevtid);
		const_qeit it,itnext;
		for(const_qeit it = its2.first,itnext=it;it != its2.second; it=itnext){
			itnext++;
			tmpevt = it->second;
			if (tmpevt)
			{
				setQuestStatus(boFirstCheck,currentuser,evtid,evtnum,nSelect);
				if(evtid != NEEDLV){
					if(sConfigMgr.m_boOpenQuestEvent){
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(tmpevt->szeventfunc);
					}
				}

			}
			tmpevt = NULL;
		}
		if (evtid == ITEMGET && evtnum > 1) {
			char func_name[QUEST_FUNC_LEN];
			sprintf(func_name, "%s", "itemget");
			CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall(func_name, evtnum);
		}
	}
}

bool CQuestList::db_reg_evt(CQuest* quest,WORD evtid,DWORD evtnum,WORD mapx,WORD mapy,BYTE range,BYTE countryid,BYTE lineid){
	FUNCTION_BEGIN;
	if (!quest){return false;}
	if (quest->m_info->_id>DB_QUEST_MAX){return false;}
	if (!quest->m_info->_questdb){return false;}
	if (evtid!=quest->m_info->_questdb->btquesttargettype){return false;}
	stQuestDB tmpdb=*(quest->m_info->_questdb);
	stQuestEvent* tmp=NULL;
	tmp=CLD_DEBUG_NEW stQuestEvent;
	if (!tmp){return false;}
	ZeroMemory(tmp,sizeof(stQuestEvent));
	stEventMapID userevtid=0;
	if (evtid==MAPAREA){
		toremd_1_2_3_4(evt,evtid,evtnum,mapx,mapy);
		userevtid=evt._value;
	}
	else{
		toremd_1_3(evt,evtid,evtnum);
		userevtid=evt._value;
	}
	tmp->quest=quest;
	tmp->mapx=mapx;
	tmp->mapy=mapy;
	tmp->maprange=range;
	((CQuestInfo*)tmp->quest->m_info)->seteventid(evtid,evtnum);
	if(stQuestDB::setEventLuaFunName(evtid) != NULL)
		strcpy_s(tmp->szeventfunc,sizeof(tmp->szeventfunc)-1,stQuestDB::setEventLuaFunName(evtid));
	if (quest->m_info->_questdb->ntimecheck!=0 && tmp->quest->m_evtvar.timercheck==0)
	{
		tmp->quest->m_evtvar.timercheck=quest->m_info->_questdb->ntimecheck;
		tmp->quest->m_evtvar.timestatus=TIMEOK;
	}
	_events.insert(userevtid,tmp);
	return true;
}

bool CQuestList::regevt(WORD evt, DWORD evtex,const char* szeventfunc){
	FUNCTION_BEGIN;
	if ( szeventfunc[0]==0 ){ return false;	}
	toremd_1_3(evtid,evt,evtex);
	if(_events.getCountByKey(evtid._value) >0) return false;
	stQuestEvent* tmp=NULL;
	tmp=CLD_DEBUG_NEW stQuestEvent;
	if (!tmp){return false;}
	if (strlen(szeventfunc)>=sizeof(tmp->szeventfunc)){SAFE_DELETE(tmp); return false;}
	ZeroMemory(tmp,sizeof(stQuestEvent));
	strcpy_s(tmp->szeventfunc,sizeof(tmp->szeventfunc)-1,szeventfunc);
	_events.insert(evtid._value,tmp);
	return true;
}

bool CQuestList::unregevt(WORD evt, DWORD evtex){
	FUNCTION_BEGIN;
	toremd_1_3(evtid,evt,evtex);
	stQuestEvent* tmp = NULL;
	_events.find(evtid._value,tmp);
	if(tmp){
		_events.remove(evtid._value,tmp);
		SAFE_DELETE(tmp);
	}
	return true;
}


stQuestEvent* findallevt(QUESTEVENTMAPS events,stEventMapID event){
	stQuestEvent* tmp = NULL;
	events.find(event,tmp);
	return tmp;
}

CQuestDB::CQuestDB(CQuestInfo *info)
	:m_qinfo(info){
		FUNCTION_BEGIN;
}

stQuestDB* CQuestDB::finddata(DWORD qid){
	FUNCTION_BEGIN;

	auto it = _questdb.find(qid);
	if(it != _questdb.end()){
		return &it->second;
	}
	return NULL;
}


bool CQuestCompleteMark::save(char* dest,DWORD& retlen){
	FUNCTION_BEGIN;
	int maxsize=retlen;
	retlen=0;
	if ( maxsize< (sizeof(int)) ){ return false;}

	int count=0;
	int len = sizeof(count);

	for (commark_ite it=_commarkmap.begin(); it!=_commarkmap.end(); ++it) {
		stQuestCompleteId* tmpcomid=&it->second;
		if (tmpcomid){
			memcpy((void *)(dest+len), tmpcomid,sizeof(stQuestCompleteId));
			len += sizeof(stQuestCompleteId);
			count++;
		}
		else {return false;}
	}
	*((int*)(dest))=count;
	retlen=ROUNDNUMALL(len,3)/3*4;	//当前长度，偏移用
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen + 1);
	base64_encode(dest,len,pin,retlen);
	memcpy(dest, pin, retlen);
	return true;
}

bool CQuestCompleteMark::load(const char* dest,int retlen,int nver){
	FUNCTION_BEGIN;
	int maxsize=retlen;

	retlen=ROUNDNUMALL(retlen,4)/4*3;
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin= ptlsbuf;
	ZeroMemory(pin,retlen + 1);
	base64_decode((char*)dest,maxsize,pin,retlen);
	memcpy((char*)dest,pin,retlen);
	maxsize=retlen;

	if (maxsize< (sizeof(int))){ return false;}

	int count= *((int*)(dest));
	int len=sizeof(count);

	while (count>0){
		count--;
		int datalen=safe_max(maxsize-len,0);
		if (datalen>=sizeof(stQuestCompleteId)){
			stQuestCompleteId* tmpcomid= (stQuestCompleteId*)(dest+len);

			if (!setdata(tmpcomid->dwCompleteId,tmpcomid->status))
			{
				return false;
			}
			else{len += sizeof(stQuestCompleteId);}
		}
		else if (datalen!=0){
			return false;
		}
	}
	return true;
}

bool CQuestCompleteMark::setdata(DWORD qid,QUESTSTATUS status){
	FUNCTION_BEGIN;
	commark_ite it=_commarkmap.find(qid);
	if (it==_commarkmap.end()){
		stQuestCompleteId tmpcomid;
		tmpcomid.dwCompleteId=qid;
		tmpcomid.status=status;
		_commarkmap.insert(QUESTCOMPLETEMARKMAPS::value_type(qid,tmpcomid));
		return true;
	}
	else{
		stQuestCompleteId* tmpcomid=&it->second;
		if (tmpcomid->dwCompleteId==qid){
			tmpcomid->status=status;
			return true;
		}
		else{
			g_logger.forceLog(zLogger::zERROR,vformat("完成标记编号不对应,原ID[%d],输入ID[%d]",tmpcomid->dwCompleteId,qid));
			return false;
		}
	}
}

bool CQuestCompleteMark::remove(DWORD qid){
	FUNCTION_BEGIN;
	commark_ite it=_commarkmap.find(qid);
	if (it!=_commarkmap.end()){
		_commarkmap.erase(it);
		return true;
	}
	return false;
}

QUESTSTATUS CQuestCompleteMark::finddata(DWORD qid){
	FUNCTION_BEGIN;
	commark_ite it=_commarkmap.find(qid);
	if (it!=_commarkmap.end()){
		return ((QUESTSTATUS)it->second.status);
	}
	return QUESTNEW;
}

bool CQuestCompleteMark::findid(DWORD qid){
	FUNCTION_BEGIN;
	commark_ite it=_commarkmap.find(qid);
	if (it!=_commarkmap.end()){
		stQuestCompleteId* tmpcomid=&it->second;
		if (tmpcomid && tmpcomid->dwCompleteId==qid && tmpcomid->status==QUESTFINISH){
			return true;
		}
		else {return false;}
	}
	return false;
}
