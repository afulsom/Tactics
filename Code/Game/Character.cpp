#define NOMINMAX

#include "Game/Character.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Game/GameCommon.hpp"
#include "Game/App.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Game/Map.hpp"
#include "Game/Tile.hpp"
#include <algorithm>
#include "Engine/Math/Vector3.hpp"
#include "Game/AbilityDefinition.hpp"


uint8_t Character::s_currentCharacterIndex = 1;

Character::Character()
	: m_stats()
	, m_currentMap(nullptr)
	, m_currentTile(nullptr)
	, m_currentBehavior(nullptr)
	, m_behaviors()
	, m_currentHP(0)
	, m_faction()
	, m_equipment()
	, m_gCostBiases()
	, m_tags()
	, m_statusEffects()
	, m_currentlyRenderingStatusEffectIndex(0)
	, m_statusEffectRenderingTimer(0.f)
{
	m_characterIndex = s_currentCharacterIndex;
	s_currentCharacterIndex++;
}

Character::~Character()
{

}

void Character::Update(float deltaSeconds)
{
	switch (m_currentState)
	{
	case STATE_IDLE:
		UpdateIdle(deltaSeconds);
		break;
	case STATE_MOVING:
		UpdateMoving(deltaSeconds);
		break;
	case STATE_ATTACKING:
		UpdateAttacking(deltaSeconds);
		break;
	case STATE_ATTACKED:
		UpdateAttacked(deltaSeconds);
		break;
	case STATE_USING_ABILITY:
		UpdateUsingAbility(deltaSeconds);
		break;
	default:
		break;
	}

	m_statusEffectRenderingTimer += deltaSeconds;
	if (m_statusEffectRenderingTimer >= 1.f)
	{
		m_statusEffectRenderingTimer = 0.f;
		m_currentlyRenderingStatusEffectIndex++;
		if (m_currentlyRenderingStatusEffectIndex >= m_statusEffects.size())
		{
			m_currentlyRenderingStatusEffectIndex = 0;
		}
	}
}

void Character::Render() const
{
	IntVector2 tileCoords = m_currentTile->m_tileCoords;
	Matrix4 billboardMatrix = g_theApp->m_game->m_theCamera.GetInverseViewMatrixXZ();
	billboardMatrix.m_translation = Vector4(m_currentPosition, 1.f);
// 	billboardMatrix.m_translation = Vector4(tileCoords.x + 0.5f, m_currentTile->m_height, tileCoords.y + 0.5f, 1.f);
	if (m_isDead) 
	{
		billboardMatrix.m_translation.y += 0.15f;
		billboardMatrix.m_translation -= Vector4(g_theApp->m_game->m_theCamera.GetForwardXZ(), 0.f);
		billboardMatrix.RotateDegreesAboutX(90.f);
	}
	//billboardMatrix.RotateDegreesAboutY(-45.f);
	AABB2 texCoords = m_currentAnim->GetCurrentTexCoords();
	if (m_shouldFlipSprite)
		std::swap(texCoords.mins.x, texCoords.maxs.x);

	float width = 1.f;
	float halfWidth = width * 0.5f;
 	float height = width * ((float)m_currentAnim->GetSpriteLayout().x / (float)m_currentAnim->GetSpriteLayout().y);
	//float height = 2.f;

	Rgba characterTint = Rgba::WHITE;

	if (!m_statusEffects.empty())
	{
		StatusEffect* currentlyRenderedStatusEffect = m_statusEffects[m_currentlyRenderingStatusEffectIndex];
		if (currentlyRenderedStatusEffect->m_visualType == VISUAL_TINT)
		{
			characterTint = currentlyRenderedStatusEffect->m_characterTint;
		}
		else if (currentlyRenderedStatusEffect->m_visualType == VISUAL_SPRITE)
		{
			currentlyRenderedStatusEffect->DrawSpriteAtOffsetFromTransform(billboardMatrix, Vector3(0.f, height + 0.5f, 0.f), 0.25f);
		}
	}

	g_theRenderer->SetModelMatrix(billboardMatrix);
	g_theRenderer->SetTexture(0, m_currentAnim->GetTexture());
	g_theRenderer->DrawQuad3D(Vector3(-halfWidth, 0.f, 0.f), Vector3(halfWidth, height, 0.f), texCoords.mins, texCoords.maxs, characterTint);
}

