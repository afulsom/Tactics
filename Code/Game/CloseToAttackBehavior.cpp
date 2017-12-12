#define NOMINMAX
#include "Game/CloseToAttackBehavior.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Game/Character.hpp"
#include <algorithm>

CloseToAttackBehavior::CloseToAttackBehavior(XMLNode element)
	: m_path()
{

}

CloseToAttackBehavior::CloseToAttackBehavior(CloseToAttackBehavior* behaviorToCopy)
	: m_path()
{
	m_utility = behaviorToCopy->m_utility;
}

CloseToAttackBehavior::~CloseToAttackBehavior()
{

}

void CloseToAttackBehavior::Act(Character* actingCharacter)
{
	float utility = 0.f;
	Tile* destinationTile = CalculateBestTileToMoveTo(utility, actingCharacter, actingCharacter->m_currentTile);

	m_path = actingCharacter->m_currentMap->GeneratePath(actingCharacter->m_currentTile->m_tileCoords, destinationTile->m_tileCoords, actingCharacter);
	actingCharacter->StartMoving(m_path);
}


float CloseToAttackBehavior::CalcUtility(Character* actingCharacter, Tile* tileToActFrom /*= nullptr*/) const
{
	if (nullptr == tileToActFrom)
		tileToActFrom = actingCharacter->m_currentTile;

	float utility = -1.f;
	CalculateBestTileToMoveTo(utility, actingCharacter, tileToActFrom);

	return std::max(utility * 0.6f, m_utility);
}

std::string CloseToAttackBehavior::GetName() const
{
	return "CloseToAttack";
}

void CloseToAttackBehavior::DebugRender(const Character* actingCharacter) const
{
	UNUSED(actingCharacter);
}

Tile* CloseToAttackBehavior::CalculateBestTileToMoveTo(float& outUtility, Character* actingCharacter, Tile* tileToStartFrom) const
{
	std::vector<Tile*> traversableTiles = actingCharacter->m_currentMap->GetTraversableTilesInRangeOfCharacter(actingCharacter, tileToStartFrom);

	Tile* bestTile = nullptr;
	float maxUtility = -99999.f;

	for (Tile* tile : traversableTiles)
	{
		for (size_t behaviorIndex = 0; behaviorIndex < actingCharacter->m_behaviors.size(); behaviorIndex++)
		{
			if (actingCharacter->m_behaviors[behaviorIndex]->GetName() == "CloseToAttack")
				continue;

			Character* nearestTarget = actingCharacter->m_currentMap->FindNearestCharacterNotOfFaction(tile->m_tileCoords, actingCharacter->m_faction);
			int distanceFromNearestTarget = 0;
			if(nullptr != nearestTarget)
			{
				distanceFromNearestTarget = actingCharacter->m_currentMap->CalculateManhattanDistance(*nearestTarget->m_currentTile, *tile);
			}

			float behaviorUtility = actingCharacter->m_behaviors[behaviorIndex]->CalcUtility(actingCharacter, tile) - distanceFromNearestTarget;
			if (behaviorUtility > maxUtility)
			{
				maxUtility = behaviorUtility;
				bestTile = tile;
			}
		}
	}

	outUtility = maxUtility;
	return bestTile;
}

Behavior* CloseToAttackBehavior::Clone()
{
	CloseToAttackBehavior* outBehavior = new CloseToAttackBehavior(this);
	return outBehavior;
}

