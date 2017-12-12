#include "Game/AbilityDefinition.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Engine/Renderer/RHI/SimpleRenderer.hpp"
#include "Game/GameCommon.hpp"

std::map<std::string, AbilityDefinition*> AbilityDefinition::s_registry;



StatusEffectType AbilityDefinition::StringToStatusEffect(std::string effectName)
{
	if (effectName == "Poison")
	{
		return STATUS_POISON;
	}
	else if (effectName == "Charm")
	{
		return STATUS_CHARM;
	}
	else if (effectName == "Confuse")
	{
		return STATUS_CONFUSE;
	}
	else if (effectName == "Wall")
	{
		return STATUS_WALL;
	}

	ERROR_AND_DIE("Attempted to create status effect of invalid type.");
	return NUM_STATUS_EFFECTS;
}

AbilityDefinition::AbilityDefinition(XMLNode element)
{
	m_name = ParseXMLAttributeString(element, "name", "ERROR_INVALID_NAME");
	ASSERT_OR_DIE(m_name != "ERROR_INVALID_NAME", "No name found for ItemDefinition element.");

	m_range = ParseXMLAttributeInt(element, "range", -1);
	ASSERT_OR_DIE(m_range >= 0, "Negative or missing range for ability.");

	m_radius = ParseXMLAttributeInt(element, "radius", -1);
	ASSERT_OR_DIE(m_radius >= 0, "Negative or missing radius for ability.");

	m_maxHeightDifference = ParseXMLAttributeInt(element, "maxHeightDifference", -1);
	ASSERT_OR_DIE(m_maxHeightDifference >= 0, "Negative or missing max height difference for ability.");

	m_areaMaxHeightDifference = ParseXMLAttributeInt(element, "areaMaxHeightDifference", -1);
	ASSERT_OR_DIE(m_areaMaxHeightDifference >= 0, "Negative or missing area max height difference for ability.");

	m_power = ParseXMLAttributeInt(element, "power", 0);

	m_speed = ParseXMLAttributeInt(element, "speed", -1);
	ASSERT_OR_DIE(m_speed >= 0, "Negative or missing speed for ability.");

	XMLNode statusEffectsNode = element.getChildNode("StatusEffects");
	int numStatusEffects = statusEffectsNode.nChildNode();
	for (int statusEffectIndex = 0; statusEffectIndex < numStatusEffects; statusEffectIndex++)
	{
		m_statusEffects.push_back(StringToStatusEffect(statusEffectsNode.getChildNode(statusEffectIndex).getName()));
		int effectDuration = ParseXMLAttributeInt(statusEffectsNode.getChildNode(statusEffectIndex), "duration", 0);
		m_statusEffectDurations.push_back(effectDuration);
	}


	std::string animType = ParseXMLAttributeString(element, "animType", "INVALID_ANIM_TYPE");
	ASSERT_OR_DIE(animType != "INVALID_ANIM_TYPE" && (animType == "melee" || animType == "spell" || animType == "ranged"), "Invalid or missing anim type. Expected either \'melee\', \'spell\', or \'ranged\'.");

	if (animType == "melee")
		m_animType = ANIM_MELEE;
	else if (animType == "spell")
		m_animType = ANIM_SPELL;
	else
		m_animType = ANIM_RANGED;

	m_effectDelay = ParseXMLAttributeFloat(element, "effectDelay", 0.f);
	ASSERT_OR_DIE(m_effectDelay >= 0.f, "Invalid effect delay. Must be a positive floating point number.");

	m_particleEffectName = ParseXMLAttributeString(element, "particleEffect", "");

	std::string soundEffectFilename = ParseXMLAttributeString(element, "sound", "");
	if (!soundEffectFilename.empty())
	{
		m_soundEffect = g_theAudio->CreateOrGetSound(soundEffectFilename);
	}


	s_registry[m_name] = this;
}

AbilityDefinition::~AbilityDefinition()
{

}

AbilityDefinition* AbilityDefinition::GetAbilityDefinition(std::string name)
{
	std::map<std::string, AbilityDefinition*>::iterator found = AbilityDefinition::s_registry.find(name);
	if (found != AbilityDefinition::s_registry.end())
		return found->second;
	else
		return nullptr;
}
