#pragma once
#include "Game/Behavior.hpp"
#include "Game/Map.hpp"



class AbilityBehavior : public Behavior
{
public:
	AbilityBehavior(XMLNode element);
	virtual ~AbilityBehavior();
	AbilityBehavior(AbilityBehavior* behaviorToCopy);

	virtual void Act(Character* actingCharacter) override;
	virtual float CalcUtility(Character* actingCharacter, Tile* tileToActFrom = nullptr) const override;
	virtual std::string GetName() const override;
	virtual void DebugRender(const Character* actingCharacter) const override;

	virtual Behavior* Clone() override;
};