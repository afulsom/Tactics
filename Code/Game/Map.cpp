#include "Game/Map.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Game/CharacterBuilder.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/MapDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Game/App.hpp"
#include "Engine/Core/Noise.hpp"
#include "Game/Tile.hpp"
#include "Engine/Core/ProfileLogScope.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"
#include "Engine/Network/NetSession.hpp"
#include "Engine/Network/NetConnection.hpp"
#include "Game/GameSession.hpp"


PathGenerator::PathGenerator(const IntVector2& start, const IntVector2& end, Map* map, Character* gCostReferenceCharacter)
	: m_start(start)
	, m_end(end)
	, m_map(map)
	, m_gCostReferenceCharacter(gCostReferenceCharacter)
	, m_openList()
{
	static int pathID = 0;
	pathID++;
	m_pathID = pathID;
	OpenNodeForProcessing(*m_map->GetTileAtTileCoords(m_start), nullptr);
}


Path PathGenerator::CreateFinalPath(OpenNode& endNode)
{
	OpenNode* currentNode = &endNode;

	Path outPath;
	while (currentNode->m_parent != nullptr)
	{
		outPath.push_back(currentNode->m_tile);
		currentNode = currentNode->m_parent;
	}

	m_finalPath = outPath;
	return outPath;
}

void PathGenerator::OpenNodeForProcessing(Tile& tileToOpen, OpenNode* parent)
{
	OpenNode* newOpenNode = new OpenNode;
	newOpenNode->m_tile = &tileToOpen;
	newOpenNode->m_parent = parent;
	newOpenNode->m_localGCost = newOpenNode->m_tile->GetGCost() + m_gCostReferenceCharacter->GetGCostBias(newOpenNode->m_tile->m_tileDefinition->m_name);
	newOpenNode->m_totalGCost = ((parent) ? parent->m_totalGCost : 0.f) + newOpenNode->m_localGCost;
	newOpenNode->m_estimatedDistToGoal = (float)m_map->CalculateManhattanDistance(*newOpenNode->m_tile, *m_map->GetTileAtTileCoords(m_end));
	newOpenNode->m_fScore = newOpenNode->m_estimatedDistToGoal + newOpenNode->m_totalGCost;

	m_openList.push_back(newOpenNode);
	tileToOpen.m_isOpenInPathID = m_pathID;
}

OpenNode* PathGenerator::SelectAndCloseBestOpenNode()
{
	int bestNodeIndex = -1;
	float lowestFScore = FLT_MAX;

	for (size_t nodeIndex = 0; nodeIndex < m_openList.size(); nodeIndex++)
	{
		OpenNode* node = m_openList[nodeIndex];
		if (node->m_fScore < lowestFScore)
		{
			lowestFScore = node->m_fScore;
			bestNodeIndex = nodeIndex;
		}
	}

	if (bestNodeIndex == -1)
		return nullptr;

	OpenNode* bestNode = m_openList[bestNodeIndex];
	bestNode->m_tile->m_isClosedInPathID = m_pathID;
	m_openList.erase(m_openList.begin() + bestNodeIndex);
	return bestNode;
}

void PathGenerator::OpenNodeIfValid(Tile* tileToOpen, OpenNode* parent)
{
	if (!tileToOpen)
		return;

	if (!tileToOpen->IsTraversableToCharacterAtHeight(m_gCostReferenceCharacter, parent->m_tile->m_height))
		return;

	if (tileToOpen->m_occupyingCharacter)
		return;

	if (tileToOpen->IsSolidToTags(m_gCostReferenceCharacter->m_tags))
		return;

	if (tileToOpen->m_isClosedInPathID == m_pathID)
		return;

	if (tileToOpen->m_isOpenInPathID == m_pathID)
		return;

	OpenNodeForProcessing(*tileToOpen, parent);
}


const float Map::DAMAGE_NUMBER_LIFETIME = 1.f;


DamageNumber::DamageNumber(std::string number, const Vector3& position, const Rgba& color, float scale)
	: m_color(color)
	, m_number(number)
	, m_position(position)
	, m_lifetime(Map::DAMAGE_NUMBER_LIFETIME)
	, m_scale(scale)
{
		
}


Map::Map(std::string mapDefinitionName)
	: m_tiles()
	, m_characters()
	, m_spriteEffects()
	, m_name()
	, m_selectedCharacter(nullptr)
	, m_selectedTile(nullptr)
{
	Character::s_currentCharacterIndex = 1;

	m_definition = MapDefinition::GetDefinition(mapDefinitionName);

	m_tiles.resize(m_definition->m_dimensions.x * m_definition->m_dimensions.y);
	for (size_t tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++)
	{
		m_tiles[tileIndex].m_tileCoords = CalculateTileCoordsFromTileIndex(tileIndex);
		m_tiles[tileIndex].m_containingMap = this;
		m_tiles[tileIndex].ChangeType(m_definition->m_fillTileType);
 		m_tiles[tileIndex].m_height += (floorf(7.f * Compute2dPerlinNoise((float)m_tiles[tileIndex].m_tileCoords.x, (float)m_tiles[tileIndex].m_tileCoords.y, 20.f, 3)) + 3.f);
// 		m_tiles[tileIndex].m_height += floorf(GetRandomFloatInRange(-3.f, 7.f));
	}

	BuildTileVerts();

	Character* testCharacter1 = CharacterBuilder::BuildNewCharacter("witch");
	Character* testCharacter2 = CharacterBuilder::BuildNewCharacter("witch");
	Character* testCharacter3 = CharacterBuilder::BuildNewCharacter("witch");
	Character* testCharacter4 = CharacterBuilder::BuildNewCharacter("witch");

	Character* testCharacter5 = CharacterBuilder::BuildNewCharacter("berzerker");
	Character* testCharacter6 = CharacterBuilder::BuildNewCharacter("berzerker");
	Character* testCharacter7 = CharacterBuilder::BuildNewCharacter("berzerker");
	Character* testCharacter8 = CharacterBuilder::BuildNewCharacter("berzerker");

	testCharacter1->m_controller = CONTROLLER_PLAYER;
	testCharacter2->m_controller = CONTROLLER_PLAYER;
	testCharacter3->m_controller = CONTROLLER_PLAYER;
	testCharacter4->m_controller = CONTROLLER_PLAYER;

	testCharacter5->m_controller = CONTROLLER_PLAYER;
	testCharacter6->m_controller = CONTROLLER_PLAYER;
	testCharacter7->m_controller = CONTROLLER_PLAYER;
	testCharacter8->m_controller = CONTROLLER_PLAYER;

	testCharacter5->m_faction = "enemy";
	testCharacter6->m_faction = "enemy";
	testCharacter7->m_faction = "enemy";
	testCharacter8->m_faction = "enemy";

	testCharacter1->m_owningPlayer = 0;
	testCharacter2->m_owningPlayer = 2;
	testCharacter5->m_owningPlayer = 1;
	testCharacter6->m_owningPlayer = 3;


	PlaceCharacterInMap(testCharacter1, GetTileAtTileCoords(IntVector2(15, 4)));
//  	PlaceCharacterInMap(testCharacter2, GetTileAtTileCoords(IntVector2(16, 5)));
//	PlaceCharacterInMap(testCharacter3, GetTileAtTileCoords(IntVector2(14, 4)));
//	PlaceCharacterInMap(testCharacter4, GetTileAtTileCoords(IntVector2(13, 6)));

	PlaceCharacterInMap(testCharacter5, GetTileAtTileCoords(IntVector2(5, 4)));
//  	PlaceCharacterInMap(testCharacter6, GetTileAtTileCoords(IntVector2(6, 5)));
// 	PlaceCharacterInMap(testCharacter7, GetTileAtTileCoords(IntVector2(4, 4)));
// 	PlaceCharacterInMap(testCharacter8, GetTileAtTileCoords(IntVector2(3, 6)));


	m_selectedTile = GetTileAtTileIndex(0);
}

