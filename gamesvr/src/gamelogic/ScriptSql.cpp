#include "ScriptSql.h"
#include "UsrEngn.h"
#include "timeMonitor.h"

bool CScriptSql::SetSql(DWORD mark,const char* szsqlurl){
	FUNCTION_BEGIN;
	if (!szsqlurl || szsqlurl[0]==0){return false;}

	do 
	{
		stScriptSqlStatement SqlStatement;
		SqlStatement.btStatementType=Statement_PutSqlUrl;
		SqlStatement.dwSqlMark=mark;
		strcpy_s(SqlStatement.szSqlStatement,_MAX_SQLSTATEMENT_LEN_-1,szsqlurl);
		SqlStatement.tAddTime=time(NULL);
		SqlStatement.onlyId =0;
		AILOCKT(m_RunScriptSql);
		m_RunScriptSql.push_back(SqlStatement);
	} while (false);
	return true;
}

bool CScriptSql::GetCount(DWORD mark,const char* tableName, const char* whereid,const char* retfunc){
	FUNCTION_BEGIN;
	if (!tableName || tableName[0]==0){return false;}
	int count=0;
		do 
		{
			stScriptSqlStatement SqlStatement;
			SqlStatement.btStatementType=Statement_GetCount;
			SqlStatement.dwSqlMark=mark;
			strcpy_s(SqlStatement.szSqlStatement,_MAX_NAME_LEN_-1,tableName);
			strcpy_s(SqlStatement.szWhereId,_MAX_RETFUNC_LEN_-1,whereid);
			strcpy_s(SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,retfunc);
			SqlStatement.tAddTime=time(NULL);
			SqlStatement.onlyId = 0;
			AILOCKT(m_RunScriptSql);
			m_RunScriptSql.push_back(SqlStatement);
		} while (false);
		return true;
	return false;
}

bool CScriptSql::SelectSql(DWORD mark,const char* szsql,const char* retfunc){
	FUNCTION_BEGIN;
	if (!szsql || szsql[0]==0){return false;}
	if (!retfunc || retfunc[0]==0){return false;}

	int count=0;
		do 
		{
			stScriptSqlStatement SqlStatement;
			SqlStatement.btStatementType=Statement_Select;
			SqlStatement.dwSqlMark=mark;
			strcpy_s(SqlStatement.szSqlStatement,_MAX_SQLSTATEMENT_LEN_-1,szsql);
			strcpy_s(SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,retfunc);
			SqlStatement.tAddTime=time(NULL);
			SqlStatement.onlyId = 0;
			AILOCKT(m_RunScriptSql);
			m_RunScriptSql.push_back(SqlStatement);
		} while (false);
		return true;
	return false;
}

bool CScriptSql::ExecuteSql(DWORD mark,const char* szsql,const char* retfunc){
	FUNCTION_BEGIN;
	if (!szsql || szsql[0]==0){return false;}
	if (!retfunc || retfunc[0]==0){return false;}
	int count=0;
		do 
		{
			stScriptSqlStatement SqlStatement;
			SqlStatement.btStatementType=Statement_Execute;
			SqlStatement.dwSqlMark=mark;
			strcpy_s(SqlStatement.szSqlStatement,_MAX_SQLSTATEMENT_LEN_-1,szsql);
			strcpy_s(SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,retfunc);
			SqlStatement.tAddTime=time(NULL);
			SqlStatement.onlyId = 0;
			AILOCKT(m_RunScriptSql);
			m_RunScriptSql.push_back(SqlStatement);
		} while (false);
		return true;
	return false;
}

//========================================================================================================================

bool CScriptSql::SuperGetCount(DWORD mark,const char* tableName, const char* whereid,const char* retfunc){
	FUNCTION_BEGIN;
	if (!tableName || tableName[0]==0){return false;}

	do 
	{
		stSuperScriptSqlStatement retcmd;
		retcmd.SqlStatement.btStatementType=Statement_GetCount;
		retcmd.SqlStatement.dwSqlMark=mark;
		strcpy_s(retcmd.SqlStatement.szSqlStatement,_MAX_NAME_LEN_-1,tableName);
		strcpy_s(retcmd.SqlStatement.szWhereId,_MAX_RETFUNC_LEN_-1,whereid);
		strcpy_s(retcmd.SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,retfunc);
		retcmd.SqlStatement.tAddTime=time(NULL);
		retcmd.SqlStatement.onlyId = 0;
		return CUserEngine::getMe().SendMsg2SuperSvr(&retcmd,sizeof(stSuperScriptSqlStatement));
	} while (false);

	return false;
}