void Character::UpdateIdle(float deltaSeconds)
{
	UpdateIdleAnim(deltaSeconds);
}

void Character::UpdateMoving(float deltaSeconds)
{
	UpdateMovingAnim(deltaSeconds);
	
	m_moveTimer += deltaSeconds * 2.f;
	Tile* nextTile = m_currentPath.back();
	m_currentPosition = Interpolate(m_currentTile->GetCenterDisplayPosition(), nextTile->GetCenterDisplayPosition(), m_moveTimer);

	if (m_moveTimer >= 0.5f && m_currentPosition.y != m_currentPath.back()->GetDisplayHeight())
		m_currentPosition.y = m_currentPath.back()->GetDisplayHeight();

	if (m_moveTimer >= 1.f)
	{
		m_currentMap->TryToMoveCharacterToTile(this, m_currentPath.back());
		m_currentPath.pop_back();
		if (m_currentPath.empty())
		{
			g_theApp->m_game->ReleaseWait();
			g_theApp->m_game->EndTurn(this, 0);
			m_currentState = STATE_IDLE;
			return;
		}
		
		m_moveTimer = 0.f;
		UpdateHeading(m_currentPath.back()->m_tileCoords - m_currentTile->m_tileCoords);
	}
}

void Character::UpdateAttacking(float deltaSeconds)
{
	UpdateAttackAnim(deltaSeconds);

	if (!m_hasDealtDamageThisAttack && m_currentAnim->GetSecondsElapsed() >= m_meleeTimeBeforeHit)
	{
		g_theAudio->PlaySoundAtVolume(g_theAudio->CreateOrGetSound("Data/Audio/Melee.mp3"));
		Attack(m_targettedCharacter);
		m_hasDealtDamageThisAttack = true;
	}
	
	if (m_currentAnim->IsFinished())
	{
		m_frontMeleeAnim->Reset();
		m_backMeleeAnim->Reset();
		g_theApp->m_game->ReleaseWait();
		m_currentState = STATE_IDLE;
		m_hasDealtDamageThisAttack = false;
		m_targettedCharacter = nullptr;
		g_theApp->m_game->EndTurn(this, 0);
	}
}

void Character::UpdateAttacked(float deltaSeconds)
{
	UpdateAttackedAnim(deltaSeconds);

	if (m_currentAnim->IsFinished())
	{
		m_frontHitAnim->Reset();
		m_backHitAnim->Reset();
		g_theApp->m_game->ReleaseWait();
		m_currentState = STATE_IDLE;
// 		m_currentMap->m_selectedCharacter = nullptr;
	}
}

void Character::UpdateUsingAbility(float deltaSeconds)
{
	UpdateAbilityAnim(deltaSeconds);

	float abilityTimeBeforeHit;
	switch (m_currentAbility->m_animType)
	{
	case ANIM_MELEE:
		abilityTimeBeforeHit = m_meleeTimeBeforeHit;
		break;
	case ANIM_SPELL:
		abilityTimeBeforeHit = m_spellTimeBeforeHit;
		break;
	case ANIM_RANGED:
		abilityTimeBeforeHit = m_rangedTimeBeforeHit;
		break;
	default:
		abilityTimeBeforeHit = 0.f;
		break;
	}

	if (!m_hasDealtDamageThisAttack && m_currentAnim->GetSecondsElapsed() >= abilityTimeBeforeHit)
	{
		ApplyAbilityEffectToArea();
		m_hasDealtDamageThisAttack = true;
	}

	if(!m_hasPlayedEffectThisAttack && m_currentAnim->GetSecondsElapsed() >= m_currentAbility->m_effectDelay)
	{
		AddSpriteEffectsToArea();
		m_hasPlayedEffectThisAttack = true;
	}


	if (m_currentAnim->IsFinished())
	{
		switch (m_currentAbility->m_animType)
		{
		case ANIM_MELEE:
			m_frontMeleeAnim->Reset();
			m_backMeleeAnim->Reset();
			break;
		case ANIM_SPELL:
			m_frontSpellAnim->Reset();
			m_backSpellAnim->Reset();
		case ANIM_RANGED:
			m_frontRangedAnim->Reset();
			m_backRangedAnim->Reset();
			break;
		}

		g_theApp->m_game->m_theMap->m_isWaitingForInput = false;
		m_currentState = STATE_IDLE;
		m_hasDealtDamageThisAttack = false;
		m_hasPlayedEffectThisAttack = false;
		m_currentAbility = nullptr;
		g_theApp->m_game->ReleaseWait();
		g_theApp->m_game->ReleaseWait();
		g_theApp->m_game->EndTurn(this, 0);
	}
}

