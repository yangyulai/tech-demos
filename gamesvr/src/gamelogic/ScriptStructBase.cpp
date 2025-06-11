#include "Script.h"
#include "BaseCreature.h"
#include "UsrEngn.h"
#include <register_lua.hpp>

#include "JsonConfig.h"
#include "JsonStructEx.h"

void CScriptSystem::BindStructBase(CLuaVM *luavm){
	sol::state_view lua(luavm->lua());
	lua.new_usertype<stARpgAbility>("stARpgAbility",
		sol::constructors<stARpgAbility()>(),
		"ClearAblity", &stARpgAbility::Clear,
		// Operator overload for addition:
		sol::meta_function::addition, [](stARpgAbility& a, const stARpgAbility& b) { return a + b; },

		// Read-only properties

		// 基础属性 (base attributes)
		"attrs",&stARpgAbility::attrs,
		// Property with getter and setter:
		"i64fightscore", sol::property(&stARpgAbility::LuaGetFightScore, &stARpgAbility::LuaSetFightScore)

	);
	lua.new_usertype<stMonsterDataBase>("stMonsterDataBase",
		"nmaxdc", sol::readonly(&stMonsterDataBase::nMaxPAtk),
		"nmindc", sol::readonly(&stMonsterDataBase::nMinPAtk),
		"nmaxmc", sol::readonly(&stMonsterDataBase::nMaxMAtk),
		"nminmc", sol::readonly(&stMonsterDataBase::nMinMAtk),
		"id", sol::readonly(&stMonsterDataBase::nID),
		"monname", sol::readonly(&stMonsterDataBase::szName),
		"mapname", sol::readonly(&stMonsterDataBase::szMapName),
		"exp", sol::readonly(&stMonsterDataBase::CanHaveExp),
		"type", sol::readonly(&stMonsterDataBase::monster_type),
		"boowner", sol::readonly(&stMonsterDataBase::boOwner),
		"monrace", sol::readonly(&stMonsterDataBase::btMonRace),
		"viewrange", sol::readonly(&stMonsterDataBase::ViewRange),
		"color", sol::readonly(&stMonsterDataBase::nNameColor),
		"lv", sol::readonly(&stMonsterDataBase::nLevel),
		"citizenvalue", sol::readonly(&stMonsterDataBase::nCitizenValue),
		"nmaxac", sol::readonly(&stMonsterDataBase::nPDef),
		"nminac", sol::readonly(&stMonsterDataBase::nMDef),
		"ncritrate", sol::readonly(&stMonsterDataBase::nCritRate),
		"ncritdefense", sol::readonly(&stMonsterDataBase::nCritRate),
		"ncritdefense", sol::readonly(&stMonsterDataBase::nCritDefense),
		"ndecattackdamage", sol::readonly(&stMonsterDataBase::nDecAttackDamage),
		"attacktick", sol::readonly(&stMonsterDataBase::AttackTick),
		"shenlongpoint", sol::readonly(&stMonsterDataBase::nShenlongPoint),
		"isboss", sol::readonly(&stMonsterDataBase::Isboss)
	);
	registerClass< stMonsterAbilityBase>(lua, "stMonsterAbilityBase",
		"wlevel", &stMonsterAbilityBase::wlevel,
		"point", &stMonsterAbilityBase::abilitypoint,
		"btloyalty", &stMonsterAbilityBase::btLoyalty,
		"btsavvy", &stMonsterAbilityBase::btSavvy,
		"nqualityofhp", &stMonsterAbilityBase::nQualityofHP,
		"nqualityofmp", &stMonsterAbilityBase::nQualityofMP,
		"nqualityofhit", &stMonsterAbilityBase::nQualityofHit,
		"nqualityofskillhit", &stMonsterAbilityBase::nQualityofSkillHit,
		"nqualityofsente", &stMonsterAbilityBase::nQualityofSente,
		"nqualityofdef", &stMonsterAbilityBase::nQualityofDef,
		"nqualityofskilldef", &stMonsterAbilityBase::nQualityofSkillDef,
		"nqualityofbackhit", &stMonsterAbilityBase::nQualityofBackHit,
		"nqualityofhitratio", &stMonsterAbilityBase::nQualityofHitRatio,
		"nqualityofbighit", &stMonsterAbilityBase::nQualityofBigHit,
		"nqualityofjouk", &stMonsterAbilityBase::nQualityofJouk,
		"nqualityofjoukbigratio", &stMonsterAbilityBase::nQualityofJoukBigRatio,
		"nrateofgrowth", &stMonsterAbilityBase::nRateofGrowth
		);
	lua.new_usertype<stBuffDataBase>("stBuffDataBase",
		"name", sol::readonly(&stBuffDataBase::szName),
		"dwid", sol::readonly(&stBuffDataBase::dwID),
		"btstatetype", sol::readonly(&stBuffDataBase::btStateType),
		"btlevel", sol::readonly(&stBuffDataBase::btLevel),
		"wbuffnum", sol::readonly(&stBuffDataBase::wBuffNum),
		"wbuffcof", sol::readonly(&stBuffDataBase::wBuffCof),
		"bttimetype", sol::readonly(&stBuffDataBase::btTimeType),
		"keeptime", sol::readonly(&stBuffDataBase::i64KeepTime),
		"dwtimeinterval", sol::readonly(&stBuffDataBase::dwTimeInterval),
		"bocover", sol::readonly(&stBuffDataBase::boCover),
		"bodieremove", sol::readonly(&stBuffDataBase::boDieRemove),
		"boofflineremove", sol::readonly(&stBuffDataBase::boOfflineRemove),
		"dweffectid", sol::readonly(&stBuffDataBase::dwEffectId),
		"wformulanum", sol::readonly(&stBuffDataBase::wFormulaNum),
		"ndura", sol::readonly(&stBuffDataBase::nDura)
	);
	lua.new_usertype<stMagicDataBase>("stMagicDataBase",
		"nid", sol::readonly(&stMagicDataBase::nID),
		"name", sol::readonly(&stMagicDataBase::szName),
		"btlevel", sol::readonly(&stMagicDataBase::btlevel),
		"buffid", sol::readonly(&stMagicDataBase::szBuffID),
		"wskillneedlv", sol::readonly(&stMagicDataBase::wSkillNeedLv),
		"btjob", sol::readonly(&stMagicDataBase::btjob),
		"btattacktype", sol::readonly(&stMagicDataBase::btAttackType),
		"dwneedmp", sol::readonly(&stMagicDataBase::dwNeedMP),
		"dwneedmpcof", sol::readonly(&stMagicDataBase::dwNeedMPCof),
		"nsucccof", sol::readonly(&stMagicDataBase::nSuccessCof),
		"dwcdbytime", sol::readonly(&stMagicDataBase::dwCDbyTime),
		"ndamage", sol::readonly(&stMagicDataBase::nDamage),
		"ndamage", sol::readonly(&stMagicDataBase::nDamage),
		"ndamagecof", sol::readonly(&stMagicDataBase::nDamageCof),
		"wattacknum", sol::readonly(&stMagicDataBase::wAttackNum),
		"btshape", sol::readonly(&stMagicDataBase::btShape),
		"ndamagetype", sol::readonly(&stMagicDataBase::nDamageType),
		"wformulanum", sol::readonly(&stMagicDataBase::wFormulaNum),
		"btatomtype", sol::readonly(&stMagicDataBase::btAtomType),
		"duration", sol::readonly(&stMagicDataBase::duration),
		"btmaxrange", sol::readonly(&stMagicDataBase::btMaxRange)
	);
	

	lua.new_usertype<stItemDataBase>("stItemDataBase",
		"nid", sol::readonly(&stItemDataBase::nID),
		"name", sol::readonly(&stItemDataBase::szName),
		"name_gb", sol::readonly(&stItemDataBase::name_gb),
		"job", sol::readonly(&stItemDataBase::btJobType),
		"nextid", sol::readonly(&stItemDataBase::nNextID),
		"station", sol::readonly(&stItemDataBase::btEquipStation),
		"sex", sol::readonly(&stItemDataBase::btSexType),
		"dwdrugbuffid", sol::readonly(&stItemDataBase::dwDrugBuffId),
		"maxcount", sol::readonly(&stItemDataBase::dwMaxCount),
		"nrare", sol::readonly(&stItemDataBase::nRare),
		"recycleyuanbao", sol::readonly(&stItemDataBase::nRecycleYuanBao),
		"dwwarrioreffectid", sol::readonly(&stItemDataBase::dwWarriorEffectId),
		"dwcdbytick", sol::readonly(&stItemDataBase::dwCdByTick),
		"bindtype", sol::readonly(&stItemDataBase::btBindType),
		"buyprice", sol::readonly(&stItemDataBase::nBuyPrice),
		"color", sol::readonly(&stItemDataBase::btColor),
		"notice", sol::readonly(&stItemDataBase::btNotice),
		"recycleexp", sol::readonly(&stItemDataBase::nRecycleExp),
		"nquality", sol::readonly(&stItemDataBase::nQuality),
		"guardtype", sol::readonly(&stItemDataBase::btGuardType),
		"faceid", sol::readonly(&stItemDataBase::nFaceId),
		"facesex", sol::readonly(&stItemDataBase::btFaceSex),
		"facejob", sol::readonly(&stItemDataBase::btFaceJob),
		"type", sol::readonly(&stItemDataBase::dwType),
		"itemlimittime", sol::readonly(&stItemDataBase::dwItemLimitTime),
		"level", sol::readonly(&stItemDataBase::dwNeedLevel),
		"privilegeuselimit", sol::readonly(&stItemDataBase::nPrivilegeUseLimit),
		"dwspecialeffid", sol::readonly(&stItemDataBase::dwDropMonEffId),
		"variablemaxcount", sol::readonly(&stItemDataBase::nVariableMaxCount),
		"nsellgameccy", sol::readonly(&stItemDataBase::nSellGameCcy),
		"recycleitem", sol::readonly(&stItemDataBase::vRecycleItem)
	);
	registerClass< stSimpleFeature>(lua, "stSimpleFeature",
		"sex", &stSimpleFeature::sex,
		"job", &stSimpleFeature::job,
		"weapon", &stSimpleFeature::weapon,
		"dress", &stSimpleFeature::dress,
		"shoe", &stSimpleFeature::shoe,
		"pants", &stSimpleFeature::pants,
		"back", &stSimpleFeature::back,
		"helmet", &stSimpleFeature::helmet,
		"eye", &stSimpleFeature::eye,
		"face", &stSimpleFeature::face,
		"hair", &stSimpleFeature::hair,
		"nose", &stSimpleFeature::nose,
		"mouth", &stSimpleFeature::mouth
	);
	lua.new_usertype<stEffectDataLoaderBase>("stEffectDataLoaderBase",
		"id", sol::readonly(&stEffectDataLoaderBase::dwID),
		"name", sol::readonly(&stEffectDataLoaderBase::szName),
		"nextid", sol::readonly(&stEffectDataLoaderBase::dwNextId)
	);

	lua.new_usertype<stServerMapInfo>("stServerMapInfo",
		"dwMapID", sol::readonly(&stServerMapInfo::dwMapID),
		"dwMapFileID", sol::readonly(&stServerMapInfo::dwMapFileID),
		"szMapFileName", sol::readonly(&stServerMapInfo::szMapFileName),
		"szName", sol::readonly(&stServerMapInfo::szName),
		"nServerIndex", sol::readonly(&stServerMapInfo::nServerIndex),
		"nMinMapIdx", sol::readonly(&stServerMapInfo::nMinMapIdx),
		"dwMapscriptid", sol::readonly(&stServerMapInfo::dwMapscriptid),
		"dwReliveMapid", sol::readonly(&stServerMapInfo::dwReliveMapid),
		"dwReliveX", sol::readonly(&stServerMapInfo::dwReliveX),
		"dwReliveY", sol::readonly(&stServerMapInfo::dwReliveY),
		"dwReliveRange", sol::readonly(&stServerMapInfo::dwReliveRange),
		"dwMapProperty", sol::readonly(&stServerMapInfo::dwMapProperty),
		"isclone", sol::readonly(&stServerMapInfo::isClone)
	);
	registerClass< stShortCuts>(lua, "stShortCuts",
		"emType", &stShortCuts::emShortCuts,
		"btValue", &stShortCuts::btShortCuts,
		"btRow", &stShortCuts::btRow,
		"btCol", &stShortCuts::btCol
	);
	registerClass< stSimpleAbility>(lua, "stSimpleAbility",
		"nstrength", &stSimpleAbility::nStrength,
		"nphysique", &stSimpleAbility::nPhysique,
		"nagility", &stSimpleAbility::nAgility,
		"nwisdom", &stSimpleAbility::nWisdom,
		"nintelligence", &stSimpleAbility::nIntelligence,
		"nmaxhp", &stSimpleAbility::nMaxHP,
		"nmaxmp", &stSimpleAbility::nMaxMP,
		"nmaxpp", &stSimpleAbility::nMaxPP
	);
	lua.new_usertype<unsigned __int64>("unsigned__int64",
		sol::call_constructor, sol::constructors<unsigned __int64()>(),
		sol::meta_function::addition, [](unsigned __int64 a, unsigned __int64 b) { return a + b; },
		sol::meta_function::subtraction, [](unsigned __int64 a, unsigned __int64 b) { return a - b; },
		sol::meta_function::multiplication, [](unsigned __int64 a, unsigned __int64 b) { return a * b; },
		sol::meta_function::division, [](unsigned __int64 a, unsigned __int64 b) { return a / b; },
		sol::meta_function::equal_to, [](unsigned __int64 a, unsigned __int64 b) { return a == b; },
		sol::meta_function::less_than, [](unsigned __int64 a, unsigned __int64 b) { return a < b; },
		sol::meta_function::less_than_or_equal_to, [](unsigned __int64 a, unsigned __int64 b) { return a <= b; }
	);
}
