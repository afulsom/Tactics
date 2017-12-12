#include "Game/MapDefinition.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineConfig.hpp"


std::map<std::string, MapDefinition*> MapDefinition::s_registry;

MapDefinition::MapDefinition(XMLNode element)
{
	m_name = ParseXMLAttributeString(element, "name", "ERROR_INVALID_NAME");
	ASSERT_OR_DIE(m_name != "ERROR_INVALID_NAME", "No name found for MapDefinition element.");

	m_dimensions = ParseXMLAttributeIntVector2(element, "dimensions", IntVector2(0, 0));
	ASSERT_OR_DIE(m_dimensions != IntVector2(0, 0), "No dimensions or invalid dimensions found for MapDefinition.");
	
	m_fillTileType = ParseXMLAttributeString(element, "fillTile", "INVALID_FILL_TILE");
	ASSERT_OR_DIE(m_fillTileType != "INVALID_FILL_TILE", "No fill tile found for MapDefinition.");
	
	XMLNode generators = element.getChildNode("Generators");
	if(!generators.isEmpty())
	{
		for (int generatorIndex = 0; generatorIndex < generators.nChildNode(); generatorIndex++)
		{
			m_generators.push_back(MapGenerator::Create(generators.getChildNode(generatorIndex)));
		}
	}

	s_registry[m_name] = this;
}

void MapDefinition::GenerateMap(Map*& mapToGenerateIn)
{
	for (size_t generatorIndex = m_currentGeneratorIndex; generatorIndex < m_generators.size(); generatorIndex++)
	{
		m_generators[generatorIndex]->GenerateMap(mapToGenerateIn);
	}
}

bool MapDefinition::StepGeneration(Map*& mapToGenerateIn)
{
	m_generators[m_currentGeneratorIndex]->GenerateMap(mapToGenerateIn);

	++m_currentGeneratorIndex;
	if (m_currentGeneratorIndex == m_generators.size())
	{
		m_currentGeneratorIndex = 0;
		return true;
	}
	else
		return false;
}

void MapDefinition::DebugRender(const Map* mapToDrawOn) const
{
	for (Tile tile : mapToDrawOn->m_tiles)
	{
		g_theRenderer->DrawCenteredText2D((Vector2)tile.m_tileCoords + Vector2(0.5f, 0.5f), g_theRenderer->m_defaultFont, std::to_string(tile.m_permanence).substr(0, 4), Rgba::RED, 0.25f);
	}
}

MapDefinition* MapDefinition::GetDefinition(std::string definitionName)
{
	std::map<std::string, MapDefinition*>::iterator found = MapDefinition::s_registry.find(definitionName);
	if (found != MapDefinition::s_registry.end())
		return found->second;
	else
		return nullptr;

}
