#pragma once
#include "ThirdParty\XMLParser\XMLParser.hpp"
#include <string>
#include "Game/Map.hpp"


class MapGenerator
{
public:
	MapGenerator(XMLNode element);

	virtual void GenerateMap(Map*& outMapToGenerate) = 0;
	void PlaceTileIfPossible(Tile* tileToChange, std::string newType, float newPermanence);

	std::string m_name;
	float m_chanceToRun = 1.f;

	static MapGenerator* Create(XMLNode element);
};