void Character::TickCT()
{
	if (nullptr != m_currentAbility)
	{
		m_currentCT += m_currentAbility->m_speed;
	}
	else
	{
		m_currentCT += m_stats[STAT_SPEED];
	}
}

void Character::Act()
{
	g_theApp->m_game->WaitUntilRelease();

	float maxUtility = -1.f;
	for (size_t behaviorIndex = 0; behaviorIndex < m_behaviors.size(); behaviorIndex++)
	{
		float behaviorUtility = m_behaviors[behaviorIndex]->CalcUtility(this);
		if (behaviorUtility > maxUtility)
		{
			maxUtility = behaviorUtility;
			m_currentBehavior = m_behaviors[behaviorIndex];
		}
	}

	m_currentBehavior->Act(this);

	g_theApp->m_game->ReleaseWait();
}

void Character::Rest()
{
	
}

void Character::MoveNorth()
{
	m_currentMap->TryToMoveCharacterToTile(this, m_currentTile->GetNorthNeighbor());
}

void Character::MoveSouth()
{
	m_currentMap->TryToMoveCharacterToTile(this, m_currentTile->GetSouthNeighbor());
}

void Character::MoveEast()
{
	m_currentMap->TryToMoveCharacterToTile(this, m_currentTile->GetEastNeighbor());
}

void Character::MoveWest()
{
	m_currentMap->TryToMoveCharacterToTile(this, m_currentTile->GetWestNeighbor());
}

void Character::Attack(Character* attackedCharacter)
{
	int damageToDeal = CalculateAttackDamage(attackedCharacter);

	Tags damageTypes;
	if (m_equipment.m_equippedItems[EQUIP_SLOT_PRIMARY_WEAPON])
		damageTypes = m_equipment.m_equippedItems[EQUIP_SLOT_PRIMARY_WEAPON]->m_damageTypes;

	attackedCharacter->ApplyDamage(damageToDeal, damageTypes);
}

void Character::StartAbility()
{
	if (m_targettedCharacter)
		UpdateHeading(m_targettedCharacter->m_currentTile->m_tileCoords - m_currentTile->m_tileCoords);
	else
		UpdateHeading(m_targettedTile->m_tileCoords - m_currentTile->m_tileCoords);

	g_theApp->m_game->WaitUntilRelease();
	m_currentState = STATE_USING_ABILITY;
	m_currentCT = 0;
}

void Character::ApplyAbilityEffectToArea()
{
	Tile* centerTileToAffect;
	if (nullptr != m_targettedCharacter)
	{
		centerTileToAffect = m_targettedCharacter->m_currentTile;
	}
	else
	{
		centerTileToAffect = m_targettedTile;
	}

	std::vector<Tile*> tilesToAffect;
	tilesToAffect = m_currentMap->GetAoETiles(centerTileToAffect->m_tileCoords, m_currentAbility->m_radius, m_currentAbility->m_areaMaxHeightDifference);

	g_theAudio->PlaySoundAtVolume(m_currentAbility->m_soundEffect);

	for (Tile* tile : tilesToAffect)
	{
		if (nullptr != tile->m_occupyingCharacter)
		{
			ApplyAbilityEffectToCharacter(tile->m_occupyingCharacter);
		}
	}
}

