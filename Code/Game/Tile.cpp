#include "Game/Tile.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Engine/Renderer/RHI/SimpleRenderer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include <string>

Tile::Tile()
	: m_tileCoords(0, 0)
	, m_containingMap(nullptr)
	, m_occupyingCharacter(nullptr)
	, m_tileInventory()
	, m_tileDefinition(nullptr)
	, m_permanence(0.f)
	, m_tags()
	, m_height(5.f)
{

}

Tile::Tile(std::string tileTypeName)
	: m_tileCoords(0, 0)
	, m_containingMap(nullptr)
	, m_occupyingCharacter(nullptr)
	, m_tileInventory()
	, m_permanence(0.f)
	, m_tags()
	, m_height(5.f)
{
	TileDefinition* tileDefinition = TileDefinition::GetTileDefinition(tileTypeName);
	if (tileDefinition == nullptr)
		ERROR_AND_DIE("INVALID TILE DEFINITION USED.");

	m_tileDefinition = tileDefinition;
}

Tile::~Tile()
{
	
}

void Tile::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	if (m_tileDefinition == nullptr)
		ERROR_AND_DIE("TILE HAS NO TYPE.");
}

void Tile::Render() const
{

}

void Tile::ChangeType(std::string tileTypeName)
{
	TileDefinition* tileDefinition = TileDefinition::GetTileDefinition(tileTypeName);
	if (tileDefinition == nullptr)
		ERROR_AND_DIE("INVALID TILE DEFINITION USED.");

	m_tileDefinition = tileDefinition;
}

Tile* Tile::GetNorthNeighbor() const
{
	return m_containingMap->GetTileAtTileCoords(m_tileCoords + IntVector2(0, 1));
}

Tile* Tile::GetSouthNeighbor() const
{
	return m_containingMap->GetTileAtTileCoords(m_tileCoords + IntVector2(0, -1));
}

Tile* Tile::GetEastNeighbor() const
{
	return m_containingMap->GetTileAtTileCoords(m_tileCoords + IntVector2(1, 0));
}

Tile* Tile::GetWestNeighbor() const
{
	return m_containingMap->GetTileAtTileCoords(m_tileCoords + IntVector2(-1, 0));
}

Tile* Tile::GetNorthEastNeighbor() const
{
	return m_containingMap->GetTileAtTileCoords(m_tileCoords + IntVector2(1, 1));
}

Tile* Tile::GetNorthWestNeighbor() const
{
	return m_containingMap->GetTileAtTileCoords(m_tileCoords + IntVector2(-1, 1));
}

Tile* Tile::GetSouthEastNeighbor() const
{
	return m_containingMap->GetTileAtTileCoords(m_tileCoords + IntVector2(1, -1));
}

Tile* Tile::GetSouthWestNeighbor() const
{
	return m_containingMap->GetTileAtTileCoords(m_tileCoords + IntVector2(-1, -1));
}

bool Tile::IsTraversableToCharacterAtHeight(const Character* character, float height)
{
	if (!character)
		return false;

	if (height + character->m_stats[STAT_JUMP] >= m_height)
		return true;

	return false;
}

bool Tile::IsSolidToTags(const Tags& tagsToCheck) const
{
	if (m_tileDefinition->m_solidExceptions.empty())
		return m_tileDefinition->m_isTraversable;

	if (tagsToCheck.MatchTags(m_tileDefinition->m_solidExceptions))
		return !m_tileDefinition->m_isTraversable;
	else
		return m_tileDefinition->m_isTraversable;
}

float Tile::GetGCost() const
{
	return 1.f;
}

