#include "Game/MapGeneratorFromFile.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "CharacterBuilder.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"


MapGeneratorFromFile::MapGeneratorFromFile(XMLNode element)
	: MapGenerator(element)
{
	m_filename = ParseXMLAttributeString(element, "filename", "INVALID_FILENAME");
	ASSERT_OR_DIE(m_filename != "INVALID_FILENAME", "No filename found for GeneratorFromFile.");

	m_isMirrored = ParseXMLAttributeBool(element, "isMirrored", false);
	m_rotationRange = ParseXMLAttributeIntVector2(element, "rotation", IntVector2(0, 0));
	ASSERT_OR_DIE((m_rotationRange.x >= 0 && m_rotationRange.x <= 3) && (m_rotationRange.y >= 0 && m_rotationRange.y <= 3), "Invalid rotation for GeneratorFromFile.");

	m_xOffsetRange = ParseXMLAttributeIntVector2(element, "offsetX", IntVector2(0, 0));
	m_yOffsetRange = ParseXMLAttributeIntVector2(element, "offsetY", IntVector2(0, 0));

	m_permanence = ParseXMLAttributeFloat(element, "permanence", 0.f);
}

void MapGeneratorFromFile::GenerateMap(Map*& outMapToGenerate)
{
	XMLNode mapHead = XMLNode::parseFile(m_filename.c_str(), "Map");

	IntVector2 offset;
	offset.x = g_random.GetRandomIntInRange(m_xOffsetRange.x, m_xOffsetRange.y);
	offset.y = g_random.GetRandomIntInRange(m_yOffsetRange.x, m_yOffsetRange.y);

	int rotation;
	rotation = g_random.GetRandomIntInRange(m_rotationRange.x, m_rotationRange.y);

	//Parse legend
	XMLNode legendNode = mapHead.getChildNode("Legend");
	std::map<const char, std::string> legend;
	for (int tileIndex = 0; tileIndex < legendNode.nChildNode("Tile"); tileIndex++)
	{
		XMLNode tileNode = legendNode.getChildNode("Tile", tileIndex);
		const char glyph = ParseXMLAttributeChar(tileNode, "glyph", ' ');
		std::string tileName = ParseXMLAttributeString(tileNode, "tile", "");
		ASSERT_OR_DIE(glyph != ' ' && tileName != "", "Missing or invalid glyph or tile name.");

		legend[glyph] = tileName;
	}

	//Parse tiles
	XMLNode tiles = mapHead.getChildNode("Tiles");
	IntVector2 mapDimensions = ParseXMLAttributeIntVector2(tiles, "dimensions", IntVector2(0, 0));

	ASSERT_OR_DIE(tiles.nChildNode("Row") == mapDimensions.y, "Mismatching number of rows and dimension of map.");

	for (int rowIndex = (tiles.nChildNode("Row") - 1); rowIndex >= 0; rowIndex--)
	{
		XMLNode rowNode = tiles.getChildNode("Row", abs(rowIndex - (tiles.nChildNode("Row") - 1)));
		std::string row = ParseXMLAttributeString(rowNode, "tiles", "");
		ASSERT_OR_DIE(row.length() == (size_t)mapDimensions.y, "Mismatching length of row and dimension of map.");

		if (m_isMirrored)
			row = Reverse(row);

		for (size_t glyphIndex = 0; glyphIndex < row.length(); glyphIndex++)
		{
			std::map<const char, std::string>::iterator found = legend.find(row.at(glyphIndex));
			if (found == legend.end())
				ERROR_AND_DIE("Attempted to use glyph not found in legend.");

			IntVector2 tileCoords(glyphIndex, rowIndex);
			RotateTileCoordsInPlace(tileCoords, rotation, mapDimensions);

			std::string tileTypeName = found->second;
			if(tileTypeName != "EMPTY")
			{
				Tile* tileToChange = outMapToGenerate->GetTileAtTileCoords(tileCoords + offset);
				if (tileToChange)
					PlaceTileIfPossible(tileToChange, tileTypeName, m_permanence);
			}
		}
	}

	//Parse characters
	XMLNode characters = mapHead.getChildNode("Characters");
	for (int characterIndex = 0; characterIndex < characters.nChildNode("Character"); characterIndex++)
	{
		XMLNode characterNode = characters.getChildNode("Character", characterIndex);
		Character* newCharacter = CharacterBuilder::BuildNewCharacter(ParseXMLAttributeString(characterNode, "name", ""));
		IntVector2 characterPosition = ParseXMLAttributeIntVector2(characterNode, "position", IntVector2(-1, -1));
		ASSERT_OR_DIE(characterPosition != IntVector2(-1, -1), "Missing position for character.");
		RotateTileCoordsInPlace(characterPosition, rotation, mapDimensions);
		outMapToGenerate->PlaceCharacterInMap(newCharacter, outMapToGenerate->GetTileAtTileCoords(characterPosition + offset));
	}

	//Parse items
	XMLNode items = mapHead.getChildNode("Items");
	for (int itemIndex = 0; itemIndex < items.nChildNode("Item"); itemIndex++)
	{
		XMLNode itemNode = items.getChildNode("Item", itemIndex);
		Item* newItem = new Item(ParseXMLAttributeString(itemNode, "name", ""));
		IntVector2 itemPosition = ParseXMLAttributeIntVector2(itemNode, "position", IntVector2(-1, -1));
		ASSERT_OR_DIE(itemPosition != IntVector2(-1, -1), "Missing position for item.");
		RotateTileCoordsInPlace(itemPosition, rotation, mapDimensions);
		outMapToGenerate->PlaceItemInMap(newItem, outMapToGenerate->GetTileAtTileCoords(itemPosition + offset));
	}
}

void MapGeneratorFromFile::RotateTileCoordsInPlace(IntVector2& coordsToRotate, int rotation, const IntVector2& dimensions)
{
	switch (rotation)
	{
	case 0:
		break;
	case 1:
		coordsToRotate.Rotate90Degrees();
		coordsToRotate.x += dimensions.y;
		break;
	case 2:
		coordsToRotate.Rotate90Degrees();
		coordsToRotate.Rotate90Degrees();
		coordsToRotate = coordsToRotate + dimensions;
		break;
	case 3:
		coordsToRotate.Rotate90Degrees();
		coordsToRotate.Rotate90Degrees();
		coordsToRotate.Rotate90Degrees();
		coordsToRotate.y += dimensions.x;
		break;
	}
}
