#include "Script.h"
#include "UsrEngn.h"
#include <register_lua.hpp>
#include "WorldBoss.h"
#include "CrawlingLadder.h"
#include "GuildFortress.h"
#include "Robot.h"
void CScriptSystem::BindActivity(CLuaVM *luavm){
	sol::state_view lua(luavm->lua());
	registerClass<ActivityCommon>(lua, "ActivityCommon",
		"sendmail", &ActivityCommon::SendMail,
		"UpdateMapId2Global", &ActivityCommon::UpdateMapId2Global
	);

	registerClass<WorldBossMgr>(lua, "WorldBossMgr",
		"setopen", &WorldBossMgr::SetOpen,
		"getopen", &WorldBossMgr::GetOpen,
		"getn", &WorldBossMgr::GetN,
		"update", &WorldBossMgr::Update,
		"clear", &WorldBossMgr::Clear,
		"getsort", &WorldBossMgr::GetSort,
		"sendrankreward", &WorldBossMgr::SendRankReward,
		"getsendreward", &WorldBossMgr::GetSendReward,
		"setsendreward", &WorldBossMgr::SetSendReward,
		"gethp", &WorldBossMgr::GetBossHp,
		"sethp", &WorldBossMgr::SetBossHp,
		"getalive", &WorldBossMgr::GetAlive,
		"setalive", &WorldBossMgr::SetAlive,
		"getstage", &WorldBossMgr::GetStage,
		"setstage", &WorldBossMgr::SetStage
	);
	registerClass<CrawlingLadderMgr>(lua, "CrawlingLadderMgr",
		"clear", &CrawlingLadderMgr::Clear,
		"update", &CrawlingLadderMgr::Update,
		"getrank", &CrawlingLadderMgr::GetRank,
		"getrankinfo", &CrawlingLadderMgr::GetRankInfo,
		"getassignedrank", &CrawlingLadderMgr::GetAssignedRank,
		"setrankinfo", &CrawlingLadderMgr::SetRankInfo
	);
	registerClass<CSkyLadder>(lua, "CSkyLadder",
		"SendLoadDataCmdToDB", &CSkyLadder::SendLoadDataCmdToDB,
		"CreateRobotMon", [](CSkyLadder& self, CGameMap* pMap, PosType x, PosType y, sol::table tab) {return self.CreateRobotMon(pMap, x, y, tab); }
	);
	registerClass<CGuildFortress>(lua, "CGuildFortress",
		"BallDamMap1", &CGuildFortress::m_ballDamage1,
		"BallDamMap2", &CGuildFortress::m_ballDamage2,
		"BallDamMap3", &CGuildFortress::m_ballDamage3,
		"BallDamMap4", &CGuildFortress::m_ballDamage4,
		"ContributeMap", &CGuildFortress::m_contributeMap,
		"BallPowerRankMap", &CGuildFortress::m_ballPowerRank,
		"ActivityStatus", &CGuildFortress::ActivityStatus,
		"SignUpTab", &CGuildFortress::SignUpTab,
		"GetAuto", &CGuildFortress::GetAuto,
		"GetSignUp", &CGuildFortress::GetSignUp,
		"SignUp", &CGuildFortress::SignUp,
		"SetAutoSignUp", &CGuildFortress::SetAutoSignUp,
		"UpdateDoorDamRank", &CGuildFortress::UpdateDoorDamRank,
		"GetDoorRankTab", &CGuildFortress::GetDoorRankTab,
		"GetMyDoorRank", &CGuildFortress::GetMyDoorRank,
		"GetCurGuildId", &CGuildFortress::GetCurGuildId,
		"SetCurGuildId", &CGuildFortress::SetCurGuildId,
		"GetCurEmblemId", &CGuildFortress::GetCurEmblemId,
		"SetCurEmblemId", &CGuildFortress::SetCurEmblemId,
		"GetZoneDonate", &CGuildFortress::GetZoneDonate,
		"ChangeZoneDonate", &CGuildFortress::ChangeZoneDonate,
		"UpdateDonateRank", &CGuildFortress::UpdateDonateRank,
		"GetDonateRankTab", &CGuildFortress::GetDonateRankTab,
		"ClearDonateRank", &CGuildFortress::ClearDonateRank,
		"GetMyDonateRank", &CGuildFortress::GetMyDonateRank,
		"AllocCloneMap", &CGuildFortress::AllocCloneMap,
		"AddStageOnePlayer", &CGuildFortress::AddStageOnePlayer,
		"ClearStageOneMap", &CGuildFortress::ClearStageOneMap,
		"SendStageOneReward", &CGuildFortress::SendStageOneReward

	);
}