std::vector<Vertex> Tile::GetTopVerts() const
{
	Vector3 topNormal(0.f, -1.f, 0.f);
	Vector3 bottomNormal(0.f, 1.f, 0.f);

	Vector3 topTangent(1.f, 0.f, 0.f);
	Vector3 bottomTangent(1.f, 0.f, 0.f);

	Vector3 topBiTangent(0.f, 0.f, 1.f);
	Vector3 bottomBiTangent(0.f, 0.f, -1.f);

	Vector3 bottomLeftFront = Vector3((float)m_tileCoords.x, 0.f, (float)m_tileCoords.y);
	Vector3 topRightBack = Vector3((float)m_tileCoords.x + 1.f, GetDisplayHeight(), (float)m_tileCoords.y + 1.f);

	Vector3 bottomLeftBack = Vector3(bottomLeftFront.x, bottomLeftFront.y, topRightBack.z);
	Vector3 bottomRightFront = Vector3(topRightBack.x, bottomLeftFront.y, bottomLeftFront.z);
	Vector3 bottomRightBack = Vector3(topRightBack.x, bottomLeftFront.y, topRightBack.z);

	Vector3 topRightFront = Vector3(topRightBack.x, topRightBack.y, bottomLeftFront.z);
	Vector3 topLeftBack = Vector3(bottomLeftFront.x, topRightBack.y, topRightBack.z);
	Vector3 topLeftFront = Vector3(bottomLeftFront.x, topRightBack.y, bottomLeftFront.z);

	std::vector<Vertex> outVector
	{
		//Top
		Vertex(topLeftBack, Vector2(0.f, 0.f), topNormal, topTangent, topBiTangent),
		Vertex(topLeftFront, Vector2(0.f, 1.f), topNormal, topTangent, topBiTangent),
		Vertex(topRightBack, Vector2(1.f, 0.f), topNormal, topTangent, topBiTangent),

		Vertex(topRightBack, Vector2(1.f, 0.f), topNormal, topTangent, topBiTangent),
		Vertex(topLeftFront, Vector2(0.f, 1.f), topNormal, topTangent, topBiTangent),
		Vertex(topRightFront, Vector2(1.f, 1.f), topNormal, topTangent, topBiTangent),

		//Bottom
		Vertex(bottomLeftFront, Vector2(0.f, 0.f), bottomNormal, bottomTangent, bottomBiTangent),
		Vertex(bottomLeftBack, Vector2(0.f, 1.f), bottomNormal, bottomTangent, bottomBiTangent),
		Vertex(bottomRightFront, Vector2(1.f, 0.f), bottomNormal, bottomTangent, bottomBiTangent),

		Vertex(bottomRightFront, Vector2(1.f, 0.f), bottomNormal, bottomTangent, bottomBiTangent),
		Vertex(bottomLeftBack, Vector2(0.f, 1.f), bottomNormal, bottomTangent, bottomBiTangent),
		Vertex(bottomRightBack, Vector2(1.f, 1.f), bottomNormal, bottomTangent, bottomBiTangent)
	};
	
	return outVector;
}

