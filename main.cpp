#include <iostream>
#include <fstream>
#include "json.h"
#include "map"
#include <set>
#include "string"
#include "main.h"

std::string shortify(const std::string& item)
{
  
  static std::map<std::string, std::string> shortnames =
  {
    {"Thunderfury, Blessed Blade of the Windseeker", "TF"},
    {"The Hungering Cold", "THC" },
    {"Gressil, Dawn of Ruin", "Gressil" },
    { "Chromatically Tempered Sword", "CTS" },
    { "Iblis, Blade of the Fallen Seraph", "Iblis" },
    { "Hatchet of Sundered Bone", "Hatchet" },
    { "Crul'shorukh, Edge of Chaos", "Crul" },
    { "Grand Marshal's Longsword", "R14 Longsword" },
    { "Grand Marshal's Swiftblade", "R14 Swiftblade" },
    { "Ancient Qiraji Ripper", "AQR" },
    { "Blessed Qiraji War Axe", "AQ40 Axe" },
    { "Maladath, Runed Blade of the Black Flight", "Maladath" },
    { "Blessed Qiraji Pugio", "Pugio" },
    { "Misplaced Servo Arm", "MSA"},
    { "High Warlord's Cleaver", "R14 Axe" },
    { "Kiss of the Spider", "Kiss" },
    { "Mark of the Champion", "Mark" },
    { "Diamond Flask", "DF" },
    { "Seal of the Dawn", "Seal" },
    { "Drake Fang Talisman", "DFT" },
    { "Badge of the Swarmguard", "Badge" },
    { "Fetish of the Sand Reaver", "Fetish" },
    {  },
    {  },    
    {  },
    {  },    
    {  },
    {  },    
    {  },
    {  },

  };
  if (shortnames.count(item) == 0)
    return item;
  return shortnames[item];

}

using json = nlohmann::json;

struct item_usage
{
  std::string name;
  unsigned count;
};

bool operator>(const item_usage& a, const item_usage& b)
{
  return a.count > b.count;
}
 
std::map<unsigned, std::map<std::string, unsigned>> itemslots;
std::map<unsigned, std::multiset < item_usage, std::greater<item_usage> >> usage_map;

std::set<std::string> thc_main;

typedef std::pair<std::string, std::string> weapon_combo;
typedef std::set<std::string> trinket_combo;

std::map <weapon_combo, unsigned> weapons;
std::map <trinket_combo, unsigned> trinkets;
std::map <std::string, unsigned> item_count;


void processReport(const char * report)
{
  std::ifstream f(report);

  json j;
  f >> j;
  auto arr = j["rankings"];

  for (auto player : j["rankings"])
  {
    //std::cout << player["name"] << ":\n";
    unsigned i = 0;

    trinket_combo trink12;
    weapon_combo mhoh;

    for (auto item : player["gear"])
    {
      auto name = item["name"];

      if(i != 3) // skip shirt
      ++item_count[name];

      ++itemslots[i][name];
      if (i == 15)
      {
        mhoh.first = shortify(name);
        if (mhoh.first == "THC") // THC main hand
        {
          auto playername = std::string(player["name"]) + "-" + std::string(player["serverName"]);
          thc_main.insert(playername);
        }

      }
      else if (i == 16)
        mhoh.second = shortify(name);
      else if (i == 12 || i == 13)
        trink12.insert(shortify(name));

      // std::cout << "\t" << item["name"] << "\n";
      ++i;
    }
    ++trinkets[trink12];
    ++weapons[mhoh];

  }
}

int main_disabled()
{
  const char * reports[] = { 
   // "U:\\downloads\\mage_patchwerk.json",

      "U:\\downloads\\patchwerk.json",
    "U:\\downloads\\thaddius.json",
    "U:\\downloads\\grobbulus.json",
    //"U:\\downloads\\anub.json",
    //"U:\\downloads\\loatheb.json",
    //"U:\\downloads\\noth.json",
    //"U:\\downloads\\heigan.json",
    //"U:\\downloads\\maexxna.json",
    //"U:\\downloads\\kt.json",
  };

  const int numreports = sizeof(reports) / sizeof(char*);

  for (auto report : reports)
  {
    processReport(report);
  }
  for (auto slot : itemslots)
  {

    for (auto item : slot.second)
    {
      item_usage usage;
      usage.name = item.first;
      usage.count = item.second;
      usage_map[slot.first].insert(usage);

    }

  }
  unsigned slotId = 0;

  const char* slots[] = { "head","neck","shoulder","shirt","chest","waist","legs","feet","wrist","hands","ring","ring2","trinket","trinket2","back","main","off","ranged" };

  for (auto slot : usage_map)
  {
    if (slotId == 3 || slotId == 11 || slotId == 13)
    {
      ++slotId;
      continue;
    }
    std::cout << slots[slotId] << ":\n";
    for (auto item : slot.second)
    {
      unsigned count = item.count;
      if (slotId == 10 || slotId == 12)
      {
        count += itemslots[slotId + 1][item.name];
      }
      std::string countstr;
      if (count < numreports)
        countstr = "<1";
      else
        countstr = std::to_string(count / numreports);
      if (countstr.size() == 1)
        countstr = " " + countstr;

      int pad = count / (numreports);
      std::string padding;
      for (int i = item.name.size(); i < 46; ++i)
        padding += " ";
      for(int i = 0; i < pad; ++i)
        padding += "=";

      std::cout << "    " << item.name << " (" << countstr <<  "%)" << padding << "\n";
    }
    ++slotId;
  }

  std::multimap<unsigned, weapon_combo, std::greater<unsigned>> sorted_weapons;
  for (auto kv : weapons)
  {
    sorted_weapons.insert(std::make_pair(kv.second, kv.first));
  }

  std::cout << "\n\nWeapon combos:\n";
  
  for (auto kv : sorted_weapons)
  {
    auto& w = kv.second;
    unsigned count = kv.first;
    if (count < 4)
      continue; // ignore meme stuff

    std::cout << "" << w.first << ", " << w.second << " ( " << count/numreports << "% ) \n";
  }

  std::multimap<unsigned, trinket_combo, std::greater<unsigned>> sorted_trinkets;
  for (auto kv : trinkets)
  {
    sorted_trinkets.insert(std::make_pair(kv.second, kv.first));
  }

  std::cout << "\n\nTrinket combos:\n";

  for (auto kv : sorted_trinkets)
  {
    auto& w = kv.second;
    unsigned count = kv.first;
    if (count < numreports)
      continue; // ignore meme stuff

    std::cout << "" << *w.begin() << ", " << *w.rbegin() << " ( " << count / numreports << "% ) \n";
  }

  std::multimap<unsigned, std::string, std::greater<unsigned>> sorted_items_overall;
  for (auto kv : item_count)
  {
    sorted_items_overall.insert(std::make_pair(kv.second, kv.first));   
  }

  std::cout << "\n\\nMost seen items overall:\n";
  for (auto kv : sorted_items_overall)
  {
    auto& w = kv.second;
    unsigned count = kv.first;
    if (count < numreports)
      continue; // ignore meme stuff

    std::cout << "" << w << " ( " << count / numreports << "% ) \n";

  }
  std::cout << "\n\\THC main users:\n";
  for (auto p : thc_main)
  {
    std::cout << p << "\n";
  }
   
  return 0;
}












