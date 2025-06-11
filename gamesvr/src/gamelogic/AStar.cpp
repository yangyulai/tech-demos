/*------------- AStar.cpp
*
* Copyright (C): qinyu (2019)
* Author       :
* Version      : V1.01
* Date         : 2019/11/12 14:50:29
*
*/
/*************************************************************
*A*Ëã·¨
*************************************************************/
#include "AStar.h"
std::CSyncList<NodePoints*> CAStar::sNodePointPool;

NodePoints* CAStar::GetPoolPoint(int nx, int ny)
{
	NodePoints* node = NULL;
	AILOCKT(sNodePointPool);
	if (sNodePointPool.size() > 0) {
		node = sNodePointPool.front();
		sNodePointPool.pop_front();
	}
	else {
		node = new NodePoints();
	}
	node->state = Node_NotExist;
	node->point.nx = nx;
	node->point.ny = ny;
	node->nFatherpoint = NULL;
	node->nValueG = 0;
	node->nValueH = 0;
	return node;
}

void CAStar::PutNode2Pool(NodePoints* node)
{
	AILOCKT(sNodePointPool);
	sNodePointPool.push_back(node);
}

CAStar::CAStar(CGameMap* pMap)
{
	FUNCTION_BEGIN;
	pdsMap = pMap;
	m_mapGrids.resize(pMap->m_nWidth * pMap->m_nHeight, NULL);
	m_nearbyPoints.reserve(8);
}

CAStar::~CAStar()
{
	int idx = 0;
	int num = pdsMap->m_nWidth * pdsMap->m_nHeight;
	while (idx < num)
	{
		if (m_mapGrids[idx] != NULL) {
			PutNode2Pool(m_mapGrids[idx]);
		}
		idx++;
	}
	m_mapGrids.clear();
	m_openList.clear();
	m_nearbyPoints.clear();
}

void CAStar::PercolateUp(int hole)
{
	size_t parent = 0;
	while (hole > 0)
	{
		parent = (hole - 1) / 2;
		if (m_openList[hole]->f() < m_openList[parent]->f())
		{
			std::swap(m_openList[hole], m_openList[parent]);
			hole = parent;
		}
		else
		{
			return;
		}
	}
}

inline int CAStar::CalculateGValue(NodePoints* parent, int ndx, int ndy)
{
	int value = (abs(parent->point.nx - ndx) + abs(parent->point.ny - ndy)) == 2 ? 14 : 10;
	return value + parent->nValueG;
}

bool CAStar::GetNodeIndex(NodePoints* node, int* index)
{
	*index = 0;
	const int openSize = m_openList.size();
	while (*index < openSize) {
		if (m_openList[*index]->point.nx == node->point.nx && m_openList[*index]->point.ny == node->point.ny)
		{
			return true;
		}
		++(*index);
	}
	return false;
}

void CAStar::ProcessNodeInOpenList(NodePoints* cur, NodePoints* dst)
{
	int gValue = CalculateGValue(cur, dst->point.nx, dst->point.ny);
	if (gValue < dst->nValueG)
	{
		dst->nValueG = gValue;
		dst->nFatherpoint = cur;
		int index = 0;
		if (GetNodeIndex(dst, &index))
		{
			PercolateUp(index);
		}
	}
}

void CAStar::ProcessNodeNotInOpenList(NodePoints* cur, NodePoints* dst, int ndx, int ndy)
{
	dst->nFatherpoint = cur;
	dst->nValueH = (abs(dst->point.nx - ndx) + abs(dst->point.ny)) * 10;
	dst->nValueG = CalculateGValue(cur, dst->point.nx, dst->point.ny);

	NodePoints*& refNode = m_mapGrids[dst->point.ny * pdsMap->m_nWidth + dst->point.nx];
	refNode = dst;
	refNode->state = Node_InOpenList;
	m_openList.push_back(dst);
	std::push_heap(m_openList.begin(), m_openList.end(), [](const NodePoints* a, const NodePoints* b)->bool
		{
			return a->f() > b->f();
		});
}