Map::~Map()
{
	for (DrawCall call : m_drawCalls)
	{
		delete call.m_VBO;
		call.m_verts.clear();
	}
}


void Map::Update(float deltaSeconds)
{
	for (size_t tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++)
	{
		m_tiles[tileIndex].Update(deltaSeconds);
	}

	for (size_t entityIndex = 0; entityIndex < m_characters.size(); entityIndex++)
	{
		m_characters[entityIndex]->Update(deltaSeconds);
	}

	m_particleSystem.Update(deltaSeconds);
	m_particleSystem.SetBillboardMatrix(g_theApp->m_game->m_theCamera.GetInverseViewMatrix());

	for (size_t effectIndex = 0; effectIndex < m_spriteEffects.size(); effectIndex++)
	{
		m_spriteEffects[effectIndex].Update(deltaSeconds);
	}

	size_t effectSize = m_spriteEffects.size();
	for (size_t effectIndex = 0; effectIndex < effectSize; effectIndex++)
	{
		if (m_spriteEffects[effectIndex].m_anim->IsFinished())
		{
			g_theApp->m_game->ReleaseWait();
			m_spriteEffects.erase(m_spriteEffects.begin() + effectIndex);
			effectIndex--;
			effectSize--;
		}
	}

	if(!m_isWaitingForInput /* && g_theApp->m_game->m_session->m_session.IsHost()*/ && !m_selectedCharacter)
	{
		Character* nextCharacterToAct = GetCharacterWithGreatestCT();
		if (nextCharacterToAct && nextCharacterToAct->m_currentCT >= 100)
		{
			if (nextCharacterToAct->m_isDead)
			{
				nextCharacterToAct->m_currentHP--;
				nextCharacterToAct->m_currentCT = 0;
				if (nextCharacterToAct->m_currentHP <= -4)
				{
					DestroyCharacter(nextCharacterToAct);
				}
			}
			else
			{
				if (g_theApp->m_game->m_session->m_session.m_myConnection != nullptr && nextCharacterToAct->m_owningPlayer == g_theApp->m_game->m_session->m_session.m_myConnection->m_connectionIndex)
				{
					m_selectedCharacter = nextCharacterToAct;
					m_activeCharacter = nextCharacterToAct;
// 					g_theApp->m_game->m_currentGameState = STATE_PLAYING;
//					m_isWaitingForInput = true;
				}
 				else
 				{
// 					g_theApp->m_game->m_session->SendTurnAlert(nextCharacterToAct->m_owningPlayer, nextCharacterToAct->m_characterIndex);
					m_isWaitingForInput = true;
					m_activeCharacter = nextCharacterToAct;
				}
				m_selectedTile = nextCharacterToAct->m_currentTile;

				if (nextCharacterToAct->HasStatusEffect(STATUS_POISON))
				{
					nextCharacterToAct->ApplyDamage((int)((float)nextCharacterToAct->m_stats[STAT_MAX_HP] * 0.1f), false);
					if (nextCharacterToAct->m_isDead)
					{
						if (m_isWaitingForInput)
						{
							m_isWaitingForInput = false;
						}
						g_theApp->m_game->EndTurn(nextCharacterToAct, 0);
					}
				}

				if (nullptr != nextCharacterToAct->m_currentAbility)
				{
					nextCharacterToAct->StartAbility();
				}
				else
				{
					if (nextCharacterToAct->m_controller == CONTROLLER_AI || (nextCharacterToAct->HasStatusEffect(STATUS_CONFUSE)) || nextCharacterToAct->HasStatusEffect(STATUS_CHARM))
					{
						nextCharacterToAct->Act();
					}
				}
			}
		}
		else
		{
			while(nextCharacterToAct && nextCharacterToAct->m_currentCT < 100)
			{
				TickCT();
				nextCharacterToAct = GetCharacterWithGreatestCT();
			}
		}
	}

	if(m_selectedCharacter)
		m_traversableTiles = GetTraversableTilesInRangeOfCharacter(m_selectedCharacter);

	UpdateDamageNumbers(deltaSeconds);
}

