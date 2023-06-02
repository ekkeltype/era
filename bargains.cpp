#include "main.h"
#include "colors.h"
#include <iostream>
#include <fstream>
#include "filesystem"
#include <ctime>

// item name; threshold price
std::map<std::string, unsigned> searches;

std::string timestamp()
{
    std::time_t t = 0;
    time(&t);
    std::tm tm_ = {};
    std::tm* p = localtime(&t);

    char timestamp[255] = {};
    unsigned dateLen = strftime(timestamp, sizeof timestamp, "%F %H:%M:%S", p);
    return timestamp;

}

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
    std::string user_profile = std::getenv("userprofile");
    std::filesystem::path outpath(user_profile);
    outpath /= "era";
    std::ofstream out(outpath / "out.html");
    std::string head = R"(<!DOCTYPE html>
      <html>
                <head>
                <style>
                table, th, td {
                    border: 1px solid black;
                    border-collapse: collapse;
                }
                th, td {
                    padding: 5px;
                    text-align: left;
                }

                body {
                    background-color: slategray;
                    color:white;
                }

                </style>
                </head>
                <body>

                <h2>AH report</h2>
                <p>Generated )";

    head += timestamp() + "</p>";

        std::string table_header = R"(
                <table style=""width:85%"">
                  <caption>Items</caption>
                  <tr>
                    <th>Repeated</th>
                    <th>Amount</th>
                    <th>Item</th>
                    <th>Price</th>
                    <th>AH</th>
                  </tr>)";

        std::string table_footer = "</table>\n";
    std::string tail = "</body>\n</html>";
    out << head;

    for (auto& entry : auctions)
    {
        const std::string& itemname = entry.first;
        auto find = searches.find(itemname);
        if (find == searches.end())
            continue;

        // found entry for item, check price threshold
//        5 x 1 
        bool hasTableHeader = false;

        for (auto& auction : entry.second)
        {
            unsigned bidPerItem = auction.bid / auction.quantity;
           // unsigned buyoutPerItem = 

            if (bidPerItem <= find->second)
            {
                if (!hasTableHeader)
                {
                    hasTableHeader = true;
                    out << table_header;
                }

                out << "<tr>";


                std::cout << "found bargain: ";
                if (auction.repeats > 0)
                {
                    std::cout << auction.repeats << " x ";
                    //out << auction.repeats << " x ";
                    out << "<td>" << auction.repeats << "</td>";
                }
                else
                {
                    out << "<td>" << 1 << "</td>";
                }

                std::cout << auction.quantity;
                out << "<td>" << auction.quantity << "</td>";

                ItemQuality qual = get_quality(auction.quality);
                printColoredString(qual, "[" + auction.itemName + "]");
             //   out << "[" << auction.itemName << "]" << " at " << readable(bidPerItem) << "\n";

                /*enum ItemQuality
                {
                    Poor,
                    Common,
                    Uncommon,
                    Rare,
                    Epic,
                    Legendary
                };*/

                std::string colors[] = { "#9d9d9d", "#ffffff", "#1eff00", "#0070dd", "#a335ee", "#ff8000" };

                out << "<td>" << "<font color=" << colors[qual] << ">" << auction.itemName << "</font>" << "</td>";
                out << "<td>" << readable(bidPerItem) << "</td>";



                std::cout << " listed at " << readable(bidPerItem) << ", " << auction.readable_buyout_per_item
                    << " (" << auction.auction_house << ")" << "\n";
                out << "<td>" << auction.auction_house << "</td>";

                out << "</tr>\n";

            }
        }
        if(hasTableHeader)
            out << table_footer;

    }
    out << tail;


}