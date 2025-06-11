/*------------- AStar.h
*
* Copyright (C):  (2011)
* Author       :
* Version      : V1.01
* Date         : 2011/12/11 18:50:29
*
*/
/*************************************************************
*A*算法
*************************************************************/
#pragma once
#include "qglobal.h"
#include "GameMap.h"

#pragma pack(push,1)
//////////////////////////////////////////////////////////////////////////

#pragma pack(pop)

#define VALUEG_TEN				10   //移动耗费10
#define VALUEG_FOURTEEN			14   //移动耗费14

#define VALUEG_ONEWANG			1000


enum enumNodePointState
{
	Node_NotExist,
	Node_InOpenList,			//在开启列表中
	Node_InClosedList,			//在关闭列表中
};

struct NodePoints
{
	Points point;					//节点所处的点  
	enumNodePointState state;
	int nValueG;					//g值(水平方向垂直方向10对角线为14)
	int nValueH;					//h值
	NodePoints* nFatherpoint;		//该节点的父节点
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
	CGameMap* pdsMap;									//所在地图指针
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