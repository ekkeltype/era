#define NOMINMAX 1
#include "json.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include "json.h"
#include "map"
#include <set>
#include <chrono>
#include <format>
#include <type_traits>
#include <thread>

#include "string"
#include "windows.h"
#include "main.h"
#include "colors.h"

using json = nlohmann::json;
const char* slots[] = { "head","neck","shoulder","shirt","chest","waist","legs","feet","wrist","hands","ring","ring2","trinket","trinket2","back","main","off","ranged" };
const std::string unknown("Unknown Item");

//constexpr unsigned num_gear_slots = 18;
constexpr unsigned num_blizzard_gear_slots = 25;

const char * quality[] = { "poor", "common", "uncommon", "rare", "epic", "legendary" };

ItemQuality get_quality(const std::string& qualstring)
{
  for (unsigned i = 0; i < sizeof(quality) / sizeof(const char *); ++i)
  {
    if (quality[i] == qualstring)
      return ItemQuality(i);
  }

  return Common;
}

void print(const Item& item)
{
  auto console = GetStdHandle(STD_OUTPUT_HANDLE);

  WORD color = 0;
  if (item.quality == Uncommon)
    color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
  else if (item.quality == Rare)
    color = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
  else if (item.quality == Epic)
    color = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY;
  else
    color = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE;

  CONSOLE_SCREEN_BUFFER_INFO consoleInfo = {};
  GetConsoleScreenBufferInfo(console, &consoleInfo);
  WORD oldcolor = consoleInfo.wAttributes;
  
  SetConsoleTextAttribute(console, color & oldcolor);
  std::cout << item.name;
  SetConsoleTextAttribute(console, oldcolor);

}

std::ostream& operator<<(std::ostream& os, const Item& item)
{
  if (&os == &std::cout)
    print(item);
  else
    os << item.name;
  return os;

}

bool operator<(const Item& a, const Item& b)
{
  if (a.ilvl != b.ilvl)
    return b.ilvl < a.ilvl;
  return a.id < b.id;
}
bool operator==(const Item& a, const Item& b)
{
    return a.id == b.id && a.name == b.name;
}
bool operator!=(const Item& a, const Item& b)
{
    return !(a==b);
}

std::map<std::string, Item> itemDb;

std::string deduceRaidTier(const std::string& bossName)
{
    // try to deduce raid tier based on encounter name
    if (bossName == "Anub'Rekhan" || bossName == "Patchwerk" || bossName == "Sapphiron"
        || bossName == "Noth the Plaguebringer" || bossName == "Kel'Thuzad")
        return "Naxxramas";
    if (bossName == "The Prophet Skeram" || bossName == "Ouro")
        return "AQ40";
    if (bossName == "Razorgore the Untamed" || bossName == "Vaelastrasz the Corrupt")
        return "Blackwing Lair";
    if (bossName == "Lucifron" || bossName == "Garr" || bossName == "Baron Geddon")
        return "Molten Core";
    if (bossName == "Onyxia")
        return "Onyxia";
    if (bossName == "Kurinnaxx" || bossName == "Moam" || bossName == "Buru the Gorger")
        return "AQ20";
    if (bossName == "Jin'do the Hexxer" || bossName == "Bloodlord Mandokir" || bossName == "Hakkar" || bossName == "High Priest Venoxis")
        return "Zul'Gurub";

    return std::string();
}

