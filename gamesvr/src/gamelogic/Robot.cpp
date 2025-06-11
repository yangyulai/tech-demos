#include "Robot.h"
#include "UsrEngn.h"
#include "Config.h"
#include "JsonConfig.h"
#include "lua_base.h"
#include "Chat.h"

CRobot::CRobot(PosType x, PosType y, char* szName, std::shared_ptr<stMonsterDataBase>& pMoninfo):
	CMonster(CRET_ROBOT, x, y, pMoninfo, 0, 0), m_siFeature(), m_siAbility()
{
	FUNCTION_BEGIN;

	m_i64UserOnlyID = 0; //唯一角色ID
	m_nVipLevel = 0;
	m_dwHeadPortrait = 0;
	m_curMagic = nullptr;
	// 数据配置的人物数据
	m_nAttackInterval = pMoninfo->AttackTick; //攻击间隔
	m_nReleaseInterval = pMoninfo->CastingTick; //释放间隔
	m_nMoveInterval = pMoninfo->MoveTick; //移动间隔
	m_nPpMoveCost = 0; //移动消耗体力
	m_fHpDrugRate = 0;
	m_fMpDrugRate = 0;
	ZeroMemory(&m_GuildInfo, sizeof(m_GuildInfo));
	ZeroMemory(&m_res, sizeof(m_res));

	m_moveType = emMove_Run;
}

CRobot::~CRobot() {
	FUNCTION_BEGIN;
}

void CRobot::Update()
{
	FUNCTION_BEGIN;
	CMonster::Update();
}

bool CRobot::Die()
{
	if (CCreature::Die())  
	{
		TriggerEvent(this, DIE, 1);
		timer_.AddTimer(2000, [this]() {
			MakeGhost(true, __FUNC_LINE__);
			}, false);
		return true;
	}
	return false;
}

