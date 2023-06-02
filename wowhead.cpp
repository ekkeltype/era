#include "main.h"

#include <string>
#include <map>
#include <set>
#include <tuple>
#include <fstream>

#include <iostream>
#include <filesystem>
#include "json.h"
#include "colors.h"
std::map<unsigned, std::string> wowHeadSlots;
using json = nlohmann::json;

std::map<unsigned, Item> wowItemDb;
std::map<std::string, std::string> slotNames;
std::string getWowApiInfo(unsigned itemId);
const char* bslots[] = { "HEAD","NECK","SHOULDER","CLOAK","BODY","TABARD","ROBE","CHEST","WRIST","HAND","WAIST","LEGS","FEET",
"FINGER","TRINKET","WEAPON","WEAPONMAINHAND","WEAPONOFFHAND","SHIELD","HOLDABLE","TWOHWEAPON","RANGED","RANGEDRIGHT","THROWN","RELIC" };

std::string oauth_token = "UShzU3FmpmLrjY7aEjLDAPHC76kAzkbf8M";

bool operator<(const Auction& a, const Auction& b)
{
	unsigned aprice = a.buyout / a.quantity;
	unsigned bprice = b.buyout / b.quantity;

	if (aprice == bprice)
		return a.auctionId < b.auctionId;
	return aprice < bprice;
}

// convert copper to readable amount
std::string readable(unsigned copper)
{
	if (copper < 100)
		return std::to_string(copper) + "c";
	if (copper < 100 * 100)
		return std::to_string(copper / 100) + "s" + readable(copper % 100);
	
	return std::to_string(copper / 10000) + "g" + readable(copper % 10000);
}

std::string getFriendlySlotName(const std::string& slot)
{
	slotNames["RANGEDRIGHT"] = "Ranged";
	return slotNames[slot];
}

unsigned getBlizzardSlotId(const std::string& slotName)
{
	// BODY == shirt, FINGER == ring, CLOAK == back

	if (slotName == "RANGEDRIGHT" || slotName == "THROWN")
		return getBlizzardSlotId("RANGED");
	if (slotName == "ROBE")
		return getBlizzardSlotId("CHEST");
	//if (slotName == "ROBE")
	//	return getBlizzardSlotId("CHEST");


	for (unsigned i = 0; i < sizeof(bslots) / sizeof(char*); ++i)
	{
		if (bslots[i] == slotName)
			return i;
	}
	return -1;

}
std::string getBlizzardSlot(unsigned slotId)
{
	return std::string(bslots[slotId]);
}

Item getItemInfo(unsigned itemId)
{
	Item item = {};
	auto raw = getWowApiInfo(itemId); // get json, either from blizzard api or cache file
	if (raw.empty())
		return item;
	json j = json::parse(raw);
	if (j["code"] == 404) // item doesn't exist
	{
		return item;
	}
	if (!j["is_equippable"])
		return item; // ignore unequippable items

	item.name = j["name"];
	item.ilvl = j["level"];

	std::string qualityString = j["quality"]["name"];
	qualityString[0] = std::tolower(qualityString[0]);
	item.quality = get_quality(qualityString);
	item.slotName = j["inventory_type"]["type"];
	item.id = itemId;
	
	if(item.slotName != "RANGEDRIGHT")
		slotNames[item.slotName] = j["inventory_type"]["name"];

	return item;
}

void buildItemDb()
{
	for (int i = 0; i < 26000;++i)
	{
		Item item = getItemInfo(i);
		if (item == Item())
			continue;

		wowItemDb[i] = item;
	}
	return;
}

bool isBis(const Item& item)
{
	static std::set<std::string> bis = {
		"Lionheart Helm",
		"Helm of Endless Rage",
		"Conqueror's Spaulders",
		"Ghoul Skin Tunic",
		"Shroud of Dominion",
		"Cloak of the Fallen God",
		"Plated Abomination Ribcage",
		"Wristguards of Vengeance",
		"Hive Defiler's Wristguards",
		"Gauntlets of Annihilation",
		"Legplates of Carnage",
		"Band of Unnatural Forces",
		"Quick Strike Ring",
		"Chromatic Boots",
		"Kiss of the Spider",
		"Mark of the Champion",
		"Slayer's Crest",
		"Gressil, Dawn of Ruin",
		"The Hungering Cold",
		"Claw of the Frost Wyrm",
		"Nerubian Slavemaker"
	};

	return bis.contains(item.name);
}


