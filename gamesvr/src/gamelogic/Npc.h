#pragma once

#include "BaseCreature.h"

class CNpcObj:public CCreature
{
public:
	CNpcObj(const CNpcObj& other) = delete;
	CNpcObj(CNpcObj&& other) noexcept = delete;
	CNpcObj& operator=(const CNpcObj& other) = delete;
	CNpcObj& operator=(CNpcObj&& other) noexcept = delete;
	CNpcObj(PosType x, PosType y,mydb_npcgen_tbl* pnpcinfo);
	~CNpcObj() override;

	void Update() final;
	bool Die() final { return false; }

	const char* getShowName(char* szbuffer = NULL, int nmaxlen = 0) override;		//获得显示用的名字
	void run();
	void SetEmblem(DWORD dwEmblemId);

	DWORD getNpcId() const { if (m_pNpcInfo){return m_pNpcInfo->id;}else {return 0;} }
	DWORD getNpcScriptId() const { if (m_pNpcInfo){return m_pNpcInfo->scriptId;}else {return 0;} }
	void EnterMapNotify(MapObject* obj) override;

	char m_szNpcFullName[_MAX_NAME_LEN_];
	char m_szNpcName[_MAX_NAME_LEN_];
	char m_szNpcShowName[_MAX_NAME_LEN_];//显示名字
	DWORD m_dwDeleteTime;				//被删除的时间
	mydb_npcgen_tbl* m_pNpcInfo;
	DWORD m_dwEmblemId;
};