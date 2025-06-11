#ifndef __GAME_SCRIPTSQL_H_389R4JSLDJFLDF_
#define __GAME_SCRIPTSQL_H_389R4JSLDJFLDF_

#include "define.h"
#include <sstream>
#include <map>
#include <hash_map>
#include "HashManage.h"
#include "db/DBConnPool.h"
#include "qglobal.h"
#include "cmd/ScriptSql_cmd.h"
#include "Script.h"
#define _SQLBUFCHECK_SUM_		0x22558800
class CScriptSql
{
public:
	bool SetSql(DWORD mark,const char* szsqlurl);//设置数据库连接字符串
	bool  GetCount(DWORD mark,const char* tableName, const char* whereid,const char* retfunc);//得到数量
	bool SelectSql(DWORD mark,const char* szsql,const char* retfunc);//执行SELECT语句
	bool ExecuteSql(DWORD mark,const char* szsql,const char* retfunc);//执行其他SQL语句
 	const char* GetData(int row,int col);//得到数据 行 列
	CScriptSql():m_dwchecksum(_SQLBUFCHECK_SUM_)/*,m_dwboardchecksum(_BOARDSQLBUFCHECK_SUM_)*/{ m_dwRunRetFuncTick=0;}

	void RunSql();//注意线程安全(现在是单线程ScriptSqlThread调用)
	void RunFunc();//注意线程安全(现在是单线程UsrEngn调用)

	bool SuperGetCount(DWORD mark,const char* tableName, const char* whereid,const char* retfunc);//super执行获取数量
	bool SuperSelectSql(CPlayerObj* player, DWORD mark,const char* szsql,const char* retfunc);//执行SELECT语句
	bool SuperExecuteSql(CPlayerObj* player, DWORD mark,const char* szsql,const char* retfunc);//super执行其他SQL语句
	
private:
	stSql m_sqldata[MAX_SQLDATA_ROW_COUNT+2];
	const DWORD m_dwchecksum;
	CLD_DBConnPool m_scriptsqldb;
	std::CSyncList< stScriptSqlStatement > m_RunScriptSql;		//多线程调用,注意线程安全,要锁
	stScriptRetFunc m_RunRetFunc;								//多线程调用,注意线程安全
	ULONGLONG m_dwRunRetFuncTick;
};


#endif __GAME_SCRIPTSQL_H_389R4JSLDJFLDF_