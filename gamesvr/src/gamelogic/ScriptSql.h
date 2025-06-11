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
	bool SetSql(DWORD mark,const char* szsqlurl);//�������ݿ������ַ���
	bool  GetCount(DWORD mark,const char* tableName, const char* whereid,const char* retfunc);//�õ�����
	bool SelectSql(DWORD mark,const char* szsql,const char* retfunc);//ִ��SELECT���
	bool ExecuteSql(DWORD mark,const char* szsql,const char* retfunc);//ִ������SQL���
 	const char* GetData(int row,int col);//�õ����� �� ��
	CScriptSql():m_dwchecksum(_SQLBUFCHECK_SUM_)/*,m_dwboardchecksum(_BOARDSQLBUFCHECK_SUM_)*/{ m_dwRunRetFuncTick=0;}

	void RunSql();//ע���̰߳�ȫ(�����ǵ��߳�ScriptSqlThread����)
	void RunFunc();//ע���̰߳�ȫ(�����ǵ��߳�UsrEngn����)

	bool SuperGetCount(DWORD mark,const char* tableName, const char* whereid,const char* retfunc);//superִ�л�ȡ����
	bool SuperSelectSql(CPlayerObj* player, DWORD mark,const char* szsql,const char* retfunc);//ִ��SELECT���
	bool SuperExecuteSql(CPlayerObj* player, DWORD mark,const char* szsql,const char* retfunc);//superִ������SQL���
	
private:
	stSql m_sqldata[MAX_SQLDATA_ROW_COUNT+2];
	const DWORD m_dwchecksum;
	CLD_DBConnPool m_scriptsqldb;
	std::CSyncList< stScriptSqlStatement > m_RunScriptSql;		//���̵߳���,ע���̰߳�ȫ,Ҫ��
	stScriptRetFunc m_RunRetFunc;								//���̵߳���,ע���̰߳�ȫ
	ULONGLONG m_dwRunRetFuncTick;
};


#endif __GAME_SCRIPTSQL_H_389R4JSLDJFLDF_