void processRaidReport(const std::string& report, Player& player)
{
    json j = json::parse(report.begin(), report.end());

    constexpr int nslots = sizeof(slots);
    //using ItemRef = std::reference_wrapper<const Item>;
    std::set<std::string> gear;// (sizeof(slots) / sizeof(slots[0]));

    for (auto encounter : j)
    {
        if (!encounter.contains("encounterName"))
        {
            std::cout << "skipping invalid encounter for report " << report << "\n";
            continue;
        }
        std::string boss = encounter["encounterName"];

        if (player.pclass.empty())
            player.pclass = encounter["class"];

        std::time_t startTime = encounter["startTime"];
        std::time_t& lastUpdate = player.lastUpdate;
        
        unsigned rank = encounter["rank"];
        unsigned total = encounter["outOf"];

        double parse = 1000*(1 - rank / static_cast<double>(total));
        unsigned& oldParse = player.parses[boss];
        if (parse > oldParse)
            oldParse = parse;
        if (startTime > lastUpdate)
            lastUpdate = startTime;

        unsigned slot = 0;
        for (auto gearItem : encounter["gear"])
        {
            Item item;
            item.name = gearItem["name"];
            if (item.name == unknown)
                continue;

            // look up in item db
            auto find = itemDb.find(item.name);
            if (find == itemDb.end())
            {
                // item not seen before, get info from blizzard api
                item = getItemInfo(gearItem["id"]);

            //    item.id = gearItem["id"];
            //    item.ilvl = std::stoi(std::string(gearItem["itemLevel"]));
                item.icon = gearItem["icon"];
                item.slot = getBlizzardSlotId(item.slotName);

                auto& ref = itemDb[item.name];
                ref = item;
              //  std::cout << "inserting " << item.name << " with slotid " << item.slot << "\n";
                gear.insert(item.name);
            }
            else
            {
                item = find->second;
              //  std::cout << "inserting " << item.name << " with slotid " << item.slot << "\n";
                gear.insert(item.name);
            }

            ++slot;
        }
    }

    player.gear = gear;

    // set player raid tier
    unsigned avgParse = 0;
    unsigned excluded = 0; 
    for (auto& bosskill : player.parses)
    {
        std::string tier = deduceRaidTier(bosskill.first);
        if (!tier.empty())
        {
            player.raidTier = tier;
        }

        if (bosskill.first == "Gothik the Harvester" || bosskill.first == "Viscidus")
        {
            ++excluded;
            continue;
        }   
        
        unsigned parse = bosskill.second;
        avgParse += parse;

    }
    if(player.parses.size() > excluded)
        avgParse = (avgParse * 10) / (player.parses.size() - excluded);

    player.avgParse = avgParse;
    return;
}
void printGear(const Player & player)
{
  unsigned slotMax[num_blizzard_gear_slots] = {};

  double sum = 0.;
  unsigned count = 0;
 

  std::map<unsigned, std::set<Item>> gearBySlot;

  for (auto& itemname  : player.gear)
  {
     const Item& item = itemDb[itemname];
     gearBySlot[item.slot].insert(item);
  }

  for (auto& slot : gearBySlot)
  {
    if (slot.first > 0)
      std::cout << "\n";
    std::cout << slots[slot.first] << ":\n";

    const auto& items = slot.second;

    if (!items.empty() && items.begin()->ilvl != 0)
    {
      ++count;
        sum += items.begin()->ilvl;
    }
    for (auto& item : items)
    {
//      print(item);
      std::cout << item << std::endl;
    }
  }

  double itemValue = sum / count;
  unsigned t = static_cast<unsigned>( itemValue * 100);
  itemValue = t / 100.0;
  return;
}

void notmain();
extern std::vector<Player> characters;

std::vector<std::string> getGuildReports(const std::string& guildName, const std::string& server)
{
    std::string request = "https://vanilla.warcraftlogs.com/v1/reports/guild/" + urlEncode(guildName) + "/" + server + "/EU?api_key=" + api_key;

    std::string reportsJson = getUrlContent(request);

    json j = json::parse(reportsJson.begin(), reportsJson.end());

    std::vector<std::string> result;

    for (auto report : j)
    {
        std::string id = report["id"];
        result.push_back(id);
    }
    return result;
}

std::string getRaidJson(const std::string& reportId)
{
    // check for cached data
    auto cachefile = std::string("cache/encounters/") + reportId + ".json";

    if (std::filesystem::exists(cachefile))
    {
        auto size = std::filesystem::file_size(cachefile);
        std::cout << "Loading cached report " << cachefile << "\n";
        std::ifstream f(cachefile);
        std::string json((std::istreambuf_iterator<char>(f)),
            std::istreambuf_iterator<char>());

        return json;
    }

    std::string request = "https://www.warcraftlogs.com/v1/report/fights/" + reportId + "?translate=true&api_key=" + api_key;
    std::string raidJson = getUrlContent(request);

    // this is static data, cache it for future use
    if (!raidJson.empty())
    {
        std::ofstream f(cachefile);
        f << raidJson;
    }

    return raidJson;
}

std::string csvExport(const Player& player)
{
    // Player;Realm;WCL id;Last updated;Head;Neck;Shoulder;Shirt;Chest;Waist;Legs;Feet;Wrist;Hands;Rings;Rings2;Trinkets;Trinkets2;Back;Main Hand;Off-hand;Ranged

    const std::string delim = ";";
    std::string csv = player.name;
    csv += delim;
    csv += player.server;
    csv += delim;
    csv += std::to_string(player.id);
    csv += delim;
    csv += player.pclass;
    csv += delim;
    
    char buf[32];
    std::time_t t = player.lastUpdate / 1000;
    strftime(buf, 32, "%d.%m.%Y %H:%M:%S", localtime(&t));
    std::string lastSeen(buf);

    csv += lastSeen;

    csv += delim;
    
    // highest raid tier seen:
    csv += player.raidTier;
    csv += delim;
    csv += std::to_string(player.avgParse);
    
    // make gear csv
    std::map<unsigned, std::set<Item>> gearBySlot;

    for (auto& itemname : player.gear)
    {
        const Item& item = itemDb[itemname];
        gearBySlot[item.slot].insert(item);
    }
    bool lastSlotEmpty = false;
    for (unsigned slot = 0; slot < num_blizzard_gear_slots; ++slot)
    {
        if (slot == getBlizzardSlotId("BODY"))
            continue; // skip shirt

        if(!lastSlotEmpty)
            csv += delim;

        const auto& slotItems = gearBySlot[slot];

        for (const auto& item : slotItems)
        {
            csv += item.name;
                csv += ';';
        }
        lastSlotEmpty = slotItems.empty();
    }
    return csv;
}

