#pragma once
#include <array>
constexpr float Denom = 10000.0f;

// 重载 std::array 的 + 运算符
template <typename T, uint8_t N>
std::array<T, N> operator+(const std::array<T, N>& arr1, const std::array<T, N>& arr2) {
    std::array<T, N> result;
    for (uint8_t i = 0; i < N; ++i) {
        result[i] = arr1[i] + arr2[i];
    }
    return result;
}

// 重载 std::array 的 - 运算符
template <typename T, uint8_t N>
std::array<T, N> operator-(const std::array<T, N>& arr1, const std::array<T, N>& arr2) {
    std::array<T, N> result;
    for (uint8_t i = 0; i < N; ++i) {
        result[i] = arr1[i] - arr2[i];
    }
    return result;
}

// 重载 std::array 的 += 运算符
template <typename T, uint8_t N>
std::array<T, N>& operator+=(std::array<T, N>& arr1, const std::array<T, N>& arr2) {
    for (uint8_t i = 0; i < N; ++i) {
        arr1[i] += arr2[i];
    }
    return arr1;
}

// 重载 std::array 的 -= 运算符
template <typename T, uint8_t N>
std::array<T, N>& operator-=(std::array<T, N>& arr1, const std::array<T, N>& arr2) {
    for (uint8_t i = 0; i < N; ++i) {
        arr1[i] -= arr2[i];
    }
    return arr1;
}

