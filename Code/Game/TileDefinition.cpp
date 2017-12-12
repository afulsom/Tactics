#include "Game/TileDefinition.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Engine/Renderer/RHI/SimpleRenderer.hpp"

std::map<std::string, TileDefinition*> TileDefinition::s_tileDefinitionRegistry;

TileDefinition* TileDefinition::GetTileDefinition(std::string name)
{
	std::map<std::string, TileDefinition*>::iterator found = s_tileDefinitionRegistry.find(name);
	if (found != s_tileDefinitionRegistry.end())
		return found->second;
	else
		return nullptr;
}

TileDefinition::TileDefinition(XMLNode element)
{
	m_name = ParseXMLAttributeString(element, "name", "ERROR_INVALID_NAME");
	ASSERT_OR_DIE(m_name != "ERROR_INVALID_NAME", "No name found for TileDefinition element.");

	m_isTraversable = ParseXMLAttributeBool(element, "isTraversable", false);
	m_isOpaque = ParseXMLAttributeBool(element, "isOpaque", false);

	std::string sideTextureFilename = ParseXMLAttributeString(element, "sideTextureFilename", "INVALID_FILENAME");
	m_sideTexture = g_theRenderer->CreateTexture2DFromFile(sideTextureFilename);

	std::string topTextureFilename = ParseXMLAttributeString(element, "topTextureFilename", "INVALID_FILENAME");
	m_topTexture = g_theRenderer->CreateTexture2DFromFile(topTextureFilename);

	if (element.nChildNode("SolidExceptions") == 1)
	{
		XMLNode solidExceptionsNode = element.getChildNode("SolidExceptions");
		m_solidExceptions = ParseXMLAttributeString(solidExceptionsNode, "tags", "");
	}
	ASSERT_OR_DIE(element.nChildNode("SolidExceptions") <= 1, "Too many solid exception elements in tile definition.");

	s_tileDefinitionRegistry[m_name] = this;
}

TileDefinition::~TileDefinition()
{
	delete m_sideTexture;
	delete m_topTexture;
}
