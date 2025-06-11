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

//�����˺��ͼ���
class Magic_Common : public stMagic
{
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
};

//����ħ���ͼ���
class Magic_Map : public stMagic
{
public:
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;

	//����ħ������ִ�У��״�ִ��ʱ������ doSkill ��ħ�������ͼ������ִ��ʱ������ run ִ��ʵ�ʲ�����
	virtual void run(CCreature* pCret, CGameMap* m_OwnerMap, int targetX, int targetY);

	static void AddMapMagic(CCreature* pCret, stMagic* pMagic, Magic_Map* pMapMagic, PosType nDx, PosType nDy);		//��ӵ�ͼ����
};

//�ƶ��༼��
class Magic_Move : public stMagic
{
public:
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
	bool OnCretStruck(CCreature* pCret) override;

	int m_nPosX = 0;
	int m_nPosY = 0;
};

//�����༼��
class Magic_Shield : public stMagic
{
public:
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
};

//BUFF�༼��
class Magic_Buff : public stMagic
{
public:
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
};


class Magic_Healing : public stMagic								//������
{
public:
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
};

class Magic_Reply : public stMagic									//�ظ�
{
	bool doSkill(CCreature* pCret, stCretAttack* pcmd) override;
};



#endif    //#define _CLD_MAGIC_EXTEND_H_