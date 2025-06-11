#pragma once
#include "JsonStructBase.h"
struct mydb_mongen_tbl {
	int dwGenId; //本表编号
	int dwMapId; //地图ID
	int dwMonsterId; //怪物ID
	std::string szMonShowName; //显示名称
	int nX; //中心点X
	int nY; //中心点Y
	int nRange; //刷新半径
	int nCount; //刷新个数
	int dwGenTime; //刷新时间间隔
	int dwGenTimetype; //刷新时间间隔类型
};
#if __cplusplus < 202002L
YLT_REFL(mydb_mongen_tbl, dwGenId, dwMapId, dwMonsterId, szMonShowName, nX, nY, nRange, nCount, dwGenTime, dwGenTimetype)
#endif
struct mydb_npcgen_tbl {
	int id; //NPC唯一标识符
	uint8_t deleted; //NPC是否删除
	std::string name; //NPC显示名称
	int type; //NPC类型标识
	std::string mapName; //所属地图名称
	int mapId; //所属地图ID
	int scriptId; //关联的脚本ID
	int monsterId; //关联的怪物模板ID
	int x; //生成坐标X轴
	int y; //生成坐标Y轴
	int direction; //生成时的朝向角度
};
#if __cplusplus < 202002L
YLT_REFL(mydb_npcgen_tbl, id, deleted, name, type, mapName, mapId, scriptId, monsterId, x, y, direction)
#endif
struct mydb_playerability_tbl {
	int dwLevel; //等级
	int dwJob; //职业
	int i64NeedExp; //战斗经验
	int dwHp; //生命
	int dwMp; //精神
	int dwPP; //体力
	int dwMinPAtk; //物攻下限
	int dwMaxPAtk; //物攻
	int dwMaxMAtk; //魔攻
	int dwPDef; //物防
	int dwMDef; //魔防
	int dwHit; //命中率
	int dwJuck; //闪避率
	int nHpRestore; //生命回复
	int nMpRestore; //精神回复
	int nPpRestore; //体力回复
	int nAttackInterval; //攻击间隔（毫秒）
	int nReleaseInterval; //释放间隔（毫秒）
	int nMoveInterval; //移动间隔（毫秒）
	int nPveCof; //PVE系数
	int nPvpCof; //PVP系数
	int nPpMoveCost; //体力移动消耗
	int nKillMaxMonLv; //击杀最大怪物等级（影响击杀获得奖励）
	int nKillMinMonLv; //击杀最小怪物等级（影响击杀获得奖励）
};
#if __cplusplus < 202002L
YLT_REFL(mydb_playerability_tbl, dwLevel, dwJob, i64NeedExp, dwHp, dwMp, dwPP, dwMinPAtk, dwMaxPAtk, dwMaxMAtk, dwPDef, dwMDef, dwHit, dwJuck, nHpRestore, nMpRestore, nPpRestore, nAttackInterval, nReleaseInterval, nMoveInterval, nPveCof, nPvpCof, nPpMoveCost, nKillMaxMonLv, nKillMinMonLv)
#endif
struct mydb_mapgate_tbl {
	int dwSrcMapId; //来源地图编号
	int16_t wSx; //传送口起点 X 坐标
	int16_t wSy; //传送口起点 Y 坐标
	int dwDstMapId; //目标地图编号
	int16_t wDx; //传送口目标 X 坐标
	int16_t wDy; //传送口目标 Y 坐标
	int nscriptidx; //触发脚本索引
};
#if __cplusplus < 202002L
YLT_REFL(mydb_mapgate_tbl, dwSrcMapId, wSx, wSy, dwDstMapId, wDx, wDy, nscriptidx)
#endif
struct Config_Chat {
	uint32_t dwId; //聊天id
	std::string szChatType; //聊天类型
	uint8_t boCountryLimit; //聊天国家限制
	uint32_t dwLevel; //聊天需要等级
	uint32_t dwZsLevel; //聊天需要转生
	uint32_t dwGold; //聊天需要金钱
	uint32_t dwRumorsExposureRate; //聊天曝光率
	uint32_t dwTime; //聊天间隔时间
	uint32_t dwCount; //聊天发言次数
	uint32_t dwItemId; //聊天需要道具
	uint32_t dwMaxChatLength; //聊天最大长度
	uint32_t nVipLevel; //聊天VIP等级
};
#if __cplusplus < 202002L
YLT_REFL(Config_Chat, dwId, szChatType, boCountryLimit, dwLevel, dwZsLevel, dwGold, dwRumorsExposureRate, dwTime, dwCount, dwItemId, dwMaxChatLength, nVipLevel)
#endif
struct mydb_magicbuff_tbl {
	uint32_t dwID; //BUFFID
	std::string szName; //BUFF名字
	uint8_t btStateType; //BUFF状态类型
	uint8_t btLevel; //BUFF等级
	uint16_t wBuffNum; //BUFF效果值
	int16_t wBuffCof; //BUFF效果系数
	uint8_t btTimeType; //BUFF时间类型
	int64_t i64KeepTime; //持续时间毫秒
	uint32_t dwTimeInterval; //间隔时间毫秒
	uint8_t boCover; //是否覆盖
	uint8_t boOverlap; //是否叠加
	uint8_t boDieRemove; //死亡消失
	uint8_t boOfflineRemove; //下线消失
	uint8_t boTimeOutRemove; //超时消失
	uint8_t boDisperse; //是否驱散
	uint8_t btSave; //是否保存
	uint32_t dwEffectId; //BUFF效果ID
	std::string szMutexBuff; //互斥BUFF
	uint32_t dwBuffRepel; //BUFF排斥
	uint32_t dwBuffCancel; //BUFF移除
	uint16_t wFormulaNum; //公式编号
	std::string szBuffTip; //Bufftips
	uint8_t boShow; //是否显示
	int32_t nDura; //BUFF耐久值
};
#if __cplusplus < 202002L
YLT_REFL(mydb_magicbuff_tbl, dwID, szName, btStateType, btLevel, wBuffNum, wBuffCof, btTimeType, i64KeepTime, dwTimeInterval, boCover, boOverlap, boDieRemove, boOfflineRemove, boTimeOutRemove, boDisperse, btSave, dwEffectId, szMutexBuff, dwBuffRepel, dwBuffCancel, wFormulaNum, szBuffTip, boShow, nDura)
#endif
struct mydb_mapinfo13824_tbl {
	uint32_t dwMapID; //id
	uint8_t deleted; //地图是否不加载
	uint32_t dwMapFileID; //mapfileid
	std::string szMapFileName; //mapfilename
	std::string szName; //name
	int32_t nServerIndex; //serverindex
	int32_t nMinMapIdx; //minmapidx
	uint32_t dwMapscriptid; //脚本编号
	uint32_t dwReliveMapid; //复活点地图ID
	uint32_t dwReliveX; //X坐标
	uint32_t dwReliveY; //Y坐标
	uint32_t dwReliveRange; //范围
	uint32_t dwMapProperty; //mapproperty
	uint8_t CrossLoad; //跨服加载
	uint8_t LimitOpenday; //开区天数限制
	uint8_t CrossRefreshMon; //CrossMap
	int32_t nViewChangeX; //视野改变x
	int32_t nViewChangeY; //视野改变y
	uint8_t isClone; //是否副本
};
#if __cplusplus < 202002L
YLT_REFL(mydb_mapinfo13824_tbl, dwMapID, deleted, dwMapFileID, szMapFileName, szName, nServerIndex, nMinMapIdx, dwMapscriptid, dwReliveMapid, dwReliveX, dwReliveY, dwReliveRange, dwMapProperty, CrossLoad, LimitOpenday, CrossRefreshMon, nViewChangeX, nViewChangeY, isClone)
#endif
struct mydb_mondropitem_tbl {
	int32_t nID; //怪物编号
	std::string szName; //怪物名称
	std::string szMapName; //刷新地图
	uint32_t dwMaxDropCount; //最大掉落数量
	std::vector<stDropItemBase> vItems; //掉落
};
#if __cplusplus < 202002L
YLT_REFL(mydb_mondropitem_tbl, nID, szName, szMapName, dwMaxDropCount, vItems)
#endif
struct mydb_monster_tbl {
	int32_t nID; //怪物编号
	std::string szMapName; //刷新地图
	std::string szName; //怪物名称
	int32_t nNameColor; //怪物名称颜色
	int32_t face_Id; //头像图档编号
	int32_t feature_id; //造型图档编号
	int32_t weapon_id; //武器图档编号
	uint8_t btSex; //性别
	uint8_t btJob; //职业
	int32_t monster_type; //怪物类型
	int32_t nLevel; //等级
	int64_t nMaxHP; //最大血量
	int64_t nMaxMP; //最大蓝量
	int64_t nMaxPAtk; //物理攻击上限值
	int64_t nMinPAtk; //物理攻击下限值
	int64_t nMaxMAtk; //魔攻上限
	int64_t nMinMAtk; //魔攻下限
	int32_t nPDef; //物理防御
	int32_t nMDef; //魔法抵抗
	int32_t nHit; //命中率
	int32_t nJuck; //闪避率
	int32_t nHitVal; //命中值
	int32_t nJuckVal; //闪避值
	int32_t nHpRestore; //每次恢复血量
	int32_t nMpRestore; //每次恢复蓝量
	int32_t nCritRate; //暴击率
	int32_t nCritDefense; //暴伤抵抗
	int32_t nAtkCrit; //暴击伤害
	int32_t nCritResist; //抗暴率
	int32_t ViewRange; //视野范围
	int32_t ActivitiesRange; //活动范围
	int32_t CastingRange; //施法范围
	int32_t AttackRange; //攻击范围
	uint32_t CastingTick; //施法间隔
	uint32_t AttackTick; //攻击间隔
	uint32_t MoveTick; //移动间隔
	uint32_t HomeTick; //回出生点时间
	int32_t MoveSpeed; //移动速度
	uint8_t BoRestoreHp; //是否自动回血
	uint8_t BoMonRecordStatus; //是否记录状态
	uint8_t ActiveAttack; //是否主动攻击
	uint8_t CanAttack; //是否能攻击
	uint8_t CanMove; //是否能移动
	uint8_t CanFly; //是否能瞬移
	uint8_t ImmunePoisoning; //是否免疫施毒术
	uint8_t boNoByPush; //禁止被推移
	uint8_t boNoParalysis; //禁止被麻痹
	uint8_t CanDir; //是否能转向
	uint8_t MonDir; //怪物方向
	std::vector<std::array<int, 2>> SkillNum; //技能编号
	int32_t BossAiID; //BOSSAI编号
	uint8_t Isboss; //是否BOSS
	uint8_t boOwner; //是否有归属权
	uint8_t boShareOut; //是否共享产出
	int32_t CanHaveExp; //经验值
	int32_t nCitizenValue; //市民经验
	int32_t nBattleExp; //战斗经验
	int32_t nMinGame; //决战币下限
	int32_t nMaxGame; //决战币上限
	int32_t AudioId; //怪物音效
	uint8_t isShowInMap; //小地图显示
	uint8_t btMonRace; //怪物种族
	int32_t nDecAttackDamage; //伤害减免
	int32_t nShenlongPoint; //神龙币
};
#if __cplusplus < 202002L
YLT_REFL(mydb_monster_tbl, nID, szMapName, szName, nNameColor, face_Id, feature_id, weapon_id, btSex, btJob, monster_type, nLevel, nMaxHP, nMaxMP, nMaxPAtk, nMinPAtk, nMaxMAtk, nMinMAtk, nPDef, nMDef, nHit, nJuck, nHitVal, nJuckVal, nHpRestore, nMpRestore, nCritRate, nCritDefense, nAtkCrit, nCritResist, ViewRange, ActivitiesRange, CastingRange, AttackRange, CastingTick, AttackTick, MoveTick, HomeTick, MoveSpeed, BoRestoreHp, BoMonRecordStatus, ActiveAttack, CanAttack, CanMove, CanFly, ImmunePoisoning, boNoByPush, boNoParalysis, CanDir, MonDir, SkillNum, BossAiID, Isboss, boOwner, boShareOut, CanHaveExp, nCitizenValue, nBattleExp, nMinGame, nMaxGame, AudioId, isShowInMap, btMonRace, nDecAttackDamage, nShenlongPoint)
#endif
struct mydb_submondropitem_tbl {
	int32_t nID; //爆率编号
	std::string szName; //爆率名称
	uint32_t dwSubMaxDropCount; //最大掉落数量
	std::vector<std::array<int, 2>> szSubDropItems; //子爆率物品表
};
#if __cplusplus < 202002L
YLT_REFL(mydb_submondropitem_tbl, nID, szName, dwSubMaxDropCount, szSubDropItems)
#endif
struct mydb_magic_tbl {
	int32_t nID; //技能编号
	std::string szName; //技能名称
	uint8_t btlevel; //技能等级
	uint8_t btActiveType; //主动被动
	uint8_t btjob; //技能职业
	uint8_t btAttackType; //近身还是远程
	uint8_t nDamageType; //物理还是魔法
	uint8_t btMagicBattleType; //魔法战斗类型
	uint8_t btMagicFuncType; //魔法功能类型
	uint8_t bSafeZone; //是否安全区可以释放 0不可以 1可以
	uint8_t bSelect; //是否选中目标 0不选中 1选中
	uint8_t btSkillModel; //技能模式
	uint8_t boSelf; //是否包括自身
	uint8_t btEnemyType; //友方还是敌方
	uint32_t dwCDbyTime; //冷却时间，毫秒计
	int32_t nSuccessCof; //技能成功率
	int32_t nSkillAddition; //技能追加
	int32_t nSkillHit; //技能追加命中率
	int32_t nDamage; //攻击伤害值
	int32_t nDamageCof; //攻击伤害系数
	uint16_t wSkillNeedLv; //学习需要等级
	int32_t dwNeedGameCcy; //学习需要决战币
	int32_t dwNeedBattleExp; //学习需要战斗经验
	int32_t dwNeedMP; //消耗的MP，一般道士、法师用
	int32_t dwNeedMPCof; //消耗的MP系数
	int32_t dwNeedHp; //使用消耗生命值
	int32_t dwNeedPp; //使用消耗体力值
	int32_t dwNeedEnergy; //使用消耗特殊能量
	uint8_t boDie; //是否包括死亡
	int32_t btShape; //点面，技能范围
	uint8_t btMaxRange; //最大攻击范围
	uint16_t wAttackNum; //攻击的个数
	int32_t duration; //地图魔法的持续时间
	int32_t intervalTime; //地图魔法的攻击间隔
	int32_t nResetCdCof; //重置CD概率
	uint8_t boPublicCD; //是否公共CD
	uint8_t boDir; //魔法方向
	uint8_t btAtomType; //魔法元素
	std::vector<int> szSelfBuffID; //SELFBUFFID
	std::vector<int> szBuffID; //BUFFID
	uint8_t bBattleTmp; //是否战斗临时加成
	uint32_t dwEffectid; //效果ID
	uint16_t wFormulaNum; //公式编号
	std::string szMutexesSkills; //互斥技能
	int32_t nSkillActionId; //技能特效ID
	uint8_t boLocked; //自动锁定的默认值
	uint8_t boLockChange; //自动锁定修改
	uint8_t boContinuousCasting; //连续施放的默认值
	uint8_t boContinuousCastChange; //连续施放修改
	uint8_t btSkillHit; //技能必中 1必中
};
#if __cplusplus < 202002L
YLT_REFL(mydb_magic_tbl, nID, szName, btlevel, btActiveType, btjob, btAttackType, nDamageType, btMagicBattleType, btMagicFuncType, bSafeZone, bSelect, btSkillModel, boSelf, btEnemyType, dwCDbyTime, nSuccessCof, nSkillAddition, nSkillHit, nDamage, nDamageCof, wSkillNeedLv, dwNeedGameCcy, dwNeedBattleExp, dwNeedMP, dwNeedMPCof, dwNeedHp, dwNeedPp, dwNeedEnergy, boDie, btShape, btMaxRange, wAttackNum, duration, intervalTime, nResetCdCof, boPublicCD, boDir, btAtomType, szSelfBuffID, szBuffID, bBattleTmp, dwEffectid, wFormulaNum, szMutexesSkills, nSkillActionId, boLocked, boLockChange, boContinuousCasting, boContinuousCastChange, btSkillHit)
#endif
struct mydb_specialeffect_tbl {
	uint32_t dwID; //效果ID
	std::string szName; //效果名字
	uint32_t dwNextId; //升级后效果ID
	int32_t nFullSkillLv; //所有技能等级
	int32_t nHundSkillLv; //百级技能等级
	int32_t nFirearmsMasterLv; //枪械大师等级
	int32_t nContinuousFiringLv; //连射等级
	int32_t nSnipeLv; //狙击等级
	int32_t nLivelyLv; //灵动等级
	int32_t nSteelSkinLv; //钢铁皮肤等级
	int32_t nThrillLv; //兴奋等级
	int32_t nCounterpunchLv; //反击等级
	int32_t nSelfMedicationLv; //自我医疗等级
	int32_t nRestoreLv; //回复等级
	int32_t nConcentrationLv; //精神集中等级
	int32_t nJuckLv; //闪避等级
	int32_t nPunctureLv; //穿刺等级
	int32_t nMuseLv; //冥思等级
	int32_t nSpiritStrengthenLv; //精神强化等级
	int32_t nMagicRefineLv; //魔法精修等级
	int32_t nSpellMasterLv; //法术大师等级
};
#if __cplusplus < 202002L
YLT_REFL(mydb_specialeffect_tbl, dwID, szName, dwNextId, nFullSkillLv, nHundSkillLv, nFirearmsMasterLv, nContinuousFiringLv, nSnipeLv, nLivelyLv, nSteelSkinLv, nThrillLv, nCounterpunchLv, nSelfMedicationLv, nRestoreLv, nConcentrationLv, nJuckLv, nPunctureLv, nMuseLv, nSpiritStrengthenLv, nMagicRefineLv, nSpellMasterLv)
#endif
struct mydb_effect_base_tbl {
	uint32_t dwID; //效果ID
	std::string szName; //效果名字
	uint32_t dwNextId; //升级后效果ID
	int32_t attrs_Strength; //力量
	int32_t attrs_Physique; //体质
	int32_t attrs_Agility; //敏捷
	int32_t attrs_Wisdom; //智慧
	int32_t attrs_Intelligence; //智力
	int32_t attrs_MinPAtk; //物攻下限
	int32_t attrs_MaxPAtk; //物攻上限
	int32_t attrs_MinMAtk; //魔攻下限
	int32_t attrs_MaxMAtk; //魔攻上限
	int32_t attrs_PAtkPer; //物攻加成
	int32_t attrs_MAtkPer; //魔攻加成
	int32_t attrs_AllAttackAdd; //物攻魔攻增加
	int32_t attrs_PAtkInc; //物攻上下限增加
	int32_t attrs_MAtkInc; //魔攻上下限增加
	int32_t attrs_AttackAdd; //攻击固定增加
	int32_t attrs_AttackDec; //攻击固定减少
	int32_t attrs_PDef; //物防
	int32_t attrs_MDef; //魔防
	int32_t attrs_PDefPer; //物防加成
	int32_t attrs_MDefPer; //魔防加成
	int32_t attrs_MaxHP; //血量
	int32_t attrs_MaxHpPer; //血量加成
	int32_t attrs_HpRestore; //血量恢复
	int32_t attrs_HpRestorePer; //血量恢复加成
	int32_t attrs_FinalMaxHp; //最终血量加成
	int32_t attrs_MaxMP; //精神
	int32_t attrs_MaxMpPer; //精神加成
	int32_t attrs_MpRestore; //精神恢复
	int32_t attrs_MpRestorePer; //精神恢复加成
	int32_t attrs_MpCostPer; //精神消耗减少率
	int32_t attrs_MpCost; //精神消耗减少值
	int32_t attrs_FinalMaxMp; //最终蓝量加成
	int32_t attrs_MaxPP; //体力
	int32_t attrs_MaxPpPer; //体力加成
	int32_t attrs_PpRestore; //体力恢复
	int32_t attrs_PpRestorePer; //体力恢复加成
	int32_t attrs_AttackSpeedPer; //攻击速度
	int32_t attrs_AttackSpeedPhase; //攻击速度阶段
	int32_t attrs_MoveSpeedPer; //移动速度
	int32_t attrs_MoveSpeedPhase; //移动速度阶段
	int32_t attrs_ReleaseSpeedPer; //施法速度
	int32_t attrs_ReleaseSpeedPhase; //施法速度阶段
	int32_t attrs_CritRate; //暴击几率
	int32_t attrs_CritMul; //暴伤倍率
	int32_t attrs_CritResist; //抗暴率
	int32_t attrs_CritDec; //暴伤减免
	int32_t attrs_PunctureRate; //穿刺率
	int32_t attrs_MPunctureRate; //魔法穿刺率
	int32_t attrs_PunctureDecRate; //穿刺减免率
	int32_t attrs_MPunctureDecRate; //魔法穿刺减免率
	int32_t attrs_ReflectMul; //反射倍率
	int32_t attrs_ReflectDecRate; //反射减免率
	int32_t attrs_RealDamageInc; //真伤
	int32_t attrs_RealDamageDec; //真伤减少
	int32_t attrs_RealDamagePer; //真伤加成
	int32_t attrs_RealDamageDecPer; //真伤减免加成
	int32_t attrs_RangeAtkMul; //范围攻击倍率
	int32_t attrs_SuckBloodRate; //吸血概率
	int32_t attrs_SuckBloodMul; //吸血倍率
	int32_t attrs_MonDamagePer; //打怪伤害加成
	int32_t attrs_PvpMul; //PVP倍率
	int32_t attrs_PveMul; //PVE倍率
	int32_t attrs_FixAddDamage; //固定值增伤
	int32_t attrs_FixDecDamage; //固定值减伤
	int32_t attrs_DamageAddPer; //伤害增加
	int32_t attrs_DamageDecPer; //伤害减免
	int32_t attrs_PDropRate; //人物爆率
	int32_t attrs_ExpMul; //经验倍率
	int32_t attrs_BattleExpMul; //战斗经验倍率
	int32_t attrs_BlueEquipRate; //蓝色装备获取率
	int32_t attrs_GoldEquipRate; //金色装备获取率
	int32_t attrs_WeaponStrengthenLv; //增加穿戴武器强化等级
	int32_t attrs_ArmorStrengthenLv; //增加穿戴防具强化等级
	int32_t attrs_BossDamageInc; //BOSS克制值
	int32_t attrs_BossDamageIncPer; //BOSS克制率
	int32_t attrs_BossDamageDec; //BOSS抵御值
	int32_t attrs_BossDamageDecPer; //BOSS抵御率
	int32_t attrs_Hit; //命中率
	int32_t attrs_Juck; //闪避率
	int32_t attrs_HitVal; //命中值
	int32_t attrs_JuckVal; //闪避值
	int32_t attrs_HPPotionPer; //血药加成
	int32_t attrs_MPPotionPer; //蓝药加成
	int32_t attrs_PPPotionPer; //体力药加成
	int32_t attrs_StrToAtk; //力量转攻击力
	int32_t attrs_StrToPP; //力量转体力
	int32_t attrs_PhyToHP; //体质转生命值
	int32_t attrs_PhyToDef; //体质转防御力
	int32_t attrs_AgiToAtk; //敏捷转攻击力
	int32_t attrs_AgiToHit; //敏捷转命中率
	int32_t attrs_AgiToJuck; //敏捷转回避率
	int32_t attrs_WisToAtk; //智慧转攻击力
	int32_t attrs_WisToDef; //智慧转防御力
	int32_t attrs_WisToMP; //智慧转精神
	int32_t attrs_CounterCoeff; //反击攻击力系数
	int32_t attrs_CounterInc; //反击攻击力增加
	int32_t attrs_AgilityToCounterDec; //反击攻击力的减少量为敏捷的百分比
	int32_t attrs_PhysiqueToCounterDec; //反击攻击力的减少量为体质的百分比
	int32_t attrs_HpRestoreFiveSec; //每5秒恢复生命
	int32_t attrs_MpRestoreFiveSec; //每5秒恢复精神
	int32_t attrs_PpRestoreFiveSec; //每5秒恢复体力
	int32_t attrs_GainTime; //增益时间
	int32_t attrs_WisdomToGainTimePer; //增益时间增幅为智慧的百分比
	int32_t attrs_MagicDmgAdd; //魔法伤害加成
	int32_t attrs_PhysDmgAdd; //物理伤害加成
	int32_t attrs_MagicDmgDec; //魔法伤害减免
	int32_t attrs_PhysDmgDec; //物理伤害减免
	int32_t attrs_DrugRestoreSpeed; //药物恢复速度
	int32_t attrs_DrugCdPhase; //药物冷却阶段
	int32_t attrs_FiveDimAttr; //五维属性
	int32_t attrs_GoldAddPer; //金钱增加
};
#if __cplusplus < 202002L
YLT_REFL(mydb_effect_base_tbl, dwID, szName, dwNextId, attrs_Strength, attrs_Physique, attrs_Agility, attrs_Wisdom, attrs_Intelligence, attrs_MinPAtk, attrs_MaxPAtk, attrs_MinMAtk, attrs_MaxMAtk, attrs_PAtkPer, attrs_MAtkPer, attrs_AllAttackAdd, attrs_PAtkInc, attrs_MAtkInc, attrs_AttackAdd, attrs_AttackDec, attrs_PDef, attrs_MDef, attrs_PDefPer, attrs_MDefPer, attrs_MaxHP, attrs_MaxHpPer, attrs_HpRestore, attrs_HpRestorePer, attrs_FinalMaxHp, attrs_MaxMP, attrs_MaxMpPer, attrs_MpRestore, attrs_MpRestorePer, attrs_MpCostPer, attrs_MpCost, attrs_FinalMaxMp, attrs_MaxPP, attrs_MaxPpPer, attrs_PpRestore, attrs_PpRestorePer, attrs_AttackSpeedPer, attrs_AttackSpeedPhase, attrs_MoveSpeedPer, attrs_MoveSpeedPhase, attrs_ReleaseSpeedPer, attrs_ReleaseSpeedPhase, attrs_CritRate, attrs_CritMul, attrs_CritResist, attrs_CritDec, attrs_PunctureRate, attrs_MPunctureRate, attrs_PunctureDecRate, attrs_MPunctureDecRate, attrs_ReflectMul, attrs_ReflectDecRate, attrs_RealDamageInc, attrs_RealDamageDec, attrs_RealDamagePer, attrs_RealDamageDecPer, attrs_RangeAtkMul, attrs_SuckBloodRate, attrs_SuckBloodMul, attrs_MonDamagePer, attrs_PvpMul, attrs_PveMul, attrs_FixAddDamage, attrs_FixDecDamage, attrs_DamageAddPer, attrs_DamageDecPer, attrs_PDropRate, attrs_ExpMul, attrs_BattleExpMul, attrs_BlueEquipRate, attrs_GoldEquipRate, attrs_WeaponStrengthenLv, attrs_ArmorStrengthenLv, attrs_BossDamageInc, attrs_BossDamageIncPer, attrs_BossDamageDec, attrs_BossDamageDecPer, attrs_Hit, attrs_Juck, attrs_HitVal, attrs_JuckVal, attrs_HPPotionPer, attrs_MPPotionPer, attrs_PPPotionPer, attrs_StrToAtk, attrs_StrToPP, attrs_PhyToHP, attrs_PhyToDef, attrs_AgiToAtk, attrs_AgiToHit, attrs_AgiToJuck, attrs_WisToAtk, attrs_WisToDef, attrs_WisToMP, attrs_CounterCoeff, attrs_CounterInc, attrs_AgilityToCounterDec, attrs_PhysiqueToCounterDec, attrs_HpRestoreFiveSec, attrs_MpRestoreFiveSec, attrs_PpRestoreFiveSec, attrs_GainTime, attrs_WisdomToGainTimePer, attrs_MagicDmgAdd, attrs_PhysDmgAdd, attrs_MagicDmgDec, attrs_PhysDmgDec, attrs_DrugRestoreSpeed, attrs_DrugCdPhase, attrs_FiveDimAttr, attrs_GoldAddPer)
#endif
struct mydb_item_base_tbl {
	int32_t nID; //物品编号
	int32_t nNextID; //物品升级编号
	std::string szName; //物品名称
	uint32_t dwNeedLevel; //等级需求
	uint32_t dwPhysique; //体质需求
	uint32_t dwStrength; //力量需求
	uint32_t dwAgility; //敏捷需求
	uint32_t dwWisdom; //智慧需求
	uint32_t dwBrains; //智力需求
	uint32_t dwType; //类型区分
	int32_t nQuality; //道具阶数
	int32_t nRare; //装备品质
	uint8_t btGuardType; //守护类型
	uint8_t btEquipStation; //穿戴位置
	uint8_t btSexType; //使用性别
	uint8_t btJobType; //职业需求
	uint8_t btBindType; //是否天生绑定
	uint8_t btMaxNpNum; //可拥有极品属性数
	uint8_t btShareCdType; //共享cd类型
	uint32_t dwWarriorEffectId; //战士效果ID
	uint32_t dwDrugBuffId; //药水BUFFID
	uint32_t dwSuitId; //套装效果ID
	int32_t nSuitType; //套装类型
	uint32_t dwDropMonEffId; //特殊属性效果ID
	uint32_t dwCdByTick; //物品冷却时间
	uint32_t dwDisappearTime; //物品消失时间
	uint8_t btItemLimitTimeType; //物品有效时间类型
	uint32_t dwItemLimitTime; //物品有效使用时间
	int32_t nBuyPrice; //购买价格
	int32_t nSellPrice; //卖出价格
	int32_t nSellGameCcy; //出售价格
	uint32_t dwFixCost; //维修费用
	uint32_t dwBaseFixCost; //基础修理费用
	int32_t nFaceId; //外观编号
	uint8_t btFaceJob; //外观职业限定
	uint8_t btFaceSex; //外观性别限定
	int32_t nIconId; //ICON编号
	uint8_t btColor; //名字颜色
	int32_t nLightBeam; //光柱
	uint32_t dwMaxCount; //最大叠加数量
	uint32_t dwMaxDura; //最大耐久
	uint32_t nVariableMaxCount; //可变最大叠加数量
	int8_t boNoDestory; //不能摧毁
	uint32_t dwScriptid; //脚本编号
	int32_t DataSYNum; //日用次数
	uint8_t btCanPutTmpBag; //可放入临时背包
	int32_t nRecycleExp; //回收经验
	int32_t nRecycleYuanBao; //回收元宝
	std::vector<std::array<int, 2>> vRecycleItem; //回收可获得道具
	int32_t nMinRndAtt; //词条最小数量
	int32_t nMaxRndAtt; //词条最大数量
	uint8_t btNotice; //公告天数
	uint32_t dwLog; //日志数量
	int32_t nLogType; //日志类型
	int32_t nLogOpd; //日志天数
	int32_t dwActionType; //交易行类型
	int32_t dwActionSubType; //交易行子类型
	int32_t nPrivilegeUseLimit; //特权使用限制
	int32_t nSortType; //排序类型
	int32_t nSubSortType; //排序子类型
};
#if __cplusplus < 202002L
YLT_REFL(mydb_item_base_tbl, nID, nNextID, szName, dwNeedLevel, dwPhysique, dwStrength, dwAgility, dwWisdom, dwBrains, dwType, nQuality, nRare, btGuardType, btEquipStation, btSexType, btJobType, btBindType, btMaxNpNum, btShareCdType, dwWarriorEffectId, dwDrugBuffId, dwSuitId, nSuitType, dwDropMonEffId, dwCdByTick, dwDisappearTime, btItemLimitTimeType, dwItemLimitTime, nBuyPrice, nSellPrice, nSellGameCcy, dwFixCost, dwBaseFixCost, nFaceId, btFaceJob, btFaceSex, nIconId, btColor, nLightBeam, dwMaxCount, dwMaxDura, nVariableMaxCount, boNoDestory, dwScriptid, DataSYNum, btCanPutTmpBag, nRecycleExp, nRecycleYuanBao, vRecycleItem, nMinRndAtt, nMaxRndAtt, btNotice, dwLog, nLogType, nLogOpd, dwActionType, dwActionSubType, nPrivilegeUseLimit, nSortType, nSubSortType)
#endif
