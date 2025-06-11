#include "Buff.h"
#include "BaseCreature.h"
#include "PlayerObj.h"
#include "UsrEngn.h"

//////////////////////////////////////////////////////////////////////////
// 状态存在
BYTE Buff_0(CCreature* Owner, stBuff* pBuff, int& nNum) {
	if (Owner && pBuff) {
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = true;
			pBuff->m_emRunType = BUFF_RUN;
		}break;
		case BUFF_RUN:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = true;
		}break;
		case BUFF_DEL:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = false;
		}break;
		}
	}
	return 0;
}

//HP术
BYTE Buff_1(CCreature* Owner,stBuff* pBuff,int& nNum){
	if (Owner && pBuff){
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				if (Owner->m_nNowHP > 0){
					auto effectDataBase = pBuff->GetBuffDataBase()->GetEffectDataBase();
					auto hp = effectDataBase->attrs[static_cast<size_t>(AttrID::HpRestore)];
					auto mp = effectDataBase->attrs[static_cast<size_t>(AttrID::MpRestore)];
					Owner->StatusValueChange(stCretStatusValueChange::hp, hp, __FUNC_LINE__);
					Owner->StatusValueChange(stCretStatusValueChange::mp, mp, __FUNC_LINE__);
				}
				pBuff->m_emRunType=BUFF_RUN;
			}break;
		case BUFF_RUN:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				if (Owner->m_nNowHP>0){
					auto effectDataBase = pBuff->GetBuffDataBase()->GetEffectDataBase();
					auto hp = effectDataBase->attrs[static_cast<size_t>(AttrID::HpRestore)];
					auto mp = effectDataBase->attrs[static_cast<size_t>(AttrID::MpRestore)];
					Owner->StatusValueChange(stCretStatusValueChange::hp, hp, __FUNC_LINE__);
					Owner->StatusValueChange(stCretStatusValueChange::mp, mp, __FUNC_LINE__);
				}
			}break;
		case BUFF_DEL:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=false;
			}break;
		}
	}
	return 0;
}

//MP术
BYTE Buff_2(CCreature* Owner,stBuff* pBuff,int& nNum){
	if (Owner && pBuff){
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				auto effectDataBase = pBuff->GetBuffDataBase()->GetEffectDataBase();
				auto mp = effectDataBase->attrs[static_cast<size_t>(AttrID::MpRestore)];
				Owner->DamageSpell(mp);
				pBuff->m_emRunType=BUFF_RUN;
			}break;
		case BUFF_RUN:
			{
				auto effectDataBase = pBuff->GetBuffDataBase()->GetEffectDataBase();
				auto mp = effectDataBase->attrs[static_cast<size_t>(AttrID::MpRestore)];
				Owner->DamageSpell(mp);
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				Owner->DamageSpell(mp);
			}break;
		case BUFF_DEL:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=false;
			}break;
		}
	}
	return 0;
}

//复活术
BYTE Buff_3(CCreature* Owner,stBuff* pBuff,int& nNum){
	if (Owner && pBuff){
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				if (Owner->isPlayer() && Owner->isDie()){
					auto effectDataBase = pBuff->GetBuffDataBase()->GetEffectDataBase();
					auto hp = effectDataBase->attrs[static_cast<size_t>(AttrID::HpRestore)];
					auto mp = effectDataBase->attrs[static_cast<size_t>(AttrID::MpRestore)];
					Owner->Relive(hp, mp);
				}
				pBuff->m_emRunType=BUFF_RUN;
			}break;
		case BUFF_RUN:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
			}break;
		case BUFF_DEL:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=false;
			}break;
		}
	}
	return 0;
}

//减速
BYTE Buff_4(CCreature* Owner,stBuff* pBuff,int& nNum){
	if (Owner && pBuff){
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				pBuff->m_emRunType=BUFF_RUN;
				Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
				Owner->UpdateAppearance(FeatureIndex::boFeature, Owner->m_cBuff.BuffFeature());
			}break;
		case BUFF_RUN:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
			}break;
		case BUFF_DEL:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=false;
				Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
				Owner->UpdateAppearance(FeatureIndex::boFeature, Owner->m_cBuff.BuffFeature());

			}break;
		}
	}
	return 0;
}

//晕眩
BYTE Buff_5(CCreature* Owner,stBuff* pBuff,int& nNum){
	if (Owner && pBuff){
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				pBuff->m_emRunType=BUFF_RUN;
			}break;
		case BUFF_RUN:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
			}break;
		case BUFF_DEL:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=false;
			}break;
		}
	}
	return 0;
}

//石化
BYTE Buff_6(CCreature* Owner,stBuff* pBuff,int& nNum){
	if (Owner && pBuff){
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				pBuff->m_emRunType=BUFF_RUN;
			}break;
		case BUFF_RUN:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
			}break;
		case BUFF_DEL:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=false;
			}break;
		}
	}
	return 0;
}

