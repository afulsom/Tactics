#pragma once
#include "ThirdParty\XMLParser\XMLParser.hpp"
#include <string>
#include "Game/MapGenerator.hpp"
#include <time.h>

struct PerlinNoiseRule
{
	std::string m_ifTile;
	std::string m_changeToTile;
	float m_ifGreaterThanNumber = 0.f;
	float m_ifLessThanNumber = 1.f;
	float m_chanceToRunPerTile = 1.f;
};

class MapGeneratorPerlinNoise : public MapGenerator
{
public:
	MapGeneratorPerlinNoise(XMLNode element);
	virtual void GenerateMap(Map*& outMapToGenerate) override;

	std::vector<PerlinNoiseRule> m_rules;
	float m_perlinScale = 30.f;
	unsigned int m_numOctaves = 3;
	float m_octavePersistance = 0.5f;
	float m_octaveScale = 2.f;
	unsigned int m_seed = (unsigned int)time(NULL);
	float m_permanence = 0.5f;
private:
	void ApplyRulesToMap(Map*& outMapToGenerate);
	void ParseRule(XMLNode ruleElement);
};