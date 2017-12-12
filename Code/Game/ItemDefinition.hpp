#pragma once
#include "Engine\Core\Rgba.hpp"
#include "Game/Stats.hpp"
#include <string>
#include "ThirdParty\XMLParser\XMLParser.hpp"
#include <map>


enum EquipSlot
{
	EQUIP_SLOT_NONE = -1,
	EQUIP_SLOT_PRIMARY_WEAPON,
	EQUIP_SLOT_HEAD,
	EQUIP_SLOT_NECK,
	EQUIP_SLOT_CHEST,
	EQUIP_SLOT_WAIST,
	EQUIP_SLOT_WRISTS,
	EQUIP_SLOT_LEFT_RING,
	EQUIP_SLOT_RIGHT_RING,
	EQUIP_SLOT_HANDS,
	EQUIP_SLOT_LEGS,
	EQUIP_SLOT_FEET,
	NUM_EQUIP_SLOTS
};



class ItemDefinition
{
public:
	ItemDefinition(XMLNode element);

	Stats m_minStats;
	Stats m_maxStats;
	EquipSlot m_slot;

	std::string m_name;
	char m_glyph;
	Rgba m_glyphColor;
	Rgba m_fillColor;

	std::string m_damageTypeString;

	static ItemDefinition* GetItemDefinition(std::string name);
	static EquipSlot GetEquipSlotFromString(std::string slotString);
	static std::string GetStringFromEquipSlot(EquipSlot slot);
	static std::map<std::string, ItemDefinition*> s_registry;
};