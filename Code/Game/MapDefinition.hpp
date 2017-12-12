#pragma once
#include <string>
#include "ThirdParty\XMLParser\XMLParser.hpp"
#include <map>
#include <vector>
#include "Game/MapGenerator.hpp"



class MapDefinition
{
public:
	MapDefinition(XMLNode element);

	void GenerateMap(Map*& mapToGenerateIn);
	bool StepGeneration(Map*& mapToGenerateIn);
	void DebugRender(const Map* mapToDrawOn) const;

	std::string m_name;
	std::string m_fillTileType;
	IntVector2 m_dimensions;
	std::vector<MapGenerator*> m_generators;
	unsigned int m_currentGeneratorIndex = 0;

	static MapDefinition* GetDefinition(std::string definitionName);
	static std::map<std::string, MapDefinition*> s_registry;
};