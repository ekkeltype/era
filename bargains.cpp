#include "main.h"
#include "colors.h"
#include <iostream>
#include <fstream>
#include "filesystem"

// item name; threshold price
std::map<std::string, unsigned> searches;

void read_bargain_cfg()
{
  //  std::filesystem::exists("bargain_searcher.cfg");

    std::ifstream f("bargain_searcher.cfg");

    std::string str;
    while (std::getline(f, str))
    {
        // Process str
        int split = str.find(';');
        if (split == str.npos) // invalid config line, skip
            continue;
        std::string item = str.substr(0, split);
        double pricegold = std::stod(str.substr(split + 1));
        searches[item] = unsigned(pricegold * 100 * 100);
    }   
}

void find_bargains(const std::map<std::string, std::set<Auction>>& auctions)
{
    read_bargain_cfg();

    for (auto& entry : auctions)
    {
        const std::string& itemname = entry.first;
        auto find = searches.find(itemname);
        if (find == searches.end())
            continue;

        // found entry for item, check price threshold
//        5 x 1 
 
        for (auto& auction : entry.second)
        {
            unsigned bidPerItem = auction.bid / auction.quantity;
           // unsigned buyoutPerItem = 
            if (bidPerItem <= find->second)
            {
                std::cout << "found bargain: ";
                if (auction.repeats > 0)
                    std::cout << auction.repeats << " x ";
                std::cout << auction.quantity;
                ItemQuality qual = get_quality(auction.quality);
                printColoredString(qual, "[" + auction.itemName + "]");
                std::cout << " listed at " << readable(bidPerItem) << ", " << auction.readable_buyout_per_item
                    << " (" << auction.auction_house << ")" << "\n";
            }
        }
    }

}