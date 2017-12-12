#pragma once
#include "Game/GameCommon.hpp"
#include "Game/Character.hpp"
#include "Game/Message.hpp"
#include "Game/Tile.hpp"
#include <set>
#include "Engine/Renderer/RHI/VertexBuffer.hpp"
#include "Engine/Renderer/RHI/SpriteAnimation2D.hpp"
#include "Engine/Core/ParticleSystem.hpp"


typedef std::vector<Tile*> Path;

class MapDefinition;
class Map;

struct SpriteEffect
{
	SpriteEffect(SpriteAnimation2D* anim, const Vector3& position) : m_anim(anim), m_position(position) {}

	void Update(float deltaSeconds)
	{
		m_anim->Update(deltaSeconds);
	}

	SpriteAnimation2D* m_anim;
	Vector3 m_position;
};

struct DamageNumber
{
	DamageNumber::DamageNumber(std::string number, const Vector3& position, const Rgba& color = Rgba::RED, float scale = 0.5f);

	Rgba m_color;
	std::string m_number;
	Vector3 m_position;
	float m_scale;
	float m_lifetime;
};

struct RaycastResult
{
	bool m_didImpact;
	float m_impactFraction;
	Vector2 m_pointBeforeImpact;
	Vector2 m_impactPosition;
	Vector2 m_impactNormal;
	std::set<Tile*> m_impactedTiles;
};

struct OpenNode
{
	Tile* m_tile;
	OpenNode* m_parent;
	float m_localGCost = 1.f;
	float m_totalGCost = 0.f;
	float m_estimatedDistToGoal = 0.f;
	float m_fScore = 0.f;
};

class PathGenerator
{
	friend class Map;

private:
	PathGenerator(const IntVector2& start, const IntVector2& end, Map* map, Character* gCostReferenceCharacter);

	void OpenNodeForProcessing(Tile& tileToOpen, OpenNode* parent);
	OpenNode* SelectAndCloseBestOpenNode();
	Path CreateFinalPath(OpenNode& endNode);
	void OpenNodeIfValid(Tile* tileToOpen, OpenNode* parent);

	IntVector2 m_start;
	IntVector2 m_end;
	Map* m_map = nullptr;
	Character* m_gCostReferenceCharacter = nullptr;
	std::vector<OpenNode*> m_openList;
	int m_pathID;

	Path m_finalPath;
};

struct DrawCall
{
	Texture2D* m_texture;
	std::vector<Vertex> m_verts;
	VertexBuffer* m_VBO;
};

class Map
{
public:
	Map(std::string mapDefinitionName);
	~Map();

	void Update(float deltaSeconds);
	void Render() const;

	void RenderDebugPathing() const;
	void RenderDebugGenerating() const;

	void DrawTraversableTiles(const std::vector<Tile*>& traversableTiles) const;
	void DrawTargettableTiles() const;
	void DrawAoETiles() const;
	void DrawSelectedTile() const;

	void TickCT();

	int CalculateTileIndexFromTileCoords(const IntVector2& tileCoords) const;
	IntVector2 CalculateTileCoordsFromTileIndex(int tileIndex) const;
	IntVector2 CalculateTileCoordsFromMapCoords(const Vector2& mapCoords) const;
	Tile* GetTileAtMapCoords(const Vector2& mapCoords);
	Tile* GetTileAtTileCoords(const IntVector2& tileCoords);
	Tile* GetTileAtTileIndex(int tileIndex);
	Tile* FindFirstTraversableTile();
	Tile* GetRandomTraversableTile();
	Tile* GetRandomTileOfType(std::string tileType);
	Tile* GetRandomTileWithTags(std::string m_patrolPointTags);
	Tile* GetRandomTile();
	bool IsInMap(const IntVector2& tileCoords) const;
	bool IsTileInTraversableTiles(Tile* tile) const;
	bool IsTileInTargettableTiles(Tile* selectedTile) const;

	Character* GetCharacterWithGreatestCT();
	bool StartMovingCharacterToTile(Character* characterToMove, Tile* destinationTile);
	bool TryToMoveCharacterToTile(Character* characterToMove, Tile* destinationTile);
	void DestroyCharacter(Character* entityToKill);

	void PlaceCharacterInMap(Character* characterToPlace, Tile* destinationTile);
	void PlaceItemInMap(Item* itemToPlace, Tile* tileToPlaceIn);

	void SpawnActors();

	int CalculateManhattanDistance(const Tile& tileA, const Tile& tileB);
	Character* FindNearestCharacterOfFaction(const IntVector2& startingPosition, std::string faction);
	Character* FindNearestCharacterNotOfFaction(const IntVector2& startingPosition, std::string faction);
	std::vector<Character*> FindAllCharactersOfFaction(std::string faction);
	std::vector<Character*> FindAllCharactersNotOfFaction(std::string faction);
	Tile* FindNearestTileOfType(const IntVector2& startingPosition, std::string type);
	Tile* FindNearestTileNotOfType(const IntVector2& startingPosition, std::string type);
	std::vector<Tile*> GetTilesInRadius(const IntVector2& tileCoords, float radius);
	std::vector<Tile*> GetTraversableTilesInRangeOfCharacter(const Character* character, const Tile* startingTile = nullptr);
	std::vector<Tile*> GetTargettableTiles(const IntVector2& startPos, int range, int maxHeightDifference);
	std::vector<Tile*> GetAoETiles(const IntVector2& centerPos, int radius, int maxAreaHeightDifference);

	RaycastResult RaycastForSolid(const Vector2& startPosition, const Vector2& direction, float maxDistance);
	RaycastResult RaycastForOpaque(const Vector2& startPosition, const Vector2& direction, float maxDistance);

	Path GeneratePath(const IntVector2& start, const IntVector2& end, Character* characterForPath = nullptr);
	void StartSteppedPath(const IntVector2& start, const IntVector2& end, Character* characterForPath = nullptr);
	bool ContinueSteppedPath(Path& out_pathWhenComplete);

	bool m_isWaitingForInput = false;

	Character* m_selectedCharacter;
	Character* m_activeCharacter = nullptr;
	Tile* m_selectedTile;
	std::vector<Tile*> m_traversableTiles;
	std::vector<Tile*> m_targettableTiles;
	std::vector<Tile*> m_AoETiles;

	std::string m_name;
	MapDefinition* m_definition;
	std::vector<Tile> m_tiles;
	std::vector<Character*> m_characters;
	std::vector<DamageNumber> m_damageNumbers;
	std::vector<SpriteEffect> m_spriteEffects;
	ParticleSystem m_particleSystem;

	std::vector<DrawCall> m_drawCalls;

	PathGenerator* m_currentPath = nullptr;

	static const float DAMAGE_NUMBER_LIFETIME;
private:
	void MoveCharacterToTile(Character* characterToMove, Tile* destinationTile);
	void UpdateDamageNumbers(float deltaSeconds);
	void RenderDamageNumbers() const;

	void BuildTileVerts();
	int FindTextureInDrawCalls(const Texture2D* texture);
	void DrawSpriteEffect(SpriteEffect effect) const;
};