void CRobot::RunTargetAI() {
	if (m_AiCfg.bitNoStruckAtt == 0) {
		if (m_curAttTarget && GetEnvir() == m_curAttTarget->GetEnvir() && !m_curAttTarget->isDie()) {
			auto targetX = m_curAttTarget->m_nCurrX;
			auto targetY = m_curAttTarget->m_nCurrY;
			if (!CanHit()) return;

			uint64_t curTick = GetTickCount64();

			// 先上buff
			m_cMagic.forEach([this, curTick](stMagic* magic) {
				if (magic->GetMagicDataBase()->btActiveType == ACTIVETYPE_ACTIVE && magic->GetMagicDataBase()->btEnemyType == ENEMYTYPE_FRIEND && magic->GetMagicDataBase()->szSelfBuffID.size() > 0)
				{
					for (DWORD buffid : magic->GetMagicDataBase()->szSelfBuffID) {
						auto* pbuff = m_cBuff.FindBuff(buffid);
						if (!pbuff)
						{
							int nDir = m_btDirection;
							if (MonsterAttack(this, magic, m_btDirection)) {
								m_dwNextHitTick = max(m_dwNextHitTick, curTick) + m_dwHitIntervalTime;
								m_dwNextCastTick = max(m_dwNextCastTick, curTick) + m_dwMoveIntervalTime;
							}
							break;
						}
					}			
				}				
			});

			sol::state_view lua(CUserEngine::getMe().m_scriptsystem.m_LuaVM->GetHandle());
			// 吃药检测
			if (m_stAbility[AttrID::MaxHP] && (m_nNowHP * 1.0 / m_stAbility[AttrID::MaxHP]) < m_fHpDrugRate && m_Packet.m_ActBag.size() > 0)
			{
				sol::table main_table = lua["item_redDrug"];
				if (main_table)
				{
					for (auto item : m_Packet.m_ActBag)
					{
						if (main_table[item->GetItemBaseID()])
						{
							auto hp = CALL_LUARET<int>("item_HPDrug", 0, item->GetItemBaseID(), this);
							StatusValueChange(stCretStatusValueChange::hp, hp, "robothp");
							m_Packet.DeleteItemInActBag(item, 1);
							m_dwNextHitTick = max(m_dwNextHitTick, curTick) + m_dwHitIntervalTime;
							m_dwNextCastTick = max(m_dwNextCastTick, curTick) + m_dwCastIntervalTime;
							break;
						}
					}
				}				
			}

			if (m_stAbility[AttrID::MaxMP] && (m_nNowMP * 1.0 / m_stAbility[AttrID::MaxMP]) < m_fMpDrugRate && m_Packet.m_ActBag.size() > 0)
			{
				sol::table main_table = lua["item_blueDrug"];
				if (main_table) {
					for (auto item : m_Packet.m_ActBag)
					{
						if (main_table[item->GetItemBaseID()])
						{
							auto mp = CALL_LUARET<int>("item_MPDrug", 0, item->GetItemBaseID(), this);
							StatusValueChange(stCretStatusValueChange::mp, mp, "robothp");
							m_Packet.DeleteItemInActBag(item, 1);
							m_dwNextHitTick = max(m_dwNextHitTick, curTick) + m_dwHitIntervalTime;
							m_dwNextCastTick = max(m_dwNextCastTick, curTick) + m_dwCastIntervalTime;
							break;
						}
					}
				}				
			}

			if (curTick < m_dwNextHitTick) return;
			if (!m_curMagic) {
				if (ChebyshevDistance(m_curAttTarget) > 5) {
					m_curMagic = ChooseActSkill([](stMagic* ma, stMagic* mb)->bool { return ma->GetMagicDataBase()->btMaxRange > mb->GetMagicDataBase()->btMaxRange; });
				}
				else {
					m_curMagic = m_cMagic.randomActiveSkill();
				}
			}

			if (m_curMagic)
			{
				if (ChebyshevDistance(m_curAttTarget) > m_curMagic->GetMagicDataBase()->btMaxRange) {
					if (CanWalk()) {
						if (curTick > m_dwNextMoveTick) {
							int dir = GetEnvir()->GetNextDirection(m_nCurrX, m_nCurrY, targetX, targetY);
							bool bowalkok = MoveTo(dir, 2, targetX, targetY, true);
							m_dwNextMoveTick = curTick + (bowalkok ? (m_dwMoveIntervalTime) : (m_dwMoveIntervalTime * 2 + _random(2000)));
						}
					}
				}
				else
				{
					if (sConfigMgr.m_bRobot && m_curAttTarget->isPlayer())
						CChat::sendClient(m_curAttTarget->toPlayer(), "机器人释放技能 %s skillid= %d", UTG(m_curMagic->getShowName()), m_curMagic->GetMagicDataBase()->nID);
					AttackTarget(m_curMagic, m_curMagic->GetMagicDataBase()->btMaxRange);
					m_curMagic = nullptr;
				}
			}
			
		}
	}
}

stMagic* CRobot::ChooseActSkill(const std::function<bool(stMagic*, stMagic*)>& predicate)
{
	FUNCTION_BEGIN;
	return m_cMagic.ChooseActSkill(this, predicate);
}

void CRobot::StudySkill(DWORD dwSkillid, BYTE btLevel) {
	FUNCTION_BEGIN;
	stMagic* pMagic = m_cMagic.addskill(dwSkillid, btLevel);
	if (pMagic && pMagic->savedata.level != btLevel) {
		pMagic->skilllevelup(btLevel, m_cMagic);
	}
}

bool CRobot::DeleteSkill(DWORD dwSkillid) {
	m_cMagic.removeskill(dwSkillid);
	return false;
}

void CRobot::ClearSkill() {
	m_cMagic.clear();
}

