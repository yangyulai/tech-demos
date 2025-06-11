#include "Buff.h"
#include "BaseCreature.h"
#include "PlayerObj.h"
#include "UsrEngn.h"

//////////////////////////////////////////////////////////////////////////
// ״̬����
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

//HP��
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

//MP��
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

//������
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

//����
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

//��ѣ
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

//ʯ��
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

//����
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

//�ٷֱ��������,��ֵ��ˣ��������
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

// �౶����
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

//ģ��
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

	g_buff_registry[MAGICSTATE_NULL] = Buff_0,	//״̬����
	g_buff_registry[MAGICSTATE_HP] = Buff_1;	//HP��(����mp)
	g_buff_registry[MAGICSTATE_MP] = Buff_2;	//MP��
	g_buff_registry[MAGICSTATE_RELIVE] = Buff_3;	//������
	g_buff_registry[MAGICSTATE_SPEEDSLOW] = Buff_4;	//����
	g_buff_registry[MAGICSTATE_DIZZY] = Buff_5;	//��ѣ
	g_buff_registry[MAGICSTATE_PETRIFACTION] = Buff_6;	//ʯ��
	g_buff_registry[MAGICSTATE_ABI] = Buff_7;	//����
	g_buff_registry[MAGICSTATE_ABI_PERCENT] = Buff_8;	//���԰ٷֱ�
	g_buff_registry[MAGICSTATE_MAGICSHIELD] = Buff_9;	//��ʦ����
	g_buff_registry[MAGICSTATE_SWORDSHIELD] = Buff_10;	//��ʿ��ħ����
	g_buff_registry[MAGICSTATE_MULTEXP] = Buff_11;	//�౶����
}

// ִ��buff�߼�
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
			case		MAGICSTATE_ABI:					//�������
				{
				}break;
			case		MAGICSTATE_SPEEDSLOW:			//����
			case		MAGICSTATE_DIZZY:				//��ѣ
			case		MAGICSTATE_PETRIFACTION:		//ʯ��
			case        MAGICSTATE_MAGICSHIELD:			//��ʦ����
			case        MAGICSTATE_SWORDSHIELD:			//��ʿ��ħ����
				{
				owner->UpdateAppearance(FeatureIndex::boFeature, owner->m_cBuff.BuffFeature());
				}break;
		}
	}
}