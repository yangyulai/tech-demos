#pragma once
#include <iguana/iguana.hpp>

struct stDropItemBase {
	int id = 0; //����id
	int tr = 0; //�����
	float ra = 0; //����
	int co = 0; //����
	int job = 0; //ְҵ
};
#if __cplusplus < 202002L
YLT_REFL(stDropItemBase, id, tr, ra, co, job)
#endif