DWORD CRobot::getVipType() {
	FUNCTION_BEGIN;
	enum {
		VIP_MAXLEVEL = 0x00000001,	//等级第一
		VIP_WARRIOR = 0x00000002,	//战士第一
		VIP_MAGE = 0x00000004,	//法师第一
		VIP_MONK = 0x00000008,	//道士第一
		VIP_NULL1 = 0x00000010,	//留空
		VIP_NULL2 = 0x00000020,	//留空
		VIP_NULL3 = 0x00000040,	//留空
		VIP_TQ1 = 0x00000080,	//特权1级
		VIP_TQ2 = 0x00000100,
		VIP_TQ3 = 0x00000200,
		VIP_TQ4 = 0x00000400,
		VIP_TQ5 = 0x00000800,
		VIP_NULL12 = 0x00001000,
		VIP_NULL13 = 0x00002000,
		VIP_NULL14 = 0x00004000,
		VIP_NULL15 = 0x00008000,
		VIP_REN_ZHENG = 0x00010000,	//官方认证
		VIP_NULL21 = 0x00020000,
		VIP_NULL22 = 0x00040000,
		VIP_NULL23 = 0x00080000,
		VIP_NULL24 = 0x00100000,
		VIP_NULL25 = 0x00200000,
		VIP_NULL26 = 0x00400000,
		VIP_NULL27 = 0x00800000,
		VIP_NULL28 = 0x01000000,
		VIP_NULL29 = 0x02000000,
		VIP_NULL30 = 0x04000000,
		VIP_NULL31 = 0x08000000,
		VIP_NULL32 = 0x10000000,
		VIP_NULL33 = 0x20000000,
		VIP_NULL34 = 0x40000000,
		VIP_NULL35 = 0x80000000,
	};

	DWORD dwTpye = 0;
	if (CUserEngine::getMe().m_i64LevelMaxPlayer == m_i64UserOnlyID) {
		dwTpye |= VIP_MAXLEVEL;
	}

	static int VIP[] = { 0,VIP_TQ1,VIP_TQ2,VIP_TQ3,VIP_TQ4, VIP_TQ5 };
	if (m_nVipLevel > 0 && m_nVipLevel < count_of(VIP)) {
		dwTpye |= VIP[m_nVipLevel];
	}
	return dwTpye;
}

void CRobot::BuildPlayerFeature(stPlayerFeature& feature)
{
	feature.btCretType = CRET_PLAYER;
	__int64 dw_bo_AllFeature = 0;
	if (m_cBuff.GetBuffState(MAGICSTATE_SPEEDSLOW)) {
		dw_bo_AllFeature |= stCretFeature::STATE_SpeedSlow;
	}
	if (m_cBuff.GetBuffState(MAGICSTATE_PETRIFACTION)) {
		dw_bo_AllFeature |= stCretFeature::STATE_Petrifaction;
	}
	if (m_cBuff.GetBuffState(MAGICSTATE_DIZZY)) {
		dw_bo_AllFeature |= stCretFeature::STATE_JinGu;
	}
	if (m_cBuff.GetBuffState(MAGICSTATE_MAGICSHIELD)) {
		dw_bo_AllFeature |= stCretFeature::STATE_MagicShield;
	}
	if (m_cBuff.GetBuffState(MAGICSTATE_SWORDSHIELD)) {
		dw_bo_AllFeature |= stCretFeature::STATE_SwordShield;
	}
	feature.n_bo_AllFeature = dw_bo_AllFeature;
	feature.wTitleId = quest_vars_get_var_n("curtitleid");
	feature.btBattleCamp = m_btBattleCamp;
	feature.dwAtkSpeedPer = m_dwHitIntervalTime;
	feature.btVip = getVipType();
	feature.dwClanId = m_GuildInfo.dwGuildId;
	feature.btNameColor = m_res[ResID::citizenLv];
	feature.dwMoveInterval = m_nMoveInterval / (1 + (m_stAbility[AttrID::MoveSpeedPer] + sConfigMgr.GetMoveSpeed(m_stAbility[AttrID::MoveSpeedPhase])) / 10000.0f);
	feature.dwHeadPortrait = m_dwHeadPortrait;
	feature.dwEmblem = m_GuildInfo.dwEmblemId;
	feature.btMilitaryRank = quest_vars_get_var_n("MilitaryRankId");
	if (!CALL_LUARET<bool>("CheckNameIsRobot", true, GTU(Name().c_str()))) {
		CALL_LUARET<bool>("PlayerDressRef", false, this);
	}	
	feature.siFeature = m_siFeature;
}

