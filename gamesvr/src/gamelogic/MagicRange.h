#ifndef _MagicRange_h_
#define _MagicRange_h_

#include "define.h"
#include <vector>
#include <hash_map>
#include "zSingleton.h"

#define MAGICRANGE_HASH(range_type , dir) ( ((((range_type)+1) * 2)+51) + ((dir) % 2 ) )
#define MAX_RELATIVE_POS 40

enum emMapShowType{
	MAP_SHOW_CLOSE,
	MAP_SHOW_OPEN,
};

//位置关系
struct stRelativePos{
	SWORD x;
	SWORD y;

	BYTE t;	//伤害类型 忽视防御之类?
	WORD w; //伤害损耗
	BYTE s;	//是否显示效果，地面瞬时魔法用
	stRelativePos(){
		ZeroMemory(this,sizeof(stRelativePos));
	}
	bool getAbsolutePos(DWORD sx,DWORD sy ,BYTE btdir,DWORD& dx,DWORD& dy);
};

typedef std::vector< stRelativePos > MagicRange;

struct stMagicRanges {
	MagicRange lib;
	DWORD num;		//最大攻击人数
	stMagicRanges(){
		num=0;
	}
};

class CMagicRangeDefine:public Singleton< CMagicRangeDefine >{
	public:
		~CMagicRangeDefine(){
			final();
		}

		bool init();
		bool get(const DWORD range_type , const DWORD dir ,const stMagicRanges* &range);
		void final();
	private:
		typedef stdext::hash_map< DWORD, stMagicRanges* > MagicRangeContainer;
		MagicRangeContainer ranges;
};

#endif