void Character::StartMoving(Path newPath)
{
	m_currentPath = newPath;
	m_moveTimer = 0.f;
	g_theApp->m_game->WaitUntilRelease();
	m_currentState = STATE_MOVING;
	UpdateHeading(m_currentPath.back()->m_tileCoords - m_currentTile->m_tileCoords);
}

int Character::CalculateAttackDamage(Character* target)
{
	Stats modifiedAttackerStats = m_stats + m_equipment.CalculateCombinedStatModifiers();
	Stats modifiedDefenderStats = target->m_stats + target->m_equipment.CalculateCombinedStatModifiers();

	int damageToDeal = modifiedAttackerStats[STAT_ATTACK];
	if (target->HasStatusEffect(STATUS_WALL))
	{
		damageToDeal = (int)((float)damageToDeal * 0.5f);
	}

	return damageToDeal;
}

int Character::CalculateMaxNetAttackDamage(Tile* tileToAttackFrom)
{
	int maxNetDamage = 0;

	std::vector<Tile*> targettableTiles = m_currentMap->GetTargettableTiles(tileToAttackFrom->m_tileCoords, m_attackRange, m_maxAttackHeightDifference);

	for (Tile* tile : targettableTiles)
	{
		if (nullptr != tile->m_occupyingCharacter && !tile->m_occupyingCharacter->m_isDead && tile->m_occupyingCharacter != this)
		{
			int potentialDamage = CalculateAttackDamage(tile->m_occupyingCharacter);

			//Attacking allies is bad
			if (tile->m_occupyingCharacter->m_faction == m_faction)
				potentialDamage *= -1;

			if (HasStatusEffect(STATUS_CHARM))
			{
				potentialDamage *= -1;
			}

			if (HasStatusEffect(STATUS_CONFUSE))
			{
				if (g_random.GetRandomFloatZeroToOne() > 0.5f)
				{
					potentialDamage *= -1;
				}
			}

			if (potentialDamage > maxNetDamage)
				maxNetDamage = potentialDamage;
		}
	}

	return maxNetDamage;
}

Character* Character::CalculateBestAttackTarget()
{
	int maxNetDamage = 0;
	Character* bestTarget = nullptr;

	std::vector<Tile*> targettableTiles = m_currentMap->GetTargettableTiles(m_currentTile->m_tileCoords, m_attackRange, m_maxAttackHeightDifference);

	for (Tile* tile : targettableTiles)
	{
		if (nullptr != tile->m_occupyingCharacter && !tile->m_occupyingCharacter->m_isDead && tile->m_occupyingCharacter != this)
		{
			int potentialDamage = CalculateAttackDamage(tile->m_occupyingCharacter);

			//Attacking allies is bad
			if (tile->m_occupyingCharacter->m_faction == m_faction)
				potentialDamage *= -1;

			if (HasStatusEffect(STATUS_CHARM))
			{
				potentialDamage *= -1;
			}

			if (HasStatusEffect(STATUS_CONFUSE))
			{
				if (g_random.GetRandomFloatZeroToOne() > 0.5f)
				{
					potentialDamage *= -1;
				}
			}

			if (potentialDamage > maxNetDamage)
			{
				maxNetDamage = potentialDamage;
				bestTarget = tile->m_occupyingCharacter;
			}
		}
	}

	return bestTarget;
}

int Character::CalculateMaxNetAbilityDamage(AbilityDefinition* ability, Tile* tileToActFrom)
{
	int maxNetDamage = 0;

	std::vector<Tile*> targettableTiles = m_currentMap->GetTargettableTiles(tileToActFrom->m_tileCoords, m_attackRange, m_maxAttackHeightDifference);

	for (Tile* tile : targettableTiles)
	{
		int potentialDamage = CalculatePotentialAbilityDamage(ability, tile);

		if (potentialDamage > maxNetDamage)
			maxNetDamage = potentialDamage;
	}

	return maxNetDamage;
}

