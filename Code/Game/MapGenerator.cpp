#include "Game/MapGenerator.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Game/MapGeneratorFromFile.hpp"
#include "Game/MapGeneratorPerlinNoise.hpp"



MapGenerator::MapGenerator(XMLNode element)
{
	m_name = ParseXMLAttributeString(element, "name", "INVALID_NAME");
	ASSERT_OR_DIE(m_name != "INVALID_NAME", "No name found for map generator.");

	m_chanceToRun = ParseXMLAttributeFloat(element, "chanceToRun", 1.f);
}

void MapGenerator::PlaceTileIfPossible(Tile* tileToChange, std::string newType, float newPermanence)
{
	if (tileToChange->m_permanence > newPermanence)
		return;

	tileToChange->ChangeType(newType);
	tileToChange->m_permanence = newPermanence;
}

MapGenerator* MapGenerator::Create(XMLNode element)
{
	std::string elementName = element.getName();

	if (elementName == "FromFile")
		return new MapGeneratorFromFile(element);

	if (elementName == "Perlin")
		return new MapGeneratorPerlinNoise(element);

	ERROR_AND_DIE("Invalid generator name.");
}
