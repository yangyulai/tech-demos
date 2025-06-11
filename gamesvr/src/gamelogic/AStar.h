/*------------- AStar.h
*
* Copyright (C):  (2011)
* Author       :
* Version      : V1.01
* Date         : 2011/12/11 18:50:29
*
*/
/*************************************************************
*A*�㷨
*************************************************************/
#pragma once
#include "qglobal.h"
#include "GameMap.h"

#pragma pack(push,1)
//////////////////////////////////////////////////////////////////////////

#pragma pack(pop)

#define VALUEG_TEN				10   //�ƶ��ķ�10
#define VALUEG_FOURTEEN			14   //�ƶ��ķ�14

#define VALUEG_ONEWANG			1000


enum enumNodePointState
{
	Node_NotExist,
	Node_InOpenList,			//�ڿ����б���
	Node_InClosedList,			//�ڹر��б���
};

struct NodePoints
{
	Points point;					//�ڵ������ĵ�  
	enumNodePointState state;
	int nValueG;					//gֵ(ˮƽ����ֱ����10�Խ���Ϊ14)
	int nValueH;					//hֵ
	NodePoints* nFatherpoint;		//�ýڵ�ĸ��ڵ�
	NodePoints()
	{
		ZEROSELF;
	}

	inline int f() const {
		return nValueG + nValueH;
	}
};

class CAStar
{
public:

	CAStar(CGameMap* pMap);
	virtual ~CAStar();
	void FindPath(int nX, int nY, int ndX, int ndY, std::list<Points>& listPoth);
private:
	static std::CSyncList<NodePoints*> sNodePointPool;
	static NodePoints* GetPoolPoint(int nx, int ny);
	static void PutNode2Pool(NodePoints* node);

	std::vector<NodePoints*> m_mapGrids;
	std::vector<NodePoints*> m_openList;
	CGameMap* pdsMap;									//���ڵ�ͼָ��
	std::vector<Points> m_nearbyPoints;
	void findCanPassNodes(NodePoints* node);
	bool CanWalk(int nx, int ny);
	bool CanWalk2Next(int nx, int ny, int ndx, int ndy);
	bool IsInOpenList(int nx, int ny, NodePoints*& outNode);
	bool IsInClosedList(int nx, int ny);
	void ProcessNodeInOpenList(NodePoints* cur, NodePoints* dst);
	void ProcessNodeNotInOpenList(NodePoints* cur, NodePoints* dst, int ndx, int ndy);
	int CalculateGValue(NodePoints* parent, int ndx, int ndy);
	bool GetNodeIndex(NodePoints* node, int* index);
	void PercolateUp(int hole);
};