void CRobot::EnterMapNotify(MapObject* obj)
{
	stMapCreatePlayer retcmd;
	retcmd.i64OnlyId = m_i64UserOnlyID;
	retcmd.location.mapid = GetEnvir()->getMapId();
	retcmd.location.ncurx = m_nCurrX;
	retcmd.location.ncury = m_nCurrY;
	retcmd.location.ncurz = m_nCurrZ;
	retcmd.dwTmpId = GetObjectId();
	retcmd.lifestate = m_LifeState;
	retcmd.dwLevel = m_dwLevel;
	retcmd.nNowHp = m_nNowHP;
	retcmd.nMaxHp = m_stAbility[AttrID::MaxHP];
	retcmd.nNowMp = m_nNowMP;
	retcmd.nMaxMp = m_stAbility[AttrID::MaxMP];
	retcmd.btDir = m_btDirection;
	CopyString(retcmd.szShowName, m_displayName);
	BuildPlayerFeature(retcmd.feature);
	retcmd.transType = transTypeNone;
	obj->SendMsgToMe(&retcmd, sizeof(retcmd));
	AfterSpaceMove(obj);
	if (CPlayerObj* player = dynamic_cast<CPlayerObj*>(obj))
	{
		SetAttackTarget(player);
	}
}


void CRobot::GetBaseProperty() {
	FUNCTION_BEGIN;
	auto pstPlayerBase = sJsonConfig.GetLvlAbilityData(m_dwLevel, GetJobType());
	if (pstPlayerBase) {

		m_stBaseAbility[AttrID::MaxHP] = pstPlayerBase->dwHp;			//生命	
		m_stBaseAbility[AttrID::MaxMP] = pstPlayerBase->dwMp;			//精神
		m_stBaseAbility[AttrID::MaxPP] = pstPlayerBase->dwPP;             //体力
		m_stBaseAbility[AttrID::MinPAtk] = pstPlayerBase->dwMinPAtk;	//物攻下限
		m_stBaseAbility[AttrID::MaxPAtk] = pstPlayerBase->dwMaxPAtk;	//物攻上限
		m_stBaseAbility[AttrID::MaxMAtk] = pstPlayerBase->dwMaxMAtk;	//最大魔攻
		m_stBaseAbility[AttrID::PDef] = pstPlayerBase->dwPDef;			//物防
		m_stBaseAbility[AttrID::MDef] = pstPlayerBase->dwMDef;			//魔防
		m_stBaseAbility[AttrID::Hit] = pstPlayerBase->dwHit;			//命中率
		m_stBaseAbility[AttrID::Juck] = pstPlayerBase->dwJuck;			//闪避率
		m_stBaseAbility[AttrID::HpRestore] = pstPlayerBase->nHpRestore;		//生命回复
		m_stBaseAbility[AttrID::MpRestore] = pstPlayerBase->nMpRestore;		//精神回复
		m_stBaseAbility[AttrID::PpRestore] = pstPlayerBase->nPpRestore;		//体力回复
		m_nAttackInterval = pstPlayerBase->nAttackInterval;	//攻击间隔
		m_nReleaseInterval = pstPlayerBase->nReleaseInterval;	//释放间隔
		m_nMoveInterval = pstPlayerBase->nMoveInterval;		//移动间隔
		m_stBaseAbility[AttrID::PveMul] = pstPlayerBase->nPveCof;					//PVE
		m_stBaseAbility[AttrID::PvpMul] = pstPlayerBase->nPvpCof;					//PVE
		m_nPpMoveCost = pstPlayerBase->nPpMoveCost;			//移动消耗体力
	}
	else {
		g_logger.error("人物基本属性表 %d级基本数据不存在!", m_dwLevel);
	}

	//人物最大血量至少为1（防止属性表对应等级数据未配置，导致人物最大血量为0人物不停死亡的问题）
	if (m_stBaseAbility[AttrID::MaxHP] < 1)
	{
		g_logger.error("人物最大血量为0，强制修正为1!");
		m_stBaseAbility[AttrID::MaxHP] = 1;
	}
}