std::vector<Vertex> Tile::GetSideVerts() const
{
	Vector3 frontNormal(0.f, 0.f, 1.f);
	Vector3 backNormal(0.f, 0.f, -1.f);
	Vector3 leftNormal(1.f, 0.f, 0.f);
	Vector3 rightNormal(-1.f, 0.f, 0.f);

	Vector3 frontTangent(1.f, 0.f, 0.f);
	Vector3 backTangent(-1.f, 0.f, 0.f);
	Vector3 leftTangent(0.f, 0.f, -1.f);
	Vector3 rightTangent(0.f, 0.f, 1.f);

	Vector3 frontBiTangent(0.f, 1.f, 0.f);
	Vector3 backBiTangent(0.f, 1.f, 0.f);
	Vector3 leftBiTangent(0.f, 1.f, 0.f);
	Vector3 rightBiTangent(0.f, 1.f, 0.f);

	std::vector<Vertex> sideVerts;

	Rgba frontColor = Rgba::WHITE;
	Rgba backColor = Rgba::WHITE;
	Rgba rightColor = Rgba::WHITE;
	Rgba leftColor = Rgba::WHITE;

	if (!GetSouthNeighbor())
		frontColor = Rgba::BLACK;

	if (!GetNorthNeighbor())
		backColor = Rgba::BLACK;

	if (!GetEastNeighbor())
		rightColor = Rgba::BLACK;

	if (!GetWestNeighbor())
		leftColor = Rgba::BLACK;

	for (int tilingIndex = 0; tilingIndex < m_height; tilingIndex++)
	{
		Vector3 tiledBottomLeftFront = Vector3((float)m_tileCoords.x, (float)tilingIndex * 0.33f, (float)m_tileCoords.y);
		Vector3 tiledTopRightBack = Vector3((float)m_tileCoords.x + 1.f, (float)(tilingIndex + 1) * 0.33f, (float)m_tileCoords.y + 1.f);

		Vector3 tiledBottomLeftBack = Vector3(tiledBottomLeftFront.x, tiledBottomLeftFront.y, tiledTopRightBack.z);
		Vector3 tiledBottomRightFront = Vector3(tiledTopRightBack.x, tiledBottomLeftFront.y, tiledBottomLeftFront.z);
		Vector3 tiledBottomRightBack = Vector3(tiledTopRightBack.x, tiledBottomLeftFront.y, tiledTopRightBack.z);

		Vector3 tiledTopRightFront = Vector3(tiledTopRightBack.x, tiledTopRightBack.y, tiledBottomLeftFront.z);
		Vector3 tiledTopLeftBack = Vector3(tiledBottomLeftFront.x, tiledTopRightBack.y, tiledTopRightBack.z);
		Vector3 tiledTopLeftFront = Vector3(tiledBottomLeftFront.x, tiledTopRightBack.y, tiledBottomLeftFront.z);

		//Front
		sideVerts.push_back(Vertex(tiledTopLeftFront, Vector2(0.f, 0.f), frontNormal, frontTangent, frontBiTangent, frontColor));
		sideVerts.push_back(Vertex(tiledBottomLeftFront, Vector2(0.f, 1.f), frontNormal, frontTangent, frontBiTangent, frontColor));
		sideVerts.push_back(Vertex(tiledTopRightFront, Vector2(1.f, 0.f), frontNormal, frontTangent, frontBiTangent, frontColor));

		sideVerts.push_back(Vertex(tiledBottomLeftFront, Vector2(0.f, 1.f), frontNormal, frontTangent, frontBiTangent, frontColor));
		sideVerts.push_back(Vertex(tiledBottomRightFront, Vector2(1.f, 1.f), frontNormal, frontTangent, frontBiTangent, frontColor));
		sideVerts.push_back(Vertex(tiledTopRightFront, Vector2(1.f, 0.f), frontNormal, frontTangent, frontBiTangent, frontColor));

		//Right
		sideVerts.push_back(Vertex(tiledTopRightFront, Vector2(0.f, 0.f), rightNormal, rightTangent, rightBiTangent, rightColor));
		sideVerts.push_back(Vertex(tiledBottomRightFront, Vector2(0.f, 1.f), rightNormal, rightTangent, rightBiTangent, rightColor));
		sideVerts.push_back(Vertex(tiledTopRightBack, Vector2(1.f, 0.f), rightNormal, rightTangent, rightBiTangent, rightColor));

		sideVerts.push_back(Vertex(tiledTopRightBack, Vector2(1.f, 0.f), rightNormal, rightTangent, rightBiTangent, rightColor));
		sideVerts.push_back(Vertex(tiledBottomRightFront, Vector2(0.f, 1.f), rightNormal, rightTangent, rightBiTangent, rightColor));
		sideVerts.push_back(Vertex(tiledBottomRightBack, Vector2(1.f, 1.f), rightNormal, rightTangent, rightBiTangent, rightColor));

		//Back
		sideVerts.push_back(Vertex(tiledTopRightBack, Vector2(0.f, 0.f), backNormal, backTangent, backBiTangent, backColor));
		sideVerts.push_back(Vertex(tiledBottomRightBack, Vector2(0.f, 1.f), backNormal, backTangent, backBiTangent, backColor));
		sideVerts.push_back(Vertex(tiledTopLeftBack, Vector2(1.f, 0.f), backNormal, backTangent, backBiTangent, backColor));

		sideVerts.push_back(Vertex(tiledTopLeftBack, Vector2(1.f, 0.f), backNormal, backTangent, backBiTangent, backColor));
		sideVerts.push_back(Vertex(tiledBottomRightBack, Vector2(0.f, 1.f), backNormal, backTangent, backBiTangent, backColor));
		sideVerts.push_back(Vertex(tiledBottomLeftBack, Vector2(1.f, 1.f), backNormal, backTangent, backBiTangent, backColor));

		//Left
		sideVerts.push_back(Vertex(tiledTopLeftBack, Vector2(0.f, 0.f), leftNormal, leftTangent, leftBiTangent, leftColor));
		sideVerts.push_back(Vertex(tiledBottomLeftBack, Vector2(0.f, 1.f), leftNormal, leftTangent, leftBiTangent, leftColor));
		sideVerts.push_back(Vertex(tiledTopLeftFront, Vector2(1.f, 0.f), leftNormal, leftTangent, leftBiTangent, leftColor));

		sideVerts.push_back(Vertex(tiledTopLeftFront, Vector2(1.f, 0.f), leftNormal, leftTangent, leftBiTangent, leftColor));
		sideVerts.push_back(Vertex(tiledBottomLeftBack, Vector2(0.f, 1.f), leftNormal, leftTangent, leftBiTangent, leftColor));
		sideVerts.push_back(Vertex(tiledBottomLeftFront, Vector2(1.f, 1.f), leftNormal, leftTangent, leftBiTangent, leftColor));
	}

	return sideVerts;
}

Vector3 Tile::GetCenterDisplayPosition() const
{
	return Vector3(m_tileCoords.x + 0.5f, GetDisplayHeight(), m_tileCoords.y + 0.5f);
}

float Tile::GetDisplayHeight() const
{
	return (m_height * 0.33f);
}
