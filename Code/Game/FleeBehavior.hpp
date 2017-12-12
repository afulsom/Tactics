#pragma once
#include "Game/Behavior.hpp"
#include "Game/Map.hpp"



class FleeBehavior : public Behavior
{
public:
	FleeBehavior(XMLNode element);
	virtual ~FleeBehavior();
	FleeBehavior(FleeBehavior* behaviorToCopy);

	virtual void Act(Character* actingCharacter) override;
	virtual float CalcUtility(Character* actingCharacter, Tile* tileToActFrom = nullptr) const override;
	virtual std::string GetName() const override;
	virtual void DebugRender(const Character* actingCharacter) const override;

	virtual Behavior* Clone() override;

	Path m_fleePath;
	float m_cowardice = 0.7f;
};