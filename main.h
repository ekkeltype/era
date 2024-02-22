#pragma once
#include <vector>
#include <set>
#include <string>
#include <map>
#include <chrono>
#include <ctime>
#include <set>
enum ItemQuality
{
	Poor,
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

struct Item
{
	std::string name;
	ItemQuality quality;
	unsigned id = 0;
	unsigned slot = 0;
	std::string icon;
	unsigned ilvl = 0;
	std::string slotName;
};

using ItemRef = std::reference_wrapper<const Item>;
struct Player
{
	int id;
	std::string name;
	std::string server;
	std::string region;
	std::set<std::string> gear;
	std::map<std::string, unsigned> parses;
	std::time_t lastUpdate = 0;
	std::string raidTier;
	std::string pclass;
	unsigned avgParse = 0;
};

struct Auction
{
	unsigned auctionId = 0;
	std::string itemName;
	unsigned itemId = 0;
	unsigned quantity = 0;
	unsigned bid = 0;
	unsigned buyout = 0;
	unsigned repeats = 0; // merge multiple identical auctions
	std::string auction_house;
	std::string readable_buyout_per_item;
	std::string timeLeft;
	std::string quality;
};
bool operator<(const Auction& a, const Auction& b);
void processReport();
std::string urlEncode(const std::string& url);

void processRaidReport(const std::string& report);
void processRaidReportFN(const char* report);
bool operator<(const Item& a, const Item& b);
bool operator==(const Item& a, const Item& b);
bool operator!=(const Item& a, const Item& b);
//int main();
std::ostream& operator<<(std::ostream& os, const Item& item);
void getFightCharactersFN(const char* reportFN);
void getFightCharacters(const std::string& report);

std::vector<std::string> getPlayerReportApi(const Player& player);
std::string getProcessOutput(const std::string& cmdline);

std::string getUrlContent(const std::string& url);
std::string getWowApiInfo(unsigned itemId);
unsigned getItemId(const std::string& itemname);
extern std::string api_key;
void get_oauth_key(const std::string& client, const std::string& secret);

ItemQuality get_quality(const std::string& qualstring);
void buildItemDb();
extern std::map<unsigned, Item> wowItemDb;

Item getItemInfo(unsigned itemId);
bool isBis(const Item& item);
unsigned getBlizzardSlotId(const std::string& slotName);
std::string getBlizzardSlot(unsigned slotId);
std::string getFriendlySlotName(const std::string& slot);
void display_item(unsigned itemId);
std::map<std::string, std::set<Auction>> getAHInfo(unsigned realmId, unsigned ahId, bool forceUpdate);
void csvExportAuctions(const std::map<std::string, std::set<Auction>>& auctions, unsigned realmId, unsigned ahId);
std::string readable(unsigned copper);
void find_bargains(const std::map<std::string, std::set<Auction>>& auctions);
std::chrono::minutes getFileAge(const std::string& filename);
void merge_auctions(std::map<std::string, std::set<Auction>>& auctions);
