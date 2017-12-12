#include "Game/AbilityBehavior.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/EngineConfig.hpp"

AbilityBehavior::AbilityBehavior(XMLNode element)
{
	UNUSED(element);
}

AbilityBehavior::AbilityBehavior(AbilityBehavior* behaviorToCopy)
{
	UNUSED(behaviorToCopy);
}

AbilityBehavior::~AbilityBehavior()
{

}

void AbilityBehavior::Act(Character* actingCharacter)
{
	actingCharacter->TargetAndSetBestAbility();
	g_theApp->m_game->EndTurn(actingCharacter, 0);

	//actingCharacter->StartAbility();
}


float AbilityBehavior::CalcUtility(Character* actingCharacter, Tile* tileToActFrom /*= nullptr*/) const
{
	if (nullptr == tileToActFrom)
		tileToActFrom = actingCharacter->m_currentTile;

	int maxNetDamage = 0;
	for (AbilityDefinition* ability : actingCharacter->m_abilities)
	{
		int abilityNetDamage = actingCharacter->CalculateMaxNetAbilityDamage(ability, tileToActFrom);
		if (abilityNetDamage > maxNetDamage)
			maxNetDamage = abilityNetDamage;
	}

	return (float)maxNetDamage;
}

std::string AbilityBehavior::GetName() const
{
	return "Ability";
}

void AbilityBehavior::DebugRender(const Character* actingCharacter) const
{
	if(actingCharacter->m_targettedCharacter)
		g_theRenderer->DrawLine2D((Vector2)actingCharacter->m_currentTile->m_tileCoords + Vector2(0.5f, 0.5f), (Vector2)actingCharacter->m_targettedCharacter->m_currentTile->m_tileCoords + Vector2(0.5f, 0.5f), 0.125f, Rgba::WHITE, Rgba::RED);
}

Behavior* AbilityBehavior::Clone()
{
	AbilityBehavior* outBehavior = new AbilityBehavior(this);
	return outBehavior;
}

