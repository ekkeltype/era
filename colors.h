#pragma once
#include <string>
#include <iostream>

#include "main.h"
#define NOMINMAX

#include "windows.h"

namespace
{
    void printColoredString(const ItemQuality& quality, const std::string& str)
    {

        auto console = GetStdHandle(STD_OUTPUT_HANDLE);

        WORD color = 0;
        if (quality == Uncommon)
            color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        else if (quality == Rare)
            color = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        else if (quality == Epic)
            color = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY;
        else if (quality == Legendary)
            color = FOREGROUND_RED | FOREGROUND_GREEN;
        else
            color = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE;

        CONSOLE_SCREEN_BUFFER_INFO consoleInfo = {};
        GetConsoleScreenBufferInfo(console, &consoleInfo);
        WORD oldcolor = consoleInfo.wAttributes;

        SetConsoleTextAttribute(console, color & oldcolor);
        std::cout << str;
        SetConsoleTextAttribute(console, oldcolor);

    }

}