//属性
BYTE Buff_7(CCreature* Owner,stBuff* pBuff,int& nNum){
	if (Owner && pBuff){
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
				pBuff->m_emRunType=BUFF_RUN;
			}break;
		case BUFF_RUN:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
			}break;
		case BUFF_DEL:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=false;
				Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
		}break;
		}
	}
	return 0;
}

//百分比属性相关,数值相乘，概率相加
BYTE Buff_8(CCreature* Owner,stBuff* pBuff,int& nNum){
	if (Owner && pBuff){
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
				pBuff->m_emRunType=BUFF_RUN;
			}break;
		case BUFF_RUN:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
			}break;
		case BUFF_DEL:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=false;
				Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
			}break;
		}
	}
	return 0;
}

BYTE Buff_9(CCreature* Owner, stBuff* pBuff, int& nNum) {
	if (Owner && pBuff) {
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = true;
			Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
			pBuff->m_emRunType = BUFF_RUN;
		}break;
		case BUFF_RUN:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = true;
		}break;
		case BUFF_DEL:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = false;
			Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
		}break;
		}
	}
	return 0;
}

BYTE Buff_10(CCreature* Owner, stBuff* pBuff, int& nNum) {
	if (Owner && pBuff) {
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = true;
			Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
			pBuff->m_emRunType = BUFF_RUN;
		}break;
		case BUFF_RUN:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = true;
		}break;
		case BUFF_DEL:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = false;
			Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
		}break;
		}
	}
	return 0;
}

// 多倍经验
BYTE Buff_11(CCreature* Owner, stBuff* pBuff, int& nNum) {
	if (Owner && pBuff) {
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = true;
			Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
			pBuff->m_emRunType = BUFF_RUN;
		}break;
		case BUFF_RUN:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = true;
		}break;
		case BUFF_DEL:
		{
			Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType] = false;
			Owner->ChangePropertyCheckPlayer(ABILITY_FLAG_BUFF);
		}break;
		}
	}
	return 0;
}

//模板
BYTE Buff_255(CCreature* Owner,stBuff* pBuff,int& nNum){
	if (Owner && pBuff){
		switch (pBuff->m_emRunType)
		{
		case BUFF_NEW:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
				pBuff->m_emRunType=BUFF_RUN;
			}break;
		case BUFF_RUN:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=true;
			}break;
		case BUFF_DEL:
			{
				Owner->m_cBuff.m_BuffStateList[pBuff->m_emStateType]=false;
			}break;
		}
	}
	return 0;
}


void CBUFFManager::initBuffFun(){
	g_buff_registry.fill(nullptr);

	g_buff_registry[MAGICSTATE_NULL] = Buff_0,	//状态存在
	g_buff_registry[MAGICSTATE_HP] = Buff_1;	//HP术(包括mp)
	g_buff_registry[MAGICSTATE_MP] = Buff_2;	//MP术
	g_buff_registry[MAGICSTATE_RELIVE] = Buff_3;	//复活术
	g_buff_registry[MAGICSTATE_SPEEDSLOW] = Buff_4;	//减速
	g_buff_registry[MAGICSTATE_DIZZY] = Buff_5;	//晕眩
	g_buff_registry[MAGICSTATE_PETRIFACTION] = Buff_6;	//石化
	g_buff_registry[MAGICSTATE_ABI] = Buff_7;	//属性
	g_buff_registry[MAGICSTATE_ABI_PERCENT] = Buff_8;	//属性百分比
	g_buff_registry[MAGICSTATE_MAGICSHIELD] = Buff_9;	//法师防护
	g_buff_registry[MAGICSTATE_SWORDSHIELD] = Buff_10;	//剑士抗魔护盾
	g_buff_registry[MAGICSTATE_MULTEXP] = Buff_11;	//多倍经验
}

// 执行buff逻辑
void CBUFFManager::Exec_BuffLogicFunc(CCreature* owner, stBuff* buff, int& nNum)
{
	if (!owner || !buff) {
		return;
	}
	const bool refresh = (buff->m_emRunType == BUFF_NEW || buff->m_emRunType == BUFF_DEL);
	const auto state_type = buff->m_emStateType;
	if (state_type < g_buff_registry.size() && g_buff_registry[state_type]) {
		g_buff_registry[state_type](owner, buff, nNum);
	}
	else {
		Buff_255(owner, buff, nNum); // Default handler
	}
	if (refresh) {
		switch (state_type)
		{
			case		MAGICSTATE_ABI:					//属性相关
				{
				}break;
			case		MAGICSTATE_SPEEDSLOW:			//减速
			case		MAGICSTATE_DIZZY:				//晕眩
			case		MAGICSTATE_PETRIFACTION:		//石化
			case        MAGICSTATE_MAGICSHIELD:			//法师护盾
			case        MAGICSTATE_SWORDSHIELD:			//剑士抗魔护盾
				{
				owner->UpdateAppearance(FeatureIndex::boFeature, owner->m_cBuff.BuffFeature());
				}break;
		}
	}
}