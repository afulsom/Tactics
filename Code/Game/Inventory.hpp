#pragma once
#include "Item.hpp"
#include <vector>

class Inventory
{
public:
	Inventory();
	~Inventory();

	bool IsEmpty() const;
	void TransferItemsToOtherInventory(Inventory& destinationInventory);
	void TransferSingleItemToOtherInventory(int itemIndex, Inventory& destinationInventory);
	void AddItem(Item* itemToAdd);
	bool ContainsItemOfType(std::string itemType);

	std::vector<Item*> m_items;
};