void Map::Render() const
{
	g_theRenderer->SetTexture(nullptr);

	for (DrawCall call : m_drawCalls)
	{
		g_theRenderer->SetTexture(call.m_texture);
		g_theRenderer->Draw(PRIMITIVE_TRIANGLES, call.m_VBO, call.m_VBO->m_bufferSize);
	}

	for (size_t tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++)
	{
		m_tiles[tileIndex].Render();
	}

	g_theRenderer->SetShader(g_theApp->m_game->m_spriteShader);
	for (size_t characterIndex = 0; characterIndex < m_characters.size(); characterIndex++)
	{
		m_characters[characterIndex]->Render();
	}

	for (size_t effectIndex = 0; effectIndex < m_spriteEffects.size(); effectIndex++)
	{
		DrawSpriteEffect(m_spriteEffects[effectIndex]);
	}

	g_theRenderer->SetModelMatrix(Matrix4::CreateIdentity());
	m_particleSystem.Render();

 	RenderDamageNumbers();
}

void Map::RenderDebugPathing() const
{
	if (!m_currentPath)
		return;

	for (Tile tile : m_tiles)
	{
		if (tile.m_isClosedInPathID == m_currentPath->m_pathID)
		{
			g_theRenderer->DrawCenteredText2D((Vector2)tile.m_tileCoords + Vector2(0.5f, 0.5f), g_theRenderer->m_defaultFont, "x", Rgba::RED, 0.5f);
		}
		else if (tile.m_isOpenInPathID == m_currentPath->m_pathID)
		{
			g_theRenderer->DrawCenteredText2D((Vector2)tile.m_tileCoords + Vector2(0.5f, 0.5f), g_theRenderer->m_defaultFont, "o", Rgba::GREEN, 0.5f);
		}
		else if (tile.m_tileDefinition->m_isTraversable)
		{
			g_theRenderer->DrawCenteredText2D((Vector2)tile.m_tileCoords + Vector2(0.5f, 0.5f), g_theRenderer->m_defaultFont, "s", Rgba::RED, 0.5f);
		}
	}

	for (OpenNode* node : m_currentPath->m_openList)
	{
		g_theRenderer->DrawCenteredText2D((Vector2)node->m_tile->m_tileCoords + Vector2(0.25f, 0.75f), g_theRenderer->m_defaultFont, std::to_string((int)node->m_totalGCost), Rgba::GREEN, 0.5f);
		g_theRenderer->DrawCenteredText2D((Vector2)node->m_tile->m_tileCoords + Vector2(0.75f, 0.75f), g_theRenderer->m_defaultFont, std::to_string((int)node->m_localGCost), Rgba::GREEN, 0.5f);
		g_theRenderer->DrawCenteredText2D((Vector2)node->m_tile->m_tileCoords + Vector2(0.75f, 0.25f), g_theRenderer->m_defaultFont, std::to_string((int)node->m_estimatedDistToGoal), Rgba::GREEN, 0.5f);
		g_theRenderer->DrawCenteredText2D((Vector2)node->m_tile->m_tileCoords + Vector2(0.25f, 0.25f), g_theRenderer->m_defaultFont, std::to_string((int)node->m_fScore), Rgba::GREEN, 0.5f);
	}

	if (!m_currentPath->m_finalPath.empty())
	{
		for (Tile* tile : m_currentPath->m_finalPath)
		{
			g_theRenderer->DrawCenteredText2D((Vector2)tile->m_tileCoords + Vector2(0.5f, 0.5f), g_theRenderer->m_defaultFont, "x", Rgba::BLUE, 0.5f);
		}

	}
}

void Map::RenderDebugGenerating() const
{
	m_definition->DebugRender(this);
}

void Map::TickCT()
{
	for (size_t entityIndex = 0; entityIndex < m_characters.size(); entityIndex++)
	{
		m_characters[entityIndex]->TickCT();
	}
}

int Map::CalculateTileIndexFromTileCoords(const IntVector2& tileCoords) const
{
	return tileCoords.y * m_definition->m_dimensions.x + tileCoords.x;
}

IntVector2 Map::CalculateTileCoordsFromTileIndex(int tileIndex) const
{
	return IntVector2(tileIndex % m_definition->m_dimensions.x, tileIndex / m_definition->m_dimensions.x);
}

IntVector2 Map::CalculateTileCoordsFromMapCoords(const Vector2& mapCoords) const
{
	return IntVector2((int)floor(mapCoords.x), (int)floor(mapCoords.y));
}

Tile* Map::GetTileAtMapCoords(const Vector2& mapCoords)
{
	IntVector2 tileCoords = CalculateTileCoordsFromMapCoords(mapCoords);
	return GetTileAtTileCoords(tileCoords);
}

Tile* Map::GetTileAtTileCoords(const IntVector2& tileCoords)
{
	if (IsInMap(tileCoords))
	{
		int tileIndex = CalculateTileIndexFromTileCoords(tileCoords);
		return GetTileAtTileIndex(tileIndex);
	}
	else
		return nullptr;
}

Tile* Map::GetTileAtTileIndex(int tileIndex)
{
	if (tileIndex < 0 || tileIndex >= (int)m_tiles.size())
		return nullptr;
	else
		return &m_tiles[tileIndex];
}

Tile* Map::GetRandomTile()
{
	IntVector2 randomTileCoords(g_random.GetRandomIntLessThan(m_definition->m_dimensions.x), g_random.GetRandomIntLessThan(m_definition->m_dimensions.y));
	return GetTileAtTileCoords(randomTileCoords);
}

Tile* Map::GetRandomTraversableTile()
{
	Tile* randomTile = GetRandomTile();
	int counter = 0;
	int maxAttempts = 1000;
	while (randomTile->m_tileDefinition->m_isTraversable || randomTile->m_occupyingCharacter != nullptr)
	{
		if (counter >= maxAttempts)
			return nullptr;

		randomTile = GetRandomTile();
		counter++;
	}
	return randomTile;
}

Tile* Map::GetRandomTileOfType(std::string tileType)
{
	Tile* randomTile = GetRandomTile();
	int counter = 0;
	int maxAttempts = 1000;
	while (randomTile->m_tileDefinition->m_name != tileType || randomTile->m_occupyingCharacter != nullptr)
	{
		if (counter >= maxAttempts)
			return nullptr;

		randomTile = GetRandomTile();
		counter++;
	}
	return randomTile;
}

