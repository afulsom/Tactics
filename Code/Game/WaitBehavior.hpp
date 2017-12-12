#pragma once
#include "Game/Behavior.hpp"
#include "Game/Map.hpp"



class WaitBehavior : public Behavior
{
public:
	WaitBehavior(XMLNode element);
	virtual ~WaitBehavior();
	WaitBehavior(WaitBehavior* behaviorToCopy);

	virtual void Act(Character* actingCharacter) override;
	virtual float CalcUtility(Character* actingCharacter, Tile* tileToActFrom = nullptr) const override;
	virtual std::string GetName() const override;
	virtual void DebugRender(const Character* actingCharacter) const override;

	virtual Behavior* Clone() override;
};