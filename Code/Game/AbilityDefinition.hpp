#pragma once
#include "Engine\Core\Rgba.hpp"
#include "Game/Stats.hpp"
#include <string>
#include "ThirdParty\XMLParser\XMLParser.hpp"
#include <map>
#include "Game/Character.hpp"
#include "Engine\Core\ParticleEffectBuilder.hpp"
#include "Game/GameCommon.hpp"


class AbilityDefinition
{
public:
	AbilityDefinition(XMLNode element);
	~AbilityDefinition();

	std::string m_name;

	int m_range;
	int m_radius;
	int m_maxHeightDifference;
	int m_areaMaxHeightDifference;

	int m_power;
	int m_speed;
	std::vector<StatusEffectType> m_statusEffects;
	std::vector<int> m_statusEffectDurations;

	CharacterAnimType m_animType;
	float m_effectDelay;

	std::string m_particleEffectName;
	SoundID m_soundEffect;

	static AbilityDefinition* GetAbilityDefinition(std::string name);
	static std::map<std::string, AbilityDefinition*> s_registry;
private:
	StatusEffectType StringToStatusEffect(std::string effectName);
};