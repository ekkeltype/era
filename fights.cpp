#include <fstream>
#include "json.h"
#include <string>
//#include "tinyformat.h"
#include <iostream>
#include "main.h"
using json = nlohmann::json;

std::string api_key = "3b50d9654fcc8dd996386e416362d2dc"; //"cd712e3ea55bc042c1a57ad33bffb04b";//"3b50d9654fcc8dd996386e416362d2dc";

//using tfm::format;
std::set<unsigned> characterIds; // just to ensure unique characters
std::vector<Player> characters;
void getFightCharactersFN(const char* report)
{
    std::ifstream f(report);

}
void getFightCharacters(  const std::string& report)
{
  json j = json::parse(report.begin(), report.end());

  for (auto character : j["exportedCharacters"])
  {
    Player c;
    c.id = character["id"];
    if (characterIds.count(c.id) > 0) // player already seen
        continue;
    
    c.name = character["name"];
    c.server = character["server"];
    c.region = character["region"];
    
    characters.push_back(c);
    characterIds.insert(c.id);
  }

  std::cout << "Report contains " << characters.size() << " characters\n";

  return;
}

std::vector<std::string> getPlayerReportApi(const Player& player)
{
  std::string base_url = "https://vanilla.warcraftlogs.com/v1/parses/character/";
  base_url += urlEncode(player.name);
  base_url += "/";
  base_url += player.server;
  base_url += "/";
//  base_url += 

// zones:
// 2000 MC
// 2001 Ony
// 2002 BWL
// 2003 ZG
// 2004 AQ20
// 2005 AQ40
// 2006 Naxxramas
  std::vector<std::string> result;

  for (int zone = 2006; zone >= 2000; --zone)
  {
      std::string apiuri = base_url + "eu?metric=dps&zone=" + std::to_string(zone) + "&includeCombatantInfo=true&api_key=" + api_key;
//      player.name, player.server, zone, api_key);
    result.push_back(apiuri);

  }


  return result;


}

