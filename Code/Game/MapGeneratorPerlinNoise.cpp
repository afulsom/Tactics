#include "Game/MapGeneratorPerlinNoise.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Game/CharacterBuilder.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Noise.hpp"

MapGeneratorPerlinNoise::MapGeneratorPerlinNoise(XMLNode element)
	: MapGenerator(element)
{
	m_perlinScale = ParseXMLAttributeFloat(element, "scale", m_perlinScale);
	m_numOctaves = ParseXMLAttributeInt(element, "octaves", m_numOctaves);
	m_octavePersistance = ParseXMLAttributeFloat(element, "octavePersistance", m_octavePersistance);
	m_octaveScale = ParseXMLAttributeFloat(element, "octaveScale", m_octaveScale);
	m_seed = ParseXMLAttributeInt(element, "seed", m_seed);

	ASSERT_OR_DIE(element.nChildNode("Rule") > 0, "No rules for Perlin Noise.");
	for (int ruleIndex = 0; ruleIndex < element.nChildNode("Rule"); ruleIndex++)
	{
		ParseRule(element.getChildNode("Rule", ruleIndex));
	}

	m_permanence = ParseXMLAttributeFloat(element, "permanence", m_permanence);
}

void MapGeneratorPerlinNoise::GenerateMap(Map*& outMapToGenerate)
{
	ApplyRulesToMap(outMapToGenerate);
}

void MapGeneratorPerlinNoise::ApplyRulesToMap(Map*& outMapToGenerate)
{
	for (size_t tileIndex = 0; tileIndex < outMapToGenerate->m_tiles.size(); tileIndex++)
	{
		IntVector2 tileCoords = outMapToGenerate->CalculateTileCoordsFromTileIndex(tileIndex);
		float noise = Compute2dPerlinNoise((float)tileCoords.x, (float)tileCoords.y, m_perlinScale, m_numOctaves, m_octavePersistance, m_octaveScale, true, m_seed);
		noise = RangeMapFloat(noise, -1.f, 1.f, 0.f, 1.f);
		for (PerlinNoiseRule rule : m_rules)
		{
			if(outMapToGenerate->m_tiles[tileIndex].m_tileDefinition->m_name == rule.m_ifTile)
			{
				if (noise >= rule.m_ifGreaterThanNumber && noise <= rule.m_ifLessThanNumber && g_random.GetRandomFloatZeroToOne() < rule.m_chanceToRunPerTile)
					PlaceTileIfPossible(&outMapToGenerate->m_tiles[tileIndex], rule.m_changeToTile, m_permanence);
			}
		}
	}
}

void MapGeneratorPerlinNoise::ParseRule(XMLNode ruleElement)
{
	PerlinNoiseRule newRule;
	newRule.m_ifTile = ParseXMLAttributeString(ruleElement, "ifTile", "INVALID_IFTILE");
	ASSERT_OR_DIE(newRule.m_ifTile != "INVALID_IFTILE", "Missing ifTile for Perlin Noise.");

	newRule.m_changeToTile = ParseXMLAttributeString(ruleElement, "changeToTile", "INVALID_CHANGETOTILE");
	ASSERT_OR_DIE(newRule.m_changeToTile != "INVALID_CHANGETOTILE", "Missing changeToTile for Perlin Noise.");

	newRule.m_chanceToRunPerTile = ParseXMLAttributeFloat(ruleElement, "chanceToRunPerTile", newRule.m_chanceToRunPerTile);
	newRule.m_ifGreaterThanNumber = ParseXMLAttributeFloat(ruleElement, "ifGreaterThan", newRule.m_ifGreaterThanNumber);
	newRule.m_ifLessThanNumber = ParseXMLAttributeFloat(ruleElement, "ifLessThan", newRule.m_ifLessThanNumber);

	m_rules.push_back(newRule);
}
