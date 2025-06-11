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
		"getname", sol::readonly_property(&CCreature::LuaGetName),					//����
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
		"sendstruck",&CCreature::SendCretStruck,					//�����˺�������
		"issafezone",&CCreature::isSafeZone,						//�Ƿ�ȫ��
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
		luaPObj.set("gmattrs", sol::property([](CPlayerObj& self)->std::array<int, static_cast<size_t>(AttrID::Count)>& {return sol::as_container(self.m_stGmAbility.attrs); }));   //GM����
		luaPObj.set("account", sol::readonly_property(&CPlayerObj::getAccount));														//����˺�
		luaPObj.set("clientip", sol::readonly_property(&CPlayerObj::getClientIp));														//�ͻ��˵�½IP
		luaPObj.set("gateip", sol::readonly_property(&CPlayerObj::getGateIp));
		luaPObj.set("sex", sol::readonly_property([](const CPlayerObj& self) { return self.m_siFeature.sex; }));						//�Ա�
		luaPObj.set("job", sol::readonly_property([](const CPlayerObj& self) { return self.m_siFeature.job; }));						//ְҵ	
		luaPObj.set("guildid", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.dwGuildId; }));				//����ID
		luaPObj.set("guildname", sol::readonly_property([](const CPlayerObj& self) { return GTU(self.m_GuildInfo.szGuildName); }));			//��������
		luaPObj.set("guildpower", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.dwPowerLevel; }));
		luaPObj.set("guildlv", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.dwGuildLevel; }));
		luaPObj.set("guildbuild", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.dwBuildDegree; }));
		luaPObj.set("guildemblem", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.dwEmblemId; }));
		luaPObj.set("guildrequest", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.IsExistAskJoin; }));
		luaPObj.set("guildmallitems", sol::readonly_property([](const CPlayerObj& self) {
			// ������ת��Ϊ vector������ sol::as_table ���� Lua ��
			return sol::as_table(std::vector<UINT>(self.m_GuildInfo.szMallItemIndex,self.m_GuildInfo.szMallItemIndex + 20));}));			// �л��̳���Ʒ
		luaPObj.set("intime", sol::readonly_property([](const CPlayerObj& self) { return self.m_GuildInfo.tInTime; }));
		luaPObj.set("secondpass", sol::readonly_property([](const CPlayerObj& self) { return self.m_szSecondPassWord; }));				//��������
		luaPObj.set("secondmail", sol::readonly_property([](const CPlayerObj& self) { return self.m_szSecondMailAddress; }));			//��������
		luaPObj.set("maxexp", sol::readonly_property([](const CPlayerObj& self) { return self.m_i64MaxExp; }));
		luaPObj.set("legion", sol::readonly_property([](const CPlayerObj& self) { return GTU(self.m_szLegion); }));
		luaPObj.set("onlyid", sol::readonly_property(&CPlayerObj::m_i64UserOnlyID));
		luaPObj.set("dayvar", &CPlayerObj::m_dayVars);
		luaPObj.set("tishi", &CPlayerObj::m_tiShi);
		luaPObj.set("packet", sol::readonly(&CPlayerObj::m_Packet));					//����������ֿ�
		luaPObj.set("tubiao", &CPlayerObj::m_tuBiao);
		luaPObj.set("canchangename", &CPlayerObj::m_boCanChangeName);				//�Ƿ��������
		luaPObj.set("sellinetime", &CPlayerObj::m_dwSelLineTime);		//��������ʱ��
		luaPObj.set("pkruntime", &CPlayerObj::m_dwPkRunTime);		//����ʱ��
		luaPObj.set("viplv", &CPlayerObj::m_nVipLevel);
		luaPObj.set("addstorepage", &CPlayerObj::m_nAddStorePage);
		luaPObj.set("lastoperatetime", &CPlayerObj::m_dwLastOperateTime);
		luaPObj.set("feature", &CPlayerObj::m_siFeature);			//��ɫ���
		luaPObj.set("pkmodel", &CPlayerObj::m_btPkModel);				//PKģʽ
		luaPObj.set("playeronlinetime", &CPlayerObj::m_dwPlayerOnlineTime);
		luaPObj.set("petm", &CPlayerObj::m_Petm);					//���������
		luaPObj.set("headportarit", &CPlayerObj::m_dwHeadPortrait); //ͷ��
		luaPObj.set("banchattime", &CPlayerObj::m_dwBanChatTime);
		luaPObj.set("bosstmpid", &CPlayerObj::m_dwBossTmpid);
		luaPObj.set("originalzone", &CPlayerObj::m_nOriginalZone);
		luaPObj.set("zoneid", sol::readonly(&CPlayerObj::m_dwOriginalZoneid));			//��ǰ�������
		luaPObj.set("srczoneid", sol::readonly(&CPlayerObj::m_dwSrcZoneId));			//���ǰ���Դ������
		luaPObj.set("srctradeid", sol::readonly(&CPlayerObj::m_wSrcTrade));			//���ǰ���ƽ̨
		luaPObj.set("currnpc", sol::readonly(&CPlayerObj::m_pVisitNPC));			//��ǰNPC
		luaPObj.set("ischangesvr", sol::readonly(&CPlayerObj::m_boIsChangeSvr));		//�Ƿ����л�������
		luaPObj.set("offlinetime", sol::readonly(&CPlayerObj::m_tLoginOuttime));		//�ϴ�����ʱ��
		luaPObj.set("logintime", sol::readonly(&CPlayerObj::m_dwLoginTime));		//���ε�½ʱ��
		luaPObj.set("pcreatetime", sol::readonly(&CPlayerObj::m_dwPlayerCreateTime));	//��Ҵ���ʱ��
		luaPObj.set("simplefeature", sol::readonly(&CPlayerObj::m_siFeature));
		luaPObj.set("gold", sol::readonly(&CPlayerObj::m_dwGold));					//��Ǯ
		luaPObj.set("lastrmbtime", sol::readonly(&CPlayerObj::m_dwLastChargeCcyTime));//���һ�γ�ֵʱ��
		luaPObj.set("usermbhistroy", sol::readonly(&CPlayerObj::m_dwUseRmb));  //��ʷ����
		luaPObj.set("gmlvl", sol::readonly(&CPlayerObj::m_btGmLvl));			//GM�ȼ�
		luaPObj.set("leaderlvl", sol::readonly(&CPlayerObj::m_btLeader));			//����Ա�ȼ�
		luaPObj.set("groupinfo", sol::readonly(&CPlayerObj::m_GroupInfo));
		luaPObj.set("friendlist", sol::readonly(&CPlayerObj::m_xFriendList));
		luaPObj.set("enemylist", sol::readonly(&CPlayerObj::m_xEnemyList));
		luaPObj.set("blocklist", sol::readonly(&CPlayerObj::m_xBlockList));
		luaPObj.set("jyrate", sol::readonly(&CPlayerObj::m_dwJyRate));
		luaPObj.set("issitchsvr", sol::readonly(&CPlayerObj::isSwitchSvr));
		luaPObj.set("srctruezoneid", sol::readonly(&CPlayerObj::m_dwSrcTrueZoneId));			//���ԭ��id
		luaPObj.set("iability", sol::readonly(&CPlayerObj::m_siAbility));		//��������

		luaPObj.set_function("damagespell", &CPlayerObj::DamageSpell);
		luaPObj.set_function("isclientready", &CPlayerObj::isClientReady);				//�ͻ���׼������
		luaPObj.set_function("isguildwar", &CPlayerObj::isGuildWar);						//�Ƿ����л�ս��
		luaPObj.set_function("equipisok", &CPlayerObj::EquipIsOk);						//������������Ƿ�װ���������Ʒ
		luaPObj.set_function("packetisok", &CPlayerObj::PacketItemIsOk);					//�������������Ƿ��������Ʒ������Ʒ�����Ƿ�����
		luaPObj.set_function("packetisokwithbind", &CPlayerObj::PacketItemIsOkWithBindSta);
		luaPObj.set_function("groupisok", &CPlayerObj::GroupIsOk);						//����Ƿ����
		luaPObj.set_function("MoveToMap", &CPlayerObj::MoveToMap);							
		luaPObj.set_function("LocalMapTransfer", &CPlayerObj::LocalMapTransfer);
		luaPObj.set_function("TransferToMap", &CPlayerObj::TransferToMap);
		luaPObj.set_function("TransferToMapId", &CPlayerObj::TransferToMapId);
		luaPObj.set_function("TransferMap", &CPlayerObj::TransferMap);
		luaPObj.set_function("goldchange", &CPlayerObj::GoldChanged);						//�Ӽ����,����
		luaPObj.set_function("reschange", &CPlayerObj::ResChange);						//��Դ�ı�
		luaPObj.set_function("setrmbgoldlog", &CPlayerObj::SetRmbGoldLog);				//Ԫ����־
		luaPObj.set_function("pkchange", &CPlayerObj::PkChange);							//�Ӽ�PKֵ,����
		luaPObj.set_function("itemdelbybaseid", &CPlayerObj::ItemDeleteByBaseIdIsOk);		//ɾ������������Ʒ��ID������
		luaPObj.set_function("itemdelbyid", &CPlayerObj::ItemDeleteIDIsOk);				//ɾ������������Ʒ��ΨһID
		luaPObj.set_function("itemdelinbody", &CPlayerObj::ItemDeleteInBody);				//ɾ������������Ʒ
		luaPObj.set_function("itemdelinbag", &CPlayerObj::ItemDeleteInBag);				//ɾ�������е���Ʒ  ���nCount= -1 ����Ʒ���ݵ���������
		luaPObj.set_function("itemdelinall", &CPlayerObj::ItemDeleteInAll);				//ɾ���������λ�õ������Ʒ ���nCount= -1 ����Ʒ���ݵ���������
		luaPObj.set_function("getnpcid", &CPlayerObj::GetVisitNpcId);						//�õ���ǰNPCID
		luaPObj.set_function("getnpc", &CPlayerObj::GetVisitNpc);						    //�õ���ǰNPC
		luaPObj.set_function("getitemname", &CPlayerObj::GetLuaItemName);					//�õ���ID��Ʒ������
		luaPObj.set_function("checklattice", &CPlayerObj::CheckLattice);					//�õ������ո�
		luaPObj.set_function("curritem", &CPlayerObj::GetCurrItem);						//��ǰʹ�õ�������Ʒ
		luaPObj.set_function("getequipitem", &CPlayerObj::GetEquipItem);					//����ΨһID�õ�װ����Ʒ
		luaPObj.set_function("createluaitem", &CPlayerObj::CreateLuaItem);				//�����ű�������Ʒ
		luaPObj.set_function("sendtobag", &CPlayerObj::SendToBag);						//������Ʒָ�뵽����
		luaPObj.set_function("sendtobagnodel", &CPlayerObj::SendToBagNoDel);				//������Ʒ������ʧ�ܲ�ɾ��
		luaPObj.set_function("getgroupplayer", &CPlayerObj::GetGroupPlayer);				//�õ�������
		luaPObj.set_function("getposequip", &CPlayerObj::GetPosEquip);					//�õ���λ�õ�װ����Ʒ
		luaPObj.set_function("senditem", &CPlayerObj::SendItem);							//ˢ����Ʒ
		luaPObj.set_function("studyskill", &CPlayerObj::StudySkill);						//ѧϰ����
		luaPObj.set_function("deleteskill", &CPlayerObj::DeleteSkill);					//ɾ������
		luaPObj.set_function("setvisitnpc", &CPlayerObj::SetVisitNpc);					//���÷���NPC
		luaPObj.set_function("wearitem", &CPlayerObj::WearItem);							//���װ��
		luaPObj.set_function("takeoffitem", &CPlayerObj::TakeOffItem);					//ժ��װ��
		luaPObj.set_function("changename", &CPlayerObj::ChangeName);						//�ı�����
		luaPObj.set_function("changenamefinal", &CPlayerObj::ChangeNameFinal);						//�ı�����
		luaPObj.set_function("addrelation", &CPlayerObj::AddRelation);					//��Ӻ��ѹ�ϵ
		luaPObj.set_function("setsecondpass", &CPlayerObj::LuaSetSecondPass);				//���ö�������
		luaPObj.set_function("setsecondmail", &CPlayerObj::LuaSetSecondMailAddress);		//���ö�������
		luaPObj.set_function("senditemtoclient", &CPlayerObj::SendItemToClient);			//������Ʒ���ݸ��ͻ���
		luaPObj.set_function("getcellcount", &CPlayerObj::GetFreeCellCount);				//��ð���ʣ�µ����пո�		
		luaPObj.set_function("checkvisitnpc", &CPlayerObj::CheckVisitNpc);				//������NPC�Ƿ��ڷ�Χ��
		luaPObj.set_function("guildwarkill", &CPlayerObj::GuildWarKill);					//����սɱ��
		luaPObj.set_function("guildwarcheck", &CPlayerObj::GuildWarCheck);				//����ս���,Ŀ����ɱ����
		luaPObj.set_function("changesex", &CPlayerObj::ChangeSex);						//�ı��Ա�
		luaPObj.set_function("changejob", &CPlayerObj::ChangeJob);						//�ı�ְҵ
		luaPObj.set_function("getrelationsize", &CPlayerObj::GetRelationSize);			//��ù�ϵ�б��е�����
		luaPObj.set_function("getmsgfromtxserver", &CPlayerObj::getMsgFromTxServer);		//��ȡ��Ѹ������
		luaPObj.set_function("updatetosuper", &CPlayerObj::UpdateToSuperSvr);				//�ű�������������
		luaPObj.set_function("playerdeathdropped", &CPlayerObj::PlayDeathDropped);			//��������
		luaPObj.set_function("call", &CPlayerObj::LuaCallLuaByFuncName);					//luaͨ���ű�����string������
		luaPObj.set_function("sendtipmsg", &CPlayerObj::LuaSendTipMsg);
		luaPObj.set_function("getmapothergroupmemeber", &CPlayerObj::LuaGetMapOtherGroupMember);		// ���ͬһ��ͼ�ڵ������������
		luaPObj.set_function("getbaseproperty", &CPlayerObj::GetBaseProperty);
		luaPObj.set_function("ranktoptosuper", &CPlayerObj::RankTopToSuper);
		luaPObj.set_function("senddropitem", &CPlayerObj::SendDropItem);
		luaPObj.set_function("additemlimittime", &CPlayerObj::AddItemLimitTime);//��������/����/�ֿ���Ʒ������ʱʱ��
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
		luaPObj.set_function("changeproperty", &CPlayerObj::LuaChangeProperty);			// ֱ��ˢ������
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
		luaMon.set("showname", sol::readonly_property(&CMonster::m_szMonsterShowName));				//������ʾ����
		luaMon.set("monid", sol::property([](CMonster& self) { return self.GetMonsterDataBase()->nID; }));	//����id
		luaMon.set("masterid", sol::readonly_property([](CMonster& self) { return self.getOwnerId(); }));	//����������id
		luaMon.set("moninfo", sol::property([](CMonster& self) {return self.GetMonsterDataBase(); }));//�������ݿ���ֵ							
		luaMon.set("noautoatt", sol::property(&CMonster::GetNoAutoAtt, &CMonster::SetNoAutoAtt));	//�Ƿ���������
		luaMon.set("cannotmove", sol::property(&CMonster::GetCanNotMove, &CMonster::SetCanNotMove));	//�Ƿ��ƶ�
		luaMon.set("nostruckatt", sol::property(&CMonster::GetNoStruckAtt, &CMonster::SetNoStruckAtt));	//�Ƿ񻹻�
		luaMon.set("bttype", sol::property([](CMonster& self) { return (uint8_t)self.m_btType; }));			//��������
		luaMon.set("diffcof", &CMonster::m_dwDifficultyCof);			//�����Ѷ�ϵ��
		luaMon.set_function("lvup", &CMonster::LevelUp);								    //��������
		luaMon.set_function("addexp", &CMonster::AddExp);                                //���Ӿ���

		luaMon.set("btzhongjilvl", &CMonster::m_btZhongJiLvl);
		luaMon.set("nguildid", &CMonster::m_guildId);
	}
	{//CRobot
		sol::usertype< CRobot> luaRobot = lua.new_usertype<CRobot>("CRobot", sol::base_classes, sol::bases<CCreature, CMonster, MapObject>());
		luaRobot.set("packet", sol::readonly(&CRobot::m_Packet));					//����������ֿ�
		luaRobot.set("feature", &CRobot::m_siFeature);			//��ɫ���
		luaRobot.set("sex", sol::readonly_property([](const CRobot& self) { return self.m_siFeature.sex; }));						//�Ա�
		luaRobot.set("job", sol::readonly_property([](const CRobot& self) { return self.m_siFeature.job; }));						//ְҵ	
	}
}