bool CScriptSql::SuperSelectSql(CPlayerObj* player, DWORD mark,const char* szsql,const char* retfunc){
	FUNCTION_BEGIN;
	if (!szsql || szsql[0]==0){return false;}
	if (!retfunc || retfunc[0]==0){return false;}

	do 
	{
		stSuperScriptSqlStatement retcmd;
		retcmd.SqlStatement.btStatementType=Statement_Select;
		retcmd.SqlStatement.dwSqlMark=mark;
		strcpy_s(retcmd.SqlStatement.szSqlStatement,_MAX_SQLSTATEMENT_LEN_-1,szsql);
		strcpy_s(retcmd.SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,retfunc);
		retcmd.SqlStatement.tAddTime=time(NULL);
		retcmd.SqlStatement.onlyId = player->m_i64UserOnlyID;
		
		return CUserEngine::getMe().SendMsg2SuperSvr(&retcmd,sizeof(stSuperScriptSqlStatement));
	} while (false);

	return false;
}

bool CScriptSql::SuperExecuteSql(CPlayerObj* player, DWORD mark,const char* szsql,const char* retfunc){
	FUNCTION_BEGIN;
	if (!szsql || szsql[0]==0){return false;}
	if (!retfunc || retfunc[0]==0){return false;}

	do 
	{
		stSuperScriptSqlStatement retcmd;
		retcmd.SqlStatement.btStatementType=Statement_Execute;
		retcmd.SqlStatement.dwSqlMark=mark;
		strcpy_s(retcmd.SqlStatement.szSqlStatement,_MAX_SQLSTATEMENT_LEN_-1,szsql);
		strcpy_s(retcmd.SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,retfunc);
		retcmd.SqlStatement.tAddTime=time(NULL);
		retcmd.SqlStatement.onlyId = player->m_i64UserOnlyID;
		return CUserEngine::getMe().SendMsg2SuperSvr(&retcmd,sizeof(stSuperScriptSqlStatement));
	} while (false);

	return false;
}

//========================================================================================================================

const char* CScriptSql::GetData(int row,int col){
	FUNCTION_BEGIN;
	if (row>=0 && row<MAX_SQLDATA_ROW_COUNT && col>=0 && col<MAX_SQLDATA_COL_COUNT ){
		return m_sqldata[row].szretsql[col];
	}
	return "";
}


