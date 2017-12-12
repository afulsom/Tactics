#include "Game/Behavior.hpp"
#include <string>
#include "Game/CloseToAttackBehavior.hpp"
#include "Game/FleeBehavior.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineConfig.hpp"
#include "Game/AttackBehavior.hpp"
#include "Game/Character.hpp"
#include "Game/WaitBehavior.hpp"
#include "Game/AbilityBehavior.hpp"

Behavior::Behavior()
{

}

Behavior::~Behavior()
{

}

float Behavior::CalcUtility(Character* actingCharacter, Tile* tileToActFrom /*= nullptr*/) const
{
	UNUSED(actingCharacter);
	UNUSED(tileToActFrom);
	return 0.f;
}

Behavior* Behavior::Create(XMLNode element)
{
	std::string elementName = element.getName();

	if (elementName == "CloseToAttack")
		return new CloseToAttackBehavior(element);

	if (elementName == "Attack")
		return new AttackBehavior(element);

	if (elementName == "Wait")
		return new WaitBehavior(element);

	if (elementName == "Ability")
		return new AbilityBehavior(element);

	ERROR_AND_DIE("Invalid behavior name.");
}
