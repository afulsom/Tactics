#include "Game/Item.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

Item::Item(std::string typeName)
	: m_damageTypes()
{
	m_definition = ItemDefinition::GetItemDefinition(typeName);
	ASSERT_OR_DIE(m_definition != nullptr, "Attempted to create item of invalid item definition.");

	m_stats = Stats::CalculateRandomStatsInRange(m_definition->m_minStats, m_definition->m_maxStats);

	m_damageTypes.SetTags(m_definition->m_damageTypeString);
}

Item::~Item()
{

}

int Item::CalculateTotalStatModifier() const
{
	return 0;
}

void Item::Use()
{
	
}