void load_oauth()
{
	std::string oauth_file = "oauth.json";
	if (!std::filesystem::exists(oauth_file))
	{
		return;
	}

	std::ifstream f(oauth_file);
	std::string rawJson((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	auto j = json::parse(rawJson);

	if (j.contains("access_token"))
	{
		oauth_token = j["access_token"];
		std::cout << " successfully updated oauth token to " << oauth_token.substr(0, 5) << "...\n";
	}
}

void get_oauth_key(const std::string& client, const std::string& secret)
{

//	curl -u {client_id}:{client_secret} -d grant_type=client_credentials https://us.battle.net/oauth/token

	std::string cmdline = "curl -s -u " + client + ":" + secret + " -d grant_type=client_credentials https://eu.battle.net/oauth/token";

	std::string rawJson = getProcessOutput(cmdline);

	auto j = json::parse(rawJson);

	if (j.contains("access_token"))
	{
		oauth_token = j["access_token"];
		std::cout << " successfully updated oauth token to " << oauth_token.substr(0, 5) << "...\n";

	}

}



std::string getWowApiInfo(unsigned itemId)
{

//	std::cout << "processing item " << itemId << "... ";
	auto cachefile = std::string("cache/items/") + std::to_string(itemId) + ".json";
	if (std::filesystem::exists(cachefile))
	{
		// cached item data exists
	//	std::cout << "already cached\n";
		std::ifstream f(cachefile);
		std::string json((std::istreambuf_iterator<char>(f)),
			std::istreambuf_iterator<char>());

		// validate
		auto j = nlohmann::json::parse(json.begin(), json.end());
		if (!j.contains("code")) 
		{
			return json;
		}
		// items known to be missing from the blizzard api, ignore them
		static std::set<int> missing_items_in_api = { 7192, 5108 };
		if (missing_items_in_api.contains(itemId))
			return json;

		std::cout << "Code " << j["code"] << " for cached item " << itemId << ", reloading from blizzard api\n";

	}

	std::string uri = "https://us.api.blizzard.com/data/wow/item/" + std::to_string(itemId)
		+ "?namespace=static-classic1x-us&locale=en_US&access_token=" + oauth_token; 

	std::string result = getUrlContent(uri);
	if (result.size() > 5)
	{
		auto j = nlohmann::json::parse(result.begin(), result.end());
		if (j["code"] == 429)
		{
			std::cout << "rate limit exceeded\n";
			return std::string();
		}

		if (j["code"] == 404)
			std::cout << "item does not exist\n";
		else
			std::cout << j["name"] << "\n";

		std::ofstream f(cachefile);
		f << result;
		return result;
	}
	return std::string();

}

std::string trim(const std::string& str)
{
	bool hasFrontQ = (str.front() == '\"');
	bool hasBackQ = (str.back() == '\"');

	auto begin = str.begin() + hasFrontQ;
	auto end = str.begin() + str.size() - hasFrontQ - hasBackQ;

	return std::string(begin, end);

}


void display_item(unsigned itemId)
{
	std::string rawJson = getWowApiInfo(itemId);

	auto j = nlohmann::json::parse(rawJson);
	Item item = getItemInfo(itemId);

	printColoredString(item.quality, j["name"]);
	std::cout << "\n";
	printColoredString(Legendary, "Item level " + std::to_string(item.ilvl));
	std::cout << "\n";
	auto p = j["preview_item"];

	std::cout << trim(p["binding"]["name"]) << "\n";
	
	if (p.contains("unique_equipped"))
		std::cout << trim(p["unique_equipped"]) << "\n";

	std::cout << trim(p["inventory_type"]["name"]) << "\t\t";

	std::string armortype = trim(p["item_subclass"]["name"]);

	if (armortype != "Miscellaneous")
		std::cout << armortype << "\n";
	else
		std::cout << "\n";
	
	if (p.contains("weapon"))
	{
		std::cout << trim(p["weapon"]["damage"]["display_string"]) << "\t\t" 
			<< trim(p["weapon"]["attack_speed"]["display_string"]) << "\n";
		std::cout << trim(p["weapon"]["dps"]["display_string"]) << "\n";
	}

	
	if(p.contains("armor"))
		std::cout << trim(p["armor"]["display"]["display_string"]) << "\n";

	for (auto& stat : p["stats"])
	{
		std::cout << trim(stat["display"]["display_string"]) << "\n";

	}
	if (p.contains("durability"))
		std::cout << trim(p["durability"]["display_string"]) << "\n";

	if (p["requirements"].contains("playable_classes"))
	{
		std::cout << trim(p["requirements"]["playable_classes"]["display_string"]) << "\n";
	}

	std::cout << trim(p["requirements"]["level"]["display_string"]) << "\n";

	for (auto& spell : p["spells"])
	{
		printColoredString(Uncommon, trim(spell["description"]));
		std::cout <<  "\n";
	}

	if (j.contains("description"))
		printColoredString(Legendary, "\"" + std::string(j["description"]) + "\"");
}

void getWowheadInfo(unsigned itemId)
{
//	std::string itemId =

	std::string uri = "https://classic.wowhead.com/item=" + std::to_string(itemId) + "?xml";
	getUrlContent(uri);
}

void csvExportAuctions(const std::map<std::string, std::set<Auction>>& auctions, unsigned realmId, unsigned ahId)
{
	std::string delim = ";";

	std::ofstream out("auctions" + std::to_string(realmId) + "_" + std::to_string(ahId) + ".csv");

	/// Name	q	bid	buyout	per item	time
	// Arcane Crystal	2	85g0s0c	90g0s0c	45g0s0c	SHORT
	std::string csv = "Item;Rarity;Repeating;Quantity;Bid;Buyout;Price/Item;Time remaining;AH;pricesortkey;qualitysortkey\n";

	for (auto& auctionset : auctions)
	{
		for (auto& auction : auctionset.second)
		{
			csv += auction.itemName;
			csv += delim;
			csv += auction.quality;
			csv += delim;
			csv += std::to_string(auction.repeats);
			csv += delim;
			csv += std::to_string(auction.quantity);
			csv += delim;
			csv += readable(auction.bid);
			csv += delim;
			csv += readable(auction.buyout);
			csv += delim;
			csv += auction.readable_buyout_per_item;
			csv += delim;
			csv += auction.timeLeft;
			csv += delim;
			csv += auction.auction_house;
			csv += delim;
			csv += std::to_string(auction.buyout / auction.quantity);
			csv += delim;
			csv += std::to_string(get_quality(auction.quality));
			csv += "\n";
		}
	}
	out << csv;

}


std::map<std::string, std::set<Auction>> getAHInfo(unsigned realmId, unsigned ahId, bool forceUpdate)
{
	load_oauth();
	const std::chrono::minutes max_ah_cache_age(40);

	std::string cachefile = "cache/auctions/" + std::to_string(realmId) + "_" + std::to_string(ahId) + ".json";
	auto age = std::chrono::duration_cast<std::chrono::hours>(getFileAge(cachefile));

	std::map<std::string, std::set<Auction>> auctions;
	std::ifstream f(cachefile);
	std::string cacheJson = std::string((std::istreambuf_iterator<char>(f)),
		std::istreambuf_iterator<char>());
		
	std::string json;
//	if (age.count() > 0 && age < max_ah_cache_age)
	std::cout << "AH " << realmId << "." << ahId << " last seen " << age << " ago\n";

	if (!forceUpdate && age.count() > 0 && age < max_ah_cache_age)
	{
		// using cached data
		json.swap(cacheJson);
	}
	else
	{
		std::string request_uri = "https://eu.api.blizzard.com/data/wow/connected-realm/" + std::to_string(realmId) +
			"/auctions/" + std::to_string(ahId) + "?namespace=dynamic-classic1x-eu&locale=en_US&access_token=" + oauth_token;

		json = getUrlContent(request_uri);
		if (json.empty())
		{
			std::cout << "Update your oauth token file and press enter to continue";
			std::system("pause");

			load_oauth();

			request_uri = "https://eu.api.blizzard.com/data/wow/connected-realm/" + std::to_string(realmId) +
				"/auctions/" + std::to_string(ahId) + "?namespace=dynamic-classic1x-eu&locale=en_US&access_token=" + oauth_token;
			json = getUrlContent(request_uri);
		}


		if (json.size() <= 5)
		{
			return auctions;
		}

		if (json.size() != cacheJson.size())
		{
			std::cout << "AH data differs from cache data\n";
			std::ofstream of(cachefile);
			of << json;
		}
	}

	std::map<unsigned, std::string> itemNames;
	std::map<unsigned, std::string> itemQuality;
	auto j = nlohmann::json::parse(json.begin(), json.end());
	if (j["code"] == 429)
	{
		std::cout << "rate limit exceeded\n";
		return auctions;
	}

	for (auto& jauction : j["auctions"])
	{
		unsigned itemId = jauction["item"]["id"];
		std::string& itemName = itemNames[itemId];
		if (itemName.empty())
		{
			auto rawItemJson = getWowApiInfo(itemId);		
			auto item = nlohmann::json::parse(rawItemJson);
			if (item.contains("code"))
			{
				// wow api lookup failed, item may be deprecated
				itemName = "Unknown Item (" + std::to_string(itemId) + ")";
				itemQuality[itemId] = "poor";
			}
			else
			{
				itemName = item["name"];
				std::string qualityString = item["quality"]["name"];
				qualityString[0] = std::tolower(qualityString[0]);
				itemQuality[itemId] = qualityString;
			}
		}

		Auction auction;
		auction.itemId = itemId;
		auction.itemName = itemName;
		auction.bid = jauction["bid"];
		auction.buyout = jauction["buyout"];
		auction.quantity = jauction["quantity"];
		auction.timeLeft = jauction["time_left"];
		auction.auctionId = jauction["id"];
		auction.auction_house = j["name"];
		auction.quality = itemQuality[itemId];
		auction.readable_buyout_per_item = readable(auction.buyout / auction.quantity);
		auctions[itemName].insert(auction);
	}

	return auctions;
	
}

bool equivalent_auctions(const Auction& a, const Auction& b)
{
	return (a.buyout == b.buyout) && a.quantity == b.quantity && a.timeLeft == b.timeLeft && a.bid == b.bid && a.itemId == b.itemId;
}

struct equiv_less
{
	bool operator()(const Auction& a, const Auction& b) const
	{
		return std::make_tuple(a.itemId, a.bid, a.buyout, a.quantity, a.timeLeft) <
			std::make_tuple(b.itemId, b.bid, b.buyout, b.quantity, b.timeLeft);
	}
};

void merge_auctions(std::map<std::string, std::set<Auction>>& auctions)
{
	for (auto& it : auctions)
	{
		auto& auction_set = it.second;

		if (auction_set.size() == 1)
			continue;

		std::map<Auction, unsigned, equiv_less> equiv_auctions;

		std::vector<Auction> eraselist;

		for (auto& auction : auction_set)
		{
			auto find = equiv_auctions.find(auction);
			if (find != equiv_auctions.end())
			{
				unsigned& count = find->second;
				if(count == 1)
					eraselist.push_back(find->first);
				
				eraselist.push_back(auction);
			}
			++equiv_auctions[auction];
		}

		for (auto a : eraselist)
			auction_set.erase(a);

		for (auto equi : equiv_auctions)
		{
			if (equi.second == 1)
			{
				continue;
			}
			Auction a = equi.first;
			a.repeats = equi.second;
			auction_set.insert(a);
		}
	}
}