void Character::TargetAndSetBestAbility()
{
	int maxNetDamage = 0;
	Tile* tileTargetToSet = nullptr;
	Character* characterTargetToSet = nullptr;
	AbilityDefinition* abilityToSet = nullptr;

	for (AbilityDefinition* ability : m_abilities)
	{
		std::vector<Tile*> targettableTiles = m_currentMap->GetTargettableTiles(m_currentTile->m_tileCoords, m_attackRange, m_maxAttackHeightDifference);

		for (Tile* tile : targettableTiles)
		{
			int potentialDamage = CalculatePotentialAbilityDamage(ability, tile);

			if (potentialDamage > maxNetDamage)
			{
				maxNetDamage = potentialDamage;
				abilityToSet = ability;
				if (nullptr != tile->m_occupyingCharacter)
				{
					characterTargetToSet = tile->m_occupyingCharacter;
					tileTargetToSet = nullptr;
				}
				else
				{
					characterTargetToSet = nullptr;
					tileTargetToSet = tile;
				}
			}
		}
	}
	
	m_targettedTile = tileTargetToSet;
	m_targettedCharacter = characterTargetToSet;
	m_currentAbility = abilityToSet;
}

bool Character::HasStatusEffect(StatusEffectType type) const
{
	for (StatusEffect* effect : m_statusEffects)
	{
		if (effect->m_type == type)
		{
			return true;
		}
	}

	return false;
}

StatusEffect* Character::GetStatusEffect(StatusEffectType type)
{
	for (StatusEffect* effect : m_statusEffects)
	{
		if (effect->m_type == type)
		{
			return effect;
		}
	}

	return nullptr;
}

void Character::AddStatusEffect(StatusEffectType type, int duration)
{
	StatusEffect* foundEffect = GetStatusEffect(type);
	if (nullptr != foundEffect)
	{
		foundEffect->m_remainingDuration = std::max(foundEffect->m_remainingDuration, duration);
	}
	else
	{
		StatusEffect* newEffect = new StatusEffect(type, duration);
		m_statusEffects.push_back(newEffect);
	}

	m_currentlyRenderingStatusEffectIndex = 0;
}

void Character::RemoveStatusEffect(StatusEffectType type)
{
	m_currentlyRenderingStatusEffectIndex = 0;

	for (size_t effectIndex = 0; effectIndex < m_statusEffects.size(); effectIndex++)
	{
		if (m_statusEffects[effectIndex]->m_type == type)
		{
			m_statusEffects.erase(m_statusEffects.begin() + effectIndex);
			return;
		}
	}
}

void Character::DecrementEffectDurations()
{
	size_t numEffects = m_statusEffects.size();
	for (size_t effectIndex = 0; effectIndex < numEffects; effectIndex++)
	{
		m_statusEffects[effectIndex]->m_remainingDuration--;
		if (m_statusEffects[effectIndex]->m_remainingDuration <= 0)
		{
			std::iter_swap(m_statusEffects.begin() + effectIndex, m_statusEffects.end() - 1);
			m_statusEffects.pop_back();
			effectIndex--;
			numEffects--;

			m_currentlyRenderingStatusEffectIndex = 0;
		}
	}
}

void Character::UpdateIdleAnim(float deltaSeconds)
{
	m_frontWalkAnim->Update(deltaSeconds);
	m_backWalkAnim->Update(deltaSeconds);
	m_frontInjuredAnim->Update(deltaSeconds);
	m_backInjuredAnim->Update(deltaSeconds);
	m_frontDeadAnim->Update(deltaSeconds);
	m_backDeadAnim->Update(deltaSeconds);

	float forwardSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetForwardXZ(), Vector3((float)m_heading.x, 0.f, (float)m_heading.y));
	float leftSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetLeftXZ(), Vector3((float)m_heading.x, 0.f, (float)m_heading.y));

	if (forwardSimilarity < 0)
	{
		if (m_isDead)
			m_currentAnim = m_frontDeadAnim;
		else if ((float)m_currentHP <= ((float)m_stats[STAT_MAX_HP] * 0.3f))
			m_currentAnim = m_frontInjuredAnim;
		else
			m_currentAnim = m_frontWalkAnim;

		if (leftSimilarity < 0)
			m_shouldFlipSprite = false;
		else
			m_shouldFlipSprite = true;
	}
	else
	{
		if (m_isDead)
			m_currentAnim = m_backDeadAnim;
		else if ((float)m_currentHP <= ((float)m_stats[STAT_MAX_HP] * 0.3f))
			m_currentAnim = m_backInjuredAnim;
		else
			m_currentAnim = m_backWalkAnim;

		if (leftSimilarity < 0)
			m_shouldFlipSprite = true;
		else
			m_shouldFlipSprite = false;
	}
}

