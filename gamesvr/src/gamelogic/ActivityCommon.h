#pragma once
#include "Script.h"

class CGameMap;
class ActivityCommon
{
public:

public:
	static bool SendMail(double receiveid, const char* strPlayerName, const char* strTitle, const char* strContent, sol::table itemtab);
	static bool UpdateMapId2Global(DWORD dwMapUnionId, DWORD dwGuildId, WORD wSvrId);
};
