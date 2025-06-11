#include "Script.h"
#include "BaseCreature.h"
#include "UsrEngn.h"
#include <register_lua.hpp>
#include "cmd/ScriptSql_cmd.h"

int LuaGetPoint(const stAbilityPoint* pobj,int pos){
	return ((stAbilityPoint*)pobj)->GetPoint(pos);
}

bool LuaWritePoint(const stAbilityPoint* pobj,int pos,int point){
	if (pos>=0 && pos<_POINT_TYPE_NUM_){
		((stAbilityPoint*)pobj)->wAddAbilityPoint[pos]=point;
		return true;
	}
	return false;
}

void CScriptSystem::BindStruct(CLuaVM *luavm){
	sol::state_view lua(luavm->lua());

	registerClass<stInt>(lua, "stInt",
		"nint", &stInt::nInt,
		"aggro", & stInt::nAggro,
		"reducedamage", & stInt::nReduceDamage,
		"sealdamage", & stInt::nSealDamage,
		"addtype", & stInt::btAddType,
		"addtypetwo", & stInt::btAddTypeTwo
	);
	registerClass<stTimer>(lua, "stTimer",
		"time", &stTimer::m_time,
		"week", & stTimer::m_week,
		"day", & stTimer::m_day,
		"hour", & stTimer::m_hours,
		"min", & stTimer::m_min,
		"sec", & stTimer::m_sec,
		"interval", & stTimer::m_interval,
		"intervaltype", & stTimer::m_intervaltype,
		"keeptime", & stTimer::m_keeptime,
		"keeptype", & stTimer::m_keeptype
	);
	registerClass<stAbilityPoint>(lua, "stAbilityPoint",
		"getpoint", &LuaGetPoint,
		"writepoint", &LuaWritePoint,
		"wremainpoint", &stAbilityPoint::wRemainPoint
	);
	registerClass<stSkillLvl>(lua, "stSkillLvl",
		"dwskillexp", &stSkillLvl::dwexp,
		"level", & stSkillLvl::level,
		"originlv", & stSkillLvl::originLv,
		"extraLv", &stSkillLvl::extraLv
	);
	lua.new_usertype<stMagic>("stMagic",
		"savedata", &stMagic::savedata,
		"iscooling", &stMagic::isCooling,
		"getleftcd", &stMagic::getleftcd,
		"resetusetime", &stMagic::resetUseTime,
		"clearusetime", &stMagic::clearUseTime,
		"changeusetime", &stMagic::changeUseTime,
		"magicbase", sol::property(&stMagic::GetMagicDataBase),
		"getmaxlevel", &stMagic::getmaxlevel
	);
	registerClass<stEvtVar>(lua, "stEvtVar",
		"timercheck", &stEvtVar::timercheck,
		"timestatus", &stEvtVar::timestatus,
		"status", &stEvtVar::status,
		"settrack", &stEvtVar::settrack,
		"star", &stEvtVar::btStar
	);
	registerClass<stQuestDB>(lua, "stQuestDB",
		"qid", &stQuestDB::i64qid,
		"btquestsection", &stQuestDB::btquestsection,
		"btquesttype", &stQuestDB::btquesttype,
		"btquestsubtype", &stQuestDB::btquestsubtype,
		"beginnpcid", &stQuestDB::beginnpcid,
		"endnpcid", &stQuestDB::endnpcid,
		"btquesttargettype", &stQuestDB::btquesttargettype,
		"nminlv", &stQuestDB::nminlv,
		"nmaxlv", &stQuestDB::nmaxlv,
		"i64frontqid", &stQuestDB::i64frontqid,
		"nmapposx", &stQuestDB::nmapposx,
		"nmapposy", &stQuestDB::nmapposy,
		"ntimecheck", &stQuestDB::ntimecheck,
		"btitemchoose", &stQuestDB::btitemchoose,
		"loopcout", &stQuestDB::nloopcount
	);
	lua.new_usertype<stHumanRankInfo>("stHumanRankInfo",
		"onlyid", sol::readonly(&stHumanRankInfo::i64OnlyId),				//ΨһID		
		"dwlevel", sol::readonly(&stHumanRankInfo::dwLevel),				//�ȼ�
		"name", sol::readonly(&stHumanRankInfo::szName),						//��ɫ����
		"guildname", sol::readonly(&stHumanRankInfo::szGuildName),			//�л�����	  *
		"btisonline", sol::readonly(&stHumanRankInfo::btIsOnline),		//1���� 0������
		"btsex", sol::readonly(&stHumanRankInfo::btSex),				//�Ա�
		"zslevel", sol::readonly(&stHumanRankInfo::dwZSLevel),
		"job", sol::readonly(&stHumanRankInfo::btJob),
		"i64FightScore", sol::readonly(&stHumanRankInfo::i64FightScore),
		"militaryrank", sol::readonly(&stHumanRankInfo::btMilitaryRank)	//����
	);
	lua.new_usertype<stPetDetailInfo>("stPetDetailInfo",
		"mastername", sol::readonly(&stPetDetailInfo::szPetMonsterName),		//����������
		"nbodyprice", sol::readonly(&stPetDetailInfo::nBodyPrice),		//���
		"nnowrank", sol::readonly(&stPetDetailInfo::nNowRank),			//����
		"nlastrank", sol::readonly(&stPetDetailInfo::nLastRank)			//�ϴ�����
	);
	registerClass<stPetSvrData>(lua, "stPetSvrData",
		"nmonbaseid", &stPetSvrData::dwMonBaseID,	        //�������ID
		"nlevel", &stPetSvrData::dwLevel,		            //��ǰ�ȼ�
		"nnowexp", &stPetSvrData::nNowExp,			    //��ǰ����
		"nmaxexp", &stPetSvrData::nMaxExp,				//�����
		"nnowhp", &stPetSvrData::nNowHp,			        //��ǰhp
		"nnowmp", &stPetSvrData::nNowMp
	);
	registerClass<stNewHumHomeMapInfo>(lua, "NewHumHomeMapInfo",
		"mapid", &stNewHumHomeMapInfo::wmapid,
		"mapcountryid", &stNewHumHomeMapInfo::btmapcountryid,
		"mapsublineid", &stNewHumHomeMapInfo::btmapsublineid,
		"homex", &stNewHumHomeMapInfo::homex,
		"homey", &stNewHumHomeMapInfo::homey,
		"countryid", &stNewHumHomeMapInfo::btCountryID,
		"level", &stNewHumHomeMapInfo::nlevel,
		"job", &stNewHumHomeMapInfo::job,
		"sex", &stNewHumHomeMapInfo::sex
	);
	registerClass<stHumMapInfo>(lua, "HumMapInfo",
		"mapid", &stHumMapInfo::dwmapid,
		"mapcountryid", &stHumMapInfo::btmapcountryid,
		"mapsublineid", &stHumMapInfo::btmapsublineid,
		"curx", &stHumMapInfo::curx,
		"cury", &stHumMapInfo::cury,
		"countryid", &stHumMapInfo::btCountryID,
		"level", &stHumMapInfo::nlevel,
		"job", &stHumMapInfo::job,
		"sex", &stHumMapInfo::sex,
		"mapcloneid", &stHumMapInfo::wmapcloneid
	);
	lua.new_usertype<stBuff>("stBuff",
		"buffid", &stBuff::dwBuffID,
		"level", &stBuff::m_buffLevel,
		"timetype", sol::property(&stBuff::GetTimeType),
		"time", sol::property(&stBuff::GetTimeLeft),
		"keeptime", &stBuff::m_keepTime,
		"attakcrettype", & stBuff::m_AttakCretType,
		"attakcrettmpid", & stBuff::m_AttackCretTmpId
	);
	registerClass<stSuperSelectData>(lua, "stSuperSelectData",
		"supergetdata", &stSuperSelectData::SuperGetDate
	);
	registerClass<stGSGroupInfo>(lua, "stGSGroupInfo",
		"groupid", &stGSGroupInfo::dwGroupId,
		"bomaster", &stGSGroupInfo::boMaster,
		"count", &stGSGroupInfo::dwGroupMemberCount
	);
	registerClass<stItem>(lua, "stItem",
		"id", &stItem::dwBaseID,
		"count", &stItem::dwCount,
		"binding", &stItem::dwBinding,
		"btquality", &stItem::btQuality
	);
	registerClass<mydb_npcgen_tbl>(lua, "mydb_npcgen_tbl",
		"id", &mydb_npcgen_tbl::id,
		"mapid", &mydb_npcgen_tbl::mapId,
		"x", &mydb_npcgen_tbl::x,
		"y", &mydb_npcgen_tbl::y
	);
	// �� stAuctionItem �� Lua
	lua.new_usertype<stAuctionItem>("stAuctionItem",
		sol::constructors<stAuctionItem()>(),
		"onlyid", & stAuctionItem::onlyid,
		"id", &stAuctionItem::id,
		"num", &stAuctionItem::num,
		"bind", &stAuctionItem::bind,
		"base", &stAuctionItem::base,
		"inc", &stAuctionItem::inc,
		"fixed", &stAuctionItem::fixed,
		"state", &stAuctionItem::state,
		"count", &stAuctionItem::count,
		// ���ö���״̬
		"init", [](stAuctionItem& item) {
			item = stAuctionItem();
		}
	);

	// �� stAuction �� Lua
	lua.new_usertype<stAuction>("stAuction",
		sol::constructors<stAuction()>(),

		// ֱ��ӳ��������Lua �� 1 �� C++ �� 1
		sol::meta_function::index, [](stAuction& auction, int lua_idx) {
			size_t cpp_idx = tosizet(lua_idx);
			if (stAuction::IsValidID(cpp_idx))
				return &auction.indexs[cpp_idx];
			else {
				return static_cast<stAuctionItem*>(nullptr);
			}
		},

		// ����ֵ��֧��ͨ�� Lua ������ֵ��
		sol::meta_function::new_index, [](stAuction& auction, int lua_idx, sol::object value) {
			size_t cpp_idx = tosizet(lua_idx);
			if (stAuction::IsValidID(cpp_idx)) {
				// ���� AuctionItem ����ֵ
				if (value.is<stAuctionItem>()) {
					auction.indexs[cpp_idx].onlyid = value.as<stAuctionItem>().onlyid;
					auction.indexs[cpp_idx].id = value.as<stAuctionItem>().id;
					auction.indexs[cpp_idx].num = value.as<stAuctionItem>().num;
					auction.indexs[cpp_idx].bind = value.as<stAuctionItem>().bind;
					auction.indexs[cpp_idx].base = value.as<stAuctionItem>().base;
					auction.indexs[cpp_idx].inc = value.as<stAuctionItem>().inc;
					auction.indexs[cpp_idx].fixed = value.as<stAuctionItem>().fixed;
					auction.indexs[cpp_idx].state = value.as<stAuctionItem>().state;
					auction.indexs[cpp_idx].count = value.as<stAuctionItem>().count;
					return;
				}

				// �����ֵ
				if (value.is<sol::table>()) {
					sol::table table = value.as<sol::table>();

					// �ӱ�����ȡ�ֶ�ֵ
					sol::optional<int64_t> onlyid = table["onlyid"];
					sol::optional<DWORD> id = table["id"];
					sol::optional<BYTE> num = table["num"];
					sol::optional<BYTE> bind = table["bind"];
					sol::optional<BYTE> base = table["base"];
					sol::optional<BYTE> inc = table["inc"];
					sol::optional<BYTE> fixed = table["fixed"];
					sol::optional<BYTE> state = table["state"];
					sol::optional<BYTE> count = table["count"];

					// ��ֵ������
					if (onlyid) auction.indexs[cpp_idx].onlyid = *onlyid;
					if (id) auction.indexs[cpp_idx].id = *id;
					if (num) auction.indexs[cpp_idx].num = *num;
					if (bind) auction.indexs[cpp_idx].bind = *bind;
					if (base) auction.indexs[cpp_idx].base = *base;
					if (inc) auction.indexs[cpp_idx].inc = *inc;
					if (fixed) auction.indexs[cpp_idx].fixed = *fixed;
					if (state) auction.indexs[cpp_idx].state = *state;
					if (count) auction.indexs[cpp_idx].count = *count;
					return;
				}
			}
		},


		// ��ȡ������С��Lua �� # ��������
		sol::meta_function::length, [](stAuction& auction) {
			return auction.indexs.size() - 1;
		},

		// ��ʼ������Ԫ��
		"init", [](stAuction& auction) {
			auction = stAuction();
		},

		// �����ض� ID ������Ʒ
		"findItemById", [](stAuction& auction, short id) {
			for (size_t i = 1; i < auction.indexs.size(); ++i) {
				if (auction.indexs[i].id == id) {
					return &auction.indexs[i];
				}
			}
			return static_cast<stAuctionItem*>(nullptr);
		}
	);


}
