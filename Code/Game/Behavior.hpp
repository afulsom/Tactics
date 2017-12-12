#pragma once
#include "ThirdParty\XMLParser\XMLParser.hpp"
#include <string>

class Character;
class Tile;

class Behavior
{
public:
	Behavior();
	virtual ~Behavior();

	virtual void Act(Character* actingCharacter) = 0;
	virtual float CalcUtility(Character* actingCharacter, Tile* tileToActFrom = nullptr) const;

	virtual void DebugRender(const Character* actingCharacter) const = 0;
	virtual std::string GetName() const = 0;
	virtual Behavior* Clone() = 0;
	static Behavior* Create(XMLNode element);
};