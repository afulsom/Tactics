#include "Game/CharacterBuilder.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Engine/Renderer/RHI/SimpleRenderer.hpp"

std::map<std::string, CharacterBuilder*> CharacterBuilder::s_registry;


CharacterBuilder::CharacterBuilder(XMLNode element)
	: m_damageTypeWeaknesses()
	, m_damageTypeResistances()
	, m_damageTypeImmunities()
{
	m_name = ParseXMLAttributeString(element, "name", "ERROR_INVALID_NAME");
	ASSERT_OR_DIE(m_name != "ERROR_INVALID_NAME", "No name found for Character element.");

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

	Texture2D* spriteSheet = g_theRenderer->CreateTexture2DFromFile(ParseXMLAttributeString(element, "spriteSheet", "INVALID_SHEET"));
	m_spriteSheet = new SpriteSheet2D(spriteSheet, 7, 4);
	m_frontWalkAnimIndices = ParseXMLAttributeIntVector2(element, "frontWalkAnim", IntVector2(0, 0));
	m_backWalkAnimIndices = ParseXMLAttributeIntVector2(element, "backWalkAnim", IntVector2(0, 0));
	m_frontInjuredAnimIndices = ParseXMLAttributeIntVector2(element, "frontInjuredAnim", IntVector2(0, 0));
	m_backInjuredAnimIndices = ParseXMLAttributeIntVector2(element, "backInjuredAnim", IntVector2(0, 0));
	m_frontDeadAnimIndices = ParseXMLAttributeIntVector2(element, "frontDeadAnim", IntVector2(0, 0));
	m_backDeadAnimIndices = ParseXMLAttributeIntVector2(element, "backDeadAnim", IntVector2(0, 0));
	m_frontMeleeAnimIndices = ParseXMLAttributeIntVector2(element, "frontMeleeAnim", IntVector2(0, 0));
	m_backMeleeAnimIndices = ParseXMLAttributeIntVector2(element, "backMeleeAnim", IntVector2(0, 0));
	m_frontSpellAnimIndices = ParseXMLAttributeIntVector2(element, "frontSpellAnim", IntVector2(0, 0));
	m_backSpellAnimIndices = ParseXMLAttributeIntVector2(element, "backSpellAnim", IntVector2(0, 0));
	m_frontRangedAnimIndices = ParseXMLAttributeIntVector2(element, "frontRangedAnim", IntVector2(0, 0));
	m_backRangedAnimIndices = ParseXMLAttributeIntVector2(element, "backRangedAnim", IntVector2(0, 0));
	m_frontHitAnimIndices = ParseXMLAttributeIntVector2(element, "frontHitAnim", IntVector2(0, 0));
	m_backHitAnimIndices = ParseXMLAttributeIntVector2(element, "backHitAnim", IntVector2(0, 0));
	m_portrait = g_theRenderer->CreateTexture2DFromFile(ParseXMLAttributeString(element, "portrait", "INVALID_PORTRAIT"));

	m_meleeAnimDuration = ParseXMLAttributeFloat(element, "meleeAnimDuration", 1.f);
	m_meleeTimeBeforeHit = ParseXMLAttributeFloat(element, "meleeTimeBeforeHit", 1.f);
	m_spellAnimDuration = ParseXMLAttributeFloat(element, "spellAnimDuration", 1.f);
	m_spellTimeBeforeHit = ParseXMLAttributeFloat(element, "spellTimeBeforeHit", 1.f);
	m_rangedAnimDuration = ParseXMLAttributeFloat(element, "rangedAnimDuration", 1.f);
	m_rangedTimeBeforeHit = ParseXMLAttributeFloat(element, "rangedTimeBeforeHit", 1.f);
	m_hitAnimDuration = ParseXMLAttributeFloat(element, "hitAnimDuration", 1.f);

	m_attackRange = ParseXMLAttributeInt(element, "attackRange", 1);
	m_maxAttackHeightDifference = ParseXMLAttributeInt(element, "maxAttackHeightDifference", 2);

	XMLNode abilityRoot = element.getChildNode("Abilities");
	for (int abilityIndex = 0; abilityIndex < abilityRoot.nChildNode(); abilityIndex++)
	{
		m_abilities.push_back(AbilityDefinition::GetAbilityDefinition(abilityRoot.getChildNode(abilityIndex).getName()));
	}

	XMLNode behaviorRoot = element.getChildNode("Behaviors");
	for (int behaviorIndex = 0; behaviorIndex < behaviorRoot.nChildNode(); behaviorIndex++)
	{
		m_behaviors.push_back(Behavior::Create(behaviorRoot.getChildNode(behaviorIndex)));
	}

	m_faction = ParseXMLAttributeString(element, "faction", "ERROR_INVALID_FACTION");
	ASSERT_OR_DIE(m_faction != "ERROR_INVALID_FACTION", "No faction found for Character element.");

	if (element.nChildNode("Tags") == 1)
	{
		XMLNode tagsNode = element.getChildNode("Tags");
		m_tagsToSet = ParseXMLAttributeString(tagsNode, "tags", "");
	}
	ASSERT_OR_DIE(element.nChildNode("Tags") <= 1, "Too many tags elements in character.");

	//Weaknesses
	if (element.nChildNode("DamageTypeWeaknesses") == 1)
	{
		XMLNode weaknessNode = element.getChildNode("DamageTypeWeaknesses");
		m_damageTypeWeaknesses = Split(ParseXMLAttributeString(weaknessNode, "tags", ""), ',');
	}
	ASSERT_OR_DIE(element.nChildNode("DamageTypeWeaknesses") <= 1, "Too many weakness elements in character.");

	//Resistances
	if (element.nChildNode("DamageTypeResistances") == 1)
	{
		XMLNode resistanceNode = element.getChildNode("DamageTypeResistances");
		m_damageTypeResistances = Split(ParseXMLAttributeString(resistanceNode, "tags", ""), ',');
	}
	ASSERT_OR_DIE(element.nChildNode("DamageTypeResistances") <= 1, "Too many resistance elements in character.");

	//Immunities
	if (element.nChildNode("DamageTypeImmunities") == 1)
	{
		XMLNode immunityNode = element.getChildNode("DamageTypeImmunities");
		m_damageTypeImmunities = Split(ParseXMLAttributeString(immunityNode, "tags", ""), ',');
	}
	ASSERT_OR_DIE(element.nChildNode("DamageTypeImmunities") <= 1, "Too many immunity elements in character.");

	s_registry[m_name] = this;
}