Tile* Map::GetRandomTileWithTags(std::string tags)
{
	std::vector<Tile*> tilesWithTags;
	for (size_t tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++)
	{
		if (m_tiles[tileIndex].m_tags.MatchTags(tags))
			tilesWithTags.push_back(&m_tiles[tileIndex]);
	}

	int randomTileIndex = g_random.GetRandomIntLessThan(tilesWithTags.size());
	return tilesWithTags[randomTileIndex];
}

bool Map::TryToMoveCharacterToTile(Character* characterToMove, Tile* destinationTile)
{
	if (!destinationTile)
		return false;

	if (destinationTile->m_occupyingCharacter)
	{
		return false;
	}

	if (destinationTile->IsSolidToTags(characterToMove->m_tags))
		return false;

	MoveCharacterToTile(characterToMove, destinationTile);
	return true;
}

void Map::SpawnActors()
{
	Character* pixie = CharacterBuilder::BuildNewCharacter("pixie");
	PlaceCharacterInMap(pixie, GetRandomTraversableTile());

	Character* pixie2 = CharacterBuilder::BuildNewCharacter("pixie");
	PlaceCharacterInMap(pixie2, GetRandomTraversableTile());

	Character* pixie3 = CharacterBuilder::BuildNewCharacter("pixie");
	PlaceCharacterInMap(pixie3, GetRandomTraversableTile());
}

int Map::CalculateManhattanDistance(const Tile& tileA, const Tile& tileB)
{
	IntVector2 distanceVector = tileB.m_tileCoords - tileA.m_tileCoords;
	return (abs(distanceVector.x) + abs(distanceVector.y));
}

Character* Map::FindNearestCharacterOfFaction(const IntVector2& startingPosition, std::string faction)
{
	Tile* startingTile = GetTileAtTileCoords(startingPosition);
	Character* nearestCharacter = nullptr;
	int distanceToNearestCharacter = INT_MAX;
	for (Tile& tile : m_tiles)
	{
		if (tile.m_occupyingCharacter && tile.m_occupyingCharacter->m_faction == faction)
		{
			int distanceToCharacter = CalculateManhattanDistance(*startingTile, tile);
			if (distanceToCharacter < distanceToNearestCharacter)
			{
				distanceToNearestCharacter = distanceToCharacter;
				nearestCharacter = tile.m_occupyingCharacter;
			}
		}
	}

	return nearestCharacter;
}

Character* Map::FindNearestCharacterNotOfFaction(const IntVector2& startingPosition, std::string faction)
{
	Tile* startingTile = GetTileAtTileCoords(startingPosition);
	Character* nearestCharacter = nullptr;
	int distanceToNearestCharacter = INT_MAX;
	for (Tile& tile : m_tiles)
	{
		if (tile.m_occupyingCharacter && tile.m_occupyingCharacter->m_faction != faction)
		{
			int distanceToCharacter = CalculateManhattanDistance(*startingTile, tile);
			if (distanceToCharacter < distanceToNearestCharacter)
			{
				distanceToNearestCharacter = distanceToCharacter;
				nearestCharacter = tile.m_occupyingCharacter;
			}
		}
	}

	return nearestCharacter;
}

std::vector<Character*> Map::FindAllCharactersOfFaction(std::string faction)
{
	std::vector<Character*> outVector;

	for (Tile& tile : m_tiles)
	{
		if (tile.m_occupyingCharacter && tile.m_occupyingCharacter->m_faction == faction)
		{
			outVector.push_back(tile.m_occupyingCharacter);
		}
	}

	return outVector;
}

std::vector<Character*> Map::FindAllCharactersNotOfFaction(std::string faction)
{
	std::vector<Character*> outVector;

	for (Tile& tile : m_tiles)
	{
		if (tile.m_occupyingCharacter && tile.m_occupyingCharacter->m_faction != faction)
		{
			outVector.push_back(tile.m_occupyingCharacter);
		}
	}

	return outVector;
}


Tile* Map::FindNearestTileOfType(const IntVector2& startingPosition, std::string type)
{
	Tile* startingTile = GetTileAtTileCoords(startingPosition);
	Tile* nearestTile = nullptr;
	int distanceToNearestTile = INT_MAX;
	for (Tile& tile : m_tiles)
	{
		if (tile.m_tileDefinition->m_name == type)
		{
			int distanceToTile = CalculateManhattanDistance(*startingTile, tile);
			if (distanceToTile < distanceToNearestTile)
			{
				distanceToNearestTile = distanceToTile;
				nearestTile = &tile;
			}
		}
	}

	return nearestTile;
}

Tile* Map::FindNearestTileNotOfType(const IntVector2& startingPosition, std::string type)
{
	Tile* startingTile = GetTileAtTileCoords(startingPosition);
	Tile* nearestTile = nullptr;
	int distanceToNearestTile = INT_MAX;
	for (Tile& tile : m_tiles)
	{
		if (tile.m_tileDefinition->m_name != type)
		{
			int distanceToTile = CalculateManhattanDistance(*startingTile, tile);
			if (distanceToTile < distanceToNearestTile)
			{
				distanceToNearestTile = distanceToTile;
				nearestTile = &tile;
			}
		}
	}

	return nearestTile;
}



void Map::DestroyCharacter(Character* characterToKill)
{
	for (Character* character : m_characters)
	{
		if (character->m_targettedCharacter == characterToKill)
			character->m_targettedCharacter = nullptr;
	}

	Tile* tileContainingCharacterToKill = characterToKill->m_currentTile;
	tileContainingCharacterToKill->m_occupyingCharacter = nullptr;

	size_t characterIndex = 0;
	for (; characterIndex < m_characters.size(); characterIndex++)
	{
		if (m_characters[characterIndex] == characterToKill)
			break;
	}
	m_characters.erase(m_characters.begin() + characterIndex);

	delete characterToKill;
	characterToKill = nullptr;
}


void Map::PlaceItemInMap(Item* itemToPlace, Tile* tileToPlaceIn)
{
	if (!tileToPlaceIn)
		return;

	tileToPlaceIn->m_tileInventory.AddItem(itemToPlace);
}

