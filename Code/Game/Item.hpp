#pragma once
#include "Engine\Core\Rgba.hpp"
#include "Game/Stats.hpp"
#include "Game/ItemDefinition.hpp"
#include <string>
#include <vector>
#include "Engine\Gameplay\Tags.hpp"



class Item
{
public:
	Item(std::string typeName);
	~Item();

	int CalculateTotalStatModifier() const;
	void Use();

	Stats m_stats;
	ItemDefinition* m_definition;
	Tags m_damageTypes;
};

