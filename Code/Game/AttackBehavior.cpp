#include "Game/AttackBehavior.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/EngineConfig.hpp"

AttackBehavior::AttackBehavior(XMLNode element)
{
	m_utility = ParseXMLAttributeFloat(element, "utility", m_utility);
}

AttackBehavior::AttackBehavior(AttackBehavior* behaviorToCopy)
{
	m_utility = behaviorToCopy->m_utility;
}

AttackBehavior::~AttackBehavior()
{

}

void AttackBehavior::Act(Character* actingCharacter)
{
	actingCharacter->m_targettedCharacter = actingCharacter->CalculateBestAttackTarget();
	if(nullptr != actingCharacter->m_targettedCharacter)
	{
		actingCharacter->StartAttack(actingCharacter->m_targettedCharacter);
	}

}


float AttackBehavior::CalcUtility(Character* actingCharacter, Tile* tileToActFrom /*= nullptr*/) const
{
	if (nullptr == tileToActFrom)
		tileToActFrom = actingCharacter->m_currentTile;

	int maxNetDamage = actingCharacter->CalculateMaxNetAttackDamage(tileToActFrom);

	return (float)maxNetDamage;
}

std::string AttackBehavior::GetName() const
{
	return "Attack";
}

void AttackBehavior::DebugRender(const Character* actingCharacter) const
{
	if(actingCharacter->m_targettedCharacter)
		g_theRenderer->DrawLine2D((Vector2)actingCharacter->m_currentTile->m_tileCoords + Vector2(0.5f, 0.5f), (Vector2)actingCharacter->m_targettedCharacter->m_currentTile->m_tileCoords + Vector2(0.5f, 0.5f), 0.125f, Rgba::WHITE, Rgba::RED);
}

Behavior* AttackBehavior::Clone()
{
	AttackBehavior* outBehavior = new AttackBehavior(this);
	return outBehavior;
}

