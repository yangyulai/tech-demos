#ifndef _CLD_MAGIC_EXTEND_H_
#define _CLD_MAGIC_EXTEND_H_
#pragma once

#include "Magic.h"
#include "Point.h"

class MagicExtend 
{
public:
	static std::unique_ptr<stMagic> CreateSkill(DWORD skillid, BYTE level, BYTE magictype);
};

//常规伤害型技能
class Magic_Common : public stMagic
{
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
};

//地面魔法型技能
class Magic_Map : public stMagic
{
public:
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;

	//地面魔法需多次执行（首次执行时，调用 doSkill 将魔法加入地图；后续执行时，调用 run 执行实际操作）
	virtual void run(CCreature* pCret, CGameMap* m_OwnerMap, int targetX, int targetY);

	static void AddMapMagic(CCreature* pCret, stMagic* pMagic, Magic_Map* pMapMagic, PosType nDx, PosType nDy);		//添加地图技能
};

//移动类技能
class Magic_Move : public stMagic
{
public:
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
	bool OnCretStruck(CCreature* pCret) override;

	int m_nPosX = 0;
	int m_nPosY = 0;
};

//护盾类技能
class Magic_Shield : public stMagic
{
public:
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
};

//BUFF类技能
class Magic_Buff : public stMagic
{
public:
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
};


class Magic_Healing : public stMagic								//治愈术
{
public:
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
};

class Magic_Reply : public stMagic									//回复
{
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
};



#endif    //#define _CLD_MAGIC_EXTEND_H_