void CRobot::ChangeProperty(bool bosend, const char* ff)
{
	DoChangeProperty(m_stAbility, bosend, ff);
}

void CRobot::DoChangeProperty(stARpgAbility& abi, bool boNotif, const char* ff) {
	FUNCTION_BEGIN;
	FUNCTION_WRAPPER(true, "");
	stARpgAbility stOldAbi = abi;
	int64_t oldFightScore = m_stAbility.i64FightScore;

	// 技能等级
	m_stSpecialAbility.Clear();
	bool bCalcSkillExtraLv = false;	//是否要计算技能的额外等级
	m_Packet.GetPlayerEquipSpecialProper(&m_stSpecialEquipAbility);
	calculateLuaSpecialAbility(&m_stSpecialLuaAbility);
	m_stSpecialAbility += m_stSpecialEquipAbility;
	m_stSpecialAbility += m_stSpecialLuaAbility;
	m_stSpecialAbility += m_stSpecialBuffAbility;
	m_cMagic.calcSkillExtarLv(m_stSpecialAbility);		// 计算技能的额外等级
	m_cMagic.calcPassiveSkillAbility(&m_stPassiveSkillAbility, this->GetJobType());		// 重新计算被动技能的被动属性

	// 属性
	abi.Clear();
	m_Packet.GetPlayerEquipProper(&m_stEquipAbility);
	calculateLuaAbility(&m_stLuaAbility);
	calculateLuaTimeAbility(&m_stLuaTimeAbility);
	calculateAttrPointAbility(&m_stAttrPointAbility);
	calculateBuffAbility();

	abi += m_siAbility;
	abi += m_stBaseAbility;
	abi += m_stEquipAbility;
	abi += m_stLuaAbility;
	abi += m_stPassiveSkillAbility;
	abi += m_stBuffAbility;
	abi += m_stLuaTimeAbility;
	abi += m_stAttrPointAbility;

	stARpgAbility tmpabi;
	tmpabi.Clear();
	// 一级属性转化二级属性
	CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<bool>("AttrConversion", false, this, &abi, &tmpabi);
	abi += tmpabi;

	CalculatingSpeed();

	abi.CalExtraAttr(m_stExtraAbility);
	abi += m_stExtraAbility;

	m_fHpDrugRate = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<float>("SkyLadderHpDrugRate", 0.4);
	m_fMpDrugRate = CUserEngine::getMe().m_scriptsystem.m_LuaVM->Call<float>("SkyLadderMpDrugRate", 0.4);

	// 作为怪物AI的属性
	m_nViewRangeX = 99;
	m_nViewRangeY = 99;
	m_AiCfg.bitNoAttack = 0;			//=1 不能攻击
	m_AiCfg.bitNoAutoAtt = 0;			//=1 不主动攻击
	m_AiCfg.bitNoStruckAtt = 0;		//=1 不还击
	m_AiCfg.bitGoStop = 0;			//=1 主动攻击或则还击 后 会回到追击或还击前的位置
	m_AiCfg.bitLowHpRunAway = 1;		//=1 血少就逃跑
	m_AiCfg.bitFlytoTarget = 0;		//=1 可以如果目标脱离攻击范围直接飞到目标附近
	m_AiCfg.bitCallNearMon = 1;		//=1 召集附近的怪物	
	m_AiCfg.bitLowHpFlyAway = 0;		//=1 血少就飞到附近躲起来	
	m_AiCfg.bitGoHome = 0;			//=1 会自己回到出生点
	m_AiCfg.bitWondering = 0;			//=1 在不激活状态也会随机行走
	m_AiCfg.bitNoPushed = 0;			//=1 不能被撞动
	m_AiCfg.bitCanNotMove = 0;		//=1 不能移动
}

const char* CRobot::getShowName(char* szbuffer, int nmaxlen) {
	FUNCTION_BEGIN;
	CCreature::getShowName(szbuffer, nmaxlen);
	return szbuffer;
}