inline bool CAStar::IsInOpenList(int nx, int ny, NodePoints*& outNode)
{
	outNode = m_mapGrids[ny * pdsMap->m_nWidth + nx];
	return outNode ? outNode->state == Node_InOpenList : false;
}

inline bool CAStar::IsInClosedList(int nx, int ny)
{
	NodePoints* node = m_mapGrids[ny * pdsMap->m_nWidth + nx];
	return node ? node->state == Node_InClosedList : false;
}

bool CAStar::CanWalk(int nx, int ny)
{
	return nx >= 0 && nx < pdsMap->m_nWidth&& ny >= 0 && ny < pdsMap->m_nHeight&& pdsMap->IsMapCanPass(nx, ny);
}

bool CAStar::CanWalk2Next(int nx, int ny, int ndx, int ndy)
{
	if (nx >= 0 && nx < pdsMap->m_nWidth && ny >= 0 && ny < pdsMap->m_nHeight) {
		if (IsInClosedList(ndx, ndy)) {
			return false;
		}
		return CanWalk(ndx, ndy);
	}
	return false;
}


void CAStar::findCanPassNodes(NodePoints* node)
{
	Points pt;
	int row_index = node->point.ny - 1;
	const int max_row = node->point.ny + 1;
	const int max_col = node->point.nx + 1;

	if (row_index < 0)
	{
		row_index = 0;
	}
	while (row_index <= max_row)
	{
		int col_index = node->point.nx - 1;

		if (col_index < 0)
		{
			col_index = 0;
		}

		while (col_index <= max_col)
		{
			pt.nx = col_index;
			pt.ny = row_index;

			if (CanWalk2Next(node->point.nx, node->point.ny, pt.nx, pt.ny))
			{
				m_nearbyPoints.push_back(pt);
			}
			++col_index;
		}
		++row_index;
	}
}


void  CAStar::FindPath(int nX, int nY, int ndX, int ndY, std::list<Points>& listPath)
{
	FUNCTION_BEGIN;
	ULONGLONG tick = GetTickCount64();
	if (pdsMap->CanWalk(nX, nY, 0, true) && pdsMap->CanWalk(ndX, ndY, 0, true))
	{
		//push 2 open list
		NodePoints* startNode = GetPoolPoint(nX, nY);
		m_openList.push_back(startNode);
		NodePoints*& refNode = m_mapGrids[nY * pdsMap->m_nWidth + nX];
		refNode = startNode;
		refNode->state = Node_InOpenList;

		while (m_openList.size() > 0) {
			NodePoints* curNode = m_openList.front();
			std::pop_heap(m_openList.begin(), m_openList.end(), [](const NodePoints* a, const NodePoints* b)->bool
				{
					return a->f() > b->f();
				});
			m_openList.pop_back();
			m_mapGrids[curNode->point.ny * pdsMap->m_nWidth + curNode->point.nx]->state = Node_InClosedList;
			//suc
			if (curNode->point.nx == ndX && curNode->point.ny == ndY) {
				listPath.clear();
				while (curNode->nFatherpoint) {
					listPath.push_back(curNode->point);
					curNode = curNode->nFatherpoint;
				}
				std::reverse(listPath.begin(), listPath.end());
				ULONGLONG cost = GetTickCount64() - tick;
				if (cost > 20) {
					g_logger.error("Astar Ñ°Â·³¬Ê± %I64d", cost);
				}
				break;
			}
			m_nearbyPoints.clear();
			findCanPassNodes(curNode);
			int cnt = 0;
			const int nodeNum = m_nearbyPoints.size();
			while (cnt < nodeNum) {
				NodePoints* nextNode = NULL;
				if (IsInOpenList(m_nearbyPoints[cnt].nx, m_nearbyPoints[cnt].ny, nextNode))
				{
					ProcessNodeInOpenList(curNode, nextNode);
				}
				else {
					nextNode = GetPoolPoint(m_nearbyPoints[cnt].nx, m_nearbyPoints[cnt].ny);
					ProcessNodeNotInOpenList(curNode, nextNode, ndX, ndY);
				}
				cnt++;
			}
		}
	}
}