Tile* Map::FindFirstTraversableTile()
{
	for (size_t tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++)
	{
		if (!GetTileAtTileIndex(tileIndex)->m_tileDefinition->m_isTraversable)
			return GetTileAtTileIndex(tileIndex);
	}

	return nullptr;
}

void Map::PlaceCharacterInMap(Character* characterToPlace, Tile* destinationTile)
{
	if (!destinationTile)
		return;

	destinationTile->m_occupyingCharacter = characterToPlace;

	characterToPlace->m_currentMap = this;
	characterToPlace->m_currentTile = destinationTile;
	characterToPlace->m_currentPosition = Vector3(destinationTile->m_tileCoords.x + 0.5f, destinationTile->GetDisplayHeight(), destinationTile->m_tileCoords.y + 0.5f);
	m_characters.push_back(characterToPlace);
}

void Map::MoveCharacterToTile(Character* characterToMove, Tile* destinationTile)
{
	Tile* startTile = characterToMove->m_currentTile;
	startTile->m_occupyingCharacter = nullptr;
	destinationTile->m_occupyingCharacter = characterToMove;

	characterToMove->m_currentTile = destinationTile;
	characterToMove->m_currentPosition = Vector3(destinationTile->m_tileCoords.x + 0.5f, destinationTile->GetDisplayHeight(), destinationTile->m_tileCoords.y + 0.5f);
}

void Map::UpdateDamageNumbers(float deltaSeconds)
{
	size_t numDamageNumbers = m_damageNumbers.size();
	for (size_t damageNumberIndex = 0; damageNumberIndex < numDamageNumbers; damageNumberIndex++)
	{
		DamageNumber& number = m_damageNumbers[damageNumberIndex];
		number.m_lifetime -= deltaSeconds;
		number.m_position.y += deltaSeconds;

		if (number.m_lifetime < 0.f)
		{
			m_damageNumbers.erase(m_damageNumbers.begin() + damageNumberIndex);
			numDamageNumbers--;
			damageNumberIndex--;
		}
	}
}

void Map::RenderDamageNumbers() const
{
	Matrix4 billboardMatrix = g_theApp->m_game->m_theCamera.GetInverseViewMatrix();

	Vector3 rightDirection(billboardMatrix.m_iBasis.x, billboardMatrix.m_iBasis.y, billboardMatrix.m_iBasis.z);
	Vector3 upDirection(billboardMatrix.m_jBasis.x, billboardMatrix.m_jBasis.y, billboardMatrix.m_jBasis.z);

	for (DamageNumber number : m_damageNumbers)
	{
		Rgba fadedColor(number.m_color);
		fadedColor.a = 0;
		Rgba numberColor = Interpolate(number.m_color, fadedColor, RangeMapFloat(number.m_lifetime, DAMAGE_NUMBER_LIFETIME, 0.f, 0.f, 1.f));
		g_theRenderer->DrawCenteredText3D(number.m_position, rightDirection, upDirection, g_theRenderer->m_defaultFont, number.m_number, numberColor, number.m_scale);
	}
}

void Map::BuildTileVerts()
{
	m_drawCalls.clear();

	for (Tile tile : m_tiles)
	{
		int topTextureIndex = FindTextureInDrawCalls(tile.m_tileDefinition->m_topTexture);
		std::vector<Vertex> topVerts = tile.GetTopVerts();

		if (topTextureIndex == -1)
		{
			DrawCall newCall;
			newCall.m_VBO = g_theRenderer->CreateEmptyVertexBuffer();
			newCall.m_texture = tile.m_tileDefinition->m_topTexture;
			newCall.m_verts.insert(newCall.m_verts.end(), topVerts.begin(), topVerts.end());
			m_drawCalls.push_back(newCall);
		}
		else
		{
			m_drawCalls[topTextureIndex].m_verts.insert(m_drawCalls[topTextureIndex].m_verts.end(), topVerts.begin(), topVerts.end());
		}

		int sideTextureIndex = FindTextureInDrawCalls(tile.m_tileDefinition->m_sideTexture);
		std::vector<Vertex> sideVerts = tile.GetSideVerts();

		if (sideTextureIndex == -1)
		{
			DrawCall newCall;
			newCall.m_VBO = g_theRenderer->CreateEmptyVertexBuffer();
			newCall.m_texture = tile.m_tileDefinition->m_sideTexture;
			newCall.m_verts.insert(newCall.m_verts.end(), sideVerts.begin(), sideVerts.end());
			m_drawCalls.push_back(newCall);
		}
		else
		{
			m_drawCalls[sideTextureIndex].m_verts.insert(m_drawCalls[sideTextureIndex].m_verts.end(), sideVerts.begin(), sideVerts.end());
		}
	}


	for (DrawCall call : m_drawCalls)
	{
		call.m_VBO->Update(g_theRenderer->m_context, call.m_verts.data(), call.m_verts.size());
	}
}

int Map::FindTextureInDrawCalls(const Texture2D* texture)
{
	for (int drawCallIndex = 0; drawCallIndex < (int)m_drawCalls.size(); drawCallIndex++)
	{
		if(m_drawCalls[drawCallIndex].m_texture == texture)
			return drawCallIndex;
	}

	return -1;
}

void Map::DrawSpriteEffect(SpriteEffect effect) const
{
	Matrix4 billboardMatrix = g_theApp->m_game->m_theCamera.GetInverseViewMatrixXZ();
	billboardMatrix.m_translation = Vector4(effect.m_position, 1.f);
	billboardMatrix.m_translation -= (Vector4(g_theApp->m_game->m_theCamera.GetForwardXYZ(), 0.f) * 0.05f);

	billboardMatrix.RotateDegreesAboutY(-45.f);
	g_theRenderer->SetModelMatrix(billboardMatrix);
	g_theRenderer->SetTexture(0, effect.m_anim->GetTexture());
	AABB2 texCoords = effect.m_anim->GetCurrentTexCoords();

	float width = 1.f;
	float halfWidth = width * 0.5f;
	float height = width * ((float)effect.m_anim->GetSpriteLayout().x / (float)effect.m_anim->GetSpriteLayout().y);

	g_theRenderer->DrawQuad3D(Vector3(-halfWidth, 0.f, -halfWidth), Vector3(halfWidth, height, halfWidth), texCoords.mins, texCoords.maxs, Rgba::WHITE);
}