std::string csv_player_target("playerdata.csv");
std::string csv_item_target("itemdata.csv");

void exportItemData()
{
    std::ofstream out(csv_item_target, std::ios::app);
    const std::string delim = ";";
    // Item; Item Id; Quality; Item level; Slot; wowhead link ; icon name; icon url; [is bis?]

    const std::string imgur = "https://wow.zamimg.com/images/wow/icons/large/";

    for (auto& entry : itemDb)
    {
        auto& item = entry.second;
        std::string itemcsv;
        itemcsv += item.name;
        itemcsv += delim;
        itemcsv += std::to_string(item.id);
        itemcsv += delim;
        itemcsv += quality[item.quality];
        itemcsv += delim;
        itemcsv += std::to_string(item.ilvl);
        itemcsv += delim;
        itemcsv += getFriendlySlotName(item.slotName);
        itemcsv += delim;
        itemcsv += "=hyperlink(\"https://classic.wowhead.com/item=" + std::to_string(item.id)+"\")";
        itemcsv += delim;
        itemcsv += item.icon;
        itemcsv += delim;
        itemcsv += imgur + item.icon;
        itemcsv += delim;
        itemcsv += (isBis(item) ? "1" : "0");
        out << itemcsv << "\n";
    }
}

std::string loadParses(const Player& player)
{
    // age before getting fresh parses from the server
    constexpr std::chrono::hours player_cache_max_age(48);

    // check for cached data
    auto cachefile = std::string("cache/parses/") + std::to_string(player.id) + ".json";

    if (std::filesystem::exists(cachefile))
    {
        auto age = getFileAge(cachefile);
        if (age < player_cache_max_age)
        {
            std::cout << "Loading cached parses for player " << player.name << ", updated " << age << " ago\n";
            std::ifstream f(cachefile);
            std::string cachedJson((std::istreambuf_iterator<char>(f)),
                std::istreambuf_iterator<char>());

            auto test = nlohmann::json::parse(cachedJson,nullptr,false, false);
            if (test.is_discarded())
            {
                std::cout << "Discarding cached parses for player " << player.name << ", invalid data\n";
                    return std::string();
            }

            return cachedJson;
        }
        else
        {
            std::cout << "Discarding cached parses for player " << player.name << ", updated " << age << " ago\n";
        }

    }
    return std::string();

}
void storeParses(const std::string& report, const Player& player)
{
    std::filesystem::create_directory("cache");
    std::filesystem::create_directory("cache/parses");

    std::filesystem::path cachefile(std::string("cache/parses/") + std::to_string(player.id) + ".json");

    if (!report.empty() && report.size() > 300)
    {
        std::ofstream f(cachefile);
        f << report;
        std::cout << "Storing parses in " << std::filesystem::absolute(cachefile) << "\n";
    }

}

std::vector<std::string> getStoredReports()
{
    std::filesystem::directory_iterator it("cache/encounters");
    std::vector<std::string> ret;
    for (auto& cache_item : it)
    {
        auto filename = cache_item.path().filename().string();
        if (filename.size() == 21) // 16 char report id + ".json"
        {
            ret.push_back(filename.substr(0,16));
        }
    }
    return  ret;
}


