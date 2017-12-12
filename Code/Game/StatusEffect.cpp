#include "Game/StatusEffect.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Engine/Renderer/RHI/SimpleRenderer.hpp"

SpriteSheet2D* StatusEffect::s_statusEffectSpriteSheet = nullptr;

StatusEffect::StatusEffect(StatusEffectType typeToCreate, int duration)
{
	if (s_statusEffectSpriteSheet == nullptr)
	{
		Texture2D* spriteSheetTexture = g_theRenderer->CreateTexture2DFromFile("Data/Images/StatusEffectSheet.png");
		s_statusEffectSpriteSheet = new SpriteSheet2D(spriteSheetTexture, 5, 5);
	}

	m_type = typeToCreate;
	m_remainingDuration = duration;

	switch (typeToCreate)
	{
	case STATUS_WALL:
		m_visualType = VISUAL_SPRITE;
		m_spriteCoords = IntVector2(0, 2);
		break;
	case STATUS_CHARM:
		m_visualType = VISUAL_TINT;
		m_characterTint = Rgba(255, 182, 193, 255);
		break;
	case STATUS_CONFUSE:
		m_visualType = VISUAL_SPRITE;
		m_spriteCoords = IntVector2(3, 3);
		break;
	case STATUS_POISON:
		m_visualType = VISUAL_TINT;
		m_characterTint = Rgba(107, 142, 35, 255);
		break;
	default:
		ERROR_AND_DIE("Attempted to create invalid status effect.");
		break;
	}
}

StatusEffect::~StatusEffect()
{

}

void StatusEffect::DrawSpriteAtOffsetFromTransform(const Matrix4& transform, const Vector3& offset, float radius) const
{
	g_theRenderer->SetTexture(s_statusEffectSpriteSheet->GetTexture());
	g_theRenderer->SetModelMatrix(transform);

	Vector3 halfExtents(radius, radius, 0.f);
	AABB2 uvs = s_statusEffectSpriteSheet->GetTexCoordsForSpriteCoords(m_spriteCoords.x, m_spriteCoords.y);
	g_theRenderer->DrawQuad3D(offset - halfExtents, offset + halfExtents, uvs.mins, uvs.maxs);
}
