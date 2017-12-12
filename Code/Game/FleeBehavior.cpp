#include "Game/FleeBehavior.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Game/Character.hpp"
#include <vector>
#include "Engine/Math/MathUtils.hpp"
#include "Game/App.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/EngineConfig.hpp"



FleeBehavior::FleeBehavior(XMLNode element)
{
	m_cowardice = ParseXMLAttributeFloat(element, "cowardice", m_cowardice);
}

FleeBehavior::FleeBehavior(FleeBehavior* behaviorToCopy)
{
	m_cowardice = behaviorToCopy->m_cowardice;
}

FleeBehavior::~FleeBehavior()
{

}

void FleeBehavior::Act(Character* actingCharacter)
{
	if(actingCharacter->m_targettedCharacter)
	{
// 		Vector2 displacementToTarget = actingCharacter->m_target->m_currentTile->m_tileCoords - actingCharacter->m_currentTile->m_tileCoords;
// 		Vector2 directionAwayFromTarget = displacementToTarget.GetNormalized() * -1.f;
// 
// 		std::vector<IntVector2> potentialTiles;
// 
// 		if (directionAwayFromTarget.x > 0.f)
// 			potentialTiles.push_back(actingCharacter->m_currentTile->m_tileCoords + IntVector2(1, 0));
// 		else if (directionAwayFromTarget.x < 0.f)
// 			potentialTiles.push_back(actingCharacter->m_currentTile->m_tileCoords + IntVector2(-1, 0));
// 
// 		if (directionAwayFromTarget.y > 0.f)
// 			potentialTiles.push_back(actingCharacter->m_currentTile->m_tileCoords + IntVector2(0, 1));
// 		else if (directionAwayFromTarget.y < 0.f)
// 			potentialTiles.push_back(actingCharacter->m_currentTile->m_tileCoords + IntVector2(0, -1));
// 
// 		if (potentialTiles.size() == 0)
// 			actingCharacter->Rest();
// 
// 		int nextTileIndex = GetRandomIntLessThan(potentialTiles.size());
// 		Tile* nextTile = actingCharacter->m_currentMap->GetTileAtTileCoords(potentialTiles[nextTileIndex]);

		if (m_fleePath.empty())
		{
			Tile* nextTile = nullptr;
			int nextTileDist = 0;
			for (int tileIndex = 0; tileIndex < 10; tileIndex++)
			{
				Tile* tempTile = actingCharacter->m_currentMap->GetRandomTraversableTile();
				int tileDist = actingCharacter->m_currentMap->CalculateManhattanDistance(*tempTile, *actingCharacter->m_targettedCharacter->m_currentTile);

				if (tileDist > nextTileDist)
				{
					nextTileDist = tileDist;
					nextTile = tempTile;
				}
			}

			m_fleePath = actingCharacter->m_currentMap->GeneratePath(actingCharacter->m_currentTile->m_tileCoords, nextTile->m_tileCoords, actingCharacter);
		}
		
	}
	Tile* nextTile = *(m_fleePath.end() - 1);
	bool successfullyMoved = actingCharacter->m_currentMap->TryToMoveCharacterToTile(actingCharacter, nextTile);
	if (successfullyMoved)
		m_fleePath.pop_back();
}

float FleeBehavior::CalcUtility(Character* actingCharacter, Tile* tileToActFrom /*= nullptr*/) const
{
	UNUSED(tileToActFrom);

	if(actingCharacter->m_targettedCharacter)
	{
		float healthPercent = (float)actingCharacter->m_currentHP / (float)actingCharacter->m_stats[STAT_MAX_HP];
		float utility = RangeMapFloat(healthPercent, 0.f, 1.f, m_cowardice, 0.f);
		return utility;
	}
	return -1.f;
}

std::string FleeBehavior::GetName() const
{
	return "Flee";
}

void FleeBehavior::DebugRender(const Character* actingCharacter) const 
{
	if(actingCharacter->m_targettedCharacter)
		g_theRenderer->DrawLine2D((Vector2)actingCharacter->m_currentTile->m_tileCoords + Vector2(0.5f, 0.5f), (Vector2)actingCharacter->m_targettedCharacter->m_currentTile->m_tileCoords + Vector2(0.5f, 0.5f), 0.125f, Rgba::WHITE, Rgba::RED);

	for (Tile* tile : m_fleePath)
	{
		g_theRenderer->DrawCenteredText2D((Vector2)tile->m_tileCoords + Vector2(0.5f, 0.5f), g_theRenderer->m_defaultFont, "p", Rgba::BLUE, 0.5f);
	}
}

Behavior* FleeBehavior::Clone()
{
	FleeBehavior* outBehavior = new FleeBehavior(this);
	return outBehavior;
}