// 属性类型
enum class AttrID : uint8_t {
    None = 0,           // 配合lua从1开始
    Strength = 1,       // 力量
    Physique = 2,       // 体质
    Agility = 3,        // 敏捷
    Wisdom = 4,         // 智慧
    Intelligence = 5,   // 智力
    MaxPAtk = 6,        // 物攻上限
    MinPAtk = 7,        // 物攻下限
    MaxMAtk = 8,        // 魔攻上限
    MinMAtk = 9,        // 魔攻下限
    PAtkInc = 10,       // 物理攻击 ddd --说明:同时增加物理攻击上限和下限 公式 取物理攻击值公式
    MAtkInc = 11,       // 魔法攻击 ddd --说明:同时增加魔法攻击上限和下限 公式 取物理攻击值公式
    PAtkPer = 12,       // 物攻加成
    MAtkPer = 13,       // 魔攻加成
    PDef = 14,          // 物防
    PDefPer = 15,       // 物防加成
    MDef = 16,          // 魔防
    MDefPer = 17,       // 魔防加成
    MaxHP = 18,         // 最大血量
    MaxHpPer = 19,      // 最大血量加成
    HpRestore = 20,     // 血量恢复
    HpRestorePer = 21,  // 血量恢复加成
    FinalMaxHp = 22,    // 最终血量加成
    MaxMP = 23,         // 最大精神(蓝量)
    MaxMpPer = 24,      // 最大精神（蓝量）加成
    MpRestore = 25,     // 精神恢复
    MpRestorePer = 26,  // 精神恢复加成
    MpCostPer = 27,     // 精神消耗减少百分比
    MpCost = 28,        // 精神消耗减少固定值
    FinalMaxMp = 29,    // 最终精神(蓝量)加成
    MaxPP = 30,         // 最大体力
    MaxPpPer = 31,      // 最大体力加成
    PpRestore = 32,     // 体力恢复
    PpRestorePer = 33,  // 体力恢复加成
    AttackSpeedPer = 34,        // 攻击速度      ddd  见攻速公式
    AttackSpeedPhase = 35,      // 攻击速度阶段  ddd  见攻速公式
    MoveSpeedPer = 36,          // 移动速度
    MoveSpeedPhase = 37,        // 移动速度阶段
    ReleaseSpeedPer = 38,       // 施法速度      ddd 见施法速度公式
    ReleaseSpeedPhase = 39,     // 施法速度阶段  ddd 见施法速度公式
    CritRate = 40,              // 暴击几率
    CritMul = 41,               // 暴击伤害+%
    CritResist = 42,            // 抗暴率
    CritDec = 43,               // 暴伤减免(暴伤抵抗)
    PunctureRate = 44,          // 穿刺率 (无视物理防御)
    MPunctureRate = 45,         // 魔法穿刺率  (无视魔法防御)
    PunctureDecRate = 46,       // 穿刺减免率 (抵抗对方无视物理防御)
    MPunctureDecRate = 47,      // 魔法穿刺减免率 (抵抗对方无视魔法防御)
    ReflectMul = 48,            // 反射倍率		未实现
    ReflectDecRate = 49,        // 反射减免率
    SuckBloodRate = 50,         // 吸血概率		未实现
    SuckBloodMul = 51,          // 吸血倍率		未实现
    MonDamagePer = 52,          // 打怪伤害加成	未实现  
    PvpMul = 53,                // PVP倍率		  ddd	 伤害计算公式
    PveMul = 54,                // PVE倍率		  ddd	 伤害计算公式
    PDropRate = 55,             // 人物爆率
    ExpMul = 56,                // 经验倍率
    BlueEquipRate = 57,         // 蓝色装备获取率
    GoldEquipRate = 58,         // 金色装备获取率
    WeaponStrengthenLv = 59,    // 武器强化等级
    ArmorStrengthenLv = 60,     // 防具强化等级
    RealDamageInc = 61,         // 真伤			
    RealDamageDec = 62,         // 真伤减少
    RealDamagePer = 63,         // 真伤加成
    RealDamageDecPer = 64,      // 真伤减免加成
    Hit = 65,                   // 精确度(命中率)
    Juck = 66,                  // 敏捷度(闪避率)
    AttackAdd = 67,             // 攻击固定增加
    AttackDec = 68,             // 攻击固定减少
    FixAddDamage = 69,          // 固定值增伤
    FixDecDamage = 70,          // 固定值减伤
    DamageAddPer = 71,          // 伤害增加+%
    DamageDecPer = 72,          // 伤害减免+%
    BattleExpMul = 73,          // 战斗经验倍率
    HitVal = 74,                // 命中值  ddd 见命中公式
    JuckVal = 75,               // 闪避值  ddd 见命中公式
    BossDamageInc = 76,         // BOSS克制值(攻击BOSS时可造成更多的伤害数值) ddd  见伤害公式
    BossDamageIncPer = 77,      // BOSS克制率(攻击BOSS时可造成更多的伤害百分比) ddd 见伤害公式
    BossDamageDec = 78,         // BOSS抵御值(受到BOSS时可减少更多的伤害数值) ddd 见伤害公式
    BossDamageDecPer = 79,      // BOSS抵御率(受到BOSS时可减少更多的伤害百分比) ddd 见伤害公式
    HPPotionPer = 80,           // 血药加成  ddd 见药剂加成公式
    MPPotionPer = 81,           // 蓝药加成  ddd 见药剂加成公式
    PPPotionPer = 82,           // 体力药加成  ddd 见药剂加成公式
    Shield = 83,                // 护盾值	若有护盾值则优先扣除护盾值 ( effect 现在没有)
    RestoreShiled = 84,         // 护盾恢复值,单位时间恢复护盾的血值 (未实现 effect 现在没有)
    RestoreShiledRate = 85,     // 护盾恢复率,单位时间恢复百分之多少的护盾血量 (未实现 effect现在没有)
    StrengthToAtk = 86,         // 力量转攻击力 ddd 见角色属性计算公式
    StrengthToMaxPP = 87,       // 力量转体力 ddd 见角色属性计算公式
    PhysiqueToMaxHP = 88,       // 体质转生命值 ddd 见角色属性计算公式
    PhysiqueToDef = 89,         // 体质转防御力 ddd 见角色属性计算公式
    AgilityToAtk = 90,          // 敏捷转攻击力 ddd 见角色属性计算公式
    AgilityToHit = 91,          // 敏捷转命中率 ddd 见角色属性计算公式
    AgilityToJuck = 92,         // 敏捷转回避率 ddd 见角色属性计算公式
    WisdomToAtk = 93,           // 智慧转攻击力 ddd 见角色属性计算公式
    WisdomToDef = 94,		    // 智慧转防御力 ddd 见角色属性计算公式
    WisdomToMaxMP = 95,		    // 智慧转精神 ddd 见角色属性计算公式
    HpRestoreFiveSec = 96,      // 每5秒恢复生命 ddd 见5秒恢复公式 
    MpRestoreFiveSec = 97,      // 每5秒恢复精神 ddd 见5秒恢复公式    
    PpRestoreFiveSec = 98,      // 每5秒恢复体力 ddd 见5秒恢复公式 
    CounterCoeff = 99,          // 反击攻击力系数 ddd 来源于被动技能，词条，作用于反击公式
    CounterInc = 100,           // 反击攻击力固定值增加量 ddd 作用于反击公式
    AgilityToCounterDec = 101,   // 反击攻击力固定值的减少量为敏捷的百分比 ddd  作用于反击公式
    PhysiqueToCounterDec = 102,  // 反击攻击力固定值的减少量为体质的百分比 ddd  作用于反击公式
    GainTime = 103,             // 增益时间增加  ddd   见增益时间公式
    GainTimePer = 104,          // 增益时间增幅 
    WisdomToGainTimePer = 105,  // 增益时间增幅为智慧的百分比 ddd  ddd   见增益时间公式
    MagicDmgAdd = 106,           // 魔法伤害加成 ddd	见伤害计算公式
    PhysDmgAdd = 107,           // 物理伤害加成 ddd 见伤害计算公式
    MagicDmgDec = 108,           // 魔法伤害减免 ddd 见伤害计算公式
    PhysDmgDec = 109,           // 物理伤害减免 ddd 见伤害计算公式
    DrugRestoreSpeed = 110,      // 药物恢复速度 ddd  	三药共CD,药剂缩短CD，百分比      见药剂加成公式
    DrugCdPhase = 111,          // 药物冷却阶段 ddd 三药共CD,药剂缩短CD 见药剂加成公式
    FiveDimAttr = 112,          // 五维属性 ddd --说明:力量,体质,敏捷,智慧,智力都增加1  ddd 见角色属性计算公式
    AllAttackAdd = 113,         // 攻击力增加（增加物攻魔攻）
	RangeAttackMul = 114,       // 范围攻击% (对范围内敌对目标操作百分比伤害) 未实现
    GoldAddPer = 115,           // 金钱增加%
    Count = 116,                // 总数（自动计算，保持最后一位）
};
