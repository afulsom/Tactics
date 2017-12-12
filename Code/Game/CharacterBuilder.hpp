#pragma once
#include <map>
#include <string>
#include "Game/Character.hpp"
#include "ThirdParty/XMLParser/XMLParser.hpp"
#include "Engine/Renderer/RHI/SpriteSheet2D.hpp"
#include "Game/AbilityDefinition.hpp"


class CharacterBuilder
{
public:
	CharacterBuilder(XMLNode element);
	~CharacterBuilder();
	static Character* BuildNewCharacter(std::string characterTypeName);

public:
	std::string m_name;
	Stats m_minStats;
	Stats m_maxStats;
	std::string m_faction;

	int m_attackRange;
	int m_maxAttackHeightDifference;

	SpriteSheet2D* m_spriteSheet;
	IntVector2 m_frontWalkAnimIndices;
	IntVector2 m_backWalkAnimIndices;
	IntVector2 m_frontInjuredAnimIndices;
	IntVector2 m_backInjuredAnimIndices;
	IntVector2 m_frontDeadAnimIndices;
	IntVector2 m_backDeadAnimIndices;
	IntVector2 m_frontMeleeAnimIndices;
	IntVector2 m_backMeleeAnimIndices;
	IntVector2 m_frontSpellAnimIndices;
	IntVector2 m_backSpellAnimIndices;
	IntVector2 m_frontRangedAnimIndices;
	IntVector2 m_backRangedAnimIndices;
	IntVector2 m_frontHitAnimIndices;
	IntVector2 m_backHitAnimIndices;
	Texture2D* m_portrait;

	float m_meleeAnimDuration;
	float m_meleeTimeBeforeHit;
	float m_spellAnimDuration;
	float m_spellTimeBeforeHit;
	float m_rangedAnimDuration;
	float m_rangedTimeBeforeHit;
	float m_hitAnimDuration;

	char m_glyph;
	Rgba m_glyphColor;
	Rgba m_fillColor;

	std::vector<AbilityDefinition*> m_abilities;

	std::vector<Behavior*> m_behaviors;
	std::vector<std::string> m_loot;
	std::map<std::string, float> m_gCostBiases;
	std::string m_tagsToSet;
	std::vector<std::string> m_damageTypeWeaknesses;
	std::vector<std::string> m_damageTypeResistances;
	std::vector<std::string> m_damageTypeImmunities;

	static std::map<std::string, CharacterBuilder*> s_registry;
private:
	static std::vector<Behavior*> CloneBehaviors(std::vector<Behavior*> behaviorsToClone);
};