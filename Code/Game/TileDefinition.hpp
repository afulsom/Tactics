#pragma once
#include <string>
#include <vector>
#include <map>
#include "Engine\Core\Rgba.hpp"
#include "Engine\Renderer\RHI\Texture2D.hpp"

struct XMLNode;

class TileDefinition
{
public:
	TileDefinition(XMLNode element);
	~TileDefinition();

	std::string m_name;
	bool m_isTraversable;
	bool m_isOpaque;
	std::string m_solidExceptions;
	Texture2D* m_sideTexture;
	Texture2D* m_topTexture;

	static std::map<std::string, TileDefinition*> s_tileDefinitionRegistry;
	static TileDefinition* GetTileDefinition(std::string name);
};