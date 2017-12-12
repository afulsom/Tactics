#include "Game/Inventory.hpp"

Inventory::Inventory()
	: m_items()
{

}

Inventory::~Inventory()
{

}

bool Inventory::IsEmpty() const
{
	return m_items.empty();
}

void Inventory::TransferItemsToOtherInventory(Inventory& destinationInventory)
{
	size_t inventorySize = m_items.size();
	for (size_t itemIndex = 0; itemIndex < inventorySize; itemIndex++)
	{
		destinationInventory.m_items.push_back(m_items[itemIndex]);
	}
	m_items.clear();
}

void Inventory::TransferSingleItemToOtherInventory(int itemIndex, Inventory& destinationInventory)
{
	destinationInventory.AddItem(m_items[itemIndex]);
	m_items.erase(m_items.begin() + itemIndex);
}

void Inventory::AddItem(Item* itemToAdd)
{
	m_items.push_back(itemToAdd);
}

bool Inventory::ContainsItemOfType(std::string itemType)
{
	for (Item* item : m_items)
	{
		if (item->m_definition->m_name == itemType)
			return true;
	}

	return false;
}
