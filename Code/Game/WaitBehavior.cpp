#include "Game/WaitBehavior.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Game/Character.hpp"
#include <vector>
#include "Engine/Math/MathUtils.hpp"
#include "Game/App.hpp"
#include "Engine/Core/XMLUtils.hpp"
#include "Engine/Core/EngineConfig.hpp"



WaitBehavior::WaitBehavior(XMLNode element)
{
}

WaitBehavior::WaitBehavior(WaitBehavior* behaviorToCopy)
{
	UNUSED(behaviorToCopy);
}

WaitBehavior::~WaitBehavior()
{

}

void WaitBehavior::Act(Character* actingCharacter)
{
	actingCharacter->Wait();
}

float WaitBehavior::CalcUtility(Character* actingCharacter, Tile* tileToActFrom /*= nullptr*/) const
{
	UNUSED(actingCharacter);
	UNUSED(tileToActFrom);
	return 0.05f;
}

std::string WaitBehavior::GetName() const
{
	return "Wait";
}

void WaitBehavior::DebugRender(const Character* actingCharacter) const 
{
	UNUSED(actingCharacter);
}

Behavior* WaitBehavior::Clone()
{
	WaitBehavior* outBehavior = new WaitBehavior(this);
	return outBehavior;
}

