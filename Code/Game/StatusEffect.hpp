#pragma once
#include "Engine\Math\IntVector2.hpp"
#include "Engine\Core\Rgba.hpp"
#include "Engine\Math\Matrix4.hpp"
#include "Engine\Renderer\RHI\SpriteSheet2D.hpp"


//Forward Declares
class Character;


//Enums
enum StatusEffectType
{
	STATUS_WALL = 0,
	STATUS_CHARM,
	STATUS_CONFUSE,
	STATUS_POISON,
	NUM_STATUS_EFFECTS
};

enum StatusEffectVisualType
{
	VISUAL_SPRITE,
	VISUAL_TINT
};



//Class
class StatusEffect
{
public:
	StatusEffect(StatusEffectType typeToCreate, int duration);
	~StatusEffect();

	void DrawSpriteAtOffsetFromTransform(const Matrix4& transform, const Vector3& offset, float radius) const;

public:
	StatusEffectType m_type;
	int m_remainingDuration;

	StatusEffectVisualType m_visualType;
	IntVector2 m_spriteCoords;
	Rgba m_characterTint;

	static SpriteSheet2D* s_statusEffectSpriteSheet;
};