void Map::DrawTraversableTiles(const std::vector<Tile*>& traversableTiles) const
{
	std::vector<Vertex> vertices;

	Vector3 topNormal(0.f, -1.f, 0.f);
	Vector3 topTangent(1.f, 0.f, 0.f);
	Vector3 topBiTangent(0.f, 0.f, 1.f);
	Rgba highlightColor(50, 50, 200, 200);

	for (Tile* tile : traversableTiles)
	{
		Vector3 leftFront = Vector3((float)tile->m_tileCoords.x, tile->GetDisplayHeight() + 0.01f, (float)tile->m_tileCoords.y);
		Vector3 rightBack = Vector3((float)tile->m_tileCoords.x + 1.f, tile->GetDisplayHeight() + 0.01f, (float)tile->m_tileCoords.y + 1.f);
		Vector3 leftBack = Vector3(leftFront.x, rightBack.y, rightBack.z);
		Vector3 rightFront = Vector3(rightBack.x, rightBack.y, leftFront.z);

		vertices.push_back(Vertex(leftBack, Vector2(0.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(leftFront, Vector2(0.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(rightBack, Vector2(1.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor));

		vertices.push_back(Vertex(rightBack, Vector2(1.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(leftFront, Vector2(0.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(rightFront, Vector2(1.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor));
	}

	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->DrawVertices(vertices.data(), vertices.size());
}

void Map::DrawTargettableTiles() const
{
	std::vector<Vertex> vertices;

	Vector3 topNormal(0.f, -1.f, 0.f);
	Vector3 topTangent(1.f, 0.f, 0.f);
	Vector3 topBiTangent(0.f, 0.f, 1.f);
	Rgba highlightColor(200, 50, 50, 200);

	for (Tile* tile : m_targettableTiles)
	{
		Vector3 leftFront = Vector3((float)tile->m_tileCoords.x, tile->GetDisplayHeight() + 0.01f, (float)tile->m_tileCoords.y);
		Vector3 rightBack = Vector3((float)tile->m_tileCoords.x + 1.f, tile->GetDisplayHeight() + 0.01f, (float)tile->m_tileCoords.y + 1.f);
		Vector3 leftBack = Vector3(leftFront.x, rightBack.y, rightBack.z);
		Vector3 rightFront = Vector3(rightBack.x, rightBack.y, leftFront.z);

		vertices.push_back(Vertex(leftBack, Vector2(0.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(leftFront, Vector2(0.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(rightBack, Vector2(1.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor));

		vertices.push_back(Vertex(rightBack, Vector2(1.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(leftFront, Vector2(0.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(rightFront, Vector2(1.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor));
	}

	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->DrawVertices(vertices.data(), vertices.size());
}

void Map::DrawAoETiles() const
{
	std::vector<Vertex> vertices;

	Vector3 topNormal(0.f, -1.f, 0.f);
	Vector3 topTangent(1.f, 0.f, 0.f);
	Vector3 topBiTangent(0.f, 0.f, 1.f);
	Rgba highlightColor(200, 150, 50, 200);

	for (Tile* tile : m_AoETiles)
	{
		Vector3 leftFront = Vector3((float)tile->m_tileCoords.x, tile->GetDisplayHeight() + 0.0115f, (float)tile->m_tileCoords.y);
		Vector3 rightBack = Vector3((float)tile->m_tileCoords.x + 1.f, tile->GetDisplayHeight() + 0.0115f, (float)tile->m_tileCoords.y + 1.f);
		Vector3 leftBack = Vector3(leftFront.x, rightBack.y, rightBack.z);
		Vector3 rightFront = Vector3(rightBack.x, rightBack.y, leftFront.z);

		vertices.push_back(Vertex(leftBack, Vector2(0.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(leftFront, Vector2(0.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(rightBack, Vector2(1.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor));

		vertices.push_back(Vertex(rightBack, Vector2(1.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(leftFront, Vector2(0.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor));
		vertices.push_back(Vertex(rightFront, Vector2(1.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor));
	}

	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->DrawVertices(vertices.data(), vertices.size());
}

void Map::DrawSelectedTile() const
{
	Vector3 topNormal(0.f, -1.f, 0.f);
	Vector3 topTangent(1.f, 0.f, 0.f);
	Vector3 topBiTangent(0.f, 0.f, 1.f);
	Rgba highlightColor(255, 255, 255, 150);

	Vector3 leftFront = Vector3((float)m_selectedTile->m_tileCoords.x, m_selectedTile->GetDisplayHeight() + 0.0125f, (float)m_selectedTile->m_tileCoords.y);
	Vector3 rightBack = Vector3((float)m_selectedTile->m_tileCoords.x + 1.f, m_selectedTile->GetDisplayHeight() + 0.0125f, (float)m_selectedTile->m_tileCoords.y + 1.f);
	Vector3 leftBack = Vector3(leftFront.x, rightBack.y, rightBack.z);
	Vector3 rightFront = Vector3(rightBack.x, rightBack.y, leftFront.z);

	Vertex selectionVerts[6]
	{
		Vertex(leftBack, Vector2(0.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor),
		Vertex(leftFront, Vector2(0.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor),
		Vertex(rightBack, Vector2(1.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor),

		Vertex(rightBack, Vector2(1.f, 0.f), topNormal, topTangent, topBiTangent, highlightColor),
		Vertex(leftFront, Vector2(0.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor),
		Vertex(rightFront, Vector2(1.f, 1.f), topNormal, topTangent, topBiTangent, highlightColor)
	};

	g_theRenderer->SetTexture(nullptr);
	g_theRenderer->DrawVertices(selectionVerts, 6);
}

std::vector<Tile*> Map::GetTilesInRadius(const IntVector2& tileCoords, float radius)
{
	std::vector<Tile*> outVector;

	float maxDistanceSquared = radius * radius;
	Vector2 centerTileCoords = (Vector2)tileCoords + Vector2(0.5f, 0.5f);

	for (int yIndex = tileCoords.y - (int)radius; yIndex <= tileCoords.y + (int)radius; yIndex++)
	{
		for (int xIndex = tileCoords.x - (int)radius; xIndex <= tileCoords.x + (int)radius; xIndex++)
		{
			Vector2 currentTileCenterCoords = Vector2(xIndex + 0.5f, yIndex + 0.5f);
			Tile* currentTile = GetTileAtMapCoords(currentTileCenterCoords);
			if(currentTile)
			{
				if (CalcDistanceSquared(currentTileCenterCoords, centerTileCoords) <= maxDistanceSquared)
				{
					outVector.push_back(currentTile);
				}
			}
		}
	}

	return outVector;
}

std::vector<Tile*> Map::GetTraversableTilesInRangeOfCharacter(const Character* character, const Tile* startingTile /*= nullptr*/)
{
	if (startingTile == nullptr)
		startingTile = character->m_currentTile;

	std::vector<int> distanceField;
	distanceField.resize(m_definition->m_dimensions.x * m_definition->m_dimensions.y, 999999);

	distanceField[CalculateTileIndexFromTileCoords(startingTile->m_tileCoords)] = 0;
	for (int distanceFieldIteration = 0; distanceFieldIteration <= character->m_stats[STAT_MOVE]; distanceFieldIteration++)
	{
		for (int distanceIndex = 0; distanceIndex < (int)distanceField.size(); distanceIndex++)
		{
			if (distanceField[distanceIndex] == distanceFieldIteration)
			{
				Tile* northNeighbor = m_tiles[distanceIndex].GetNorthNeighbor();
				Tile* southNeighbor = m_tiles[distanceIndex].GetSouthNeighbor();
				Tile* eastNeighbor = m_tiles[distanceIndex].GetEastNeighbor();
				Tile* westNeighbor = m_tiles[distanceIndex].GetWestNeighbor();

				if (northNeighbor && northNeighbor->IsTraversableToCharacterAtHeight(character, m_tiles[distanceIndex].m_height) && northNeighbor->m_occupyingCharacter == nullptr)
				{
					if (distanceField[CalculateTileIndexFromTileCoords(northNeighbor->m_tileCoords)] > (distanceFieldIteration + 1))
						distanceField[CalculateTileIndexFromTileCoords(northNeighbor->m_tileCoords)] = distanceFieldIteration + 1;
				}

				if (southNeighbor && southNeighbor->IsTraversableToCharacterAtHeight(character, m_tiles[distanceIndex].m_height) && southNeighbor->m_occupyingCharacter == nullptr)
				{
					if (distanceField[CalculateTileIndexFromTileCoords(southNeighbor->m_tileCoords)] > (distanceFieldIteration + 1))
						distanceField[CalculateTileIndexFromTileCoords(southNeighbor->m_tileCoords)] = distanceFieldIteration + 1;
				}

				if (eastNeighbor && eastNeighbor->IsTraversableToCharacterAtHeight(character, m_tiles[distanceIndex].m_height) && eastNeighbor->m_occupyingCharacter == nullptr)
				{
					if (distanceField[CalculateTileIndexFromTileCoords(eastNeighbor->m_tileCoords)] > (distanceFieldIteration + 1))
						distanceField[CalculateTileIndexFromTileCoords(eastNeighbor->m_tileCoords)] = distanceFieldIteration + 1;
				}

				if (westNeighbor && westNeighbor->IsTraversableToCharacterAtHeight(character, m_tiles[distanceIndex].m_height) && westNeighbor->m_occupyingCharacter == nullptr)
				{
					if (distanceField[CalculateTileIndexFromTileCoords(westNeighbor->m_tileCoords)] > (distanceFieldIteration + 1))
						distanceField[CalculateTileIndexFromTileCoords(westNeighbor->m_tileCoords)] = distanceFieldIteration + 1;
				}
			}
		}
	}

	std::vector<Tile*> outputVector;
	for (int tileIndex = 0; tileIndex < m_definition->m_dimensions.x * m_definition->m_dimensions.y; tileIndex++)
	{
		if (distanceField[tileIndex] > 0 && distanceField[tileIndex] <= character->m_stats[STAT_MOVE] && m_tiles[tileIndex].m_occupyingCharacter == nullptr)
			outputVector.push_back(GetTileAtTileIndex(tileIndex));
	}

	return outputVector;
}

std::vector<Tile*> Map::GetTargettableTiles(const IntVector2& startPos, int range, int maxHeightDifference)
{
	std::vector<Tile*> tempTiles;
	Tile* startTile = GetTileAtTileCoords(startPos);

	for (size_t tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++)
	{
		Tile* tile = GetTileAtTileIndex(tileIndex);
		if (CalculateManhattanDistance(*startTile, *tile) <= range && (abs(startTile->m_height - tile->m_height) <= maxHeightDifference))
		{
			tempTiles.push_back(tile);
		}
	}

	return tempTiles;
}

std::vector<Tile*> Map::GetAoETiles(const IntVector2& centerPos, int radius, int maxAreaHeightDifference)
{
	std::vector<Tile*> tempTiles;
	Tile* startTile = GetTileAtTileCoords(centerPos);

	for (size_t tileIndex = 0; tileIndex < m_tiles.size(); tileIndex++)
	{
		Tile* tile = GetTileAtTileIndex(tileIndex);
		if (CalculateManhattanDistance(*startTile, *tile) <= radius && (abs(startTile->m_height - tile->m_height) <= maxAreaHeightDifference))
		{
			tempTiles.push_back(tile);
		}
	}

	return tempTiles;
}

bool Map::IsInMap(const IntVector2& tileCoords) const
{
	if (tileCoords.x < 0 || tileCoords.x >= m_definition->m_dimensions.x)
		return false;

	if (tileCoords.y < 0 || tileCoords.y >= m_definition->m_dimensions.y)
		return false;

	return true;
}

bool Map::IsTileInTraversableTiles(Tile* selectedTile) const
{
	for (Tile* tile : m_traversableTiles)
	{
		if (tile == selectedTile)
			return true;
	}

	return false;
}

bool Map::IsTileInTargettableTiles(Tile* selectedTile) const
{
	for (Tile* tile : m_targettableTiles)
	{
		if (tile == selectedTile)
			return true;
	}

	return false;
}

Character* Map::GetCharacterWithGreatestCT()
{
	int maxCT = 0;
	Character* characterWithMaxCT = nullptr;

	for (Character* character : m_characters)
	{
		if (character->m_currentCT >= maxCT)
		{
			maxCT = character->m_currentCT;
			characterWithMaxCT = character;
		}
	}

	return characterWithMaxCT;
}

bool Map::StartMovingCharacterToTile(Character* characterToMove, Tile* destinationTile)
{
	Path newPath = GeneratePath(characterToMove->m_currentTile->m_tileCoords, destinationTile->m_tileCoords, characterToMove);
	if (newPath.empty())
		return false;

	characterToMove->StartMoving(newPath);
	return true;
}

RaycastResult Map::RaycastForSolid(const Vector2& startPosition, const Vector2& direction, float maxDistance)
{
	RaycastResult result;
	Vector2 endPosition = startPosition + (direction * maxDistance);
	Vector2 displacement = endPosition - startPosition;
	Vector2 singleStep = displacement * 0.01f;
	for (int stepIndex = 1; stepIndex < 100; stepIndex++)
	{
		Vector2 previousPosition = startPosition + (singleStep * (float)(stepIndex - 1));
		Vector2 currentPosition = startPosition + (singleStep * (float)stepIndex);
		Tile* currentTile = &m_tiles[CalculateTileIndexFromTileCoords(CalculateTileCoordsFromMapCoords(currentPosition))];
		result.m_impactedTiles.insert(currentTile);
		if (currentTile->m_tileDefinition->m_isTraversable)
		{
			result.m_didImpact = true;
			result.m_impactedTiles.insert(currentTile);

			Vector2 currentDisplacement = currentPosition - startPosition;
			result.m_impactFraction = currentDisplacement.CalcLength() / maxDistance;

			result.m_pointBeforeImpact = previousPosition;
			result.m_impactPosition = currentPosition;

			const Tile* previousTile = &m_tiles[CalculateTileIndexFromTileCoords(CalculateTileCoordsFromMapCoords(currentPosition))];
			Vector2 tileDisplacement = previousTile->m_tileCoords - currentTile->m_tileCoords;
			result.m_impactNormal = tileDisplacement.GetNormalized();

			return result;
		}
	}
	result.m_didImpact = false;
	result.m_impactedTiles;
	result.m_impactFraction = 1.0f;
	result.m_impactPosition = startPosition + (direction * maxDistance);
	result.m_pointBeforeImpact = result.m_impactPosition;
	return result;
}

RaycastResult Map::RaycastForOpaque(const Vector2& startPosition, const Vector2& direction, float maxDistance)
{
	RaycastResult result;
	Vector2 endPosition = startPosition + (direction * maxDistance);
	Vector2 displacement = endPosition - startPosition;
	Vector2 singleStep = displacement * 0.01f;
	for (int stepIndex = 1; stepIndex < 100; stepIndex++)
	{
		Vector2 previousPosition = startPosition + (singleStep * (float)(stepIndex - 1));
		Vector2 currentPosition = startPosition + (singleStep * (float)stepIndex);
		if(IsInMap(CalculateTileCoordsFromMapCoords(currentPosition)))
		{
			Tile* currentTile = &m_tiles[CalculateTileIndexFromTileCoords(CalculateTileCoordsFromMapCoords(currentPosition))];
			result.m_impactedTiles.insert(currentTile);
			if (currentTile->m_tileDefinition->m_isOpaque)
			{
				result.m_didImpact = true;
				result.m_impactedTiles.insert(currentTile);

				Vector2 currentDisplacement = currentPosition - startPosition;
				result.m_impactFraction = currentDisplacement.CalcLength() / maxDistance;

				result.m_pointBeforeImpact = previousPosition;
				result.m_impactPosition = currentPosition;

				const Tile* previousTile = &m_tiles[CalculateTileIndexFromTileCoords(CalculateTileCoordsFromMapCoords(currentPosition))];
				Vector2 tileDisplacement = previousTile->m_tileCoords - currentTile->m_tileCoords;
				result.m_impactNormal = tileDisplacement.GetNormalized();

				return result;
			}
		}
	}
	result.m_didImpact = false;
	result.m_impactFraction = 1.0f;
	result.m_impactPosition = startPosition + (direction * maxDistance);
	result.m_pointBeforeImpact = result.m_impactPosition;
	return result;
}

Path Map::GeneratePath(const IntVector2& start, const IntVector2& end, Character* characterForPath /*= nullptr*/)
{
	StartSteppedPath(start, end, characterForPath);
	Path outPath;

	bool isCompleted = false;
	while (!isCompleted)
	{
		isCompleted = ContinueSteppedPath(outPath);
	}

	return outPath;
}

void Map::StartSteppedPath(const IntVector2& start, const IntVector2& end, Character* characterForPath /*= nullptr*/)
{
	if (m_currentPath)
		delete m_currentPath;

	m_currentPath = new PathGenerator(start, end, this, characterForPath);
}

bool Map::ContinueSteppedPath(Path& out_pathWhenComplete)
{
	//select and close best open node
	OpenNode* currentNode = m_currentPath->SelectAndCloseBestOpenNode();

	if (!currentNode)
		return true;

	//see if goal
	if (currentNode->m_tile->m_tileCoords == m_currentPath->m_end)
	{
		out_pathWhenComplete = m_currentPath->CreateFinalPath(*currentNode);
		return true;
	}

	m_currentPath->OpenNodeIfValid(currentNode->m_tile->GetNorthNeighbor(), currentNode);
	m_currentPath->OpenNodeIfValid(currentNode->m_tile->GetEastNeighbor(), currentNode);
	m_currentPath->OpenNodeIfValid(currentNode->m_tile->GetSouthNeighbor(), currentNode);
	m_currentPath->OpenNodeIfValid(currentNode->m_tile->GetWestNeighbor(), currentNode);

	return false;
}