void Character::UpdateMovingAnim(float deltaSeconds)
{
	m_frontWalkAnim->Update(deltaSeconds);
	m_backWalkAnim->Update(deltaSeconds);

	float forwardSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetForwardXZ(), Vector3((float)m_heading.x, 0.f, (float)m_heading.y));
	float leftSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetLeftXZ(), Vector3((float)m_heading.x, 0.f, (float)m_heading.y));

	if (forwardSimilarity < 0)
	{
		m_currentAnim = m_frontWalkAnim;

		if (leftSimilarity < 0)
			m_shouldFlipSprite = false;
		else
			m_shouldFlipSprite = true;
	}
	else
	{
		m_currentAnim = m_backWalkAnim;

		if (leftSimilarity < 0)
			m_shouldFlipSprite = true;
		else
			m_shouldFlipSprite = false;
	}
}

void Character::UpdateHeading(IntVector2 newHeading)
{
	m_heading = newHeading;
}

void Character::UpdateAttackAnim(float deltaSeconds)
{
	m_frontMeleeAnim->Update(deltaSeconds);
	m_backMeleeAnim->Update(deltaSeconds);

	float forwardSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetForwardXZ(), Vector3((float)m_heading.x, 0.f, (float)m_heading.y));
	float leftSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetLeftXZ(), Vector3((float)m_heading.x, 0.f, (float)m_heading.y));

	if (forwardSimilarity < 0)
	{
		m_currentAnim = m_frontMeleeAnim;

		if (leftSimilarity < 0)
			m_shouldFlipSprite = false;
		else
			m_shouldFlipSprite = true;
	}
	else
	{
		m_currentAnim = m_backMeleeAnim;

		if (leftSimilarity < 0)
			m_shouldFlipSprite = true;
		else
			m_shouldFlipSprite = false;
	}
}

void Character::UpdateAttackedAnim(float deltaSeconds)
{
	m_frontHitAnim->Update(deltaSeconds);
	m_backHitAnim->Update(deltaSeconds);

	float forwardSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetForwardXZ(), Vector3((float)m_heading.x, 0.f, (float)m_heading.y));
	float leftSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetLeftXZ(), Vector3((float)m_heading.x, 0.f, (float)m_heading.y));

	if (forwardSimilarity < 0)
	{
		m_currentAnim = m_frontHitAnim;

		if (leftSimilarity < 0)
			m_shouldFlipSprite = false;
		else
			m_shouldFlipSprite = true;
	}
	else
	{
		m_currentAnim = m_backHitAnim;

		if (leftSimilarity < 0)
			m_shouldFlipSprite = true;
		else
			m_shouldFlipSprite = false;
	}
}

void Character::ApplyAbilityEffectToCharacter(Character* effectedCharacter)
{
	int power = CalculateAbilityDamageToCharacter(m_currentAbility, effectedCharacter);

	effectedCharacter->ApplyDamage(power);
	for(size_t effectIndex = 0; effectIndex < m_currentAbility->m_statusEffects.size(); effectIndex++)
	{
		effectedCharacter->AddStatusEffect(m_currentAbility->m_statusEffects[effectIndex], m_currentAbility->m_statusEffectDurations[effectIndex]);
	}
}

