#pragma once
#include "Game/TileDefinition.hpp"
#include "Game/Inventory.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Game/Message.hpp"
#include <string>

class Character;
class Map;

class Tile
{
public:
	Tile();
	Tile(std::string tileTypeName);
	~Tile();

	void Update(float deltaSeconds);
	void Render() const;

	void ChangeType(std::string tileTypeName);

	Tile* GetNorthNeighbor() const;
	Tile* GetSouthNeighbor() const;
	Tile* GetEastNeighbor() const;
	Tile* GetWestNeighbor() const;
	Tile* GetNorthEastNeighbor() const;
	Tile* GetNorthWestNeighbor() const;
	Tile* GetSouthEastNeighbor() const;
	Tile* GetSouthWestNeighbor() const;

	bool IsTraversableToCharacterAtHeight(const Character* character, float height);

	bool IsSolidToTags(const Tags& tagsToCheck) const;
	float GetGCost() const;

	std::vector<Vertex> GetTopVerts() const;
	std::vector<Vertex> GetSideVerts() const;

	Vector3 GetCenterDisplayPosition() const;
	float GetDisplayHeight() const;

public:
	TileDefinition* m_tileDefinition;

	Map* m_containingMap;
	IntVector2 m_tileCoords;
	Character* m_occupyingCharacter;
	Inventory m_tileInventory;

	Tags m_tags;

	float m_height;

	bool m_isVisibleToPlayer = false;
	bool m_hasBeenSeenByPlayer = false;

	int m_isClosedInPathID = 0;
	int m_isOpenInPathID = 0;
	float m_permanence;
};

