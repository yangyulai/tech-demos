
#include <vector>
#include "zXMLParser.h"
#include "MagicRange.h"
#include "GameMap.h"

#define MAGICRANGEFILE		"MagicRange.xml"

bool stRelativePos::getAbsolutePos(DWORD sx,DWORD sy ,BYTE btdir,DWORD& dx,DWORD& dy){
	FUNCTION_BEGIN;
	if (btdir>DRI_NUM){ dx=sx;dy=sy;return false; }
	dx=sx;
	dy=sy;
	eGameDir dir=DR_NULL;
	if (btdir>7){
		dir=DR_MID;
	}else{
		dir=gameDirs[btdir];
	}
	switch (dir)
	{
	case DR_MID:
		{
			dx =sx+ x;
			dy =sy+ y;
		}break;
			
	case DR_DOWN:case DR_UP:
	{
		if (dir & DR_UP){
			dy =sy- y;dx =sx- x;
		}else if (dir & DR_DOWN){
			dy =sy+ y;dx =sx+ x;
		}
	}break;
	case DR_LEFT:case DR_RIGHT:
	{
		if (dir & DR_LEFT){
			dx =sx- y;dy =sy- x;
		}else if (dir & DR_RIGHT){
			dx =sx+ y;dy =sy+ x;
		}
	}break;
	case DR_UPRIGHT:case DR_UPLEFT:case DR_DOWNRIGHT:case DR_DOWNLEFT:
		{
			if (dir & DR_UP){
				dy =sy- y;
			}else if (dir & DR_DOWN){
				dy =sy+ y;
			}
			if (dir & DR_LEFT){
				dx =sx- x;
			}else if (dir & DR_RIGHT){
				dx =sx+ x;
			}
		}break;
	}
	return true;
}

bool CMagicRangeDefine::init(){
	FUNCTION_BEGIN;
	final();

 	zXMLParser xml;
 	if (!xml.initFile(MAGICRANGEFILE)){
 		g_logger.error("¼ÓÔØ¹¥»÷·¶Î§ÅäÖÃÎÄ¼þ " MAGICRANGEFILE" Ê§°Ü");
 		return false;
 	}

	xmlNodePtr root = xml.getRootNode("defineranges");
	if (root){
		xmlNodePtr node = xml.getChildNode(root, "range");
		while(node){
			if (stricmp(node->Value(), "range") == 0){
				DWORD type;
				DWORD dir;
				DWORD num;
				stMagicRanges* range_vector=CLD_DEBUG_NEW stMagicRanges;
				if (range_vector==NULL){
					g_logger.error("¼ÓÔØ¹¥»÷·¶Î§ÅäÖÃÎÄ¼þ " MAGICRANGEFILE" Ê§°Ü,ÄÚ´æ²»×ã!");
					return false;
				}
				range_vector->num = 0;
				xml.getNodePropNum(node, "range_type", type);
				xml.getNodePropNum(node, "dir_type", dir);
				xml.getNodePropNum(node, "max_num", num);
				range_vector->num = num;
				xmlNodePtr subnode = xml.getChildNode(node , "pos");
				while(subnode){
					if(stricmp(subnode->Value() , "pos") == 0){
						stRelativePos pos;
						xml.getNodePropNum(subnode , "x" , pos.x );
						xml.getNodePropNum(subnode , "y" , pos.y );
						xml.getNodePropNum(subnode , "t" , pos.t );
						xml.getNodePropNum(subnode , "w" , pos.w );
						xml.getNodePropNum(subnode , "s" , pos.s );
						range_vector->lib.push_back(pos);
					}
					subnode = xml.getNextNode(subnode, NULL);
				}
				ranges.insert(MagicRangeContainer::value_type(MAGICRANGE_HASH(type , dir) , range_vector));
			}
			node = xml.getNextNode(node, NULL);
		}
		g_logger.info("³õÊ¼»¯¹¥»÷·¶Î§³É¹¦");
		return true;
	}

 	g_logger.error( "¼ÓÔØ¹¥»÷·¶Î§ÅäÖÃÎÄ¼þ " MAGICRANGEFILE" Ê§°Ü,defineranges Ã»ÓÐÕÒµ½!" );
 	return false;
}

bool CMagicRangeDefine::get(const DWORD range_type , const DWORD dir ,const stMagicRanges* &range){
	FUNCTION_BEGIN;
	MagicRangeContainer::const_iterator it = ranges.find(MAGICRANGE_HASH(range_type , dir));
	if(it != ranges.end() && it->second){
		range = it->second;
		return true;
	}
	range=NULL;
	return false;
}

void CMagicRangeDefine::final(){
	FUNCTION_BEGIN;
	clearpmap2(ranges);
}
