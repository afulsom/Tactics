#pragma once
#include "Game/Behavior.hpp"
#include "Game/Map.hpp"



class AttackBehavior : public Behavior
{
public:
	AttackBehavior(XMLNode element);
	virtual ~AttackBehavior();
	AttackBehavior(AttackBehavior* behaviorToCopy);

	virtual void Act(Character* actingCharacter) override;
	virtual float CalcUtility(Character* actingCharacter, Tile* tileToActFrom = nullptr) const override;
	virtual std::string GetName() const override;
	virtual void DebugRender(const Character* actingCharacter) const override;

	virtual Behavior* Clone() override;

	float m_utility = 0.7f;
};