void CScriptSql::RunSql(){
	FUNCTION_BEGIN;
	if (!m_RunRetFunc.boRunState){
		std::CSyncList< stScriptSqlStatement >::iterator it;
		bool boHaveRunSql=false;
		m_RunRetFunc.nCount=0;
		m_RunRetFunc.SqlStatement.szSqlStatement[0]=0;
		m_RunRetFunc.SqlStatement.szRetFunc[0]=0;
		do 
		{
			AILOCKT(m_RunScriptSql);
			it=m_RunScriptSql.begin();
			if (it!=m_RunScriptSql.end()){
				m_RunRetFunc.SqlStatement=(*it);
				boHaveRunSql=true;
				m_RunScriptSql.erase(it);
			}
		} while (false);

		if (boHaveRunSql){
			boHaveRunSql=false;
			time_t thistime=time(NULL);
			int noldStatementType=m_RunRetFunc.SqlStatement.btStatementType;
			if ((thistime>m_RunRetFunc.SqlStatement.tAddTime+_MAX_TIMEOUT_) && m_RunRetFunc.SqlStatement.btStatementType!=Statement_PutSqlUrl){
				m_RunRetFunc.SqlStatement.btStatementType=Statement_TimeOut;
			}
			switch (m_RunRetFunc.SqlStatement.btStatementType)
			{
			case Statement_Select:
				{
					GETAUTOSQL(CSqlClientHandle*, sqlchandle,m_scriptsqldb, m_RunRetFunc.SqlStatement.dwSqlMark);
					int count=0;
					if (sqlchandle){
						FUNCTION_MONITOR(1000*60,"CScriptSql::SelectSql()");
						count=sqlchandle->execSelectSql( m_RunRetFunc.SqlStatement.szSqlStatement,Sql_define,(unsigned char*)&m_sqldata[0],sizeof(m_sqldata)-sizeof(stSql)*2 );
						if (m_dwchecksum==_SQLBUFCHECK_SUM_){
							if (count>0){
								//sprintf_s(m_RunRetFunc.SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,"%s(%d,%d)",m_RunRetFunc.SqlStatement.szRetFunc,1,count);
								m_RunRetFunc.btRetFuncErrorType=RetFunc_Success;
							}
							else{
								//sprintf_s(m_RunRetFunc.SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,"%s(%d,%d)",m_RunRetFunc.SqlStatement.szRetFunc,0,count);
								m_RunRetFunc.btRetFuncErrorType=RetFunc_Fail;
							}
							m_RunRetFunc.nCount=count;
						}else{
							g_logger.error("SelectSql 数据量过大,缓冲溢出!");
							m_RunRetFunc.btRetFuncErrorType=RetFunc_Error_BufferOverflow;
						}
					}else{
						g_logger.error("SelectSql 没有数据连接!");
						m_RunRetFunc.btRetFuncErrorType=RetFunc_Error_NoSqlConnection;
					}
				}break;
			case Statement_Execute:
				{
					GETAUTOSQL(CSqlClientHandle*, sqlchandle,m_scriptsqldb, m_RunRetFunc.SqlStatement.dwSqlMark);
					int count=0;
					if (sqlchandle){
						FUNCTION_MONITOR(1000*60,"CScriptSql::ExecuteSql()");
						if (sqlchandle->execSql(m_RunRetFunc.SqlStatement.szSqlStatement,sizeof(m_RunRetFunc.SqlStatement.szSqlStatement))==0){
							count=max(0,sqlchandle->getaffectedrows());
							//sprintf_s(m_RunRetFunc.SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,"%s(%d,%d)",m_RunRetFunc.SqlStatement.szRetFunc,1,count);
							m_RunRetFunc.btRetFuncErrorType=RetFunc_Success;
						}
						else{
							count=max(0,sqlchandle->getaffectedrows());
							//sprintf_s(m_RunRetFunc.SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,"%s(%d,%d)",m_RunRetFunc.SqlStatement.szRetFunc,0,count);
							m_RunRetFunc.btRetFuncErrorType=RetFunc_Fail;
						}
						m_RunRetFunc.nCount=count;
					}else{
						g_logger.error("ExecuteSql 没有数据连接!");
						m_RunRetFunc.btRetFuncErrorType=RetFunc_Error_NoSqlConnection;
					}
				}break;
			case Statement_SelectBoard:
				{
// 					GETAUTOSQL(CSqlClientHandle*, sqlchandle,m_scriptsqldb, m_RunRetFunc.SqlStatement.dwSqlMark);
// 					int count=0;
// 					if (sqlchandle){
// 						FUNCTION_MONITOR(1000*60,"CScriptSql::SelectBoard()");
// 						memset(m_board, 0, sizeof(stBoardTbl)*MAX_BOARD_RECORD_NUM);
// 						count=sqlchandle->execSelectSql( m_RunRetFunc.SqlStatement.szSqlStatement, BoardTbl_define, (unsigned char*)&m_board[0], sizeof(m_board)-sizeof(stBoardTbl)*2);
// 						if (m_dwboardchecksum==_BOARDSQLBUFCHECK_SUM_){
// 							if (count>0){
// 								//sprintf_s(m_RunRetFunc.SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,"%s(%d,%d)",m_RunRetFunc.SqlStatement.szRetFunc,1,count);
// 								m_RunRetFunc.btRetFuncErrorType=RetFunc_Success;
// 							}
// 							else{
// 								//sprintf_s(m_RunRetFunc.SqlStatement.szRetFunc,_MAX_RETFUNC_LEN_-1,"%s(%d,%d)",m_RunRetFunc.SqlStatement.szRetFunc,0,count);
// 								m_RunRetFunc.btRetFuncErrorType=RetFunc_Fail;
// 							}
// 							m_RunRetFunc.nCount=count;
// 						}else{
// 							g_logger.error("SelectBoard 数据量过大,缓冲溢出!");
// 							m_RunRetFunc.btRetFuncErrorType=RetFunc_Error_BufferOverflow;
// 						}
// 					}else{
// 						g_logger.error("SelectBoard 没有数据连接!");
// 						m_RunRetFunc.btRetFuncErrorType=RetFunc_Error_NoSqlConnection;
// 					}
				}break;
			case Statement_GetCount:
				{
					GETAUTOSQL(CSqlClientHandle*, sqlchandle,m_scriptsqldb, m_RunRetFunc.SqlStatement.dwSqlMark);
					int count=0;
					if (sqlchandle){
						FUNCTION_MONITOR(1000*60,"CScriptSql::GetCount()");
						count=sqlchandle->getCount(m_RunRetFunc.SqlStatement.szSqlStatement,m_RunRetFunc.SqlStatement.szWhereId);
						if (count>=0){
							m_RunRetFunc.btRetFuncErrorType=RetFunc_Success;
							m_RunRetFunc.nCount=count;
						}else{
							m_RunRetFunc.btRetFuncErrorType=RetFunc_Fail;
							m_RunRetFunc.nCount=0;
						}
					}else{
						g_logger.error("GetCount 没有数据连接!");
						m_RunRetFunc.btRetFuncErrorType=RetFunc_Error_NoSqlConnection;
					}
				}break;
			case Statement_PutSqlUrl:
				{
				 	FUNCTION_MONITOR(1000*60,"CScriptSql::SetSql()");
				 	if (m_scriptsqldb.putURL(m_RunRetFunc.SqlStatement.dwSqlMark,m_RunRetFunc.SqlStatement.szSqlStatement,false,4)){
				 		m_RunRetFunc.btRetFuncErrorType=RetFunc_Success;
					}else{
						m_RunRetFunc.btRetFuncErrorType=RetFunc_Fail;
					}
					return;
				}break;
			default:
				{
					g_logger.error( "(%d)脚本执行sql超时:%d:%s (%s) ",noldStatementType,m_RunRetFunc.SqlStatement.dwSqlMark,m_RunRetFunc.SqlStatement.szRetFunc,m_RunRetFunc.SqlStatement.szSqlStatement );
					m_RunRetFunc.btRetFuncErrorType=RetFunc_Error_TimeOut;
				}break;
			}
			m_RunRetFunc.boRunState=true;
		}
	}
}

void CScriptSql::RunFunc(){
	FUNCTION_BEGIN;
	ULONGLONG thistick=GetTickCount();
	if (thistick>m_dwRunRetFuncTick){
		if (m_RunRetFunc.boRunState){
			if (CUserEngine::getMe().m_scriptsystem.m_LuaVM){
				CPlayerObj* pCurPlayer=CUserEngine::getMe().m_playerhash.FindByOnlyId(m_RunRetFunc.SqlStatement.onlyId);
				stAutoSetScriptParam autoparam(pCurPlayer);
				switch (m_RunRetFunc.btRetFuncErrorType)
				{
				case RetFunc_Success:
					{
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(m_RunRetFunc.SqlStatement.szRetFunc,m_RunRetFunc.btRetFuncErrorType,m_RunRetFunc.nCount);
					}break;
				default:
					{
						CUserEngine::getMe().m_scriptsystem.m_LuaVM->VCall_LuaStr(m_RunRetFunc.SqlStatement.szRetFunc,m_RunRetFunc.btRetFuncErrorType,m_RunRetFunc.nCount);
					}break;
				}
			}
			m_RunRetFunc.boRunState=false;
		}
		m_dwRunRetFuncTick=thistick+100;
	}
}