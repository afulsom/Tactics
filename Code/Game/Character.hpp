#pragma once
#include "Game/Stats.hpp"
#include "Game/Behavior.hpp"
#include <set>
#include "Engine/Gameplay/Tags.hpp"
#include "Game/Inventory.hpp"
#include "Engine/Renderer/RHI/SpriteAnimation2D.hpp"
#include "StatusEffect.hpp"

class Map;
class Tile;
class AbilityDefinition;
typedef std::vector<Tile*> Path;

struct Equipment
{
	Item* m_equippedItems[NUM_EQUIP_SLOTS];

	Equipment() 
	{
		for (int itemIndex = 0; itemIndex < NUM_EQUIP_SLOTS; itemIndex++)
		{
			m_equippedItems[itemIndex] = nullptr;
		}
	}

	Stats CalculateCombinedStatModifiers() const
	{
		Stats combinedStatModifiers;
		for (Item* equippedItem : m_equippedItems)
		{
			if(equippedItem)
				combinedStatModifiers += equippedItem->m_stats;
		}

		return combinedStatModifiers;
	}

	bool IsItemEquipped(const Item* itemToCheck) const
	{
		if (itemToCheck == nullptr)
			return false;

		for (Item* equippedItem : m_equippedItems)
		{
			if (equippedItem == itemToCheck)
				return true;
		}

		return false;
	}

};

enum CharacterState
{
	STATE_IDLE,
	STATE_MOVING,
	STATE_ATTACKING,
	STATE_ATTACKED,
	STATE_USING_ABILITY
};

enum CharacterAnimType
{
	ANIM_MELEE,
	ANIM_SPELL,
	ANIM_RANGED
};

enum CharacterController
{
	CONTROLLER_AI,
	CONTROLLER_PLAYER
};

class Character
{
public:
	Character();
	~Character();

	void Update(float deltaSeconds);
	void Render() const;

	void UpdateIdle(float deltaSeconds);
	void UpdateMoving(float deltaSeconds);
	void UpdateAttacking(float deltaSeconds);
	void UpdateAttacked(float deltaSeconds);
	void UpdateUsingAbility(float deltaSeconds);

	void TickCT();
	void Act();

	float GetGCostBias(std::string tileType) const;

	void Wait();
	void Rest();
	void MoveNorth();
	void MoveSouth();
	void MoveEast();
	void MoveWest();

	void ApplyDamage(int damageToDeal, const Tags& damageTypesString);
	void ApplyDamage(int damageToDeal, bool shouldPlayHitAnim = true);
	void StartAttack(Character* characterToAttack);
	void Attack(Character* attackedCharacter);

	void StartAbility();
	void ApplyAbilityEffectToArea();

	void StartMoving(Path newPath);

	int CalculateAttackDamage(Character* target);
	int CalculateMaxNetAttackDamage(Tile* tileToAttackFrom);
	Character* CalculateBestAttackTarget();

	int CalculateMaxNetAbilityDamage(AbilityDefinition* ability, Tile* tileToActFrom);
	void TargetAndSetBestAbility();

	bool HasStatusEffect(StatusEffectType type) const;
	StatusEffect* GetStatusEffect(StatusEffectType type);
	void AddStatusEffect(StatusEffectType type, int duration);
	void RemoveStatusEffect(StatusEffectType type);


	void DecrementEffectDurations();
public:
	static uint8_t s_currentCharacterIndex;

	CharacterController m_controller;
	uint8_t m_owningPlayer;
	uint8_t m_characterIndex;

	std::string m_name;
	Map* m_currentMap;
	Tile* m_currentTile;
	Vector3 m_currentPosition;
	IntVector2 m_heading = IntVector2(1, 0);

	CharacterState m_currentState = STATE_IDLE;

	Path m_currentPath;
	float m_moveTimer = 0.f;
	float m_meleeTimeBeforeHit;
	float m_spellTimeBeforeHit;
	float m_rangedTimeBeforeHit;
	bool m_hasDealtDamageThisAttack = false;
	bool m_hasPlayedEffectThisAttack = false;

	Equipment m_equipment;

	int m_currentCT = 0;
	int m_currentHP;
	Stats m_stats;

	std::vector<StatusEffect*> m_statusEffects;
	size_t m_currentlyRenderingStatusEffectIndex;
	float m_statusEffectRenderingTimer;

	int m_attackRange;
	int m_maxAttackHeightDifference;

	bool m_isDead = false;

	std::string m_faction;
	std::vector<Behavior*> m_behaviors;
	Behavior* m_currentBehavior;

	std::vector<AbilityDefinition*> m_abilities;
	AbilityDefinition* m_currentAbility = nullptr;

	std::map<std::string, float> m_gCostBiases;
	Tags m_tags;
	std::vector<std::string> m_damageTypeWeaknesses;
	std::vector<std::string> m_damageTypeResistances;
	std::vector<std::string> m_damageTypeImmunities;

	Character* m_targettedCharacter = nullptr;
	Tile* m_targettedTile = nullptr;

	SpriteAnimation2D* m_currentAnim;
	bool m_shouldFlipSprite;

	SpriteAnimation2D* m_frontWalkAnim;
	SpriteAnimation2D* m_backWalkAnim;

	SpriteAnimation2D* m_frontInjuredAnim;
	SpriteAnimation2D* m_backInjuredAnim;

	SpriteAnimation2D* m_frontDeadAnim;
	SpriteAnimation2D* m_backDeadAnim;

	SpriteAnimation2D* m_frontMeleeAnim;
	SpriteAnimation2D* m_backMeleeAnim;

	SpriteAnimation2D* m_frontSpellAnim;
	SpriteAnimation2D* m_backSpellAnim;

	SpriteAnimation2D* m_frontRangedAnim;
	SpriteAnimation2D* m_backRangedAnim;

	SpriteAnimation2D* m_frontHitAnim;
	SpriteAnimation2D* m_backHitAnim;

	Texture2D* m_portrait;
private:
	void UpdateIdleAnim(float deltaSeconds);
	void UpdateMovingAnim(float deltaSeconds);
	void UpdateHeading(IntVector2 newHeading);
	void UpdateAttackAnim(float deltaSeconds);
	void UpdateAttackedAnim(float deltaSeconds);
	void UpdateAbilityAnim(float deltaSeconds);

	void ApplyAbilityEffectToCharacter(Character* effectedCharacter);
	void AddSpriteEffectsToArea();

	int CalculatePotentialAbilityDamage(AbilityDefinition* ability, Tile* targettedTile);
	int CalculateAbilityDamageToCharacter(AbilityDefinition* ability, Character* targettedCharacter);
};