bool CRobot::LoadHumanBaseData(stLoadPlayerData* pgamedata)
{
	FUNCTION_BEGIN;
	stLoadPlayerData& gamedata = *pgamedata;
	m_i64UserOnlyID = gamedata.i64UserOnlyId;
	m_wHomeMapID = gamedata.whomamapid;
	m_wHomeCloneMapId = gamedata.whomeclonemapid;
	m_nHomeX = gamedata.homex;
	m_nHomeY = gamedata.homey;
	m_dwLevel = gamedata.nlevel;
	m_dwLastLevelUpTime = gamedata.dwLastLvupTime ? gamedata.dwLastLvupTime : (DWORD)time(0);
	m_siFeature = gamedata.siFeature;
	m_siAbility = gamedata.siAbility;
	m_nNowHP = gamedata.dwNowHP;
	m_LifeState = (m_nNowHP <= 0) ? ISDIE : NOTDIE;
	m_nNowMP = gamedata.dwNowMP;
	m_GuildInfo.dwGuildId = gamedata.dwGuildID;
	m_nVipLevel = gamedata.dwVipLv;
	m_nNowPP = gamedata.nNowPP;
	return true;
}

//注意 线程安全 多线程需要修改
static unsigned char g_szSaveDataBuffer[1024 * 1024 * 4];
static unsigned char g_szTmpSaveDataBuffer[1024 * 16];
#define _SAVE_DATA_CHECKNUM_			0x12345678

