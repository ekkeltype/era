#include <map>
#include <set>
#include <string>
#include <iostream>
#include "colors.h"
#include "main.h"


struct craft
{
	std::string crafted_item;
	// baseline price in gold (if not found on AH)
	int baseline_price = 0; 
	// list of item, quantity pairs from AH
	std::vector<std::pair<std::string, int>> ingredients;
};

// return total price for buying a given amount of an item off the AH
int buy_item(const std::string& name, int quantity, const std::map<std::string, std::set<Auction>>& auctions) 
{
	int total_price = 0;
	auto find = auctions.find(name);
	if (find == auctions.end())
	{
		// total price can't be determined
		return total_price;
	}

	int remaining_quantity = quantity;

	const std::set<Auction>& item_auctions = find->second;
	for (const Auction& a : item_auctions)
	{
		int price_per_item = a.buyout / a.quantity;
		int available = a.quantity * std::max(a.repeats, 1u);
		if (available < remaining_quantity)
		{
			remaining_quantity -= available;
			total_price += available * price_per_item;
		}
		else
		{
			total_price += remaining_quantity * price_per_item;
			remaining_quantity = 0;
			break;
		}
	}
	if (remaining_quantity > 0)
	{
		// not enough reagents on AH
		total_price = 0;
	}
//	std::cout << "    bought " << quantity << " x " << name << " for " << readable(total_price) << "\n";
	return total_price;
}



void evaluate_craft(craft c_item, const std::map<std::string, std::set<Auction>>& auctions)
{
	auto find = auctions.find(c_item.crafted_item);

	// purchase mats for this many crafts
	const int test_amounts[] = { 1, 5, 10 };

	int craft_item_price = c_item.baseline_price * 100 * 100;
	
	ItemQuality qual = {};
	if (find != auctions.end())
	{
		qual = get_quality(find->second.begin()->quality);
		craft_item_price = buy_item(c_item.crafted_item, 1, auctions);	
	}
	else
	{
		std::cout << c_item.crafted_item << " not found on AH, using fallback price\n";
	}
	printColoredString(qual, "[" + c_item.crafted_item + "]");

	std::cout << " AH price: " << readable(craft_item_price) << "\n";

	for (int test_amount : test_amounts)
	{
		int total_price = 0;
		for (auto ingredient : c_item.ingredients)
		{
			int quantity = ingredient.second * test_amount;
			int price_for_this_item = buy_item(ingredient.first, quantity, auctions);
			if (price_for_this_item == 0) // not available on AH, fail
			{
				std::cout << ingredient.first << " x " << quantity << " couldn't be bought on AH, craft failed\n";
				return;
			}
			total_price += price_for_this_item;
		}

		std::cout << " mats price: " << readable(total_price/ test_amount) << " avg (for up to " << test_amount << " items)" <<
			", expected profit: " << readable(craft_item_price * test_amount * 0.95 - total_price ) << "\n";

	}




}

void find_crafts(const std::map<std::string, std::set<Auction>>& auctions)
{
	craft sapper = {
		"Goblin Sapper Charge", 8,
		{ { "Solid Stone", 8}, { "Mithril Bar", 1}, {"Mageweave Cloth", 2} }
	};

	
	craft lip = {
		"Limited Invulnerability Potion", 8,
		{ { "Blindweed", 2}, {"Ghost Mushroom", 1} }
	};

	craft mrp = {
		"Mighty Rage Potion", 8,
		{ { "Gromsblood", 3} }
	};

	craft poisonres = {
	"Elixir of Poison Resistance", 10,
	{ { "Large Venom Sac", 1}, {"Bruiseweed", 1} }
	};

	craft titans = {
		"Flask of the Titans", 175,
		{{"Gromsblood", 30}, {"Black Lotus", 1}, {"Stonescale Eel", 10}}

	};

	craft fap = {
		"Free Action Potion", 3,
		{{"Oily Blackmouth", 4}, {"Stranglekelp",1}}
	};

	craft frostoil = {
		"Frost Oil", 6,
		{{"Khadgar's Whisker", 4}, {"Wintersbite", 2}}

	};

	craft crafts[] = { sapper, lip, fap, mrp, poisonres, frostoil, titans };


	// for test
	//buy_item("Solid Stone", 40, auctions);


	//buy_item("Solid Stone", 80, auctions);

	for (craft c : crafts)
	{
		std::cout << "\n";
		evaluate_craft(c, auctions);
	}
	
}
