#include <register_lua.hpp>

#include "Script.h"
#include "BaseCreature.h"
#include "UsrEngn.h"
#include "Chat.h"
#include "MonsterObj.h"
#include <winapiini.h>
#include "Robot.h"

bool LuaGetSecondOK(const CPlayerObj* pobj){
	return ((CPlayerObj*)pobj)->GetSecondOK();
}

void LuaSetSecondOK(const CPlayerObj* pobj,bool boSet){
	((CPlayerObj*)pobj)->SetSecondOK(boSet);
}

void CScriptSystem::BindCreature(CLuaVM *luavm){
	sol::state_view lua(luavm->lua());
	RegisterArrayWrapper<int, static_cast<size_t>(AttrID::Count)>(lua, "AttrArray");
	RegisterArrayWrapper<int, static_cast<size_t>(ResID::count)>(lua, "ResArray");
	registerClass<MapObject>(lua,"MapObject",
		"objectId", &MapObject::m_objectId,
		"currx", &MapObject::m_nCurrX,
		"curry", &MapObject::m_nCurrY,
		"ismonster", &MapObject::isMonster,
		"isplayer", &MapObject::isPlayer,
		"toplayer", &CCreature::toPlayer,
		"isnpc", &MapObject::isNpc,
		"ispet", &MapObject::isPet,
		"topet", &CCreature::toPet,
		"isrobot", &MapObject::isRobot,
		"torobot", &MapObject::toRobot
	);
	lua.new_usertype<CCreature>("CCreature", sol::base_classes, sol::bases<MapObject>(),
		"attrs", sol::property([](CCreature& self)->std::array<int, static_cast<size_t>(AttrID::Count)>& {return sol::as_container(self.m_stAbility.attrs); }),
		"getname", sol::readonly_property(&CCreature::LuaGetName),					//名字
		"nnowhp", sol::readonly_property(&CCreature::m_nNowHP),
		"nnowmp", sol::readonly_property(&CCreature::m_nNowMP),
		"nnowpp", &CCreature::m_nNowPP,
		"var", &CCreature::m_vars,
		"tm", &CCreature::m_Timer,
		"buff", &CCreature::m_cBuff,
		"homemapid", &CCreature::m_wHomeMapID,
		"homex", &CCreature::m_nHomeX,
		"homey", &CCreature::m_nHomeY,
		"targetstate", &CCreature::m_btTargetState,
		"dir", &CCreature::m_btDirection,
		"countryid", sol::readonly_property([](const CPlayerObj& self) { return 0; }),
		"lv", &CCreature::m_dwLevel,
		"baseability", &CCreature::m_stBaseAbility,
		"ability", &CCreature::m_stAbility,
		"damagetype", &CCreature::m_btDamageType,
		"damageloss", &CCreature::m_wDamageLoss,
		"curtarget", &CCreature::m_curAttTarget,
		"bogmhide", &CCreature::m_boGmHide,
		"nocdmode", &CCreature::m_NoCdMode,
		"isTransMove", &CCreature::isTransMove,
		"lasthiter", &CCreature::GetLastHitter,
		"battlecamp", &CCreature::GetBattleCamp,
		"battlewudi", &CCreature::m_boBattleWuDi,
		"wudi", &CCreature::m_btWudi,
		"miaosha", &CCreature::m_btMiaosha,
		"allowspace", &CCreature::m_boAllowSpace,
		"curmap", &CCreature::GetEnvir,
		"statusvaluechange", &CCreature::StatusValueChange,
		"changepropertycheckplayer", &CCreature::ChangePropertyCheckPlayer,
		"findmagic", &CCreature::FindSkill,
		"MakeGhost", &CCreature::MakeGhost,
		"selfaddbuff", &CCreature::LuaSelfAddBuff,
		"selfaddbuff2", &CCreature::LuaSelfAddBuff2,
		"findbuff", &CCreature::LuaFindBuff,
		"getbuffbytype", &CCreature::LuaGetBuffByType,
		"removebuff", &CCreature::LuaRemoveBuff,
		"removebuffstate", &CCreature::LuaRemoveBuffState,
		"buffclear", &CCreature::LuaBuffClear,
		"getbuffstate", &CCreature::LuaGetBuffState,
		"featurechanged", &CCreature::LuaFeatureChanged,
		"updateappearance", &CCreature::LuaUpdateAppearance,
		"namechanged", &CCreature::NameChanged,
		"sendstruck",&CCreature::SendCretStruck,					//发送伤害扣篮包
		"issafezone",&CCreature::isSafeZone,						//是否安全区
		"isdie",&CCreature::isDie,
		"iscanvisit",&CCreature::isCanVisit,
		"isenemy",&CCreature::isEnemy,
		"relive",&CCreature::LuaRelive,
		"tomonster",&CCreature::toMonster,
		"struckdamage",&CCreature::StruckDamage,
		"die",&CCreature::Die,
		"selfcretaction",&CCreature::SelfCretAction,
		"triggerevent",&CCreature::TriggerEvent,
		"isskillrange",&CCreature::isSkillRange,
		"disappear",&CCreature::Disappear,
		"islittlemon", & CCreature::isLittleMon,
		"iselitemon",&CCreature::isEliteMon,
		"isboss",&CCreature::isBoss,
		"learnskill",&CCreature::learnSkill
	);
	{//CPlayerObj
		sol::usertype< CPlayerObj> luaPObj = lua.new_usertype<CPlayerObj>("PlayerObj", sol::base_classes, sol::bases<CCreature, MapObject>());
		luaPObj.set("res", sol::property([](CPlayerObj& self)->std::array<int, static_cast<size_t>(ResID::count)>& {return sol::as_container(self.m_res.res); }));
		luaPObj.set("gmattrs", sol::property([](CPlayerObj& self)->std::array<int, static_cast<size_t>(AttrID::Count)>& {return sol::as_container(self.m_stGmAbility.attrs); }));   //GM属性
		luaPObj.set("account", sol::readonly_property(&CPlayerObj::getAccount));														//玩家账号
		luaPObj.set("clientip", sol::readonly_property(&CPlayerObj::getClientIp));														//客户端登陆IP
		luaPObj.set("gateip", sol::readonly_property(&CPlayerObj::getGateIp));
		luaPObj.set("sex", sol::readonly_property([](const CPlayerObj& self) { return self.m_siFeature.sex; }));						//性别
		luaPObj.set("job", sol::readonly_property([](const CPlayerObj& self) { return self.m_siFeature.job; }));						//职业	
		luaPObj.set("guildid", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.dwGuildId; }));				//公会ID
		luaPObj.set("guildname", sol::readonly_property([](const CPlayerObj& self) { return GTU(self.m_GuildInfo.szGuildName); }));			//工会名字
		luaPObj.set("guildpower", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.dwPowerLevel; }));
		luaPObj.set("guildlv", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.dwGuildLevel; }));
		luaPObj.set("guildbuild", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.dwBuildDegree; }));
		luaPObj.set("guildemblem", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.dwEmblemId; }));
		luaPObj.set("guildrequest", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.IsExistAskJoin; }));
		luaPObj.set("guildmallitems", sol::readonly_property([](const CPlayerObj& self) {
			// 将数组转换为 vector，再用 sol::as_table 生成 Lua 表
			return sol::as_table(std::vector<UINT>(self.m_GuildInfo.szMallItemIndex,self.m_GuildInfo.szMallItemIndex + 20));}));			// 行会商城物品
		luaPObj.set("intime", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.tInTime; }));
		luaPObj.set("secondpass", sol::readonly_property([](const CPlayerObj& self) { return self.m_szSecondPassWord; }));				//二级密码
		luaPObj.set("secondmail", sol::readonly_property([](const CPlayerObj& self) { return self.m_szSecondMailAddress; }));			//二级邮箱
		luaPObj.set("maxexp", sol::readonly_property([](const CPlayerObj& self) { return self.m_i64MaxExp; }));
		luaPObj.set("legion", sol::readonly_property([](const CPlayerObj& self) { return GTU(self.m_szLegion); }));
		luaPObj.set("onlyid", sol::readonly_property(&CPlayerObj::m_i64UserOnlyID));
		luaPObj.set("dayvar", &CPlayerObj::m_dayVars);
		luaPObj.set("tishi", &CPlayerObj::m_tiShi);
		luaPObj.set("packet", sol::readonly(&CPlayerObj::m_Packet));					//人物包裹及仓库
		luaPObj.set("tubiao", &CPlayerObj::m_tuBiao);
		luaPObj.set("canchangename", &CPlayerObj::m_boCanChangeName);				//是否允许改名
		luaPObj.set("sellinetime", &CPlayerObj::m_dwSelLineTime);		//允许切线时间
		luaPObj.set("pkruntime", &CPlayerObj::m_dwPkRunTime);		//灰名时间
		luaPObj.set("viplv", &CPlayerObj::m_nVipLevel);
		luaPObj.set("addstorepage", &CPlayerObj::m_nAddStorePage);
		luaPObj.set("lastoperatetime", &CPlayerObj::m_dwLastOperateTime);
		luaPObj.set("feature", &CPlayerObj::m_siFeature);			//角色外观
		luaPObj.set("pkmodel", &CPlayerObj::m_btPkModel);				//PK模式
		luaPObj.set("playeronlinetime", &CPlayerObj::m_dwPlayerOnlineTime);
		luaPObj.set("petm", &CPlayerObj::m_Petm);					//宠物管理器
		luaPObj.set("headportarit", &CPlayerObj::m_dwHeadPortrait); //头像
		luaPObj.set("banchattime", &CPlayerObj::m_dwBanChatTime);
		luaPObj.set("bosstmpid", &CPlayerObj::m_dwBossTmpid);
		luaPObj.set("originalzone", &CPlayerObj::m_nOriginalZone);
		luaPObj.set("zoneid", sol::readonly(&CPlayerObj::m_dwOriginalZoneid));			//当前玩家区号
		luaPObj.set("srczoneid", sol::readonly(&CPlayerObj::m_dwSrcZoneId));			//跨服前玩家源服务器
		luaPObj.set("srctradeid", sol::readonly(&CPlayerObj::m_wSrcTrade));			//跨服前玩家平台
		luaPObj.set("currnpc", sol::readonly(&CPlayerObj::m_pVisitNPC));			//当前NPC
		luaPObj.set("ischangesvr", sol::readonly(&CPlayerObj::m_boIsChangeSvr));		//是否在切换服务器
		luaPObj.set("offlinetime", sol::readonly(&CPlayerObj::m_tLoginOuttime));		//上次下线时间
		luaPObj.set("logintime", sol::readonly(&CPlayerObj::m_dwLoginTime));		//本次登陆时间
		luaPObj.set("pcreatetime", sol::readonly(&CPlayerObj::m_dwPlayerCreateTime));	//玩家创建时间
		luaPObj.set("simplefeature", sol::readonly(&CPlayerObj::m_siFeature));
		luaPObj.set("gold", sol::readonly(&CPlayerObj::m_dwGold));					//金钱
		luaPObj.set("lastrmbtime", sol::readonly(&CPlayerObj::m_dwLastChargeCcyTime));//最后一次充值时间
		luaPObj.set("usermbhistroy", sol::readonly(&CPlayerObj::m_dwUseRmb));  //历史消费
		luaPObj.set("gmlvl", sol::readonly(&CPlayerObj::m_btGmLvl));			//GM等级
		luaPObj.set("leaderlvl", sol::readonly(&CPlayerObj::m_btLeader));			//引导员等级
		luaPObj.set("groupinfo", sol::readonly(&CPlayerObj::m_GroupInfo));
		luaPObj.set("friendlist", sol::readonly(&CPlayerObj::m_xFriendList));
		luaPObj.set("enemylist", sol::readonly(&CPlayerObj::m_xEnemyList));
		luaPObj.set("blocklist", sol::readonly(&CPlayerObj::m_xBlockList));
		luaPObj.set("jyrate", sol::readonly(&CPlayerObj::m_dwJyRate));
		luaPObj.set("issitchsvr", sol::readonly(&CPlayerObj::isSwitchSvr));
		luaPObj.set("srctruezoneid", sol::readonly(&CPlayerObj::m_dwSrcTrueZoneId));			//玩家原区id
		luaPObj.set("iability", sol::readonly(&CPlayerObj::m_siAbility));		//创角属性

		luaPObj.set_function("damagespell", &CPlayerObj::DamageSpell);
		luaPObj.set_function("isclientready", &CPlayerObj::isClientReady);				//客户端准备好了
		luaPObj.set_function("isguildwar", &CPlayerObj::isGuildWar);						//是否在行会战中
		luaPObj.set_function("equipisok", &CPlayerObj::EquipIsOk);						//检查人物身上是否装备了这个物品
		luaPObj.set_function("packetisok", &CPlayerObj::PacketItemIsOk);					//检查人物包裹中是否有这个物品，和物品数量是否满足
		luaPObj.set_function("packetisokwithbind", &CPlayerObj::PacketItemIsOkWithBindSta);
		luaPObj.set_function("groupisok", &CPlayerObj::GroupIsOk);						//检查是否组队
		luaPObj.set_function("MoveToMap", &CPlayerObj::MoveToMap);							
		luaPObj.set_function("LocalMapTransfer", &CPlayerObj::LocalMapTransfer);
		luaPObj.set_function("TransferToMap", &CPlayerObj::TransferToMap);
		luaPObj.set_function("TransferToMapId", &CPlayerObj::TransferToMapId);
		luaPObj.set_function("TransferMap", &CPlayerObj::TransferMap);
		luaPObj.set_function("goldchange", &CPlayerObj::GoldChanged);						//加减金币,正负
		luaPObj.set_function("reschange", &CPlayerObj::ResChange);						//资源改变
		luaPObj.set_function("setrmbgoldlog", &CPlayerObj::SetRmbGoldLog);				//元宝日志
		luaPObj.set_function("pkchange", &CPlayerObj::PkChange);							//加减PK值,正负
		luaPObj.set_function("itemdelbybaseid", &CPlayerObj::ItemDeleteByBaseIdIsOk);		//删除人物身上物品，ID，数量
		luaPObj.set_function("itemdelbyid", &CPlayerObj::ItemDeleteIDIsOk);				//删除人物身上物品，唯一ID
		luaPObj.set_function("itemdelinbody", &CPlayerObj::ItemDeleteInBody);				//删除人物身上物品
		luaPObj.set_function("itemdelinbag", &CPlayerObj::ItemDeleteInBag);				//删除包裹中的物品  如果nCount= -1 则按物品数据的数量操作
		luaPObj.set_function("itemdelinall", &CPlayerObj::ItemDeleteInAll);				//删除玩家所有位置的这个物品 如果nCount= -1 则按物品数据的数量操作
		luaPObj.set_function("getnpcid", &CPlayerObj::GetVisitNpcId);						//得到当前NPCID
		luaPObj.set_function("getnpc", &CPlayerObj::GetVisitNpc);						    //得到当前NPC
		luaPObj.set_function("getitemname", &CPlayerObj::GetLuaItemName);					//得到此ID物品的名字
		luaPObj.set_function("checklattice", &CPlayerObj::CheckLattice);					//得到包裹空格
		luaPObj.set_function("curritem", &CPlayerObj::GetCurrItem);						//当前使用的任务物品
		luaPObj.set_function("getequipitem", &CPlayerObj::GetEquipItem);					//根据唯一ID得到装备物品
		luaPObj.set_function("createluaitem", &CPlayerObj::CreateLuaItem);				//创建脚本所需物品
		luaPObj.set_function("sendtobag", &CPlayerObj::SendToBag);						//发送物品指针到包裹
		luaPObj.set_function("sendtobagnodel", &CPlayerObj::SendToBagNoDel);				//发送物品到包裹失败不删除
		luaPObj.set_function("getgroupplayer", &CPlayerObj::GetGroupPlayer);				//得到组队玩家
		luaPObj.set_function("getposequip", &CPlayerObj::GetPosEquip);					//得到该位置的装备物品
		luaPObj.set_function("senditem", &CPlayerObj::SendItem);							//刷新物品
		luaPObj.set_function("studyskill", &CPlayerObj::StudySkill);						//学习技能
		luaPObj.set_function("deleteskill", &CPlayerObj::DeleteSkill);					//删除技能
		luaPObj.set_function("setvisitnpc", &CPlayerObj::SetVisitNpc);					//设置访问NPC
		luaPObj.set_function("wearitem", &CPlayerObj::WearItem);							//佩戴装备
		luaPObj.set_function("takeoffitem", &CPlayerObj::TakeOffItem);					//摘下装备
		luaPObj.set_function("changename", &CPlayerObj::ChangeName);						//改变名字
		luaPObj.set_function("changenamefinal", &CPlayerObj::ChangeNameFinal);						//改变名字
		luaPObj.set_function("addrelation", &CPlayerObj::AddRelation);					//添加好友关系
		luaPObj.set_function("setsecondpass", &CPlayerObj::LuaSetSecondPass);				//设置二级密码
		luaPObj.set_function("setsecondmail", &CPlayerObj::LuaSetSecondMailAddress);		//设置二级邮箱
		luaPObj.set_function("senditemtoclient", &CPlayerObj::SendItemToClient);			//发送物品数据给客户端
		luaPObj.set_function("getcellcount", &CPlayerObj::GetFreeCellCount);				//获得包裹剩下的所有空格		
		luaPObj.set_function("checkvisitnpc", &CPlayerObj::CheckVisitNpc);				//检查访问NPC是否在范围内
		luaPObj.set_function("guildwarkill", &CPlayerObj::GuildWarKill);					//公会战杀人
		luaPObj.set_function("guildwarcheck", &CPlayerObj::GuildWarCheck);				//公会战检查,目标是杀人者
		luaPObj.set_function("changesex", &CPlayerObj::ChangeSex);						//改变性别
		luaPObj.set_function("changejob", &CPlayerObj::ChangeJob);						//改变职业
		luaPObj.set_function("getrelationsize", &CPlayerObj::GetRelationSize);			//获得关系列表中的数量
		luaPObj.set_function("getmsgfromtxserver", &CPlayerObj::getMsgFromTxServer);		//获取腾迅的数据
		luaPObj.set_function("updatetosuper", &CPlayerObj::UpdateToSuperSvr);				//脚本主动保持数据
		luaPObj.set_function("playerdeathdropped", &CPlayerObj::PlayDeathDropped);			//死亡掉落
		luaPObj.set_function("call", &CPlayerObj::LuaCallLuaByFuncName);					//lua通过脚本方法string名调用
		luaPObj.set_function("sendtipmsg", &CPlayerObj::LuaSendTipMsg);
		luaPObj.set_function("getmapothergroupmemeber", &CPlayerObj::LuaGetMapOtherGroupMember);		// 获得同一地图内的其他队伍玩家
		luaPObj.set_function("getbaseproperty", &CPlayerObj::GetBaseProperty);
		luaPObj.set_function("ranktoptosuper", &CPlayerObj::RankTopToSuper);
		luaPObj.set_function("senddropitem", &CPlayerObj::SendDropItem);
		luaPObj.set_function("additemlimittime", &CPlayerObj::AddItemLimitTime);//人物身上/包裹/仓库物品增加限时时间
		luaPObj.set_function("updatetoglobalsvr", &CPlayerObj::UpdateToGlobalSvr);
		luaPObj.set_function("updatemembername", &CPlayerObj::UpdateMemberName);
		luaPObj.set_function("luasetpetstate", &CPlayerObj::LuaSetPetState);
		luaPObj.set_function("lualoadallpets", &CPlayerObj::LuaLoadAllPets);
		luaPObj.set_function("getmagicpet", &CPlayerObj::GetMagicPet);
		luaPObj.set_function("xfxdecode", &CPlayerObj::xfxDecode);
		luaPObj.set_function("getplatformtype", &CPlayerObj::getPlatFormType);
		luaPObj.set_function("addluaeffid", &CPlayerObj::addLuaEffid);
		luaPObj.set_function("addspecialluaeffid", &CPlayerObj::addSpecialLuaEffid);
		luaPObj.set_function("addcofluaeffid", &CPlayerObj::addCofLuaEffid);
		luaPObj.set_function("luaplayerattack", &CPlayerObj::luaPlayerAttack);
		luaPObj.set_function("changepkmode", &CPlayerObj::ChangePkMode);

		luaPObj.set_function("updateplayerinfo", &CPlayerObj::UpdatePlayerInfo);
		luaPObj.set_function("clientBundleId", &CPlayerObj::getClientBundleId);
		luaPObj.set_function("clientVersion", &CPlayerObj::getClientVersion);
		luaPObj.set_function("clientPlatform", &CPlayerObj::getClientPlatform);
		luaPObj.set_function("subPlatform", &CPlayerObj::getSubPlatform);
		luaPObj.set_function("sendspecialringskillcd", &CPlayerObj::SendSpecialRingSkillCd);
		luaPObj.set_function("drop", &CPlayerObj::DropTest);
		luaPObj.set_function("sendwechatinfo", &CPlayerObj::SendWeChatInfo);
		luaPObj.set_function("sendplayerproperty", &CPlayerObj::SendPlayerProperty);
		luaPObj.set_function("getshortcut", &CPlayerObj::GetShortCuts);
		luaPObj.set_function("addshortcut", &CPlayerObj::AddShortCuts);
		luaPObj.set_function("changeproperty", &CPlayerObj::LuaChangeProperty);			// 直接刷新属性
		luaPObj.set("bosecondpassok", sol::property(&LuaGetSecondOK, &LuaSetSecondOK));
		luaPObj.set("isfighting", sol::property(&CPlayerObj::IsFighting));
		luaPObj.set("clearfight", &CPlayerObj::ClearFighting);
		luaPObj.set("ispking", sol::property(&CPlayerObj::IsPking));
		luaPObj.set("clearpk", &CPlayerObj::ClearPking);
		luaPObj.set("setheadportrait", &CPlayerObj::SetHeadPortrait);
	}
	{//CMonster
		sol::usertype< CMonster> luaMon = lua.new_usertype<CMonster>("CMonster", sol::base_classes, sol::bases<CCreature, MapObject>());
		luaMon.set("baseid", sol::readonly_property([](CMonster& self) {return self.GetMonsterDataBase()->nID; }));
		luaMon.set("showname", sol::readonly_property(&CMonster::m_szMonsterShowName));				//怪物显示名字
		luaMon.set("monid", sol::property([](CMonster& self) { return self.GetMonsterDataBase()->nID; }));	//怪物id
		luaMon.set("masterid", sol::readonly_property([](CMonster& self) { return self.getOwnerId(); }));	//怪物归属玩家id
		luaMon.set("moninfo", sol::property([](CMonster& self) {return self.GetMonsterDataBase(); }));//怪物数据库数值							
		luaMon.set("noautoatt", sol::property(&CMonster::GetNoAutoAtt, &CMonster::SetNoAutoAtt));	//是否主动攻击
		luaMon.set("cannotmove", sol::property(&CMonster::GetCanNotMove, &CMonster::SetCanNotMove));	//是否移动
		luaMon.set("nostruckatt", sol::property(&CMonster::GetNoStruckAtt, &CMonster::SetNoStruckAtt));	//是否还击
		luaMon.set("bttype", sol::property([](CMonster& self) { return (uint8_t)self.m_btType; }));			//怪物类型
		luaMon.set("diffcof", &CMonster::m_dwDifficultyCof);			//怪物难度系数
		luaMon.set_function("lvup", &CMonster::LevelUp);								    //怪物升级
		luaMon.set_function("addexp", &CMonster::AddExp);                                //增加经验

		luaMon.set("btzhongjilvl", &CMonster::m_btZhongJiLvl);
		luaMon.set("nguildid", &CMonster::m_guildId);
	}
	{//CRobot
		sol::usertype< CRobot> luaRobot = lua.new_usertype<CRobot>("CRobot", sol::base_classes, sol::bases<CCreature, CMonster, MapObject>());
		luaRobot.set("packet", sol::readonly(&CRobot::m_Packet));					//人物包裹及仓库
		luaRobot.set("feature", &CRobot::m_siFeature);			//角色外观
		luaRobot.set("sex", sol::readonly_property([](const CRobot& self) { return self.m_siFeature.sex; }));						//性别
		luaRobot.set("job", sol::readonly_property([](const CRobot& self) { return self.m_siFeature.job; }));						//职业	
	}
}