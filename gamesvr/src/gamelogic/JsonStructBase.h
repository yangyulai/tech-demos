#pragma once
#include <iguana/iguana.hpp>

struct stDropItemBase {
	int id = 0; //道具id
	int tr = 0; //掉落包
	float ra = 0; //概率
	int co = 0; //数量
	int job = 0; //职业
};
#if __cplusplus < 202002L
YLT_REFL(stDropItemBase, id, tr, ra, co, job)
#endif