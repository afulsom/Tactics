#pragma once
#include "Game/Behavior.hpp"
#include "Game/Map.hpp"



class CloseToAttackBehavior : public Behavior
{
public:
	CloseToAttackBehavior(XMLNode element);
	virtual ~CloseToAttackBehavior();
	CloseToAttackBehavior(CloseToAttackBehavior* behaviorToCopy);

	virtual void Act(Character* actingCharacter) override;
	virtual float CalcUtility(Character* actingCharacter, Tile* tileToActFrom = nullptr) const override;
	virtual std::string GetName() const override;
	virtual void DebugRender(const Character* actingCharacter) const override;

	Tile* CalculateBestTileToMoveTo(float& outUtility, Character* actingCharacter, Tile* tileToStartFrom) const;

	virtual Behavior* Clone() override;

	float m_utility = 0.5f;
	Path m_path;
};