void Character::AddSpriteEffectsToArea()
{
	Tile* centerTileToAffect;
	if (nullptr != m_targettedCharacter)
	{
		centerTileToAffect = m_targettedCharacter->m_currentTile;
	}
	else
	{
		centerTileToAffect = m_targettedTile;
	}

	std::vector<Tile*> tilesToAffect;
	tilesToAffect = m_currentMap->GetAoETiles(centerTileToAffect->m_tileCoords, m_currentAbility->m_radius, m_currentAbility->m_areaMaxHeightDifference);
	for (Tile* tile : tilesToAffect)
	{
		if (nullptr != tile->m_occupyingCharacter)
		{
			//g_theApp->m_game->WaitUntilRelease();
			if(!m_currentAbility->m_particleEffectName.empty())
			{
				m_currentMap->m_particleSystem.PlayOneOffEffectAtLocation(m_currentAbility->m_particleEffectName, Vector3((float)tile->m_tileCoords.x + 0.5f, tile->GetDisplayHeight(), (float)tile->m_tileCoords.y + 0.5f));
			}
		}
	}

}

int Character::CalculatePotentialAbilityDamage(AbilityDefinition* ability, Tile* targettedTile)
{
	std::vector<Tile*> tilesInRadius = targettedTile->m_containingMap->GetAoETiles(targettedTile->m_tileCoords, ability->m_radius, ability->m_areaMaxHeightDifference);
	std::vector<Character*> charactersInRadius;

	for (Tile* tile : tilesInRadius)
	{
		if (nullptr != tile->m_occupyingCharacter && !tile->m_occupyingCharacter->m_isDead)
		{
			charactersInRadius.push_back(tile->m_occupyingCharacter);
		}
	}

	int netDamage = 0;
	for (Character* character : charactersInRadius)
	{
		int abilityDamage = CalculateAbilityDamageToCharacter(ability, character);
		if (character->m_currentHP - abilityDamage > character->m_stats[STAT_MAX_HP])
			abilityDamage = character->m_currentHP - character->m_stats[STAT_MAX_HP];

		if (character->m_faction == m_faction)
		{
			abilityDamage *= -1;
		}

		if (HasStatusEffect(STATUS_CHARM))
		{
			abilityDamage *= -1;
		}

		if (HasStatusEffect(STATUS_CONFUSE))
		{
			if (g_random.GetRandomFloatZeroToOne() > 0.5f)
			{
				abilityDamage *= -1;
			}
		}

		netDamage += abilityDamage;
	}

	return netDamage;
}

int Character::CalculateAbilityDamageToCharacter(AbilityDefinition* ability, Character* targettedCharacter)
{
	float truePowerMultiplier = RangeMapFloat((float)m_stats[STAT_FAITH], 0.f, 100.f, 0.25f, 2.f);
	float truePower = truePowerMultiplier * ability->m_power;
	
	if (targettedCharacter->HasStatusEffect(STATUS_WALL))
	{
		truePower *= 0.5f;
	}

	return (int)truePower;
}

void Character::Wait()
{
	g_theApp->m_game->WaitCharacter(this);
}

void Character::UpdateAbilityAnim(float deltaSeconds)
{
	SpriteAnimation2D* frontAbilityAnim;
	SpriteAnimation2D* backAbilityAnim;

	switch (m_currentAbility->m_animType)
	{
	case ANIM_MELEE:
		frontAbilityAnim = m_frontMeleeAnim;
		backAbilityAnim = m_backMeleeAnim;
		break;
	case ANIM_SPELL:
		frontAbilityAnim = m_frontSpellAnim;
		backAbilityAnim = m_backSpellAnim;
		break;
	case ANIM_RANGED:
		frontAbilityAnim = m_frontRangedAnim;
		backAbilityAnim = m_backRangedAnim;
	default:
		frontAbilityAnim = m_frontSpellAnim;
		backAbilityAnim = m_backSpellAnim;
		break;
	}
	frontAbilityAnim->Update(deltaSeconds);
	backAbilityAnim->Update(deltaSeconds);

	float forwardSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetForwardXZ(), Vector3((float)m_heading.x, 0.f, (float)m_heading.y));
	float leftSimilarity = DotProduct(g_theApp->m_game->m_theCamera.GetLeftXZ(), Vector3((float)m_heading.x, 0.f, (float)m_heading.y));

	if (forwardSimilarity < 0)
	{
		m_currentAnim = frontAbilityAnim;

		if (leftSimilarity < 0)
			m_shouldFlipSprite = false;
		else
			m_shouldFlipSprite = true;
	}
	else
	{
		m_currentAnim = backAbilityAnim;

		if (leftSimilarity < 0)
			m_shouldFlipSprite = true;
		else
			m_shouldFlipSprite = false;
	}
}

