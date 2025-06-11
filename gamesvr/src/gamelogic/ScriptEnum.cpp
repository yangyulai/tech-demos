#include <EnumExporter.h>
#include "BaseCreature.h"
#include "Script.h"
#include "PlayerObj.h"

void CScriptSystem::ExportEnum() const
{
	if (GameService::getMe().m_lineId == 1 && GameService::getMe().m_btAllisGm > 0)
	{
		m_enumExporter->register_enum<eCretType>();
		m_enumExporter->register_enum<ResID>();
		m_enumExporter->register_enum<emPkModel>();
		m_enumExporter->register_enum<FeatureIndex>();
		m_enumExporter->register_enum<SavedSharedData>();
		m_enumExporter->register_enum<emGuildMemberPowerLvl>();
		m_enumExporter->register_enum<emRankType>();
		m_enumExporter->register_enum<emMonsterType>();
		m_enumExporter->export_to_lua(R"(.\luaScript\gamebase\CppEnum.lua)");
	}
}
