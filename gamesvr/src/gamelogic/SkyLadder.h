#pragma once
#include "server_cmd.h"
#include "lua_base.h"
#include "Point.h"
class CPlayerObj;
class CRobot;
class CGameMap;

class CSkyLadder
{
public:
	CSkyLadder();
	~CSkyLadder() = default;

	void run();
	void SendLoadDataCmdToDB(CPlayerObj* pPlayer, int64_t i64DstOnlyid, std::string strAcctount);
	void doLoadRobotData(stLoadDataToRobotRet* pCmd);
	CRobot* CreateRobotMon(CPlayerObj* pPlayer, stLoadPlayerData* pGameData);
	CRobot* CreateRobotMon(CGameMap* pMap, PosType x, PosType y, sol::table tab);

	ULONGLONG m_nextRunTickCount;
	std::CSyncList<stLoadDataToRobotRet*> m_waitCreateRobot;
};