float Character::GetGCostBias(std::string tileType) const
{
	std::map<std::string, float>::const_iterator found = m_gCostBiases.find(tileType);
	if (found == m_gCostBiases.end())
		return 0.f;

	return found->second;
}

void Character::ApplyDamage(int damageToDeal, const Tags& damageTypes)
{
	float damageModifier = 1.f;
	for (std::string weaknessTag : m_damageTypeWeaknesses)
	{
		if (damageTypes.MatchTags(weaknessTag))
			damageModifier *= 2.f;
	}

	for (std::string resistanceTag : m_damageTypeResistances)
	{
		if (damageTypes.MatchTags(resistanceTag))
			damageModifier *= 0.5f;
	}

	for (std::string immunityTag : m_damageTypeImmunities)
	{
		if (damageTypes.MatchTags(immunityTag))
		{
			damageModifier *= 0.f;
			break;
		}
	}

	damageToDeal = (int)floor((float)damageToDeal * damageModifier);

	if (damageToDeal > 0 && m_currentState == STATE_IDLE)
	{
		g_theApp->m_game->WaitUntilRelease();
		m_currentState = STATE_ATTACKED;
	}

	m_currentHP -= damageToDeal;
	if (m_currentHP > m_stats[STAT_MAX_HP])
		m_currentHP = m_stats[STAT_MAX_HP];

	if (damageToDeal != 0)
	{
		Rgba numberColor;
		if (damageToDeal > 0)
		{
			numberColor = Rgba::RED;
		}
		else
		{
			numberColor = Rgba::GREEN;
			damageToDeal *= -1;
		}

		m_currentMap->m_damageNumbers.push_back(DamageNumber(std::to_string(damageToDeal), Vector3(m_currentPosition.x, m_currentPosition.y + 2.f, m_currentPosition.z), numberColor, ClampFloat(damageModifier * 0.5f, 0.4f, 1.25f)));
	}

	if (m_currentHP <= 0)
	{
		m_currentHP = 0;
		m_isDead = true;
	}
}

void Character::ApplyDamage(int damageToDeal, bool shouldPlayHitAnim /*= true*/)
{
	damageToDeal = (int)floor((float)damageToDeal);

	if (shouldPlayHitAnim && damageToDeal > 0 && m_currentState == STATE_IDLE)
	{
		m_currentState = STATE_ATTACKED;
		g_theApp->m_game->WaitUntilRelease();
	}

	m_currentHP -= damageToDeal;
	if (m_currentHP > m_stats[STAT_MAX_HP])
		m_currentHP = m_stats[STAT_MAX_HP];

	if (damageToDeal != 0)
	{
		Rgba numberColor;
		if (damageToDeal > 0)
		{
			numberColor = Rgba::RED;
		}
		else
		{
			numberColor = Rgba::GREEN;
			damageToDeal *= -1;
		}

		m_currentMap->m_damageNumbers.push_back(DamageNumber(std::to_string(damageToDeal), Vector3(m_currentPosition.x, m_currentPosition.y + 2.f, m_currentPosition.z), numberColor, 0.75f));
	}
	if (m_currentHP <= 0)
	{
		m_currentHP = 0;
		m_isDead = true;
	}
}

void Character::StartAttack(Character* characterToAttack)
{
	UpdateHeading(characterToAttack->m_currentTile->m_tileCoords - m_currentTile->m_tileCoords);
	g_theApp->m_game->WaitUntilRelease();
	m_currentState = STATE_ATTACKING;
	m_targettedCharacter = characterToAttack;
}
