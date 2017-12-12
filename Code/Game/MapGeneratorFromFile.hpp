#pragma once
#include "ThirdParty\XMLParser\XMLParser.hpp"
#include <string>
#include "Game/MapGenerator.hpp"


class MapGeneratorFromFile : public MapGenerator
{
public:
	MapGeneratorFromFile(XMLNode element);

	virtual void GenerateMap(Map*& outMapToGenerate) override;

	std::string m_filename;
	IntVector2 m_rotationRange;
	bool m_isMirrored;
	IntVector2 m_xOffsetRange;
	IntVector2 m_yOffsetRange;
	float m_permanence;
private:
	void RotateTileCoordsInPlace(IntVector2& coordsToRotate, int rotation, const IntVector2& dimensions);
};