void find_flips(const std::map<std::string, std::set<Auction>>& auctions)
{
    const double gain_threshold = 2.0; // 200% flip

    for (auto it : auctions)
    {

        auto firstBuyout = it.second.begin();
        while (firstBuyout != it.second.end() && firstBuyout->buyout == 0) // skip auctions with no buyout
        {
            firstBuyout++;
        }
        if (firstBuyout == it.second.end())
            continue;
        Auction cheapest = *firstBuyout;

        double gain = 0.;
        for (auto& auc : it.second)
        {
            if (auc.auction_house != cheapest.auction_house)
            {
                Auction flip = auc;
                gain = flip.buyout / double(cheapest.buyout);
                if (gain >= gain_threshold)
                {
                    std::cout << cheapest.itemName << " seen for " << readable(cheapest.buyout) << " (" << cheapest.auction_house << "), "
                        << readable(flip.buyout) << " (" << flip.auction_house << "), " << int(gain*100) << "% profit\n";
                    break;
                }

            }
        }
    }
}
void read_bargain_cfg();
void find_crafts(const std::map<std::string, std::set<Auction>>& auctions);
void load_oauth();
int price_check_main(const std::string& itemname)
{
    load_oauth();
    unsigned id = getItemId(itemname);
    if (id == 0)
    {
        std::cout << "the item was not found\n";
        return 0;
    }
    Item item = getItemInfo(id);
    

    const unsigned threshold = 40; // how many items to check before aborting search

    std::cout << "searching for: "; 
    printColoredString(item.quality, "[" + itemname + "]");
    std::cout << "\n";
    system("md cache");
    auto horde_auctions = getAHInfo(5233, 6, true); // Horde
    auto ally_auctions = getAHInfo(5233, 2, true); // Alliance
    decltype(horde_auctions) all_auctions(horde_auctions);
    for (auto it : ally_auctions)
    {
        all_auctions[it.first].insert(it.second.begin(), it.second.end());
    }
    std::cout << "merging auctions\n";
    merge_auctions(all_auctions);
    auto find = all_auctions.find(itemname);
    if (find == all_auctions.end())
    {
        std::cout << itemname << " not seen\n";
        return 0;

    }

    // found entry for item, check price threshold
//        5 x 1 

    unsigned totalNumSeen = 0;
    for (auto& auction : find->second)
    {
        unsigned bidPerItem = auction.bid / auction.quantity;
        unsigned buyoutPerItem = auction.buyout / auction.quantity;
        if (auction.repeats > 0)
            std::cout << auction.repeats << " x ";
        std::cout << auction.quantity;
        totalNumSeen += auction.quantity * std::max(auction.repeats, 1u);
        ItemQuality qual = get_quality(auction.quality);
        printColoredString(qual, "[" + auction.itemName + "]");
        std::cout << " listed at " << readable(bidPerItem) << ", " << auction.readable_buyout_per_item
            << " (" << auction.auction_house << ")" << "\n";

        if (totalNumSeen > threshold)
            return 0;
        
    }

    return 0;

}



int ah_main()
{
    //std::cout << "Enter client id:";
    //std::string client; 
    //std::cin >> client;

    //std::cout << "Enter secret:";
    //std::string secret;
    //std::cin >> secret;
    //get_oauth_key(client, secret);

    read_bargain_cfg();
    system("md cache");

    auto horde_auctions = getAHInfo(5233, 6, true); // Horde
    auto ally_auctions = getAHInfo(5233, 2, true); // Alliance

    decltype(horde_auctions) all_auctions(horde_auctions);

    for (auto it : ally_auctions)
    {
        all_auctions[it.first].insert(it.second.begin(), it.second.end());
    }

    merge_auctions(all_auctions);

    // find_flips(all_auctions);

    find_bargains(all_auctions);
    find_crafts(all_auctions);
    csvExportAuctions(all_auctions, 5233, 666);
    system("pause");
    return 0;
}

int main(int argc, char**argv)
{
    time_t szClock = 0;
    time(&szClock);
    tm* newTime = localtime(&szClock);
    std::cout << asctime(newTime) << "\n";
    std::string param(GetCommandLineA());
    
    if (argc > 1)
    {
        param = param.substr(strlen(argv[0]) + 2);
        return price_check_main(param);

    }

    return price_check_main("Large Brilliant Shard");

    return ah_main();
  //  buildItemDb();
  //  return 0;

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE);

    auto reports = getGuildReports("Fun Police", "Noggenfogger"); //  getStoredReports(); 

    constexpr int reportLimit = 10; // limit to only this many reports
    
    for (unsigned int i = 0; i < reportLimit; ++i)
    {
        if (i >= reports.size())
            break;
        std::string raidJson = getRaidJson(reports[i]);
        getFightCharacters(raidJson);
    }

    for (auto& player : characters)
    {
        auto reportUris = getPlayerReportApi(player);
        // grab the report URIs for a player's parses
        std::cout << "Processing " << player.name << "-" << player.server << "\n";
        // stop at the highest-tier raid found for now
        // time will tell if wcl accepts getting all the logs
        for (auto reportUri : reportUris)
        {
            try
            {
                std::string report = loadParses(player);
                if (report.empty())
                {
                    report = getUrlContent(reportUri);
                    storeParses(report, player);
                }
                processRaidReport(report, player);

            }
            catch (const std::exception& e)
            {
                std::cout << e.what() << "\n";
            }

            if (!player.parses.empty())
                break;
        }

        std::string csv = csvExport(player);
        std::ofstream out(csv_player_target, std::ios::app);

        out << csv << std::endl;
    }

    exportItemData();
}