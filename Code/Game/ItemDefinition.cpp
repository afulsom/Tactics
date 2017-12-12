#include "Game/ItemDefinition.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

std::map<std::string, ItemDefinition*> ItemDefinition::s_registry;

EquipSlot ItemDefinition::GetEquipSlotFromString(std::string slotString)
{
	if (slotString == "none")		return EQUIP_SLOT_NONE;
	if (slotString == "primary")	return EQUIP_SLOT_PRIMARY_WEAPON;
	if (slotString == "head")		return EQUIP_SLOT_HEAD;
	if (slotString == "neck")		return EQUIP_SLOT_NECK;
	if (slotString == "chest")		return EQUIP_SLOT_CHEST;
	if (slotString == "waist")		return EQUIP_SLOT_WAIST;
	if (slotString == "wrists")		return EQUIP_SLOT_WRISTS;
	if (slotString == "left ring")	return EQUIP_SLOT_LEFT_RING;
	if (slotString == "right ring")	return EQUIP_SLOT_RIGHT_RING;
	if (slotString == "hands")		return EQUIP_SLOT_HANDS;
	if (slotString == "legs")		return EQUIP_SLOT_LEGS;
	if (slotString == "feet")		return EQUIP_SLOT_FEET;

	ERROR_AND_DIE("Attempted to get invalid equip slot.")
}

std::string ItemDefinition::GetStringFromEquipSlot(EquipSlot slot)
{
	if (slot == EQUIP_SLOT_NONE)				return "none";
	if (slot == EQUIP_SLOT_PRIMARY_WEAPON)		return "primary";
	if (slot == EQUIP_SLOT_HEAD)				return "head";
	if (slot == EQUIP_SLOT_NECK)				return "neck";
	if (slot == EQUIP_SLOT_CHEST)				return "chest";
	if (slot == EQUIP_SLOT_WAIST)				return "waist";
	if (slot == EQUIP_SLOT_WRISTS)				return "wrists";
	if (slot == EQUIP_SLOT_LEFT_RING)			return "left ring";
	if (slot == EQUIP_SLOT_RIGHT_RING)			return "right ring";
	if (slot == EQUIP_SLOT_HANDS)				return "hands";
	if (slot == EQUIP_SLOT_LEGS)				return "legs";
	if (slot == EQUIP_SLOT_LEGS)				return "feet";

	return "INVALID SLOT";
}

ItemDefinition::ItemDefinition(XMLNode element)
{
	m_name = ParseXMLAttributeString(element, "name", "ERROR_INVALID_NAME");
	ASSERT_OR_DIE(m_name != "ERROR_INVALID_NAME", "No name found for ItemDefinition element.");

	m_glyph = ParseXMLAttributeChar(element, "glyph", ' ');

	m_glyphColor = ParseXMLAttributeRgba(element, "glyphColor", Rgba::WHITE);

	m_fillColor = ParseXMLAttributeRgba(element, "fillColor", Rgba::WHITE);

	m_minStats.m_stats[STAT_BRAVERY] = ParseXMLAttributeInt(element, "minBravery", 0);
	m_maxStats.m_stats[STAT_BRAVERY] = ParseXMLAttributeInt(element, "maxBravery", 0);

	m_minStats.m_stats[STAT_FAITH] = ParseXMLAttributeInt(element, "minFaith", 0);
	m_maxStats.m_stats[STAT_FAITH] = ParseXMLAttributeInt(element, "maxFaith", 0);

	m_minStats.m_stats[STAT_SPEED] = ParseXMLAttributeInt(element, "minSpeed", 0);
	m_maxStats.m_stats[STAT_SPEED] = ParseXMLAttributeInt(element, "maxSpeed", 0);

	m_minStats.m_stats[STAT_ATTACK] = ParseXMLAttributeInt(element, "minAttack", 0);
	m_maxStats.m_stats[STAT_ATTACK] = ParseXMLAttributeInt(element, "maxAttack", 0);

	m_minStats.m_stats[STAT_EVASION] = ParseXMLAttributeInt(element, "minEvasion", 0);
	m_maxStats.m_stats[STAT_EVASION] = ParseXMLAttributeInt(element, "maxEvasion", 0);

	m_minStats.m_stats[STAT_MOVE] = ParseXMLAttributeInt(element, "minMove", 0);
	m_maxStats.m_stats[STAT_MOVE] = ParseXMLAttributeInt(element, "maxMove", 0);

	m_minStats.m_stats[STAT_JUMP] = ParseXMLAttributeInt(element, "minJump", 0);
	m_maxStats.m_stats[STAT_JUMP] = ParseXMLAttributeInt(element, "maxJump", 0);

	m_minStats.m_stats[STAT_MAX_HP] = ParseXMLAttributeInt(element, "minHP", 0);
	m_maxStats.m_stats[STAT_MAX_HP] = ParseXMLAttributeInt(element, "maxHP", 0);

	m_minStats.m_stats[STAT_MAX_MP] = ParseXMLAttributeInt(element, "minMP", 0);
	m_maxStats.m_stats[STAT_MAX_MP] = ParseXMLAttributeInt(element, "maxMP", 0);


	if (element.nChildNode("EquipSlot") == 0)
	{
		m_slot = EQUIP_SLOT_NONE;
	}
	else
	{
		XMLNode equipSlot = element.getChildNode("EquipSlot");
		std::string slotString = ParseXMLAttributeString(equipSlot, "slot", "ERROR_INVALID_SLOT");
		ASSERT_OR_DIE(slotString != "ERROR_INVALID_SLOT", "No slot attribute found for EquipSlot element.");

		m_slot = GetEquipSlotFromString(slotString);
	}

	if(element.nChildNode("DamageTypes") > 0)
	{
		XMLNode damageTypeElement = element.getChildNode("DamageTypes");
		m_damageTypeString = ParseXMLAttributeString(damageTypeElement, "tags", "");
	}
	else
	{
		m_damageTypeString = "";
	}

	s_registry[m_name] = this;
}

ItemDefinition* ItemDefinition::GetItemDefinition(std::string name)
{
	std::map<std::string, ItemDefinition*>::iterator found = ItemDefinition::s_registry.find(name);
	if (found != ItemDefinition::s_registry.end())
		return found->second;
	else
		return nullptr;
}