bool CRobot::LoadHumanData(stLoadPlayerData* pgamedata) {
	FUNCTION_BEGIN;
#define _LOADMOVE(pbindata,bindatasize,len) {\
	pbindata+=len;\
	bindatasize-=len;\
	len=bindatasize;\
	}
	if (pgamedata) {
		if (pgamedata->getBinData().size > 0 && pgamedata->getUncompressSize() > 0) {
			unsigned char* pbindata = &g_szSaveDataBuffer[0];
			unsigned long bindatasize = sizeof(g_szSaveDataBuffer);
			//////////////////////////////////////////////////////////////////////////
			//解压2进制流
			if (uncompresszlib(&pgamedata->getBinData()[0], pgamedata->getBinData().size, pbindata, bindatasize) != Z_OK || bindatasize != pgamedata->getUncompressSize()) {
				g_logger.error("Robot 角色 %s 读档数据解压缩失败( 压缩前:%d : 压缩后:%d )!", getName(), pgamedata->getUncompressSize(), pgamedata->getBinData().size);
				pgamedata->getBinData().clear();
				return false;
			}
			//////////////////////////////////////////////////////////////////////////
			//读取2进制流数据 pbindata 数据首指针  bindatasize 数据长度
			pbindata[bindatasize] = 0;
			Json::Reader reader;
			Json::Value json_object;
			if (reader.parse((char*)pbindata, json_object)) {
				int nver = json_object["PbinVer"].asInt();

				const char* szEquip = json_object["Equip"].asCString();
				if (szEquip && !LoadEquip(szEquip, (int)strlen(szEquip), nver)) {
					g_logger.error("Robot 角色 %s 读取装备失败!", getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szSkill = json_object["Skill"].asCString();
				emJobType job = (emJobType)m_siFeature.job;
				if (szSkill && !m_cMagic.loadskill(szSkill, (int)strlen(szSkill), nver, job)) {
					g_logger.error("Robot  角色 %s 读取技能失败!", getName());
					pgamedata->getBinData().clear();
					return false;
				}

				const char* szVars = json_object["Vars"].asCString();
				if (szVars && !m_vars.load(szVars, (int)strlen(szVars), nver)) {
					g_logger.error("Robot  角色 %s 读取全局变量失败!", getName());
					pgamedata->getBinData().clear();
					return false;
				}
			}
		}
		return true;
	}
	return false;
}

bool CRobot::LoadEquip(const char* dest, int retlen, int nver) {
	FUNCTION_BEGIN;
	int maxsize = retlen;
	if (maxsize == 0) return true;

	retlen = ROUNDNUMALL(retlen, 4) / 4 * 3;
	_GET_TH_LOOPCHARBUF(retlen + 1, true);
	char* pin = ptlsbuf;
	ZeroMemory(pin, retlen);
	base64_decode((char*)dest, maxsize, pin, retlen);
	memcpy((char*)dest, pin, retlen);
	maxsize = retlen;

	if (maxsize < sizeof(int)) { return false; }
	int count = *((int*)(dest));
	int len = sizeof(int);//物品数据开始的偏移

	int nstructlen = sizeof(stItem);
	if (maxsize < int(count * nstructlen + sizeof(int))) { return false; }

	for (int i = 0; i < count; i++)
	{
		int ncurrver = nver;
		stItem* pItemData = NULL;
		char szupdatebuffer[2][sizeof(stItem) + 1024 * 4];
		ZeroMemory(&szupdatebuffer[0], 2 * (sizeof(stItem) + 1024 * 4));
		int nowbuffer = 0;
		char* poldbuffer = (char*)&dest[len];

		stItem item;
		memcpy(&item, (stItem*)poldbuffer, sizeof(stItem));
		if (item.i64ItemID)
		{
			CItem* pItem = NULL;
			pItem = CItem::LoadItem(&item, __FUNC_LINE__);
			if (pItem)
			{
				switch (item.Location.btLocation)
				{
					case ITEMCELLTYPE_EQUIP:
					{
						m_Packet.m_stEquip[item.Location.btIndex] = pItem;
					}break;
					default: {
						CUserEngine::getMe().ReleasePItem(pItem, __FUNC_LINE__);
					}
				}
			}
		}
		len += nstructlen;
	}
	return true;
}


bool CRobot::isEnemy(CCreature* pTarget/* =NULL */) {
	if (!pTarget || this == pTarget) return false;

	if (pTarget->isPet() && pTarget->toPet()->getMaster()->isPlayer() && !pTarget->toPet()->getMaster()->toPlayer()->isWudi())
	{
		return true;
	}
	if (pTarget->isPlayer() && !pTarget->isWudi())
	{
		return true;
	}
	return false;
}

void CRobot::calculateLuaAbility(stARpgAbility* abi) {
	FUNCTION_BEGIN;
	if (abi) {
		abi->Clear();
		CUserEngine::getMe().m_abilityKeys.forEach([&](const std::string& key)
			{
				DWORD effectId = quest_vars_get_var_n(key);
				if (effectId == 0) return;
				auto effect = sJsonConfig.GetEffectDataById(effectId);
				if (!effect) return;
				abi->Add(*effect);
			});
	}
}

void CRobot::calculateLuaTimeAbility(stARpgAbility* abi) {
	FUNCTION_BEGIN;
	if (abi) {
		abi->Clear();
		CUserEngine::getMe().m_abilityTimeKeys.forEach([&](const std::string& key)
			{
				DWORD effectId = quest_vars_get_var_n(key);
				if (effectId == 0) return;
				auto effect = sJsonConfig.GetEffectDataById(effectId);
				if (!effect) return;
				abi->Add(*effect);
			});
	}
}

void CRobot::calculateAttrPointAbility(stARpgAbility* abi)
{
	if (abi) {
		abi->Clear();
		for (size_t i = 1; i <= 5; i++) {
			switch (i)
			{
			case 1:
				abi->attrs[tosizet(AttrID::Strength)] = quest_vars_get_var_n("ap1");
			case 2:
				abi->attrs[tosizet(AttrID::Physique)] = quest_vars_get_var_n("ap2");
			case 3:
				abi->attrs[tosizet(AttrID::Agility)] = quest_vars_get_var_n("ap3");
			case 4:
				abi->attrs[tosizet(AttrID::Wisdom)] = quest_vars_get_var_n("ap4");
			case 5:
				abi->attrs[tosizet(AttrID::Intelligence)] = quest_vars_get_var_n("ap5");
			default:
				break;
			}
		}
	}
}

bool CRobot::CalculatingSpeed()
{
	float RatioDenom = 10000.0f;
	int nAttackSpeed = 0; // 攻击速度阶段攻速加成
	int nMoveSpeed = 0; // 移动速度阶段攻速加成
	int nReleaseSpeed = 0; // 释放速度阶段攻速加成
	auto AttackAdd = sConfigMgr.GetAttackSpeed(m_stAbility[AttrID::AttackSpeedPhase]);
	if (AttackAdd) {
		nAttackSpeed = AttackAdd;
	}
	auto MoveAdd = sConfigMgr.GetMoveSpeed(m_stAbility[AttrID::MoveSpeedPhase]);
	if (MoveAdd) {
		nMoveSpeed = MoveAdd;
	}
	auto ReleaseAdd = sConfigMgr.GetReleaseSpeed(m_stAbility[AttrID::ReleaseSpeedPhase]);
	if (ReleaseAdd) {
		nReleaseSpeed = ReleaseAdd;
	}
	m_dwHitIntervalTime = m_nAttackInterval / (1 + (m_stAbility[AttrID::AttackSpeedPer] + nAttackSpeed) / RatioDenom);
	m_dwHitIntervalTime = m_dwHitIntervalTime > 0 ? m_dwHitIntervalTime : 1000;

	m_dwCastIntervalTime = m_nReleaseInterval / (1 + (m_stAbility[AttrID::ReleaseSpeedPer] + nReleaseSpeed) / RatioDenom);
	m_dwCastIntervalTime = m_dwCastIntervalTime > 0 ? m_dwCastIntervalTime : 1000;

	m_dwMoveIntervalTime = m_nMoveInterval / (1 + (m_stAbility[AttrID::MoveSpeedPer] + nMoveSpeed) / RatioDenom);
	m_dwMoveIntervalTime = m_dwMoveIntervalTime > 0 ? m_dwMoveIntervalTime : 800;

	if (auto cof = m_cBuff.GetBuffCof(MAGICSTATE_SPEEDSLOW)) {
		m_dwMoveIntervalTime = m_dwMoveIntervalTime / (1 - 1.0 * cof / 10000);
	}
	return true;
}

bool CRobot::ResChange(ResID rid, int nChange, const char* szEvtName) {							// 资源更改
	FUNCTION_BEGIN;
	if (szEvtName == NULL || szEvtName[0] == 0 || nChange == 0)
		return false;
	if (!stRes::IsValidID(rid)) {
		return false;
	}
	int old = m_res[rid];
	int newVal = old + nChange;;
	switch (rid)
	{
	case ResID::charge:
	case ResID::chargebind:
	case ResID::game:
	case ResID::gamebind:
	case ResID::horus:
	case ResID::shellGold:
	case ResID::superShellGold:
	case ResID::ladderPoint:
	case ResID::daycharge:
	case ResID::hischarge:
	case ResID::buildDegree:
	case ResID::contribution:
	case ResID::battleExp:
	case ResID::citizenExp:
	case ResID::crime:
	case ResID::energy:
	case ResID::fatigue:
	case ResID::vitality:
	case ResID::pk:
		break;
	case ResID::exp:
		AddExp(nChange);
		break;
	case ResID::citizenLv:
		UpdateAppearance(FeatureIndex::nameColor, newVal);
		break;
	case ResID::attrPoint:
	case ResID::skillPoint:
	case ResID::superSkillPoint:
		if (nChange < 0 && newVal < 0)
			return false;
		break;
	case ResID::neng:
		newVal = max(0, newVal);
		newVal = min(CUserEngine::getMe().m_nNengLimit, newVal);
		break;
	default:
		break;
	}
	if (old != newVal)
	{
		m_res[rid] = newVal;
		stPlayerResChange retcmd;
		retcmd.btIndex = tosizet(rid);
		retcmd.nChange = newVal - old;
		retcmd.nNow = newVal;
		SendMsgToMe(&retcmd, sizeof(retcmd));
	}
	return true;
}