CharacterBuilder::~CharacterBuilder()
{
	delete m_portrait;
	delete m_spriteSheet;
}

Character* CharacterBuilder::BuildNewCharacter(std::string characterTypeName)
{
	std::map<std::string, CharacterBuilder*>::iterator found = s_registry.find(characterTypeName);
	if (found == s_registry.end())
		ERROR_AND_DIE("Attempted to build unknown character type.");

	CharacterBuilder* foundBuilder = found->second;

	Character* newCharacter = new Character();
	newCharacter->m_name = foundBuilder->m_name;

	newCharacter->m_faction = foundBuilder->m_faction;
	newCharacter->m_stats = Stats::CalculateRandomStatsInRange(foundBuilder->m_minStats, foundBuilder->m_maxStats);
	newCharacter->m_behaviors = CloneBehaviors(foundBuilder->m_behaviors);
	newCharacter->m_currentHP = newCharacter->m_stats[STAT_MAX_HP];
	newCharacter->m_gCostBiases = foundBuilder->m_gCostBiases;
	newCharacter->m_tags.SetTags(foundBuilder->m_tagsToSet);
	newCharacter->m_damageTypeWeaknesses = foundBuilder->m_damageTypeWeaknesses;
	newCharacter->m_damageTypeResistances = foundBuilder->m_damageTypeResistances;
	newCharacter->m_damageTypeImmunities = foundBuilder->m_damageTypeImmunities;

	newCharacter->m_attackRange = foundBuilder->m_attackRange;
	newCharacter->m_maxAttackHeightDifference = foundBuilder->m_maxAttackHeightDifference;
	
	newCharacter->m_frontWalkAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, 1.f, SPRITE_ANIM_MODE_LOOPING, foundBuilder->m_frontWalkAnimIndices.x, foundBuilder->m_frontWalkAnimIndices.y);
	newCharacter->m_backWalkAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, 1.f, SPRITE_ANIM_MODE_LOOPING, foundBuilder->m_backWalkAnimIndices.x, foundBuilder->m_backWalkAnimIndices.y);
	
	newCharacter->m_frontInjuredAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, 1.f, SPRITE_ANIM_MODE_LOOPING, foundBuilder->m_frontInjuredAnimIndices.x, foundBuilder->m_frontInjuredAnimIndices.y);
	newCharacter->m_backInjuredAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, 1.f, SPRITE_ANIM_MODE_LOOPING, foundBuilder->m_backInjuredAnimIndices.x, foundBuilder->m_backInjuredAnimIndices.y);
	
	newCharacter->m_frontDeadAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, 1.f, SPRITE_ANIM_MODE_LOOPING, foundBuilder->m_frontDeadAnimIndices.x, foundBuilder->m_frontDeadAnimIndices.y);
	newCharacter->m_backDeadAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, 1.f, SPRITE_ANIM_MODE_LOOPING, foundBuilder->m_backDeadAnimIndices.x, foundBuilder->m_backDeadAnimIndices.y);
	
	newCharacter->m_frontMeleeAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, foundBuilder->m_meleeAnimDuration, SPRITE_ANIM_MODE_PLAY_TO_END, foundBuilder->m_frontMeleeAnimIndices.x, foundBuilder->m_frontMeleeAnimIndices.y);
	newCharacter->m_backMeleeAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, foundBuilder->m_meleeAnimDuration, SPRITE_ANIM_MODE_PLAY_TO_END, foundBuilder->m_backMeleeAnimIndices.x, foundBuilder->m_backMeleeAnimIndices.y);
	
	newCharacter->m_frontSpellAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, foundBuilder->m_spellAnimDuration, SPRITE_ANIM_MODE_PLAY_TO_END, foundBuilder->m_frontSpellAnimIndices.x, foundBuilder->m_frontSpellAnimIndices.y);
	newCharacter->m_backSpellAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, foundBuilder->m_spellAnimDuration, SPRITE_ANIM_MODE_PLAY_TO_END, foundBuilder->m_backSpellAnimIndices.x, foundBuilder->m_backSpellAnimIndices.y);
	
	newCharacter->m_frontRangedAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, foundBuilder->m_rangedAnimDuration, SPRITE_ANIM_MODE_PLAY_TO_END, foundBuilder->m_frontRangedAnimIndices.x, foundBuilder->m_frontRangedAnimIndices.y);
	newCharacter->m_backRangedAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, foundBuilder->m_rangedAnimDuration, SPRITE_ANIM_MODE_PLAY_TO_END, foundBuilder->m_backRangedAnimIndices.x, foundBuilder->m_backRangedAnimIndices.y);

	newCharacter->m_frontHitAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, foundBuilder->m_hitAnimDuration, SPRITE_ANIM_MODE_PLAY_TO_END, foundBuilder->m_frontHitAnimIndices.x, foundBuilder->m_frontHitAnimIndices.y);
	newCharacter->m_backHitAnim = new SpriteAnimation2D(*foundBuilder->m_spriteSheet, foundBuilder->m_hitAnimDuration, SPRITE_ANIM_MODE_PLAY_TO_END, foundBuilder->m_backHitAnimIndices.x, foundBuilder->m_backHitAnimIndices.y);
	
	newCharacter->m_portrait = foundBuilder->m_portrait;

	newCharacter->m_meleeTimeBeforeHit = foundBuilder->m_meleeTimeBeforeHit;
	newCharacter->m_spellTimeBeforeHit = foundBuilder->m_spellTimeBeforeHit;
	newCharacter->m_rangedTimeBeforeHit = foundBuilder->m_rangedTimeBeforeHit;

	newCharacter->m_abilities = foundBuilder->m_abilities;

	return newCharacter;
}

std::vector<Behavior*> CharacterBuilder::CloneBehaviors(std::vector<Behavior*> behaviorsToClone)
{
	std::vector<Behavior*> outputBehaviors;
	for (size_t behaviorIndex = 0; behaviorIndex < behaviorsToClone.size(); behaviorIndex++)
	{
		outputBehaviors.push_back(behaviorsToClone[behaviorIndex]->Clone());
	}

	return outputBehaviors;
}